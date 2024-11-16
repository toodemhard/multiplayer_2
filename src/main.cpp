#include <SDL3/SDL.h>
#include "app.h"
#include "font.h"

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

    // Font::save_font_atlas_image();
    // return 0;
    app::run();

    return 0;
}
