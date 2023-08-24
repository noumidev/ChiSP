/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "dmacplus.hpp"

#include <cassert>
#include <cstdio>

#include "intc.hpp"
#include "memory.hpp"
#include "scheduler.hpp"

namespace psp::dmacplus {

enum class DMACPlusReg {
    IRQSTATUS = 0x1C800004,
    IRQCLEAR  = 0x1C800008,
    ERRORSTATUS = 0x1C80000C,
    FRAMEBUFADDR = 0x1C800100,
    FRAMEBUFFMT  = 0x1C800104,
    FRAMEBUFWIDTH  = 0x1C800108,
    FRAMEBUFSTRIDE = 0x1C80010C,
    FRAMEBUFCONFIG = 0x1C800110,
    DMASRC  = 0x1C800180,
    DMADST  = 0x1C800184,
    DMATAG  = 0x1C800188,
    DMAATTR = 0x1C80018C,
    DMASTATUS = 0x1C800190,
};

// For channel 3-5
struct Channel {
    u32 srcAddr, dstAddr, tagAddr;
    u32 status;

    // Attributes
    u32 length;
    u32 srcStep, dstStep;
    u32 srcLengthShift, dstLengthShift;
    u32 unknown; // Bits 24-25
    bool srcInc, dstInc, triggerIRQ;
};

enum ChannelStatus {
    IN_PROGRESS  = 1 << 0,
    DDR_REQUIRED = 1 << 8,
};

enum FRAMEBUFCONFIG {
    ENABLE_SCANOUT = 1 << 0,
};

const char *chnNames[] = {
    "Sc2Me", "Me2Sc", "Sc128",
};

u32 irqstatus, irqen, errorstatus, erroren;

// DmacplusLcdc
u32 framebufaddr, framebuffmt, framebufwidth, framebufstride, framebufconfig;

// DmacplusAvc
u32 csc[17];

// Sc2Me, Me2Sc, Sc128
Channel channels[3];

u64 idFinishTransfer;

void checkInterrupt() {
    if (irqstatus & irqen) {
        intc::sendIRQ(intc::InterruptSource::DMACplus);
    } else {
        intc::clearIRQ(intc::InterruptSource::DMACplus);
    }
}

void sendIRQ(int chnID) {
    irqstatus |= 1 << chnID;

    checkInterrupt();
}

void finishTransfer(int chnID) {
    std::printf("[DMACplus] Channel %d end\n", chnID);

    auto &chn = channels[chnID];

    chn.status &= ~ChannelStatus::IN_PROGRESS;

    if (chn.triggerIRQ) {
        sendIRQ(chnID + 2); // Skip LCDC and AVC channels
    }
}

void doTransfer(int chnID) {
    std::printf("[DMACplus] Channel %d (%s) transfer\n", chnID, chnNames[chnID]);

    assert(chnID == 2);

    const auto &chn = channels[chnID];

    std::printf("DMASRC: 0x%08X, DMADST: 0x%08X, DMATAG: 0x%08X\n", chn.srcAddr, chn.dstAddr, chn.tagAddr);
    std::printf("Length: 0x%X, src step: %u, dst step: %u\n", chn.length, chn.srcStep, chn.dstStep);
    std::printf("Src length shift: %u, dst length shift: %u, src increment: %d, dst increment: %d\n", chn.srcLengthShift, chn.dstLengthShift, chn.srcInc, chn.dstInc);
    std::printf("Τrigger IRQ: %d\n", chn.triggerIRQ);

    assert(!chn.tagAddr);
    assert((chn.srcStep == 1) && (chn.dstStep == 1));
    assert((chn.srcLengthShift == 4) && (chn.dstLengthShift == 4)); // What do those do?

    auto srcAddr = chn.srcAddr;
    auto dstAddr = chn.dstAddr;

    const auto srcOffset = (chn.srcInc) ? 0x10 : -0x10;
    const auto dstOffset = (chn.dstInc) ? 0x10 : -0x10;

    for (u32 i = 0; i < chn.length; i++) {
        u8 data[16];

        memory::read128 (srcAddr, data);
        memory::write128(dstAddr, data);

        srcAddr += srcOffset;
        dstAddr += dstOffset;
    }

    scheduler::addEvent(idFinishTransfer, chnID, 8 * chn.length);
}

void init() {
    idFinishTransfer = scheduler::registerEvent([](int chnID) {finishTransfer(chnID);});

    irqen = 0x1F;
}

u32 read(u32 addr) {
    if ((addr >= 0x1C800120) && (addr < 0x1C800164)) {
        const auto idx = (addr - 0x1C800120) >> 2;

        std::printf("[DMACplus] Unhandled CSC read @ 0x%08X\n", addr);
    
        return csc[idx];
    } else if ((addr >= 0x1C800180) && (addr < 0x1C8001D4)) {
        const auto idx = (addr >> 5) & 3;

        auto &chn = channels[idx];

        switch ((DMACPlusReg)(addr & ~0x60)) {
            case DMACPlusReg::DMASRC:
                std::printf("[DMACplus] Read @ DMA%dSRC\n", idx);

                return chn.srcAddr;
            case DMACPlusReg::DMADST:
                std::printf("[DMACplus] Read @ DMA%dDST\n", idx);

                return chn.dstAddr;
            case DMACPlusReg::DMATAG:
                std::printf("[DMACplus] Read @ DMA%dTAG\n", idx);

                return chn.tagAddr;
            case DMACPlusReg::DMAATTR:
                {
                    std::printf("[DMACplus] Read @ DMA%dATTR\n", idx);

                    u32 data = 0;

                    data |= chn.length;
                    data |= chn.srcStep << 12;
                    data |= chn.dstStep << 15;
                    data |= chn.srcLengthShift << 18;
                    data |= chn.dstLengthShift << 21;
                    data |= chn.unknown << 24;
                    data |= (u32)chn.srcInc << 26;
                    data |= (u32)chn.dstInc << 27;
                    data |= (u32)chn.triggerIRQ << 31;

                    return data;
                }
                break;
            case DMACPlusReg::DMASTATUS:
                std::printf("[DMACplus] Read @ DMA%dSTATUS\n", idx);

                return chn.status;
            default:
                std::printf("[DMACplus] Unhandled channel %d read @ 0x%08X\n", idx, addr);

                exit(0);
        }
    }

    switch ((DMACPlusReg)addr) {
        case DMACPlusReg::IRQSTATUS:
            std::puts("[DMACplus] Read @ IRQSTATUS");

            return irqstatus;
        case DMACPlusReg::ERRORSTATUS:
            std::puts("[DMACplus] Read @ ERRORSTATUS");

            return errorstatus;
        case DMACPlusReg::FRAMEBUFADDR:
            std::puts("[DMACplus] Read @ FRAMEBUFADDR");

            return framebufaddr;
        case DMACPlusReg::FRAMEBUFFMT:
            std::puts("[DMACplus] Read @ FRAMEBUFFMT");

            return framebuffmt;
        case DMACPlusReg::FRAMEBUFWIDTH:
            std::puts("[DMACplus] Read @ FRAMEBUFWIDTH");

            return framebufwidth;
        case DMACPlusReg::FRAMEBUFSTRIDE:
            std::puts("[DMACplus] Read @ FRAMEBUFSTRIDE");

            return framebufstride;
        case DMACPlusReg::FRAMEBUFCONFIG:
            std::puts("[DMACplus] Read @ FRAMEBUFCONFIG");

            return framebufconfig;
        default:
            std::printf("[DMACplus] Unhandled read @ 0x%08X\n", addr);

            exit(0);
    }
}

void write(u32 addr, u32 data) {
    if ((addr >= 0x1C800120) && (addr < 0x1C800164)) {
        const auto idx = (addr - 0x1C800120) >> 2;

        std::printf("[DMACplus] Unhandled CSC write @ 0x%08X = 0x%08X\n", addr, data);
    
        csc[idx] = data;

        return;
    } else if ((addr >= 0x1C800180) && (addr < 0x1C8001D4)) {
        const auto idx = (addr >> 5) & 3;

        auto &chn = channels[idx];

        switch ((DMACPlusReg)(addr & ~0x60)) {
            case DMACPlusReg::DMASRC:
                std::printf("[DMACplus] Write @ DMA%dSRC = 0x%08X\n", idx, data);

                chn.srcAddr = data;
                break;
            case DMACPlusReg::DMADST:
                std::printf("[DMACplus] Write @ DMA%dDST = 0x%08X\n", idx, data);

                chn.dstAddr = data;
                break;
            case DMACPlusReg::DMATAG:
                std::printf("[DMACplus] Write @ DMA%dTAG = 0x%08X\n", idx, data);

                chn.tagAddr = data;
                break;
            case DMACPlusReg::DMAATTR:
                {
                    std::printf("[DMACplus] Write @ DMA%dATTR = 0x%08X\n", idx, data);

                    chn.length = (data & 0xFFF);
                    chn.srcStep = (data >> 12) & 7;
                    chn.dstStep = (data >> 15) & 7;
                    chn.srcLengthShift = (data >> 18) & 7;
                    chn.dstLengthShift = (data >> 21) & 7;
                    chn.unknown = (data >> 24) & 3;
                    chn.srcInc = data & (1 << 26);
                    chn.dstInc = data & (1 << 27);
                    chn.triggerIRQ = data & (1 << 31);
                }
                break;
            case DMACPlusReg::DMASTATUS:
                std::printf("[DMACplus] Write @ DMA%dSTATUS = 0x%08X\n", idx, data);

                chn.status = data;
                
                if (chn.status & ChannelStatus::IN_PROGRESS) {
                    doTransfer(idx);
                }
                break;
            default:
                std::printf("[DMACplus] Unhandled channel %d write @ 0x%08X = 0x%08X\n", idx, addr, data);

                exit(0);
        }

        return;
    }

    switch ((DMACPlusReg)addr) {
        case DMACPlusReg::IRQCLEAR:
            std::printf("[DMACplus] Write @ IRQCLEAR = 0x%08X\n", data);

            irqstatus &= ~data;

            checkInterrupt();
            break;
        case DMACPlusReg::FRAMEBUFADDR:
            std::printf("[DMACplus] Write @ FRAMEBUFADDR = 0x%08X\n", data);

            framebufaddr = data;
            break;
        case DMACPlusReg::FRAMEBUFFMT:
            std::printf("[DMACplus] Write @ FRAMEBUFFMT = 0x%08X\n", data);

            framebuffmt = data;
            break;
        case DMACPlusReg::FRAMEBUFWIDTH:
            std::printf("[DMACplus] Write @ FRAMEBUFWIDTH = 0x%08X\n", data);

            framebufwidth = data;
            break;
        case DMACPlusReg::FRAMEBUFSTRIDE:
            std::printf("[DMACplus] Write @ FRAMEBUFSTRIDE = 0x%08X\n", data);

            framebufstride = data;
            break;
        case DMACPlusReg::FRAMEBUFCONFIG:
            std::printf("[DMACplus] Write @ FRAMEBUFCONFIG = 0x%08X\n", data);

            framebufconfig = data;
            break;
        default:
            std::printf("[DMACplus] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

void getFBConfig(u32 *data) {
    data[0] = framebufaddr;
    data[1] = framebuffmt;
    data[2] = framebufwidth;
    data[3] = framebufstride;
    data[4] = framebufconfig;
}

}
