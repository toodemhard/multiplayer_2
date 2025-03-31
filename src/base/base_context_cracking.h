#ifdef _WIN32
    #define OS_WINDOWS
#else
    #error missing OS detection
#endif

#if defined(_MSC_VER)
    #define COMPILER_MSVC 1
#elif defined(__clang__)
    #define COMPILER_CLANG 1
#else
    #error missing compiler detection
#endif
