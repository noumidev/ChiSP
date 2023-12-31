/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::memory {

enum class MemoryBase {
    MESPRAM = 0x00000000,
    SPRAM = 0x00010000,
    EDRAM = 0x04000000,
    VME0 = 0x040FF000,
    DRAM  = 0x08000000,
    MEMPROT = 0x1C000000,
    SysCon  = 0x1C100000,
    INTC  = 0x1C300000,
    Timer = 0x1C500000,
    SysTime = 0x1C600000,
    DMACplus = 0x1C800000,
    DMAC0 = 0x1C900000,
    DMAC1 = 0x1CA00000,
    VME1 = 0x1CC00000,
    DDR = 0x1D000000,
    NAND = 0x1D101000,
    MS  = 0x1D200000,
    WLAN = 0x1D300000,
    GE  = 0x1D400000,
    ATA0 = 0x1D600000,
    ATA1 = 0x1D700000,
    KIRK = 0x1DE00000,
    SPOCK = 0x1DF00000,
    Audio = 0x1E000000,
    LCDC = 0x1E140000,
    I2C  = 0x1E200000,
    GPIO = 0x1E240000,
    POWERMAN = 0x1E300000,
    UART0 = 0x1E4C0000,
    HPRemote = 0x1E500000,
    SysConSerial = 0x1E580000,
    Display = 0x1E740000,
    BootROM = 0x1FC00000,
    SharedRAM  = 0x1FD00000,
    NANDBuffer = 0x1FF00000,
    PAddrSpace = 0x20000000,
};

enum class MemorySize {
    SPRAM = 0x4000,
    EDRAM = 0x200000,
    VME0 = 0x1000,
    DRAM  = 0x2000000,
    MEMPROT = 0x54,
    SysCon  = 0x104,
    INTC  = 0x2C,
    Timer = 0x404,
    SysTime = 0x14,
    DMACplus = 0x1E0,
    DMAC = 0x200,
    VME1 = 0x74,
    DDR = 0x48,
    NAND  = 0x304,
    MS  = 0x44,
    WLAN = 0x44,
    GE  = 0xE80,
    ATA0  = 0x48,
    ATA1  = 0x0F,
    KIRK  = 0x54,
    SPOCK = 0x98,
    Audio = 0xD4,
    LCDC  = 0x74,
    I2C = 0x30,
    UART  = 0x48,
    GPIO  = 0x4C,
    POWERMAN = 0x60,
    SysConSerial = 0x28,
    Display = 0x28,
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

void read128 (u32 addr, u8 *data);
void write128(u32 addr, u8 *data);

u8  meRead8 (u32 addr);
u16 meRead16(u32 addr);
u32 meRead32(u32 addr);

void meWrite8 (u32 addr, u8  data);
void meWrite16(u32 addr, u16 data);
void meWrite32(u32 addr, u32 data);

void unmapBootROM();

}
