#include "app.h"
#include "client.h"
#include "font.h"
#include "serialize.h"
#include "yojimbo_address.h"
#include "yojimbo_allocator.h"
#include "yojimbo_config.h"
#include "yojimbo_constants.h"
#include "yojimbo_message.h"
#include "yojimbo_platform.h"
#include "yojimbo_serialize.h"
#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <format>
#include <iostream>
#include <stdexcept>
#include <yojimbo.h>
#include <EASTL/any.h>

// std::vector<char> read_file(const char* file_name) {
//     std::string path = "data/" + std::string(file_name);
//     std::ifstream file{path, std::ios::binary | std::ios::ate};
//     if (!file) {
//         exit(1);
//     }
//
//     auto size = file.tellg();
//     std::vector<char> data(size, '\0');
//     file.seekg(0);
//     file.read(data.data(), size);
//
//     return data;
// }



int main() {
    app::run();

    return 0;
}
