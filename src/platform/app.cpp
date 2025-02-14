#include "../pch.h"

#include "common/allocator.h"
#include "client/exports.h"

#include <windows.h>

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
    if (!dll->object) {
        std::cout << SDL_GetError();
    }

    dll->update = (update_func*)SDL_LoadFunction(dll->object, "update");
    dll->init = (init_func*)SDL_LoadFunction(dll->object, "init");
}


void run(){
    {
        b2WorldDef def = b2DefaultWorldDef();
        auto world_id = b2CreateWorld(&def);
        b2DestroyWorld(world_id);
    }



    // auto object = SDL_LoadObject("./game.dll");
    DLL dll{};
    load_dll(&dll);


    void* memory = malloc(megabytes(10));

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
        std::filesystem::file_time_type current_write;
        try {
            current_write = std::filesystem::last_write_time("./game.dll");
        } catch(std::runtime_error& e) {
            current_write = last_write;
        }

        // auto other_write = std::filesystem::last_write_time("./game.dll");

        using namespace std::chrono_literals;
        if (last_write != current_write) {
            bool file_locked = true;
            while (file_locked) {
                HANDLE file = CreateFileA(
                    "game.dll",
                    GENERIC_READ,
                    0,              // Exclusive access
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );

                if (file != INVALID_HANDLE_VALUE) {
                    file_locked = false;
                    CloseHandle(file);
                } else {
                    printf("file locked\n");
                    DWORD error = GetLastError();
                    printf("Error code: %lu\n", error);

                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(50ms);
                }
            }

            current_write = std::filesystem::last_write_time("./game.dll");
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
