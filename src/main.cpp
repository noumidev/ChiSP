/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#include <plog/Init.h>
#include <plog/Log.h>
#include <plog/Formatters/FuncMessageFormatter.h>
#include <plog/Appenders/ColorConsoleAppender.h>

#include "sys/emulator.hpp"

int main(int argc, char **argv) {
    // Set up logger
    static plog::ColorConsoleAppender<plog::FuncMessageFormatter> consoleAppender;
    plog::init(plog::verbose, &consoleAppender);

    if (argc < 3) {
        PLOG_FATAL << "Usage: ChiSP preipl.bin nand.bin";

        return -1;
    }

    sys::emulator::init(argv[1], argv[2]);
    sys::emulator::run();

    sys::emulator::deinit();

    return 0;
}
