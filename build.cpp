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
    std::filesystem::path rel_include_path;
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

        if (lib_def.buildsystem == buildsystem::cmake) {
            std::string commands;
            commands += std::format("mkdir {0} & ", libs[i].name);
            commands += std::format("cd {} &&", libs[i].name);
            commands += std::format("cmake -S {}/{} -B . &&", lib_path, libs[i].name);
            commands += std::format("cmake --build .");

            system(commands.data());
        }
    }
}


int main() {
    lib libs[] = {
        lib {
            .name = "box2d",
            .rel_include_path = "include",
            .lib_path = "src/box2dd.lib",
        },
        lib {
            .name = "glm",
            .rel_include_path = ".",
            .lib_path = "glm/glm.lib",
        },
        lib {
            .name = "SDL",
            .rel_include_path = "include",
            .lib_path = "SDL3.lib",
            .shared_lib_path = "SDL3.dll"
        },
        lib {
            .name = "EASTL",
            .rel_include_path = "include",
            .lib_path = "EASTL.lib",
        },
        lib {
            .name = "tracy",
            .rel_include_path = "public",
            .lib_path = "TracyClient.lib",
        },
        lib {
            .name = "yojimbo",
            .buildsystem = buildsystem::premake,
            .rel_include_path = "include",
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

    std::vector<std::filesystem::path> include_dirs = {
        "src/",
        "src/client",
        "src/common",
        "lib/stb",
        "lib/imgui",
        "lib/yojimbo/serialize",
        "b2/EASTL/_deps/eabase-src/include/Common",
    };

    // build_libs(libs, lib_count);

    const char* flags = R"(/std:c++20 /EHsc /Zi /MP /Yu"../pch.h" /Fp"pch.pch")";


    std::string includes = "";
    for (int i = 0; i < lib_count; i++) {
        auto& lib = libs[i];
        includes += std::format("/I {} ", (std::filesystem::path("../lib") / lib.name / lib.rel_include_path).lexically_normal().string());
    }

    for (auto& include_dir : include_dirs) {
        includes += std::format("/I {} ", (std::filesystem::path("..") / include_dir).lexically_normal().string());

    }

    // printf("%s\n", includes.data());
    


    std::string commands = "";
    commands += std::format("cl {} {} /c ../src/client/*.cpp /c ../src/common/*.cpp && ", flags, includes);
    // commands += std::format(R"(cl /std:c++20 /EHsc /Zi /MP {} /c ../lib/imgui/*.cpp && )", includes);

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

    commands += std::format("link /INCREMENTAL *.obj {} msvcrtd.lib /OUT:client.exe", lib_files);
    printf("%s\n", commands.data());
    system(commands.data());
}
