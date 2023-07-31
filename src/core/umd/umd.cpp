/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "umd.hpp"

#include <cassert>
#include <cstdio>

#include "../gpio.hpp"
#include "../intc.hpp"

namespace psp::umd {

constexpr i64 UMD_OP_CYCLES = 256;

enum class UMDReg {
    RESET = 0x1DF00008,
    COMMAND  = 0x1DF00010,
    UNKNOWN1 = 0x1DF00018,
    UNKNOWN2 = 0x1DF0001C,
    IRQFLAGS = 0x1DF00020,
    IRQCLEAR = 0x1DF00024,
    IRQEN  = 0x1DF00028,
    IRQDIS = 0x1DF0002C,
    UNKNOWN3 = 0x1DF00030,
    UNKNOWN4 = 0x1DF00038,
};

// UMD regs
u32 reset;
u32 irqen, irqflags;
u32 taddr[10], tsize[10];
u32 unknown[6];

void checkInterrupt() {
    if (irqflags & irqen) {
        intc::sendIRQ(intc::InterruptSource::UMD);
    }
}

void sendIRQ(int irq) {
    irqflags |= irq;

    checkInterrupt();
}

void doCommand(u8 cmd) {
    switch (cmd) {
        case 0x01:
            std::puts("[UMD     ] Command 0x01");

            sendIRQ(1); // Maybe???
            break;
        case 0x02:
            std::puts("[UMD     ] Command 0x02");

            sendIRQ(1); // Maybe???
            break;
        case 0x0B: // No interrupt?
            std::puts("[UMD     ] Command 0x0B");
            break;
        default:
            std::printf("Unhandled UMD command 0x%02X\n", cmd);

            exit(0);
    }
}

void init() {
}

u32 read(u32 addr) {
    switch ((UMDReg)addr) {
        case UMDReg::RESET:
            std::puts("[UMD     ] Read @ RESET");

            return reset;
        case UMDReg::UNKNOWN1:
            std::printf("[UMD     ] Unknown read @ 0x%08X\n", addr);

            return unknown[1];
        case UMDReg::UNKNOWN2:
            std::printf("[UMD     ] Unknown read @ 0x%08X\n", addr);

            return unknown[2];
        case UMDReg::IRQFLAGS:
            std::puts("[UMD     ] Read @ IRQFLAGS");

            return irqen;
        case UMDReg::IRQCLEAR:
            std::puts("[UMD     ] Read @ IRQCLEAR");

            return 0; // What's this supposed to return?
        case UMDReg::IRQEN:
            std::puts("[UMD     ] Read @ IRQEN");

            return irqen;
        case UMDReg::IRQDIS:
            std::puts("[UMD     ] Read @ IRQDIS");

            return 0; // What's this supposed to return?
        case UMDReg::UNKNOWN3:
            std::printf("[UMD     ] Unknown read @ 0x%08X\n", addr);

            return unknown[3];
        case UMDReg::UNKNOWN4:
            std::printf("[UMD     ] Unknown read @ 0x%08X\n", addr);

            return unknown[4];
        default:
            std::printf("[UMD     ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    if ((addr >= 0x1DF00040) && (addr < 0x1DF00090)) {
        const auto idx = ((addr - 0x1DF00040) >> 3) & 0xF;

        if (addr & 4) {
            std::printf("[UMD     ] Write @ TSIZE%u = 0x%08X\n", idx, data);

            tsize[idx] = data;
        } else {
            std::printf("[UMD     ] Write @ TADDR%u = 0x%08X\n", idx, data);

            taddr[idx] = data;
        }

        return;
    }

    switch ((UMDReg)addr) {
        case UMDReg::RESET:
            std::printf("[UMD     ] Write @ RESET = 0x%08X\n", data);

            reset = data;

            if (data & 1) { // TODO: test on hardware
                // The kernel expects the Bluetooth GPIO pin to be high after accessing UMD registers.
                // Set this pin on reset.

                gpio::set(gpio::GPIOPin::BLUETOOTH);
            }
            break;
        case UMDReg::COMMAND:
            std::printf("[UMD     ] Write @ COMMAND = 0x%08X\n", data);

            doCommand(data);
            break;
        case UMDReg::IRQCLEAR:
            std::printf("[UMD     ] Write @ IRQCLEAR = 0x%08X\n", data);

            irqflags &= ~data;
            break;
        case UMDReg::IRQEN:
            std::printf("[UMD     ] Write @ IRQEN = 0x%08X\n", data);

            irqen |= data;
            break;
        case UMDReg::IRQDIS:
            std::printf("[UMD     ] Write @ IRQDIS = 0x%08X\n", data);

            irqen &= ~data;
            break;
        case UMDReg::UNKNOWN3:
            std::printf("[UMD     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[3] = data;
            break;
        case UMDReg::UNKNOWN4:
            std::printf("[UMD     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[4] = data;
            break;
        default:
            std::printf("[UMD     ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
