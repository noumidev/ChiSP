/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "nand.hpp"

#include <array>
#include <cassert>
#include <cstdio>

#include "../common/file.hpp"

namespace psp::nand {

constexpr u64 PAGE_SIZE = 512;
constexpr u64 PAGE_SIZE_ECC = PAGE_SIZE + 16;
constexpr u64 BLOCK_SIZE = 32 * PAGE_SIZE_ECC;
constexpr u64 NAND_SIZE  = 2048 * BLOCK_SIZE;

constexpr u32 NAND_ID = 0x35EC;

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
    READ2 = 0x50,
    READ_STATUS = 0x70,
    READ_ID = 0x90,
    RESET = 0xFF,
};

enum class NANDStatus {
    ERASE_ERROR  = 1 << 0,
    DEVICE_READY = 1 << 6,
    WRITE_PROTECT = 1 << 7,
};

enum class NANDState {
    IDLE,
    READ_PAGE,
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

std::array<u8, NAND_SIZE> nand;
std::array<u8, PAGE_SIZE_ECC> nandBuffer;

// NAND serial data
std::array<u8, BLOCK_SIZE> serialData;
u32 serialIdx, serialSize;

u32 control, status, nandpage, dmapage, dmactrl, dmaintr;

u8 deviceStatus; // NAND chip status

NANDState state = NANDState::IDLE;

void setSerialSize(u32 size) {
    serialSize = size;
    serialIdx  = 0;
}

u32 getSerialData() {
    const auto data = serialData[serialIdx];

    serialIdx = (serialIdx + 1) % serialSize;

    return data;
}

void doCommand(u8 cmd) {
    switch ((NANDCommand)cmd) {
        case NANDCommand::READ2:
            std::puts("[NAND    ] Read (2)");

            state = NANDState::READ_PAGE; // Next write to PAGE triggers a read
            break;
        case NANDCommand::READ_STATUS:
            std::puts("[NAND    ] Read Status");

            serialData[0] = deviceStatus;
            setSerialSize(1);
            break;
        case NANDCommand::READ_ID:
            std::puts("[NAND    ] Read ID");

            serialData[0] = (u8)NAND_ID;
            serialData[1] = (u8)(NAND_ID >> 8);
            setSerialSize(2);
            break;
        case NANDCommand::RESET:
            std::puts("[NAND    ] Reset");

            status = STATUS::NAND_READY;
            break;
        default:
            std::printf("Unhandled NAND command 0x%02X\n", cmd);

            exit(0);
    }
}

void doDMA(bool toNAND, bool isPageEnabled, bool isSpareEnabled) {
    assert(!toNAND && isPageEnabled && isSpareEnabled);

    std::printf("[NAND    ] DMA transfer from NAND page 0x%X\n", dmapage);

    std::memcpy(nandBuffer.data(), &nand[PAGE_SIZE_ECC * dmapage], PAGE_SIZE_ECC);

    // Clear DMA busy bit
    dmactrl &= ~DMA_BUSY;

    std::puts("NAND buffer is:");
    for (u64 i = 0; i < PAGE_SIZE_ECC; i += 16) {
        std::printf("0x%08X 0x%08X 0x%08X 0x%08X\n", *(u32 *)&nandBuffer[i], *(u32 *)&nandBuffer[i + 4], *(u32 *)&nandBuffer[i + 8], *(u32 *)&nandBuffer[i + 12]);
    }

    dmaintr |= 2;
}

// Algorithm taken from: https://www.psdevwiki.com/psp/NAND_Flash_Memory#ECC_algorithms
u8 getParity(u8 byte) {
    return ((0x6996 >> (byte & 0xF)) ^ (0x6996 >> (byte >> 4))) & 1;
}

// Generates an ECC for the current page in the NAND buffer
// Algorithm taken from: https://www.psdevwiki.com/psp/NAND_Flash_Memory#ECC_algorithm_for_the_page_data
u32 generateECC() {
    u8 output[24];
    std::memset(output, 0, sizeof(output));

    for (int i = 0; i < (int)PAGE_SIZE; i++) {
        const auto byte = nandBuffer[i];

        const auto parity = getParity(byte);

        for (int j = 0; j < 9; j++) {
            if (i & (1 << j)) {
                output[1 + 2 * j] ^= parity;
            } else {
                output[2 * j] ^= parity;
            }

            output[18] ^= ((byte >> 6) ^ (byte >> 4) ^ (byte >> 2) ^ (byte >> 0)) & 1;
            output[19] ^= ((byte >> 7) ^ (byte >> 5) ^ (byte >> 3) ^ (byte >> 1)) & 1;
            output[20] ^= ((byte >> 5) ^ (byte >> 4) ^ (byte >> 1) ^ (byte >> 0)) & 1;
            output[21] ^= ((byte >> 7) ^ (byte >> 6) ^ (byte >> 3) ^ (byte >> 2)) & 1;
            output[22] ^= ((byte >> 3) ^ (byte >> 2) ^ (byte >> 1) ^ (byte >> 0)) & 1;
            output[23] ^= ((byte >> 7) ^ (byte >> 6) ^ (byte >> 5) ^ (byte >> 4)) & 1;
        }
    }

    u32 result = 0;
    for (int i = 0; i < 24; i++) {
        result |= (u32)output[i] << i;
    }

    std::printf("[NAND    ] ECC is: 0x%08X\n", result);

    return result;
}

// Loads a NAND image
void init(const char *nandPath) {
    std::printf("[NAND    ] Loading NAND image \"%s\"\n", nandPath);
    assert(loadFile(nandPath, nand.data(), NAND_SIZE));

    deviceStatus = (u32)NANDStatus::DEVICE_READY;

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
            return status;
        case NANDReg::DMACTRL:
            std::puts("[NAND    ] Read @ DMACTRL");
            return dmactrl;
        case NANDReg::DMASTATUS:
            std::puts("[NAND    ] Read @ DMASTATUS");
            return 0;
        case NANDReg::DMAINTR:
            std::puts("[NAND    ] Read @ DMAINTR");
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

            control = data;
            break;
        case NANDReg::STATUS: // ??
            std::printf("[NAND    ] Write @ STATUS = 0x%08X\n", data);

            status |= data & 0x80; // Write-protect
            break;
        case NANDReg::COMMAND:
            std::printf("[NAND    ] Write @ COMMAND = 0x%08X\n", data);

            doCommand(data);
            break;
        case NANDReg::PAGE:
            std::printf("[NAND    ] Write @ PAGE = 0x%08X\n", data);

            nandpage = (data >> 10) & 0x1FFFF;

            if (state == NANDState::READ_PAGE) {
                std::printf("[NAND    ] Reading page 0x%X\n", nandpage);

                std::memcpy(&serialData[0], &nand[PAGE_SIZE_ECC * nandpage], PAGE_SIZE);
                setSerialSize(PAGE_SIZE);

                state = NANDState::IDLE;
            }
            break;
        case NANDReg::RESET:
            std::printf("[NAND    ] Write @ RESET = 0x%08X\n", data);
            break;
        case NANDReg::DMAPAGE:
            std::printf("[NAND    ] Write @ DMAPAGE = 0x%08X\n", data);

            dmapage = (data >> 10) & 0x1FFFF;
            break;
        case NANDReg::DMACTRL:
            std::printf("[NAND    ] Write @ DMACTRL = 0x%08X\n", data);

            dmactrl = data;

            if (data & DMA_BUSY) doDMA(data & TO_NAND, data & PAGE_DATA_EN, data & SPARE_DATA_EN);
            break;
        case NANDReg::DMAINTR:
            std::printf("[NAND    ] Write @ DMAINTR = 0x%08X\n", data);

            dmaintr = data;
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
            case 0x1FF00800: // ECC
                return generateECC();
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
