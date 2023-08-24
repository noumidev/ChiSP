/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "vfpu.hpp"

#include <cassert>
#include <cstdio>

namespace psp::allegrex::vfpu {

constexpr int NUM_VREGS = 128;

u32 vregs[NUM_VREGS];

// Prefix stacks
u32 pfx[3];

// Condition code
u32 cc;

// Internal registers
u32 inf4, rsv5, rsv6;

// Revision
u32 rev;

// PRNG internal registers
u32 rcx[8];

enum {
    VFPU_PFXS = 128,
    VFPU_PFXT = 129,
    VFPU_PFXD = 130,
    VFPU_CC = 131,
    VFPU_INF4 = 132,
    VFPU_RSV5 = 133,
    VFPU_RSV6 = 134,
    VFPU_REV = 135,
    VFPU_RCX0 = 136,
    VFPU_RCX1 = 137,
    VFPU_RCX2 = 138,
    VFPU_RCX3 = 139,
    VFPU_RCX4 = 140,
    VFPU_RCX5 = 141,
    VFPU_RCX6 = 142,
    VFPU_RCX7 = 143,
};

u32 getControl(int idx) {
    if (idx < 128) {
        return vregs[idx];
    }

    if ((idx >= VFPU_RCX0) && (idx <= VFPU_RCX7)) {
        idx -= VFPU_RCX0;

        std::printf("[VFPU    ] Read @ RCX%d\n", idx);

        return rcx[idx];
    }

    switch (idx) {
        case VFPU_PFXS:
            std::puts("[VFPU    ] Read @ PFXS");

            return pfx[0];
        case VFPU_PFXT:
            std::puts("[VFPU    ] Read @ PFXT");

            return pfx[1];
        case VFPU_PFXD:
            std::puts("[VFPU    ] Read @ PFXD");

            return pfx[2];
        case VFPU_CC:
            std::puts("[VFPU    ] Read @ CC");

            return cc;
        case VFPU_INF4:
            std::puts("[VFPU    ] Read @ INF4");

            return inf4;
        case VFPU_RSV5:
            std::puts("[VFPU    ] Read @ RSV5");

            return rsv5;
        case VFPU_RSV6:
            std::puts("[VFPU    ] Read @ RSV6");

            return rsv6;
        case VFPU_REV:
            std::puts("[VFPU    ] Read @ REV");

            return rev;
        default:
            std::printf("Unhandled VFPU control read @ %d\n", idx);

            exit(0);
    }
}

void readMtxQuadword(int vt, u32 *data) {
    for (int i = 0; i < 4; i++) {
        data[i] = vregs[(vt + 32 * i) & (NUM_VREGS - 1)];
    }
}

}
