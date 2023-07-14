/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include <cstdio>

#include "core/psp.hpp"

int main(int argc, char **argv) {
    if (argc < 3) {
        std::puts("Usage: ChiSP boot.bin nand.bin");

        return -1;
    }

    psp::init(argv[1], argv[2]);
    psp::run();

    return 0;
}
