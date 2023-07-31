/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "syscon.hpp"

#include <cassert>
#include <cstdio>
#include <queue>

#include "gpio.hpp"
#include "psp.hpp"
#include "scheduler.hpp"

namespace psp::syscon {

constexpr i64 SYSCON_OP_CYCLES = 1 << 16;

constexpr u32 BARYON_VERSION  = 0x00110001;
constexpr u32 TACHYON_VERSION = 0x40000001;

constexpr u32 FUSECONFIG = 0x00001100; // Matches Baryon version? TODO: check this

constexpr u32 BATTERY_TEMP = 20;         // 20C?
constexpr u32 BATTERY_VOLT = 4150;       // 4150mV
constexpr u32 BATTERY_ELEC = 4150;       // ??
constexpr u32 BATTERY_FULL_CAP = 1800;   // 1800mAh
constexpr u32 BATTERY_CURR_CAP = 1800;   // 1800mAh
constexpr u32 BATTERY_LIMIT_TIME = 400; // Taken from JPCSP

enum class SysConReg {
    NMIEN = 0x1C100000,
    NMIFLAG = 0x1C100004,
    UNKNOWN0 = 0x1C10003C,
    RAMSIZE = 0x1C100040,
    RESETEN = 0x1C10004C,
    BUSCLKEN  = 0x1C100050,
    CLKEN  = 0x1C100054,
    GPIOCLKEN = 0x1C100058,
    CLKSEL2 = 0x1C100060,
    SPICLK  = 0x1C100064,
    PLLFREQ = 0x1C100068,
    IOEN = 0x1C100078,
    GPIOEN = 0x1C10007C,
    FUSECONFIG = 0x1C100098,
    UNKNOWN1 = 0x1C1000FC,
};

enum class SysConSerialReg {
    INIT  = 0x1E580000,
    CONTROL = 0x1E580004,
    DATA  = 0x1E580008,
    FLAGS = 0x1E58000C,
    UNKNOWN0 = 0x1E580014,
    UNKNOWN1 = 0x1E580018,
    UNKNOWN2 = 0x1E580020,
    UNKNOWN3 = 0x1E580024,
};

enum class SysConCommand {
    GET_BARYON_VERSION = 0x01,
    GET_TACHYON_TEMP = 0x05,
    GET_KERNEL_DIGITAL_KEY = 0x07,
    READ_CLOCK = 0x09,
    READ_ALARM = 0x0A,
    GET_POWER_SUPPLY_STATUS = 0x0B,
    GET_WAKE_UP_FACTOR = 0x0E,
    GET_TIMESTAMP = 0x11,
    READ_SCRATCHPAD  = 0x24,
    SEND_SETPARAM = 0x25,
    CTRL_TACHYON_WDT = 0x31,
    CTRL_ANALOG_XY_POLLING = 0x33,
    CTRL_HR_POWER = 0x34,
    CTRL_VOLTAGE  = 0x42,
    GET_POWER_STATUS = 0x46,
    CTRL_LED = 0x47,
    BATTERY_GET_STATUS_CAP = 0x61,
    BATTERY_GET_TEMP = 0x62,
    BATTERY_GET_VOLT = 0x63,
    BATTERY_GET_ELEC = 0x64,
    BATTERY_GET_FULL_CAP = 0x67,
    BATTERY_GET_LIMIT_TIME = 0x69,
};

struct SysConRegs {
    // NMI registers
    u32 nmien, nmiflag;

    // Clock registers
    u32 busclken, gpioclken;

    // Enable
    u32 reseten, ioen, gpioen;

    u32 ramsize = TACHYON_VERSION, pllfreq = 333, spiclk;

    u32 unknown[2];
};

SysConRegs regs[2];

// Serial registers
u32 serialflags;

// SysCon commands
std::queue<u8> txQueue, rxQueue;

// SysCon internal registers
u32 powerStatus = 0;

u64 idFinishCommand;

u8 getTxQueue() {
    if (txQueue.empty()) {
        std::puts("SysCon TX queue is empty");

        exit(0);
    }

    const auto data = txQueue.front(); txQueue.pop();

    return data;
}

u16 getRxQueue() {
    if (rxQueue.empty()) {
        std::puts("SysCon RX queue is empty");

        exit(0);
    }

    u16 data = 0;
    data |= (u16)rxQueue.front() << 8; rxQueue.pop();

    if (!rxQueue.empty()) {
        data |= (u16)rxQueue.front() << 0; rxQueue.pop();
    }

    if (rxQueue.empty()) {
        serialflags &= ~(1 << 2);
    }

    return data;
};

void clearTxQueue() {
    while (!txQueue.empty()) {
        txQueue.pop();
    }
}

void writeResponse(u8 len) {
    rxQueue.push(len + 3); // Header + RX data
    rxQueue.push(0x82);    // Maybe??
}

// Reads a 32-bit syscon register
void commonRead(SysConCommand cmd) {
    writeResponse(4);

    u32 data;

    switch (cmd) {
        case SysConCommand::GET_BARYON_VERSION:
            std::puts("[SysCon  ] Get Baryon Version");

            data = BARYON_VERSION;
            break;
        case SysConCommand::GET_TACHYON_TEMP:
            std::puts("[SysCon  ] Get Tachyon Temp");

            data = 13094; // Kernel expects this command to return this value for some reason.
            break;
        case SysConCommand::GET_KERNEL_DIGITAL_KEY:
            std::puts("[SysCon  ] Get Kernel Digital Key");

            data = 0; // ??
            break;
        case SysConCommand::READ_CLOCK:
            std::puts("[SysCon  ] Read Clock");

            data = 0;
            break;
        case SysConCommand::READ_ALARM:
            std::puts("[SysCon  ] Read Alarm");

            data = 0;
            break;
        case SysConCommand::GET_POWER_SUPPLY_STATUS:
            std::puts("[SysCon  ] Get Power Supply Status");

            data = 0xC2; // Battery equipped and charging
            break;
        case SysConCommand::GET_WAKE_UP_FACTOR:
            std::puts("[SysCon  ] Get Wake Up Factor");

            data = 0; // ??
            break;
        case SysConCommand::GET_POWER_STATUS:
            std::puts("[SysCon  ] Get Power Status");

            data = 0;
            break;
        default:
            std::printf("Unhandled SysCon common read 0x%02X\n", (u8)cmd);

            exit(0);
    }

    rxQueue.push((u8)(data >>  0));
    rxQueue.push((u8)(data >>  8));
    rxQueue.push((u8)(data >> 16));
    rxQueue.push((u8)(data >> 24));
}

// Writes 32-bit syscon register
void commonWrite(SysConCommand cmd) {
    writeResponse(0);

    switch (cmd) {
        case SysConCommand::CTRL_TACHYON_WDT:
            std::puts("[SysCon  ] Ctrl Tachyon WDT");
            break;
        case SysConCommand::CTRL_ANALOG_XY_POLLING:
            std::puts("[SysCon  ] Ctrl Analog XY Polling");
            break;
        case SysConCommand::CTRL_HR_POWER:
            std::puts("[SysCon  ] Ctrl HR Power");
            break;
        case SysConCommand::CTRL_VOLTAGE:
            std::puts("[SysCon  ] Ctrl Voltage");
            break;
        case SysConCommand::CTRL_LED:
            std::puts("[SysCon  ] Ctrl LED");
            break;
        default:
            std::printf("Unhandled SysCon common write 0x%02X\n", (u8)cmd);

            exit(0);
    }
}

void batteryCommon(SysConCommand cmd, int len, const u8 *data) {
    writeResponse(len);

    switch (cmd) {
        case SysConCommand::BATTERY_GET_TEMP:
            std::puts("[SysCon  ] Battery Get Temp");
            break;
        case SysConCommand::BATTERY_GET_VOLT:
            std::puts("[SysCon  ] Battery Get Volt");
            break;
        case SysConCommand::BATTERY_GET_ELEC:
            std::puts("[SysCon  ] Battery Get Elec");
            break;
        case SysConCommand::BATTERY_GET_FULL_CAP:
            std::puts("[SysCon  ] Battery Get Full Cap");
            break;
        case SysConCommand::BATTERY_GET_LIMIT_TIME:
            std::puts("[SysCon  ] Battery Get Limit Time");
            break;
        default:
            std::printf("Unhandled SysCon battery command 0x%02X\n", (u8)cmd);

            exit(0);
    }

    for (int i = 0; i < len; i++) {
        rxQueue.push(data[i]);
    }
}

void cmdBatteryGetStatusCap() {
    std::printf("[SysCon  ] Battery Get Status Cap\n");

    writeResponse(4);

    rxQueue.push(((u8 *)&BATTERY_CURR_CAP)[0]);
    rxQueue.push(((u8 *)&BATTERY_CURR_CAP)[1]);
    rxQueue.push(((u8 *)&BATTERY_CURR_CAP)[2]);
    rxQueue.push(((u8 *)&BATTERY_CURR_CAP)[3]);
}

void cmdGetTimestamp() {
    std::puts("[SysCon  ] Get Timestamp");

    writeResponse(12);

    for (int i = 0; i < 6; i++) {
        rxQueue.push(0);
        rxQueue.push(0);
    }
}

void cmdReadScratchpad() {
    const auto in = getTxQueue();

    const auto src  = in >> 2;
    const auto size = 1 << (in & 3);

    std::printf("[SysCon  ] Read Scratchpad - Source: 0x%02X, size: %d\n", src, size);

    writeResponse(size);

    for (int i = 0; i < size; i++) {
        rxQueue.push(0xFF);
    }
}

void cmdSendSetparam() {
    std::printf("[SysCon  ] Send Setparam\n");

    writeResponse(0);
}

// Calculates and pushes response hash
void pushRxHash() {
    // Swap contents of RX queue with temporary queue
    std::queue<u8> temp;
    temp.swap(rxQueue);

    // Hash = ~(sum of all elements) & 0xFF
    u8 hash = 0;
    while (!temp.empty()) {
        const auto data = temp.front(); temp.pop();

        hash += data;

        // Push data back to rxQueue
        rxQueue.push(data);
    }

    rxQueue.push(~hash);
}

// HLE SysCon commands
void doCommand() {
    const auto cmd = getTxQueue();
    const auto len = getTxQueue();

    rxQueue.push(0x01); // Baryon status (AC plugged in)

    switch ((SysConCommand)cmd) {
        case SysConCommand::GET_BARYON_VERSION:
            commonRead(SysConCommand::GET_BARYON_VERSION);
            break;
        case SysConCommand::GET_TACHYON_TEMP:
            commonRead(SysConCommand::GET_TACHYON_TEMP);
            break;
        case SysConCommand::GET_KERNEL_DIGITAL_KEY:
            commonRead(SysConCommand::GET_KERNEL_DIGITAL_KEY);
            break;
        case SysConCommand::READ_CLOCK:
            commonRead(SysConCommand::READ_CLOCK);
            break;
        case SysConCommand::READ_ALARM:
            commonRead(SysConCommand::READ_ALARM);
            break;
        case SysConCommand::GET_POWER_SUPPLY_STATUS:
            commonRead(SysConCommand::GET_POWER_SUPPLY_STATUS);
            break;
        case SysConCommand::GET_WAKE_UP_FACTOR:
            commonRead(SysConCommand::GET_WAKE_UP_FACTOR);
            break;
        case SysConCommand::GET_TIMESTAMP:
            cmdGetTimestamp();
            break;
        case SysConCommand::READ_SCRATCHPAD:
            cmdReadScratchpad();
            break;
        case SysConCommand::SEND_SETPARAM:
            cmdSendSetparam();
            break;
        case SysConCommand::CTRL_TACHYON_WDT:
            commonWrite(SysConCommand::CTRL_TACHYON_WDT);
            break;
        case SysConCommand::CTRL_ANALOG_XY_POLLING:
            commonWrite(SysConCommand::CTRL_ANALOG_XY_POLLING);
            break;
        case SysConCommand::CTRL_HR_POWER:
            commonWrite(SysConCommand::CTRL_HR_POWER);
            break;
        case SysConCommand::CTRL_VOLTAGE:
            commonWrite(SysConCommand::CTRL_VOLTAGE);
            break;
        case SysConCommand::GET_POWER_STATUS:
            commonRead(SysConCommand::GET_POWER_STATUS);
            break;
        case SysConCommand::CTRL_LED:
            commonWrite(SysConCommand::CTRL_LED);
            break;
        case SysConCommand::BATTERY_GET_STATUS_CAP:
            cmdBatteryGetStatusCap();
            break;
        case SysConCommand::BATTERY_GET_TEMP:
            batteryCommon(SysConCommand::BATTERY_GET_TEMP, 4, (u8 *)&BATTERY_TEMP);
            break;
        case SysConCommand::BATTERY_GET_VOLT:
            batteryCommon(SysConCommand::BATTERY_GET_VOLT, 4, (u8 *)&BATTERY_VOLT);
            break;
        case SysConCommand::BATTERY_GET_ELEC:
            batteryCommon(SysConCommand::BATTERY_GET_ELEC, 4, (u8 *)&BATTERY_ELEC);
            break;
        case SysConCommand::BATTERY_GET_FULL_CAP:
            batteryCommon(SysConCommand::BATTERY_GET_FULL_CAP, 4, (u8 *)&BATTERY_FULL_CAP);
            break;
        case SysConCommand::BATTERY_GET_LIMIT_TIME:
            batteryCommon(SysConCommand::BATTERY_GET_LIMIT_TIME, 4, (u8 *)&BATTERY_LIMIT_TIME);
            break;
        default:
            std::printf("Unhandled SysCon command 0x%02X, length: %u\n", cmd, len);

            exit(0);
    }

    pushRxHash();

    serialflags |= 5;

    gpio::set(gpio::GPIOPin::SYSCON_END);
    gpio::clear(gpio::GPIOPin::SYSCON_START);
}

void finishCommand() {
    doCommand();
}

void init() {
    idFinishCommand = scheduler::registerEvent([](int) {finishCommand();});
}

u32 read(int cpuID, u32 addr) {
    const auto &r = regs[cpuID];

    switch ((SysConReg)addr) {
        case SysConReg::UNKNOWN0:
            std::printf("[SysCon  ] Unknown read @ 0x%08X\n", addr);

            return r.unknown[0];
        case SysConReg::NMIEN:
            std::puts("[SysCon  ] Read @ NMIEN");

            return r.nmien;
        case SysConReg::RAMSIZE:
            std::puts("[SysCon  ] Read @ RAMSIZE");

            return r.ramsize;
        case SysConReg::RESETEN:
            std::puts("[SysCon  ] Read @ RESETEN");

            return r.reseten;
        case SysConReg::BUSCLKEN:
            std::puts("[SysCon  ] Read @ BUSCLKEN");

            return r.busclken;
        case SysConReg::CLKEN:
            std::puts("[SysCon  ] Read @ CLKEN");

            return 0;
        case SysConReg::GPIOCLKEN:
            std::puts("[SysCon  ] Read @ GPIOCLKEN");

            return r.gpioclken;
        case SysConReg::CLKSEL2:
            std::puts("[SysCon  ] Read @ CLKSEL2");

            return 0;
        case SysConReg::SPICLK:
            std::puts("[SysCon  ] Read @ SPICLK");

            return r.spiclk;
        case SysConReg::PLLFREQ:
            std::puts("[SysCon  ] Read @ PLLFREQ");

            return r.pllfreq;
        case SysConReg::IOEN:
            std::puts("[SysCon  ] Read @ IOEN");

            return r.ioen;
        case SysConReg::GPIOEN:
            std::puts("[SysCon  ] Read @ GPIOEN");

            return r.gpioen;
        case SysConReg::FUSECONFIG:
            std::puts("[SysCon  ] Read @ FUSECONFIG");

            return FUSECONFIG;
        case SysConReg::UNKNOWN1:
            std::printf("[SysCon  ] Unknown read @ 0x%08X\n", addr);

            return r.unknown[1];
        default:
            std::printf("[SysCon  ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

u32 readSerial(u32 addr) {
    switch ((SysConSerialReg)addr) {
        case SysConSerialReg::DATA:
            std::puts("[SysCon  ] Read @ SERIALDATA");

            return getRxQueue();
        case SysConSerialReg::FLAGS:
            std::puts("[SysCon  ] Read @ SERIALFLAGS");

            return serialflags;
        case SysConSerialReg::UNKNOWN1:
            std::printf("[SysCon  ] Unknown serial read @ 0x%08X\n", addr);

            return 0;
        default:
            std::printf("[SysCon  ] Unhandled serial read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(int cpuID, u32 addr, u32 data) {
    auto &r = regs[cpuID];

    switch ((SysConReg)addr) {
        case SysConReg::NMIFLAG:
            std::printf("[SysCon  ] Write @ NMIFLAG = 0x%08X\n", data);

            r.nmiflag &= ~data;
            break;
        case SysConReg::UNKNOWN0:
            std::printf("[SysCon  ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            r.unknown[0] = data;
            break;
        case SysConReg::RAMSIZE:
            std::printf("[SysCon  ] Write @ RAMSIZE = 0x%08X\n", data);

            r.ramsize = data;
            break;
        case SysConReg::RESETEN:
            std::printf("[SysCon  ] Write @ RESETEN = 0x%08X\n", data);

            if (cpuID == 0) {
                if (!(regs[0].reseten & 2) && (data & 2)) resetCPU();
                if (!(regs[0].reseten & 4) && (data & 4)) resetME();
            }

            r.reseten = data;
            break;
        case SysConReg::BUSCLKEN:
            std::printf("[SysCon  ] Write @ BUSCLKEN = 0x%08X\n", data);

            r.busclken = data;
            break;
        case SysConReg::CLKEN:
            std::printf("[SysCon  ] Write @ CLKEN = 0x%08X\n", data);
            break;
        case SysConReg::GPIOCLKEN:
            std::printf("[SysCon  ] Write @ GPIOCLKEN = 0x%08X\n", data);

            r.gpioclken = data;
            break;
        case SysConReg::CLKSEL2:
            std::printf("[SysCon  ] Write @ CLKSEL2 = 0x%08X\n", data);
            break;
        case SysConReg::SPICLK:
            std::printf("[SysCon  ] Write @ SPICLK = 0x%08X\n", data);

            r.spiclk = data;
            break;
        case SysConReg::IOEN:
            std::printf("[SysCon  ] Write @ IOEN = 0x%08X\n", data);

            r.ioen = data;
            break;
        case SysConReg::GPIOEN:
            std::printf("[SysCon  ] Write @ GPIOEN = 0x%08X\n", data);

            r.gpioen = data;
            break;
        case SysConReg::UNKNOWN1:
            std::printf("[SysCon  ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            r.unknown[1] = data;
            break;
        default:
            std::printf("[SysCon  ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

void writeSerial(u32 addr, u32 data) {
    switch ((SysConSerialReg)addr) {
        case SysConSerialReg::INIT:
            std::printf("[SysCon  ] Write @ SERIALINIT = 0x%08X\n", data);
            break;
        case SysConSerialReg::CONTROL:
            std::printf("[SysCon  ] Write @ SERIALCONTROL = 0x%08X\n", data);

            switch (data) {
                case 4:
                    clearTxQueue();
                    break;
                case 6:
                    gpio::clear(gpio::GPIOPin::SYSCON_END);

                    scheduler::addEvent(idFinishCommand, 0, SYSCON_OP_CYCLES);
                    break;
                default:
                    break;
            }
            break;
        case SysConSerialReg::DATA:
            std::printf("[SysCon  ] Write @ SERIALDATA = 0x%08X\n", data);

            // Endianness is reversed
            txQueue.push(data >> 8);
            txQueue.push(data >> 0);
            break;
        case SysConSerialReg::UNKNOWN0:
        case SysConSerialReg::UNKNOWN2:
        case SysConSerialReg::UNKNOWN3:
            std::printf("[SysCon  ] Unknown serial write @ 0x%08X = 0x%08X\n", addr, data);
            break;
        default:
            std::printf("[SysCon  ] Unhandled serial write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
