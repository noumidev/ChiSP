/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "spock.hpp"

#include <cassert>
#include <cstdio>

#include "../gpio.hpp"
#include "../intc.hpp"
#include "../scheduler.hpp"

namespace psp::spock {

constexpr i64 SPOCK_OP_CYCLES = 1 << 10;

enum class SPOCKReg {
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

// SPOCK regs
u32 reset;
u32 irqen, irqflags;
u32 taddr[10], tsize[10];
u32 unknown[6];

u8 cmd; // Current SPOCK command

u64 idFinishCommand;

void checkInterrupt() {
    if (irqflags & irqen) {
        intc::sendIRQ(intc::InterruptSource::UMD);
    } else {
        intc::clearIRQ(intc::InterruptSource::UMD);
    }
}

void sendIRQ(int irq) {
    irqflags |= irq;

    checkInterrupt();
}

void doCommand() {
    switch (cmd) {
        case 0x01:
            std::puts("[SPOCK   ] Command 0x01");
            break;
        case 0x02:
            std::puts("[SPOCK   ] Command 0x02");
            break;
        default:
            std::printf("Unhandled SPOCK command 0x%02X\n", cmd);

            exit(0);
    }
}

void finishCommand() {
    doCommand();
    
    sendIRQ(1);
}

void startCommand() {
    if (cmd == 0x0B) { // Doesn't send an interrupt?
        std::puts("[SPOCK   ] Init");
    } else {
        scheduler::addEvent(idFinishCommand, 0, SPOCK_OP_CYCLES);
    }
}

void init() {
    idFinishCommand = scheduler::registerEvent([](int) {finishCommand();});

    std::puts("[SPOCK   ] OK");
}

u32 read(u32 addr) {
    switch ((SPOCKReg)addr) {
        case SPOCKReg::RESET:
            std::puts("[SPOCK   ] Read @ RESET");

            return reset;
        case SPOCKReg::UNKNOWN1:
            std::printf("[SPOCK   ] Unknown read @ 0x%08X\n", addr);

            return unknown[1];
        case SPOCKReg::UNKNOWN2:
            std::printf("[SPOCK   ] Unknown read @ 0x%08X\n", addr);

            return unknown[2];
        case SPOCKReg::IRQFLAGS:
            std::puts("[SPOCK   ] Read @ IRQFLAGS");

            return irqen;
        case SPOCKReg::IRQCLEAR:
            std::puts("[SPOCK   ] Read @ IRQCLEAR");

            return 0; // What's this supposed to return?
        case SPOCKReg::IRQEN:
            std::puts("[SPOCK   ] Read @ IRQEN");

            return irqen;
        case SPOCKReg::IRQDIS:
            std::puts("[SPOCK   ] Read @ IRQDIS");

            return 0; // What's this supposed to return?
        case SPOCKReg::UNKNOWN3:
            std::printf("[SPOCK   ] Unknown read @ 0x%08X\n", addr);

            return unknown[3];
        case SPOCKReg::UNKNOWN4:
            std::printf("[SPOCK   ] Unknown read @ 0x%08X\n", addr);

            return unknown[4];
        default:
            std::printf("Unhandled SPOCK read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    if ((addr >= 0x1DF00040) && (addr < 0x1DF00090)) {
        const auto idx = ((addr - 0x1DF00040) >> 3) & 0xF;

        if (addr & 4) {
            std::printf("[SPOCK   ] Write @ TSIZE%u = 0x%08X\n", idx, data);

            tsize[idx] = data;
        } else {
            std::printf("[SPOCK   ] Write @ TADDR%u = 0x%08X\n", idx, data);

            taddr[idx] = data;
        }

        return;
    }

    switch ((SPOCKReg)addr) {
        case SPOCKReg::RESET:
            std::printf("[SPOCK   ] Write @ RESET = 0x%08X\n", data);

            reset = data;

            if (data & 1) { // TODO: test on hardware
                // The kernel expects this GPIO pin to go high. On PSP Go, this is also used for Bluetooth?

                gpio::set(gpio::GPIOPin::SPOCK);

                reset &= ~1;
            }
            break;
        case SPOCKReg::COMMAND:
            std::printf("[SPOCK   ] Write @ COMMAND = 0x%08X\n", data);

            cmd = data;

            startCommand();
            break;
        case SPOCKReg::UNKNOWN1:
            std::printf("[SPOCK   ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[1] = data;
            break;
        case SPOCKReg::IRQCLEAR:
            std::printf("[SPOCK   ] Write @ IRQCLEAR = 0x%08X\n", data);

            irqflags &= ~data;

            checkInterrupt();
            break;
        case SPOCKReg::IRQEN:
            std::printf("[SPOCK   ] Write @ IRQEN = 0x%08X\n", data);

            irqen |= data;

            checkInterrupt();
            break;
        case SPOCKReg::IRQDIS:
            std::printf("[SPOCK   ] Write @ IRQDIS = 0x%08X\n", data);

            irqen &= ~data;

            checkInterrupt();
            break;
        case SPOCKReg::UNKNOWN3:
            std::printf("[SPOCK   ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[3] = data;
            break;
        case SPOCKReg::UNKNOWN4:
            std::printf("[SPOCK   ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[4] = data;
            break;
        default:
            std::printf("Unhandled SPOCK write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
