/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "cy27040.hpp"

#include <cassert>
#include <cstdio>

namespace psp::cy27040 {

namespace ClockgenReg {
    enum : u8 {
        All = 0x00,
        Revision = 0x80,
        ClockControl = 0x81,
        SpreadSpectrumControl = 0x82,
    };
};

constexpr u8 REVISION = 4;

u8 clockControl, spreadSpectrumControl;

void transmit(u8 *txData) {
    std::puts("[CY27040 ] Transmit");

    const auto addr = txData[0];

    switch (addr) {
        case ClockgenReg::ClockControl:
            clockControl = txData[1];

            std::printf("[CY27040 ] Set Clock Control = 0x%02X\n", clockControl);
            break;
        default:
            std::printf("Unhandled CY27040 address 0x%02X\n", addr);

            exit(0);
    }
}

void transmitAndReceive(u8 *txData, std::queue<u8> &rxQueue) {
    std::puts("[CY27040 ] Transmit and Receive");

    const auto addr = txData[0];

    switch (addr) {
        case ClockgenReg::All:
            std::puts("[CY27040 ] Get all registers");

            rxQueue.push(3);
            rxQueue.push(REVISION);
            rxQueue.push(clockControl);
            rxQueue.push(spreadSpectrumControl);
            break;
        default:
            std::printf("Unhandled CY27040 address 0x%02X\n", addr);

            exit(0);
    }
}

}
