typedef struct DLL {
    SDL_SharedObject* object;
    
    // init_func* init;
    UpdateFunction update;
} DLL;

void load_dll(DLL* dll) {
    if (dll->object != NULL) {
        SDL_UnloadObject(dll->object);
    }
    
    if (!SDL_CopyFile("./game.dll", "./temp_game.dll")) {
        printf("%s\n", SDL_GetError());
    };

    dll->object = SDL_LoadObject("./temp_game.dll");
    if (!dll->object) {
        fprintf(stderr, "%s\n", SDL_GetError());
    }

    dll->update = (UpdateFunction)SDL_LoadFunction(dll->object, "update");
    // dll->init = (init_func*)SDL_LoadFunction(dll->object, "init");
}


void ErrorExit() 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    if (FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL) == 0) {
        // MessageBox(NULL, TEXT("FormatMessage failed"), TEXT("Error"), MB_OK);
        // ExitProcess(dw);
        printf("%s", lpMsgBuf);
    }


    // MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK);
    //
    // LocalFree(lpMsgBuf);
}

void run(){
    // TracyNoop;

    ASSERT(SDL_LoadObject("box2dd.dll") != NULL);
    ASSERT(SDL_LoadObject("enet.dll") != NULL);

    // auto object = SDL_LoadObject("./game.dll");
    DLL dll = {0};
    load_dll(&dll);


    void* memory = malloc(memory_size);
    memset(memory, 0, memory_size);

    OS_Handle dll_file = os_file_open(string8_literal("./game.dll"));
    u64 last_write;
    if (!os_file_write_time(dll_file, &last_write)) {
        ASSERT(false);
    }
    os_file_close(dll_file);

    bool reload = false;

    while (1) {
        u64 current_write;
        OS_Handle dll_file = os_file_open(string8_literal("./game.dll"));
        if (!os_file_write_time(dll_file, &current_write)) {
            // ErrorExit();
            // printf("%s", GetLastError());
            // ASSERT(false);
        }
        os_file_close(dll_file);

        // auto other_write = std::filesystem::last_write_time("./game.dll");

        if (last_write != current_write || reload) {
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

                    os_sleep_milliseconds(50);
                }
            }

            OS_Handle dll_file = os_file_open(string8_literal("./game.dll"));
            if (!os_file_write_time(dll_file, &current_write)) {
                ASSERT(false);
            }
            os_file_close(dll_file);

            fprintf(stderr, "reloaded: %llu\n", current_write);
            // ("{}\n", last_write);
            last_write = current_write;


            load_dll(&dll);
        }

        Signals signals = dll.update(memory);
        reload = signals.reload;

        if (signals.quit) {
            return;
        }

    }
}
