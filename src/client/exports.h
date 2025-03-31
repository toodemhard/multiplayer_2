typedef struct Signals Signals;
struct Signals {
    bool quit;
    bool reload;
};

#ifdef HOT_RELOAD
    #if defined(OS_WINDOWS) && defined(COMPILER_MSVC)
        #define DLL_EXPORT __declspec
        #ifdef DLL
            #define DLL_API __declspec(dllexport)
        #else
            #define DLL_API __declspec(dllimport)
        #endif
    #else
        #error DLL_API not defined for this toolchain
    #endif
#else 
    #define DLL_API 
    #define DLL_EXPORT
#endif

const u64 memory_size = megabytes(64);

// #define UPDATE(name) Signals name(void* memory)
typedef Signals (*UpdateFunction)(void* memory);

DLL_API Signals update(void* memory);
