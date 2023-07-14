/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "syscon.hpp"

#include <cassert>
#include <cstdio>

namespace psp::syscon {

enum class SysConReg {
    NMIEN = 0x1C100000,
    RESETEN = 0x1C10004C,
    BUSCLKEN  = 0x1C100050,
    GPIOCLKEN = 0x1C100058,
    PLLFREQ = 0x1C100068,
    IOEN = 0x1C100078,
    GPIOEN = 0x1C10007C,
};

// NMI registers
u32 nmien;

// Clock registers
u32 busclken, gpioclken;

// Enable
u32 reseten = -1, ioen, gpioen;

u32 pllfreq;

u32 read(u32 addr) {
    switch ((SysConReg)addr) {
        case SysConReg::NMIEN:
            std::puts("[SysCon  ] Read @ NMIEN");

            return nmien;
        case SysConReg::RESETEN:
            std::puts("[SysCon  ] Read @ RESETEN");

            return reseten;
        case SysConReg::BUSCLKEN:
            std::puts("[SysCon  ] Read @ BUSCLKEN");

            return busclken;
        case SysConReg::GPIOCLKEN:
            std::puts("[SysCon  ] Read @ GPIOCLKEN");

            return gpioclken;
        case SysConReg::PLLFREQ:
            std::puts("[SysCon  ] Read @ PLLFREQ");

            return pllfreq;
        case SysConReg::IOEN:
            std::puts("[SysCon  ] Read @ IOEN");

            return ioen;
        case SysConReg::GPIOEN:
            std::puts("[SysCon  ] Read @ GPIOEN");

            return gpioen;
        default:
            std::printf("[SysCon  ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    switch ((SysConReg)addr) {
        case SysConReg::RESETEN:
            std::printf("[SysCon  ] Write @ RESETEN = 0x%08X\n", data);

            reseten = data;
            break;
        case SysConReg::BUSCLKEN:
            std::printf("[SysCon  ] Write @ BUSCLKEN = 0x%08X\n", data);

            busclken = data;
            break;
        case SysConReg::GPIOCLKEN:
            std::printf("[SysCon  ] Write @ GPIOCLKEN = 0x%08X\n", data);

            gpioclken = data;
            break;
        case SysConReg::IOEN:
            std::printf("[SysCon  ] Write @ IOEN = 0x%08X\n", data);

            ioen = data;
            break;
        case SysConReg::GPIOEN:
            std::printf("[SysCon  ] Write @ GPIOEN = 0x%08X\n", data);

            gpioen = data;
            break;
        default:
            std::printf("[SysCon  ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
