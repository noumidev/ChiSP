/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "cop0.hpp"

#include <cassert>
#include <cstdio>

namespace psp::allegrex::cop0 {

constexpr u32 CONFIG = 0x480;

const char *cop0Name[] = {
    "COP0:CPU", "COP0:ME ",
};

enum class StatusReg {
    Count   = 0x09,
    Compare = 0x0B,
    Status = 0x0C,
    Cause  = 0x0D,
    Config = 0x10,
    CPUId = 0x16,
    EBase = 0x19,
    TagLo = 0x1C,
    TagHi = 0x1D,
};

void COP0::init(int cpuID) {
    assert((cpuID >= 0) && (cpuID < 2));

    this->cpuID = cpuID;

    std::printf("[%s] OK\n", cop0Name[cpuID]);
}

u32 COP0::getControl(int idx) {
    return cregs[idx];
}

void COP0::setControl(int idx, u32 data) {
    cregs[idx] = data;
}

u32 COP0::getStatus(int idx) {
    switch ((StatusReg)idx) {
        case StatusReg::Count:
            return count;
        case StatusReg::Compare:
            return compare;
        case StatusReg::Status:
            return status;
        case StatusReg::Config:
            return CONFIG;
        case StatusReg::CPUId:
            return cpuID;
        case StatusReg::TagLo:
            return tagLo;
        case StatusReg::TagHi:
            return tagHi;
        default:
            std::printf("Unhandled %s status read @ %d\n", cop0Name[cpuID], idx);

            exit(0);
    }
}

void COP0::setStatus(int idx, u32 data) {
    switch ((StatusReg)idx) {
        case StatusReg::Count:
            count = data;
            break;
        case StatusReg::Compare:
            compare = data;
            break;
        case StatusReg::Status:
            status = data;
            break;
        case StatusReg::Cause:
            cause = data;
            break;
        case StatusReg::EBase:
            ebase = data;
            break;
        case StatusReg::TagLo:
            tagLo = data;
            break;
        case StatusReg::TagHi:
            tagHi = data;
            break;
        default:
            std::printf("Unhandled %s status write @ %d = 0x%08X\n", cop0Name[cpuID], idx, data);

            exit(0);
    }
}

}
