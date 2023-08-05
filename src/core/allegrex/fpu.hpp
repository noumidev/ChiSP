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

    f32  getF32(int idx);
    void setF32(int idx, f32 data);

    void doSingle(u32 instr);
    void doWord(u32 instr);

    bool cpcond;
private:
    int cpuID;

    // Floating-point registers
    u32 fgrs[32];

    // Control
    u32 cregs[32];

    // FPU instructions
    // Single
    void iADD(u32 instr);
    void iC(u32 instr);
    void iDIV(u32 instr);
    void iMOV(u32 instr);
    void iMUL(u32 instr);
    void iSQRT(u32 instr);
    void iSUB(u32 instr);
    void iTRUNCW(u32 instr);

    // Word
    void iCVTS(u32 instr);
};

}
