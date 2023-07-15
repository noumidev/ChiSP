/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

namespace psp {

void init(const char *bootPath, const char *nandPath);
void resetCPU();

void run();

}
