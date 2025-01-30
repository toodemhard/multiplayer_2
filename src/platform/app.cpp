#include "../pch.h"

#include "client/test_dll.h"


void run(){
    auto object = SDL_LoadObject("./game.dll");

    auto update = (update_func*)SDL_LoadFunction(object, "update");
    auto init = (init_func*)SDL_LoadFunction(object, "init");

    DLL_State state = {};

    init(&state);
}
