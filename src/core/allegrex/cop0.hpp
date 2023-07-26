/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../../common/types.hpp"

namespace psp::allegrex::cop0 {

enum class Exception {
    Interrupt = 0,
};

struct COP0 {
    void init(int cpuID);

    u32  getControl(int idx);
    void setControl(int idx, u32 data);

    u32  getStatus(int idx);
    void setStatus(int idx, u32 data);

    u32 getEBase();

    void setEPC(u32 pc);

    bool isBEV();

    bool isEXL();
    void setEXL(bool exl);

    bool getIC();
    void setIC(bool ic);

    void setEXCODE(Exception excode);
    void setBD(bool bd);

    bool isInterruptPending();
    void setIRQPending(bool irqPending);

    void exceptionEntry();
    u32 exceptionReturn();
private:
    // Status
    u32 cpuID;
    u32 count, compare;
    u32 status, cause;
    u32 epc;
    u32 ebase;
    u32 tagLo, tagHi;

    // Control
    u32 cregs[32];
};

}
