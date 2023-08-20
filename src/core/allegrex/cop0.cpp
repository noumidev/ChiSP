/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "cop0.hpp"

#include <cassert>
#include <cstdio>

#include "allegrex.hpp"

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
    EPC = 0x0E,
    Config = 0x10,
    SCCode = 0x15,
    CPUId = 0x16,
    EBase = 0x19,
    TagLo = 0x1C,
    TagHi = 0x1D,
};

enum Cause {
    EXCODE = 0x1F << 2,
    IP  = 0xFF << 8,
    IP0 = 1 << 10,
    IP5 = 1 << 15,
    BD  = 1 << 31,
};

enum Status {
    IE  = 1 << 0,
    EXL = 1 << 1,
    ERL = 1 << 2,
    IM  = 0xFF << 8,
    BEV = 1 << 22,
};

void COP0::init(Allegrex *allegrex, int cpuID) {
    assert((cpuID >= 0) && (cpuID < 2));

    this->allegrex = allegrex;
    this->cpuID = cpuID;

    compare = -1;

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
        case StatusReg::Cause:
            return cause;
        case StatusReg::EPC:
            return epc;
        case StatusReg::Config:
            return CONFIG;
        case StatusReg::SCCode:
            return scCode;
        case StatusReg::CPUId:
            return cpuID;
        case StatusReg::EBase:
            return ebase;
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
            std::printf("COUNT: 0x%08X\n", data);
            count = oldCount = data;
            break;
        case StatusReg::Compare:
            std::printf("COMPARE: 0x%08X\n", data);
            compare = data;

            setCountPending(false);
            break;
        case StatusReg::Status:
            status = data;

            allegrex->checkInterrupt();
            break;
        case StatusReg::Cause:
            cause = (cause & 0xFFFFFCFF) | (data & 0x300);

            allegrex->checkInterrupt();
            break;
        case StatusReg::EPC:
            epc = data;
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

void COP0::runCount(i64 runCycles) {
    oldCount = count;

    count += runCycles;

    if ((oldCount < compare) && (count >= compare)) {
        std::printf("COUNT >= COMPARE (0x%08X 0x%08X)\n", count, compare);

        setCountPending(true);

        allegrex->checkInterrupt();
    }
}

bool COP0::isCOPUsable(int copN) {
    return (!copN || (status & (1 << (28 + copN))));
}

u32 COP0::getEBase() {
    return ebase;
}

void COP0::setEPC(u32 pc) {
    epc = pc;
}

bool COP0::isBEV() {
    return status & Status::BEV;
}

bool COP0::isEXL() {
    return status & Status::EXL;
}

void COP0::setEXL(bool exl) {
    status &= ~Status::EXL;
    status |= (u32)exl << 1;
}

// IC is Status.IE
bool COP0::getIC() {
    return status & Status::IE;
}

// IC is Status.IE
void COP0::setIC(bool ic) {
    status &= ~Status::IE;
    status |= (u32)ic;
}

void COP0::setEXCODE(Exception excode) {
    cause &= ~Cause::EXCODE;
    cause |= (u32)excode << 2;
}

void COP0::setBD(bool bd) {
    cause &= ~Cause::BD;
    cause |= (u32)bd << 31;
}

bool COP0::isInterruptPending() {
    return (status & Status::IE) && !(status & Status::EXL) && !(status & Status::ERL) && ((status & Status::IM) & (cause & Cause::IP));
}

void COP0::setIRQPending(bool irqPending) {
    // Clear old interrupt pending bit, set new value
    cause &= ~Cause::IP0;
    cause |= (u32)irqPending << 10;
}

void COP0::setCountPending(bool countPending) {
    // Clear count pending bit, set new value
    cause &= ~Cause::IP5;
    cause |= (u32)countPending << 15;
}

void COP0::setSyscallCode(u32 code) {
    scCode = code << 2;
}

// Return appropriate EPC, clear exception/error flag
u32 COP0::exceptionReturn() {
    u32 pc;
    if (status & Status::ERL) {
        std::puts("Unhandled error return");

        exit(0);
    } else {
        pc = epc;

        status &= ~Status::EXL;
    }

    return pc;
}

}
