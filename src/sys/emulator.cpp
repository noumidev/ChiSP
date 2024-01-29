/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#include "sys/emulator.hpp"

#include <plog/Log.h>

#include "sys/memory.hpp"

namespace sys::emulator {

void init(const char *bootPath, const char *nandPath) {
    (void)nandPath;

    memory::init(bootPath);
}

void deinit() {
    memory::deinit();
}

void reset() {
    memory::reset();
}

void run() {}

}
