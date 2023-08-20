/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "nand.hpp"

#include <array>
#include <cassert>
#include <cstdio>

#include "intc.hpp"
#include "scheduler.hpp"
#include "../common/file.hpp"

namespace psp::nand {

constexpr i64 NAND_OP_CYCLES = 1 << 16;

constexpr u64 PAGE_SIZE = 512;
constexpr u64 PAGE_SIZE_ECC = PAGE_SIZE + 16;
constexpr u64 BLOCK_SIZE = 32 * PAGE_SIZE_ECC;
constexpr u64 NAND_SIZE  = 2048 * BLOCK_SIZE;

constexpr u32 NAND_ID[2] = {0xEC, 0x35};

enum class NANDReg {
    CONTROL = 0x1D101000,
    STATUS  = 0x1D101004,
    COMMAND = 0x1D101008,
    PAGE  = 0x1D10100C,
    RESET = 0x1D101014,
    DMAPAGE = 0x1D101020,
    DMACTRL = 0x1D101024,
    DMASTATUS = 0x1D101028,
    DMAINTR = 0x1D101038,
    RESUME  = 0x1D101200,
    SERIALDATA = 0x1D101300,
};

enum class NANDCommand {
    READ_SPARE = 0x50,
    BLOCK_ERASE = 0x60,
    READ_STATUS = 0x70,
    READ_ID = 0x90,
    ERASE_CONFIRM = 0xD0,
    RESET = 0xFF,
};

enum class NANDStatus {
    ERASE_ERROR  = 1 << 0,
    DEVICE_READY = 1 << 6,
    NOT_WRITE_PROTECTED = 1 << 7,
};

enum class NANDState {
    IDLE,
    READ_SPARE,
};

enum STATUS {
    NAND_READY = 1 << 0,
};

enum DMACTRL {
    DMA_BUSY = 1 << 0,
    TO_NAND  = 1 << 1,
    PAGE_DATA_EN  = 1 << 8,
    SPARE_DATA_EN = 1 << 9,
};

enum DMAINTR {
    READ_FINISHED  = 1 << 0,
    WRITE_FINISHED = 1 << 1,
};

std::array<u8, NAND_SIZE> nand;
std::array<u8, PAGE_SIZE_ECC> nandBuffer;

// NAND serial data
u8 *serialData;
u32 serialIdx, serialSize;

u32 control, nandpage, dmapage, dmactrl, dmaintr;

u32 deviceStatus = (u32)NANDStatus::NOT_WRITE_PROTECTED | (u32)NANDStatus::DEVICE_READY; // NAND chip status

NANDState state = NANDState::IDLE;

u64 idFinishTransfer, idFinishErase, idUnlockNand;

void setSerialSize(u32 size) {
    serialSize = size;
    serialIdx  = 0;
}

u32 getSerialData() {
    u32 data;
    std::memcpy(&data, &serialData[serialIdx], sizeof(u32));

    serialIdx = (serialIdx + 4) % serialSize;

    return data;
}

void checkInterrupt() {
    if (dmaintr & 3) {
        intc::sendIRQ(intc::InterruptSource::NAND);
    } else {
        intc::clearIRQ(intc::InterruptSource::NAND);
    }
}

void sendIRQ(int type) {
    dmaintr |= 0x300 | type; // All operations appear to set bits 8 and 9

    checkInterrupt();
}

void doDMA(bool toNAND, bool isPageEnabled, bool isSpareEnabled) {
    assert(!toNAND && isPageEnabled && isSpareEnabled);

    std::printf("[NAND    ] DMA transfer from NAND page 0x%X\n", dmapage);

    std::memcpy(nandBuffer.data(), &nand[PAGE_SIZE_ECC * dmapage], PAGE_SIZE_ECC);

    std::puts("NAND buffer is:");
    for (u64 i = 0; i < PAGE_SIZE_ECC; i += 16) {
        std::printf("0x%08X 0x%08X 0x%08X 0x%08X\n", *(u32 *)&nandBuffer[i], *(u32 *)&nandBuffer[i + 4], *(u32 *)&nandBuffer[i + 8], *(u32 *)&nandBuffer[i + 12]);
    }
}

void finishTransfer() {
    doDMA(dmactrl & DMACTRL::TO_NAND, dmactrl & DMACTRL::PAGE_DATA_EN, dmactrl & DMACTRL::SPARE_DATA_EN);

    dmactrl &= ~DMACTRL::DMA_BUSY; // Clear DMA busy bit
    deviceStatus |= (u32)NANDStatus::DEVICE_READY;

    sendIRQ((dmactrl & DMACTRL::TO_NAND) ? DMAINTR::WRITE_FINISHED : DMAINTR::READ_FINISHED); // 1 = Read finished, 2 = Write finished?
}

void finishErase() {
    std::printf("[NAND    ] Erasing block 0x%X\n", nandpage);

    assert(!(nandpage & 0x1F));

    std::memset(&nand[PAGE_SIZE_ECC * nandpage], 0xFF, BLOCK_SIZE);

    deviceStatus |= (u32)NANDStatus::DEVICE_READY;
    deviceStatus &= ~(u32)NANDStatus::ERASE_ERROR;

    sendIRQ(DMAINTR::WRITE_FINISHED); // Hardware sets this to 0x302 after a block erase
}

void startTransfer() {
    deviceStatus &= ~(u32)NANDStatus::DEVICE_READY;

    scheduler::addEvent(idFinishTransfer, 0, NAND_OP_CYCLES);
}

void startErase() {
    deviceStatus &= ~(u32)NANDStatus::DEVICE_READY;

    scheduler::addEvent(idFinishErase, 0, NAND_OP_CYCLES); // Normally takes ~1.7 ms
}

void doCommand(u8 cmd) {
    switch ((NANDCommand)cmd) {
        case NANDCommand::READ_SPARE:
            std::puts("[NAND    ] Read Spare Array");

            state = NANDState::READ_SPARE; // Next write to PAGE triggers a read
            break;
        case NANDCommand::BLOCK_ERASE:
            std::puts("[NAND    ] Block Erase");
            break;
        case NANDCommand::READ_STATUS:
            std::puts("[NAND    ] Read Status");

            serialData = (u8 *)&deviceStatus;
            setSerialSize(4);
            break;
        case NANDCommand::READ_ID:
            std::puts("[NAND    ] Read ID");

            serialData = (u8 *)&NAND_ID;
            setSerialSize(8);
            break;
        case NANDCommand::ERASE_CONFIRM:
            startErase();
            break;
        case NANDCommand::RESET:
            std::puts("[NAND    ] Reset");

            assert(deviceStatus & (u32)NANDStatus::DEVICE_READY);

            deviceStatus = (u32)NANDStatus::NOT_WRITE_PROTECTED | (u32)NANDStatus::DEVICE_READY;
            break;
        default:
            std::printf("Unhandled NAND command 0x%02X\n", cmd);

            exit(0);
    }
}

void unlockNAND() {
    deviceStatus |= (u32)NANDStatus::NOT_WRITE_PROTECTED;
}

// Loads a NAND image
void init(const char *nandPath) {
    std::printf("[NAND    ] Loading NAND image \"%s\"\n", nandPath);
    assert(loadFile(nandPath, nand.data(), NAND_SIZE));

    idFinishTransfer = scheduler::registerEvent([](int) {finishTransfer();});
    idFinishErase = scheduler::registerEvent([](int) {finishErase();});
    idUnlockNand = scheduler::registerEvent([](int) {unlockNAND();});

    std::puts("[NAND    ] OK");
}

u32 read(u32 addr) {
    u32 data;

    switch ((NANDReg)addr) {
        case NANDReg::CONTROL:
            std::puts("[NAND    ] Read @ CONTROL");
            return control;
        case NANDReg::STATUS:
            std::puts("[NAND    ] Read @ STATUS");
            return (deviceStatus & (u32)NANDStatus::NOT_WRITE_PROTECTED) | ((deviceStatus >> 6) & 1);
        case NANDReg::DMACTRL:
            //std::puts("[NAND    ] Read @ DMACTRL");
            return dmactrl;
        case NANDReg::DMASTATUS:
            std::puts("[NAND    ] Read @ DMASTATUS");
            return 0;
        case NANDReg::DMAINTR:
            //std::puts("[NAND    ] Read @ DMAINTR");
            return dmaintr;
        case NANDReg::SERIALDATA:
            std::puts("[NAND    ] Read @ SERIALDATA");
            
            return getSerialData();
        default:
            std::printf("[NAND    ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }

    return data;
}

void write(u32 addr, u32 data) {
    switch ((NANDReg)addr) {
        case NANDReg::CONTROL:
            std::printf("[NAND    ] Write @ CONTROL = 0x%08X\n", data);

            control = data & 0x30103;
            break;
        case NANDReg::STATUS:
            std::printf("[NAND    ] Write @ STATUS = 0x%08X\n", data);

            if (!(deviceStatus & (u32)NANDStatus::NOT_WRITE_PROTECTED) && (data & (u32)NANDStatus::NOT_WRITE_PROTECTED)) {
                scheduler::addEvent(idUnlockNand, 0, 1000);
            }
            break;
        case NANDReg::COMMAND:
            std::printf("[NAND    ] Write @ COMMAND = 0x%08X\n", data);

            doCommand(data);
            break;
        case NANDReg::PAGE:
            std::printf("[NAND    ] Write @ PAGE = 0x%08X\n", data);

            nandpage = (data >> 10) & 0x1FFFF;

            assert(!(data & 0x3FF) && (nandpage < 0x10000));

            switch (state) {
                case NANDState::READ_SPARE:
                    std::printf("[NAND    ] Reading spare array of page 0x%X\n", nandpage);

                    serialData = &nand[PAGE_SIZE_ECC * nandpage + PAGE_SIZE];
                    setSerialSize(16);
                    break;
                default:
                    break;
            }

            state = NANDState::IDLE;
            break;
        case NANDReg::RESET:
            std::printf("[NAND    ] Write @ RESET = 0x%08X\n", data);

            assert(deviceStatus & (u32)NANDStatus::DEVICE_READY);

            deviceStatus = (u32)NANDStatus::NOT_WRITE_PROTECTED | (u32)NANDStatus::DEVICE_READY;
            dmaintr = 0;

            intc::clearIRQ(intc::InterruptSource::NAND);
            break;
        case NANDReg::DMAPAGE:
            std::printf("[NAND    ] Write @ DMAPAGE = 0x%08X\n", data);

            dmapage = (data >> 10) & 0x1FFFF;

            assert(!(data & 0x3FF) && (dmapage < 0x10000));
            break;
        case NANDReg::DMACTRL:
            std::printf("[NAND    ] Write @ DMACTRL = 0x%08X\n", data);

            dmactrl = data;

            if (data & DMA_BUSY) startTransfer();
            break;
        case NANDReg::DMAINTR:
            std::printf("[NAND    ] Write @ DMAINTR = 0x%08X\n", data);

            dmaintr = (data & 0x300) | (dmaintr & (~data & 3)); // [9:8] is R/W, [1:0] is cleared upon writing "1" bits

            checkInterrupt();
            break;
        case NANDReg::RESUME:
            std::printf("[NAND    ] Write @ RESUME = 0x%08X\n", data);
            break;
        default:
            std::printf("[NAND    ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

// Returns DMA buffer data
u32 readBuffer32(u32 addr) {
    u32 data;

    if (addr < 0x1FF00200) {
        std::memcpy(&data, &nandBuffer[addr & (PAGE_SIZE - 1)], sizeof(u32));
    } else {
        switch (addr) {
            case 0x1FF00800:
                std::memcpy(&data, &nandBuffer[PAGE_SIZE + 0x0], sizeof(u32));
                break;
            case 0x1FF00900:
                std::memcpy(&data, &nandBuffer[PAGE_SIZE + 0x4], sizeof(u32));
                break;
            case 0x1FF00904:
                std::memcpy(&data, &nandBuffer[PAGE_SIZE + 0x8], sizeof(u32));
                break;
            case 0x1FF00908:
                std::memcpy(&data, &nandBuffer[PAGE_SIZE + 0xC], sizeof(u32));
                break;
            default:
                std::printf("[NAND    ] Unhandled buffer read32 @ 0x%08X\n", addr);

                exit(0);
        }
    }

    return data;
}

}
