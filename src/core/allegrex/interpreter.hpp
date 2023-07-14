/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../../common/types.hpp"

namespace psp::allegrex {

struct Allegrex;

}

namespace psp::allegrex::interpreter {

void run(Allegrex *allegrex, i64 runCycles);

}
