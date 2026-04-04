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
    llama.cpp
)

foreach(_rel_path IN LISTS _include_paths)
    set(_src "${SOURCE_DIR}/${_rel_path}")
    if(EXISTS "${_src}")
        if(IS_DIRECTORY "${_src}")
            # For directories, use copy_directory to preserve all contents
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${_src}" "${OUTPUT_DIR}/${_rel_path}"
                RESULT_VARIABLE _copy_result
            )
        else()
            # For files, copy to parent directory or root
            get_filename_component(_rel_dir "${_rel_path}" DIRECTORY)
            if(_rel_dir)
                file(MAKE_DIRECTORY "${OUTPUT_DIR}/${_rel_dir}")
                file(COPY "${_src}" DESTINATION "${OUTPUT_DIR}/${_rel_dir}")
            else()
                file(COPY "${_src}" DESTINATION "${OUTPUT_DIR}")
            endif()
        endif()
    endif()
endforeach()

message(STATUS "Docker context prepared with allowlist")

