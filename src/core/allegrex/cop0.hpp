/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../../common/types.hpp"

namespace psp::allegrex::cop0 {

struct COP0 {
    void init(int cpuID);

    u32  getControl(int idx);
    void setControl(int idx, u32 data);

    u32  getStatus(int idx);
    void setStatus(int idx, u32 data);
private:
    // Status
    u32 cpuID;
    u32 count, compare;
    u32 status, cause;
    u32 ebase;
    u32 tagLo, tagHi;

    // Control
    u32 cregs[32];
};

}
