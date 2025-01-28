#include <cstdlib>
#include <filesystem>

enum class buildsystem {
    cmake,
    premake,
};

struct lib {
    const char* name;
    buildsystem buildsystem;
    // .lib file path relative to lib build dir
    std::filesystem::path lib_path;
    std::filesystem::path shared_lib_path;

    std::filesystem::path lib_out_dir;
    std::vector<std::string> lib_files;
};

const char* lib_path = "../../lib";

void build_libs(lib* libs, int lib_count) {
    for (int i = 0; i < lib_count; i++) {
        auto& lib_def = libs[i];
        printf("process %s\n", lib_def.name);
        if (std::filesystem::exists(std::filesystem::path(lib_def.name) / lib_def.lib_path)) {
            printf("skip %s\n", lib_def.name);
            continue;
        }

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


int main() {
    lib libs[] = {
        lib {
            .name = "box2d",
            .lib_path = "src/box2dd.lib",
        },
        lib {
            .name = "glm",
            .lib_path = "glm/glm.lib",
        },
        lib {
            .name = "SDL",
            .lib_path = "SDL3.lib",
            .shared_lib_path = "SDL3.dll"
        },
        lib {
            .name = "EASTL",
            .lib_path = "EASTL.lib",
        },
        lib {
            .name = "tracy",
            .lib_path = "TracyClient.lib",
        },
        lib {
            .name = "yojimbo",
            .buildsystem = buildsystem::premake,
            .lib_out_dir = "../lib/yojimbo/bin/Debug",
            .lib_files = {
                "sodium-builtin.lib",
                "tlsf.lib",
                "netcode.lib",
                "reliable.lib",
                "yojimbo.lib",
            }

        },
    };

    int lib_count = sizeof(libs) / sizeof(lib);

    // build_libs(libs, lib_count);

    std::string lib_files = "";
    for (int i = 0; i < lib_count; i++) {

        auto& lib = libs[i];
        if (!lib.lib_path.empty()) {
            lib_files += (std::filesystem::path(lib.name) / lib.lib_path).lexically_normal().string() + " ";
        }

        if (!lib.lib_out_dir.empty()) {
            for (auto& lib_file : lib.lib_files) {
                lib_files += (lib.lib_out_dir / lib_file).lexically_normal().string() + " ";
            }
        }

        if (!lib.shared_lib_path.empty() && !std::filesystem::exists(lib.shared_lib_path.filename()) ) {
            std::filesystem::copy(std::filesystem::path(lib.name) / lib.shared_lib_path, lib.shared_lib_path.filename());
        }
    }

    std::string command = std::format("link *.obj {} msvcrtd.lib /OUT:client.exe", lib_files);
    printf("%s\n", command.data());
    system(command.data());
}
