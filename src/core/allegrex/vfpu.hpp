/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../../common/types.hpp"

namespace psp::allegrex::vfpu {

u32 getControl(int idx);

// Matrix reads
void readMtxQuadword(int vt, u32 *data);

}
