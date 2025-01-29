#include "../pch.h"
#include "app.h"
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
