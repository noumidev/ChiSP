/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "ata.hpp"

#include <cassert>
#include <cstdio>

namespace psp::umd::ata {

enum class ATA0Reg {
    UNKNOWN0 = 0x1D600000,
    UNKNOWN1 = 0x1D600004,
    UNKNOWN2 = 0x1D600010,
    UNKNOWN3 = 0x1D600014,
    UNKNOWN4 = 0x1D60001C,
    UNKNOWN5 = 0x1D600034,
    UNKNOWN8 = 0x1D600044,
};

enum class ATA1Reg {
    SECTORCOUNT = 0x1D700002,
    STATUS1 = 0x1D700007,
    CONTROL = 0x1D70000E,
    STATUS2 = 0x1D70000E,
};

enum ATAStatus {
    DEVICE_READY = 0x40,
};

// ATA0 regs
u32 ata0Unknown[9];

// ATA1 regs
u8 sectorcount;
u8 status = ATAStatus::DEVICE_READY;

void reset() {
    sectorcount = 1;
}

void init() {
    reset();

    std::puts("[ATA     ] OK");
}

u32 readATA0(u32 addr) {
    switch ((ATA0Reg)addr) {
        case ATA0Reg::UNKNOWN0:
            std::printf("[UMD     ] Unknown read @ 0x%08X\n", addr);

            return 0x10033; // ?
        case ATA0Reg::UNKNOWN2:
            std::printf("[UMD     ] Unknown read @ 0x%08X\n", addr);

            return ata0Unknown[2];
        case ATA0Reg::UNKNOWN5:
            std::printf("[UMD     ] Unknown read @ 0x%08X\n", addr);

            return ata0Unknown[5];
        case ATA0Reg::UNKNOWN8:
            std::printf("[UMD     ] Unknown read @ 0x%08X\n", addr);

            return ata0Unknown[8];
        default:
            std::printf("[ATA     ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void writeATA0(u32 addr, u32 data) {
    switch ((ATA0Reg)addr) {
        case ATA0Reg::UNKNOWN1:
            std::printf("[UMD     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[1] = data;
            break;
        case ATA0Reg::UNKNOWN2:
            std::printf("[UMD     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[2] = data;
            break;
        case ATA0Reg::UNKNOWN3:
            std::printf("[UMD     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[3] = data;
            break;
        case ATA0Reg::UNKNOWN4:
            std::printf("[UMD     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[4] = data;
            break;
        case ATA0Reg::UNKNOWN5:
            std::printf("[UMD     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[5] = data;
            break;
        case ATA0Reg::UNKNOWN8:
            std::printf("[UMD     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[8] = data;
            break;
        default:
            std::printf("[ATA     ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

u8 readATA1(u32 addr) {
    switch ((ATA1Reg)addr) {
        case ATA1Reg::SECTORCOUNT:
            std::puts("[ATA     ] Read @ SECTORCOUNT");
        
            return sectorcount;
        case ATA1Reg::STATUS1:
            std::puts("[ATA     ] Read @ STATUS1");
        
            return status;
        case ATA1Reg::STATUS2:
            std::puts("[ATA     ] Read @ STATUS2");
        
            return status;
        default:
            std::printf("[ATA     ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void writeATA1(u32 addr, u8 data) {
    switch ((ATA1Reg)addr) {
        default:
            std::printf("[ATA     ] Unhandled write @ 0x%08X = 0x%02X\n", addr, data);

            exit(0);
    }
}

}
