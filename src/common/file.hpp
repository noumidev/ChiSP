/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include <cstdio>

#include "types.hpp"

// Returns true on success
bool loadFile(const char *path, u8 *buf, i64 size) {
    const auto file = std::fopen(path, "rb");

    if (file == NULL) return false;

    // Check file size
    std::fseek(file, 0, SEEK_END);
    const auto fileSize = std::ftell(file);

    if (fileSize < size) { // Abort if file is too small
        std::fclose(file);

        return false;
    }

    // Seek back to beginning, read file into buffer
    std::fseek(file, 0, SEEK_SET);
    std::fread(buf, sizeof(u8), size, file);
    std::fclose(file);

    return true;
}
