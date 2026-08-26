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

add_executable(bird_flu_tracker main.cpp)
target_link_libraries(bird_flu_tracker PRIVATE httplib::httplib)

add_custom_command(TARGET bird_flu_tracker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/web
        $<TARGET_FILE_DIR:bird_flu_tracker>/web
)
