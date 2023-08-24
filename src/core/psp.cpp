/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "psp.hpp"

#include <cstdio>

#include "ata.hpp"
#include "display.hpp"
#include "dmacplus.hpp"
#include "ge.hpp"
#include "hpremote.hpp"
#include "intc.hpp"
#include "i2c.hpp"
#include "memory.hpp"
#include "nand.hpp"
#include "scheduler.hpp"
#include "syscon.hpp"
#include "systime.hpp"
#include "allegrex/allegrex.hpp"
#include "allegrex/interpreter.hpp"
#include "crypto/kirk.hpp"
#include "crypto/spock.hpp"

#include <SDL2/SDL.h>

#undef main

namespace psp {

using namespace allegrex;

using ge::SCR_HEIGHT;
using ge::SCR_WIDTH;

struct Screen {
    SDL_Renderer *renderer;
    SDL_Window  *window;
    SDL_Texture *texture;
};

Screen screen;
SDL_Event event;

auto isRunning = true;

Allegrex cpu, me;

void sdlInit() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    SDL_CreateWindowAndRenderer(SCR_WIDTH, SCR_HEIGHT, 0, &screen.window, &screen.renderer);
    SDL_SetWindowSize(screen.window, 2 * SCR_WIDTH, 2 * SCR_HEIGHT);
    SDL_RenderSetLogicalSize(screen.renderer, 2 * SCR_WIDTH, 2 * SCR_HEIGHT);
    SDL_SetWindowResizable(screen.window, SDL_FALSE);
    SDL_SetWindowTitle(screen.window, "ChiSP");

    screen.texture = SDL_CreateTexture(screen.renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, SCR_WIDTH, SCR_HEIGHT);
}

void init(const char *bootPath, const char *nandPath) {
    sdlInit();

    memory::init(bootPath);
    nand::init(nandPath);

    cpu.init(Type::Allegrex);
    me.init(Type::MediaEngine);

    // MediaEngine is booted later on
    me.isHalted = true;

    display::init();
    dmacplus::init();
    ge::init();
    hpremote::init();
    i2c::init();
    kirk::init();
    spock::init();
    syscon::init();
    systime::init();
    ata::init();

    std::puts("[PSP     ] OK");
}

void run() {
    while (isRunning) {
        const auto runCycles = scheduler::getRunCycles();

        interpreter::run(&cpu, runCycles);
        interpreter::run(&me , runCycles >> 1);

        scheduler::run(runCycles);
    }
}

void update(u8 *fb) {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            default:
                break;
        }
    }

    SDL_UpdateTexture(screen.texture, nullptr, fb, 4 * SCR_WIDTH);
    SDL_RenderCopy(screen.renderer, screen.texture, nullptr, nullptr);
    SDL_RenderPresent(screen.renderer);
}

void setIRQPending(bool irqPending) {
    cpu.setIRQPending(irqPending);
}

void meSetIRQPending(bool irqPending) {
    me.setIRQPending(irqPending);
}

void resetCPU() {
    cpu.reset();

    memory::unmapBootROM();
}

void resetME() {
    me.reset();
}

void postME() {
    intc::meSendIRQ(intc::InterruptSource::ME_VME);
}

}
