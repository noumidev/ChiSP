/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "memory.hpp"

#include <array>
#include <cassert>
#include <cstdio>

#include "../common/file.hpp"

namespace psp::memory {

// PSP system memory
std::array<u8, (u64)MemorySize::BootROM> bootROM;

void init(const char *bootPath) {
    std::printf("[Memory  ] Loading boot ROM \"%s\"\n", bootPath);
    assert(loadFile(bootPath, bootROM.data(), (u64)MemorySize::BootROM));

    std::puts("[Memory  ] OK");
}

}
