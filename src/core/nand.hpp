/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::nand {

void init(const char *nandPath);

u32  read (u32 addr);
void write(u32 addr, u32 data);

u32 readBuffer32(u32 addr);

}
