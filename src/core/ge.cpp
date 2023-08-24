/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "ge.hpp"

#include <array>
#include <cassert>
#include <cstdio>

#include "dmacplus.hpp"
#include "intc.hpp"
#include "memory.hpp"
#include "psp.hpp"
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
    GEOCLK  = 0x1D400200,
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

enum {
    CMD_NOP = 0x00,
    CMD_VADR = 0x01,
    CMD_IADR = 0x02,
    CMD_PRIM = 0x04,
    CMD_JUMP = 0x08,
    CMD_END = 0x0C,
    CMD_FINISH = 0x0F,
    CMD_BASE = 0x10,
    CMD_VTYPE = 0x12,
    CMD_OFFSET = 0x13,
    CMD_ORIGIN = 0x14,
    CMD_REGION1 = 0x15,
    CMD_REGION2 = 0x16,
    CMD_LTE = 0x17,
    CMD_LE0 = 0x18,
    CMD_LE1 = 0x19,
    CMD_LE2 = 0x1A,
    CMD_LE3 = 0x1B,
    CMD_CLE = 0x1C,
    CMD_BCE = 0x1D,
    CMD_TME = 0x1E,
    CMD_FGE = 0x1F,
    CMD_DTE = 0x20,
    CMD_ABE = 0x21,
    CMD_ATE = 0x22,
    CMD_ZTE = 0x23,
    CMD_STE = 0x24,
    CMD_AAE = 0x25,
    CMD_PCE = 0x26,
    CMD_CTE = 0x27,
    CMD_LOE = 0x28,
    CMD_WEIGHT0 = 0x2C,
    CMD_WEIGHT1 = 0x2D,
    CMD_WEIGHT2 = 0x2E,
    CMD_WEIGHT3 = 0x2F,
    CMD_WEIGHT4 = 0x30,
    CMD_WEIGHT5 = 0x31,
    CMD_WEIGHT6 = 0x32,
    CMD_WEIGHT7 = 0x33,
    CMD_DIVIDE = 0x36,
    CMD_PPM = 0x37,
    CMD_PFACE = 0x38,
    CMD_SX = 0x42,
    CMD_SY = 0x43,
    CMD_SZ = 0x44,
    CMD_TX = 0x45,
    CMD_TY = 0x46,
    CMD_TZ = 0x47,
    CMD_SU = 0x48,
    CMD_SV = 0x49,
    CMD_TU = 0x4A,
    CMD_TV = 0x4B,
};

struct VTYPE {
    u32 tt; // Tex coord type
    u32 ct; // Color type
    u32 nt; // Normal type
    u32 vt; // Model coordinate type
    u32 wt; // Weight type
    u32 it; // Index type
    u32 wc; // Weight count
    u32 mc;
    bool tru;
};

struct FrameBufferConfig {
    u32 addr, fmt, width, stride, config;
} __attribute__((packed));

struct Registers {
    // --- Addresses

    u32 base;

    // Feature enable
    bool tme, zte;

    // --- Vertices

    VTYPE vtype;

    // Vertex weights
    f32 weight[8];

    // --- Coordinates

    // Viewport
    f32 s[3], t[3];

    // --- Textures

    // Texture scale + offset
    f32 su, sv, tu, tv;
};

std::array<u32, SCR_WIDTH * SCR_HEIGHT> fb;

u32 cmdargs[256];

// Matrices
u32 bone[96], world[12], view[12], proj[12], tgen[12], count[12];

u32 control;
u32 edramsize2;
u32 listaddr, stalladdr;
u32 retaddr[2];
u32 vtxaddr, idxaddr;
u32 origin[3];
u32 cmdstatus, irqstatus;

u32 geoclk;

u32 unknown[1];

u32 pc, stall;

FrameBufferConfig fbConfig;

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
    } else if ((addr >= 0x1D400C00) && (addr < 0x1D400D80)) {
        const auto idx = (addr - 0x1D400C00) >> 2;

        std::printf("[GE      ] Read @ BONE%u\n", idx);

        return bone[idx];
    } else if ((addr >= 0x1D400D80) && (addr < 0x1D400DB0)) {
        const auto idx = (addr - 0x1D400D80) >> 2;

        std::printf("[GE      ] Read @ WORLD%u\n", idx);

        return world[idx];
    } else if ((addr >= 0x1D400DB0) && (addr < 0x1D400DE0)) {
        const auto idx = (addr - 0x1D400DB0) >> 2;

        std::printf("[GE      ] Read @ VIEW%u\n", idx);

        return view[idx];
    } else if ((addr >= 0x1D400DE0) && (addr < 0x1D400E20)) {
        const auto idx = (addr - 0x1D400DE0) >> 2;

        std::printf("[GE      ] Read @ PROJ%u\n", idx);

        return proj[idx];
    } else if ((addr >= 0x1D400E20) && (addr < 0x1D400E50)) {
        const auto idx = (addr - 0x1D400E20) >> 2;

        std::printf("[GE      ] Read @ TGEN%u\n", idx);

        return tgen[idx];
    } else if ((addr >= 0x1D400E50) && (addr < 0x1D400E80)) {
        const auto idx = (addr - 0x1D400E50) >> 2;

        std::printf("[GE      ] Read @ COUNT%u\n", idx);

        return count[idx];
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
        case GEReg::LISTADDR:
            std::printf("[GE      ] Read @ LISTADDR\n");

            return listaddr;
        case GEReg::STALLADDR:
            std::printf("[GE      ] Read @ STALLADDR\n");

            return stalladdr;
        case GEReg::RETADDR1:
            std::printf("[GE      ] Read @ RETADDR1\n");

            return retaddr[0];
        case GEReg::RETADDR2:
            std::printf("[GE      ] Read @ RETADDR2\n");

            return retaddr[1];
        case GEReg::VTXADDR:
            std::printf("[GE      ] Read @ VTXADDR\n");

            return vtxaddr;
        case GEReg::IDXADDR:
            std::printf("[GE      ] Read @ IDXADDR\n");

            return idxaddr;
        case GEReg::ORIGIN0:
            std::printf("[GE      ] Read @ ORIGIN0\n");

            return origin[0];
        case GEReg::ORIGIN1:
            std::printf("[GE      ] Read @ ORIGIN1\n");

            return origin[1];
        case GEReg::ORIGIN2:
            std::printf("[GE      ] Read @ ORIGIN2\n");

            return origin[2];
        case GEReg::GEOCLK:
            std::printf("[GE      ] Read @ GEOCLK\n");

            return geoclk;
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
            case CMD_NOP:
                std::printf("[GE      ] [0x%08X] NOP\n", cpc);
                break;
            case CMD_VADR:
                vtxaddr = regs.base | (instr & 0xFFFFFF);

                std::printf("[GE      ] [0x%08X] VADR 0x%08X\n", cpc, vtxaddr);
                break;
            case CMD_IADR:
                idxaddr = regs.base | (instr & 0xFFFFFF);

                std::printf("[GE      ] [0x%08X] IADR 0x%08X\n", cpc, idxaddr);
                break;
            case CMD_PRIM:
                std::printf("[GE      ] [0x%08X] PRIM %u, %u\n", cpc, (instr >> 16) & 7, instr & 0xFFFF);

                exit(0);
            case CMD_JUMP:
                pc = regs.base | (instr & 0xFFFFFF);

                std::printf("[GE      ] [0x%08X] JUMP 0x%08X\n", cpc, pc);
                break;
            case CMD_END:
                std::printf("[GE      ] [0x%08X] END\n", cpc);

                isEnd = true;

                scheduler::addEvent(idSendIRQ, CMDSTATUS::END, (count) ? 5 * count : 128);
                break;
            case CMD_FINISH:
                std::printf("[GE      ] [0x%08X] FINISH\n", cpc);

                scheduler::addEvent(idSendIRQ, CMDSTATUS::FINISH, (count) ? 5 * count : 128);
                break;
            case CMD_BASE:
                regs.base = (instr & 0xFF0000) << 8;

                std::printf("[GE      ] [0x%08X] BASE 0x%08X\n", cpc, regs.base);
                break;
            case CMD_VTYPE:
                {
                    auto vtype = &regs.vtype;

                    std::printf("[GE      ] [0x%08X] VTYPE 0x%06X\n", cpc, instr & 0xFFFFFF);

                    vtype->tt = (instr >>  0) & 3;
                    vtype->ct = (instr >>  2) & 7;
                    vtype->nt = (instr >>  5) & 3;
                    vtype->vt = (instr >>  7) & 3;
                    vtype->wt = (instr >>  9) & 3;
                    vtype->it = (instr >> 11) & 3;
                    vtype->wc = (instr >> 14) & 7;
                    vtype->mc = (instr >> 18) & 7;

                    vtype->tru = instr & (1 << 23);
                }
                break;
            case CMD_OFFSET:
                std::printf("[GE      ] [0x%08X] OFFSET 0x%08X\n", cpc, (instr & 0xFFFFFF) << 8);
                break;
            case CMD_ORIGIN:
                std::printf("[GE      ] [0x%08X] ORIGIN\n", cpc);
                break;
            case CMD_REGION1:
                std::printf("[GE      ] [0x%08X] REGION1 0x%05X\n", cpc, instr & 0xFFFFF);
                break;
            case CMD_REGION2:
                std::printf("[GE      ] [0x%08X] REGION2 0x%05X\n", cpc, instr & 0xFFFFF);
                break;
            case CMD_LTE:
                std::printf("[GE      ] [0x%08X] LTE %u\n", cpc, instr & 1);
                break;
            case CMD_LE0:
            case CMD_LE1:
            case CMD_LE2:
            case CMD_LE3:
                std::printf("[GE      ] [0x%08X] LE%d %u\n", cpc, cmd - CMD_LE0, instr & 1);
                break;
            case CMD_CLE:
                std::printf("[GE      ] [0x%08X] CLE %u\n", cpc, instr & 1);
                break;
            case CMD_BCE:
                std::printf("[GE      ] [0x%08X] BCE %u\n", cpc, instr & 1);
                break;
            case CMD_TME:
                std::printf("[GE      ] [0x%08X] TME %u\n", cpc, instr & 1);

                regs.tme = instr & 1;
                break;
            case CMD_FGE:
                std::printf("[GE      ] [0x%08X] FGE %u\n", cpc, instr & 1);
                break;
            case CMD_DTE:
                std::printf("[GE      ] [0x%08X] DTE %u\n", cpc, instr & 1);
                break;
            case CMD_ABE:
                std::printf("[GE      ] [0x%08X] ABE %u\n", cpc, instr & 1);
                break;
            case CMD_ATE:
                std::printf("[GE      ] [0x%08X] ATE %u\n", cpc, instr & 1);
                break;
            case CMD_ZTE:
                std::printf("[GE      ] [0x%08X] ZTE %u\n", cpc, instr & 1);

                regs.zte = instr & 1;
                break;
            case CMD_STE:
                std::printf("[GE      ] [0x%08X] STE %u\n", cpc, instr & 1);
                break;
            case CMD_AAE:
                std::printf("[GE      ] [0x%08X] AAE %u\n", cpc, instr & 1);
                break;
            case CMD_PCE:
                std::printf("[GE      ] [0x%08X] PCE %u\n", cpc, instr & 1);
                break;
            case CMD_CTE:
                std::printf("[GE      ] [0x%08X] CTE %u\n", cpc, instr & 1);
                break;
            case CMD_LOE:
                std::printf("[GE      ] [0x%08X] LOE %u\n", cpc, instr & 1);
                break;
            case CMD_WEIGHT0:
            case CMD_WEIGHT1:
            case CMD_WEIGHT2:
            case CMD_WEIGHT3:
            case CMD_WEIGHT4:
            case CMD_WEIGHT5:
            case CMD_WEIGHT6:
            case CMD_WEIGHT7:
                {
                    const auto idx = cmd - CMD_WEIGHT0;

                    regs.weight[idx] = toFloat(instr << 8);

                    std::printf("[GE      ] [0x%08X] WEIGHT%d %f\n", cpc, idx, regs.weight[idx]);
                }
                break;
            case CMD_DIVIDE:
                std::printf("[GE      ] [0x%08X] DIVIDE 0x%04X\n", cpc, instr & 0x7FFF);
                break;
            case CMD_PPM:
                std::printf("[GE      ] [0x%08X] PPM %u\n", cpc, instr & 3);
                break;
            case CMD_PFACE:
                std::printf("[GE      ] [0x%08X] PFACE %u\n", cpc, instr & 1);
                break;
            case CMD_SX:
                regs.s[0] = toFloat(instr << 8);

                std::printf("[GE      ] [0x%08X] SX %f\n", cpc, regs.s[0]);
                break;
            case CMD_SY:
                regs.s[1] = toFloat(instr << 8);
                
                std::printf("[GE      ] [0x%08X] SY %f\n", cpc, regs.s[1]);
                break;
            case CMD_SZ:
                regs.s[2] = toFloat(instr << 8);
                
                std::printf("[GE      ] [0x%08X] SZ %f\n", cpc, regs.s[2]);
                break;
            case CMD_TX:
                regs.t[0] = toFloat(instr << 8);

                std::printf("[GE      ] [0x%08X] TX %f\n", cpc, regs.t[0]);
                break;
            case CMD_TY:
                regs.t[1] = toFloat(instr << 8);
                
                std::printf("[GE      ] [0x%08X] TY %f\n", cpc, regs.t[1]);
                break;
            case CMD_TZ:
                regs.t[2] = toFloat(instr << 8);
                
                std::printf("[GE      ] [0x%08X] TZ %f\n", cpc, regs.t[2]);
                break;
            case CMD_SU:
                regs.su = toFloat(instr << 8);
                
                std::printf("[GE      ] [0x%08X] SU %f\n", cpc, regs.su);
                break;
            case CMD_SV:
                regs.sv = toFloat(instr << 8);
                
                std::printf("[GE      ] [0x%08X] SV %f\n", cpc, regs.sv);
                break;
            case CMD_TU:
                regs.tu = toFloat(instr << 8);
                
                std::printf("[GE      ] [0x%08X] TU %f\n", cpc, regs.tu);
                break;
            case CMD_TV:
                regs.tv = toFloat(instr << 8);
                
                std::printf("[GE      ] [0x%08X] TV %f\n", cpc, regs.tv);
                break;
            default:
                std::printf("[GE      ] [0x%08X] Command 0x%02X (0x%08X)\n", cpc, cmd, instr);

                exit(0);
        }

        ++count;
    }

    control &= ~CONTROL::RUNNING;
}

void drawScreen() {
    // Get frame buffer configuration
    dmacplus::getFBConfig((u32 *)&fbConfig);

    if (!(fbConfig.config & 1) || !fbConfig.width || !fbConfig.stride) { // Scanout disabled
        std::memset(fb.data(), 0, fb.size());

        update((u8 *)fb.data());

        return;
    }

    std::printf("[GE      ] FB addr: 0x%08X, format: %u, width: %u, stride: %u\n", fbConfig.addr, fbConfig.fmt, fbConfig.width, fbConfig.stride);

    assert(!fbConfig.fmt); // RGBA8888
    assert(fbConfig.width == SCR_WIDTH);
    assert(fbConfig.stride == 512);

    const auto fbBase = memory::getMemoryPointer(fbConfig.addr);

    for (int i = 0; i < (int)SCR_HEIGHT; i++) {
        std::memcpy(&fb[i * SCR_WIDTH], &fbBase[4 * i * fbConfig.stride], SCR_WIDTH * 4);
    }

    exit(0);
}

}
