/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "kirk.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

#include "../memory.hpp"

namespace psp::kirk {

enum class KIRKReg {
    PHASE = 0x1DE0000C,
    COMMAND = 0x1DE00010,
    SRC = 0x1DE0002C,
    DST = 0x1DE00030,
};

enum class KIRKCommand {
    DECRYPT_PRIVATE = 1,
};

u8 cmd; // Current KIRK command

// Buffer addresses
u32 srcAddr, dstAddr;

// KIRK command 1
void cmdDecryptPrivate() {
    std::puts("[KIRK    ] Decrypt Private");

    // Get pointers to source and dest buffers
    const auto srcBuffer = memory::getMemoryPointer(srcAddr);
    const auto dstBuffer = memory::getMemoryPointer(dstAddr);

    // Get key and metadata headers
    u8 keyHeader[0x60], metadataHeader[0x30];

    std::memcpy(keyHeader, &srcBuffer[0], sizeof(keyHeader));
    std::memcpy(metadataHeader, &srcBuffer[0x60], sizeof(metadataHeader));

    // (Debug) Print headers
    std::puts("Key header is:");
    for (u64 i = 0; i < 0x60; i += 16) {
        std::printf("0x%08X 0x%08X 0x%08X 0x%08X\n", *(u32 *)&keyHeader[i], *(u32 *)&keyHeader[i + 4], *(u32 *)&keyHeader[i + 8], *(u32 *)&keyHeader[i + 12]);
    }

    std::puts("Metadata header is:");
    for (u64 i = 0; i < 0x30; i += 16) {
        std::printf("0x%08X 0x%08X 0x%08X 0x%08X\n", *(u32 *)&metadataHeader[i], *(u32 *)&metadataHeader[i + 4], *(u32 *)&metadataHeader[i + 8], *(u32 *)&metadataHeader[i + 12]);
    }

    exit(0);
}

void doCommand() {
    switch ((KIRKCommand)cmd) {
        case KIRKCommand::DECRYPT_PRIVATE:
            cmdDecryptPrivate();
            break;
        default:
            std::printf("Unhandled KIRK command 0x%02X\n", cmd);

            exit(0);
    }
}

u32 read(u32 addr) {
    switch ((KIRKReg)addr) {
        default:
            std::printf("[KIRK    ] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    switch ((KIRKReg)addr) {
        case KIRKReg::PHASE:
            std::printf("[KIRK    ] Write @ PHASE = 0x%08X\n", data);

            if (data & 1) doCommand();

            assert(!(data & 2));
            break;
        case KIRKReg::COMMAND:
            std::printf("[KIRK    ] Write @ COMMAND = 0x%08X\n", data);

            cmd = data;
            break;
        case KIRKReg::SRC:
            std::printf("[KIRK    ] Write @ SRC = 0x%08X\n", data);

            srcAddr = data;
            break;
        case KIRKReg::DST:
            std::printf("[KIRK    ] Write @ DST = 0x%08X\n", data);

            dstAddr = data;
            break;
        default:
            std::printf("[KIRK    ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

}
