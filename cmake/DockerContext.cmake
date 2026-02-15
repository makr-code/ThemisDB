# ThemisDB Docker context generator (CMake-only)
# Copies tracked files into a clean context directory to avoid huge/locked trees.

if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is required")
endif()

string(REPLACE "\"" "" SOURCE_DIR "${SOURCE_DIR}")

if(NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "OUTPUT_DIR is required")
endif()

string(REPLACE "\"" "" OUTPUT_DIR "${OUTPUT_DIR}")

file(REMOVE_RECURSE "${OUTPUT_DIR}")
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

# Allowlist of paths required by Dockerfile
set(_include_paths
    CMakeLists.txt
    VERSION
    Dockerfile
    docker
    cmake
    include
    src
    proto
    internal
    ports
    vcpkg.json
    vcpkg-configuration.json
    docker/vcpkg-community.json
    docker/vcpkg-minimal.json
    docker/vcpkg-enterprise.json
    docker/vcpkg-hyperscaler.json
    llama.cpp
)

foreach(_rel_path IN LISTS _include_paths)
    set(_src "${SOURCE_DIR}/${_rel_path}")
    if(EXISTS "${_src}")
        get_filename_component(_rel_dir "${_rel_path}" DIRECTORY)
        if(_rel_dir)
            file(MAKE_DIRECTORY "${OUTPUT_DIR}/${_rel_dir}")
            file(COPY "${_src}" DESTINATION "${OUTPUT_DIR}/${_rel_dir}")
        else()
            file(COPY "${_src}" DESTINATION "${OUTPUT_DIR}")
        endif()
    endif()
endforeach()
