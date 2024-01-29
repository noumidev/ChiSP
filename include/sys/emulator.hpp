/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#pragma once

namespace sys::emulator {

void init(const char *bootPath, const char *nandPath);
void deinit();

void reset();

void run();

}
