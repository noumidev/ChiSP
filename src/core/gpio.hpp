/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::gpio {

enum GPIOPin {
    SYSCON_START = 1 << 3,
    SYSCON_END = 1 << 4,
};

u32  read (u32 addr);
void write(u32 addr, u32 data);

void set(GPIOPin pin);
void clear(GPIOPin pin);

}
