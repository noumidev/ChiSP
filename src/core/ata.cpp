/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "ata.hpp"

#include <cassert>
#include <cstdio>

namespace psp::ata {

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
    // Command block
    DATA  = 0x1D700000,
    FEATURE = 0x1D700001,
    ERROR = 0x1D700001,
    SECTORCOUNT = 0x1D700002,
    REASON  = 0x1D700002,
    CYLINDERLOW = 0x1D700003,
    BYTECOUNTLOW  = 0x1D700004,
    BYTECOUNTHIGH = 0x1D700005,
    DEVICE  = 0x1D700006,
    COMMAND = 0x1D700007,
    STATUS1 = 0x1D700007,
    // Control block
    DEVCTL  = 0x1D70000E,
    STATUS2 = 0x1D70000E,
};

enum ATAStatus {
    DEVICE_READY = 0x40,
    DEVICE_ERROR = 0x01,
};

// ATA0 regs
u32 ata0Unknown[9];

// ATA1 (ATAPI) regs
u8 features, error;
u8 sectorcount, reason;
u8 cylinderlow;
u8 bytecountlow, bytecounthigh;
u8 device;
u8 command, status;
u8 devctl;

FILE *umd = NULL;

void reset() {
    status = ATAStatus::DEVICE_READY;

    reason = 1;
    cylinderlow = 1;
}

void init(const char *path) {
    std::puts("[ATA     ] OK");

    umd = std::fopen(path, "rb");

    if (umd == NULL) {
        std::puts("[ATA     ] No UMD inserted");
    }

    reset();
}

u32 readATA0(u32 addr) {
    switch ((ATA0Reg)addr) {
        case ATA0Reg::UNKNOWN0:
            std::printf("[ATA     ] Unknown read @ 0x%08X\n", addr);

            return 0x10033; // ?
        case ATA0Reg::UNKNOWN2:
            std::printf("[ATA     ] Unknown read @ 0x%08X\n", addr);

            return 0;
        case ATA0Reg::UNKNOWN5:
            std::printf("[ATA     ] Unknown read @ 0x%08X\n", addr);

            return 0;
        case ATA0Reg::UNKNOWN8:
            std::printf("[ATA     ] Unknown read @ 0x%08X\n", addr);

            return (~ata0Unknown[8]) >> 16;
        default:
            std::printf("Unhandled ATA read @ 0x%08X\n", addr);

            exit(0);
    }
}

void writeATA0(u32 addr, u32 data) {
    switch ((ATA0Reg)addr) {
        case ATA0Reg::UNKNOWN1:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[1] = data;
            break;
        case ATA0Reg::UNKNOWN2:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[2] = data;
            break;
        case ATA0Reg::UNKNOWN3:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[3] = data;
            break;
        case ATA0Reg::UNKNOWN4:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[4] = data;
            break;
        case ATA0Reg::UNKNOWN5:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[5] = data;
            break;
        case ATA0Reg::UNKNOWN8:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[8] = data;
            break;
        default:
            std::printf("Unhandled ATA write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

u8 readATA1(u32 addr) {
    switch ((ATA1Reg)addr) {
        case ATA1Reg::ERROR:
            std::puts("[ATA     ] Read @ ERROR");
        
            return error;
        case ATA1Reg::REASON:
            std::puts("[ATA     ] Read @ REASON");
        
            return reason;
        case ATA1Reg::CYLINDERLOW:
            std::puts("[ATA     ] Read @ CYLINDERLOW");
        
            return cylinderlow;
        case ATA1Reg::BYTECOUNTLOW:
            std::puts("[ATA     ] Read @ BYTECOUNTLOW");
        
            return bytecountlow;
        case ATA1Reg::BYTECOUNTHIGH:
            std::puts("[ATA     ] Read @ BYTECOUNTHIGH");
        
            return bytecounthigh;
        case ATA1Reg::DEVICE:
            std::puts("[ATA     ] Read @ DEVICE");
        
            return device;
        case ATA1Reg::STATUS1:
            reason = 0;
        case ATA1Reg::STATUS2:
            std::puts("[ATA     ] Read @ STATUS");
        
            return status;
        default:
            std::printf("Unhandled ATA read @ 0x%08X\n", addr);

            exit(0);
    }
}

void writeATA1(u32 addr, u8 data) {
    switch ((ATA1Reg)addr) {
        case ATA1Reg::DEVICE:
            std::printf("[ATA     ] Write @ DEVICE = 0x%02X\n", data);
        
            device = data;
            break;
        case ATA1Reg::DEVCTL:
            std::printf("[ATA     ] Write @ DEVCTL = 0x%02X\n", data);
            break;
        default:
            std::printf("Unhandled ATA write @ 0x%08X = 0x%02X\n", addr, data);

            exit(0);
    }
}

bool isUMDInserted() {
    return umd != NULL;
}

}
