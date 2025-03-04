cmake_minimum_required(VERSION 3.15)
project(neoneo VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Add executable
add_executable(neoneo 
    src/main.cpp
    src/ollama_client.cpp
    src/config/config.cpp
    src/terminal/terminal.cpp
    src/tools/tools_base.cpp
    src/tools/calculator_tool.cpp
    src/tools/shell_tool.cpp
    src/tools/bash_tool.cpp
    src/tools/file_tools.cpp
    src/tools/model_list_tool.cpp
)

# Include directories
target_include_directories(neoneo PRIVATE include)

# Find required packages
find_package(CURL REQUIRED)
find_package(Readline REQUIRED)

# If Readline is not found by the above, try pkg-config
if(NOT Readline_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Readline REQUIRED IMPORTED_TARGET readline)
    set(Readline_LIBRARIES PkgConfig::Readline)
endif()

# Add third-party libraries
include(FetchContent)

# Add nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Link libraries
target_link_libraries(neoneo PRIVATE
    CURL::libcurl
    ${Readline_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Install target
install(TARGETS neoneo DESTINATION bin)

# Enable warnings and debug symbols
if(MSVC)
    target_compile_options(neoneo PRIVATE /W4 /Zi)
    target_link_options(neoneo PRIVATE /DEBUG)
else()
    target_compile_options(neoneo PRIVATE -Wall -Wextra -Wpedantic -g)
endif()

# Set build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Print status
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")