/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#pragma once

#include <array>
#include <cstdlib>
#include <ios>
#include <type_traits>

#include <plog/Log.h>

#include "common/types.hpp"

#include "sys/memory.hpp"

namespace hw::allegrex {

constexpr int GPR_NUM = 34;

constexpr u32 BOOT_EXCEPTION_VECTOR_BASE = 0xBFC00000;

enum class CPUType {
    Allegrex, MediaEngine,
};

enum class CPUStatus {
    Halted,
    Running,
};

template<typename T>
inline bool isAligned(u32 addr) requires std::is_unsigned_v<T> {
    if constexpr (std::is_same<T, u8>()) {
        return true;
    }

    return (addr & (sizeof(T) - 1)) == 0;
}

template<CPUType type>
class CPU {
    std::array<u32, GPR_NUM> regs;

    // Program counters
    u32 pc, npc;

    // Delay slot helper
    bool inDelaySlot[2];

    // Load Linked bit
    bool loadLinked;

    CPUStatus status;

    // Read/write handlers
    u8 (*read8)(u32);
    u16 (*read16)(u32);
    u32 (*read32)(u32);

    void (*write8)(u32, const u8);
    void (*write16)(u32, const u16);
    void (*write32)(u32, const u32);

public:
    // Memory access handlers
    template<typename T>
    T read(u32 vaddr) requires std::is_unsigned_v<T> {
        if constexpr (std::is_same<T, u8>()) {
            return read8(vaddr);
        } else if (std::is_same<T, u16>()) {
            return read16(vaddr);
        } else if (std::is_same<T, u32>()) {
            return read32(vaddr);
        }
    }

    template<typename T>
    void write(u32 vaddr, const T data) requires std::is_unsigned_v<T> {
        if constexpr (std::is_same<T, u8>()) {
            return write8(vaddr, data);
        } else if (std::is_same<T, u16>()) {
            return write16(vaddr, data);
        } else if (std::is_same<T, u32>()) {
            return write32(vaddr, data);
        }
    }

    void init() {
        reset();

        // Set memory access handlers
        if constexpr (type == CPUType::Allegrex) {
            read8 = &sys::memory::read<u8>;
            read16 = &sys::memory::read<u16>;
            read32 = &sys::memory::read<u32>;

            write8 = &sys::memory::write<u8>;
            write16 = &sys::memory::write<u16>;
            write32 = &sys::memory::write<u32>;
        } else {
            PLOG_WARNING << "Unhandled MediaEngine init";
        }
    }

    void reset() {
        PLOG_INFO << "Reset";

        // Halt MediaEngine
        if constexpr (type == CPUType::Allegrex) {
            status = CPUStatus::Running;
        } else {
            status = CPUStatus::Halted;
        }

        // Clear registers
        regs.fill(0);

        // Reset delay slot helpers
        inDelaySlot[0] = inDelaySlot[1] = false;

        setPC(BOOT_EXCEPTION_VECTOR_BASE);
    }

    u32 get(int idx) {
        return regs[idx];
    }

    void set(int idx, u32 data) {
        regs[idx] = data;

        // R0 is hardwired to 0
        regs[0] = 0;
    }

    u32 getPC() {
        return pc;
    }

    // Sets PC and NPC
    void setPC(u32 addr) {
        if (addr == 0) {
            PLOG_FATAL << "Jump to NULL";

            exit(0);
        }

        if (!isAligned<u32>(addr)) {
            PLOG_FATAL << "Jump target is not aligned (addr = " << std::hex << addr << ")";

            exit(0);
        }

        pc = addr;
        npc = addr + 4;
    }

    // Sets NPC on branches
    void setBranchPC(u32 addr) {
        if (addr == 0) {
            PLOG_FATAL << "Jump to NULL";

            exit(0);
        }

        if (!isAligned<u32>(addr)) {
            PLOG_FATAL << "Jump target is not aligned (addr = " << std::hex << addr << ")";

            exit(0);
        }

        npc = addr;
    }

    void advanceDelaySlot() {
        inDelaySlot[0] = inDelaySlot[1];
        inDelaySlot[1] = false;
    }

    void advancePC() {
        pc = npc;
        npc += 4;
    }

    template<bool isLikely>
    void doBranch(u32 target, bool cond, u32 linkReg) {
        if (isDelaySlot()) {
            PLOG_FATAL << "Branch instruction in delay slot";

            exit(0);
        }

        set(linkReg, npc);

        inDelaySlot[1] = true;

        if (cond) {
            setBranchPC(target);
        } else {
            if constexpr (isLikely) {
                // Skip delay slot
                setPC(npc);

                inDelaySlot[1] = false;
            }
        }
    }

    bool isDelaySlot() {
        return inDelaySlot[0];
    }

    bool isHalted() {
        return status == CPUStatus::Halted;
    }
};

}
