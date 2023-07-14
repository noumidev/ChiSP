/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::syscon {

u32  read (u32 addr);
void write(u32 addr, u32 data);

}