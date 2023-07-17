/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "syscon.hpp"

#include <cassert>
#include <cstdio>

#include "psp.hpp"

namespace psp::syscon {

constexpr u32 TACHYON_VERSION = 0x40000001;

enum class SysConReg {
    NMIEN = 0x1C100000,
    NMIFLAG = 0x1C100004,
    RAMSIZE = 0x1C100040,
    RESETEN = 0x1C10004C,
    BUSCLKEN  = 0x1C100050,
    GPIOCLKEN = 0x1C100058,
    SPICLK  = 0x1C100064,
    PLLFREQ = 0x1C100068,
    IOEN = 0x1C100078,
    GPIOEN = 0x1C10007C,
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

// NMI registers
u32 nmien, nmiflag;

// Clock registers
u32 busclken, gpioclken;

// Enable
u32 reseten, ioen, gpioen;

u32 ramsize = TACHYON_VERSION, pllfreq, spiclk;

// Serial registers
u32 serialflags;

u32 read(u32 addr) {
    switch ((SysConReg)addr) {
        case SysConReg::NMIEN:
            std::puts("[SysCon  ] Read @ NMIEN");

            return nmien;
        case SysConReg::RAMSIZE:
            std::puts("[SysCon  ] Read @ RAMSIZE");

            return ramsize;
        case SysConReg::RESETEN:
            std::puts("[SysCon  ] Read @ RESETEN");

            return reseten;
        case SysConReg::BUSCLKEN:
            std::puts("[SysCon  ] Read @ BUSCLKEN");

            return busclken;
        case SysConReg::GPIOCLKEN:
            std::puts("[SysCon  ] Read @ GPIOCLKEN");

            return gpioclken;
        case SysConReg::SPICLK:
            std::puts("[SysCon  ] Read @ SPICLK");

            return spiclk;
        case SysConReg::PLLFREQ:
            std::puts("[SysCon  ] Read @ PLLFREQ");

            return pllfreq;
        case SysConReg::IOEN:
            std::puts("[SysCon  ] Read @ IOEN");

            return ioen;
        case SysConReg::GPIOEN:
            std::puts("[SysCon  ] Read @ GPIOEN");

            return gpioen;
        default:
            std::printf("[SysCon  ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

u32 readSerial(u32 addr) {
    switch ((SysConSerialReg)addr) {
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

void write(u32 addr, u32 data) {
    switch ((SysConReg)addr) {
        case SysConReg::NMIFLAG:
            std::printf("[SysCon  ] Write @ NMIFLAG = 0x%08X\n", data);

            nmiflag &= ~data;
            break;
        case SysConReg::RAMSIZE:
            std::printf("[SysCon  ] Write @ RAMSIZE = 0x%08X\n", data);

            ramsize = data;
            break;
        case SysConReg::RESETEN:
            std::printf("[SysCon  ] Write @ RESETEN = 0x%08X\n", data);

            reseten = data;

            if (data & 2) resetCPU();
            break;
        case SysConReg::BUSCLKEN:
            std::printf("[SysCon  ] Write @ BUSCLKEN = 0x%08X\n", data);

            busclken = data;
            break;
        case SysConReg::GPIOCLKEN:
            std::printf("[SysCon  ] Write @ GPIOCLKEN = 0x%08X\n", data);

            gpioclken = data;
            break;
        case SysConReg::SPICLK:
            std::printf("[SysCon  ] Write @ SPICLK = 0x%08X\n", data);

            spiclk = data;
            break;
        case SysConReg::IOEN:
            std::printf("[SysCon  ] Write @ IOEN = 0x%08X\n", data);

            ioen = data;
            break;
        case SysConReg::GPIOEN:
            std::printf("[SysCon  ] Write @ GPIOEN = 0x%08X\n", data);

            gpioen = data;
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
            break;
        case SysConSerialReg::DATA:
            std::printf("[SysCon  ] Write @ SERIALDATA = 0x%08X\n", data);
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
