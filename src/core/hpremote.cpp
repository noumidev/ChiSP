/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "hpremote.hpp"

#include <cassert>
#include <cstdio>

#include "intc.hpp"
#include "scheduler.hpp"

namespace psp::hpremote {

//constexpr i64 HP_OP_CYCLES = 1024;

u64 idSendIRQ;

// Send HP remote interrupt
void sendIRQ() {
    intc::sendIRQ(intc::InterruptSource::HPRemote);
}

void init() {
    idSendIRQ = scheduler::registerEvent([](int) {sendIRQ();});
}

u32 read(u32 addr) {
    std::printf("[HPRemote] Unhandled read @ 0x%08X\n", addr);

    if (addr == 0x1E500018) {
        return 0x10;
    }

    return 0;
}

void write(u32 addr, u32 data) {
    std::printf("[HPRemote] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
}

}
