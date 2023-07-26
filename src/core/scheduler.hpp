/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include <functional>

#include "../common/types.hpp"

namespace psp::scheduler {

u64 registerEvent(std::function<void(int)> func);

void addEvent(u64 id, int param, i64 cyclesUntilEvent);

i64 getRunCycles();

}
