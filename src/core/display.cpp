/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "display.hpp"

#include <cassert>
#include <cstdio>

#include "ge.hpp"
#include "intc.hpp"
#include "scheduler.hpp"

namespace psp::display {

constexpr i64 VSYNC_CYCLES = 333000000 / 60;

u64 idVsync;

// Send VSYNC interrupt
void vsync() {
    intc::sendIRQ(intc::InterruptSource::VSYNC);

    ge::drawScreen();

    scheduler::addEvent(idVsync, 0, VSYNC_CYCLES);
}

void init() {
    idVsync = scheduler::registerEvent([](int) {vsync();});

    scheduler::addEvent(idVsync, 0, VSYNC_CYCLES);
}

u32 read(u32 addr) {
    std::printf("[Display ] Unhandled read @ 0x%08X\n", addr);

    return 0;
}

void write(u32 addr, u32 data) {
    std::printf("[Display ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
}

}
