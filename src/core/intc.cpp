/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "intc.hpp"

#include <cassert>
#include <cstdio>

namespace psp::intc {

enum class INTCRegs {
    UNKNOWN0 = 0x1C300000,
    FLAG0 = 0x1C300004,
    MASK0 = 0x1C300008,
    UNKNOWN1 = 0x1C300010,
    FLAG1 = 0x1C300014,
    MASK1 = 0x1C300018,
    UNKNOWN2 = 0x1C300020,
    FLAG2 = 0x1C300024,
    MASK2 = 0x1C300028,
};

u32 flag[3], mask[3];

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
        case INTCRegs::UNKNOWN0:
        case INTCRegs::UNKNOWN1:
        case INTCRegs::UNKNOWN2:
            std::printf("[INTC    ] Unknown read @ 0x%08X\n", addr);
            return 0;
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

                flag[idx] = data;
            }
            break;
        case INTCRegs::MASK0:
        case INTCRegs::MASK1:
        case INTCRegs::MASK2:
            {
                const auto idx = (addr >> 4) & 3;

                std::printf("[INTC    ] Write @ MASK%u = 0x%08X\n", idx, data);

                mask[idx] = data;
            }
            break;
        case INTCRegs::UNKNOWN0:
        case INTCRegs::UNKNOWN1:
        case INTCRegs::UNKNOWN2:
            std::printf("[INTC    ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);
            break;
        default:
            std::printf("[INTC    ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
