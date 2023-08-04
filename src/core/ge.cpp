/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "ge.hpp"

#include <cassert>
#include <cstdio>

#include "intc.hpp"
#include "scheduler.hpp"

namespace psp::ge {

enum class GEReg {
    CONTROL = 0x1D400100,
    LISTADDR  = 0x1D400108,
    STALLADDR = 0x1D40010C,
    RETADDR1  = 0x1D400110,
    RETADDR2  = 0x1D400114,
    VTXADDR = 0x1D400118,
    IDXADDR = 0x1D40011C,
    ORIGIN0 = 0x1D400120,
    ORIGIN1 = 0x1D400124,
    ORIGIN2 = 0x1D400128,
    IRQSTATUS = 0x1D400308,
    IRQSWAP = 0x1D40030C,
    EDRAMSIZE = 0x1D400400,
};

enum CONTROL {
    RUNNING = 1 << 0,
    BCOND_FALSE = 1 << 1,
    IS_DEPTH_1  = 1 << 8,
    IS_DEPTH_2  = 1 << 9,
};

u32 cmdargs[256];

u32 control;
u32 edramsize;
u32 listaddr, stalladdr;
u32 retaddr[2];
u32 vtxaddr, idxaddr;
u32 origin[3];
u32 irqstatus;

void init() {
}

u32 read(u32 addr) {
    if ((addr >= 0x1D400800) && (addr < 0x1D400C00)) {
        const auto idx = (addr - 0x1D400800) >> 2;

        std::printf("[GE      ] Read @ CMDARGS%u\n", idx);

        return cmdargs[idx];
    }

    switch ((GEReg)addr) {
        case GEReg::CONTROL:
            std::printf("[GE      ] Read @ CONTROL\n");

            return control;
        default:
            std::printf("[GE      ] Unhandled read @ 0x%08X\n", addr);

         exit(0);
    }
}

void write(u32 addr, u32 data) {
    switch ((GEReg)addr) {
        case GEReg::CONTROL:
            std::printf("[GE      ] Write @ CONTROL = 0x%08X\n", data);

            control = (control & ~CONTROL::RUNNING) | (data & CONTROL::RUNNING);

            if (control & CONTROL::RUNNING) {
                control &= ~CONTROL::RUNNING;
            }
            break;
        case GEReg::LISTADDR:
            std::printf("[GE      ] Write @ LISTADDR = 0x%08X\n", data);

            listaddr = data;
            break;
        case GEReg::STALLADDR:
            std::printf("[GE      ] Write @ STALLADDR = 0x%08X\n", data);

            stalladdr = data;
            break;
        case GEReg::RETADDR1:
            std::printf("[GE      ] Write @ RETADDR1 = 0x%08X\n", data);

            retaddr[0] = data;
            break;
        case GEReg::RETADDR2:
            std::printf("[GE      ] Write @ RETADDR2 = 0x%08X\n", data);

            retaddr[1] = data;
            break;
        case GEReg::VTXADDR:
            std::printf("[GE      ] Write @ VTXADDR = 0x%08X\n", data);

            vtxaddr = data;
            break;
        case GEReg::IDXADDR:
            std::printf("[GE      ] Write @ IDXADDR = 0x%08X\n", data);

            idxaddr = data;
            break;
        case GEReg::ORIGIN0:
            std::printf("[GE      ] Write @ ORIGIN0 = 0x%08X\n", data);

            origin[0] = data;
            break;
        case GEReg::ORIGIN1:
            std::printf("[GE      ] Write @ ORIGIN1 = 0x%08X\n", data);

            origin[1] = data;
            break;
        case GEReg::ORIGIN2:
            std::printf("[GE      ] Write @ ORIGIN2 = 0x%08X\n", data);

            origin[2] = data;
            break;
        case GEReg::IRQSTATUS:
            std::printf("[GE      ] Write @ IRQSTATUS = 0x%08X\n", data);

            irqstatus &= ~data;
            break;
        case GEReg::IRQSWAP:
            std::printf("[GE      ] Write @ IRQSWAP = 0x%08X\n", data);
            break;
        case GEReg::EDRAMSIZE:
            std::printf("[GE      ] Write @ EDRAMSIZE = 0x%08X\n", data);

            edramsize = data;
            break;
        default:
            std::printf("[GE      ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
