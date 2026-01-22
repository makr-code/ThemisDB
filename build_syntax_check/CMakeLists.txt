cmake_minimum_required(VERSION 3.20)
project(OptimizerCostModelTest CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Minimal configuration - no includes from parent
include_directories(../include)

# Compile the implementation
add_library(optimizer_cost_model OBJECT
    ../src/query/optimizer_cost_model.cpp
)

target_include_directories(optimizer_cost_model PUBLIC 
    ../include
)

set_target_properties(optimizer_cost_model PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

message(STATUS "Optimizer cost model compilation check complete")
