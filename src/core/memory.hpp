/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::memory {

enum class MemoryBase {
    BootROM = 0x1FC00000,
};

enum class MemorySize {
    BootROM = 0x1000,
};

void init(const char *bootPath);

}
