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

enum class NANDReg {
    STATUS  = 0x1D101004,
    COMMAND = 0x1D101008,
    RESET = 0x1D101014,
    DMAPAGE = 0x1D101020,
    DMACTRL = 0x1D101024,
    DMASTATUS = 0x1D101028,
};

enum class NANDCommand {
    RESET = 0xFF,
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

u32 status, dmapage, dmactrl;

void doCommand(u8 cmd) {
    switch ((NANDCommand)cmd) {
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
}

// Loads a NAND image
void init(const char *nandPath) {
    std::printf("[NAND    ] Loading NAND image \"%s\"\n", nandPath);
    assert(loadFile(nandPath, nand.data(), NAND_SIZE));

    std::puts("[NAND    ] OK");
}

u32 read(u32 addr) {
    switch ((NANDReg)addr) {
        case NANDReg::STATUS:
            std::puts("[NAND    ] Read @ STATUS");
            return status;
        case NANDReg::DMACTRL:
            std::puts("[NAND    ] Read @ DMACTRL");
            return dmactrl;
        case NANDReg::DMASTATUS:
            std::puts("[NAND    ] Read @ DMASTATUS");
            return 0;
        default:
            std::printf("[NAND    ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    switch ((NANDReg)addr) {
        case NANDReg::COMMAND:
            std::printf("[NAND    ] Write @ COMMAND = 0x%08X\n", data);

            doCommand(data);
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
