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

const char *typeName[] = {
    "Allegrex", "MediaEng",
};

void Allegrex::init(Type type) {
    this->type = type;

    if (type == Type::MediaEngine) {
        std::puts("MediaEngine is not supported");

        exit(0);
    }

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

    std::printf("[%s] OK\n", typeName[(int)type]);
}

const char *Allegrex::getTypeName() {
    return typeName[(int)type];
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
        std::printf("%s jumped to NULL\n", typeName[(int)type]);

        exit(0);
    }

    if (addr & 3) {
        std::printf("%s jumped to unaligned address 0x%08X\n", typeName[(int)type], addr);

        exit(0);
    }

    pc = addr;
    npc = addr + 4;
}

void Allegrex::advanceDelay() {
    inDelaySlot[0] = inDelaySlot[1];
    inDelaySlot[1] = false;
}

void Allegrex::advancePC() {
    pc = npc;
    npc += 4;
}

}
