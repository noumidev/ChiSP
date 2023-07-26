/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "scheduler.hpp"

#include <cassert>
#include <queue>
#include <vector>

namespace psp::scheduler {

constexpr i64 MAX_RUN_CYCLES = 512;

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
    if (events.empty()) {
        globalTimestamp += MAX_RUN_CYCLES;

        return MAX_RUN_CYCLES;
    }

    // Get next event, run cycles; update timestamp for new events
    const auto nextEvent = events.top(); events.pop();
    const auto runCycles = nextEvent.timestamp - globalTimestamp;

    globalTimestamp = nextEvent.timestamp;

    registeredFuncs[nextEvent.id](nextEvent.param);

    while (true) { // Run all events with same timestamp
        if (events.empty()) break;

        const auto nextNextEvent = events.top();

        if (nextNextEvent.timestamp != nextEvent.timestamp) break;
        events.pop();

        registeredFuncs[nextNextEvent.id](nextNextEvent.param);
    }

    return runCycles;
}

}
