cmake_minimum_required (VERSION 3.11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER "clang++")

project (idk LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)


add_custom_target(compile_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E env py ${CMAKE_SOURCE_DIR}/scripts/compile_shaders.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(define_assets ALL
    COMMAND ${CMAKE_COMMAND} -E env py ${CMAKE_SOURCE_DIR}/scripts/define_assets.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


# add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

option(TRACY_ENABLE "" OFF)
option(TRACY_ON_DEMAND "" OFF)

# add_subdirectory(lib/EASTL)
add_subdirectory(lib/tracy)
add_subdirectory(lib/glm)

set(SOURCE_DIRECTORY src)
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS ${SOURCE_DIRECTORY}/*.cpp)

# list(REMOVE_ITEM SOURCE_FILES 
#     "${CMAKE_CURRENT_SOURCE_DIR}/src/game.cpp"
#     "${CMAKE_CURRENT_SOURCE_DIR}/src/editor.cpp"
# )


add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    lib/stb
)

target_link_libraries(${PROJECT_NAME} PUBLIC TracyClient)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3::SDL3
    glm::glm
    # EASTL
)

file(GLOB_RECURSE SHADER_SOURCE_FILES CONFIGURE_DEPENDS shaders/*.vert shaders/*.frag shaders/*.comp)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}
)

add_dependencies(${PROJECT_NAME} copy_assets)
#
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
)
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
