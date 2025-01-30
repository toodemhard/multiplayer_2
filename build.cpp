#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unordered_map>

enum class buildsystem {
    cmake,
    premake,
    manual,
};

enum class files_type {
    single,
    glob,
};

struct source_file {
    files_type type;
    std::filesystem::path glob_dir;
    std::filesystem::path file_path;
};

enum class target_type {
    executable,
    static_lib,
    shared_lib,
};

struct target {
    target_type type;

    const char* name;
    std::vector<source_file> files;
    std::vector<std::string> libs;
    std::vector<std::filesystem::path> include_dirs;
};

struct lib {
    const char* name;
    buildsystem buildsystem;
    // .lib file path relative to lib build dir
    std::vector<std::filesystem::path> rel_include_paths;
    std::filesystem::path lib_path;
    std::filesystem::path shared_lib_path;

    std::filesystem::path lib_out_dir;
    std::vector<std::string> lib_files;

    // manual
    source_file source_files;
};

const char* lib_path = "../../lib";

const char* compile_flags = R"(/std:c++20 /EHsc /Zi /MP)";
const char* pch_flags = R"(/Yu"../pch.h" /Fp"pch.pch")";


std::filesystem::path project_root;
std::filesystem::path compiler_path;

void build_libs(lib* libs, int lib_count) {
    for (int i = 0; i < lib_count; i++) {
        auto& lib = libs[i];
        printf("process %s\n", lib.name);
        if (std::filesystem::exists(std::filesystem::path(lib.name) / lib.lib_path)) {
            printf("skip %s\n", lib.name);
            continue;
        }

        if (lib.buildsystem == buildsystem::cmake) {
            std::string commands;
            commands += std::format("mkdir {0} & ", lib.name);
            commands += std::format("cd {} &&", lib.name);
            commands += std::format("cmake -S {}/{} -B . &&", lib_path, lib.name);
            commands += std::format("cmake --build .");

            system(commands.data());
        }

        if (lib.buildsystem == buildsystem::manual) {
            std::string commands;
            commands += std::format("mkdir {0} & ", lib.name);
            commands += std::format("cd {} &&", lib.name);
            commands += std::format(R"(cl  {} /c ../../lib/imgui/*.cpp && )", (project_root / lib.name).string() ).data();
            commands += std::format("lib /out:imgui.lib *.obj");

            system(commands.data());
        }
    }
}

void glob_files(std::vector<std::filesystem::path>* files, std::filesystem::path path) {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.path().extension() == ".cpp") {
            files->push_back(entry.path().lexically_normal());
        }
    }
}

std::string escape_json(const std::string& path) {
    std::string result;
    for (char ch : path) {
        if (ch == '\\') result += "\\\\";
        else result += ch;
    }
    return result;
}

std::string escape_quotes(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '\"') {
            result += "\\\"";
        } else {
            result += c;
        }
    }
    return result;
}

void generate_compile_commands(std::filesystem::path build_dir, std::string flags, std::string includes, const std::vector<std::filesystem::path>& src_files) {
    includes = escape_json(includes);
    std::ofstream file("compile_commands.json");


    file << "[";
    for (int i = 0; i < src_files.size(); i++) {
        auto& src_file = src_files[i];
        std::string command = std::format("{} {} {} {}", compiler_path.string(), flags, includes, escape_json(src_file.string()));

        auto idk =  std::format(R"({{
    "directory": "{}",
    "command": "{}",
    "file": "{}",
    "output": "{}.obj"
}})", escape_json(build_dir.string()), command, escape_json(src_file.string()), escape_json((build_dir / src_file.stem()).string()));
        file << idk;

        if (i != src_files.size() - 1) {
            file << ',';
        }

        file << '\n';
    }

    file << "]";
}

std::string command_output(const char* command) {
    std::string result;
    char buffer[128];

    // Run command and open a pipe
    FILE* pipe = _popen(command, "r");  
    if (!pipe) return "ERROR";  

    // Read output
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    _pclose(pipe);  
    result.pop_back();
    return result;
}
// _popen(const char *Command, const char *Mode)

void build_target(target target, lib libs[], int lib_count) {
    std::unordered_map<std::string, int> lib_index;

    for (int i = 0; i < lib_count; i++) {
        lib_index[libs[i].name] = i;
    }
    std::string includes = "";
    for (auto& lib_name : target.libs) {
        auto& lib = libs[lib_index[lib_name]];
        for (auto& include_path : lib.rel_include_paths) {
            includes += std::format("/I {} ", (project_root / "lib" / lib.name / include_path).lexically_normal().string());
        }
    }

    for (auto& include_dir : target.include_dirs) {
        includes += std::format("/I {} ", (project_root / include_dir).lexically_normal().string());
    }

    std::string pch_header = escape_json((project_root / "src" / "pch.h").lexically_normal().string());
    std::string pch_file = escape_json((project_root / "b2" / "pch.pch").lexically_normal().string());
    auto idk = std::format("/Yu{} /Fp{} /FI {}", pch_header, pch_file, pch_header);
    // generate_compile_commands(project_root / "b2", std::format("{} {}", compile_flags, idk), includes, src_files);

    std::string out_dir = std::format("targets\\{}\\", target.name);

    std::string commands = "";
    commands += std::format("mkdir {} &", out_dir);


    std::string source_files = "";

    for (auto& src : target.files) {
        if (src.type == files_type::glob)  {
            source_files += std::format("/c {} ", (project_root / src.glob_dir / "*.cpp").string());
        }
    }
    commands += std::format("cl {} {} {} {} /Fo{} && ", compile_flags, pch_flags, includes, source_files, out_dir);

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

    const char* linker_flags = "/DEBUG /INCREMENTAL";
    switch (target.type) {
    case target_type::executable: {
        commands += std::format("link {} {}/*.obj pch.obj {} msvcrtd.lib /OUT:{}.exe", linker_flags, out_dir, lib_files, target.name);
    } break;
    case target_type::shared_lib: {
        commands += std::format("link {} /DLL {}/*.obj pch.obj {} msvcrtd.lib /OUT:{}.dll /EXPORT:init", linker_flags, out_dir, lib_files, target.name);
    } break;
    }

    printf("%s\n", commands.data());
    system(commands.data());
}

int main(int argc, char* argv[]) {

    project_root = (std::filesystem::current_path() / "..").lexically_normal();

    compiler_path = std::format("\\\"{}\\\"", escape_json(command_output("which clang-cl")));

    std::vector<std::filesystem::path> src_files;
    glob_files(&src_files, project_root / "src");
    glob_files(&src_files, project_root / "src/common");
    glob_files(&src_files, project_root / "src/client");

    lib libs[] = {
        lib {
            .name = "box2d",
            .rel_include_paths = {"include"},
            .lib_path = "src/box2dd.lib",
        },
        lib {
            .name = "glm",
            .rel_include_paths = {"."},
            .lib_path = "glm/glm.lib",
        },
        lib {
            .name = "SDL",
            .rel_include_paths = {"include"},
            .lib_path = "SDL3.lib",
            .shared_lib_path = "SDL3.dll"
        },
        // lib {
        //     .name = "EASTL",
        //     .rel_include_path = "include",
        //     .lib_path = "EASTL.lib",
        // },
        lib {
            .name = "tracy",
            .rel_include_paths = {"public"},
            .lib_path = "TracyClient.lib",
        },
        lib {
            .name = "yojimbo",
            .buildsystem = buildsystem::premake,
            .rel_include_paths = {"include", "serialize"},
            .lib_out_dir = "../lib/yojimbo/bin/Debug",
            .lib_files = {
                "sodium-builtin.lib",
                "tlsf.lib",
                "netcode.lib",
                "reliable.lib",
                "yojimbo.lib",
            }
        },

        lib {
            .name = "imgui",
            .buildsystem = buildsystem::manual,
            .rel_include_paths = {"."},
            .lib_path = "imgui.lib",
        }
    };
    int lib_count = sizeof(libs) / sizeof(lib);

    std::vector<std::filesystem::path> include_dirs = {
        "src/",
        "src/client",
        "src/common",
        "lib/stb",
        // "lib/yojimbo/serialize",
    };

    target targets[] = {
        target{
            .type = target_type::shared_lib,
            .name = "game",
            .files = {
                source_file { 
                    .type = files_type::glob,
                    .glob_dir = "src/client",
                },
                source_file { 
                    .type = files_type::glob,
                    .glob_dir = "src/common",
                },
            },
            .libs = {
                "box2d",
                "glm",
                "SDL",
                "tracy",
                "yojimbo",
                "imgui",
            },
            .include_dirs = include_dirs,
        },
        target{
            .type = target_type::executable,
            .name = "platform",
            .files = {
                source_file { 
                    .type = files_type::glob,
                    .glob_dir = "src/platform",
                },
            },
            .libs = {
                "SDL",
            },
            .include_dirs = {
                "src",
                "src/platform",
            },
        },
    };

    // build_libs(libs, lib_count);

    for (auto& target : targets) {
        build_target(target, libs, lib_count);
    }
}
