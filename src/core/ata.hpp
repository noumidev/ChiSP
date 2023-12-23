/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::ata {

void finishSCSICommand();

void init(const char *path);

// ATA0
u32  ata0Read (u32 addr);
void ata0Write(u32 addr, u32 data);

// ATA1
u8  ata1Read8(u32 addr);
u16 ata1Read16(u32 addr);

void ata1Write8 (u32 addr, u8  data);
void ata1Write16(u32 addr, u16 data);

bool isUMDInserted();

}
