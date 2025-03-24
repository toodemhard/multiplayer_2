struct Signals {
    bool quit;
    bool reload;
};

// #define INIT(name) void name(void* memory)
// typedef INIT(init_func);
// extern "C" INIT(init);

const u64 memory_size = megabytes(64);

#define UPDATE(name) Signals name(void* memory)
typedef UPDATE(update_func);
extern "C" UPDATE(update);
