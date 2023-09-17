/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include <functional>

#include "../common/types.hpp"

namespace psp::scheduler {

constexpr i64 _1US = 333;

u64 registerEvent(std::function<void(int)> func);

void addEvent(u64 id, int param, i64 cyclesUntilEvent);

i64 getRunCycles();

void run(i64 runCycles);

}
