cmake_minimum_required(VERSION 3.15)

# GNN Embeddings Example
project(gnn_embeddings_example)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ThemisDB libraries
find_package(ThemisDB REQUIRED)

# Add executable
add_executable(gnn_embeddings_example
    gnn_embeddings_example.cpp
)

# Link against ThemisDB libraries
target_link_libraries(gnn_embeddings_example
    PRIVATE
        ThemisDB::index
        ThemisDB::storage
        ThemisDB::utils
)

# Include directories
target_include_directories(gnn_embeddings_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Install target
install(TARGETS gnn_embeddings_example
    RUNTIME DESTINATION bin/examples
)
