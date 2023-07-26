/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "systime.hpp"

#include <cassert>
#include <cstdio>

#include "intc.hpp"
#include "scheduler.hpp"

namespace psp::systime {

constexpr i64 SYSTIME_CYCLES = 333;

enum class SysTimeReg {
    TIME  = 0x1C600000,
    ALARM = 0x1C600004,
    UNKNOWN0 = 0x1C600008,
    UNKNOWN1 = 0x1C60000C,
    UNKNOWN2 = 0x1C600010,
};

u32 time, alarm;

u64 idClockSysTime;

void clockSysTime() {
    ++time;

    if (time == alarm) {
        intc::sendIRQ(intc::InterruptSource::SysTime);
    }

    scheduler::addEvent(idClockSysTime, 0, SYSTIME_CYCLES);
}

void init() {
    idClockSysTime = scheduler::registerEvent([](int) {clockSysTime();});

    scheduler::addEvent(idClockSysTime, 0, SYSTIME_CYCLES);
}

u32 read(u32 addr) {
    switch ((SysTimeReg)addr) {
        case SysTimeReg::TIME:
            std::printf("[SysTime ] Read @ TIME\n");
            return time;
        case SysTimeReg::ALARM:
            std::printf("[SysTime ] Read @ ALARM\n");
            return alarm;
        case SysTimeReg::UNKNOWN0:
        case SysTimeReg::UNKNOWN1:
        case SysTimeReg::UNKNOWN2:
            std::printf("[SysTime ] Unknown read @ 0x%08X\n", addr);
            return 0;
        default:
            std::printf("[SysTime ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    switch ((SysTimeReg)addr) {
        case SysTimeReg::TIME:
            std::printf("[SysTime ] Write @ TIME = 0x%08X\n", data);

            time = data;

            intc::clearIRQ(intc::InterruptSource::SysTime);
            break;
        case SysTimeReg::ALARM:
            std::printf("[SysTime ] Write @ ALARM = 0x%08X\n", data);
            
            alarm = data;

            intc::clearIRQ(intc::InterruptSource::SysTime);
            break;
        case SysTimeReg::UNKNOWN0:
        case SysTimeReg::UNKNOWN1:
        case SysTimeReg::UNKNOWN2:
            std::printf("[SysTime ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);
            break;
        default:
            std::printf("[SysTime ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
