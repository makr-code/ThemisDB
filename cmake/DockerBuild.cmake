# ThemisDB Docker build helper (CMake-only)
# Usage (example):
# cmake -P cmake/DockerBuild.cmake \
#   -DSOURCE_DIR=C:/VCC/themis \
#   -DOUTPUT_DIR=C:/VCC/themis/build-docker-context \
#   -DDOCKERFILE=C:/VCC/themis/Dockerfile \
#   -DTAG=themisdb:community \
#   -DBUILD_ARGS=THEMIS_EDITION=COMMUNITY;ENABLE_LLM=ON;ENABLE_GPU=OFF;FORCE_CPU_ONLY=ON;BUILD_TESTS=OFF;BUILD_BENCHMARKS=OFF

if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is required")
endif()

string(REPLACE "\"" "" SOURCE_DIR "${SOURCE_DIR}")

if(NOT DEFINED OUTPUT_DIR)
    set(OUTPUT_DIR "${SOURCE_DIR}/build-docker-context")
endif()

string(REPLACE "\"" "" OUTPUT_DIR "${OUTPUT_DIR}")

if(NOT DEFINED DOCKERFILE)
    set(DOCKERFILE "${SOURCE_DIR}/Dockerfile")
endif()

string(REPLACE "\"" "" DOCKERFILE "${DOCKERFILE}")

if(NOT DEFINED TAG)
    set(TAG "themisdb:community")
endif()

find_program(DOCKER_EXECUTABLE docker)
if(NOT DOCKER_EXECUTABLE)
    message(FATAL_ERROR "docker not found in PATH")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -DSOURCE_DIR="${SOURCE_DIR}"
        -DOUTPUT_DIR="${OUTPUT_DIR}"
        -P "${SOURCE_DIR}/cmake/DockerContext.cmake"
    RESULT_VARIABLE _ctx_result
)

if(NOT _ctx_result EQUAL 0)
    message(FATAL_ERROR "Failed to prepare docker context")
endif()

set(_docker_cmd "${DOCKER_EXECUTABLE}" build -t "${TAG}" -f "${DOCKERFILE}")

if(DEFINED BUILD_ARGS)
    foreach(_arg IN LISTS BUILD_ARGS)
        list(APPEND _docker_cmd --build-arg "${_arg}")
    endforeach()
endif()

list(APPEND _docker_cmd "${OUTPUT_DIR}")

execute_process(
    COMMAND ${_docker_cmd}
    RESULT_VARIABLE _build_result
)

if(NOT _build_result EQUAL 0)
    message(FATAL_ERROR "docker build failed")
endif()
