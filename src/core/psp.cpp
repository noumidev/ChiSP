/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "psp.hpp"

#include <cstdio>

#include "ata.hpp"
#include "display.hpp"
#include "ge.hpp"
#include "hpremote.hpp"
#include "i2c.hpp"
#include "memory.hpp"
#include "nand.hpp"
#include "scheduler.hpp"
#include "syscon.hpp"
#include "systime.hpp"
#include "allegrex/allegrex.hpp"
#include "allegrex/interpreter.hpp"
#include "crypto/kirk.hpp"
#include "crypto/spock.hpp"

namespace psp {

using namespace allegrex;

Allegrex cpu, me;

void init(const char *bootPath, const char *nandPath) {
    memory::init(bootPath);
    nand::init(nandPath);

    cpu.init(Type::Allegrex);
    me.init(Type::MediaEngine);

    // MediaEngine is booted later on
    me.isHalted = true;

    display::init();
    ge::init();
    hpremote::init();
    i2c::init();
    kirk::init();
    spock::init();
    syscon::init();
    systime::init();
    ata::init();

    std::puts("[PSP     ] OK");
}

void run() {
    while (true) {
        const auto runCycles = scheduler::getRunCycles();

        interpreter::run(&cpu, runCycles);
        interpreter::run(&me , runCycles >> 1);
    }
}

void setIRQPending(bool irqPending) {
    cpu.setIRQPending(irqPending);
}

void meSetIRQPending(bool irqPending) {
    me.setIRQPending(irqPending);
}

void resetCPU() {
    cpu.reset();

    memory::unmapBootROM();
}

void resetME() {
    me.reset();
}

}
