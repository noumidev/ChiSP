/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "gpio.hpp"

#include <cassert>
#include <cstdio>

namespace psp::gpio {

enum class GPIOReg {
    OUTEN = 0x1E240000,
    INEN  = 0x1E240040,
};

// GPIO pin enable
u32 outen, inen;

u32 read(u32 addr) {
    switch ((GPIOReg)addr) {
        case GPIOReg::OUTEN:
            std::puts("[GPIO    ] Read @ OUTEN");

            return outen;
        case GPIOReg::INEN:
            std::puts("[GPIO    ] Read @ INEN");

            return inen;
        default:
            std::printf("[GPIO    ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    switch ((GPIOReg)addr) {
        case GPIOReg::OUTEN:
            std::printf("[GPIO    ] Write @ OUTEN = 0x%08X\n", data);

            outen = data;
            break;
        case GPIOReg::INEN:
            std::printf("[GPIO    ] Write @ INEN = 0x%08X\n", data);

            inen = data;
            break;
        default:
            std::printf("[GPIO    ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
