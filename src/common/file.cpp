/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#include "common/file.hpp"

#include <cstdio>
#include <cstdlib>

#include <plog/Log.h>

void readFile(const char *path, u8 *data, size_t size) {
    // Try opening the file
    FILE *file = std::fopen(path, "rb");

    if (file == NULL) {
        PLOG_FATAL << "Unable to open file " << path;

        exit(0);
    }

    // Move file pointer to beginning of file
    std::fseek(file, 0, SEEK_SET);

    // Try reading `size` bytes into array
    if (std::fread(data, sizeof(u8), size, file) != size) {
        PLOG_FATAL << "Failed to read file " << path;

        exit(0);
    }

    std::fclose(file);
}
