/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#pragma once

#include "common/types.hpp"

namespace sys::memory {

// Memory region base addresses
namespace MemoryBase {
    enum : u32 {
        BootROM = 0x1FC00000,
        AddressSpace = 0x20000000,
    };
}

void init(const char *bootPath);
void deinit();

void reset();

// Reads data from system memory
template<typename T>
T read(u32 vaddr);

// Writes data to system memory
template<typename T>
void write(u32 vaddr, T data);

}
