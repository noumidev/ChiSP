/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#pragma once

#include "common/types.hpp"

namespace hw::allegrex {

constexpr int NUM_REGS = 32;
constexpr int REG_MASK = NUM_REGS - 1;

template<class Derived>
class Coprocessor {
    bool cpcond;

public:
    void reset() {
        (static_cast<Derived *>(this))->resetImpl();
    }

    u32 get(int idx) {
        return (static_cast<Derived *>(this))->getImpl(idx);
    }

    u32 getControl(int idx) {
        return (static_cast<Derived *>(this))->getControlImpl(idx);
    }

    void set(int idx, const u32 data) {
        (static_cast<Derived *>(this))->setImpl(idx, data);
    }

    void setControl(int idx, u32 data) {
        (static_cast<Derived *>(this))->setControlImpl(idx, data);
    }

    bool getCPCOND() {
        return cpcond;
    }
};

}
