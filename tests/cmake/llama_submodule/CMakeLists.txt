cmake_minimum_required(VERSION 3.20)
project(LlamaSubmoduleContract NONE)
enable_testing()
foreach(case IN ITEMS missing unpatched pinned mismatch snapshot)
    add_test(NAME llama_submodule_${case}
        COMMAND "${CMAKE_COMMAND}"
            "-DCASE=${case}"
            "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/${case}"
            "-DCHECK=${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/LlamaCppSubmoduleCheck.cmake"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run.cmake")
endforeach()
