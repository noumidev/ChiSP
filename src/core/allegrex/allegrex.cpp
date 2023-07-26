/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "allegrex.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

#include "../memory.hpp"

namespace psp::allegrex {

constexpr u32 BOOT_EXCEPTION_BASE = 0xBFC00000;

const char *typeNames[] = {
    "Allegrex", "MediaEng",
};

void Allegrex::init(Type type) {
    this->type = type;

    if (type == Type::MediaEngine) {
        std::puts("MediaEngine is not supported");

        exit(0);
    }

    cop0.init((int)type);
    fpu.init((int)type);

    // Clear all GPRs and delay slot helpers
    std::memset(regs, 0, sizeof(regs));

    inDelaySlot[0] = inDelaySlot[1] = false;

    // Set initial PC
    setPC(BOOT_EXCEPTION_BASE);

    // Install read/write handlers
    if (type == Type::MediaEngine) {
        exit(0);
    } else {
        read8  = &memory::read8;
        read16 = &memory::read16;
        read32 = &memory::read32;

        write8  = &memory::write8;
        write16 = &memory::write16;
        write32 = &memory::write32;
    }

    std::printf("[%s] OK\n", typeNames[(int)type]);
}

void Allegrex::reset() {
    // Clear all GPRs and delay slot helpers
    std::memset(regs, 0, sizeof(regs));

    inDelaySlot[0] = inDelaySlot[1] = false;

    // Set initial PC
    setPC(BOOT_EXCEPTION_BASE);

    std::printf("[%s] Reset OK\n", typeNames[(int)type]);
}

const char *Allegrex::getTypeName() {
    return typeNames[(int)type];
}

u32 Allegrex::get(int idx) {
    return regs[idx];
}

void Allegrex::set(int idx, u32 data) {
    regs[idx] = data;
    regs[0] = 0;      // Hardwired to 0
}

u32 Allegrex::getPC() {
    return pc;
}

void Allegrex::setPC(u32 addr) {
    if (!addr) {
        std::printf("%s jumped to NULL\n", typeNames[(int)type]);

        exit(0);
    }

    if (addr & 3) {
        std::printf("%s jumped to unaligned address 0x%08X\n", typeNames[(int)type], addr);

        exit(0);
    }

    pc = addr;
    npc = addr + 4;
}

void Allegrex::setBranchPC(u32 addr) {
    if (!addr) {
        std::printf("%s jumped to NULL\n", typeNames[(int)type]);

        exit(0);
    }

    if (addr & 3) {
        std::printf("%s jumped to unaligned address 0x%08X\n", typeNames[(int)type], addr);

        exit(0);
    }

    if ((addr == (pc - 4)) && (!memory::read32(addr + 4))) {
        std::printf("Infinite loop @ 0x%08X\n", addr);

        exit(0);
    }

    npc = addr;
}

void Allegrex::advanceDelay() {
    inDelaySlot[0] = inDelaySlot[1];
    inDelaySlot[1] = false;
}

void Allegrex::advancePC() {
    pc = npc;
    npc += 4;
}

void Allegrex::doBranch(u32 target, bool cond, int linkReg, bool isLikely) {
    if (inDelaySlot[0]) {
        std::printf("%s branch instruction in delay slot\n", typeNames[(int)type]);

        exit(0);
    }

    set(linkReg, npc);

    inDelaySlot[1] = true;

    if (cond) {
        setBranchPC(target);
    } else if (isLikely) {
        // Skip delay slot
        setPC(npc);

        inDelaySlot[1] = false;
    }
}

void Allegrex::checkInterrupt() {
    if (cop0.isInterruptPending()) {
        raiseException(Exception::Interrupt);
    }
}

void Allegrex::setIRQPending(bool irqPending) {
    cop0.setIRQPending(irqPending);

    checkInterrupt();
}

// Raises exception (Level 1)
void Allegrex::raiseException(Exception excode) {
    std::printf("[%s] Exception 0x%02X @ 0x%08X\n", typeNames[(int)type], (u32)excode, pc);

    isHalted = false;

    cop0.setEXCODE(excode);

    u32 vector;
    if (cop0.isBEV()) {
        vector = 0xBFC00200;
    } else {
        vector = cop0.getEBase();
    }
    
    if (excode == Exception::Interrupt) {
        vector += 0x200;
    } else {
        vector += 0x180;
    }

    advanceDelay();

    // Set exception PC
    if (!cop0.isEXL()) {
        cop0.setBD(inDelaySlot[0]);

        if (inDelaySlot[0]) {
            cop0.setEPC(pc - 4);
        } else {
            cop0.setEPC(pc);
        }
    }

    inDelaySlot[0] = inDelaySlot[1] = false;

    cop0.setEXL(true);

    setPC(vector);
}

void Allegrex::exceptionReturn() {
    // Clear load linked bit
    ll = false;

    setPC(cop0.exceptionReturn());

    std::printf("[%s] Returning from exception, PC: 0x%08X\n", typeNames[(int)type], pc);
}

}
