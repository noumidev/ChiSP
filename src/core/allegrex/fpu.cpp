/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "fpu.hpp"

#include <cassert>
#include <cstdio>

namespace psp::allegrex::fpu {

const char *fpuName[] = {
    "FPU:CPU ", "FPU:ME  ",
};

void FPU::init(int cpuID) {
    assert((cpuID >= 0) && (cpuID < 2));

    this->cpuID = cpuID;

    std::printf("[%s] OK\n", fpuName[cpuID]);
}

u32 FPU::getControl(int idx) {
    return cregs[idx];
}

void FPU::setControl(int idx, u32 data) {
    cregs[idx] = data;
}

u32 FPU::get(int idx) {
    return fgrs[idx];
}

void FPU::set(int idx, u32 data) {
    fgrs[idx] = data;
}

}
