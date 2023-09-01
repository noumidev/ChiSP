/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp {

void init(const char *bootPath, const char *nandPath, const char *umdPath);
void run();

void update(u8 *fb);

void setIRQPending(bool irqPending);
void meSetIRQPending(bool irqPending);

void resetCPU();
void resetME();

void postME();

}
