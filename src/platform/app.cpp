#include "../pch.h"

#include "client/exports.h"
#include <cstdlib>

void run(){
    auto object = SDL_LoadObject("./game.dll");

    auto update = (update_func*)SDL_LoadFunction(object, "update");
    auto init = (init_func*)SDL_LoadFunction(object, "init");

    void* memory = malloc(1024 * 1024);


    init(memory);

    while (1) {
        auto signals = update(memory);

        if (signals.quit) {
            return;
        }

    }
}
