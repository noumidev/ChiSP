/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#include "sys/emulator.hpp"

#include <plog/Log.h>

#include "hw/allegrex/cpu.hpp"

#include "sys/memory.hpp"
#include "sys/scheduler.hpp"

namespace sys::emulator {

using hw::allegrex::CPU;
using hw::allegrex::CPUType;

CPU<CPUType::Allegrex> cpu;
CPU<CPUType::MediaEngine> me;

void init(const char *bootPath, const char *nandPath) {
    (void)nandPath;

    cpu.init();
    me.init();

    memory::init(bootPath);
    scheduler::init();
}

void deinit() {
    memory::deinit();
    scheduler::deinit();
}

void reset() {
    cpu.reset();
    me.reset();

    memory::reset();
    scheduler::reset();
}

void run() {}

}
