/*
    ChiSP is an LLE PlayStation Portable emulator.
    Copyright (C) 2023-2024  noumidev
*/

#include "sys/scheduler.hpp"

#include <cstdlib>
#include <queue>
#include <vector>

#include <plog/Log.h>

namespace sys::scheduler {

constexpr i64 MAX_RUN_CYCLES = 256;

struct SchedulerEvent {
    u64 id;
    int param;

    i64 timestamp;

    bool operator>(const SchedulerEvent &other) const {
        return timestamp > other.timestamp;
    }
};

// Event queue
std::priority_queue<SchedulerEvent, std::vector<SchedulerEvent>, std::greater<SchedulerEvent>> events;

std::vector<std::function<void(int)>> registeredFuncs;

i64 globalTimestamp = 0;

void init() {
    reset();
}

void deinit() {}

void reset() {
    // Clear event queue
    std::priority_queue<SchedulerEvent, std::vector<SchedulerEvent>, std::greater<SchedulerEvent>> empty;

    events.swap(empty);
}

u64 registerEvent(const std::function<void(int)> &func) {
    static u64 idPool;

    registeredFuncs.push_back(func);

    return idPool++;
}

void addEvent(u64 id, int param, const i64 eventCycles) {
    if (eventCycles <= 0) {
        PLOG_FATAL << "Invalid event cycles";

        exit(0);
    }

    events.emplace(SchedulerEvent{id, param, globalTimestamp + eventCycles});
}

i64 getRunCycles() {
    return MAX_RUN_CYCLES;
}

void run(const i64 runCycles) {
    const auto newTimestamp = globalTimestamp + runCycles;

    while (events.top().timestamp <= newTimestamp) {
        globalTimestamp = events.top().timestamp;

        const auto id = events.top().id;
        const auto param = events.top().param;

        events.pop();

        registeredFuncs[id](param);
    }

    globalTimestamp = newTimestamp;
}

}
