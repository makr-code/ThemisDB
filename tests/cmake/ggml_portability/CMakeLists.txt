cmake_minimum_required(VERSION 3.20)
project(GgmlPortabilityRegression LANGUAGES NONE)

# Standalone, dependency-free counterpart to GgmlErrnoWorkaroundTests in
# tests/test_deployment_workflow_regressions.py. Never fetch or modify llama.cpp.
# Run: cmake -S tests/cmake/ggml_portability -B /tmp/ggml-portability
#      ctest --test-dir /tmp/ggml-portability --output-on-failure
if(NOT DEFINED CASE)
    enable_testing()
    find_program(FIXTURE_CC clang REQUIRED)
    find_program(FIXTURE_CXX clang++ REQUIRED)
    foreach(_case IN ITEMS AppleClang NonApple OtherCompiler AppleClangID Unsupported
                           MissingTargets MissingMetal MissingCpu MissingMemory
                           MissingEnumException UnrelatedWarning)
        add_test(NAME ggml-portability-${_case}
            COMMAND "${CMAKE_COMMAND}"
                "-DCASE=${_case}"
                "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
                "-DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/${_case}"
                "-DCC=${FIXTURE_CC}" "-DCXX=${FIXTURE_CXX}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/RunCase.cmake")
    endforeach()
    return()
endif()

enable_language(C)
enable_language(CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(APPLE TRUE)
if(CASE STREQUAL "NonApple" OR CASE STREQUAL "MissingMemory" OR
   CASE STREQUAL "MissingEnumException")
    set(APPLE FALSE)
endif()
# These two cases simulate ID guards, not the respective native toolchains.
if(CASE STREQUAL "OtherCompiler")
    set(CMAKE_CXX_COMPILER_ID GNU)
elseif(CASE STREQUAL "AppleClangID")
    set(CMAKE_CXX_COMPILER_ID AppleClang)
elseif(CASE STREQUAL "Unsupported")
    # CheckCXXCompilerFlag's documented cache override models an older Clang.
    set(_THEMIS_HAVE_ELABORATED_ENUM_BASE FALSE CACHE INTERNAL "")
endif()

set(_repo "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
file(READ "${_repo}/cmake/Dependencies.cmake" _dependencies)
string(FIND "${_dependencies}" "FetchContent_MakeAvailable(llama_cpp)" _start)
string(FIND "${_dependencies}" "# APPLY MSVC CHAR8_T FIXES TO LLAMA TARGETS" _end)
if(_start LESS 0 OR _end LESS _start)
    message(FATAL_ERROR "Cannot locate post-FetchContent portability block")
endif()
string(LENGTH "FetchContent_MakeAvailable(llama_cpp)" _length)
math(EXPR _start "${_start} + ${_length}")
math(EXPR _length "${_end} - ${_start}")
string(SUBSTRING "${_dependencies}" ${_start} ${_length} _portability)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/portability.cmake" "${_portability}\n")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/helper.c" "int helper(void) { return 0; }\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/base.cpp"
    "#include <cerrno>\nint base_errno() { return errno; }\n")
set(_memory "#include <memory>\n")
if(APPLE OR CASE STREQUAL "MissingMemory")
    set(_memory "")
endif()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/metal.cpp"
    "${_memory}std::unique_ptr<int> metal_value() { return std::unique_ptr<int>(new int(1)); }\n")
set(_cpu "int cpu_value() { return 0; }\n")
if(CASE STREQUAL "AppleClang" OR CASE STREQUAL "MissingEnumException")
    # Minimal reproduction of the Xcode 15.4 SDK declaration, not an SDK copy.
    string(PREPEND _cpu "typedef enum example : int example;\n")
elseif(CASE STREQUAL "UnrelatedWarning")
    set(_cpu "int cpu_value() { int unused; return 0; }\n")
endif()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cpu.cpp" "${_cpu}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/consumer.cpp" "int consumer() { return 0; }\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/unrelated.cpp" "int unrelated() { return 0; }\n")

foreach(_target IN ITEMS ggml-base ggml-metal ggml-cpu consumer unrelated)
    if(CASE STREQUAL "MissingTargets" AND _target MATCHES "^ggml-")
        continue()
    elseif(CASE STREQUAL "MissingMetal" AND _target STREQUAL "ggml-metal")
        continue()
    elseif(CASE STREQUAL "MissingCpu" AND _target STREQUAL "ggml-cpu")
        continue()
    endif()
    string(REPLACE "ggml-" "" _source "${_target}")
    add_library(${_target} STATIC "${CMAKE_CURRENT_BINARY_DIR}/${_source}.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/helper.c")
    target_compile_options(${_target} PRIVATE -Wall -Werror)
endforeach()
foreach(_target IN ITEMS ggml-base ggml-metal ggml-cpu)
    if(TARGET ${_target})
        target_link_libraries(consumer PRIVATE ${_target})
    endif()
endforeach()
include("${CMAKE_CURRENT_BINARY_DIR}/portability.cmake")
foreach(_target IN ITEMS ggml-base ggml-metal ggml-cpu)
    if(TARGET ${_target})
        get_target_property(_interface ${_target} INTERFACE_COMPILE_OPTIONS)
        if(_interface)
            message(FATAL_ERROR "${_target} exports private portability options: ${_interface}")
        endif()
    endif()
endforeach()
