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
    READ  = 0x1E240004,
    SET = 0x1E240008,
    CLEAR = 0x1E24000C,
    IRQEN = 0x1E24001C,
    INEN  = 0x1E240040,
};

// GPIO pin enable
u32 outen, inen;

// GPIO interrupts
u32 irqen;

u32 read(u32 addr) {
    switch ((GPIOReg)addr) {
        case GPIOReg::OUTEN:
            std::puts("[GPIO    ] Read @ OUTEN");

            return outen;
        case GPIOReg::READ:
            std::puts("[GPIO    ] Read @ READ");

            return 0;
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
        case GPIOReg::SET:
            std::printf("[GPIO    ] Write @ SET = 0x%08X\n", data);
            break;
        case GPIOReg::CLEAR:
            std::printf("[GPIO    ] Write @ CLEAR = 0x%08X\n", data);
            break;
        case GPIOReg::IRQEN:
            std::printf("[GPIO    ] Write @ IRQEN = 0x%08X\n", data);

            irqen = data;
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
