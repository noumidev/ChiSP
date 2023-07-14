/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "psp.hpp"

#include <cstdio>

#include "memory.hpp"

namespace psp {

void init(const char *bootPath) {
    memory::init(bootPath);

    std::puts("[PSP     ] OK");
}

}
