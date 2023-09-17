/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "i2c.hpp"

#include <cassert>
#include <cstdio>
#include <queue>

#include "cy27040.hpp"
#include "intc.hpp"
#include "scheduler.hpp"
#include "wm8750.hpp"

namespace psp::i2c {

constexpr i64 I2C_OP_CYCLES = 20 * scheduler::_1US;

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

namespace I2CDevice {
    enum : u8 {
        WM8750 = 0x34,
        CY27040 = 0xD2,
    };
};

u32 command, length, irqstatus;

// 16 bytes each according to uofw
u8 txData[16];
u32 txPtr;

std::queue<u8> rxQueue;

u32 unknown[5];

u64 idFinishTransfer;

void checkInterrupt() {
    if (irqstatus) {
        intc::sendIRQ(intc::InterruptSource::I2C);
    } else {
        intc::clearIRQ(intc::InterruptSource::I2C);
    }
}

void finishTransfer() {
    irqstatus |= 1;

    checkInterrupt();
}

void init() {
    idFinishTransfer = scheduler::registerEvent([](int) {finishTransfer();});
}

void doCommand() {
    switch (command) {
        case 0x85:
            std::puts("[I2C     ] Command 0x85");
            break;
        case 0x87:
            {
                std::puts("[I2C     ] Command 0x87 (Transmit)");

                // Send data to I2C device
                const auto deviceAddr = txData[0];

                switch (deviceAddr) {
                    case I2CDevice::WM8750:
                        wm8750::transmit(&txData[1]);
                        break;
                    case I2CDevice::CY27040:
                        cy27040::transmit(&txData[1]);
                        break;
                    default:
                        std::printf("Unhandled I2C address 0x%02X\n", deviceAddr);

                        exit(0);
                }
            }
            break;
        case 0x8A:
            {
                std::puts("[I2C     ] Command 0x8A (Transmit and Receive)");

                // Send data to I2C device (kernel sets bit 0; clear it)
                const auto deviceAddr = txData[0] ^ 1;

                switch (deviceAddr) {
                    case I2CDevice::CY27040:
                        cy27040::transmitAndReceive(&txData[1], rxQueue);
                        break;
                    default:
                        std::printf("Unhandled I2C address 0x%02X\n", deviceAddr);

                        exit(0);
                }
            }
            break;
        default:
            std::printf("Unhandled I2C command 0x%02X\n", command);

            exit(0);
    }
    
    scheduler::addEvent(idFinishTransfer, 0, I2C_OP_CYCLES);
}

u8 getRxQueue() {
    if (rxQueue.empty()) {
        return 0;
    }

    const auto data = rxQueue.front(); rxQueue.pop();

    return data;
}

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

            return getRxQueue();
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

            doCommand();
            break;
        case I2CReg::LENGTH:
            std::printf("[I2C     ] Write @ LENGTH = 0x%08X\n", data);

            // Actually length + 1? Doesn't matter
            length = data;

            txPtr = 0;
            break;
        case I2CReg::DATA:
            std::printf("[I2C     ] Write @ DATA = 0x%08X\n", data);

            assert(txPtr < sizeof(txData));
            
            txData[txPtr++] = data;
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

            checkInterrupt();
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
