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
    // Clear page table
    for (u32 i = 0; i < PAGE_NUM; i++) {
        pageTable[i] = NULL;
    }
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

template<typename T>
T read(u32 vaddr) {
    // Mask virtual address
    vaddr = maskAddress(vaddr);

    const u32 page = vaddr >> PAGE_SHIFT;

    // Look up address in page table, return data directly if it's mapped
    if (pageTable[page] != NULL) {
        T data;
        std::memcpy(&data, &pageTable[page][vaddr & PAGE_MASK], sizeof(T));

        return data;
    } else {
        switch (vaddr) {
            PLOG_FATAL << "Unrecognized read" << 8 * sizeof(T) << " (addr = " << std::hex << vaddr << ")";

            exit(0);
        }
    }
}

template<typename T>
void write(u32 vaddr, const T data) {
    // Mask virtual address
    vaddr = maskAddress(vaddr);

    const u32 page = vaddr >> PAGE_SHIFT;

    // Look up address in page table, write data directly if it's mapped
    if (pageTable[page] != NULL) {
        std::memcpy(&pageTable[page][vaddr & PAGE_MASK], &data, sizeof(T));
    } else {
        switch (vaddr) {
            PLOG_FATAL << "Unrecognized write" << 8 * sizeof(T) << " (addr = " << std::hex << vaddr << ", data" << data << ")";

            exit(0);
        }
    }
}

}
