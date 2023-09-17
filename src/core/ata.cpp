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
    FEATURES = 0x1D700001,
    ERROR = 0x1D700001,
    SECTORCOUNT = 0x1D700002,
    LBALOW  = 0x1D700003,
    LBAMID  = 0x1D700004,
    LBAHIGH = 0x1D700005,
    DRIVE   = 0x1D700006,
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
u8 sectorcount;
u8 lbalow, lbamid, lbahigh;
u8 drive;
u8 command, status;
u8 devctl;

FILE *umd = NULL;

void reset() {
    status = ATAStatus::DEVICE_READY;

    // Write the packet device signature
    sectorcount = lbalow = 1;
    lbamid = 0x14;
    lbahigh = 0xEB;
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
        case ATA1Reg::SECTORCOUNT:
            std::puts("[ATA     ] Read @ SECTORCOUNT");
        
            return sectorcount;
        case ATA1Reg::LBALOW:
            std::puts("[ATA     ] Read @ LBALOW");
        
            return lbalow;
        case ATA1Reg::LBAMID:
            std::puts("[ATA     ] Read @ LBAMID");
        
            return lbamid;
        case ATA1Reg::LBAHIGH:
            std::puts("[ATA     ] Read @ LBAHIGH");
        
            return lbahigh;
        case ATA1Reg::DRIVE:
            std::puts("[ATA     ] Read @ DRIVE");
        
            return drive;
        case ATA1Reg::STATUS1:
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
        case ATA1Reg::FEATURES:
            std::printf("[ATA     ] Write @ FEATURES = 0x%02X\n", data);

            features = data;
            break;
        case ATA1Reg::SECTORCOUNT:
            std::printf("[ATA     ] Write @ SECTORCOUNT = 0x%02X\n", data);

            sectorcount = data;
            break;
        case ATA1Reg::LBALOW:
            std::printf("[ATA     ] Write @ LBALOW = 0x%02X\n", data);

            lbalow = data;
            break;
        case ATA1Reg::LBAMID:
            std::printf("[ATA     ] Write @ LBAMID = 0x%02X\n", data);

            lbamid = data;
            break;
        case ATA1Reg::LBAHIGH:
            std::printf("[ATA     ] Write @ LBAHIGH = 0x%02X\n", data);

            lbahigh = data;
            break;
        case ATA1Reg::DRIVE:
            std::printf("[ATA     ] Write @ DRIVE = 0x%02X\n", data);
        
            drive = data;
            break;
        case ATA1Reg::COMMAND:
            std::printf("[ATA     ] Write @ COMMAND = 0x%02X\n", data);

            command = data;
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
