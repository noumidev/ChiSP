/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "intc.hpp"

#include <cassert>
#include <cstdio>

#include "psp.hpp"

namespace psp::intc {

enum class INTCRegs {
    FLAG0 = 0x1C300000,
    MASK0 = 0x1C300008,
    FLAG1 = 0x1C300010,
    MASK1 = 0x1C300018,
    FLAG2 = 0x1C300020,
    MASK2 = 0x1C300028,
};

u32 flag[3], mask[3];

void checkInterrupt() {
    setIRQPending((flag[0] & mask[0]) | (flag[1] & mask[1]) | (flag[2] & mask[2]));
}

u32 read(u32 addr) {
    switch ((INTCRegs)addr) {
        case INTCRegs::FLAG0:
        case INTCRegs::FLAG1:
        case INTCRegs::FLAG2:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Read @ FLAG%u\n", idx);

                return flag[idx];
            }
            break;
        case INTCRegs::MASK0:
        case INTCRegs::MASK1:
        case INTCRegs::MASK2:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Read @ MASK%u\n", idx);

                return mask[idx];
            }
            break;
        default:
            std::printf("[INTC    ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    switch ((INTCRegs)addr) {
        case INTCRegs::FLAG0:
        case INTCRegs::FLAG1:
        case INTCRegs::FLAG2:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Write @ FLAG%u = 0x%08X\n", idx, data);

                flag[idx] &= ~data;
            }
            break;
        case INTCRegs::MASK0:
        case INTCRegs::MASK1:
        case INTCRegs::MASK2:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Write @ MASK%u = 0x%08X\n", idx, data);

                mask[idx] = data;
                flag[idx] &= data; // I think this clears FLAG?? Test this!
            }
            break;
        default:
            std::printf("[INTC    ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }

    checkInterrupt();
}

void sendIRQ(InterruptSource irqSource) {
    auto irqNum = (u32)irqSource;

    std::printf("[INTC    ] Requesting interrupt %u\n", irqNum);

    // Get the right flag/mask regs
    u32 *irqFlag, *irqMask;
    if (irqNum < 32) {
        irqFlag = &flag[0];
        irqMask = &mask[0];
    } else if (irqNum < 64) {
        irqFlag = &flag[1];
        irqMask = &mask[1];
    } else {
        irqFlag = &flag[2];
        irqMask = &mask[2];
    }

    // Mask number to 0-31 range
    irqNum &= 0x1F;

    if (*irqMask & (1 << irqNum)) {
        *irqFlag |= 1 << irqNum;

        setIRQPending(true); // Always pending if we get here
    }
}

}
