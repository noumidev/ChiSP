/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "i2c.hpp"

#include <cassert>
#include <cstdio>

namespace psp::i2c {

u32 command, length, irqstatus;

u32 unknown[5];

enum class I2CReg {
    UNKNOWN0 = 0x1E200000,
    COMMAND = 0x1E200004,
    LENGTH  = 0x1E200008,
    DATA = 0x1E20000C,
    UNKNOWN1 = 0x1E200010,
    UNKNOWN2 = 0x1E200014,
    UNKNOWN3 = 0x1E20001C,
    IRQSTATUS = 0x1E200028,
    UNKNOWN4 = 0x1E20002C,
};

u32 read(u32 addr) {
    switch ((I2CReg)addr) {
        case I2CReg::UNKNOWN0:
            std::printf("[I2C     ] Unknown read @ 0x%08X\n", addr);

            return unknown[0];
        case I2CReg::COMMAND:
            std::printf("[I2C     ] Read @ COMMAND\n");

            return command;
        case I2CReg::LENGTH:
            std::printf("[I2C     ] Read @ LENGTH\n");

            return length;
        case I2CReg::DATA:
            std::printf("[I2C     ] Read @ DATA\n");

            return 0;
        case I2CReg::UNKNOWN1:
            std::printf("[I2C     ] Unknown read @ 0x%08X\n", addr);

            return unknown[1];
        case I2CReg::UNKNOWN2:
            std::printf("[I2C     ] Unknown read @ 0x%08X\n", addr);

            return unknown[2];
        case I2CReg::UNKNOWN3:
            std::printf("[I2C     ] Unknown read @ 0x%08X\n", addr);

            return unknown[3];
        case I2CReg::IRQSTATUS:
            std::printf("[I2C     ] Read @ IRQSTATUS\n");

            return irqstatus;
        default:
            std::printf("[I2C     ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    switch ((I2CReg)addr) {
        case I2CReg::COMMAND:
            std::printf("[I2C     ] Write @ COMMAND = 0x%08X\n", data);

            command = data;

            irqstatus |= 1;
            break;
        case I2CReg::LENGTH:
            std::printf("[I2C     ] Write @ LENGTH = 0x%08X\n", data);

            length = data;
            break;
        case I2CReg::DATA:
            std::printf("[I2C     ] Write @ DATA = 0x%08X\n", data);
            break;
        case I2CReg::UNKNOWN1:
            std::printf("[I2C     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[1] = data;
            break;
        case I2CReg::UNKNOWN2:
            std::printf("[I2C     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[2] = data;
            break;
        case I2CReg::UNKNOWN3:
            std::printf("[I2C     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[3] = data;
            break;
        case I2CReg::IRQSTATUS:
            std::printf("[I2C     ] Write @ IRQSTATUS = 0x%08X\n", data);

            irqstatus &= ~data;
            break;
        case I2CReg::UNKNOWN4:
            std::printf("[I2C     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[4] = data;
            break;
        default:
            std::printf("[I2C     ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
