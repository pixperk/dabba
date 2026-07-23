# 3.16 is ubuntu 20.04's version, safe floor
cmake_minimum_required(VERSION 3.16)

# CXX only, so cmake doesn't probe for a C or fortran compiler
project(dabba CXX)

set(CMAKE_CXX_STANDARD 20)

# without this, c++20 is only a preference and silently falls back to 17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# writes build/compile_commands.json so clangd finds our headers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -Wextra includes -Wreorder, which catches member init list order bugs
add_compile_options(-Wall -Wextra -Wpedantic)

# only .cpp goes here. headers get pasted in by the preprocessor.
# forgetting to add a new .cpp is the #1 cause of "undefined reference"
add_executable(dabba
    src/main.cpp
    src/fs_setup.cpp)

# for #include "util.hpp". PRIVATE = don't propagate to linkers of this target
target_include_directories(dabba PRIVATE include)