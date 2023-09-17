/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "syscon.hpp"

#include <cassert>
#include <cstdio>
#include <queue>

#include "gpio.hpp"
#include "intc.hpp"
#include "psp.hpp"
#include "scheduler.hpp"

namespace psp::syscon {

constexpr i64 SYSCON_OP_CYCLES = 3500 * scheduler::_1US;

constexpr u8 BARYON_TIMESTAMP[] = {2, 0, 0, 5, 0, 9, 2, 6, 0, 4, 4, 1}; // 2005/09/26 04:41

constexpr u32 BARYON_VERSION  = 0x00114000;
constexpr u32 TACHYON_VERSION = 0x40000001;

constexpr u32 FUSEID[2] = {0xB2A18793, 0x0000590B};
constexpr u32 FUSECONFIG = 0x0000590B;

constexpr u32 BATTERY_TEMP = 20;         // In Celsius
constexpr u32 BATTERY_VOLT = 3906;       // Max: 4150mV
constexpr u32 BATTERY_ELEC = -244;       // Current - max voltage?
constexpr u32 BATTERY_FULL_CAP = 1790;   // Max: 1800mAh
constexpr u32 BATTERY_CURR_CAP = 748;
constexpr u32 BATTERY_LIMIT_TIME = 187;  // In minutes?

enum class SysConReg {
    NMIEN = 0x1C100000,
    NMIFLAG = 0x1C100004,
    UNKNOWN0 = 0x1C10003C,
    RAMSIZE = 0x1C100040,
    POSTME  = 0x1C100044,
    RESETEN = 0x1C10004C,
    BUSCLKEN  = 0x1C100050,
    CLKEN  = 0x1C100054,
    GPIOCLKEN = 0x1C100058,
    CLKSEL1 = 0x1C10005C,
    CLKSEL2 = 0x1C100060,
    SPICLK  = 0x1C100064,
    PLLFREQ = 0x1C100068,
    AVCPOWER = 0x1C100070,
    UNKNOWN1 = 0x1C100074,
    IOEN = 0x1C100078,
    GPIOEN = 0x1C10007C,
    CONNECTSTATUS = 0x1C100080,
    FUSEID_LOW  = 0x1C100090,
    FUSEID_HIGH = 0x1C100094,
    FUSECONFIG  = 0x1C100098,
    UNKNOWN2 = 0x1C1000FC,
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
    GET_WAKE_UP_REQ = 0x0F,
    GET_TIMESTAMP = 0x11,
    WRITE_SCRATCHPAD = 0x23,
    READ_SCRATCHPAD  = 0x24,
    SEND_SETPARAM = 0x25,
    RECEIVE_SETPARAM = 0x26,
    CTRL_TACHYON_WDT = 0x31,
    RESET_DEVICE = 0x32,
    CTRL_ANALOG_XY_POLLING = 0x33,
    CTRL_HR_POWER = 0x34,
    POWER_SUSPEND = 0x36,
    CTRL_VOLTAGE  = 0x42,
    GET_POWER_STATUS = 0x46,
    CTRL_LED = 0x47,
    CTRL_LEPTON_POWER = 0x4B,
    CTRL_MS_POWER = 0x4C,
    CTRL_WLAN_POWER = 0x4D,
    BATTERY_GET_STATUS_CAP = 0x61,
    BATTERY_GET_TEMP = 0x62,
    BATTERY_GET_VOLT = 0x63,
    BATTERY_GET_ELEC = 0x64,
    BATTERY_GET_FULL_CAP = 0x67,
    BATTERY_GET_LIMIT_TIME = 0x69,
};

enum BaryonStatus {
    AC_POWER   = 1 << 0,
    WLAN_POWER = 1 << 1,
    HR_POWER   = 1 << 2,
    ALARM      = 1 << 3,
};

struct SysConRegs {
    // NMI registers
    u32 nmien, nmiflag;

    // Clock registers
    u32 busclken, gpioclken;

    // Enable
    u32 reseten, ioen, gpioen;

    u32 spiclk;

    u32 unknown[3];
};

SysConRegs regs[2];

// Values taken from my PSP
u8 scratchpad[0x20] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x2F, 0x00, 0x00, 0xEA, 0x3C, 0x91, 0x4B,
    0x4F, 0x5F, 0x52, 0x58, 0x1C, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

u8 setparam[8];

u32 avcpower;
u32 clksel1, clksel2;

u32 ramsize = TACHYON_VERSION;
u32 pllfreq = 3;

// Serial registers
u32 serialflags;

// SysCon commands
std::queue<u8> txQueue, rxQueue;

// SysCon internal registers
u8  baryonStatus;
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

            data = 0xFFEF7FFF; // Value from my PSP
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

            data = 0x440; // 0x4C0 on my console, but crashes in IPL if bit 7 is set
            break;
        case SysConCommand::GET_WAKE_UP_REQ:
            std::puts("[SysCon  ] Get Wake Up Req");

            data = 0xFF; // ??
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
        case SysConCommand::RESET_DEVICE:
            std::puts("[SysCon  ] Reset Device");
            break;
        case SysConCommand::CTRL_ANALOG_XY_POLLING:
            std::puts("[SysCon  ] Ctrl Analog XY Polling");
            break;
        case SysConCommand::CTRL_HR_POWER:
            std::puts("[SysCon  ] Ctrl HR Power");

            if (getTxQueue() & 1) {
                baryonStatus |= BaryonStatus::HR_POWER;
            } else {
                baryonStatus &= ~BaryonStatus::HR_POWER;
            }
            break;
        case SysConCommand::POWER_SUSPEND:
            std::puts("[SysCon  ] Power Suspend");
            break;
        case SysConCommand::CTRL_VOLTAGE:
            std::puts("[SysCon  ] Ctrl Voltage");
            break;
        case SysConCommand::CTRL_LED:
            std::puts("[SysCon  ] Ctrl LED");
            break;
        case SysConCommand::CTRL_LEPTON_POWER:
            std::puts("[SysCon  ] Ctrl Lepton Power");
            break;
        case SysConCommand::CTRL_MS_POWER:
            std::puts("[SysCon  ] Ctrl MS Power");
            break;
        case SysConCommand::CTRL_WLAN_POWER:
            std::printf("[SysCon  ] Ctrl WLAN Power\n");

            if (getTxQueue() & 1) {
                baryonStatus |= BaryonStatus::WLAN_POWER;
            } else {
                baryonStatus &= ~BaryonStatus::WLAN_POWER;
            }
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

    for (int i = 0; i < 12; i++) {
        rxQueue.push(BARYON_TIMESTAMP[i]);
    }
}

void cmdReadScratchpad() {
    const auto in = getTxQueue();

    const auto src  = in >> 2;
    const auto size = 1 << (in & 3);

    std::printf("[SysCon  ] Read Scratchpad - Source: 0x%02X, size: %d\n", src, size);

    writeResponse(size);

    for (int i = 0; i < size; i++) {
        rxQueue.push(scratchpad[src + i]);
    }
}

void cmdWriteScratchpad() {
    const auto in = getTxQueue();

    const auto dest = in >> 2;
    const auto size = 1 << (in & 3);

    std::printf("[SysCon  ] Write Scratchpad - Destination: 0x%02X, size: %d\n", dest, size);

    writeResponse(0);

    for (int i = 0; i < size; i++) {
        scratchpad[dest + i] = getTxQueue();
    }
}

void cmdReceiveSetparam() {
    std::printf("[SysCon  ] Receive Setparam\n");

    writeResponse(8);

    for (int i = 0; i < (int)sizeof(setparam); i++) {
        rxQueue.push(setparam[i]);
    }
}

void cmdSendSetparam() {
    std::printf("[SysCon  ] Send Setparam\n");

    writeResponse(0);

    for (int i = 0; i < (int)sizeof(setparam); i++) {
        setparam[i] = getTxQueue();
    }
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

    rxQueue.push(baryonStatus);

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
        case SysConCommand::GET_WAKE_UP_REQ:
            commonRead(SysConCommand::GET_WAKE_UP_REQ);
            break;
        case SysConCommand::GET_TIMESTAMP:
            cmdGetTimestamp();
            break;
        case SysConCommand::WRITE_SCRATCHPAD:
            cmdWriteScratchpad();
            break;
        case SysConCommand::READ_SCRATCHPAD:
            cmdReadScratchpad();
            break;
        case SysConCommand::SEND_SETPARAM:
            cmdSendSetparam();
            break;
        case SysConCommand::RECEIVE_SETPARAM:
            cmdReceiveSetparam();
            break;
        case SysConCommand::CTRL_TACHYON_WDT:
            commonWrite(SysConCommand::CTRL_TACHYON_WDT);
            break;
        case SysConCommand::RESET_DEVICE:
            commonWrite(SysConCommand::RESET_DEVICE);
            break;
        case SysConCommand::CTRL_ANALOG_XY_POLLING:
            commonWrite(SysConCommand::CTRL_ANALOG_XY_POLLING);
            break;
        case SysConCommand::CTRL_HR_POWER:
            commonWrite(SysConCommand::CTRL_HR_POWER);
            break;
        case SysConCommand::POWER_SUSPEND:
            commonWrite(SysConCommand::POWER_SUSPEND);
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
        case SysConCommand::CTRL_LEPTON_POWER:
            commonWrite(SysConCommand::CTRL_LEPTON_POWER);
            break;
        case SysConCommand::CTRL_MS_POWER:
            commonWrite(SysConCommand::CTRL_MS_POWER);
            break;
        case SysConCommand::CTRL_WLAN_POWER:
            commonWrite(SysConCommand::CTRL_WLAN_POWER);
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
}

void finishCommand() {
    doCommand();
}

void init() {
    idFinishCommand = scheduler::registerEvent([](int) {finishCommand();});

    baryonStatus = BaryonStatus::ALARM | BaryonStatus::AC_POWER;
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

            return ramsize;
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
        case SysConReg::CLKSEL1:
            std::puts("[SysCon  ] Read @ CLKSEL1");

            return clksel1;
        case SysConReg::CLKSEL2:
            std::puts("[SysCon  ] Read @ CLKSEL2");

            return clksel2;
        case SysConReg::SPICLK:
            std::puts("[SysCon  ] Read @ SPICLK");

            return r.spiclk;
        case SysConReg::AVCPOWER:
            std::puts("[SysCon  ] Read @ AVCPOWER");

            return avcpower;
        case SysConReg::UNKNOWN1:
            std::printf("[SysCon  ] Unknown read @ 0x%08X\n", addr);

            return r.unknown[1];
        case SysConReg::PLLFREQ:
            std::puts("[SysCon  ] Read @ PLLFREQ");

            return pllfreq;
        case SysConReg::IOEN:
            std::puts("[SysCon  ] Read @ IOEN");

            return r.ioen;
        case SysConReg::GPIOEN:
            std::puts("[SysCon  ] Read @ GPIOEN");

            return r.gpioen;
        case SysConReg::CONNECTSTATUS:
            std::puts("[SysCon  ] Read @ CONNECTSTATUS");

            return 0;
        case SysConReg::FUSEID_LOW:
            std::puts("[SysCon  ] Read @ FUSEID_LOW");

            return FUSEID[0];
        case SysConReg::FUSEID_HIGH:
            std::puts("[SysCon  ] Read @ FUSEID_HIGH");

            return FUSEID[1];
        case SysConReg::FUSECONFIG:
            std::puts("[SysCon  ] Read @ FUSECONFIG");

            return FUSECONFIG;
        case SysConReg::UNKNOWN2:
            std::printf("[SysCon  ] Unknown read @ 0x%08X\n", addr);

            return r.unknown[2];
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
        case SysConReg::RAMSIZE: // Possibly shared
            std::printf("[SysCon  ] Write @ RAMSIZE = 0x%08X\n", data);

            ramsize = (ramsize & 0xFF000800) | (data & 0xFFF7FF); // Tachyon version is RO
            break;
        case SysConReg::POSTME:
            std::printf("[SysCon  ] Write @ POSTME = 0x%08X\n", data);

            if (data & 1) {
                if (cpuID == 0) {
                    intc::meSendIRQ(intc::InterruptSource::ME);

                    postME();
                } else {
                    intc::sendIRQ(intc::InterruptSource::ME);
                }
            }
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
        case SysConReg::CLKSEL1:
            std::printf("[SysCon  ] Write @ CLKSEL1 = 0x%08X\n", data);

            clksel1 = data;
            break;
        case SysConReg::CLKSEL2:
            std::printf("[SysCon  ] Write @ CLKSEL2 = 0x%08X\n", data);

            clksel2 = data;
            break;
        case SysConReg::SPICLK:
            std::printf("[SysCon  ] Write @ SPICLK = 0x%08X\n", data);

            r.spiclk = data;
            break;
        case SysConReg::AVCPOWER: // Shared?
            std::printf("[SysCon  ] Write @ AVCPOWER = 0x%08X\n", data);

            avcpower = data;
            break;
        case SysConReg::UNKNOWN1:
            std::printf("[SysCon  ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            r.unknown[1] = data;
            break;
        case SysConReg::IOEN:
            std::printf("[SysCon  ] Write @ IOEN = 0x%08X\n", data);

            r.ioen = data;
            break;
        case SysConReg::GPIOEN:
            std::printf("[SysCon  ] Write @ GPIOEN = 0x%08X\n", data);

            r.gpioen = data;
            break;
        case SysConReg::CONNECTSTATUS:
            std::printf("[SysCon  ] Write @ CONNECTSTATUS = 0x%08X\n", data);
            break;
        case SysConReg::UNKNOWN2:
            std::printf("[SysCon  ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            r.unknown[2] = data;
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

            if (!(data & 2)) {
                gpio::clear(gpio::GPIOPin::SYSCON_END);
            }

            switch (data) {
                case 4:
                    clearTxQueue();
                    break;
                case 6:
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
