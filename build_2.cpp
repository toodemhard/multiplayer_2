#include <filesystem>
#include <format>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>

#include <cstdlib>
#include "tinycthread.h"

#include <stdarg.h>
#include "common/types.h"
#include "common/allocator.h"

#include "tinycthread.c"
#include "common/allocator.cpp"
 

#include <iostream>

struct Future {
    bool finish;
    mtx_t mtx;
    cnd_t cnd;
};

typedef void (*Fn)(void* data);

struct Task {
    Fn fn;
    void* data;
    Future* future;
};


constexpr u64 max_threads = 256;

struct ThreadPool {
    RingBuffer<Task, 1024> task_queue;
    mtx_t queue_mtx;
    cnd_t queue_cnd;

    Slice<Future> futures;
    mtx_t futures_mtx;

    thrd_t threads[max_threads];
    u32 thread_count; 

    bool terminate;
};

ThreadPool pool;

void wait_future(Future* future) {
    mtx_lock(&future->mtx);
    while (future->finish != true) {
        cnd_wait(&future->cnd, &future->mtx);
    }
    mtx_unlock(&future->mtx);
}

int worker_thread(void* data) {
    ThreadPool* pool = (ThreadPool*)data;

    while (true) {

        mtx_lock(&pool->queue_mtx);
        while (pool->task_queue.length <= 0 && !pool->terminate) {
            cnd_wait(&pool->queue_cnd, &pool->queue_mtx);
        }
        if (pool->terminate) {
            mtx_unlock(&pool->queue_mtx);
            break;
        }
        Task task = ring_buffer_pop_front(&pool->task_queue);
        mtx_unlock(&pool->queue_mtx);

        task.fn(task.data);
        mtx_lock(&task.future->mtx);
        task.future->finish = true;
        mtx_unlock(&task.future->mtx);
        cnd_signal(&task.future->cnd);
    }

    return 0;
}

void thread_pool_init(ThreadPool* pool, Arena* a, int thread_count) {
    *pool = {};
    mtx_init(&pool->queue_mtx, 0);
    cnd_init(&pool->queue_cnd);
    mtx_init(&pool->futures_mtx, 0);

    slice_init(&pool->futures, a, 1000);

    pool->thread_count = thread_count;
    for (int i = 0; i < thread_count; i++) {
        thrd_create(&pool->threads[i], worker_thread, pool);
    }
}

void thread_pool_shutdown(ThreadPool* pool) {
    mtx_lock(&pool->queue_mtx);
    pool->terminate = true;
    mtx_unlock(&pool->queue_mtx);
    cnd_broadcast(&pool->queue_cnd);

    for (int i = 0; i < pool->thread_count; i++) {
        thrd_join(&pool->threads[i], NULL);
    }
}

Future* queue_task(ThreadPool* pool, Fn task, void* data) {
    mtx_lock(&pool->queue_mtx);
    

    mtx_lock(&pool->futures_mtx);
    slice_push(&pool->futures, {});
    Future* future = slice_back(pool->futures);
    mtx_init(&future->mtx, 0);
    cnd_init(&future->cnd);

    
    ring_buffer_push_back(&pool->task_queue, {
        .fn = task,
        .data = data,
        .future = future,
    });

    mtx_unlock(&pool->futures_mtx);
    mtx_unlock(&pool->queue_mtx);

    cnd_signal(&pool->queue_cnd);

    return future;
}

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
    std::filesystem::path value;
};

source_file src_glob(std::filesystem::path value) {
    return {files_type::glob, value};
}

source_file src_single(std::filesystem::path value) {
    return {files_type::single, value};
}

enum class target_type {
    executable,
    static_lib,
    shared_lib,
    precompiled_header,
};

struct Target {
    target_type type;

    const char* name;
    std::string main;
    std::vector<source_file> files;
    std::vector<std::string> libs;
    std::vector<std::filesystem::path> include_dirs;
    std::string linker_flags;

    //assuming pch.h, pch.cpp producees pch.pch: pch_name is pch
    std::string pch_name;

    std::string crt;
};

struct Lib {
    const char* name;
    buildsystem buildsystem;
    // .lib file path relative to lib build dir
    std::vector<std::filesystem::path> rel_include_paths;
    std::filesystem::path lib_path;
    std::filesystem::path shared_lib_path;

    std::filesystem::path lib_out_dir;
    std::vector<std::string> lib_files;

    std::string cmake_args;

    // manual
    source_file source_files;
};

const char* lib_path = "../../lib";

// #define TRACY_DEFINES "/D TRACY_ENABLE /D TRACY_ON_DEMAND"// /D TRACY_MANUAL_LIFETIME /D TRACY_DELAYED_INIT";
#define TRACY_DEFINES ""
const char* tracy_defines = TRACY_DEFINES;


std::filesystem::path build_dir;
std::filesystem::path project_root;
std::filesystem::path compiler_path;

struct timed {
    const char* name;
    std::chrono::duration<double, std::milli> duration;
    int depth;
};

std::vector<timed> g_profile;
int g_timer_scope_depth = 0;

struct timer {
    const char* name;
    std::chrono::time_point<std::chrono::steady_clock> start;
    bool ended = false;

    timer(const char* _name) {
        name = _name;
        start = std::chrono::high_resolution_clock::now();

        g_timer_scope_depth++;
    }

    ~timer() {
        if (!ended) {
            end_timer();
        }
    }

    void end_timer()  {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =  std::chrono::duration<double, std::milli>(end - start);

        g_timer_scope_depth--;
        g_profile.push_back({
            .name = name,
            .duration = duration,
            .depth = g_timer_scope_depth,
        });

        ended = true;
    }
};

std::string strings[1000];
int string_count = 0;

const char* static_string(std::string string) {
    strings[string_count] = string;

    string_count++;

    return strings[string_count - 1].data();
}

void build_libs(Lib* libs, int lib_count) {
    for (int i = 0; i < lib_count; i++) {
        auto& lib = libs[i];

        timer timer(static_string(std::format("{}", lib.name)));

        bool skip = true;

        if (!std::filesystem::exists(std::filesystem::path(lib.name) / lib.lib_path)) {
            skip = false;
        }

        if (!lib.shared_lib_path.empty() && !std::filesystem::exists(lib.shared_lib_path.filename())) {
            skip = false;
        }

        if (skip) {
            continue;
        }


        if (lib.buildsystem == buildsystem::cmake) {
            printf("building %s\n", lib.name);

            std::string commands;
            commands += std::format("mkdir {0} & ", lib.name);
            commands += std::format("cd {} &&", lib.name);
            commands += std::format("cmake -S {}/{} {} -B . &&", lib_path, lib.name, lib.cmake_args);
            commands += std::format("cmake --build .");

            std::cout << commands;

            system(commands.data());
        }

        if (lib.buildsystem == buildsystem::manual) {
            std::string commands;
            commands += std::format("mkdir {0} & ", lib.name);
            commands += std::format("cd {} &&", lib.name);
            commands += std::format(R"(cl {} /LD {}/public/TracyClient.cpp)", tracy_defines, (project_root / "lib" / lib.name).string() ).data();

            printf("%s\n", commands.data());

            system(commands.data());
        }

        if (!lib.shared_lib_path.empty() && !std::filesystem::exists(lib.shared_lib_path.filename()) ) {
            std::filesystem::copy(std::filesystem::path(lib.name) / lib.shared_lib_path, lib.shared_lib_path.filename());
        }
    }
}

void glob_files(std::vector<std::filesystem::path>* source_files, std::vector<std::filesystem::path>* header_files, std::filesystem::path path) {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        const auto& extension = entry.path().extension();
        if (extension == ".cpp" || extension == ".c") {
            source_files->push_back(entry.path().lexically_normal());
        }
        if (extension == ".h" || extension == ".hpp") {
            header_files->push_back(entry.path().lexically_normal());
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

std::string add_compile_command(std::string flags, std::string includes, std::string compile_file, std::string file) {
    includes = escape_json(includes);

    std::string commands_json = "";

    std::string command = std::format("{} {} {} {}", compiler_path.string(), flags, includes, escape_json(compile_file));

    commands_json += std::format(R"(
{{
    "directory": "{}",
    "command": "{}",
    "file": "{}",
    "output": ""
}},)", escape_json(build_dir.string()), command, escape_json(file));

    return commands_json;
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

void build_target(Target target, Lib libs[], int lib_count, std::string compiler_flags, std::string& commands_json) {
    timer total(target.name);

    std::unordered_map<std::string, int> lib_index;

    for (int i = 0; i < lib_count; i++) {
        lib_index[libs[i].name] = i;
    }
    std::string includes = "";
    // for (auto& lib_name : target.libs) {
    //     auto& lib = libs[lib_index[lib_name]];
    //     for (auto& include_path : lib.rel_include_paths) {
    //         includes += std::format("/I {} ", (project_root / "lib" / lib.name / include_path).lexically_normal().string());
    //     }
    // }

    for (auto& include_dir : target.include_dirs) {
        includes += std::format("-I {} ", (project_root / include_dir).lexically_normal().string());
    }

    // std::string pch_header = escape_json((project_root / "src" / "pch.h").lexically_normal().string());
    // std::string pch_file = escape_json((project_root / "b2" / "pch.pch").lexically_normal().string());

    std::string out_dir = std::format("targets\\{}\\", target.name);

    // std::string commands = "";
    // commands += std::format("mkdir {} &", out_dir);
    std::filesystem::create_directory(out_dir);


    std::vector<std::filesystem::path> source_files = {};
    std::vector<std::filesystem::path> header_files = {};

    for (auto& src : target.files) {
        if (src.type == files_type::glob) {
            glob_files(&source_files, &header_files, project_root / src.value);
        }

        if (src.type == files_type::single) {
            auto file_path = project_root / src.value;
            const auto& extension = file_path.extension();
            if (extension == ".cpp" || extension == ".c") {
                source_files.push_back(file_path.lexically_normal());
            }
            if (extension == ".h" || extension == ".hpp") {
                header_files.push_back(file_path.lexically_normal());
            }
        }
    }


    std::unordered_map<std::string, std::filesystem::file_time_type> obj_write_times;
    std::filesystem::file_time_type last_compile;

    for (const auto& entry : std::filesystem::directory_iterator(out_dir)) {
        const auto& write_time = std::filesystem::last_write_time(entry.path());
        obj_write_times[entry.path().filename().stem().string()] = write_time;
        if (write_time > last_compile) {
            last_compile = write_time;
        }
    }

    bool compile = false;
    if (target.type == target_type::precompiled_header) {
        // last_compile = std::filesystem::last_write_time("game.o");
        if (std::filesystem::exists("./pch.pch")) {
            last_compile = std::filesystem::last_write_time("./pch.pch");
        } else {
            compile = true;
        }
        printf("%s\n", std::filesystem::current_path().string().data());
    }

    for (auto& src : source_files) {
        std::string include_ext = "";
        include_ext += std::format(" -include {} ", (project_root / "src" / (target.pch_name + ".h") ).string());
        if (src != project_root / target.main) {
            include_ext += std::format(" -include {} ", (project_root / target.main).string());
        }
        include_ext += includes;

        commands_json += add_compile_command(compiler_flags,include_ext, src.string(), src.string());
    }
    for (auto& h : header_files) {
        std::string include_ext = "";
        include_ext += std::format(" -include {} ", (project_root / "src" / (target.pch_name + ".h") ).string());
        include_ext += std::format(" -include {} ", (project_root / target.main).string());
        include_ext += includes;

        commands_json += add_compile_command(compiler_flags,include_ext, h.string() + ".cpp", h.string());
    }


    if (target.type != target_type::precompiled_header) {
        std::filesystem::path main_filepath = project_root / target.main;
        std::string main_name = main_filepath.filename().stem().string();
        if (!obj_write_times.contains(main_name)) {
            compile = true;
        }

        for (auto& src : source_files) {
            if (obj_write_times[main_name] < std::filesystem::last_write_time(src)) {
                compile = true;
            }

            if (compile) {
                break;
            }
        }
    }

    if (target.main != "") {
        std::filesystem::path main_filepath = project_root / target.main;
        std::string name = main_filepath.filename().stem().string();

        if (!obj_write_times.contains(name)) {
            compile = true;
        } else if (obj_write_times[name] < std::filesystem::last_write_time(main_filepath)) {
            compile = true;
        }
    }


    for (auto& h : header_files) {
        if (std::filesystem::last_write_time(h) > last_compile) {
            compile = true;
            break;
        }
    }

    if (!compile) {
        printf("target: %s skipped\n", target.name);
    }


    ArenaTemp scratch = scratch_get(0,0);
    defer(scratch_release(scratch));
    if (compile) {
        timer timer("compile");

        if (target.type == target_type::precompiled_header) {
            for (auto& h : header_files) {
                std::string pch_arg = "";
                // pch_arg = std::format("/Yc{}.h", target.name);
                pch_arg = std::format("-x c++-header {}", h.string());
                // command = std::format("cl /diagnostics:color {} {} {} {} /Fo{}", compiler_flags, pch_arg, includes, source_args, out_dir);
                std::string command = std::format("clang {} {} {} -o {}.pch", compiler_flags, pch_arg, includes, target.name);

                system(command.data());
            }
        } else {
            std::string command = "";
            std::string use_pch_arg = "";


            if (target.pch_name != "") {
                // use_pch_arg = std::format(R"(/Yu"{0}.h" /Fp"{0}.pch")", target.pch_name);
                use_pch_arg = std::format("-include-pch {}", (build_dir / (target.pch_name + ".pch")).string() );
            }

            command += std::format("cd targets/{} && ", target.name);
            command += std::format("clang {} {} {} -c {}", compiler_flags, use_pch_arg, includes, (project_root / target.main).string());

            system(command.data());
            printf("%s\n", command.data());
        }
    }

    bool link = false;

    std::string out_file;
    switch (target.type) {
        case target_type::executable: {
            out_file = std::format("{}.exe", target.name);
        } break;
        case target_type::shared_lib: {
            out_file = std::format("{}.dll", target.name);
        } break;
        case target_type::static_lib: {
            out_file = std::format("{}.lib", target.name);
        } break;
    }

    if (out_file != "") {
        if (!std::filesystem::exists(out_file) || compile == true) {
            link = true;
        } else if (std::filesystem::last_write_time(out_file) < last_compile) {
            link = true;
        }
    }

    if (!link) {
        printf("%s skip linking\n", target.name);
    }

    if (link) {
        timer timer("linking");

        std::string lib_files = "";
        for (auto& lib_name : target.libs) {

            auto& lib = libs[lib_index[lib_name]];
            if (!lib.lib_path.empty()) {
                lib_files += (std::filesystem::path(lib.name) / lib.lib_path).lexically_normal().string() + " ";
            }

            if (!lib.lib_out_dir.empty()) {
                for (auto& lib_file : lib.lib_files) {
                    lib_files += (lib.lib_out_dir / lib_file).lexically_normal().string() + " ";
                }
            }

        }

        if (strcmp(target.name, "game") == 0) {
            system("del *_game.pdb *_game.rdi");
        }

        std::string pch_obj = "";
        // if (target.pch_name != "") {
        //     pch_obj = std::format("targets/{0}/{0}.obj", target.pch_name);
        // }

        std::string obj_args = out_dir + std::filesystem::path(target.main).stem().string() + ".o";
        // for (auto& src : source_files) {
        //     obj_args += " " + out_dir + src.stem().string() + ".o";
        // }

        std::string commands = "";
        const char* linker_flags = "/DEBUG /INCREMENTAL:NO";
        switch (target.type) {
        case target_type::executable: {
            commands += std::format("link {} {} {} {} {} {} /OUT:{}.exe", linker_flags, target.linker_flags, pch_obj, obj_args, lib_files, target.crt, target.name);
        } break;
        case target_type::shared_lib: {
            auto now = std::chrono::system_clock::now().time_since_epoch().count();

            commands += std::format("link {} /DLL {} {} {} {} /PDB:{}_game.pdb /OUT:{}.dll /EXPORT:update", linker_flags, obj_args, pch_obj, target.crt, lib_files, now, target.name);
        } break;
        }

        system(commands.data());
        printf("%s\n", commands.data());
    }

}

#define ns_per_s 100000000
int main(int argc, char* argv[]) {
    u64 arena_size = megabytes(4);
    Arena arena = arena_create(malloc(arena_size), arena_size);

    Arena arenas[scratch_count];
    for (i32 i = 0; i < scratch_count; i++)  {
        arenas[i] = arena_suballoc(&arena, megabytes(1));
        scratch_arenas[i] = &arenas[i];
    }

    // thread_pool_init(&pool, &arena, 8);
    // defer(thread_pool_shutdown(&pool));

    // u32 task_count = 16;
    // Slice<Future*> futures = slice_create_fixed<Future*>(&arena, task_count);
    // for (int i = 0; i < task_count; i++) {
    //     futures[i] = queue_task(&pool, [](void*) {
    //         timespec time = {
    //             .tv_sec = 1,
    //             // .tv_nsec = ns_per_s * 4,
    //         };
    //         thrd_sleep(&time, NULL);
    //         printf("kys\n");
    //     }, NULL);
    // }
    //
    //
    // for (int i = 0; i < task_count; i++) {
    //     wait_future(futures[i]);
    // }

    // printf("%s\n", compile_flags);

    timer total_time("total");
    // g_timer_scope_depth--;

    {
        timer timer("get paths");

        build_dir = std::filesystem::current_path();
        project_root = (std::filesystem::current_path() / "..").lexically_normal();
        compiler_path = std::format("\\\"{}\\\"", escape_json(command_output("which clang")));
    }


    bool release = false;


    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-release") == 0) {
            release = true;
            printf("aaaaa\n");
        }
    }

    Lib libs[] = {
        Lib {
            .name = "box2d",
            .rel_include_paths = {"include"},
            .lib_path = "src/box2dd.lib",
            .shared_lib_path = "src/box2dd.dll",
            .cmake_args = R"(-DBUILD_SHARED_LIBS=ON)"
        },
        Lib {
            .name = "SDL",
            .rel_include_paths = {"include"},
            .lib_path = "SDL3.lib",
            .shared_lib_path = "SDL3.dll"
        },
        Lib {
            .name = "tracy",
            .buildsystem = buildsystem::cmake,
            .rel_include_paths = {"public"},
            .lib_path = "TracyClient.lib",
            .shared_lib_path = "TracyClient.dll",
            .cmake_args = "-DBUILD_SHARED_LIBS=ON -DTRACY_ENABLE=OFF -DTRACY_ON_DEMAND=OFF"// -DTRACY_DELAYED_INIT=ON -DTRACY_MANUAL_LIFETIME=ON"
        },
        Lib {
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
        Lib {
            .name = "enet",
            .rel_include_paths = {"include"},
            .lib_path = "enet.lib",
            .shared_lib_path = "enet.dll",
            .cmake_args = "-DBUILD_SHARED_LIBS=ON"
        },
    };
    int lib_count = sizeof(libs) / sizeof(Lib);

    std::vector<std::filesystem::path> include_dirs = {
        "src/",
        "lib/stb"
        // "src/client",
        // "src/common",
    };

    for (auto& lib : libs) {
        // auto& lib = libs[lib_index[lib_name]];
        for (auto& include_path : lib.rel_include_paths) {
            include_dirs.push_back(project_root / "lib" / lib.name / include_path);
            // std::format("/I {} ", (project_root / "lib" / lib.name / include_path).lexically_normal().string());
        }
    }

    Target targets[] = {
        Target{
            .type = target_type::precompiled_header,
            .name = "pch",
            .files = {
                src_single("src/pch.h"),
            },
            .libs = {
                "box2d",
                "SDL",
                "tracy",
                "enet",
            },
            .include_dirs = include_dirs,
        },
        Target{
            .type = target_type::shared_lib,
            .name = "game",
            .main = "src/client/app.cpp",
            .files = {
                source_file { 
                    .type = files_type::glob,
                    .value = "src/client",
                },
                source_file { 
                    .type = files_type::glob,
                    .value = "src/common",
                },
                src_glob("src/os"),
            },
            .libs = {
                "box2d",
                "SDL",
                "tracy",
                "enet",
            },
            .include_dirs = include_dirs,
            .pch_name = "pch",
        },
        Target{
            .type = target_type::executable,
            .name = "platform",
            .main = "src/platform/main.cpp",
            .files = {
                source_file { 
                    .type = files_type::glob,
                    .value = "src/platform",
                },
                src_single("src/pch.h"),
            },
            .libs = {
                "tracy",
                "box2d",
                "SDL",
                "enet",
            },
            .include_dirs = include_dirs,
            .pch_name = "pch",
        },
        Target{
            .type = target_type::executable,
            .name = "server",
            .main = "src/server/main.cpp",
            .files = {
                source_file { 
                    .type = files_type::glob,
                    .value = "src/server",
                },
                source_file { 
                    .type = files_type::glob,
                    .value = "src/common",
                },
            },
            .libs = {
                // "tracy",
                "box2d",
                "enet",
                "SDL",
            },
            .include_dirs = include_dirs,
            .pch_name = "pch",
        },
    };

    std::string crt = "msvcrtd.lib";
    // std::string add_flags = "/Zi /MDd";
    std::string add_flags = "-g";

    if (release) {
        // add_flags = "/MT";
        add_flags = "";
        crt = "libcmt.lib";
    } 

    std::string compiler_flags = std::format("-std=c++20 -w -O0 -fno-exceptions -fno-rtti -Wno-deprecated-declarations -ftime-trace {} " TRACY_DEFINES " -D ENET_DLL", add_flags);

    {
        timer timer("build libs");
        build_libs(libs, lib_count);
    }
    std::string commands_json;

    // std::string includes;
    // for (int i = 0; i < lib_count; i++) {
    //     auto& lib = libs[i];
    //     for (auto& include_path : lib.rel_include_paths) {
    //         includes += std::format("-I {} ", (project_root / "lib" / lib.name / include_path).lexically_normal().string());
    //     }
    // }

    {
        timer timer("build targets");
        std::filesystem::create_directory("targets");
        for (auto& target : targets) {
            target.crt = crt;
            build_target(target, libs, lib_count, compiler_flags, commands_json);
        }
    }


    {
        timer timer("write compile_commands.json");

        commands_json.pop_back();

        std::ofstream file("compile_commands.json");
        file << "[\n";
        file << commands_json;
        file << "]";
    }

    total_time.end_timer();

    printf("\n");
    for (int i = g_profile.size() - 1; i >= 0; i--) {
        auto& timer = g_profile[i];

        std::string indentation = "";
        for (int j = 0; j < timer.depth; j++) {
            indentation += "    ";
        }

        printf("%s", std::format("{}{}: {}\n", indentation, timer.name, timer.duration).data());
    }
}
