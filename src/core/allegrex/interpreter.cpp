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

enum Reg {
    R0 =  0, AT =  1, V0 =  2, V1 =  3,
    A0 =  4, A1 =  5, A2 =  6, A3 =  7,
    T0 =  8, T1 =  9, T2 = 10, T3 = 11,
    T4 = 12, T5 = 13, T6 = 14, T7 = 15,
    S0 = 16, S1 = 17, S2 = 18, S3 = 19,
    S4 = 20, S5 = 21, S6 = 22, S7 = 23,
    T8 = 24, T9 = 25, K0 = 26, K1 = 27,
    GP = 28, SP = 29, S8 = 30, RA = 31,
    LO = 32, HI = 33,
};

const char *regNames[34] = {
    "R0", "AT", "V0", "V1", "A0", "A1", "A2", "A3",
    "T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7",
    "S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7",
    "T8", "T9", "K0", "K1", "GP", "SP", "S8", "RA",
    "LO", "HI",
};

enum class Opcode {
    LUI  = 0x0F,
    COP0 = 0x10,
    LW = 0x23,
};

enum class COPOpcode {
    CTC = 0x06,
};

u32 cpc; // Current program counter

// Returns primary opcode
u32 getOpcode(u32 instr) {
    return instr >> 26;
}

// Returns secondary opcode
u32 getFunct(u32 instr) {
    return instr & 0x3F;
}

// Returns shift amount
u32 getShamt(u32 instr) {
    return (instr >> 6) & 0x1F;
}

// Returns 16-bit immediate
u32 getImm(u32 instr) {
    return instr & 0xFFFF;
}

// Returns branch offset
u32 getOffset(u32 instr) {
    return instr & 0x3FFFFFF;
}

// Returns Rd
u32 getRd(u32 instr) {
    return (instr >> 11) & 0x1F;
}

// Returns Rs
u32 getRs(u32 instr) {
    return (instr >> 21) & 0x1F;
}

// Returns Rt
u32 getRt(u32 instr) {
    return (instr >> 16) & 0x1F;
}

// move to ConTrol Coprocessor
void iCTC(Allegrex *allegrex, int copN, u32 instr) {
    assert((copN >= 0) && (copN < 4));

    const auto rd = getRd(instr);
    const auto rt = getRt(instr);

    const auto t = allegrex->get(rt);

    switch (copN) {
        case 0:
            allegrex->cop0.setControl(rd, t);
            break;
        default:
            std::printf("Unhandled %s CTC coprocessor %d\n", allegrex->getTypeName(), copN);

            exit(0);
    }

    if (ENABLE_DISASM) {
        std::printf("CTC%d %s, %d; %d = 0x%08X\n", copN, regNames[rt], rd, rd, t);
    }
}

// Load Upper Immediate
void iLUI(Allegrex *allegrex, u32 instr) {
    const auto rt  = getRt(instr);
    const auto imm = getImm(instr);

    allegrex->set(rt, imm << 16);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] LUI %s, 0x%04X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rt], allegrex->get(rt));
    }
}

// Load Word
void iLW(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] LW %s, 0x%X(%s); %s = [0x%08X]\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], regNames[rt], addr);
    }

    if (addr & 3) {
        std::printf("Misaligned %s LW address 0x%08X, PC: 0x%08X\n", allegrex->getTypeName(), addr, cpc);

        exit(0);
    }

    allegrex->set(rt, allegrex->read32(addr));
}

i64 doInstr(Allegrex *allegrex) {
    const auto instr = allegrex->read32(cpc);

    allegrex->advancePC();

    // Get and decode opcode
    const auto opcode = getOpcode(instr);

    switch ((Opcode)opcode) {
        case Opcode::LUI:
            iLUI(allegrex, instr);
            break;
        case Opcode::COP0:
            {
                const auto rs = getRs(instr);

                switch ((COPOpcode)rs) {
                    case COPOpcode::CTC:
                        iCTC(allegrex, 0, instr);
                        break;
                    default:
                        std::printf("Unhandled %s coprocessor instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), rs, instr, cpc);

                        exit(0);
                }
            }
            break;
        case Opcode::LW:
            iLW(allegrex, instr);
            break;
        default:
            std::printf("Unhandled %s instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), opcode, instr, cpc);

            exit(0);
    }

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
