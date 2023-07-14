/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "cop0.hpp"
#include "../../common/types.hpp"

namespace psp::allegrex {

using cop0::COP0;

enum class Type {
    Allegrex,
    MediaEngine,
};

// Allegrex CPU structure
struct Allegrex {
    void init(Type type);

    const char *getTypeName();

    u32  get(int idx);
    void set(int idx, u32 data);

    u32  getPC();
    void setPC(u32 addr);
    void setBranchPC(u32 addr);

    void advanceDelay();
    void advancePC();

    void doBranch(u32 target, bool cond, int linkReg, bool isLikely);

    // Read/write handlers
    u8  (*read8 )(u32);
    u16 (*read16)(u32);
    u32 (*read32)(u32);

    void (*write8 )(u32, u8);
    void (*write16)(u32, u16);
    void (*write32)(u32, u32);

    COP0 cop0;

private:
    Type type; // CPU type

    u32 regs[34];     // 32 GPRs, LO, HI
    u32 pc, npc; // Program counters

    bool inDelaySlot[2]; // Delay slot helper
};

}
