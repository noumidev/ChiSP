/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#pragma once

#include <functional>

#include "common/types.hpp"

namespace sys::scheduler {

constexpr i64 ONE_MICROSECOND = 333;

void init();
void deinit();

void reset();

u64 registerEvent(const std::function<void(int)> &func);

void addEvent(const u64 id, const int param, const i64 eventCycles);

i64 getRunCycles();

void run(const i64 runCycles);

}
