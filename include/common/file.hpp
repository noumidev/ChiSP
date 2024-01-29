/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#pragma once

#include "types.hpp"

// Reads data from file `path` into array `data`
void readFile(const char *path, u8 *const data, const size_t size);
