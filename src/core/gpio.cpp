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
    EDGEDETECT  = 0x1E240010,
    FALLINGEDGE = 0x1E240014,
    RISINGEDGE  = 0x1E240018,
    IRQEN  = 0x1E24001C,
    IRQSTATUS = 0x1E240020,
    IRQACK = 0x1E240024,
    CAPTEN = 0x1E240030,
    TIMERCAPTEN = 0x1E240034,
    INEN = 0x1E240040,
    UNKNOWN = 0x1E240048,
};

// GPIO pin enable
u32 outen, inen;

// GPIO interrupts
u32 irqen, irqstatus;

// Clock edge detection
u32 edgedetect, fallingedge, risingedge;

u32 capten, timercapten;

u32 unknown;

u32 read(u32 addr) {
    switch ((GPIOReg)addr) {
        case GPIOReg::OUTEN:
            std::puts("[GPIO    ] Read @ OUTEN");

            return outen;
        case GPIOReg::READ:
            std::puts("[GPIO    ] Read @ READ");

            return 0;
        case GPIOReg::IRQSTATUS:
            std::puts("[GPIO    ] Read @ IRQSTATUS");

            return irqstatus;
        case GPIOReg::INEN:
            std::puts("[GPIO    ] Read @ INEN");

            return inen;
        case GPIOReg::UNKNOWN:
            std::printf("[GPIO    ] Unknown read @ 0x%08X\n", addr);

            return unknown;
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
        case GPIOReg::EDGEDETECT:
            std::printf("[GPIO    ] Write @ EDGEDETECT = 0x%08X\n", data);

            edgedetect = data;
            break;
        case GPIOReg::FALLINGEDGE:
            std::printf("[GPIO    ] Write @ FALLINGEDGE = 0x%08X\n", data);

            fallingedge = data;
            break;
        case GPIOReg::RISINGEDGE:
            std::printf("[GPIO    ] Write @ RISINGEDGE = 0x%08X\n", data);

            risingedge = data;
            break;
        case GPIOReg::IRQEN:
            std::printf("[GPIO    ] Write @ IRQEN = 0x%08X\n", data);

            irqen = data;
            break;
        case GPIOReg::IRQACK:
            std::printf("[GPIO    ] Write @ IRQACK = 0x%08X\n", data);

            irqstatus &= ~data;
            break;
        case GPIOReg::CAPTEN:
            std::printf("[GPIO    ] Write @ CAPTEN = 0x%08X\n", data);

            capten = data;
            break;
        case GPIOReg::TIMERCAPTEN:
            std::printf("[GPIO    ] Write @ TIMERCAPTEN = 0x%08X\n", data);

            timercapten = data;
            break;
        case GPIOReg::INEN:
            std::printf("[GPIO    ] Write @ INEN = 0x%08X\n", data);

            inen = data;
            break;
        case GPIOReg::UNKNOWN:
            std::printf("[GPIO    ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown = data;
            break;
        default:
            std::printf("[GPIO    ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

void sendIRQ(GPIOInterrupt irq) {
    irqstatus |= (u32)irq;
}

}
