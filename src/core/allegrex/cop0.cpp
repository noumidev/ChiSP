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

enum class ControlReg {
    V0 = 0x04,
    ErrorHandler = 0x09,
    Unknown17 = 0x11,
};

enum class StatusReg {
    Status = 0x0C,
    Cause  = 0x0D,
    Config = 0x10,
    TagLo = 0x1C,
    TagHi = 0x1D,
};

void COP0::init(int cpuID) {
    assert((cpuID >= 0) && (cpuID < 2));

    this->cpuID = cpuID;

    std::printf("[%s] OK\n", cop0Name[cpuID]);
}

u32 COP0::getControl(int idx) {
    switch ((ControlReg)idx) {
        default:
            std::printf("Unhandled %s control read @ %d\n", cop0Name[cpuID], idx);

            exit(0);
    }
}

void COP0::setControl(int idx, u32 data) {
    switch ((ControlReg)idx) {
        case ControlReg::V0:
            v0 = data;
            break;
        case ControlReg::ErrorHandler:
            errorHandler = data;
            break;
        case ControlReg::Unknown17:
            unknown17 = data;
            break;
        default:
            std::printf("Unhandled %s control write @ %d = 0x%08X\n", cop0Name[cpuID], idx, data);

            exit(0);
    }
}

u32 COP0::getStatus(int idx) {
    switch ((StatusReg)idx) {
        case StatusReg::Config:
            return CONFIG;
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
        case StatusReg::Status:
            status = data;
            break;
        case StatusReg::Cause:
            cause = data;
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
