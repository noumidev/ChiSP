/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "fpu.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

namespace psp::allegrex::fpu {

constexpr auto ENABLE_DISASM = true;

enum class SingleOpcode {
    MUL = 0x02,
};

const char *fpuName[] = {
    "FPU:CPU ", "FPU:ME  ",
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

// ConVert To Single
void FPU::iCVTS(u32 instr) {
    const auto fd = getFd(instr);
    const auto fs = getFs(instr);

    setF32(fd, (f32)(i32)get(fs));

    if (ENABLE_DISASM) {
        std::printf("[%s] CVT.S.W F%u, F%u; F%u = %f\n", fpuName[cpuID], fd, fs, fd, getF32(fd));
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

void FPU::doSingle(u32 instr) {
    const auto opcode = instr & 0x3F;

    switch ((SingleOpcode)opcode) {
        case SingleOpcode::MUL:
            iMUL(instr);
            break;
        default:
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
