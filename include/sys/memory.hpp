/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#pragma once

#include <type_traits>

#include "common/types.hpp"

namespace sys::memory {

constexpr u32 PAGE_SHIFT = 12;
constexpr u32 PAGE_SIZE = 1 << PAGE_SHIFT;
constexpr u32 PAGE_MASK = PAGE_SIZE - 1;

// Memory region base addresses
namespace MemoryBase {
    enum : u32 {
        BootROM = 0x1FC00000,
    };
}

// Memory region sizes
namespace MemorySize {
    enum : u32 {
        BootROM = 0x1000,
        AddressSpace = 0x20000000,
    };
}

constexpr int PAGE_NUM = MemorySize::AddressSpace >> PAGE_SHIFT;

inline u32 maskAddress(const u32 vaddr) {
    return vaddr & (MemorySize::AddressSpace - 1);
}

void init(const char *bootPath);
void deinit();

void reset();

// Returns a pointer to raw memory (unsafe!)
void *getPointer(u32 vaddr);

// Reads data from system memory
template<typename T>
T read(u32 vaddr) requires std::is_unsigned_v<T>;

// Writes data to system memory
template<typename T>
void write(u32 vaddr, const T data) requires std::is_unsigned_v<T>;

}
