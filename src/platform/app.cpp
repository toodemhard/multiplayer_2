#include "../pch.h"

#include "client/exports.h"

struct DLL {
    SDL_SharedObject* object;
    
    init_func* init;
    update_func* update;
};

void load_dll(DLL* dll) {
    if (dll->object != NULL) {
        SDL_UnloadObject(dll->object);
    }
    
    if (!SDL_CopyFile("./game.dll", "./temp_game.dll")) {
        printf("%s\n", SDL_GetError());
    };

    dll->object = SDL_LoadObject("./temp_game.dll");

    dll->update = (update_func*)SDL_LoadFunction(dll->object, "update");
    dll->init = (init_func*)SDL_LoadFunction(dll->object, "init");
}

void run(){

    // auto object = SDL_LoadObject("./game.dll");
    DLL dll{};
    load_dll(&dll);


    void* memory = malloc(1024 * 1024);

    auto pwd = std::filesystem::current_path();

    // KYS kys{};
    // kys.allocFcn = alloc;
    // kys.freeFcn = free;

    // char buffer[1024];
    // DWORD bytes_returned
    // ReadDirectoryChanges(HANDLE hDirectory, LPVOID lpBuffer, DWORD nBufferLength, BOOL bWatchSubtree, DWORD dwNotifyFilter, &bytes_returned, LPOVERLAPPED lpOverlapped, int lpCompletionRoutine)
    //
    // )


    dll.init(memory);

    auto last_write = std::filesystem::last_write_time("./game.dll");
    // std::cout << std::format("{}\n", x);


    while (1) {
        auto current_write = std::filesystem::last_write_time("./game.dll");

        if (last_write != current_write) {
            std::cout << std::format("{}\n", last_write);
            last_write = current_write;

            load_dll(&dll);

        }

        auto signals = dll.update(memory);

        if (signals.quit) {
            return;
        }

    }
}
