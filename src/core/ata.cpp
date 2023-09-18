/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "ata.hpp"

#include <cassert>
#include <cstdio>
#include <queue>

#include "intc.hpp"
#include "scheduler.hpp"

namespace psp::ata {

enum class ATA0Reg {
    UNKNOWN0 = 0x1D600000,
    UNKNOWN1 = 0x1D600004,
    UNKNOWN2 = 0x1D600010,
    UNKNOWN3 = 0x1D600014,
    UNKNOWN4 = 0x1D60001C,
    UNKNOWN5 = 0x1D600034,
    UNKNOWN8 = 0x1D600044,
};

enum class ATA1Reg {
    // Command block
    DATA  = 0x1D700000,
    FEATURES = 0x1D700001,
    ERROR = 0x1D700001,
    SECTORCOUNT = 0x1D700002,
    LBALOW  = 0x1D700003,
    LBAMID  = 0x1D700004,
    LBAHIGH = 0x1D700005,
    DRIVE   = 0x1D700006,
    COMMAND = 0x1D700007,
    STATUS1 = 0x1D700007,
    ENDOFDATA = 0x1D700008,
    // Control block
    DEVCTL  = 0x1D70000E,
    STATUS2 = 0x1D70000E,
};

namespace ATAStatus {
    enum : u8 {
        DEVICE_ERROR = 1 << 0,
        DATA_REQUEST = 1 << 3,
        DEVICE_READY = 1 << 6,
        DEVICE_BUSY  = 1 << 7,
    };
}

namespace ATAInterrupt {
    enum : u8 {
        CD = 1 << 0,
        IO = 1 << 1,
    };
}

namespace ATAPICommand {
    enum : u8 {
        PACKET = 0xA0,
    };
};

namespace SCSICommand {
    enum : u8 {
        TEST_UNIT_READY = 0x00,
        INQUIRY = 0x12,
    };
};

// ATA0 regs
u32 ata0Unknown[9];

// ATA1 (ATAPI) regs
u8 features, error;
u8 sectorcount;
u8 lbalow, lbamid, lbahigh;
u8 drive;
u8 command, status;
u8 devctl;

std::queue<u8> inQueue, outQueue;

u32 length;

FILE *umd = NULL;

u64 idFinishSCSICommand;

void sendIRQ(u8 irq) {
    sectorcount = irq;

    intc::sendIRQ(intc::InterruptSource::ATAPI);
}

void clearIRQ() {
    intc::clearIRQ(intc::InterruptSource::ATAPI);
}

void startSCSICommand(int cyclesUntilEvent) {
    status |= ATAStatus::DEVICE_BUSY;

    scheduler::addEvent(idFinishSCSICommand, 0, cyclesUntilEvent);
}

void finishSCSICommand() {
    // Set command length
    lbamid = length & 0xFF;
    lbahigh = (length >> 8) & 0xFF;

    error = 0;

    status |= ATAStatus::DATA_REQUEST;
    status |= ATAStatus::DEVICE_READY;
    status &= ~ATAStatus::DEVICE_BUSY;

    sendIRQ(ATAInterrupt::IO);
}

void reset() {
    status = ATAStatus::DEVICE_READY;

    // Write the packet device signature
    sectorcount = lbalow = 1;
    lbamid = 0x14;
    lbahigh = 0xEB;
}

void init(const char *path) {
    std::puts("[ATA     ] OK");

    idFinishSCSICommand = scheduler::registerEvent([](int) {finishSCSICommand();});

    umd = std::fopen(path, "rb");

    if (umd == NULL) {
        std::puts("[ATA     ] No UMD inserted");
    }

    reset();
}

void clearInQueue() {
    while (!inQueue.empty()) {
        inQueue.pop();
    }
}

u8 discardAndGetInQueue(int num) {
    for (int n = 0; n < num; n++) {
        inQueue.pop();
    }

    const u8 data = inQueue.front(); inQueue.pop();

    return data;
}

u16 getOutQueue() {
    assert(!outQueue.empty());

    u16 data = 0;

    data |= (u16)outQueue.front(); outQueue.pop();

    if (!outQueue.empty()) {
        data |= (u16)outQueue.front() << 8; outQueue.pop();
    }

    if (outQueue.empty()) {
        status &= ~ATAStatus::DATA_REQUEST;

        sendIRQ(ATAInterrupt::CD | ATAInterrupt::IO);
    }

    return data;
}

void scsiCmdInquiry() {
    length = discardAndGetInQueue(3);

    std::printf("[ATA     ] INQUIRY, length: 0x%02X\n", length);

    // Push some information about the UMD and drive
    // Values taken from JPCSP
    outQueue.push(0x05);
    outQueue.push(0x80);
    outQueue.push(0x00);
    outQueue.push(0x32);
    outQueue.push(0x5C);
    outQueue.push(0x00);
    outQueue.push(0x00);
    outQueue.push(0x00);

    // Drive ID taken from my PSP
    constexpr const char DRIVE_ID[] = "SCEI    UMD ROM DRIVE       1.150AAug30 ,2005   ";

    for (size_t i = 0; i < (sizeof(DRIVE_ID) - 1); i++) {
        outQueue.push(DRIVE_ID[i]);
    }

    // INQUIRY takes about 2ms
    startSCSICommand(2000 * scheduler::_1US);
}

void scsiCmdTestUnitReady() {
    std::printf("[ATA     ] TEST_UNIT_READY\n");

    // Note: I think this doesn't do anything if the logical unit is ready??

    // TODO: test command duration
    startSCSICommand(2000 * scheduler::_1US);
}

void doCommand() {
    switch (command) {
        case ATAPICommand::PACKET:
            std::puts("[ATA     ] PACKET");

            // Update command block
            status = ATAStatus::DEVICE_READY | ATAStatus::DATA_REQUEST;

            sendIRQ(ATAInterrupt::CD);
            break;
        default:
            std::printf("Unhandled ATA command 0x%02X\n", command);

            exit(0);
    }
}

void doSCSICommand() {
    const auto scsiCommand = inQueue.front(); inQueue.pop();

    switch (scsiCommand) {
        case SCSICommand::TEST_UNIT_READY:
            scsiCmdTestUnitReady();
            break;
        case SCSICommand::INQUIRY:
            scsiCmdInquiry();
            break;
        default:
            std::printf("Unhandled SCSI command 0x%02X\n", scsiCommand);

            exit(0);
    }

    clearInQueue();
}

u32 ata0Read(u32 addr) {
    switch ((ATA0Reg)addr) {
        case ATA0Reg::UNKNOWN0:
            std::printf("[ATA     ] Unknown read @ 0x%08X\n", addr);

            return 0x10033; // ?
        case ATA0Reg::UNKNOWN2:
            std::printf("[ATA     ] Unknown read @ 0x%08X\n", addr);

            return 0;
        case ATA0Reg::UNKNOWN5:
            std::printf("[ATA     ] Unknown read @ 0x%08X\n", addr);

            return 0;
        case ATA0Reg::UNKNOWN8:
            std::printf("[ATA     ] Unknown read @ 0x%08X\n", addr);

            return (~ata0Unknown[8]) >> 16;
        default:
            std::printf("Unhandled ATA read @ 0x%08X\n", addr);

            exit(0);
    }
}

void ata0Write(u32 addr, u32 data) {
    switch ((ATA0Reg)addr) {
        case ATA0Reg::UNKNOWN1:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[1] = data;
            break;
        case ATA0Reg::UNKNOWN2:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[2] = data;
            break;
        case ATA0Reg::UNKNOWN3:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[3] = data;
            break;
        case ATA0Reg::UNKNOWN4:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[4] = data;
            break;
        case ATA0Reg::UNKNOWN5:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[5] = data;
            break;
        case ATA0Reg::UNKNOWN8:
            std::printf("[ATA     ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            ata0Unknown[8] = data;
            break;
        default:
            std::printf("Unhandled ATA write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

u8 ata1Read8(u32 addr) {
    switch ((ATA1Reg)addr) {
        case ATA1Reg::ERROR:
            std::puts("[ATA     ] Read @ ERROR");
        
            return error;
        case ATA1Reg::SECTORCOUNT:
            std::puts("[ATA     ] Read @ SECTORCOUNT");
        
            return sectorcount;
        case ATA1Reg::LBALOW:
            std::puts("[ATA     ] Read @ LBALOW");
        
            return lbalow;
        case ATA1Reg::LBAMID:
            std::puts("[ATA     ] Read @ LBAMID");
        
            return lbamid;
        case ATA1Reg::LBAHIGH:
            std::puts("[ATA     ] Read @ LBAHIGH");
        
            return lbahigh;
        case ATA1Reg::DRIVE:
            std::puts("[ATA     ] Read @ DRIVE");
        
            return drive;
        case ATA1Reg::STATUS1:
            clearIRQ();
        case ATA1Reg::STATUS2:
            std::puts("[ATA     ] Read @ STATUS");
        
            return status;
        default:
            std::printf("Unhandled ATA read @ 0x%08X\n", addr);

            exit(0);
    }
}

u16 ata1Read16(u32 addr) {
    switch ((ATA1Reg)addr) {
        case ATA1Reg::DATA:
            std::puts("[ATA     ] Read @ DATA");

            return getOutQueue();
        default:
            std::printf("Unhandled ATA read @ 0x%08X\n", addr);

            exit(0);
    }
}

void ata1Write8(u32 addr, u8 data) {
    switch ((ATA1Reg)addr) {
        case ATA1Reg::FEATURES:
            std::printf("[ATA     ] Write @ FEATURES = 0x%02X\n", data);

            features = data;
            break;
        case ATA1Reg::SECTORCOUNT:
            std::printf("[ATA     ] Write @ SECTORCOUNT = 0x%02X\n", data);

            sectorcount = data;
            break;
        case ATA1Reg::LBALOW:
            std::printf("[ATA     ] Write @ LBALOW = 0x%02X\n", data);

            lbalow = data;
            break;
        case ATA1Reg::LBAMID:
            std::printf("[ATA     ] Write @ LBAMID = 0x%02X\n", data);

            lbamid = data;
            break;
        case ATA1Reg::LBAHIGH:
            std::printf("[ATA     ] Write @ LBAHIGH = 0x%02X\n", data);

            lbahigh = data;
            break;
        case ATA1Reg::DRIVE:
            std::printf("[ATA     ] Write @ DRIVE = 0x%02X\n", data);
        
            drive = data;
            break;
        case ATA1Reg::COMMAND:
            std::printf("[ATA     ] Write @ COMMAND = 0x%02X\n", data);

            command = data;

            doCommand();
            break;
        case ATA1Reg::ENDOFDATA:
            std::printf("[ATA     ] Write @ ENDOFDATA = 0x%02X\n", data);

            doSCSICommand();
            break;
        case ATA1Reg::DEVCTL:
            std::printf("[ATA     ] Write @ DEVCTL = 0x%02X\n", data);
            break;
        default:
            std::printf("Unhandled ATA write @ 0x%08X = 0x%02X\n", addr, data);

            exit(0);
    }
}

void ata1Write16(u32 addr, u16 data) {
    switch ((ATA1Reg)addr) {
        case ATA1Reg::DATA:
            std::printf("[ATA     ] Write @ DATA = 0x%04X\n", data);

            inQueue.push((u8)data);
            inQueue.push((u8)(data >> 8));
            break;
        default:
            std::printf("Unhandled ATA write @ 0x%08X = 0x%04X\n", addr, data);

            exit(0);
    }
}

bool isUMDInserted() {
    return umd != NULL;
}

}
