/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::syscon {

void init();

u32 read(int cpuID, u32 addr);
u32 readSerial(u32 addr);

void write(int cpuID, u32 addr, u32 data);
void writeSerial(u32 addr, u32 data);

}
