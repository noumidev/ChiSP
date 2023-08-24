/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "fpu.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace psp::allegrex::fpu {

constexpr auto ENABLE_DISASM = true;

enum class SingleOpcode {
    ADD = 0x00,
    SUB = 0x01,
    MUL = 0x02,
    DIV = 0x03,
    SQRT = 0x04,
    MOV = 0x06,
    NEG = 0x07,
    TRUNCW = 0x0D,
    C = 0x30,
};

const char *fpuName[] = {
    "FPU:CPU ", "FPU:ME  ",
};

const char *condNames[] = {
    "F" , "UN"  , "EQ" , "UEQ", "OLT", "ULT", "OLE", "ULE",
    "SF", "NGLE", "SEQ", "NGL", "LT" , "NGE", "LE" , "NGT",
};

u32 getFd(u32 instr) {
    return (instr >> 6) & 0x1F;
}

u32 getFs(u32 instr) {
    return (instr >> 11) & 0x1F;
}

u32 getFt(u32 instr) {
    return (instr >> 16) & 0x1F;
}

void FPU::init(int cpuID) {
    assert((cpuID >= 0) && (cpuID < 2));

    this->cpuID = cpuID;

    std::printf("[%s] OK\n", fpuName[cpuID]);
}

u32 FPU::getControl(int idx) {
    return cregs[idx];
}

void FPU::setControl(int idx, u32 data) {
    cregs[idx] = data;
}

u32 FPU::get(int idx) {
    return fgrs[idx];
}

void FPU::set(int idx, u32 data) {
    fgrs[idx] = data;
}

f32 FPU::getF32(int idx) {
    f32 data;
    std::memcpy(&data, &fgrs[idx], sizeof(u32));

    return data;
}

void FPU::setF32(int idx, f32 data) {
    std::memcpy(&fgrs[idx], &data, sizeof(u32));
}

/* ADD */
void FPU::iADD(u32 instr) {
    const auto fd = getFd(instr);
    const auto fs = getFs(instr);
    const auto ft = getFt(instr);

    setF32(fd, getF32(fs) + getF32(ft));

    if (ENABLE_DISASM) {
        std::printf("[%s] ADD.S F%u, F%u, F%u; F%u = %f\n", fpuName[cpuID], fd, fs, ft, fd, getF32(fd));
    }
}

/* Compare */
void FPU::iC(u32 instr) {
    const auto cond = instr & 0xF;
    const auto fs = getFs(instr);
    const auto ft = getFt(instr);

    bool isLess, isEqual, isUnordered;

    const auto s = getF32(fs);
    const auto t = getF32(ft);

    if (std::isnan(s) || std::isnan(t)) {
        isLess = isEqual = false;
        isUnordered = true;

        // TODO: throw exception on signaling NaNs
    } else {
        isLess = s < t;
        isEqual = s == t;
        isUnordered = false;
    }

    cpcond = ((cond & (1 << 2)) && isLess) || ((cond & (1 << 1)) && isEqual) || ((cond & (1 << 0)) && isUnordered);

    std::printf("[%s] C.%s.S F%u, F%u; F%u = %f, F%u = %f, CPCOND: %d\n", fpuName[cpuID], condNames[cond], fs, ft, fs, s, ft, t, cpcond);
}

// ConVert To Single
void FPU::iCVTS(u32 instr) {
    const auto fd = getFd(instr);
    const auto fs = getFs(instr);

    setF32(fd, (f32)(i32)get(fs));

    if (ENABLE_DISASM) {
        std::printf("[%s] CVT.S.W F%u, F%u; F%u = %f\n", fpuName[cpuID], fd, fs, fd, getF32(fd));
    }
}

/* DIVide */
void FPU::iDIV(u32 instr) {
    const auto fd = getFd(instr);
    const auto fs = getFs(instr);
    const auto ft = getFt(instr);

    setF32(fd, getF32(fs) / getF32(ft));

    if (ENABLE_DISASM) {
        std::printf("[%s] DIV.S F%u, F%u, F%u; F%u = %f\n", fpuName[cpuID], fd, fs, ft, fd, getF32(fd));
    }
}

/* MOVe */
void FPU::iMOV(u32 instr) {
    const auto fd = getFd(instr);
    const auto fs = getFs(instr);

    setF32(fd, getF32(fs));

    if (ENABLE_DISASM) {
        std::printf("[%s] MOV.S F%u, F%u; F%u = %f\n", fpuName[cpuID], fd, fs, fd, getF32(fd));
    }
}

/* MULtiply */
void FPU::iMUL(u32 instr) {
    const auto fd = getFd(instr);
    const auto fs = getFs(instr);
    const auto ft = getFt(instr);

    setF32(fd, getF32(fs) * getF32(ft));

    if (ENABLE_DISASM) {
        std::printf("[%s] MUL.S F%u, F%u, F%u; F%u = %f\n", fpuName[cpuID], fd, fs, ft, fd, getF32(fd));
    }
}

/* NEGate */
void FPU::iNEG(u32 instr) {
    const auto fd = getFd(instr);
    const auto fs = getFs(instr);

    setF32(fd, -getF32(fs));

    if (ENABLE_DISASM) {
        std::printf("[%s] NEG.S F%u, F%u; F%u = %f\n", fpuName[cpuID], fd, fs, fd, getF32(fd));
    }
}

/* SQuare RooT */
void FPU::iSQRT(u32 instr) {
    const auto fd = getFd(instr);
    const auto fs = getFs(instr);

    setF32(fd, std::sqrt(getF32(fs)));

    if (ENABLE_DISASM) {
        std::printf("[%s] SQRT.S F%u, F%u; F%u = %f\n", fpuName[cpuID], fd, fs, fd, getF32(fd));
    }
}

/* SUBtract */
void FPU::iSUB(u32 instr) {
    const auto fd = getFd(instr);
    const auto fs = getFs(instr);
    const auto ft = getFt(instr);

    setF32(fd, getF32(fs) - getF32(ft));

    if (ENABLE_DISASM) {
        std::printf("[%s] SUB.S F%u, F%u, F%u; F%u = %f\n", fpuName[cpuID], fd, fs, ft, fd, getF32(fd));
    }
}

// TRUNCate To Word
void FPU::iTRUNCW(u32 instr) {
    const auto fd = getFd(instr);
    const auto fs = getFs(instr);

    set(fd, (i32)std::trunc(getF32(fs)));

    if (ENABLE_DISASM) {
        std::printf("[%s] TRUNC.W.S F%u, F%u; F%u = 0x%08X\n", fpuName[cpuID], fd, fs, fd, get(fd));
    }
}

void FPU::doSingle(u32 instr) {
    const auto opcode = instr & 0x3F;

    switch ((SingleOpcode)opcode) {
        case SingleOpcode::ADD:
            iADD(instr);
            break;
        case SingleOpcode::SUB:
            iSUB(instr);
            break;
        case SingleOpcode::MUL:
            iMUL(instr);
            break;
        case SingleOpcode::DIV:
            iDIV(instr);
            break;
        case SingleOpcode::SQRT:
            iSQRT(instr);
            break;
        case SingleOpcode::MOV:
            iMOV(instr);
            break;
        case SingleOpcode::NEG:
            iNEG(instr);
            break;
        case SingleOpcode::TRUNCW:
            iTRUNCW(instr);
            break;
        default:
            if (opcode >= (u32)SingleOpcode::C) {
                return iC(instr);
            }

            std::printf("Unhandled %s Single instruction 0x%02X (0x%08X)\n", fpuName[cpuID], opcode, instr);

            exit(0);
    }
}

void FPU::doWord(u32 instr) {
    const auto opcode = instr & 0x3F;

    assert(opcode == 0x20); // Only CVT.S.W?

    iCVTS(instr);
}

}
