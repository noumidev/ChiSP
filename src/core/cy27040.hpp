/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include <queue>

#include "../common/types.hpp"

namespace psp::cy27040 {

void transmit(u8 *txData);
void transmitAndReceive(u8 *txData, std::queue<u8> &rxQueue);

}
