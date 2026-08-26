include(FetchContent)

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
