include(FetchContent)

# Some machines (e.g. those with MSYS2/MinGW on PATH) have a zstd CMake
# package on the system that CMake's find_package() auto-discovers. cpp-httplib
# would then link against it and pull MinGW headers into an MSVC build,
# breaking compilation. Disable the auto-detected zstd support since this
# project doesn't need it.
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

if (MSVC)
    add_compile_options(/utf-8)
endif()

add_library(flu_core STATIC
    flu_data.cpp
    risk.cpp
)
target_link_libraries(flu_core PUBLIC nlohmann_json::nlohmann_json)
target_include_directories(flu_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(bird_flu_tracker main.cpp)
target_link_libraries(bird_flu_tracker PRIVATE flu_core httplib::httplib)

add_executable(backend_tests tests.cpp)
target_link_libraries(backend_tests PRIVATE flu_core)

add_custom_command(TARGET bird_flu_tracker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/web
        $<TARGET_FILE_DIR:bird_flu_tracker>/web
)
