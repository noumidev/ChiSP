/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "psp.hpp"

#include <cstdio>

#include "memory.hpp"
#include "allegrex/allegrex.hpp"
#include "allegrex/interpreter.hpp"

namespace psp {

using namespace allegrex;

Allegrex cpu;

void init(const char *bootPath) {
    memory::init(bootPath);

    cpu.init(Type::Allegrex);

    std::puts("[PSP     ] OK");
}

void run() {
    while (true) {
        interpreter::run(&cpu, 128);
    }
}

}
