/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../../common/types.hpp"

namespace psp::umd::ata {

void init();

// ATA0
u32  readATA0 (u32 addr);
void writeATA0(u32 addr, u32 data);

// ATA1
u8   readATA1 (u32 addr);
void writeATA1(u32 addr, u8 data);

}
