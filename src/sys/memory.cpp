/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#include "sys/memory.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "common/file.hpp"

namespace sys::memory {

// Memory arrays
std::array<u8, MemorySize::BootROM> bootROM;

// Page table
std::array<u8 *, PAGE_NUM> pageTable;

void init(const char *bootPath) {
    reset();

    PLOG_INFO << "Loading boot ROM \"" << bootPath << "\"";

    readFile(bootPath, bootROM.data(), MemorySize::BootROM);
}

void deinit() {}

void reset() {
    PLOG_INFO << "Memory reset";

    // Clear page table
    pageTable.fill(NULL);
}

void *getPointer(u32 vaddr) {
    // Mask virtual address
    vaddr = maskAddress(vaddr);

    const u32 page = vaddr >> PAGE_SHIFT;

    if (pageTable[page] != NULL) {
        return (void *)&pageTable[page][vaddr & PAGE_MASK];
    }

    PLOG_FATAL << "Invalid pointer (address = " << std::hex << vaddr << ")";

    exit(0);
}

template<>
u8 read(u32 vaddr) {
    // Mask virtual address
    vaddr = maskAddress(vaddr);

    const u32 page = vaddr >> PAGE_SHIFT;

    // Look up address in page table, return data directly if it's mapped
    if (pageTable[page] != NULL) {
        return pageTable[page][vaddr & PAGE_MASK];
    } else {
        switch (vaddr) {
            default:
                PLOG_FATAL << "Unrecognized read8 (addr = " << std::hex << vaddr << ")";

                exit(0);
        }
    }
}

template<>
u16 read(u32 vaddr) {
    // Mask virtual address
    vaddr = maskAddress(vaddr);

    const u32 page = vaddr >> PAGE_SHIFT;

    // Look up address in page table, return data directly if it's mapped
    if (pageTable[page] != NULL) {
        u16 data;
        std::memcpy(&data, &pageTable[page][vaddr & PAGE_MASK], sizeof(u16));

        return data;
    } else {
        switch (vaddr) {
            default:
                PLOG_FATAL << "Unrecognized read16 (addr = " << std::hex << vaddr << ")";

                exit(0);
        }
    }
}

template<>
u32 read(u32 vaddr) {
    // Mask virtual address
    vaddr = maskAddress(vaddr);

    const u32 page = vaddr >> PAGE_SHIFT;

    // Look up address in page table, return data directly if it's mapped
    if (pageTable[page] != NULL) {
        u32 data;
        std::memcpy(&data, &pageTable[page][vaddr & PAGE_MASK], sizeof(u32));

        return data;
    } else {
        switch (vaddr) {
            default:
                PLOG_FATAL << "Unrecognized read32 (addr = " << std::hex << vaddr << ")";

                exit(0);
        }
    }
}

template<>
void write(u32 vaddr, const u8 data) {
    // Mask virtual address
    vaddr = maskAddress(vaddr);

    const u32 page = vaddr >> PAGE_SHIFT;

    // Look up address in page table, write data directly if it's mapped
    if (pageTable[page] != NULL) {
        pageTable[page][vaddr & PAGE_MASK] = data;
    } else {
        switch (vaddr) {
            default:
                PLOG_FATAL << "Unrecognized write8 (addr = " << std::hex << vaddr << ", data = " << (u32)data << ")";

                exit(0);
        }
    }
}

template<>
void write(u32 vaddr, const u16 data) {
    // Mask virtual address
    vaddr = maskAddress(vaddr);

    const u32 page = vaddr >> PAGE_SHIFT;

    // Look up address in page table, write data directly if it's mapped
    if (pageTable[page] != NULL) {
        std::memcpy(&pageTable[page][vaddr & PAGE_MASK], &data, sizeof(u16));
    } else {
        switch (vaddr) {
            default:
                PLOG_FATAL << "Unrecognized write16 (addr = " << std::hex << vaddr << ", data = " << data << ")";

                exit(0);
        }
    }
}

template<>
void write(u32 vaddr, const u32 data) {
    // Mask virtual address
    vaddr = maskAddress(vaddr);

    const u32 page = vaddr >> PAGE_SHIFT;

    // Look up address in page table, write data directly if it's mapped
    if (pageTable[page] != NULL) {
        std::memcpy(&pageTable[page][vaddr & PAGE_MASK], &data, sizeof(u32));
    } else {
        switch (vaddr) {
            default:
                PLOG_FATAL << "Unrecognized write32 (addr = " << std::hex << vaddr << ", data = " << data << ")";

                exit(0);
        }
    }
}

}
