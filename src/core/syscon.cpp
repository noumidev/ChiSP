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
};

// NMI registers
u32 nmien;

u32 read(u32 addr) {
    switch ((SysConReg)addr) {
        case SysConReg::NMIEN:
            std::puts("[SysCon  ] Read @ NMIEN");

            return nmien;
        default:
            std::printf("[SysCon  ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

}
