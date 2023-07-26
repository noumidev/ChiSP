/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::intc {

enum class InterruptSource {
    KIRK = 24,
};

u32  read (u32 addr);
void write(u32 addr, u32 data);

void sendIRQ(InterruptSource irqSource);

}
