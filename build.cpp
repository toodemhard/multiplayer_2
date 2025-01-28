#include <cstdlib>
#include <filesystem>

enum class buildsystem {
    cmake,
    premake,
};

struct lib {
    buildsystem buildsystem;
    const char* name;
    std::filesystem::path path;
};

int main() {
    const char* lib_path = "../../lib";
    lib libs[] = {
        lib {
            .name = "box2d"
        },
        lib {
            .name = "glm"
        },
        lib {
            .name = "SDL"
        },
        lib {
            .name = "EASTL"
        },
        lib {
            .name = "tracy"
        },
    };


    for (int i = 0; i < sizeof(libs) / sizeof(lib); i++) {
        std::string commands;
        commands += std::format("mkdir {0} & ", libs[i].name);
        commands += std::format("cd {} &&", libs[i].name);
        commands += std::format("cmake -S {}/{} -B . &&", lib_path, libs[i].name);
        commands += std::format("cmake --build .");
        // commands += std::format("popd").data());
        // commands.std::format("cd");

        // std::string command;
        // for (int i = 0; i < commands.size(); i++) {
        //     command += commands[i];
        //
        //     if(i != commands.size() - 1) {
        //         command += " && ";
        //     }
        // }

        system(commands.data());
    }
}
