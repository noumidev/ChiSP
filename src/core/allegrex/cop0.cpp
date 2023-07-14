/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "cop0.hpp"

#include <cassert>
#include <cstdio>

namespace psp::allegrex::cop0 {

const char *cop0Name[] = {
    "COP0:CPU", "COP0:ME ",
};

enum Control {
    V0 = 0x04,
};

void COP0::init(int cpuID) {
    assert((cpuID >= 0) && (cpuID < 2));

    cpuid = cpuID;

    std::printf("[%s] OK\n", cop0Name[cpuid]);
}

u32 COP0::getControl(int idx) {
    switch (idx) {
        default:
            std::printf("Unhandled %s control read @ %d\n", cop0Name[cpuid], idx);

            exit(0);
    }
}

void COP0::setControl(int idx, u32 data) {
    switch (idx) {
        case Control::V0:
            v0 = data;
            break;
        default:
            std::printf("Unhandled %s control write @ %d = 0x%08X\n", cop0Name[cpuid], idx, data);

            exit(0);
    }
}

}
