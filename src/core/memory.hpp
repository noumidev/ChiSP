/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::memory {

enum class MemoryBase {
    SPRAM = 0x00010000,
    EDRAM = 0x04000000,
    DRAM  = 0x08000000,
    MEMPROT = 0x1C000000,
    SysCon  = 0x1C100000,
    INTC  = 0x1C300000,
    Timer = 0x1C500000,
    SysTime = 0x1C600000,
    DMACplus = 0x1C800000,
    DDR = 0x1D000000,
    NAND = 0x1D101000,
    GE  = 0x1D400000,
    KIRK = 0x1DE00000,
    LCDC = 0x1E140000,
    I2C  = 0x1E200000,
    GPIO = 0x1E240000,
    POWERMAN = 0x1E300000,
    UART0 = 0x1E4C0000,
    SysConSerial = 0x1E580000,
    BootROM = 0x1FC00000,
    SharedRAM  = 0x1FD00000,
    NANDBuffer = 0x1FF00000,
    PAddrSpace = 0x20000000,
};

enum class MemorySize {
    SPRAM = 0x4000,
    EDRAM = 0x200000,
    DRAM  = 0x2000000,
    MEMPROT = 0x54,
    SysCon  = 0x104,
    INTC  = 0x2C,
    Timer = 0x404,
    SysTime = 0x14,
    DMACplus = 0x1E0,
    DDR = 0x48,
    NAND  = 0x304,
    GE  = 0xE80,
    KIRK  = 0x54,
    LCDC  = 0x74,
    I2C = 0x30,
    UART  = 0x48,
    GPIO  = 0x4C,
    POWERMAN = 0x60,
    SysConSerial = 0x28,
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

void unmapBootROM();

}
