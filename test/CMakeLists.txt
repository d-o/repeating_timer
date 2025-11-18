cmake_minimum_required(VERSION 3.14)

project(AsyncTimerTest LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Enable/Disable output of compile commands during generation." FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)

# Fetch the content *before* any target is created.
FetchContent_MakeAvailable(asio)

add_executable(repeating_timer_test
    ${CMAKE_SOURCE_DIR}/test.cpp
)

target_include_directories(repeating_timer_test PRIVATE
    ${asio_SOURCE_DIR}/asio/include
    ${CMAKE_SOURCE_DIR}/../
)

add_executable(multi_timer_test
    ${CMAKE_SOURCE_DIR}/multi_thread_test.cpp
)

target_include_directories(multi_timer_test PRIVATE
    ${asio_SOURCE_DIR}/asio/include
    ${CMAKE_SOURCE_DIR}/../
)

find_package(Threads REQUIRED)
target_link_libraries(repeating_timer_test PRIVATE Threads::Threads)

target_compile_options(repeating_timer_test PRIVATE
    -Wall -Wextra -Wpedantic
)
