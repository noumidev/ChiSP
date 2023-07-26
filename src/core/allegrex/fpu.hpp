/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../../common/types.hpp"

namespace psp::allegrex::fpu {

struct FPU {
    void init(int cpuID);

    u32  getControl(int idx);
    void setControl(int idx, u32 data);

    u32  get(int idx);
    void set(int idx, u32 data);
private:
    int cpuID;

    // Floating-point registers
    u32 fgrs[32];

    // Control
    u32 cregs[32];
};

}
