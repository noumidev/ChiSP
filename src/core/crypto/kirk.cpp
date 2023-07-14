/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "kirk.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

#include <cryptopp/aes.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/modes.h>

#include "../memory.hpp"

namespace psp::kirk {

constexpr u64 KHEADER_SIZE = 0x60;
constexpr u64 MHEADER_SIZE = 0x20;

constexpr u8 AES_MASTER_KEY[] = {0x98, 0xC9, 0x40, 0x97, 0x5C, 0x1D, 0x10, 0xE8, 0x7F, 0xE6, 0x0E, 0xA3, 0xFD, 0x03, 0xA8, 0xBA};

enum class KIRKReg {
    PHASE = 0x1DE0000C,
    COMMAND = 0x1DE00010,
    RESULT  = 0x1DE00014,
    STATUS  = 0x1DE0001C,
    PRVSTS  = 0x1DE00028,
    SRC = 0x1DE0002C,
    DST = 0x1DE00030,
};

enum class KIRKCommand {
    DECRYPT_PRIVATE = 1,
};

enum STATUS {
    PHASE_1_DONE = 1 << 0,
};

struct KeyHeaderAES {
    u8 decryptKey[16], cmacKey[16];
    u8 headerHash[16], dataHash[16];
    u8 _unused0[32];
};

static_assert(sizeof(KeyHeaderAES) == KHEADER_SIZE);

struct MetadataHeader {
    u32 _unused0;    // Always 1
    u32 version;     // 0 = AES-CBC, 1 = ECDSA
    u32 _unused1[2]; // 0, 0 or -1
    u32 dataLength, paddingLength;
    u32 _unused2[2]; // 0
} __attribute__((packed));

static_assert(sizeof(MetadataHeader) == MHEADER_SIZE);

u8 cmd; // Current KIRK command

u32 status;

// Buffer addresses
u32 srcAddr, dstAddr;

// Decrypt data with AES (ECB)
void kirkDecryptAES(const u8 *key, u8 *data, u64 size) {
    // Set up AES engine
    auto aes = CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption(key, 16);

    aes.ProcessData(data, data, size);
}

// KIRK command 1
void cmdDecryptPrivate() {
    std::puts("[KIRK    ] Decrypt Private");

    // Get pointers to source and dest buffers
    const auto srcBuffer = memory::getMemoryPointer(srcAddr);
    const auto dstBuffer = memory::getMemoryPointer(dstAddr);

    // Get key and metadata headers
    u8 keyHeader[KHEADER_SIZE], metadataHeader[MHEADER_SIZE];

    std::memcpy(keyHeader, &srcBuffer[0], KHEADER_SIZE);
    std::memcpy(metadataHeader, &srcBuffer[KHEADER_SIZE], MHEADER_SIZE);

    // (Debug) Print headers
    std::puts("Key header is:");
    for (u64 i = 0; i < KHEADER_SIZE; i += 16) {
        std::printf("0x%08X 0x%08X 0x%08X 0x%08X\n", *(u32 *)&keyHeader[i], *(u32 *)&keyHeader[i + 4], *(u32 *)&keyHeader[i + 8], *(u32 *)&keyHeader[i + 12]);
    }

    std::puts("Metadata header is:");
    for (u64 i = 0; i < MHEADER_SIZE; i += 16) {
        std::printf("0x%08X 0x%08X 0x%08X 0x%08X\n", *(u32 *)&metadataHeader[i], *(u32 *)&metadataHeader[i + 4], *(u32 *)&metadataHeader[i + 8], *(u32 *)&metadataHeader[i + 12]);
    }

    MetadataHeader mHeader;
    std::memcpy(&mHeader, metadataHeader, MHEADER_SIZE);

    // Safeguards
    assert(mHeader._unused0 == 1);
    assert(!mHeader._unused1[0]);
    assert(!mHeader._unused2[0]);
    assert(!mHeader._unused2[1]);

    if (!mHeader.version) { // AES-CBC
        std::puts("[KIRK    ] Decrypting key header keys");

        // Get AES key header
        KeyHeaderAES kHeader;
        std::memcpy(&kHeader, keyHeader, KHEADER_SIZE);

        kirkDecryptAES(AES_MASTER_KEY, kHeader.decryptKey, 16);
        kirkDecryptAES(AES_MASTER_KEY, kHeader.cmacKey, 16);

        // (Debug) Print decrypted keys
        std::puts("Decrypt key is:");
        std::printf("0x%08X 0x%08X 0x%08X 0x%08X\n", *(u32 *)&kHeader.decryptKey[0], *(u32 *)&kHeader.decryptKey[4], *(u32 *)&kHeader.decryptKey[8], *(u32 *)&kHeader.decryptKey[12]);
        std::puts("CMAC key is:");
        std::printf("0x%08X 0x%08X 0x%08X 0x%08X\n", *(u32 *)&kHeader.cmacKey[0], *(u32 *)&kHeader.cmacKey[4], *(u32 *)&kHeader.cmacKey[8], *(u32 *)&kHeader.cmacKey[12]);

        // Decrypt data area
        u8 data[mHeader.dataLength];
        std::memcpy(data, &srcBuffer[KHEADER_SIZE + MHEADER_SIZE + mHeader.paddingLength], mHeader.dataLength);

        kirkDecryptAES(kHeader.decryptKey, data, mHeader.dataLength);

        // (Debug) Print decrypted data
        std::puts("Data is:");
        for (u64 i = 0; i < mHeader.dataLength; i += 16) {
            std::printf("0x%08X 0x%08X 0x%08X 0x%08X\n", *(u32 *)&data[i], *(u32 *)&data[i + 4], *(u32 *)&data[i + 8], *(u32 *)&data[i + 12]);
        }

        std::memcpy(dstBuffer, data, mHeader.dataLength);
    } else if (mHeader.version == 1) {
        std::puts("Unimplemented ECDSA");

        exit(0);
    } else {
        std::printf("Invalid KIRK decryption method %u\n", mHeader.version);

        exit(0);
    }
}

void doCommand() {
    status &= ~STATUS::PHASE_1_DONE;

    switch ((KIRKCommand)cmd) {
        case KIRKCommand::DECRYPT_PRIVATE:
            cmdDecryptPrivate();
            break;
        default:
            std::printf("Unhandled KIRK command 0x%02X\n", cmd);

            exit(0);
    }

    status |= STATUS::PHASE_1_DONE;
}

u32 read(u32 addr) {
    switch ((KIRKReg)addr) {
        case KIRKReg::RESULT:
            std::printf("[KIRK    ] Read @ RESULT\n");
            return 0;
        case KIRKReg::STATUS:
            std::printf("[KIRK    ] Read @ STATUS\n");
            return status;
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
        case KIRKReg::PRVSTS:
            std::printf("[KIRK    ] Write @ PRVSTS = 0x%08X\n", data);
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
