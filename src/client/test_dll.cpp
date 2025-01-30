#include "../pch.h"

#include "test_dll.h"

extern "C" INIT(init) {
    printf("called from dll\n");
}
