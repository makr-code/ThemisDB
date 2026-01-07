# ThemisDB Root CMakeLists.txt
# This file delegates to cmake/CMakeLists.txt to keep root clean
# Purpose: Minimal root CMakeLists for toolchain and module discovery

cmake_minimum_required(VERSION 3.20)

# Delegate to cmake/CMakeLists.txt where actual configuration happens
add_subdirectory(cmake)

