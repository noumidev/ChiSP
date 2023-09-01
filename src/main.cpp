/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include <cstdio>

#include "core/psp.hpp"

int main(int argc, char **argv) {
    if (argc < 3) {
        std::puts("Usage: ChiSP boot.bin nand.bin [umd.iso]");

        return -1;
    }

    if (argc == 3) {
        psp::init(argv[1], argv[2], NULL); // Last argument *can* be NULL!
    } else {
        psp::init(argv[1], argv[2], argv[3]);
    }

    psp::run();

    return 0;
}
