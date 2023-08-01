/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "interpreter.hpp"

#include <bit>
#include <cassert>
#include <cstdio>
#include <cstring>

#include "allegrex.hpp"
#include "../memory.hpp"

namespace psp::allegrex::interpreter {

constexpr auto ENABLE_DISASM = false;

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
    REGIMM  = 0x01,
    J = 0x02,
    JAL  = 0x03,
    BEQ  = 0x04,
    BNE  = 0x05,
    BLEZ  = 0x06,
    BGTZ  = 0x07,
    ADDI  = 0x08,
    ADDIU = 0x09,
    SLTI  = 0x0A,
    SLTIU = 0x0B,
    ANDI = 0x0C,
    ORI  = 0x0D,
    XORI = 0x0E,
    LUI  = 0x0F,
    COP0 = 0x10,
    COP1 = 0x11,
    BEQL = 0x14,
    BNEL = 0x15,
    BLEZL = 0x16,
    BGTZL = 0x17,
    SPECIAL2 = 0x1C,
    SPECIAL3 = 0x1F,
    LB  = 0x20,
    LH  = 0x21,
    LWL = 0x22,
    LW  = 0x23,
    LBU = 0x24,
    LHU = 0x25,
    LWR = 0x26,
    SB  = 0x28,
    SH  = 0x29,
    SWL = 0x2A,
    SW  = 0x2B,
    SWR = 0x2E,
    CACHE = 0x2F,
    LWC1  = 0x31,
    SWC1  = 0x39,
};

enum class SPECIAL {
    SLL  = 0x00,
    SRL  = 0x02,
    SRA  = 0x03,
    SLLV = 0x04,
    SRLV = 0x06,
    SRAV = 0x07,
    JR = 0x08,
    JALR = 0x09,
    MOVZ = 0x0A,
    MOVN = 0x0B,
    SYSCALL = 0x0C,
    SYNC = 0x0F,
    MFHI = 0x10,
    MTHI = 0x11,
    MFLO = 0x12,
    MTLO = 0x13,
    CLZ = 0x16,
    MULT  = 0x18,
    MULTU = 0x19,
    DIV  = 0x1A,
    DIVU = 0x1B,
    ADD  = 0x20,
    ADDU = 0x21,
    SUB  = 0x22,
    SUBU = 0x23,
    AND = 0x24,
    OR  = 0x25,
    XOR = 0x26,
    NOR = 0x27,
    SLT  = 0x2A,
    SLTU = 0x2B,
    MAX = 0x2C,
    MIN = 0x2D,
};

enum class SPECIAL2 {
    HALT = 0x00,
    MFIC = 0x24,
    MTIC = 0x26,
};

enum class SPECIAL3 {
    EXT = 0x00,
    INS = 0x04,
    BSHFL = 0x20,
};

enum class BSHFL {
    SEB = 0x10,
    BITREV = 0x14,
    SEH = 0x18,
};

enum class REGIMM {
    BLTZ  = 0x00,
    BGEZ  = 0x01,
    BLTZL = 0x02,
    BGEZL = 0x03,
    BLTZAL = 0x10,
    BGEZAL = 0x11,
};

enum class COPOpcode {
    MFC = 0x00,
    CFC = 0x02,
    MTC = 0x04,
    CTC = 0x06,
    CO  = 0x10,
    W = 0x14,
};

enum class COP0Opcode {
    ERET = 0x18,
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

// ADD
void iADD(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    const auto result = (u64)(i32)allegrex->get(rs) + (u64)(i32)allegrex->get(rt);

    if (((result >> 31) & 1) != ((result >> 32) & 1)) { // ?? Is this a thing on Allegrex?
        std::puts("ADD overflow");

        exit(0);
    }

    allegrex->set(rd, (u32)result);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] ADD %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// ADD Immediate
void iADDI(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (u32)(i16)getImm(instr);

    const auto result = (u64)allegrex->get(rs) + (u64)(i32)imm;

    if (((result >> 31) & 1) != ((result >> 32) & 1)) { // ?? Is this a thing on Allegrex?
        std::puts("ADDI overflow");

        exit(0);
    }

    allegrex->set(rt, (u32)result);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] ADDI %s, %s, 0x%X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rs], imm, regNames[rt], allegrex->get(rt));
    }
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

// AND
void iAND(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, allegrex->get(rs) & allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] AND %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// AND Immediate
void iANDI(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = getImm(instr);

    allegrex->set(rt, allegrex->get(rs) & imm);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] ANDI %s, %s, 0x%X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rs], imm, regNames[rt], allegrex->get(rt));
    }
}

// Branch if EQual
void iBEQ(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);
    const auto t = allegrex->get(rt);

    allegrex->doBranch(target, s == t, Reg::R0, false);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BEQ %s, %s, 0x%08X; %s = 0x%08X, %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], regNames[rt], target, regNames[rs], s, regNames[rt], t);
    }
}

// Branch if EQual Likely
void iBEQL(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);
    const auto t = allegrex->get(rt);

    allegrex->doBranch(target, s == t, Reg::R0, true);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BEQL %s, %s, 0x%08X; %s = 0x%08X, %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], regNames[rt], target, regNames[rs], s, regNames[rt], t);
    }
}

// Branch if Greater than or Equal Zero
void iBGEZ(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);

    allegrex->doBranch(target, (i32)s >= 0, Reg::R0, false);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BGEZ %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target, regNames[rs], s);
    }
}

// Branch if Greater than or Equal Zero And Link
void iBGEZAL(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);

    allegrex->doBranch(target, (i32)s >= 0, Reg::RA, false);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BGEZAL %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target, regNames[rs], s);
    }
}

// Branch if Greater than or Equal Zero Likely
void iBGEZL(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);

    allegrex->doBranch(target, (i32)s >= 0, Reg::R0, true);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BGEZL %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target, regNames[rs], s);
    }
}

// Branch if Greater Than Zero
void iBGTZ(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);

    allegrex->doBranch(target, (i32)s > 0, Reg::R0, false);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BGTZ %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target, regNames[rs], s);
    }
}

// Branch if Greater Than Zero Likely
void iBGTZL(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);

    allegrex->doBranch(target, (i32)s > 0, Reg::R0, true);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BGTZL %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target, regNames[rs], s);
    }
}

// BIT REVerse
void iBITREV(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rt = getRt(instr);

    const auto t = allegrex->get(rt);

    u32 data = 0;
    for (int i = 0; i < 32; i++) {
        if (t & (1 << i)) data |= 1 << (31 - i);
    }

    allegrex->set(rd, data);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BITREV %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// Branch if Less than or Equal Zero
void iBLEZ(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);

    allegrex->doBranch(target, (i32)s <= 0, Reg::R0, false);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BLEZ %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target, regNames[rs], s);
    }
}

// Branch if Less than or Equal Zero Likely
void iBLEZL(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);

    allegrex->doBranch(target, (i32)s <= 0, Reg::R0, true);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BLEZL %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target, regNames[rs], s);
    }
}

// Branch if Less Than Zero
void iBLTZ(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);

    allegrex->doBranch(target, (i32)s < 0, Reg::R0, false);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BLTZ %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target, regNames[rs], s);
    }
}

// Branch if Less Than Zero And Link
void iBLTZAL(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);

    allegrex->doBranch(target, (i32)s < 0, Reg::RA, false);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BLTZAL %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target, regNames[rs], s);
    }
}

// Branch if Less Than Zero Likely
void iBLTZL(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);

    allegrex->doBranch(target, (i32)s < 0, Reg::R0, true);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BLTZL %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], target, regNames[rs], s);
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

// Branch if Not Equal Likely
void iBNEL(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto offset = (i32)(i16)getImm(instr) << 2;

    const auto target = allegrex->getPC() + offset;

    const auto s = allegrex->get(rs);
    const auto t = allegrex->get(rt);

    allegrex->doBranch(target, s != t, Reg::R0, true);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] BNEL %s, %s, 0x%08X; %s = 0x%08X, %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], regNames[rt], target, regNames[rs], s, regNames[rt], t);
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

// move Coprocessor From Control
void iCFC(Allegrex *allegrex, int copN, u32 instr) {
    assert((copN >= 0) && (copN < 4));

    const auto rd = getRd(instr);
    const auto rt = getRt(instr);

    u32 data;

    switch (copN) {
        case 0:
            data = allegrex->cop0.getControl(rd);
            break;
        case 1:
            data = allegrex->fpu.getControl(rd);
            break;
        default:
            std::printf("Unhandled %s CFC coprocessor %d\n", allegrex->getTypeName(), copN);

            exit(0);
    }

    allegrex->set(rt, data);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] CFC%d %s, %d; %s = 0x%08X\n", allegrex->getTypeName(), cpc, copN, regNames[rt], rd, regNames[rt], data);
    }
}

/* Count Leading Zeroes */
void iCLZ(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);

    const auto s = allegrex->get(rs);

    if (!s) {
        allegrex->set(rd, 32);
    } else {
        allegrex->set(rd, std::__countl_zero(s));
    }

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] CLZ %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rd], allegrex->get(rd));
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
        case 1:
            allegrex->fpu.setControl(rd, t);
            break;
        default:
            std::printf("Unhandled %s CTC coprocessor %d\n", allegrex->getTypeName(), copN);

            exit(0);
    }

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] CTC%d %s, %d; %d = 0x%08X\n", allegrex->getTypeName(), cpc, copN, regNames[rt], rd, rd, t);
    }
}

/* DIVide */
void iDIV(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    const auto n = (i32)allegrex->get(rs);
    const auto d = (i32)allegrex->get(rt);

    assert((d != 0) && !((n == INT32_MIN) && (d == -1)));

    allegrex->set(Reg::LO, n / d);
    allegrex->set(Reg::HI, n % d);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] DIV %s, %s; LO = 0x%08X, HI = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], regNames[rt], allegrex->get(Reg::LO), allegrex->get(Reg::HI));
    }
}

/* DIVide Unsigned */
void iDIVU(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    const auto n = allegrex->get(rs);
    const auto d = allegrex->get(rt);

    if (!d) {
        allegrex->set(Reg::LO, -1);
        allegrex->set(Reg::HI, n);
    } else {
        allegrex->set(Reg::LO, n / d);
        allegrex->set(Reg::HI, n % d);
    }

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] DIVU %s, %s; LO = 0x%08X, HI = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], regNames[rt], allegrex->get(Reg::LO), allegrex->get(Reg::HI));
    }
}

// Exception RETurn
void iERET(Allegrex *allegrex, u32 instr) {
    (void)instr;

    allegrex->exceptionReturn();

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] ERET\n", allegrex->getTypeName(), cpc);
    }

    allegrex->checkInterrupt();
}

// Extract
void iEXT(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto pos  = getShamt(instr);
    const auto size = getRd(instr) + 1;

    assert((pos + size) <= 32);

    // Shift rs by pos, then mask with (1 << size) - 1 (mask is all 1s)
    const auto mask = 0xFFFFFFFFu >> (32 - size);

    allegrex->set(rt, (allegrex->get(rs) >> pos) & mask);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] EXT %s, %s, %u, %u; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rs], pos, size, regNames[rt], allegrex->get(rt));
    }
}

// HALT
void iHALT(Allegrex *allegrex, u32 instr) {
    (void)instr;

    allegrex->isHalted = true;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] HALT\n", allegrex->getTypeName(), cpc);
    }
}

/* INSert */
void iINS(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto pos  = getShamt(instr);
    const auto size = (getRd(instr) + 1) - pos;

    assert(size && (size <= 32));

    const auto mask = 0xFFFFFFFFu >> (32 - size);

    allegrex->set(rt, (allegrex->get(rt) & ~(mask << pos)) | ((allegrex->get(rs) & mask) << pos));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] INS %s, %s, %u, %u; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rs], pos, size, regNames[rt], allegrex->get(rt));
    }
}

// Jump
void iJ(Allegrex *allegrex, u32 instr) {
    const auto target = (allegrex->getPC() & 0xF0000000) | (getOffset(instr) << 2);

    allegrex->doBranch(target, true, Reg::R0, false);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] J 0x%08X\n", allegrex->getTypeName(), cpc, target);
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

// Jump And Link Register
void iJALR(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);

    const auto target = allegrex->get(rs);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] JALR %s, %s; %s = 0x%08X, PC = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rd], allegrex->get(rd), target);
    }

    allegrex->doBranch(target, true, rd, false);
}

// Load Byte
void iLB(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] LB %s, 0x%X(%s); %s = [0x%08X]\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], regNames[rt], addr);
    }

    allegrex->set(rt, (i8)allegrex->read8(addr));
}

// Load Byte Unsigned
void iLBU(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] LBU %s, 0x%X(%s); %s = [0x%08X]\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], regNames[rt], addr);
    }

    allegrex->set(rt, allegrex->read8(addr));
}

// Load Halfword
void iLH(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] LH %s, 0x%X(%s); %s = [0x%08X]\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], regNames[rt], addr);
    }

    if (addr & 1) {
        std::printf("Misaligned %s LH address 0x%08X, PC: 0x%08X\n", allegrex->getTypeName(), addr, cpc);

        exit(0);
    }

    allegrex->set(rt, (i16)allegrex->read16(addr));
}

// Load Halfword Unsigned
void iLHU(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] LHU %s, 0x%X(%s); %s = [0x%08X]\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], regNames[rt], addr);
    }

    if (addr & 1) {
        std::printf("Misaligned %s LHU address 0x%08X, PC: 0x%08X\n", allegrex->getTypeName(), addr, cpc);

        exit(0);
    }

    allegrex->set(rt, allegrex->read16(addr));
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

// Load Word Coprocessor
void iLWC(Allegrex *allegrex, int copN, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] LWC%d %u, 0x%X(%s); %s = [0x%08X]\n", allegrex->getTypeName(), cpc, copN, rt, imm, regNames[rs], regNames[rt], addr);
    }

    if (addr & 3) {
        std::printf("Misaligned %s LWC address 0x%08X, PC: 0x%08X\n", allegrex->getTypeName(), addr, cpc);

        exit(0);
    }

    const auto data = allegrex->read32(addr);

    switch (copN) {
        case 1:
            allegrex->fpu.set(rt, data);
            break;
        default:
            std::printf("Unhandled %s LWC coprocessor %d\n", allegrex->getTypeName(), copN);

            exit(0);
    }
}

/* Load Word Left */
void iLWL(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] LWL %s, 0x%X(%s); %s = [0x%08X]\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], regNames[rt], addr);
    }

    const auto shift = 24 - 8 * (addr & 3);
    const auto mask = ~(~0 << shift);

    allegrex->set(rt, (allegrex->get(rt) & mask) | (allegrex->read32(addr & ~3) << shift));
}

/* Load Word Right */
void iLWR(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] LWR %s, 0x%X(%s); %s = [0x%08X]\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], regNames[rt], addr);
    }

    const auto shift = 8 * (addr & 3);
    const auto mask = 0xFFFFFF00 << (24 - shift);

    allegrex->set(rt, (allegrex->get(rt) & mask) | (allegrex->read32(addr & ~3) >> shift));
}

/* MAX */
void iMAX(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    if ((i32)allegrex->get(rs) > (i32)allegrex->get(rt)) {
        allegrex->set(rd, allegrex->get(rs));
    } else {
        allegrex->set(rd, allegrex->get(rt));
    }

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MAX %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
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
        case 1:
            data = allegrex->fpu.get(rd);
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

/* Move From HI */
void iMFHI(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);

    allegrex->set(rd, allegrex->get(Reg::HI));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MFHI %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rd], allegrex->get(rd));
    }
}

// Move From Interrupt Control
void iMFIC(Allegrex *allegrex, u32 instr) {
    const auto rt = getRt(instr);

    const auto ic = allegrex->cop0.getIC();

    allegrex->set(rt, ic);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MFIC %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rt], ic);
    }
}

/* Move From LO */
void iMFLO(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);

    allegrex->set(rd, allegrex->get(Reg::LO));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MFLO %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rd], allegrex->get(rd));
    }
}

/* MINimum */
void iMIN(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    if ((i32)allegrex->get(rs) < (i32)allegrex->get(rt)) {
        allegrex->set(rd, allegrex->get(rs));
    } else {
        allegrex->set(rd, allegrex->get(rt));
    }

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MIN %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

/* MOVe if Not zero */
void iMOVN(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    if (allegrex->get(rt)) allegrex->set(rd, allegrex->get(rs));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MOVN %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

/* MOVe if Zero */
void iMOVZ(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    if (!allegrex->get(rt)) allegrex->set(rd, allegrex->get(rs));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MOVZ %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
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

            allegrex->checkInterrupt();
            break;
        case 1:
            allegrex->fpu.set(rd, t);
            break;
        default:
            std::printf("Unhandled %s MTC coprocessor %d\n", allegrex->getTypeName(), copN);

            exit(0);
    }

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MTC%d %s, %d; %d = 0x%08X\n", allegrex->getTypeName(), cpc, copN, regNames[rt], rd, rd, t);
    }
}

/* Move To HI */
void iMTHI(Allegrex *allegrex, u32 instr) {
    const auto rs = getRd(instr);

    allegrex->set(Reg::HI, allegrex->get(rs));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MTHI %s; HI = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], allegrex->get(Reg::HI));
    }
}

// Move To Interrupt Control
void iMTIC(Allegrex *allegrex, u32 instr) {
    const auto rt = getRt(instr);

    allegrex->cop0.setIC(allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MTIC %s; IC = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], allegrex->cop0.getIC());
    }

    allegrex->checkInterrupt();
}

/* Move To LO */
void iMTLO(Allegrex *allegrex, u32 instr) {
    const auto rs = getRd(instr);

    allegrex->set(Reg::LO, allegrex->get(rs));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MTLO %s; LO = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], allegrex->get(Reg::LO));
    }
}

/* MULTiply */
void iMULT(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    const auto result = (i64)(i32)allegrex->get(rs) * (i64)(i32)allegrex->get(rt);

    allegrex->set(Reg::LO, result >>  0);
    allegrex->set(Reg::HI, result >> 32);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MULT %s, %s; LO = 0x%08X, HI = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], regNames[rt], allegrex->get(Reg::LO), allegrex->get(Reg::HI));
    }
}   

/* MULTiply Unsigned */
void iMULTU(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    const auto result = (u64)allegrex->get(rs) * (u64)allegrex->get(rt);

    allegrex->set(Reg::LO, result >>  0);
    allegrex->set(Reg::HI, result >> 32);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] MULTU %s, %s; LO = 0x%08X, HI = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rs], regNames[rt], allegrex->get(Reg::LO), allegrex->get(Reg::HI));
    }
}   

// NOR
void iNOR(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, ~(allegrex->get(rs) | allegrex->get(rt)));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] NOR %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// OR
void iOR(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, allegrex->get(rs) | allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] OR %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// OR Immediate
void iORI(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = getImm(instr);

    allegrex->set(rt, allegrex->get(rs) | imm);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] ORI %s, %s, 0x%X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rs], imm, regNames[rt], allegrex->get(rt));
    }
}

// ROTate Right Variable
void iROTRV(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    const auto t = allegrex->get(rt);
    const auto shamt = allegrex->get(rs) & 0x1F;

    allegrex->set(rd, std::__rotr(t, shamt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] ROTRV %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rt], regNames[rs], regNames[rd], allegrex->get(rd));
    }
}

// Store Byte
void iSB(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;
    const auto data = (u8)allegrex->get(rt);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SB %s, 0x%X(%s); [0x%08X] = 0x%02X\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], addr, data);
    }

    allegrex->write8(addr, data);
}

/* Sign Extend Byte */
void iSEB(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, (i8)allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SEB %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

/* Sign Extend Halfword */
void iSEH(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, (i16)allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SEH %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// Store Halfword
void iSH(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;
    const auto data = (u16)allegrex->get(rt);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SH %s, 0x%X(%s); [0x%08X] = 0x%04X\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], addr, data);
    }

    if (addr & 1) {
        std::printf("Misaligned %s SH address 0x%08X, PC: 0x%08X\n", allegrex->getTypeName(), addr, cpc);

        exit(0);
    }

    allegrex->write16(addr, data);
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

/* Set on Less Than */
void iSLT(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, (i32)allegrex->get(rs) < (i32)allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SLT %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// Set on Less Than Immediate
void iSLTI(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    allegrex->set(rt, (i32)allegrex->get(rs) < imm);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SLTIU %s, %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rs], imm, regNames[rt], allegrex->get(rt));
    }
}

// Set on Less Than Immediate Unsigned
void iSLTIU(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (u32)(i16)getImm(instr);

    allegrex->set(rt, allegrex->get(rs) < imm);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SLTIU %s, %s, 0x%08X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rs], imm, regNames[rt], allegrex->get(rt));
    }
}

/* Set on Less Than Unsigned */
void iSLTU(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, allegrex->get(rs) < allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SLTU %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// Shift Right Arithmetic
void iSRA(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rt = getRt(instr);
    const auto shamt = getShamt(instr);

    allegrex->set(rd, (i32)allegrex->get(rt) >> shamt);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SRA %s, %s, %u; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rt], shamt, regNames[rd], allegrex->get(rd));
    }
}

// Shift Right Arithmetic Variable
void iSRAV(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, (i32)allegrex->get(rt) >> (allegrex->get(rs) & 0x1F));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SRAV %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rt], regNames[rs], regNames[rd], allegrex->get(rd));
    }
}

// Shift Right Logical
void iSRL(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rt = getRt(instr);
    const auto shamt = getShamt(instr);

    allegrex->set(rd, allegrex->get(rt) >> shamt);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SRL %s, %s, %u; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rt], shamt, regNames[rd], allegrex->get(rd));
    }
}

// Shift Right Logical Variable
void iSRLV(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, allegrex->get(rt) >> (allegrex->get(rs) & 0x1F));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SRLV %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rt], regNames[rs], regNames[rd], allegrex->get(rd));
    }
}

// SUBtract
void iSUB(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    // TODO: SUB overflow check?

    allegrex->set(rd, allegrex->get(rs) - allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SUB %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// SUBtract Unsigned
void iSUBU(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, allegrex->get(rs) - allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SUBU %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
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

// Store Word Coprocessor
void iSWC(Allegrex *allegrex, int copN, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;
    
    u32 data;
    switch (copN) {
        case 1:
            data = allegrex->fpu.get(rt);
            break;
        default:
            std::printf("Unhandled %s SWC coprocessor %d\n", allegrex->getTypeName(), copN);

            exit(0);
    }

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SWC%d %u, 0x%X(%s); [0x%08X] = 0x%08X\n", allegrex->getTypeName(), cpc, copN, rt, imm, regNames[rs], addr, data);
    }

    if (addr & 3) {
        std::printf("Misaligned %s SWC address 0x%08X, PC: 0x%08X\n", allegrex->getTypeName(), addr, cpc);

        exit(0);
    }

    allegrex->write32(addr, data);
}

/* Store Word Left */
void iSWL(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;

    const auto shift = 8 * (addr & 3);
    const auto mask  = 0xFFFFFF00 << shift;

    const auto data = (allegrex->read32(addr & ~3) & mask) | (allegrex->get(rt) >> (24 - shift));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SWL %s, 0x%X(%s); [0x%08X] = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], addr, data);
    }

    allegrex->write32(addr & ~3, data);
}

/* Store Word Right */
void iSWR(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    const auto imm = (i32)(i16)getImm(instr);

    const auto addr = allegrex->get(rs) + imm;

    const auto shift = 8 * (addr & 3);
    const auto mask  = ~(~0 << shift);

    const auto data = (allegrex->read32(addr & ~3) & mask) | (allegrex->get(rt) << shift);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SWR %s, 0x%X(%s); [0x%08X] = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], imm, regNames[rs], addr, data);
    }

    allegrex->write32(addr & ~3, data);
}

// SYNC
void iSYNC(Allegrex *allegrex, u32 instr) {
    (void)instr;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SYNC\n", allegrex->getTypeName(), cpc);
    }
}

// SYStem CALL
void iSYSCALL(Allegrex *allegrex, u32 instr) {
    (void)instr;

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] SYSCALL\n", allegrex->getTypeName(), cpc);
    }

    allegrex->raiseException(Exception::SystemCall);
    allegrex->cop0.setSyscallCode((instr >> 6) & 0xFFFFF);
}

// XOR
void iXOR(Allegrex *allegrex, u32 instr) {
    const auto rd = getRd(instr);
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);

    allegrex->set(rd, allegrex->get(rs) ^ allegrex->get(rt));

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] XOR %s, %s, %s; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rd], regNames[rs], regNames[rt], regNames[rd], allegrex->get(rd));
    }
}

// XOR Immediate
void iXORI(Allegrex *allegrex, u32 instr) {
    const auto rs = getRs(instr);
    const auto rt = getRt(instr);
    const auto imm = getImm(instr);

    allegrex->set(rt, allegrex->get(rs) ^ imm);

    if (ENABLE_DISASM) {
        std::printf("[%s] [0x%08X] XORI %s, %s, 0x%X; %s = 0x%08X\n", allegrex->getTypeName(), cpc, regNames[rt], regNames[rs], imm, regNames[rt], allegrex->get(rt));
    }
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
                    case SPECIAL::SRL:
                        {
                            const auto rs = getRs(instr);

                            switch (rs) {
                                case 0:
                                    iSRL(allegrex, instr);
                                    break;
                                case 1:
                                    std::puts("Unimplemented ROTR");
                                    
                                    exit(0);
                                default:
                                    std::printf("Invalid %s instruction 0x%02X:0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), funct, rs, instr, cpc);

                                    exit(0);
                            }
                        }
                        break;
                    case SPECIAL::SRA:
                        iSRA(allegrex, instr);
                        break;
                    case SPECIAL::SLLV:
                        iSLLV(allegrex, instr);
                        break;
                    case SPECIAL::SRLV:
                        {
                            const auto shamt = getShamt(instr);

                            switch (shamt) {
                                case 0:
                                    iSRLV(allegrex, instr);
                                    break;
                                case 1:
                                    iROTRV(allegrex, instr);
                                    break;
                                default:
                                    std::printf("Invalid %s instruction 0x%02X:0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), funct, shamt, instr, cpc);

                                    exit(0);
                            }
                        }
                        break;
                    case SPECIAL::SRAV:
                        iSRAV(allegrex, instr);
                        break;
                    case SPECIAL::JR:
                        iJR(allegrex, instr);
                        break;
                    case SPECIAL::JALR:
                        iJALR(allegrex, instr);
                        break;
                    case SPECIAL::MOVZ:
                        iMOVZ(allegrex, instr);
                        break;
                    case SPECIAL::MOVN:
                        iMOVN(allegrex, instr);
                        break;
                    case SPECIAL::SYSCALL:
                        iSYSCALL(allegrex, instr);
                        break;
                    case SPECIAL::SYNC:
                        iSYNC(allegrex, instr);
                        break;
                    case SPECIAL::MFHI:
                        iMFHI(allegrex, instr);
                        break;
                    case SPECIAL::MTHI:
                        iMTHI(allegrex, instr);
                        break;
                    case SPECIAL::MFLO:
                        iMFLO(allegrex, instr);
                        break;
                    case SPECIAL::MTLO:
                        iMTLO(allegrex, instr);
                        break;
                    case SPECIAL::CLZ:
                        iCLZ(allegrex, instr);
                        break;
                    case SPECIAL::MULT:
                        iMULTU(allegrex, instr);
                        break;
                    case SPECIAL::MULTU:
                        iMULTU(allegrex, instr);
                        break;
                    case SPECIAL::DIV:
                        iDIV(allegrex, instr);
                        break;
                    case SPECIAL::DIVU:
                        iDIVU(allegrex, instr);
                        break;
                    case SPECIAL::ADD:
                        iADD(allegrex, instr);
                        break;
                    case SPECIAL::ADDU:
                        iADDU(allegrex, instr);
                        break;
                    case SPECIAL::SUB:
                        iSUB(allegrex, instr);
                        break;
                    case SPECIAL::SUBU:
                        iSUBU(allegrex, instr);
                        break;
                    case SPECIAL::AND:
                        iAND(allegrex, instr);
                        break;
                    case SPECIAL::OR:
                        iOR(allegrex, instr);
                        break;
                    case SPECIAL::XOR:
                        iXOR(allegrex, instr);
                        break;
                    case SPECIAL::NOR:
                        iNOR(allegrex, instr);
                        break;
                    case SPECIAL::SLT:
                        iSLT(allegrex, instr);
                        break;
                    case SPECIAL::SLTU:
                        iSLTU(allegrex, instr);
                        break;
                    case SPECIAL::MAX:
                        iMAX(allegrex, instr);
                        break;
                    case SPECIAL::MIN:
                        iMIN(allegrex, instr);
                        break;
                    default:
                        std::printf("Unhandled %s SPECIAL instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), funct, instr, cpc);

                        exit(0);
                }
            }
            break;
        case Opcode::REGIMM:
            {
                const auto rt = getRt(instr);

                switch ((REGIMM)rt) {
                    case REGIMM::BLTZ:
                        iBLTZ(allegrex, instr);
                        break;
                    case REGIMM::BGEZ:
                        iBGEZ(allegrex, instr);
                        break;
                    case REGIMM::BLTZL:
                        iBLTZL(allegrex, instr);
                        break;
                    case REGIMM::BGEZL:
                        iBGEZL(allegrex, instr);
                        break;
                    case REGIMM::BLTZAL:
                        iBLTZAL(allegrex, instr);
                        break;
                    case REGIMM::BGEZAL:
                        iBGEZAL(allegrex, instr);
                        break;
                    default:
                        std::printf("Unhandled %s REGIMM instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), rt, instr, cpc);

                        exit(0);
                }
            }
            break;
        case Opcode::J:
            iJ(allegrex, instr);
            break;
        case Opcode::JAL:
            iJAL(allegrex, instr);
            break;
        case Opcode::BEQ:
            iBEQ(allegrex, instr);
            break;
        case Opcode::BNE:
            iBNE(allegrex, instr);
            break;
        case Opcode::BLEZ:
            iBLEZ(allegrex, instr);
            break;
        case Opcode::BGTZ:
            iBGTZ(allegrex, instr);
            break;
        case Opcode::ADDI:
            iADDI(allegrex, instr);
            break;
        case Opcode::ADDIU:
            iADDIU(allegrex, instr);
            break;
        case Opcode::SLTI:
            iSLTI(allegrex, instr);
            break;
        case Opcode::SLTIU:
            iSLTIU(allegrex, instr);
            break;
        case Opcode::ANDI:
            iANDI(allegrex, instr);
            break;
        case Opcode::ORI:
            iORI(allegrex, instr);
            break;
        case Opcode::XORI:
            iXORI(allegrex, instr);
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
                    case COPOpcode::CFC:
                        iCFC(allegrex, 0, instr);
                        break;
                    case COPOpcode::MTC:
                        iMTC(allegrex, 0, instr);
                        break;
                    case COPOpcode::CTC:
                        iCTC(allegrex, 0, instr);
                        break;
                    case COPOpcode::CO:
                        {
                            const auto funct = getFunct(instr);

                            switch ((COP0Opcode)funct) {
                                case COP0Opcode::ERET:
                                    iERET(allegrex, instr);
                                    break;
                                default:
                                    std::printf("Unhandled %s COP0 operation 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), funct, instr, cpc);

                                    exit(0);
                            }
                        }
                        break;
                    default:
                        std::printf("Unhandled %s coprocessor instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), rs, instr, cpc);

                        exit(0);
                }
            }
            break;
        case Opcode::COP1:
            {
                const auto rs = getRs(instr);

                switch ((COPOpcode)rs) {
                    case COPOpcode::MFC:
                        iMFC(allegrex, 1, instr);
                        break;
                    case COPOpcode::CFC:
                        iCFC(allegrex, 1, instr);
                        break;
                    case COPOpcode::MTC:
                        iMTC(allegrex, 1, instr);
                        break;
                    case COPOpcode::CTC:
                        iCTC(allegrex, 1, instr);
                        break;
                    case COPOpcode::CO: // Actually Single instructions
                        allegrex->fpu.doSingle(instr);
                        break;
                    case COPOpcode::W:
                        allegrex->fpu.doWord(instr);
                        break;
                    default:
                        std::printf("Unhandled %s coprocessor instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), rs, instr, cpc);

                        exit(0);
                }
            }
            break;
        case Opcode::BEQL:
            iBEQL(allegrex, instr);
            break;
        case Opcode::BNEL:
            iBNEL(allegrex, instr);
            break;
        case Opcode::BLEZL:
            iBLEZL(allegrex, instr);
            break;
        case Opcode::BGTZL:
            iBGTZL(allegrex, instr);
            break;
        case Opcode::SPECIAL2:
            {
                const auto funct = getFunct(instr);

                switch ((SPECIAL2)funct) {
                    case SPECIAL2::HALT:
                        iHALT(allegrex, instr);
                        break;
                    case SPECIAL2::MFIC:
                        iMFIC(allegrex, instr);
                        break;
                    case SPECIAL2::MTIC:
                        iMTIC(allegrex, instr);
                        break;
                    default:
                        std::printf("Unhandled %s SPECIAL2 instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), funct, instr, cpc);

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
                    case SPECIAL3::INS:
                        iINS(allegrex, instr);
                        break;
                    case SPECIAL3::BSHFL:
                        {
                            const auto shamt = getShamt(instr);

                            switch ((BSHFL)shamt) {
                                case BSHFL::SEB:
                                    iSEB(allegrex, instr);
                                    break;
                                case BSHFL::BITREV:
                                    iBITREV(allegrex, instr);
                                    break;
                                case BSHFL::SEH:
                                    iSEH(allegrex, instr);
                                    break;
                                default:
                                    std::printf("Unhandled BSHFL instruction 0x%02X (0x%08X) @ 0x%08X\n", shamt, instr, cpc);

                                    exit(0);
                            }
                        }
                        break;
                    default:
                        std::printf("Unhandled %s SPECIAL3 instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), funct, instr, cpc);

                        exit(0);
                }
            }
            break;
        case Opcode::LB:
            iLB(allegrex, instr);
            break;
        case Opcode::LH:
            iLH(allegrex, instr);
            break;
        case Opcode::LWL:
            iLWL(allegrex, instr);
            break;
        case Opcode::LW:
            iLW(allegrex, instr);
            break;
        case Opcode::LBU:
            iLBU(allegrex, instr);
            break;
        case Opcode::LHU:
            iLHU(allegrex, instr);
            break;
        case Opcode::LWR:
            iLWR(allegrex, instr);
            break;
        case Opcode::SB:
            iSB(allegrex, instr);
            break;
        case Opcode::SH:
            iSH(allegrex, instr);
            break;
        case Opcode::SWL:
            iSWL(allegrex, instr);
            break;
        case Opcode::SW:
            iSW(allegrex, instr);
            break;
        case Opcode::SWR:
            iSWR(allegrex, instr);
            break;
        case Opcode::CACHE:
            iCACHE(allegrex, instr);
            break;
        case Opcode::LWC1:
            iLWC(allegrex, 1, instr);
            break;
        case Opcode::SWC1:
            iSWC(allegrex, 1, instr);
            break;
        default:
            std::printf("Unhandled %s instruction 0x%02X (0x%08X) @ 0x%08X\n", allegrex->getTypeName(), opcode, instr, cpc);

            exit(0);
    }

    return 1;
}

/**
 * Loadcore Boot Information - Used to boot the system via Loadcore.
 */
struct SceLoadCoreBootInfo { 
    /** 
     * Pointer to a memory block which will be cleared in case the system initialization via 
     * Loadcore fails.
     */
    u32 memBase; // 0
    /** The size of the memory block to clear. */
    u32 memSize; // 4
    /** Number of modules already loaded during boot process. */
    u32 loadedModules; // 8
    /** Number of modules to boot. */
    u32 numModules; // 12
    /** The modules to boot. */
    u32 modules; // 16
     /** Unknown. */
    i32 unk20; //20
     /** Unknown. */
    u8 unk24; //24
    /** Reserved - padding. */
    u8 reserved[3]; // ?
    /** The number of protected (?)modules.*/
    i32 numProtects; // 28
    /** Pointer to the protected (?)modules. */
    u32 protects; // 32
    /** The ID of a protected info. */
    u32 modProtId;
    /** The ID of a module's arguments? */
    u32 modArgProtId; // 40
     /** Unknown. */
    i32 unk44;
     /** Unknown. */
    i32 buildVersion;
     /** Unknown. */
    i32 unk52;
    /** The path/name of a boot configuration file. */
    u32 configFile; // 56
    /** Unknown. */
    i32 unk60;
    /** Unknown. */
    i32 unk64;
    /** Unknown. */
    i32 unk68;
    /** Unknown. */
    i32 unk72;
    /** Unknown. */
    i32 unk76;
    /** Unknown. */
    u32 unk80;
    /** Unknown. */
    u32 unk84;
    /** Unknown. */
    u32 unk98;
    /** Unknown. */
    u32 unk92;
    /** Unknown. */
    u32 unk96;
    /** Unknown. */
    u32 unk100;
    /** Unknown. */
    u32 unk104;
    /** Unknown. */
    u32 unk108;
    /** Unknown. */
    u32 unk112;
    /** Unknown. */
    u32 unk116;
    /** Unknown. */
    u32 unk120;
    /** Unknown. */
    u32 unk124;
} __attribute__((packed)); //size = 128

static_assert(sizeof(SceLoadCoreBootInfo) == 128);

/**
 * This structure is used to boot system modules during the initialization of Loadcore. It represents
 * a module object with all the necessary information needed to boot it.
 */
struct SceLoadCoreBootModuleInfo {
    /** The full path (including filename) of the module. */
    u32 modPath; //0
    /** The buffer with the entire file content. */
    u32 modBuf; //4
    /** The size of the module. */
    u32 modSize; //8
    /** Unknown. */
    i32 unk12; //12
    /** Attributes. */
    u32 attr; //16
    /** 
     * Contains the API type of the module prior to the allocation of memory for the module. 
     * Once memory is allocated, ::bootData contains the ID of that memory partition.
     */
    i32 bootData; //20
    /** The size of the arguments passed to the module's entry function? */
    u32 argSize; //24
    /** The partition ID of the arguments passed to the module's entry function? */
    u32 argPartId; //28
} __attribute__((packed));

static_assert(sizeof(SceLoadCoreBootModuleInfo) == 32);

SceLoadCoreBootInfo *bootInfo = NULL;

void printModules() {
    if (bootInfo == NULL) return;

    std::printf("Number of modules to boot: %u (loaded: %u)\n", bootInfo->numModules, bootInfo->loadedModules);

    const auto modules = (SceLoadCoreBootModuleInfo *)memory::getMemoryPointer(bootInfo->modules);

    for (u32 i = 0; i < bootInfo->numModules; i++) {
        const auto mod = modules[i];

        std::printf("Module: %s\n", memory::getMemoryPointer(mod.modBuf + 10));
    }
}

void run(Allegrex *allegrex, i64 runCycles) {
    for (i64 i = 0; i < runCycles;) {
        if (allegrex->isHalted) return;

        cpc = allegrex->getPC();

        if (cpc == 0x04007DE8) allegrex->set(Reg::V0, 0); // I still need this hack :(

        if (cpc == 0x880629CC) {
            std::puts("InitThreadEntry");

            bootInfo = (SceLoadCoreBootInfo *)memory::getMemoryPointer(memory::read32(allegrex->get(Reg::A1) + 4));

            printModules();
        }

        if (cpc == 0x880402EC) {
            printModules();
        }

        allegrex->advanceDelay();

        i += doInstr(allegrex);
    }
}

}
