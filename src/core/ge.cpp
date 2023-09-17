/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "ge.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

#include "dmacplus.hpp"
#include "intc.hpp"
#include "memory.hpp"
#include "psp.hpp"
#include "scheduler.hpp"

namespace psp::ge {

using memory::MemoryBase;
using memory::MemorySize;

constexpr auto ENABLE_DEBUG_PRINT = false;

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
    CMD_BEZIER = 0x05,
    CMD_SPLINE = 0x06,
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
    CMD_BONEN = 0x2A,
    CMD_BONED = 0x2B,
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
    CMD_WORLDN = 0x3A,
    CMD_WORLDD = 0x3B,
    CMD_VIEWN = 0x3C,
    CMD_VIEWD = 0x3D,
    CMD_PROJN = 0x3E,
    CMD_PROJD = 0x3F,
    CMD_TGENN = 0x40,
    CMD_TGEND = 0x41,
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
    CMD_OFFSETX = 0x4C,
    CMD_OFFSETY = 0x4D,
    CMD_SHADE = 0x50,
    CMD_NREV = 0x51,
    CMD_MATERIAL = 0x53,
    CMD_MEC = 0x54,
    CMD_MAC = 0x55,
    CMD_MDC = 0x56,
    CMD_MSC = 0x57,
    CMD_MAA = 0x58,
    CMD_MK = 0x5B,
    CMD_AC = 0x5C,
    CMD_AA = 0x5D,
    CMD_LMODE = 0x5E,
    CMD_LTYPE0 = 0x5F,
    CMD_LTYPE1 = 0x60,
    CMD_LTYPE2 = 0x61,
    CMD_LTYPE3 = 0x62,
    CMD_LX0 = 0x63,
    CMD_LY0 = 0x64,
    CMD_LZ0 = 0x65,
    CMD_LX1 = 0x66,
    CMD_LY1 = 0x67,
    CMD_LZ1 = 0x68,
    CMD_LX2 = 0x69,
    CMD_LY2 = 0x6A,
    CMD_LZ2 = 0x6B,
    CMD_LX3 = 0x6C,
    CMD_LY3 = 0x6D,
    CMD_LZ3 = 0x6E,
    CMD_LDX0 = 0x6F,
    CMD_LDY0 = 0x70,
    CMD_LDZ0 = 0x71,
    CMD_LDX1 = 0x72,
    CMD_LDY1 = 0x73,
    CMD_LDZ1 = 0x74,
    CMD_LDX2 = 0x75,
    CMD_LDY2 = 0x76,
    CMD_LDZ2 = 0x77,
    CMD_LDX3 = 0x78,
    CMD_LDY3 = 0x79,
    CMD_LDZ3 = 0x7A,
    CMD_LKA0 = 0x7B,
    CMD_LKB0 = 0x7C,
    CMD_LKC0 = 0x7D,
    CMD_LKA1 = 0x7E,
    CMD_LKB1 = 0x7F,
    CMD_LKC1 = 0x80,
    CMD_LKA2 = 0x81,
    CMD_LKB2 = 0x82,
    CMD_LKC2 = 0x83,
    CMD_LKA3 = 0x84,
    CMD_LKB3 = 0x85,
    CMD_LKC3 = 0x86,
    CMD_LKS0 = 0x87,
    CMD_LKS1 = 0x88,
    CMD_LKS2 = 0x89,
    CMD_LKS3 = 0x8A,
    CMD_LKO0 = 0x8B,
    CMD_LKO1 = 0x8C,
    CMD_LKO2 = 0x8D,
    CMD_LKO3 = 0x8E,
    CMD_LAC0 = 0x8F,
    CMD_LDC0 = 0x90,
    CMD_LSC0 = 0x91,
    CMD_LAC1 = 0x92,
    CMD_LDC1 = 0x93,
    CMD_LSC1 = 0x94,
    CMD_LAC2 = 0x95,
    CMD_LDC2 = 0x96,
    CMD_LSC2 = 0x97,
    CMD_LAC3 = 0x98,
    CMD_LDC3 = 0x99,
    CMD_LSC3 = 0x9A,
    CMD_CULL = 0x9B,
    CMD_FBP = 0x9C,
    CMD_FBW = 0x9D,
    CMD_ZBP = 0x9E,
    CMD_ZBW = 0x9F,
    CMD_TBP0 = 0xA0,
    CMD_TBP1 = 0xA1,
    CMD_TBP2 = 0xA2,
    CMD_TBP3 = 0xA3,
    CMD_TBP4 = 0xA4,
    CMD_TBP5 = 0xA5,
    CMD_TBP6 = 0xA6,
    CMD_TBP7 = 0xA7,
    CMD_TBW0 = 0xA8,
    CMD_TBW1 = 0xA9,
    CMD_TBW2 = 0xAA,
    CMD_TBW3 = 0xAB,
    CMD_TBW4 = 0xAC,
    CMD_TBW5 = 0xAD,
    CMD_TBW6 = 0xAE,
    CMD_TBW7 = 0xAF,
    CMD_CBP = 0xB0,
    CMD_CBW = 0xB1,
    CMD_XBP1 = 0xB2,
    CMD_XBW1 = 0xB3,
    CMD_XBP2 = 0xB4,
    CMD_XBW2 = 0xB5,
    CMD_TSIZE0 = 0xB8,
    CMD_TSIZE1 = 0xB9,
    CMD_TSIZE2 = 0xBA,
    CMD_TSIZE3 = 0xBB,
    CMD_TSIZE4 = 0xBC,
    CMD_TSIZE5 = 0xBD,
    CMD_TSIZE6 = 0xBE,
    CMD_TSIZE7 = 0xBF,
    CMD_TMAP = 0xC0,
    CMD_TSHADE = 0xC1,
    CMD_TMODE = 0xC2,
    CMD_TPF = 0xC3,
    CMD_CLOAD = 0xC4,
    CMD_CLUT = 0xC5,
    CMD_TFILTER = 0xC6,
    CMD_TWRAP = 0xC7,
    CMD_TLEVEL = 0xC8,
    CMD_TFUNC = 0xC9,
    CMD_TEC = 0xCA,
    CMD_TFLUSH = 0xCB,
    CMD_TSYNC = 0xCC,
    CMD_FOG1 = 0xCD,
    CMD_FOG2 = 0xCE,
    CMD_FC = 0xCF,
    CMD_TSLOPE = 0xD0,
    CMD_FPF = 0xD2,
    CMD_CMODE = 0xD3,
    CMD_SCISSOR1 = 0xD4,
    CMD_SCISSOR2 = 0xD5,
    CMD_MINZ = 0xD6,
    CMD_MAXZ = 0xD7,
    CMD_CTEST = 0xD8,
    CMD_CREF = 0xD9,
    CMD_CMSK = 0xDA,
    CMD_ATEST = 0xDB,
    CMD_STEST = 0xDC,
    CMD_SOP = 0xDD,
    CMD_ZTEST = 0xDE,
    CMD_BLEND = 0xDF,
    CMD_FIXA = 0xE0,
    CMD_FIXB = 0xE1,
    CMD_DITH1 = 0xE2,
    CMD_DITH2 = 0xE3,
    CMD_DITH3 = 0xE4,
    CMD_DITH4 = 0xE5,
    CMD_LOP = 0xE6,
    CMD_ZMSK = 0xE7,
    CMD_PMSK1 = 0xE8,
    CMD_PMSK2 = 0xE9,
    CMD_XPOS1 = 0xEB,
    CMD_XPOS2 = 0xEC,
    CMD_XSIZE = 0xEE,
};

enum {
    PRIM_POINT,
    PRIM_LINE,
    PRIM_LINESTRIP,
    PRIM_TRIANGLE,
    PRIM_TRIANGLESTRIP,
    PRIM_TRIANGLEFAN,
    PRIM_SPRITE,
    PRIM_RESERVED,
};

const char *primNames[] {
    "Point",
    "Line",
    "LineStrip",
    "Triangle",
    "TriangleStrip",
    "TriangleFan",
    "Sprite",
    "Reserved",
};

enum {
    CLUT_CPF_RGB565,
    CLUT_CPF_RGBA5551,
    CLUT_CPF_RGBA4444,
    CLUT_CPF_RGBA8888,
};

enum {
    ZTF_NEVER,
    ZTF_ALWAYS,
    ZTF_EQUAL,
    ZTF_NOTEQUAL,
    ZTF_LESS,
    ZTF_LEQUAL,
    ZTF_GREATER,
    ZTF_GEQUAL,
};

enum PSM {
    PSM16,
    PSM32,
};

const char *psmNames[] {
    "PSM16",
    "PSM32",
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
    bool tme, zte, iip;

    // --- Vertices

    VTYPE vtype;

    // Vertex weights
    f32 weight[8];

    // --- Coordinates

    // Screen coordinate offset
    f32 offsetx, offsety;

    // Viewport
    f32 s[3], t[3];

    // --- Textures

    // Base pointer + width
    u32 tbp[8], tbw[8];

    // CLUT base pointer
    u32 cbp;

    // CLUT
    u32 np;
    u32 cpf, sft, msk, csa;

    // Texture scale + offset
    f32 su, sv, tu, tv;

    // Texture size
    f32 tw[8], th[8];

    // Texture mapping
    u32 tmn, tmi;

    // Texture format
    u32 tpf;
    bool ext;

    // Texture mode
    bool hsm, mc;
    u32 mxl;

    // Texture wrapping
    bool twms, twmt;

    // Texture function
    u32 txf;
    bool tcc, cd;

    // Texture environment
    f32 tec[3];

    // --- Frame buffer
    u32 fbp, fbw, fpf;

    // --- Depth buffer
    u32 zbp, zbw, ztf;
    bool zmsk;

    // --- Depth range
    u32 minz, maxz;

    // --- Scissor area
    f32 sx1, sx2, sy1, sy2;

    // --- CLEAR mode
    bool set, cen, aen, zen;
};

struct Vertex {
    // Weights
    f32 w[8];

    // Tex coords
    f32 s, t;

    // Color
    f32 c[4];

    // Normal vector
    f32 n[3];

    // Model coordinate (X, Y, Z, W)
    f32 m[4];
};

std::array<u32, SCR_WIDTH * SCR_HEIGHT> fb;

std::array<u32, 16 * 32> clut;

u32 cmdargs[256];

// Matrices
f32 bone[96], world[12], view[12], proj[12], tgen[12], count[12];

// Indices
u32 bonen, worldn, viewn, projn, tgenn;

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

inline u32 convertRGBA4444(u32 in) {
    const auto color  = ((in & 0xF000) << 12) | ((in & 0xF00) << 8) | ((in & 0xF0) << 4) | (in & 0xF);

    return color | (color << 4);
}

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

        return *(u32 *)&bone[idx];
    } else if ((addr >= 0x1D400D80) && (addr < 0x1D400DB0)) {
        const auto idx = (addr - 0x1D400D80) >> 2;

        std::printf("[GE      ] Read @ WORLD%u\n", idx);

        return *(u32 *)&world[idx];
    } else if ((addr >= 0x1D400DB0) && (addr < 0x1D400DE0)) {
        const auto idx = (addr - 0x1D400DB0) >> 2;

        std::printf("[GE      ] Read @ VIEW%u\n", idx);

        return *(u32 *)&view[idx];
    } else if ((addr >= 0x1D400DE0) && (addr < 0x1D400E20)) {
        const auto idx = (addr - 0x1D400DE0) >> 2;

        std::printf("[GE      ] Read @ PROJ%u\n", idx);

        return *(u32 *)&proj[idx];
    } else if ((addr >= 0x1D400E20) && (addr < 0x1D400E50)) {
        const auto idx = (addr - 0x1D400E20) >> 2;

        std::printf("[GE      ] Read @ TGEN%u\n", idx);

        return *(u32 *)&tgen[idx];
    } else if ((addr >= 0x1D400E50) && (addr < 0x1D400E80)) {
        const auto idx = (addr - 0x1D400E50) >> 2;

        std::printf("[GE      ] Read @ COUNT%u\n", idx);

        return *(u32 *)&count[idx];
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

inline u32 getAddr8(u32 base, u32 width, u32 x, u32 y) {
    return base + 4 * (x >> 2) + 4 * ((width * y) >> 2) + (x & 3);
}

inline u32 getAddr16(u32 base, u32 width, u32 x, u32 y) {
    return base + 4 * (x >> 1) + 4 * ((width * y) >> 1) + 2 * (x & 1);
}

inline u32 getAddr32(u32 base, u32 width, u32 x, u32 y) {
    return base + 4 * x + 4 * width * y;
}

template<PSM psm>
u32 readVRAM(u32 base, u32 width, u32 x, u32 y) {
    switch (psm) {
        case PSM::PSM16:
            base += 4 * (x >> 1) + 4 * ((width * y) >> 1);
            break;
        case PSM::PSM32:
            base += 4 * x + 4 * width * y;
            break;
        default:
            std::printf("Unhandled pixel storage mode %s\n", psmNames[psm]);

            exit(0);
    }

    base &= (u32)MemorySize::EDRAM - 1;

    const auto edram = memory::getMemoryPointer((u32)MemoryBase::EDRAM);

    u32 data = 0;
    switch (psm) {
        case PSM::PSM16:
            std::memcpy(&data, &edram[base + 2 * (x & 1)], sizeof(u16));
            break;
        case PSM::PSM32:
            std::memcpy(&data, &edram[base], sizeof(u32));
            break;
        default:
            std::printf("Unhandled pixel storage mode %s\n", psmNames[psm]);

            exit(0);
    }

    return data;
}

template<PSM psm>
void writeVRAM(u32 base, u32 width, u32 x, u32 y, u32 data) {
    switch (psm) {
        case PSM::PSM16:
            base += 4 * (x >> 1) + 4 * ((width * y) >> 1);
            break;
        case PSM::PSM32:
            base += 4 * x + 4 * width * y;
            break;
        default:
            std::printf("Unhandled pixel storage mode %s\n", psmNames[psm]);

            exit(0);
    }

    base &= (u32)MemorySize::EDRAM - 1;

    const auto edram = memory::getMemoryPointer((u32)MemoryBase::EDRAM);

    switch (psm) {
        case PSM::PSM16:
            std::memcpy(&edram[base + 2 * (x & 1)], &data, sizeof(u16));
            break;
        case PSM::PSM32:
            std::memcpy(&edram[base], &data, sizeof(u32));
            break;
        default:
            std::printf("Unhandled pixel storage mode %s\n", psmNames[psm]);

            exit(0);
    }
}

void transform3(f32 *mtx, Vertex *vtxList, u32 count) {
    for (u32 i = 0; i < count; i++) {
        const auto vtx = &vtxList[i];

        if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Vertex %u transform\n", i);

        f32 w[3];

        // Multiply model coordinates by matrix
        for (int j = 0; j < 3; j++) {
            w[j] = mtx[j] * vtx->m[0] + mtx[j + 3] * vtx->m[1] + mtx[j + 6] * vtx->m[2];
        }

        // Add translation vector
        for (int j = 0; j < 3; j++) {
            vtx->m[j] = w[j] + mtx[9 + j];
        }

        if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] X: %f, Y: %f, Z: %f\n", vtx->m[0], vtx->m[1], vtx->m[2]);
    }
}

void transform4(f32 *mtx, Vertex *vtxList, u32 count) {
    for (u32 i = 0; i < count; i++) {
        const auto vtx = &vtxList[i];

        if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Vertex %u transform\n", i);

        f32 w[4];

        // Multiply model coordinates by matrix
        for (int j = 0; j < 4; j++) {
            w[j] = mtx[j] * vtx->m[0] + mtx[j + 4] * vtx->m[1] + mtx[j + 8] * vtx->m[2] + mtx[j + 12] * 1.0;
        }

        std::memcpy(vtx->m, w, sizeof(w));

        if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] X: %f, Y: %f, Z: %f, W: %f\n", vtx->m[0], vtx->m[1], vtx->m[2], vtx->m[3]);
    }
}

void transformViewport(f32 *s, f32 *t, Vertex *vtxList, u32 count) {
    for (u32 i = 0; i < count; i++) {
        const auto vtx = &vtxList[i];

        if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Vertex %u viewport transform\n", i);

        for (int j = 0; j < 3; j++) {
            vtx->m[j] = s[j] * vtx->m[j] / vtx->m[3] + t[j];
        }

        if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] X: %f, Y: %f, Z: %f\n", vtx->m[0], vtx->m[1], vtx->m[2]);
    }
}

void transformDisplay(f32 offsetx, f32 offsety, Vertex *vtxList, u32 count) {
    for (u32 i = 0; i < count; i++) {
        const auto vtx = &vtxList[i];

        if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Vertex %u display transform\n", i);

        vtx->m[0] -= offsetx;
        vtx->m[1] -= offsety;

        if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] X: %f, Y: %f, Z: %f\n", vtx->m[0], vtx->m[1], vtx->m[2]);
    }
}

bool depthTest(f32 x, f32 y, u16 z) {
    if (!regs.zte) return true; // Depth testing disabled, every pixel passes

    const auto oldZ = (u16)readVRAM<PSM::PSM16>(regs.zbp, regs.zbw, x, y);

    if (!regs.set) { // Handle CLEAR mode
        switch (regs.ztf) {
            case ZTF_NEVER: // Never
                return false;
            case ZTF_ALWAYS: // Always
                break;
            case ZTF_EQUAL: // Equal
                if (z != oldZ) return false;
                break;
            case ZTF_NOTEQUAL: // Not equal
                if (z == oldZ) return false;
                break;
            case ZTF_LESS: // Less
                if (z >= oldZ) return false;
                break;
            case ZTF_LEQUAL: // Less/Equal
                if (z > oldZ) return false;
                break;
            case ZTF_GREATER: // Greater
                if (z <= oldZ) return false;
                break;
            case ZTF_GEQUAL: // Greater/Equal
                if (z < oldZ) return false;
                break;
        }
    }

    // Write new Z
    if (!regs.zmsk || (regs.set && regs.zen)) writeVRAM<PSM::PSM16>(regs.zbp, regs.zbw, x, y, z);

    return true;
}

void loadCLUT() {
    if (!regs.np) return;

    const auto palSize = (regs.cpf == CLUT_CPF_RGBA8888) ? 8 : 16;
    const auto csa = 16 * regs.csa;

    auto clutBase = regs.cbp;

    for (u32 np = 0; np < regs.np; np++) {
        for (int i = 0; i < palSize; i++) {
            const auto index = csa + palSize * np + i;

            switch (regs.cpf) {
                case CLUT_CPF_RGBA8888:
                    clut[index] = memory::read32(clutBase);

                    clutBase += 4;
                    break;
                default:
                    std::printf("Unhandled CLUT buffer format %u\n", regs.cpf);

                    exit(0);
            }

            if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] CLUT[0x%03X] = 0x%08X\n", index, clut[index]);
        }
    }
}

u32 getCLUT(u32 index) {
    u32 finalIndex;

    // Shift index, output low 8 bits
    index >>= regs.sft;
    index &= 0xFF;

    // AND with mask
    index &= regs.msk;

    // Low 4 bits are unchanged
    finalIndex = index & 0xF;

    // OR high 4 bits with CSA[3:0]
    finalIndex |= (index & 0xF0) | ((regs.csa & 0xF) << 4);

    // High CSA bit is bit 8 of final index
    finalIndex |= ((regs.csa & 0x10) << 4);

    return clut[finalIndex];
}

void fetchTex(f32 s, f32 t, f32 *texColors) {
    const auto u = (u32)s;
    const auto v = (u32)t;

    auto clutLookup = false;

    u32 texel;
    switch (regs.tpf) {
        case 2: // RGBA4444
            texel = convertRGBA4444(memory::read16(getAddr16(regs.tbp[0], regs.tbw[0], u, v)));
            //break;
        case 3: // RGBA8888
            texel = memory::read32(getAddr32(regs.tbp[0], regs.tbw[0], u, v));
            break;
        case 5: // 8-bit indexed
            clutLookup = true;

            texel = memory::read8(getAddr8(regs.tbp[0], regs.tbw[0], u, v));
            break;
        default:
            std::printf("Unhandled texture storage mode %u\n", regs.tpf);
            
            exit(0);
    }

    if (clutLookup) {
        texel = getCLUT(texel);
    }

    texColors[3] = (f32)((texel >> 24) & 0xFF);
    texColors[2] = (f32)((texel >> 16) & 0xFF);
    texColors[1] = (f32)((texel >>  8) & 0xFF);
    texColors[0] = (f32)((texel >>  0) & 0xFF);
}

f32 edgeFunction(f32 *a, f32 *b, f32 *c) {
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
}

f32 interpolate(f32 w0, f32 w1, f32 w2, f32 a, f32 b, f32 c, f32 area) {
    return (w0 * a + w1 * b + w2 * c) / area;
}

void interpolateUV(f32 w0, f32 w1, f32 w2, Vertex *vtxList, f32 *texCoords) {
    auto a = &vtxList[0];
    auto b = &vtxList[1];
    auto c = &vtxList[2];

    const auto w = (1.0 / a->m[3]) * w0 + (1.0 / b->m[3]) * w1 + (1.0 / c->m[3]) * w2;
    const auto s = (a->s / a->m[3]) * w0 + (b->s / b->m[3]) * w1 + (c->s / c->m[3]) * w2;
    const auto t = (a->t / a->m[3]) * w0 + (b->t / b->m[3]) * w1 + (c->t / c->m[3]) * w2;

    texCoords[0] = s / w;
    texCoords[1] = t / w;
}

// Interpolate ST coordinates
f32 getTexCoordStart(f32 x, f32 tex1, f32 x1, f32 tex2, f32 x2) {
    auto temp = tex1 * (x2 - x) + tex2 * (x - x1);

    if ((x2 - x1) == 0.0) return tex1;

    return temp / (tex2 - tex1);
}

// Returns ST step
f32 getTexCoordStep(f32 tex1, f32 x1, f32 tex2, f32 x2) {
    if ((x2 - x1) == 0.0) return tex2 - tex1;
    return (tex2 - tex1) / (x2 - x1);
}

void clamp(f32 *colors) {
    for (int i = 0; i < 3; i++) {
        if (colors[i] < 0.0) {
            colors[i] = 0.0;
        } else if (colors [i] > 255.0) {
            colors[i] = 255.0;
        }
    }
}

void drawTriangle(Vertex *vtxList) {
    auto a = &vtxList[0];
    auto b = &vtxList[1];
    auto c = &vtxList[2];

    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Triangle - [%f,%f] [%f,%f] [%f,%f]\n", a->m[0], a->m[1], b->m[0], b->m[1], c->m[0], c->m[1]);

    if (edgeFunction(a->m, b->m, c->m) < 0.0) {
        std::swap(b, c);
    }

    const auto area = edgeFunction(a->m, b->m, c->m);

    // Calculate bounding box
    const auto xMin = std::round(std::max(std::min(c->m[0], std::min(a->m[0], b->m[0])), regs.sx1));
    const auto xMax = std::round(std::min(std::max(c->m[0], std::max(a->m[0], b->m[0])), regs.sx2 + 1.0f));
    const auto yMin = std::round(std::max(std::min(c->m[1], std::min(a->m[1], b->m[1])), regs.sy1));
    const auto yMax = std::round(std::min(std::max(c->m[1], std::max(a->m[1], b->m[1])), regs.sy2 + 1.0f));

    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Bounding box - [%f,%f] [%f,%f]\n", xMin, yMin, xMax, yMax);

    if (xMin >= xMax) return;
    if (yMin >= yMax) return;

    f32 p[2], triColors[4];

    // Flat shading
    if (!regs.iip) {
        for (int i = 0; i < 4; i++) {
            triColors[i] = c->c[i];
        }
    }

    p[1] = yMin;
    for (; p[1] < yMax; p[1] += 1.0) {
        p[0] = xMin;
        for (; p[0] < xMax; p[0] += 1.0) {
            // Calculate weights
            const auto w0 = edgeFunction(b->m, c->m, p);
            const auto w1 = edgeFunction(c->m, a->m, p);
            const auto w2 = edgeFunction(a->m, b->m, p);

            if ((w0 >= 0.0) && (w1 >= 0.0) && (w2 >= 0.0)) {
                const auto z = (u16)std::round(interpolate(w0, w1, w2, a->m[2], b->m[2], c->m[2], area));

                if ((z < regs.minz) || (z > regs.maxz)) continue;

                f32 colors[4];
                u32 finalColor;

                if (regs.iip) {
                    for (int i = 0; i < 4; i++) {
                        triColors[i] = interpolate(w0, w1, w2, a->c[i], b->c[i], c->c[i], area);
                    }
                }

                if (regs.tme) { // Fetch texel
                    f32 texCoords[2];
                    switch (regs.tmn) {
                        case 0: // UV mapping
                            interpolateUV(w0, w1, w2, vtxList, texCoords);

                            // Scale and offset tex coords
                            texCoords[0] = texCoords[0] * regs.su + regs.tu;
                            texCoords[1] = texCoords[1] * regs.sv + regs.tv;
                            break;
                        default:
                            std::printf("Unhandled texture mapping mode %u\n", regs.tmn);

                            exit(0);
                    }

                    // Clamp/wrap tex coords
                    if (regs.twms) {
                        if (texCoords[0] < 0.0) {
                            texCoords[0] = 0.0;
                        } else if (texCoords[0] > 1.0) {
                            texCoords[0] = 1.0;
                        }
                    } else {
                        texCoords[0] = std::fmod(texCoords[0], 1.0f);
                    }

                    if (regs.twmt) {
                        if (texCoords[1] < 0.0) {
                            texCoords[1] = 0.0;
                        } else if (texCoords[1] > 1.0) {
                            texCoords[1] = 1.0;
                        }
                    } else {
                        texCoords[1] = std::fmod(texCoords[1], 1.0f);
                    }

                    // Get texel coordinates
                    texCoords[0] = std::floor(texCoords[0] * regs.tw[0]);
                    texCoords[1] = std::floor(texCoords[1] * regs.th[0]);

                    f32 texColors[4];

                    fetchTex(texCoords[0], texCoords[1], texColors);

                    // Blend texture and vertex colors
                    switch (3) {
                        case 0: // Modulate
                            for (int i = 0; i < 3; i++) {
                                colors[i] = (texColors[i] * triColors[i]) / 255.0;
                            }

                            assert(!regs.tcc);

                            colors[3] = triColors[3];
                            break;
                        case 3: // Replace
                            for (int i = 0; i < 3; i++) {
                                colors[i] = texColors[i];
                            }
                            break;
                        case 4: // Add
                            for (int i = 0; i < 3; i++) {
                                colors[i] = texColors[i] + triColors[i];
                            }

                            assert(!regs.tcc);

                            colors[3] = triColors[3];
                            break;
                        default:
                            std::printf("Unhandled texture function %u\n", regs.txf);

                            exit(0);
                    }

                    clamp(colors);
                } else { // Use vertex colors
                    for (int i = 0; i < 4; i++) {
                        colors[i] = triColors[i];
                    }
                }

                finalColor = ((u32)colors[3] << 24) | ((u32)colors[2] << 16) | ((u32)colors[1] << 8) | (u32)colors[0];

                if (!depthTest((u32)std::round(p[0]), (u32)std::round(p[1]), z)) continue;

                writeVRAM<PSM::PSM32>(regs.fbp, regs.fbw, (u32)std::round(p[0]), (u32)std::round(p[1]), finalColor);
            }
        }
    }
}

void drawSprite(Vertex *vtxList) {
    auto a = &vtxList[0];
    auto b = &vtxList[1];

    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Sprite - [%f,%f] [%f,%f]\n", a->m[0], a->m[1], b->m[0], b->m[1]);

    // Calculate "bounding box"
    const auto xMin = std::round(std::max(std::min(a->m[0], b->m[0]), regs.sx1));
    const auto xMax = std::round(std::min(std::max(a->m[0], b->m[0]), regs.sx2 + 1.0f));
    const auto yMin = std::round(std::max(std::min(a->m[1], b->m[1]), regs.sy1));
    const auto yMax = std::round(std::min(std::max(a->m[1], b->m[1]), regs.sy2 + 1.0f));

    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Bounding box - [%f,%f] [%f,%f]\n", xMin, yMin, xMax, yMax);

    if (xMin >= xMax) return;
    if (yMin >= yMax) return;

    const auto sStart = getTexCoordStart(xMin, a->s, a->m[0], b->s, b->m[0]);
    const auto tStart = getTexCoordStart(yMin, a->t, a->m[1], b->t, b->m[1]);

    const auto sStep = getTexCoordStep(a->s, a->m[0], b->s, b->m[0]);
    const auto tStep = getTexCoordStep(a->t, a->m[1], b->t, b->m[1]);

    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] S: %f, S step: %f, T: %f, T step: %f\n", sStart, sStep, tStart, tStep);

    const auto z = (u16)std::round(b->m[2]);

    if ((z < regs.minz) || (z > regs.maxz)) return;

    auto t = tStart;
    for (auto y = yMin; y < yMax; y += 1.0) {
        auto s = sStart;
        for (auto x = xMin; x < xMax; x += 1.0) {
            if (!depthTest((u32)std::round(x), (u32)std::round(y), z)) {
                s += sStep;
                continue;
            }

            f32 colors[4];
            u32 finalColor;

            if (regs.tme && !regs.set) {
                f32 texColors[4];

                fetchTex(std::floor(s), std::floor(t), texColors);

                // Blend texture and vertex colors
                switch (3) {
                    case 0: // Modulate
                        for (int i = 0; i < 3; i++) {
                            colors[i] = (texColors[i] * b->c[i]) / 255.0;
                        }
                        break;
                    case 3: // Replace
                        for (int i = 0; i < 3; i++) {
                            colors[i] = texColors[i];
                        }
                        break;
                    case 4: // Add
                        for (int i = 0; i < 3; i++) {
                            colors[i] = texColors[i] + b->c[i];
                        }
                        break;
                    default:
                        std::printf("Unhandled texture function %u\n", regs.txf);

                        exit(0);
                }

                if (regs.tcc) {
                    colors[3] = texColors[3];
                } else {
                    colors[3] = b->c[3];
                }

                clamp(colors);
            } else { // Use vertex colors
                for (int i = 0; i < 4; i++) {
                    colors[i] = b->c[i];
                }
            }

            finalColor = ((u32)colors[3] << 24) | ((u32)colors[2] << 16) | ((u32)colors[1] << 8) | (u32)colors[0];

            writeVRAM<PSM::PSM32>(regs.fbp, regs.fbw, (u32)std::round(x), (u32)std::round(y), finalColor);

            s += sStep;
        }

        t += tStep;
    }
}

void drawPrim(u32 prim, u32 count) {
    if (!count) {
        std::puts("[GE      ] Primitive count of 0");

        return;
    }

    // Fetch vertex list
    std::vector<Vertex> vtxList;
    vtxList.resize(count);

    auto vtxAddr = vtxaddr;

    const auto &vtype = regs.vtype;

    assert(!vtype.it);

    for (u32 i = 0; i < count; i++) {
        if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Fetching vertex %u\n", i);

        const auto vtx = &vtxList[i];

        // Fetch weights
        switch (vtype.wt) {
            case 0: // None
                break;
            default:
                std::printf("Unhandled weight type %u\n", vtype.tt);

                exit(0);
        }

        // Fetch tex coords
        switch (vtype.tt) {
            case 0: // None
                break;
            case 2: // 16-bit unsigned
                vtx->s = (f32)(i16)memory::read16(vtxAddr + 0);
                vtx->t = (f32)(i16)memory::read16(vtxAddr + 2);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] S: %f, T: %f\n", vtx->s, vtx->t);

                vtxAddr += 4;
                break;
            case 3: // float32
                vtx->s = toFloat(memory::read32(vtxAddr + 0));
                vtx->t = toFloat(memory::read32(vtxAddr + 4));

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] S: %f, T: %f\n", vtx->s, vtx->t);

                vtxAddr += 8;
                break;
            default:
                std::printf("Unhandled texture coordinate type %u\n", vtype.tt);

                exit(0);
        }

        u32 c;
        // Fetch colors
        switch (vtype.ct) {
            case 0: // None
                for (int j = 0; j < 4; j++) {
                    vtx->c[j] = 0.0;
                }
                break;
            case 7: // RGBA8888
                c = memory::read32(vtxAddr);

                for (int j = 0; j < 4; j++) {
                    vtx->c[j] = (f32)(u8)(c >> (8 * j));
                }

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] R: %f, G: %f, B: %f, A: %f\n", vtx->c[0], vtx->c[1], vtx->c[2], vtx->c[3]);

                vtxAddr += 4;
                break;
            default:
                std::printf("Unhandled color type %u\n", vtype.ct);

                exit(0);
        }

        // Fetch normals
        switch (vtype.nt) {
            case 0: // None
                break;
            case 3:
                for (int j = 0; j < 3; j++) {
                    vtx->n[j] = toFloat(memory::read32(vtxAddr + 4 * j));
                }

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Nx: %f, Ny: %f, Nz: %f\n", vtx->n[0], vtx->n[1], vtx->n[2]);

                vtxAddr += 12;
                break;
            default:
                std::printf("Unhandled normal type %u\n", vtype.nt);

                exit(0);
        }
        
        // Fetch model coordinates
        switch (vtype.vt) {
            case 2: // Signed 16-bit fixed point
                for (int j = 0; j < 3; j++) {
                    vtx->m[j] = ((f32)(i16)memory::read16(vtxAddr + 2 * j));
                }

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] X: %f, Y: %f, Z: %f\n", vtx->m[0], vtx->m[1], vtx->m[2]);

                vtxAddr += 8;
                break;
            case 3:
                for (int j = 0; j < 3; j++) {
                    vtx->m[j] = toFloat(memory::read32(vtxAddr + 4 * j));
                }

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] X: %f, Y: %f, Z: %f\n", vtx->m[0], vtx->m[1], vtx->m[2]);

                vtxAddr += 12;
                break;
            default:
                std::printf("Unhandled model coordinate type %u\n", vtype.vt);

                exit(0);
        }
    }

    // Transform vertex coordinates
    // Note: sprite coordinates are already in the display coordinate system!!
    if (!vtype.tru && (prim != PRIM_SPRITE)) {
        transform3(world, vtxList.data(), count);
        transform3(view , vtxList.data(), count);
        transform4(proj , vtxList.data(), count);
        transformViewport(regs.s, regs.t, vtxList.data(), count);
    }

    if (prim != PRIM_SPRITE) transformDisplay(regs.offsetx, regs.offsety, vtxList.data(), count);

    switch (prim) {
        case PRIM_TRIANGLESTRIP:
            assert(count > 2);

            for (u32 i = 0; i < (count - 2); i++) {
                drawTriangle(&vtxList[i]);
            }
            break;
        case PRIM_SPRITE:
            assert(!(count & 1));

            for (u32 i = 0; i < count; i += 2) {
                drawSprite(&vtxList[i]);
            }
            break;
        default:
            std::printf("Unhandled primitive %s\n", primNames[prim]);

            exit(0);
    }
}

void executeDisplayList() {
    if (pc == listaddr) {
        if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] Executing display list @ 0x%08X, stall: 0x%08X\n", listaddr, stalladdr);
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
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] NOP\n", cpc);
                break;
            case CMD_VADR:
                vtxaddr = regs.base | (instr & 0xFFFFFF);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] VADR 0x%08X\n", cpc, vtxaddr);
                break;
            case CMD_IADR:
                idxaddr = regs.base | (instr & 0xFFFFFF);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] IADR 0x%08X\n", cpc, idxaddr);
                break;
            case CMD_PRIM:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] PRIM %u, %u\n", cpc, (instr >> 16) & 7, instr & 0xFFFF);

                drawPrim((instr >> 16) & 7, instr & 0xFFFF);
                break;
            case CMD_BEZIER:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] BEZIER 0x%04X\n", cpc, instr & 0xFFFF);
                break;
            case CMD_SPLINE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] SPLINE 0x%05X\n", cpc, instr & 0xFFFFF);
                break;
            case CMD_JUMP:
                pc = regs.base | (instr & 0xFFFFFF);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] JUMP 0x%08X\n", cpc, pc);
                break;
            case CMD_END:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] END\n", cpc);

                isEnd = true;

                scheduler::addEvent(idSendIRQ, CMDSTATUS::END, (count) ? 5 * count : 128);
                break;
            case CMD_FINISH:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] FINISH\n", cpc);

                scheduler::addEvent(idSendIRQ, CMDSTATUS::FINISH, (count) ? 5 * count : 128);
                break;
            case CMD_BASE:
                regs.base = (instr & 0xFF0000) << 8;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] BASE 0x%08X\n", cpc, regs.base);
                break;
            case CMD_VTYPE:
                {
                    auto vtype = &regs.vtype;

                    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] VTYPE 0x%06X\n", cpc, instr & 0xFFFFFF);

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
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] OFFSET 0x%08X\n", cpc, (instr & 0xFFFFFF) << 8);
                break;
            case CMD_ORIGIN:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] ORIGIN\n", cpc);
                break;
            case CMD_REGION1:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] REGION1 0x%05X\n", cpc, instr & 0xFFFFF);
                break;
            case CMD_REGION2:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] REGION2 0x%05X\n", cpc, instr & 0xFFFFF);
                break;
            case CMD_LTE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] LTE %u\n", cpc, instr & 1);
                break;
            case CMD_LE0:
            case CMD_LE1:
            case CMD_LE2:
            case CMD_LE3:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] LE%d %u\n", cpc, cmd - CMD_LE0, instr & 1);
                break;
            case CMD_CLE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CLE %u\n", cpc, instr & 1);
                break;
            case CMD_BCE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] BCE %u\n", cpc, instr & 1);
                break;
            case CMD_TME:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TME %u\n", cpc, instr & 1);

                regs.tme = instr & 1;
                break;
            case CMD_FGE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] FGE %u\n", cpc, instr & 1);
                break;
            case CMD_DTE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] DTE %u\n", cpc, instr & 1);
                break;
            case CMD_ABE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] ABE %u\n", cpc, instr & 1);
                break;
            case CMD_ATE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] ATE %u\n", cpc, instr & 1);
                break;
            case CMD_ZTE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] ZTE %u\n", cpc, instr & 1);

                regs.zte = instr & 1;
                break;
            case CMD_STE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] STE %u\n", cpc, instr & 1);
                break;
            case CMD_AAE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] AAE %u\n", cpc, instr & 1);
                break;
            case CMD_PCE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] PCE %u\n", cpc, instr & 1);
                break;
            case CMD_CTE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CTE %u\n", cpc, instr & 1);
                break;
            case CMD_LOE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] LOE %u\n", cpc, instr & 1);
                break;
            case CMD_BONEN:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] BONEN 0x%02X\n", cpc, instr & 0x3F);

                bonen = instr & 0x3F;
                break;
            case CMD_BONED:
                bone[bonen++] = toFloat(instr << 8);
            
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] BONED %f\n", cpc, toFloat(instr << 8));
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

                    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] WEIGHT%d %f\n", cpc, idx, regs.weight[idx]);
                }
                break;
            case CMD_DIVIDE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] DIVIDE 0x%04X\n", cpc, instr & 0x7FFF);
                break;
            case CMD_PPM:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] PPM %u\n", cpc, instr & 3);
                break;
            case CMD_PFACE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] PFACE %u\n", cpc, instr & 1);
                break;
            case CMD_WORLDN:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] WORLDN %u\n", cpc, instr & 0xF);

                worldn = instr & 0xF;
                break;
            case CMD_WORLDD:
                world[worldn++] = toFloat(instr << 8);
            
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] WORLDD %f\n", cpc, toFloat(instr << 8));
                break;
            case CMD_VIEWN:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] VIEWN %u\n", cpc, instr & 0xF);

                viewn = instr & 0xF;
                break;
            case CMD_VIEWD:
                view[viewn++] = toFloat(instr << 8);
            
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] VIEWD %f\n", cpc, toFloat(instr << 8));
                break;
            case CMD_PROJN:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] PROJN %u\n", cpc, instr & 0xF);

                projn = instr & 0xF;
                break;
            case CMD_PROJD:
                proj[projn++] = toFloat(instr << 8);
            
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] PROJD %f\n", cpc, toFloat(instr << 8));
                break;
            case CMD_TGENN:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TGENN %u\n", cpc, instr & 0xF);

                tgenn = instr & 0xF;
                break;
            case CMD_TGEND:
                tgen[tgenn++] = toFloat(instr << 8);
            
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TGEND %f\n", cpc, toFloat(instr << 8));
                break;
            case CMD_SX:
                regs.s[0] = toFloat(instr << 8);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] SX %f\n", cpc, regs.s[0]);
                break;
            case CMD_SY:
                regs.s[1] = toFloat(instr << 8);
                
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] SY %f\n", cpc, regs.s[1]);
                break;
            case CMD_SZ:
                regs.s[2] = toFloat(instr << 8);
                
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] SZ %f\n", cpc, regs.s[2]);
                break;
            case CMD_TX:
                regs.t[0] = toFloat(instr << 8);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TX %f\n", cpc, regs.t[0]);
                break;
            case CMD_TY:
                regs.t[1] = toFloat(instr << 8);
                
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TY %f\n", cpc, regs.t[1]);
                break;
            case CMD_TZ:
                regs.t[2] = toFloat(instr << 8);
                
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TZ %f\n", cpc, regs.t[2]);
                break;
            case CMD_SU:
                regs.su = toFloat(instr << 8);
                
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] SU %f\n", cpc, regs.su);
                break;
            case CMD_SV:
                regs.sv = toFloat(instr << 8);
                
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] SV %f\n", cpc, regs.sv);
                break;
            case CMD_TU:
                regs.tu = toFloat(instr << 8);
                
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TU %f\n", cpc, regs.tu);
                break;
            case CMD_TV:
                regs.tv = toFloat(instr << 8);
                
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TV %f\n", cpc, regs.tv);
                break;
            case CMD_OFFSETX:
                regs.offsetx = ((f32)(u16)instr) / 16.0;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] OFFSETX %f\n", cpc, regs.offsetx);
                break;
            case CMD_OFFSETY:
                regs.offsety = ((f32)(u16)instr) / 16.0;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] OFFSETY %f\n", cpc, regs.offsety);
                break;
            case CMD_SHADE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] SHADE %u\n", cpc, instr & 1);

                regs.iip = instr & 1;
                break;
            case CMD_NREV:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] NREV %u\n", cpc, instr & 1);
                break;
            case CMD_MATERIAL:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] MATERIAL %u\n", cpc, instr & 7);
                break;
            case CMD_MEC:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] MEC 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_MAC:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] MAC 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_MDC:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] MDC 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_MSC:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] MSC 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_MAA:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] MAA 0x%02X\n", cpc, instr & 0xFF);
                break;
            case CMD_MK:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] MK 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_AC:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] AC 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_AA:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] AA 0x%02X\n", cpc, instr & 0xFF);
                break;
            case CMD_LMODE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] LMODE %u\n", cpc, instr & 1);
                break;
            case CMD_LTYPE0:
            case CMD_LTYPE1:
            case CMD_LTYPE2:
            case CMD_LTYPE3:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] LTYPE%d 0x%03X\n", cpc, cmd - CMD_LTYPE0, instr & 0x3FF);
                break;
            case CMD_LX0:
            case CMD_LY0:
            case CMD_LZ0:
            case CMD_LX1:
            case CMD_LY1:
            case CMD_LZ1:
            case CMD_LX2:
            case CMD_LY2:
            case CMD_LZ2:
            case CMD_LX3:
            case CMD_LY3:
            case CMD_LZ3:
                {
                    const auto coord = (cmd - CMD_LX0) % 3;
                    const auto idx = (cmd - CMD_LX0) / 3;

                    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] L%c%d %f\n", cpc, 'X' + coord, idx, toFloat(instr << 8));
                }
                break;
            case CMD_LDX0:
            case CMD_LDY0:
            case CMD_LDZ0:
            case CMD_LDX1:
            case CMD_LDY1:
            case CMD_LDZ1:
            case CMD_LDX2:
            case CMD_LDY2:
            case CMD_LDZ2:
            case CMD_LDX3:
            case CMD_LDY3:
            case CMD_LDZ3:
                {
                    const auto coord = (cmd - CMD_LDX0) % 3;
                    const auto idx = (cmd - CMD_LDX0) / 3;

                    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] LD%c%d %f\n", cpc, 'X' + coord, idx, toFloat(instr << 8));
                }
                break;
            case CMD_LKA0:
            case CMD_LKB0:
            case CMD_LKC0:
            case CMD_LKA1:
            case CMD_LKB1:
            case CMD_LKC1:
            case CMD_LKA2:
            case CMD_LKB2:
            case CMD_LKC2:
            case CMD_LKA3:
            case CMD_LKB3:
            case CMD_LKC3:
                {
                    const auto coord = (cmd - CMD_LKA0) % 3;
                    const auto idx = (cmd - CMD_LKA0) / 3;

                    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] LK%c%d %f\n", cpc, 'A' + coord, idx, toFloat(instr << 8));
                }
                break;
            case CMD_LKS0:
            case CMD_LKS1:
            case CMD_LKS2:
            case CMD_LKS3:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] LKS%d %f\n", cpc, cmd - CMD_LKS0, toFloat(instr << 8));
                break;
            case CMD_LKO0:
            case CMD_LKO1:
            case CMD_LKO2:
            case CMD_LKO3:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] LKO%d %f\n", cpc, cmd - CMD_LKO0, toFloat(instr << 8));
                break;
            case CMD_LAC0:
            case CMD_LDC0:
            case CMD_LSC0:
            case CMD_LAC1:
            case CMD_LDC1:
            case CMD_LSC1:
            case CMD_LAC2:
            case CMD_LDC2:
            case CMD_LSC2:
            case CMD_LAC3:
            case CMD_LDC3:
            case CMD_LSC3:
                {
                    constexpr char comp[3] = {'A', 'D', 'S'};

                    const auto compIdx = (cmd - CMD_LAC0) % 3;
                    const auto idx = (cmd - CMD_LAC0) / 3;

                    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] L%cC%d %f\n", cpc, comp[compIdx], idx, toFloat(instr << 8));
                }
                break;
            case CMD_CULL:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CULL %u\n", cpc, instr & 1);
                break;
            case CMD_FBP:
                regs.fbp = instr & 0xFFE000;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] FBP 0x%08X\n", cpc, regs.fbp);
                break;
            case CMD_FBW:
                regs.fbw  = instr & 0x7C0;
                regs.fbp |= (instr & 0xFF0000) << 8;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] FBW 0x%08X, %u\n", cpc, regs.fbp, regs.fbw);
                break;
            case CMD_ZBP:
                regs.zbp = instr & 0xFFE000;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] ZBP 0x%08X\n", cpc, regs.zbp);
                break;
            case CMD_ZBW:
                regs.zbw  = instr & 0x7C0;
                regs.zbp |= (instr & 0xFF0000) << 8;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] ZBW 0x%08X, %u\n", cpc, regs.zbp, regs.zbw);
                break;
            case CMD_TBP0:
            case CMD_TBP1:
            case CMD_TBP2:
            case CMD_TBP3:
            case CMD_TBP4:
            case CMD_TBP5:
            case CMD_TBP6:
            case CMD_TBP7:
                {
                    const auto idx = cmd - CMD_TBP0;

                    regs.tbp[idx] = instr & 0xFFFFF0;

                    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TBP%d 0x%08X\n", cpc, idx, regs.tbp[idx]);
                }
                break;
            case CMD_TBW0:
            case CMD_TBW1:
            case CMD_TBW2:
            case CMD_TBW3:
            case CMD_TBW4:
            case CMD_TBW5:
            case CMD_TBW6:
            case CMD_TBW7:
                {
                    const auto idx = cmd - CMD_TBW0;

                    regs.tbw[idx]  = instr & 0x7FF;
                    regs.tbp[idx] |= (instr & 0xFF0000) << 8;

                    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TBW%d 0x%08X, %u\n", cpc, idx, regs.tbp[idx], regs.tbw[idx]);
                }
                break;
            case CMD_CBP:
                regs.cbp = instr & 0xFFFFF0;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CBP 0x%08X\n", cpc, regs.cbp);
                break;
            case CMD_CBW:
                regs.cbp |= (instr & 0xFF0000) << 8;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CBW 0x%08X\n", cpc, regs.cbp);
                break;
            case CMD_XBP1:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] XBP1 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_XBW1:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] XBW1 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_XBP2:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] XBP2 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_XBW2:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] XBW2 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_TSIZE0:
            case CMD_TSIZE1:
            case CMD_TSIZE2:
            case CMD_TSIZE3:
            case CMD_TSIZE4:
            case CMD_TSIZE5:
            case CMD_TSIZE6:
            case CMD_TSIZE7:
                {
                    const auto idx = cmd - CMD_TSIZE0;

                    regs.tw[idx] = (f32)(1 << ((instr >> 0) & 0xF));
                    regs.th[idx] = (f32)(1 << ((instr >> 8) & 0xF));

                    if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TSIZE%u %f, %f\n", cpc, idx, regs.tw[idx], regs.th[idx]);
                }
                break;
            case CMD_TMAP:
                regs.tmn = (instr >> 0) & 3;
                regs.tmi = (instr >> 8) & 3;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TMAP %u, %u\n", cpc, regs.tmn, regs.tmi);
                break;
            case CMD_TSHADE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TSHADE 0x%03X\n", cpc, instr & 0x3FF);
                break;
            case CMD_TMODE:
                regs.hsm = instr & 1;
                regs.mc = instr & (1 << 8);
                regs.mxl = (instr >> 16) & 7;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TMODE %d, %d, %u\n", cpc, regs.hsm, regs.mc, regs.mxl);
                break;
            case CMD_TPF:
                regs.tpf = instr & 0xF;
                regs.ext = instr & (1 << 8);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TPF %u, %d\n", cpc, regs.tpf, regs.ext);
                break;
            case CMD_CLOAD:
                regs.np = instr & 0x3F;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CLOAD %u\n", cpc, regs.np);

                if (regs.cbp) loadCLUT();
                break;
            case CMD_CLUT:
                regs.cpf = (instr >>  0) & 3;
                regs.sft = (instr >>  2) & 0x1F;
                regs.msk = (instr >>  8) & 0xFF;
                regs.csa = (instr >> 16) & 0x1F;

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CLUT %u, %u, 0x%02X, 0x%X\n", cpc, regs.cpf, regs.sft, regs.msk, regs.csa);
                break;
            case CMD_TFILTER:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TFILTER 0x%03X\n", cpc, instr & 0x1FF);
                break;
            case CMD_TWRAP:
                regs.twms = instr & (1 << 0);
                regs.twmt = instr & (1 << 8);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TWRAP %d, %d\n", cpc, regs.twms, regs.twmt);
                break;
            case CMD_TLEVEL:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TLEVEL 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_TFUNC:
                regs.txf = instr & 7;
                regs.tcc = instr & (1 <<  0);
                regs.cd  = instr & (1 << 16);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TFUNC %u, %d, %d\n", cpc, regs.txf, regs.tcc, regs.cd);
                break;
            case CMD_TEC:
                regs.tec[0] = (f32)((instr >>  0) & 0xFF);
                regs.tec[1] = (f32)((instr >>  8) & 0xFF);
                regs.tec[2] = (f32)((instr >> 16) & 0xFF);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TEC %f, %f, %f\n", cpc, regs.tec[0], regs.tec[1], regs.tec[2]);
                break;
            case CMD_TFLUSH:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TFLUSH\n", cpc);
                break;
            case CMD_TSYNC:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TSYNC\n", cpc);
                break;
            case CMD_FOG1:
            case CMD_FOG2:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] FOG%d %f\n", cpc, 1 + cmd - CMD_FOG1, toFloat(instr << 8));
                break;
            case CMD_FC:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] FC 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_TSLOPE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] TSLOPE %f\n", cpc, toFloat(instr << 8));
                break;
            case CMD_FPF:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] FPF %u\n", cpc, instr & 3);

                regs.fpf = instr & 3;
                break;
            case CMD_CMODE:
                regs.set = instr & (1 <<  0);
                regs.cen = instr & (1 <<  8);
                regs.aen = instr & (1 <<  9);
                regs.zen = instr & (1 << 10);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CMODE %d, %d, %d, %d\n", cpc, regs.set, regs.cen, regs.aen, regs.zen);
                break;
            case CMD_SCISSOR1:
                regs.sx1 = (f32)((instr >>  0) & 0x3FF);
                regs.sy1 = (f32)((instr >> 10) & 0x3FF);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] SCISSOR1 %f, %f\n", cpc, regs.sx1, regs.sy1);
                break;
            case CMD_SCISSOR2:
                regs.sx2 = (f32)((instr >>  0) & 0x3FF);
                regs.sy2 = (f32)((instr >> 10) & 0x3FF);

                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] SCISSOR2 %f, %f\n", cpc, regs.sx2, regs.sy2);
                break;
            case CMD_MINZ:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] MINZ 0x%04X\n", cpc, instr & 0xFFFF);

                regs.minz = instr & 0xFFFF;
                break;
            case CMD_MAXZ:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] MAXZ 0x%04X\n", cpc, instr & 0xFFFF);

                regs.maxz = instr & 0xFFFF;
                break;
            case CMD_CTEST:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CTEST %u\n", cpc, instr & 3);
                break;
            case CMD_CREF:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CREF 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_CMSK:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] CMSK 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_ATEST:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] ATEST 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_STEST:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] STEST 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_SOP:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] SOP 0x%05X\n", cpc, instr & 0x7FFFF);
                break;
            case CMD_ZTEST:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] ZTEST %u\n", cpc, instr & 7);

                regs.ztf = instr & 7;
                break;
            case CMD_BLEND:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] BLEND 0x%04X\n", cpc, instr & 0x7FFF);
                break;
            case CMD_FIXA:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] FIXA 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_FIXB:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] FIXB 0x%06X\n", cpc, instr & 0xFFFFFF);
                break;
            case CMD_DITH1:
            case CMD_DITH2:
            case CMD_DITH3:
            case CMD_DITH4:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] DITH%u 0x%06X\n", cpc, 1 + cmd - CMD_DITH1, instr & 0xFFFFFF);
                break;
            case CMD_LOP:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] LOP 0x%X\n", cpc, instr & 0xF);
                break;
            case CMD_ZMSK:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] ZMSK %u\n", cpc, instr & 1);

                regs.zmsk = instr & 1;
                break;
            case CMD_PMSK1:
            case CMD_PMSK2:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] PMSK%u 0x%06X\n", cpc, 1 + cmd - CMD_PMSK1, instr & 0xFFFFFF);
                break;
            case CMD_XPOS1:
            case CMD_XPOS2:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] XPOS%u 0x%05X\n", cpc, 1 + cmd - CMD_XPOS1, instr & 0xFFFFF);
                break;
            case CMD_XSIZE:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] XSIZE 0x%05X\n", cpc, instr & 0xFFFFF);
                break;
            case 0xF0:
            case 0xF1:
            case 0xF2:
            case 0xF3:
            case 0xF4:
            case 0xF5:
            case 0xF6:
            case 0xF7:
            case 0xF8:
            case 0xF9:
            case 0xFF:
                if (ENABLE_DEBUG_PRINT) std::printf("[GE      ] [0x%08X] Command 0x%02X (0x%08X)\n", cpc, cmd, instr);
                break;
            default:
                std::printf("Unhandled GE command 0x%02X (0x%08X) @ 0x%08X\n", cmd, instr, cpc);

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

    update((u8 *)fb.data());
}

}
