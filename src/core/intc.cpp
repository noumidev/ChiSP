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
    UNMASKEDFLAGS1 = 0x1C300000,
    FLAGS1 = 0x1C300004,
    MASK1  = 0x1C300008,
    UNMASKEDFLAGS2 = 0x1C300010,
    FLAGS2 = 0x1C300014,
    MASK2  = 0x1C300018,
    UNMASKEDFLAGS3 = 0x1C300020,
    FLAGS3 = 0x1C300024,
    MASK3  = 0x1C300028,
};

u32 unmaskedflags[2][3], flags[2][3], mask[2][3];

void checkInterrupt() {
    setIRQPending((unmaskedflags[0][0] & mask[0][0]) | (unmaskedflags[0][1] & mask[0][1]) | (unmaskedflags[0][2] & mask[0][2]));
}

void meCheckInterrupt() {
    meSetIRQPending((unmaskedflags[1][0] & mask[1][0]) | (unmaskedflags[1][1] & mask[1][1]) | (unmaskedflags[1][2] & mask[1][2]));
}

u32 read(int cpuID, u32 addr) {
    switch ((INTCRegs)addr) {
        case INTCRegs::UNMASKEDFLAGS1:
        case INTCRegs::UNMASKEDFLAGS2:
        case INTCRegs::UNMASKEDFLAGS3:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Read @ UNMASKEDFLAGS%u\n", idx + 1);

                return unmaskedflags[cpuID][idx];
            }
            break;
        case INTCRegs::FLAGS1:
        case INTCRegs::FLAGS2:
        case INTCRegs::FLAGS3:
            {
                const auto idx = (addr >> 4) & 3;

                //std::printf("[INTC    ] Read @ FLAGS%u\n", idx + 1);

                return flags[cpuID][idx];
            }
        case INTCRegs::MASK1:
        case INTCRegs::MASK2:
        case INTCRegs::MASK3:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Read @ MASK%u\n", idx + 1);

                return mask[cpuID][idx];
            }
            break;
        default:
            std::printf("[INTC    ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(int cpuID, u32 addr, u32 data) {
    switch ((INTCRegs)addr) {
        case INTCRegs::UNMASKEDFLAGS1:
        case INTCRegs::UNMASKEDFLAGS2:
        case INTCRegs::UNMASKEDFLAGS3:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Write @ UNMASKEDFLAGS%u = 0x%08X\n", idx + 1, data);

                unmaskedflags[cpuID][idx] &= ~data;
                flags[cpuID][idx] &= ~data;
            }
            break;
        case INTCRegs::MASK1:
        case INTCRegs::MASK2:
        case INTCRegs::MASK3:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Write @ MASK%u = 0x%08X\n", idx + 1, data);

                mask[cpuID][idx] = data;

                const auto newFlags = mask[cpuID][idx] & flags[cpuID][idx];
                
                unmaskedflags[cpuID][idx] |= newFlags;
            }
            break;
        default:
            std::printf("[INTC    ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }

    if (cpuID == 0) {
        checkInterrupt();
    } else {
        meCheckInterrupt();
    }
}

void sendIRQ(InterruptSource irqSource) {
    auto irqNum = (u32)irqSource;

    std::printf("[INTC    ] Requesting interrupt %u\n", irqNum);

    // Get the right flag/mask regs
    u32 *irqUnmaskedflags, *irqFlags, *irqMask;
    if (irqNum < 32) {
        irqUnmaskedflags = &unmaskedflags[0][0];
        irqFlags = &flags[0][0];
        irqMask  = &mask [0][0];
    } else if (irqNum < 64) {
        irqUnmaskedflags = &unmaskedflags[0][1];
        irqFlags = &flags[0][1];
        irqMask  = &mask [0][1];
    } else {
        irqUnmaskedflags = &unmaskedflags[0][2];
        irqFlags = &flags[0][2];
        irqMask  = &mask [0][2];
    }

    // Mask number to 0-31 range
    irqNum &= 0x1F;

    *irqFlags |= 1 << irqNum;

    if (*irqMask & (1 << irqNum)) {
        *irqUnmaskedflags |= 1 << irqNum;

        setIRQPending(true); // Always pending if we get here
    }
}

void meSendIRQ(InterruptSource irqSource) {
    auto irqNum = (u32)irqSource;

    std::printf("[INTC    ] Requesting ME interrupt %u\n", irqNum);

    // Get the right flag/mask regs
    u32 *irqUnmaskedflags, *irqFlags, *irqMask;
    if (irqNum < 32) {
        irqUnmaskedflags = &unmaskedflags[1][0];
        irqFlags = &flags[1][0];
        irqMask  = &mask [1][0];
    } else if (irqNum < 64) {
        irqUnmaskedflags = &unmaskedflags[1][1];
        irqFlags = &flags[1][1];
        irqMask  = &mask [1][1];
    } else {
        irqUnmaskedflags = &unmaskedflags[1][2];
        irqFlags = &flags[1][2];
        irqMask  = &mask [1][2];
    }

    // Mask number to 0-31 range
    irqNum &= 0x1F;

    *irqFlags |= 1 << irqNum;

    if (*irqMask & (1 << irqNum)) {
        *irqUnmaskedflags |= 1 << irqNum;

        meSetIRQPending(true); // Always pending if we get here
    }
}

void clearIRQ(InterruptSource irqSource) {
    auto irqNum = (u32)irqSource;

    std::printf("[INTC    ] Clearing interrupt %u\n", irqNum);

    // Get the right flag reg
    u32 *irqUnmaskedflags, *irqFlags;
    if (irqNum < 32) {
        irqUnmaskedflags = &unmaskedflags[0][0];
        irqFlags = &flags[0][0];
    } else if (irqNum < 64) {
        irqUnmaskedflags = &unmaskedflags[0][1];
        irqFlags = &flags[0][1];
    } else {
        irqUnmaskedflags = &unmaskedflags[0][2];
        irqFlags = &flags[0][2];
    }

    // Mask number to 0-31 range
    irqNum &= 0x1F;

    *irqUnmaskedflags &= ~(1 << irqNum);
    *irqFlags &= ~(1 << irqNum);

    checkInterrupt();
}

}
