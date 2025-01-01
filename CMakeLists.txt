cmake_minimum_required (VERSION 3.11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER "clang++")

project (idk LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)


# find_package(GameNetworkingSockets CONFIG REQUIRED)
find_package(SDL3 REQUIRED)

add_custom_target(compile_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E env py ${CMAKE_SOURCE_DIR}/scripts/compile_shaders.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(define_assets ALL
    COMMAND ${CMAKE_COMMAND} -E env py ${CMAKE_SOURCE_DIR}/scripts/define_assets.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# option(BUILD_SHARED_LIBS OFF)
#
# add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

option(TRACY_ENABLE "" OFF)
option(TRACY_ON_DEMAND "" OFF)

add_subdirectory(lib/EASTL)
add_subdirectory(lib/tracy)
add_subdirectory(lib/glm)
add_subdirectory(lib/box2d)


# list(REMOVE_ITEM SRC_CLIENT 
#     "${CMAKE_CURRENT_SOURCE_DIR}/src/game.cpp"
#     "${CMAKE_CURRENT_SOURCE_DIR}/src/editor.cpp"
# )

set(imgui_dir lib/imgui)

file(GLOB imgui CONFIGURE_DEPENDS lib/imgui/*.cpp lib/imgui/*.h)


file(GLOB_RECURSE src_client CONFIGURE_DEPENDS src/client/*.cpp)

file(GLOB_RECURSE src_common CONFIGURE_DEPENDS src/common/*.cpp)


add_library(common ${src_common})

target_link_libraries(common PUBLIC
    glm::glm
    ${yojimbo_lib}
    EASTL
    box2d
)

file(GLOB_RECURSE PLS CONFIGURE_DEPENDS src/common/*.h)
set(COMMON_DIR ${CMAKE_SOURCE_DIR}/src/common/)

set(kys_dir ${CMAKE_SOURCE_DIR}/src/client)

# set(die
#     ${kys_dir}/imgui_impl_sdl3.cpp
#     ${kys_dir}/imgui_impl_sdl3.h
#     ${kys_dir}/imgui_impl_sdlrenderer3.cpp
#     ${kys_dir}//imgui_impl_sdlrenderer3.h
# )
#
# list(REMOVE_ITEM src_client ${die})

message(STATUS "Source files: ${src_client}")

add_executable(client ${src_client} ${imgui})

target_include_directories(client PRIVATE
    lib/stb
    lib/glm/include
    lib/SDL/include

    lib/imgui

    ${COMMON_DIR}

    ${yojimbo_DIR}/include
    ${yojimbo_DIR}/serialize
)


# add_library(kys SHARED kys.cpp)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(yojimbo_build_dir ${yojimbo_DIR}/bin/Debug)
else()
    set(yojimbo_build_dir ${yojimbo_DIR}/bin/Release)
endif()

set(yojimbo_lib
    ${yojimbo_build_dir}/sodium-builtin.lib
    ${yojimbo_build_dir}/tlsf.lib
    ${yojimbo_build_dir}/netcode.lib
    ${yojimbo_build_dir}/reliable.lib
    ${yojimbo_build_dir}/yojimbo.lib
)


target_link_libraries(client PUBLIC TracyClient)

target_link_libraries(client PRIVATE 
    SDL3::SDL3
    glm::glm
    ${yojimbo_lib}
    EASTL
    common

    # box2d::box2d

    # GameNetworkingSockets::static
)

file(GLOB_RECURSE src_server CONFIGURE_DEPENDS src/server/*.cpp)
list(APPEND src_server lib/imgui/backends/imgui_impl_sdl3.cpp lib/imgui/backends/imgui_impl_sdl3.h ${imgui_dir}/backends/imgui_impl_sdlrenderer3.cpp)

set(COMMON_DIR ${CMAKE_SOURCE_DIR}/src/common/)

add_executable(server ${src_server} ${imgui})

target_include_directories(server PRIVATE
    # lib/stb
    # lib/glm/include
    # lib/SDL/include
    lib/imgui

    ${COMMON_DIR}
    ${yojimbo_DIR}/include
    ${yojimbo_DIR}/serialize
)

target_link_libraries(server PRIVATE
    SDL3::SDL3
    glm::glm
    ${yojimbo_lib}
    common
    EASTL
)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}
)

add_dependencies(copy_assets compile_shaders)

add_custom_target(copy_build_script
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/scripts/run.ps1 ${CMAKE_CURRENT_BINARY_DIR}
)

add_dependencies(client copy_assets compile_shaders copy_build_script)

add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:client> $<TARGET_FILE_DIR:client>
    COMMAND_EXPAND_LISTS
)
#
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL3_DIR}/SDL3.pdb $<TARGET_FILE_DIR:${PROJECT_NAME}>
# )
#
# option(COPY_TO_DISTRIBUTION "Copy application, libs, and data to clean distribution output dir" OFF)
#
# if (COPY_TO_DISTRIBUTION)
#     set(DISTRIBUTION_DIR ${CMAKE_SOURCE_DIR}/dist)
#     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> ${DISTRIBUTION_DIR}
#         COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${DISTRIBUTION_DIR}
#         COMMAND_EXPAND_LISTS
#     )
#
#     add_custom_target(copy_assets_to_dist
#         COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${DISTRIBUTION_DIR}/data
#     )
#
#     add_dependencies(${PROJECT_NAME} copy_assets_to_dist)
# endif()
#
#
# install(TARGETS ${PROJECT_NAME}
#     RUNTIME DESTINATION bin
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
# )
#
# install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/ DESTINATION ${CMAKE_INSTALL_PREFIX}/data)
