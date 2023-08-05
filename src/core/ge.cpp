/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "ge.hpp"

#include <cassert>
#include <cstdio>

#include "intc.hpp"
#include "memory.hpp"
#include "scheduler.hpp"

namespace psp::ge {

enum class GEReg {
    UNKNOWN0 = 0x1D400004,
    EDRAMSIZE1 = 0x1D400008,
    CONTROL = 0x1D400100,
    LISTADDR  = 0x1D400108,
    STALLADDR = 0x1D40010C,
    RETADDR1  = 0x1D400110,
    RETADDR2  = 0x1D400114,
    VTXADDR = 0x1D400118,
    IDXADDR = 0x1D40011C,
    ORIGIN0 = 0x1D400120,
    ORIGIN1 = 0x1D400124,
    ORIGIN2 = 0x1D400128,
    CMDSTATUS = 0x1D400304,
    IRQSTATUS = 0x1D400308,
    IRQSWAP = 0x1D40030C,
    CMDSWAP = 0x1D400310,
    EDRAMSIZE2 = 0x1D400400,
};

enum CONTROL {
    RUNNING = 1 << 0,
    BCOND_FALSE = 1 << 1,
    IS_DEPTH_1  = 1 << 8,
    IS_DEPTH_2  = 1 << 9,
};

enum CMDSTATUS {
    SIGNAL = 0,
    END = 1,
    FINISH = 2,
    ERROR  = 3,
};

struct Registers {
    u32 base;
};

u32 cmdargs[256];

u32 control;
u32 edramsize2;
u32 listaddr, stalladdr;
u32 retaddr[2];
u32 vtxaddr, idxaddr;
u32 origin[3];
u32 cmdstatus, irqstatus;

u32 unknown[1];

u32 pc, stall;

Registers regs;

u64 idSendIRQ;

void executeDisplayList();

void checkInterrupt() {
    if (irqstatus) {
        intc::sendIRQ(intc::InterruptSource::GE);
    } else {
        intc::clearIRQ(intc::InterruptSource::GE);
    }
}

void sendIRQ(int irq) {
    cmdstatus |= 1 << irq;
    irqstatus |= 1 << irq;

    intc::sendIRQ(intc::InterruptSource::GE);
}

void init() {
    idSendIRQ = scheduler::registerEvent([](int irq) {sendIRQ(irq);});
}

u32 read(u32 addr) {
    if ((addr >= 0x1D400800) && (addr < 0x1D400C00)) {
        const auto idx = (addr - 0x1D400800) >> 2;

        std::printf("[GE      ] Read @ CMDARGS%u\n", idx);

        return cmdargs[idx];
    }

    switch ((GEReg)addr) {
        case GEReg::UNKNOWN0:
            std::printf("[GE      ] Unknown read @ 0x%08X\n", addr);

            return unknown[0];
        case GEReg::EDRAMSIZE1:
            std::printf("[GE      ] Read @ EDRAMSIZE1\n");

            return 0x200000 >> 10;
        case GEReg::CONTROL:
            std::printf("[GE      ] Read @ CONTROL\n");

            return control;
        case GEReg::CMDSTATUS:
            std::printf("[GE      ] Read @ CMDSTATUS\n");

            return cmdstatus;
        default:
            std::printf("[GE      ] Unhandled read @ 0x%08X\n", addr);

         exit(0);
    }
}

void write(u32 addr, u32 data) {
    switch ((GEReg)addr) {
        case GEReg::UNKNOWN0:
            std::printf("[GE      ] Unknown write @ 0x%08X = 0x%08X\n", addr, data);

            unknown[0] = data;
            break;
        case GEReg::CONTROL:
            std::printf("[GE      ] Write @ CONTROL = 0x%08X\n", data);

            control = (control & ~CONTROL::RUNNING) | (data & CONTROL::RUNNING);

            if (control & CONTROL::RUNNING) {
                executeDisplayList();
            }
            break;
        case GEReg::LISTADDR:
            std::printf("[GE      ] Write @ LISTADDR = 0x%08X\n", data);

            listaddr = pc = data;
            break;
        case GEReg::STALLADDR:
            std::printf("[GE      ] Write @ STALLADDR = 0x%08X\n", data);

            stalladdr = stall = data;

            if (control & CONTROL::RUNNING) executeDisplayList();
            break;
        case GEReg::RETADDR1:
            std::printf("[GE      ] Write @ RETADDR1 = 0x%08X\n", data);

            retaddr[0] = data;
            break;
        case GEReg::RETADDR2:
            std::printf("[GE      ] Write @ RETADDR2 = 0x%08X\n", data);

            retaddr[1] = data;
            break;
        case GEReg::VTXADDR:
            std::printf("[GE      ] Write @ VTXADDR = 0x%08X\n", data);

            vtxaddr = data;
            break;
        case GEReg::IDXADDR:
            std::printf("[GE      ] Write @ IDXADDR = 0x%08X\n", data);

            idxaddr = data;
            break;
        case GEReg::ORIGIN0:
            std::printf("[GE      ] Write @ ORIGIN0 = 0x%08X\n", data);

            origin[0] = data;
            break;
        case GEReg::ORIGIN1:
            std::printf("[GE      ] Write @ ORIGIN1 = 0x%08X\n", data);

            origin[1] = data;
            break;
        case GEReg::ORIGIN2:
            std::printf("[GE      ] Write @ ORIGIN2 = 0x%08X\n", data);

            origin[2] = data;
            break;
        case GEReg::IRQSTATUS:
            std::printf("[GE      ] Write @ IRQSTATUS = 0x%08X\n", data);

            irqstatus &= ~data;

            checkInterrupt();
            break;
        case GEReg::IRQSWAP:
            std::printf("[GE      ] Write @ IRQSWAP = 0x%08X\n", data);

            irqstatus &= ~data;

            checkInterrupt();
            break;
        case GEReg::CMDSWAP:
            std::printf("[GE      ] Write @ CMDSWAP = 0x%08X\n", data);

            cmdstatus &= ~data;
            irqstatus &= ~data;

            checkInterrupt();
            break;
        case GEReg::EDRAMSIZE2:
            std::printf("[GE      ] Write @ EDRAMSIZE2 = 0x%08X\n", data);

            edramsize2 = data;
            break;
        default:
            std::printf("[GE      ] Unhandled write @ 0x%08X = 0x%08X\n", addr, data);

            exit(0);
    }
}

void executeDisplayList() {
    if (pc == listaddr) {
        std::printf("[GE      ] Executing display list @ 0x%08X, stall: 0x%08X\n", listaddr, stalladdr);
    }

    pc &= 0x1FFFFFFF;
    stall &= 0x1FFFFFFF;

    auto isEnd = false;

    i64 count = 0;

    while (!isEnd) {
        if (stall && (pc == stall)) {
            return;
        }

        const auto instr = memory::read32(pc);
        const auto cmd = instr >> 24;

        cmdargs[cmd] = instr;

        const auto cpc = pc;
        pc += 4;

        switch (cmd) {
            case 0x09:
            case 0x0A:
            case 0x0B:
            case 0x0E:
                std::printf("Unhandled GE command 0x%02X (0x%08X) @ 0x%08X\n", cmd, instr, cpc);

                exit(0);
            case 0x08:
                pc = regs.base | (instr & 0xFFFFFF);

                std::printf("[GPU     ] [0x%08X] JUMP 0x%08X\n", cpc, pc);
                break;
            case 0x0C:
                std::printf("[GE      ] [0x%08X] END\n", cpc);

                isEnd = true;

                scheduler::addEvent(idSendIRQ, CMDSTATUS::END, 5 * count);
                break;
            case 0x0F:
                std::printf("[GE      ] [0x%08X] FINISH\n", cpc);

                scheduler::addEvent(idSendIRQ, CMDSTATUS::FINISH, 5 * count);
                break;
            case 0x10:
                regs.base = (instr & 0xFF0000) << 8;

                std::printf("[GPU     ] [0x%08X] BASE 0x%08X\n", cpc, regs.base);
                break;
            default: // Ignore "unimportant" commands
                std::printf("[GE      ] [0x%08X] Command 0x%02X (0x%08X)\n", cpc, cmd, instr);
                break;
        }

        ++count;
    }

    control &= ~CONTROL::RUNNING;
}

}
