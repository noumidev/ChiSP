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
    OTHERFLAG0 = 0x1C300004,
    MASK0 = 0x1C300008,
    FLAG1 = 0x1C300010,
    OTHERFLAG1 = 0x1C300014,
    MASK1 = 0x1C300018,
    FLAG2 = 0x1C300020,
    OTHERFLAG2 = 0x1C300024,
    MASK2 = 0x1C300028,
};

u32 flag[2][3], mask[2][3];

void checkInterrupt() {
    setIRQPending((flag[0][0] & mask[0][0]) | (flag[0][1] & mask[0][1]) | (flag[0][2] & mask[0][2]));
}

void meCheckInterrupt() {
    setIRQPending((flag[1][0] & mask[1][0]) | (flag[1][1] & mask[1][1]) | (flag[1][2] & mask[1][2]));
}

u32 read(int cpuID, u32 addr) {
    switch ((INTCRegs)addr) {
        case INTCRegs::FLAG0:
        case INTCRegs::FLAG1:
        case INTCRegs::FLAG2:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Read @ FLAG%u\n", idx);

                return flag[cpuID][idx];
            }
            break;
        case INTCRegs::OTHERFLAG0:
        case INTCRegs::OTHERFLAG1:
        case INTCRegs::OTHERFLAG2:
            {
                const auto idx = (addr >> 4) & 3;

                //std::printf("[INTC    ] Read @ OTHERFLAG%u\n", idx);

                return 0;
            }
        case INTCRegs::MASK0:
        case INTCRegs::MASK1:
        case INTCRegs::MASK2:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Read @ MASK%u\n", idx);

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
        case INTCRegs::FLAG0:
        case INTCRegs::FLAG1:
        case INTCRegs::FLAG2:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Write @ FLAG%u = 0x%08X\n", idx, data);

                flag[cpuID][idx] &= ~data;
            }
            break;
        case INTCRegs::MASK0:
        case INTCRegs::MASK1:
        case INTCRegs::MASK2:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Write @ MASK%u = 0x%08X\n", idx, data);

                mask[cpuID][idx] = data;
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
    u32 *irqFlag, *irqMask;
    if (irqNum < 32) {
        irqFlag = &flag[0][0];
        irqMask = &mask[0][0];
    } else if (irqNum < 64) {
        irqFlag = &flag[0][1];
        irqMask = &mask[0][1];
    } else {
        irqFlag = &flag[0][2];
        irqMask = &mask[0][2];
    }

    // Mask number to 0-31 range
    irqNum &= 0x1F;

    if (*irqMask & (1 << irqNum)) {
        *irqFlag |= 1 << irqNum;

        setIRQPending(true); // Always pending if we get here
    }
}

void meSendIRQ(InterruptSource irqSource) {
    auto irqNum = (u32)irqSource;

    std::printf("[INTC    ] Requesting ME interrupt %u\n", irqNum);

    // Get the right flag/mask regs
    u32 *irqFlag, *irqMask;
    if (irqNum < 32) {
        irqFlag = &flag[1][0];
        irqMask = &mask[1][0];
    } else if (irqNum < 64) {
        irqFlag = &flag[1][1];
        irqMask = &mask[1][1];
    } else {
        irqFlag = &flag[1][2];
        irqMask = &mask[1][2];
    }

    // Mask number to 0-31 range
    irqNum &= 0x1F;

    if (*irqMask & (1 << irqNum)) {
        *irqFlag |= 1 << irqNum;

        meSetIRQPending(true); // Always pending if we get here
    }
}

void clearIRQ(InterruptSource irqSource) {
    auto irqNum = (u32)irqSource;

    std::printf("[INTC    ] Clearing interrupt %u\n", irqNum);

    // Get the right flag reg
    u32 *irqFlag;
    if (irqNum < 32) {
        irqFlag = &flag[0][0];
    } else if (irqNum < 64) {
        irqFlag = &flag[0][1];
    } else {
        irqFlag = &flag[0][2];
    }

    // Mask number to 0-31 range
    irqNum &= 0x1F;

    *irqFlag &= ~(1 << irqNum);

    checkInterrupt();
}

}
