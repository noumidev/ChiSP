/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "ddr.hpp"

#include <cassert>
#include <cstdio>

namespace psp::ddr {

u32 unknown[8];

enum class DDRReg {
    FLUSH = 0x1D000004,
    UNKNOWN0 = 0x1D000020,
    UNKNOWN1 = 0x1D000024,
    UNKNOWN2 = 0x1D00002C,
    UNKNOWN3 = 0x1D000030,
    UNKNOWN4 = 0x1D000034,
    UNKNOWN5 = 0x1D000038,
    UNKNOWN6 = 0x1D000040,
    UNKNOWN7 = 0x1D000044,
};

u32 read(u32 addr) {
    switch ((DDRReg)addr) {
        case DDRReg::FLUSH:
            std::puts("[DDR     ] Read @ FLUSH");

            return 0;
        case DDRReg::UNKNOWN0:
            std::printf("[DDR     ] Unknown read @ 0x%08X\n", addr);

            return unknown[0];
        case DDRReg::UNKNOWN2:
            std::printf("[DDR     ] Unknown read @ 0x%08X\n", addr);

            return unknown[2];
        case DDRReg::UNKNOWN3:
            std::printf("[DDR     ] Unknown read @ 0x%08X\n", addr);

            return unknown[3];
        case DDRReg::UNKNOWN6:
            std::printf("[DDR     ] Unknown read @ 0x%08X\n", addr);

            return unknown[6];
        default:
            std::printf("[DDR     ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    switch ((DDRReg)addr) {
        case DDRReg::FLUSH:
            std::printf("[DDR     ] Write @ FLUSH = 0x%08X\n", data);
            break;
        case DDRReg::UNKNOWN0:
            std::printf("[DDR     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[0] = data;
            break;
        case DDRReg::UNKNOWN1:
            std::printf("[DDR     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[1] = data;
            break;
        case DDRReg::UNKNOWN2:
            std::printf("[DDR     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[2] = data;
            break;
        case DDRReg::UNKNOWN3:
            std::printf("[DDR     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[3] = data;
            break;
        case DDRReg::UNKNOWN4:
            std::printf("[DDR     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[4] = data;
            break;
        case DDRReg::UNKNOWN5:
            std::printf("[DDR     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[5] = data;
            break;
        case DDRReg::UNKNOWN6:
            std::printf("[DDR     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[6] = data;
            break;
        case DDRReg::UNKNOWN7:
            std::printf("[DDR     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[7] = data;
            break;
        default:
            std::printf("[DDR     ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
