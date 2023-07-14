/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "interpreter.hpp"

#include <cassert>
#include <cstdio>

#include "allegrex.hpp"
#include "../memory.hpp"

namespace psp::allegrex::interpreter {

constexpr auto ENABLE_DISASM = true;

u32 cpc; // Current program counter

// Returns primary opcode
u32 getOpcode(u32 instr) {
    return instr >> 26;
}

i64 doInstr(Allegrex *allegrex) {
    const auto instr = allegrex->read32(cpc);

    allegrex->advancePC();

    // Get and decode opcode
    const auto opcode = getOpcode(instr);

    switch (opcode) {
        default:
            std::printf("Unhandled %s instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), opcode, instr, cpc);

            exit(0);
    }

    assert(!ENABLE_DISASM);

    return 1;
}

void run(Allegrex *allegrex, i64 runCycles) {
    for (i64 i = 0; i < runCycles;) {
        cpc = allegrex->getPC();

        allegrex->advanceDelay();

        i += doInstr(allegrex);
    }
}

}
