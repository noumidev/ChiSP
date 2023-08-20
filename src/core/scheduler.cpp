/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "scheduler.hpp"

#include <cassert>
#include <queue>
#include <vector>

namespace psp::scheduler {

constexpr i64 MAX_RUN_CYCLES = 64;

// Scheduler event
struct Event {
    u64 id;
    int param;

    i64 timestamp;

    bool operator>(const Event &other) const {
        return timestamp > other.timestamp;
    }
};

// Event queue
std::priority_queue<Event, std::vector<Event>, std::greater<Event>> events;

std::vector<std::function<void(int)>> registeredFuncs;

i64 globalTimestamp = 0;

// Registers an event, returns event ID
u64 registerEvent(std::function<void(int)> func) {
    static u64 idPool;

    registeredFuncs.push_back(func);

    return idPool++;
}

// Adds a scheduler event
void addEvent(u64 id, int param, i64 cyclesUntilEvent) {
    assert(cyclesUntilEvent > 0);

    events.emplace(Event{id, param, globalTimestamp + cyclesUntilEvent});
}

i64 getRunCycles() {
    return MAX_RUN_CYCLES;
}

void run(i64 runCycles) {
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
