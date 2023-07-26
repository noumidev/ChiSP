/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "psp.hpp"

#include <cstdio>

#include "memory.hpp"
#include "nand.hpp"
#include "scheduler.hpp"
#include "allegrex/allegrex.hpp"
#include "allegrex/interpreter.hpp"
#include "crypto/kirk.hpp"

namespace psp {

using namespace allegrex;

Allegrex cpu;

void init(const char *bootPath, const char *nandPath) {
    memory::init(bootPath);
    nand::init(nandPath);

    cpu.init(Type::Allegrex);

    kirk::init();

    std::puts("[PSP     ] OK");
}

void resetCPU() {
    static auto hasReset = false;

    if (!hasReset) {
        cpu.reset();

        memory::unmapBootROM();

        hasReset = true;
    }
}

void run() {
    while (true) {
        const auto runCycles = scheduler::getRunCycles();

        interpreter::run(&cpu, runCycles);
    }
}

}
