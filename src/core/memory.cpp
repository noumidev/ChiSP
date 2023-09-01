/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "memory.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

#include "ata.hpp"
#include "ddr.hpp"
#include "display.hpp"
#include "dmacplus.hpp"
#include "ge.hpp"
#include "gpio.hpp"
#include "hpremote.hpp"
#include "intc.hpp"
#include "i2c.hpp"
#include "nand.hpp"
#include "syscon.hpp"
#include "systime.hpp"
#include "crypto/kirk.hpp"
#include "crypto/spock.hpp"
#include "../common/file.hpp"

namespace psp::memory {

constexpr auto CPUID_CPU = 0;
constexpr auto CPUID_ME  = 1;

// PSP system memory
std::array<u8, (u64)MemorySize::BootROM> bootROM;
std::array<u8, (u64)MemorySize::SPRAM> spram;
std::array<u8, (u64)MemorySize::EDRAM> edram, sharedRAM, meSPRAM;
std::array<u8, (u64)MemorySize::DRAM>  dram;

u8 *resetVector = bootROM.data();
u32 resetSize = (u32)MemorySize::BootROM;

u32 cpufreq[2] = {0x1FF01FF, 0x1FF01FF}, busfreq[2] = {0x1FF01FF, 0x1FF01FF};

// Returns true if addr is in the range base,(base + size)
bool inRange(u64 addr, u64 base, u64 size) {
    return (addr >= base) && (addr < (base + size));
}

void init(const char *bootPath) {
    std::printf("[Memory  ] Loading boot ROM \"%s\"\n", bootPath);
    assert(loadFile(bootPath, bootROM.data(), (u64)MemorySize::BootROM));

    std::puts("[Memory  ] OK");
}

u8 *getMemoryPointer(u32 addr) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (inRange(addr, (u64)MemoryBase::EDRAM, (u64)MemorySize::EDRAM)) {
        return &edram[addr & ((u32)MemorySize::EDRAM - 1)];
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        return &dram[addr & ((u32)MemorySize::DRAM - 1)];
    } else if (inRange(addr, (u64)MemoryBase::SharedRAM, (u64)MemorySize::EDRAM)) {
        return &sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)];
    } else {
        switch (addr) {
            default:
                std::printf("Unhandled memory region @ 0x%08X\n", addr);

                exit(0);
        }
    }
}

u8 read8(u32 addr) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (inRange(addr, (u64)MemoryBase::SPRAM, (u64)MemorySize::SPRAM)) {
        return spram[addr & ((u32)MemorySize::SPRAM - 1)];
    } else if (inRange(addr, (u64)MemoryBase::EDRAM, (u64)MemorySize::EDRAM)) {
        return edram[addr & ((u32)MemorySize::EDRAM - 1)];
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        return dram[addr & ((u32)MemorySize::DRAM - 1)];
    } else if (inRange(addr, (u64)MemoryBase::MS, (u64)MemorySize::MS)) {
        std::printf("[MS      ] Unhandled read8 @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::WLAN, (u64)MemorySize::WLAN)) {
        std::printf("[WLAN    ] Unhandled read8 @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::ATA1, (u64)MemorySize::ATA1)) {
        return ata::readATA1(addr);
    } else if (inRange(addr, (u64)MemoryBase::BootROM, resetSize)) {
        return resetVector[addr & (resetSize - 1)];
    } else if (inRange(addr, (u64)MemoryBase::SharedRAM, (u64)MemorySize::EDRAM)) {
        return sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)];
    } else {
        switch (addr) {
            default:
                std::printf("Unhandled read8 @ 0x%08X\n", addr);

                exit(0);
        }
    }
}

u16 read16(u32 addr) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    u16 data;

    if (inRange(addr, (u64)MemoryBase::SPRAM, (u64)MemorySize::SPRAM)) {
        std::memcpy(&data, &spram[addr & ((u32)MemorySize::SPRAM - 1)], sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::EDRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&data, &edram[addr & ((u32)MemorySize::EDRAM - 1)], sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        std::memcpy(&data, &dram[addr & ((u32)MemorySize::DRAM - 1)], sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::MS, (u64)MemorySize::MS)) {
        std::printf("[MS      ] Unhandled read16 @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::WLAN, (u64)MemorySize::WLAN)) {
        std::printf("[WLAN    ] Unhandled read16 @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::BootROM, resetSize)) {
        std::memcpy(&data, &resetVector[addr & (resetSize - 1)], sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::SharedRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&data, &sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)], sizeof(u16));
    } else {
        switch (addr) {
            case 0x11800000: // IPL checks for string "MS"
                std::printf("[Memory  ] Unknown read16 @ 0x%08X\n", addr);
                return 0;
            default:
                std::printf("Unhandled read16 @ 0x%08X\n", addr);

                exit(0);
        }
    }

    return data;
}

u32 read32(u32 addr) {
    //if (addr == 0x880402EC) writeFile("ram.bin", dram.data(), (u64)MemorySize::DRAM);

    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    u32 data;

    if (inRange(addr, (u64)MemoryBase::SPRAM, (u64)MemorySize::SPRAM)) {
        std::memcpy(&data, &spram[addr & ((u32)MemorySize::SPRAM - 1)], sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::EDRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&data, &edram[addr & ((u32)MemorySize::EDRAM - 1)], sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        std::memcpy(&data, &dram[addr & ((u32)MemorySize::DRAM - 1)], sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::MEMPROT, (u64)MemorySize::MEMPROT)) {
        std::printf("[MEMPROT ] Unhandled read @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::SysCon, (u64)MemorySize::SysCon)) {
        return syscon::read(CPUID_CPU, addr);
    } else if (inRange(addr, (u64)MemoryBase::INTC, (u64)MemorySize::INTC)) {
        return intc::read(CPUID_CPU, addr);
    } else if (inRange(addr, (u64)MemoryBase::Timer, (u64)MemorySize::Timer)) {
        std::printf("[Timer   ] Unhandled read @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::SysTime, (u64)MemorySize::SysTime)) {
        return systime::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::DMACplus, (u64)MemorySize::DMACplus)) {
        return dmacplus::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::DMAC0, (u64)MemorySize::DMAC)) {
        std::printf("[DMAC    ] Unhandled read @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::DMAC1, (u64)MemorySize::DMAC)) {
        std::printf("[DMAC    ] Unhandled read @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::DDR, (u64)MemorySize::DDR)) {
        return ddr::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::NAND, (u64)MemorySize::NAND)) {
        return nand::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::MS, (u64)MemorySize::MS)) {
        std::printf("[MS      ] Unhandled read32 @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::WLAN, (u64)MemorySize::WLAN)) {
        std::printf("[WLAN    ] Unhandled read32 @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::GE, (u64)MemorySize::GE)) {
        return ge::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::ATA0, (u64)MemorySize::ATA0)) {
        return ata::readATA0(addr);
    } else if (inRange(addr, (u64)MemoryBase::KIRK, (u64)MemorySize::KIRK)) {
        return kirk::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::SPOCK, (u64)MemorySize::SPOCK)) {
        return spock::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::Audio, (u64)MemorySize::Audio)) {
        std::printf("[Audio   ] Unhandled read @ 0x%08X\n", addr);

        if (addr == 0x1E000028) {
            return -1;
        }

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::LCDC, (u64)MemorySize::LCDC)) {
        std::printf("[LCDC    ] Unhandled read @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::I2C, (u64)MemorySize::I2C)) {
        return i2c::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::GPIO, (u64)MemorySize::GPIO)) {
        return gpio::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::POWERMAN, (u64)MemorySize::POWERMAN)) {
        std::printf("[POWERMAN] Unhandled read @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::UART0, (u64)MemorySize::UART)) {
        std::printf("[UART0   ] Unhandled read @ 0x%08X\n", addr);

        if (addr == 0x1E4C0018) {
            return 0x80;
        }

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::HPRemote, (u64)MemorySize::UART)) {
        return hpremote::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::SysConSerial, (u64)MemorySize::SysConSerial)) {
        return syscon::readSerial(addr);
    } else if (inRange(addr, (u64)MemoryBase::Display, (u64)MemorySize::Display)) {
        return display::read(addr);
    } else if (inRange(addr, (u64)MemoryBase::BootROM, resetSize)) {
        std::memcpy(&data, &resetVector[addr & (resetSize - 1)], sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::SharedRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&data, &sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)], sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::NANDBuffer, (u64)MemorySize::NANDBuffer)) {
        return nand::readBuffer32(addr);
    } else {
        switch (addr) {
            case 0x1C200000:
                std::printf("[FREQ    ] Read @ CPUFREQ\n");

                return cpufreq[CPUID_CPU];
            case 0x1C200004:
                std::printf("[FREQ    ] Read @ BUSFREQ\n");

                return busfreq[CPUID_CPU];
            case 0x1D500000:
                std::printf("[Memory  ] Unhandled read32 @ EDRAMREFRESH0\n");
                return 0;
            case 0x1D500010:
                std::printf("[Memory  ] Unhandled read32 @ EDRAMINIT1\n");
                return 0; // Pre IPL hangs if bit 0 is high
            case 0x1D500020:
                std::printf("[Memory  ] Unhandled read32 @ EDRAMREFRESH1\n");
                return 0;
            case 0x1D500030:
                std::printf("[Memory  ] Unhandled read32 @ EDRAMREFRESH2\n");
                return 0;
            case 0x1D500040:
                std::printf("[Memory  ] Unhandled read32 @ EDRAMINIT2\n");
                return 0;
            case 0x1D500070:
                std::printf("[Memory  ] Unhandled read32 @ EDRAMTCONTROL\n");
                return 0;
            case 0x1D500080:
                std::printf("[Memory  ] Unhandled read32 @ EDRAMTADDR\n");
                return 0;
            default:
                std::printf("Unhandled read32 @ 0x%08X\n", addr);

                exit(0);
        }
    }

    return data;
}

void write8(u32 addr, u8 data) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (inRange(addr, (u64)MemoryBase::SPRAM, (u64)MemorySize::SPRAM)) {
        spram[addr & ((u32)MemorySize::SPRAM - 1)] = data;
    } else if (inRange(addr, (u64)MemoryBase::EDRAM, (u64)MemorySize::EDRAM)) {
        edram[addr & ((u32)MemorySize::EDRAM - 1)] = data;
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        dram[addr & ((u32)MemorySize::DRAM - 1)] = data;
    } else if (inRange(addr, (u64)MemoryBase::MS, (u64)MemorySize::MS)) {
        std::printf("[MS      ] Unhandled write8 @ 0x%08X = 0x%02X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::WLAN, (u64)MemorySize::WLAN)) {
        std::printf("[WLAN    ] Unhandled write8 @ 0x%08X = 0x%02X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::ATA1, (u64)MemorySize::ATA1)) {
        return ata::writeATA1(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::BootROM, resetSize)) {
        resetVector[addr & (resetSize - 1)] = data;
    } else if (inRange(addr, (u64)MemoryBase::SharedRAM, (u64)MemorySize::EDRAM)) {
        sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)] = data;
    } else {
        switch (addr) {
            default:
                std::printf("Unhandled write8 @ 0x%08X = 0x%02X\n", addr, data);

                exit(0);
        }
    }
}

void write16(u32 addr, u16 data) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (inRange(addr, (u64)MemoryBase::SPRAM, (u64)MemorySize::SPRAM)) {
        std::memcpy(&spram[addr & ((u32)MemorySize::SPRAM - 1)], &data, sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::EDRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&edram[addr & ((u32)MemorySize::EDRAM - 1)], &data, sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        std::memcpy(&dram[addr & ((u32)MemorySize::DRAM - 1)], &data, sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::MS, (u64)MemorySize::MS)) {
        std::printf("[MS      ] Unhandled write16 @ 0x%08X = 0x%04X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::WLAN, (u64)MemorySize::WLAN)) {
        std::printf("[WLAN    ] Unhandled write16 @ 0x%08X = 0x%04X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::BootROM, resetSize)) {
        std::memcpy(&resetVector[addr & (resetSize - 1)], &data, sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::SharedRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)], &data, sizeof(u16));
    } else {
        switch (addr) {
            default:
                std::printf("Unhandled write16 @ 0x%08X = 0x%04X\n", addr, data);

                exit(0);
        }
    }
}

void write32(u32 addr, u32 data) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (addr == 0x1C100044) {
        writeFile("ram.bin", dram.data(), (u64)MemorySize::DRAM);
    }

    if (inRange(addr, (u64)MemoryBase::SPRAM, (u64)MemorySize::SPRAM)) {
        std::memcpy(&spram[addr & ((u32)MemorySize::SPRAM - 1)], &data, sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::EDRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&edram[addr & ((u32)MemorySize::EDRAM - 1)], &data, sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        std::memcpy(&dram[addr & ((u32)MemorySize::DRAM - 1)], &data, sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::MEMPROT, (u64)MemorySize::MEMPROT)) {
        std::printf("[MEMPROT ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::SysCon, (u64)MemorySize::SysCon)) {
        return syscon::write(CPUID_CPU, addr, data);
    } else if (inRange(addr, (u64)MemoryBase::INTC, (u64)MemorySize::INTC)) {
        return intc::write(CPUID_CPU, addr, data);
    } else if (inRange(addr, (u64)MemoryBase::Timer, (u64)MemorySize::Timer)) {
        std::printf("[Timer   ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::SysTime, (u64)MemorySize::SysTime)) {
        return systime::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::DMACplus, (u64)MemorySize::DMACplus)) {
        return dmacplus::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::DMAC0, (u64)MemorySize::DMAC)) {
        std::printf("[DMAC    ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::DMAC1, (u64)MemorySize::DMAC)) {
        std::printf("[DMAC    ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::DDR, (u64)MemorySize::DDR)) {
        return ddr::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::NAND, (u64)MemorySize::NAND)) {
        return nand::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::MS, (u64)MemorySize::MS)) {
        std::printf("[MS      ] Unhandled write32 @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::WLAN, (u64)MemorySize::WLAN)) {
        std::printf("[WLAN    ] Unhandled write32 @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::GE, (u64)MemorySize::GE)) {
        return ge::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::ATA0, (u64)MemorySize::ATA0)) {
        return ata::writeATA0(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::KIRK, (u64)MemorySize::KIRK)) {
        return kirk::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::SPOCK, (u64)MemorySize::SPOCK)) {
        return spock::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::Audio, (u64)MemorySize::Audio)) {
        std::printf("[Audio   ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::LCDC, (u64)MemorySize::LCDC)) {
        std::printf("[LCDC    ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::I2C, (u64)MemorySize::I2C)) {
        return i2c::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::GPIO, (u64)MemorySize::GPIO)) {
        return gpio::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::POWERMAN, (u64)MemorySize::POWERMAN)) {
        std::printf("[POWERMAN] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::UART0, (u64)MemorySize::UART)) {
        if (addr == (u32)MemoryBase::UART0) {
            std::putchar(data);
        } else {
            std::printf("[UART0   ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
        }
    } else if (inRange(addr, (u64)MemoryBase::HPRemote, (u64)MemorySize::UART)) {
        return hpremote::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::SysConSerial, (u64)MemorySize::SysConSerial)) {
        return syscon::writeSerial(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::Display, (u64)MemorySize::Display)) {
        return display::write(addr, data);
    } else if (inRange(addr, (u64)MemoryBase::BootROM, resetSize)) {
        std::memcpy(&resetVector[addr & (resetSize - 1)], &data, sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::SharedRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)], &data, sizeof(u32));
    } else {
        switch (addr) {
            case 0x1C200000:
                std::printf("[FREQ    ] Write @ CPUFREQ = 0x%08X\n", data);

                cpufreq[CPUID_CPU] = data;
                break;
            case 0x1C200004:
                std::printf("[FREQ    ] Write @ BUSFREQ = 0x%08X\n", data);

                busfreq[CPUID_CPU] = data;
                break;
            case 0x1D500000:
                std::printf("[Memory  ] Unhandled write32 @ EDRAMREFRESH0 = 0x%08X\n", data);
                break;
            case 0x1D500010:
                std::printf("[Memory  ] Unhandled write32 @ EDRAMINIT1 = 0x%08X\n", data);
                break;
            case 0x1D500020:
                std::printf("[Memory  ] Unhandled write32 @ EDRAMREFRESH1 = 0x%08X\n", data);
                break;
            case 0x1D500030:
                std::printf("[Memory  ] Unhandled write32 @ EDRAMREFRESH2 = 0x%08X\n", data);
                break;
            case 0x1D500040:
                std::printf("[Memory  ] Unhandled write32 @ EDRAMINIT2 = 0x%08X\n", data);
                break;
            case 0x1D500070:
                std::printf("[Memory  ] Unhandled write32 @ EDRAMTCONTROL = 0x%08X\n", data);
                break;
            case 0x1D500080:
                std::printf("[Memory  ] Unhandled write32 @ EDRAMTADDR = 0x%08X\n", data);
                break;
            case 0x1D500090:
                std::printf("[Memory  ] Unhandled write32 @ EDRAMUNK2 = 0x%08X\n", data);
                break;
            default:
                std::printf("Unhandled write32 @ 0x%08X = 0x%08X\n", addr, data);

                exit(0);
        }
    }
}

void read128(u32 addr, u8 *data) {
    assert(!(addr & 0xF));

    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        std::memcpy(data, &dram[addr & ((u32)MemorySize::DRAM - 1)], 4 * sizeof(u32));
    } else {
        std::printf("Unhandled read128 @ 0x%08X\n", addr);

        exit(0);
    }
}

void write128(u32 addr, u8 *data) {
    assert(!(addr & 0xF));

    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (inRange(addr, (u64)MemoryBase::EDRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&edram[addr & ((u32)MemorySize::EDRAM - 1)], data, 4 * sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        std::memcpy(&dram[addr & ((u32)MemorySize::DRAM - 1)], data, 4 * sizeof(u32));
    } else {
        std::printf("Unhandled write128 @ 0x%08X = 0x%08X%08X%08X%08X\n", addr, *(u32 *)&data[0], *(u32 *)&data[4], *(u32 *)&data[8], *(u32 *)&data[12]);

        exit(0);
    }
}

u8 meRead8(u32 addr) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (inRange(addr, (u64)MemoryBase::MESPRAM, (u64)MemorySize::EDRAM)) {
        return meSPRAM[addr & ((u32)MemorySize::EDRAM - 1)];
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        return dram[addr & ((u32)MemorySize::DRAM - 1)];
    } else if (inRange(addr, (u64)MemoryBase::BootROM, (u64)MemorySize::EDRAM)) {
        return sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)];
    } else {
        switch (addr) {
            default:
                std::printf("Unhandled ME read8 @ 0x%08X\n", addr);

                exit(0);
        }
    }
}

u16 meRead16(u32 addr) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    u16 data;

    if (inRange(addr, (u64)MemoryBase::MESPRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&data, &meSPRAM[addr & ((u32)MemorySize::EDRAM - 1)], sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        std::memcpy(&data, &dram[addr & ((u32)MemorySize::DRAM - 1)], sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::BootROM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&data, &sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)], sizeof(u16));
    } else {
        switch (addr) {
            default:
                std::printf("Unhandled ME read16 @ 0x%08X\n", addr);

                exit(0);
        }
    }

    return data;
}

u32 meRead32(u32 addr) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    u32 data;

    if (inRange(addr, (u64)MemoryBase::MESPRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&data, &meSPRAM[addr & ((u32)MemorySize::EDRAM - 1)], sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::VME0, (u64)MemorySize::VME0)) {
        std::printf("[VME     ] Unhandled read @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        std::memcpy(&data, &dram[addr & ((u32)MemorySize::DRAM - 1)], sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::MEMPROT, (u64)MemorySize::MEMPROT)) {
        std::printf("[MEMPROT ] Unhandled read @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::SysCon, (u64)MemorySize::SysCon)) {
        return syscon::read(CPUID_ME, addr);
    } else if (inRange(addr, (u64)MemoryBase::INTC, (u64)MemorySize::INTC)) {
        return intc::read(CPUID_ME, addr);
    } else if (inRange(addr, (u64)MemoryBase::VME1, (u64)MemorySize::VME1)) {
        std::printf("[VME     ] Unhandled read @ 0x%08X\n", addr);

        return 0;
    } else if (inRange(addr, (u64)MemoryBase::BootROM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&data, &sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)], sizeof(u32));
    } else {
        switch (addr) {
            case 0x1C200000:
                std::printf("[MEFREQ  ] Read @ CPUFREQ\n");

                return cpufreq[CPUID_ME];
            case 0x1C200004:
                std::printf("[MEFREQ  ] Read @ BUSFREQ\n");

                return busfreq[CPUID_ME];
            default:
                std::printf("Unhandled ME read32 @ 0x%08X\n", addr);

                exit(0);
        }
    }

    return data;
}

void meWrite8(u32 addr, u8 data) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (inRange(addr, (u64)MemoryBase::MESPRAM, (u64)MemorySize::EDRAM)) {
        meSPRAM[addr & ((u32)MemorySize::EDRAM - 1)] = data;
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        dram[addr & ((u32)MemorySize::DRAM - 1)] = data;
    } else if (inRange(addr, (u64)MemoryBase::BootROM, (u64)MemorySize::EDRAM)) {
        sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)] = data;
    } else {
        switch (addr) {
            default:
                std::printf("Unhandled ME write8 @ 0x%08X = 0x%02X\n", addr, data);

                exit(0);
        }
    }
}

void meWrite16(u32 addr, u16 data) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (inRange(addr, (u64)MemoryBase::MESPRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&meSPRAM[addr & ((u32)MemorySize::EDRAM - 1)], &data, sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        std::memcpy(&dram[addr & ((u32)MemorySize::DRAM - 1)], &data, sizeof(u16));
    } else if (inRange(addr, (u64)MemoryBase::BootROM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)], &data, sizeof(u16));
    } else {
        switch (addr) {
            default:
                std::printf("Unhandled ME write16 @ 0x%08X = 0x%04X\n", addr, data);

                exit(0);
        }
    }
}

void meWrite32(u32 addr, u32 data) {
    addr &= (u32)MemoryBase::PAddrSpace - 1; // Mask virtual address

    if (inRange(addr, (u64)MemoryBase::MESPRAM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&meSPRAM[addr & ((u32)MemorySize::EDRAM - 1)], &data, sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::VME0, (u64)MemorySize::VME0)) {
        std::printf("[VME     ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::DRAM, (u64)MemorySize::DRAM)) {
        std::memcpy(&dram[addr & ((u32)MemorySize::DRAM - 1)], &data, sizeof(u32));
    } else if (inRange(addr, (u64)MemoryBase::MEMPROT, (u64)MemorySize::MEMPROT)) {
        std::printf("[MEMPROT ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::SysCon, (u64)MemorySize::SysCon)) {
        return syscon::write(CPUID_ME, addr, data);
    } else if (inRange(addr, (u64)MemoryBase::INTC, (u64)MemorySize::INTC)) {
        return intc::write(CPUID_ME, addr, data);
    } else if (inRange(addr, (u64)MemoryBase::VME1, (u64)MemorySize::VME1)) {
        std::printf("[VME     ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);
    } else if (inRange(addr, (u64)MemoryBase::BootROM, (u64)MemorySize::EDRAM)) {
        std::memcpy(&sharedRAM[addr & ((u32)MemorySize::EDRAM - 1)], &data, sizeof(u32));
    } else {
        switch (addr) {
            case 0x1C200000:
                std::printf("[MEFREQ  ] Write @ CPUFREQ = 0x%08X\n", data);

                cpufreq[CPUID_ME] = data;
                break;
            case 0x1C200004:
                std::printf("[MEFREQ  ] Write @ BUSFREQ = 0x%08X\n", data);

                busfreq[CPUID_ME] = data;
                break;
            default:
                std::printf("Unhandled ME write32 @ 0x%08X = 0x%08X\n", addr, data);

                exit(0);
        }
    }
}

void unmapBootROM() {
    resetVector = sharedRAM.data();
    resetSize = (u32)MemorySize::EDRAM;
}

}
