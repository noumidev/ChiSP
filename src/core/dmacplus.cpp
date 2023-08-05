/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "dmacplus.hpp"

#include <cassert>
#include <cstdio>

#include "intc.hpp"
#include "scheduler.hpp"

namespace psp::dmacplus {

u64 idFinishTransfer;

void finishTransfer(int chnID) {
    (void)chnID; // ?????

    intc::sendIRQ(intc::InterruptSource::DMACplus);
}

void init() {
    idFinishTransfer = scheduler::registerEvent([](int chnID) {finishTransfer(chnID);});

    //scheduler::addEvent(idVsync, 0, VSYNC_CYCLES);
}

u32 read(u32 addr) {
    if ((addr < 0x1C80001C) || (addr >= 0x1C800180)) {
        std::printf("[DMACplus] Unhandled read @ 0x%08X\n", addr);

        exit(0);
    } else {
        std::printf("[DMACplus] Unhandled read @ 0x%08X\n", addr);

        return 0;
    }
    
}

void write(u32 addr, u32 data) {
    if ((addr < 0x1C80001C) || (addr >= 0x1C800180)) {
        std::printf("[DMACplus] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

        exit(0);
    } else {
        std::printf("[DMACplus] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

        return;
    }
}

}
