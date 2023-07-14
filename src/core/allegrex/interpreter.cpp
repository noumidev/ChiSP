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
    SPECIAL = 0x00,
    JAL  = 0x03,
    BNE  = 0x05,
    ADDIU = 0x09,
    LUI  = 0x0F,
    COP0 = 0x10,
    SPECIAL3 = 0x1F,
    LW = 0x23,
    SW = 0x2B,
    CACHE = 0x2F,
};

enum class SPECIAL {
    SLL  = 0x00,
    SLLV = 0x04,
    JR = 0x08,
    ADDU = 0x21,
};

enum class SPECIAL3 {
    EXT = 0x00,
};

enum class COPOpcode {
    MFC = 0x00,
    MTC = 0x04,
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

// ADD Immediate Unsigned
void iADDIU(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (u32)(i16)getImm(instr);

    allegrex->set(rt, allegrex->get(rs) + imm);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] ADDIU %s, %s, 0x%X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rs], imm, regNames[rt], allegrex->get(rt));
    }
}

// ADD Unsigned
void iADDU(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, allegrex->get(rs) + allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] ADDU %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// Branch if Not Equal
void iBNE(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);
    const auto t = allegrex->get(rt);

    allegrex->doBranch(target, s != t, Reg::R0, false);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BNE %s, %s, 0x%08X; %s = 0x%08X, %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], regNames[rt], target, regNames[rs], s, regNames[rt], t);
    }
}

// CACHE
void iCACHE(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] CACHE 0x%X, 0x%X(%s)\n", allegrex->getTypeName(), cpc, rt, imm, regNames[rs]);
    }
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
        std::printf("[%s] [0x%08X] CTC%d %s, %d; %d = 0x%08X\n", allegrex->getTypeName(), cpc, copN, regNames[rt], rd, rd, t);
    }
}

// Extract
void iEXT(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto pos  = getShamt(instr);
    const auto size = getRd(instr) + 1;

    assert(size && ((pos + size) < 32));

    // Shift rs by pos, then mask with (1 << size) - 1 (mask is all 1s)
    allegrex->set(rt, (allegrex->get(rs) >> pos) & ((1llu << size) - 1));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] EXT %s, %s, %u, %u; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rs], pos, size, regNames[rt], allegrex->get(rt));
    }
}

// Jump And Link
void iJAL(Allegrex *allegrex, u32 instr) {
    const auto target = (allegrex->getPC() & 0xF0000000) | (getOffset(instr) << 2);

    allegrex->doBranch(target, true, Reg::RA, false);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] JAL 0x%08X; RA = 0x%08X, PC = 0x%08X\n", allegrex->getTypeName(), cpc, target, allegrex->get(Reg::RA), target);
    }
}

// Jump Register
void iJR(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);

    const auto target = allegrex->get(rs);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] JR %s; PC = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target);
    }

    allegrex->doBranch(target, true, Reg::R0, false);
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

// Move From Coprocessor
void iMFC(Allegrex *allegrex, int copN, u32 instr) {
    assert((copN >= 0) && (copN < 4));

    const auto rd = getRd(instr);
    const auto rt = getRt(instr);

    u32 data;

    switch (copN) {
        case 0:
            data = allegrex->cop0.getStatus(rd);
            break;
        default:
            std::printf("Unhandled %s MFC coprocessor %d\n", allegrex->getTypeName(), copN);

            exit(0);
    }

    allegrex->set(rt, data);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MFC%d %s, %d; %s = 0x%08X\n", allegrex->getTypeName(), cpc, copN, regNames[rt], rd, regNames[rt], data);
    }
}

// Move To Coprocessor
void iMTC(Allegrex *allegrex, int copN, u32 instr) {
    assert((copN >= 0) && (copN < 4));

    const auto rd = getRd(instr);
    const auto rt = getRt(instr);

    const auto t = allegrex->get(rt);

    switch (copN) {
        case 0:
            allegrex->cop0.setStatus(rd, t);
            break;
        default:
            std::printf("Unhandled %s MTC coprocessor %d\n", allegrex->getTypeName(), copN);

            exit(0);
    }

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MTC%d %s, %d; %d = 0x%08X\n", allegrex->getTypeName(), cpc, copN, regNames[rt], rd, rd, t);
    }
}

// Shift Left Logical
void iSLL(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rt = getRt(instr);
    const auto shamt = getShamt(instr);

    allegrex->set(rd, allegrex->get(rt) << shamt);

    if (ENABLE_DISASM) {
        if (!instr) {
            std::printf("[%s] [0x%08X] NOP\n", allegrex->getTypeName(), cpc);
        } else {
            std::printf("[%s] [0x%08X] SLL %s, %s, %u; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rt], shamt, regNames[rd], allegrex->get(rd));
        }
    }
}

// Shift Left Logical Variable
void iSLLV(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, allegrex->get(rt) << (allegrex->get(rs) & 0x1F));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SLLV %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rt], regNames[rs], regNames[rd], allegrex->get(rd));
    }
}

// Store Word
void iSW(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;
    const auto data = allegrex->get(rt);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SW %s, 0x%X(%s); [0x%08X] = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], addr, data);
    }

    if (addr & 3) {
        std::printf("Misaligned %s SW address 0x%08X, PC: 0x%08X\n", allegrex->getTypeName(), addr, cpc);

        exit(0);
    }

    allegrex->write32(addr, data);
}

i64 doInstr(Allegrex *allegrex) {
    const auto instr = allegrex->read32(cpc);

    allegrex->advancePC();

    // Get and decode opcode
    const auto opcode = getOpcode(instr);

    switch ((Opcode)opcode) {
        case Opcode::SPECIAL:
            {
                const auto funct = getFunct(instr);

                switch ((SPECIAL)funct) {
                    case SPECIAL::SLL:
                        iSLL(allegrex, instr);
                        break;
                    case SPECIAL::SLLV:
                        iSLLV(allegrex, instr);
                        break;
                    case SPECIAL::JR:
                        iJR(allegrex, instr);
                        break;
                    case SPECIAL::ADDU:
                        iADDU(allegrex, instr);
                        break;
                    default:
                        std::printf("Unhandled %s SPECIAL instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), funct, instr, cpc);

                        exit(0);
                }
            }
            break;
        case Opcode::JAL:
            iJAL(allegrex, instr);
            break;
        case Opcode::BNE:
            iBNE(allegrex, instr);
            break;
        case Opcode::ADDIU:
            iADDIU(allegrex, instr);
            break;
        case Opcode::LUI:
            iLUI(allegrex, instr);
            break;
        case Opcode::COP0:
            {
                const auto rs = getRs(instr);

                switch ((COPOpcode)rs) {
                    case COPOpcode::MFC:
                        iMFC(allegrex, 0, instr);
                        break;
                    case COPOpcode::MTC:
                        iMTC(allegrex, 0, instr);
                        break;
                    case COPOpcode::CTC:
                        iCTC(allegrex, 0, instr);
                        break;
                    default:
                        std::printf("Unhandled %s coprocessor instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), rs, instr, cpc);

                        exit(0);
                }
            }
            break;
        case Opcode::SPECIAL3:
            {
                const auto funct = getFunct(instr);

                switch ((SPECIAL3)funct) {
                    case SPECIAL3::EXT:
                        iEXT(allegrex, instr);
                        break;
                    default:
                        std::printf("Unhandled %s SPECIAL3 instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), funct, instr, cpc);

                        exit(0);
                }
            }
            break;
        case Opcode::LW:
            iLW(allegrex, instr);
            break;
        case Opcode::SW:
            iSW(allegrex, instr);
            break;
        case Opcode::CACHE:
            iCACHE(allegrex, instr);
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
