/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::memory {

enum class MemoryBase {
    SPRAM = 0x00010000,
    SysCon  = 0x1C100000,
    NAND  = 0x1D101000,
    KIRK  = 0x1DE00000,
    GPIO  = 0x1E240000,
    BootROM = 0x1FC00000,
    IRAM  = 0x1FD00000,
    NANDBuffer = 0x1FF00000,
    PAddrSpace = 0x20000000,
};

enum class MemorySize {
    SPRAM = 0x4000,
    SysCon  = 0xE8,
    NAND  = 0x304,
    KIRK  = 0x54,
    GPIO  = 0x4C,
    BootROM = 0x1000,
    NANDBuffer = 0x910,
};

void init(const char *bootPath);

u8 *getMemoryPointer(u32 addr);

// Allegrex read/write handlers
u8  read8 (u32 addr);
u16 read16(u32 addr);
u32 read32(u32 addr);

void write8 (u32 addr, u8  data);
void write16(u32 addr, u16 data);
void write32(u32 addr, u32 data);

// TODO: MediaEngine read/write handlers

}
