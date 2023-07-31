/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace psp::intc {

enum class InterruptSource {
    UART = 0,
    GPIO = 4,
    I2C  = 12,
    SysTime = 19,
    NAND = 20,
    KIRK = 24,
    VSYNC = 30,
    HPRemote = 36,
};

u32  read (int cpuID, u32 addr);
void write(int cpuID, u32 addr, u32 data);

void sendIRQ(InterruptSource irqSource);
void meSendIRQ(InterruptSource irqSource);

}
