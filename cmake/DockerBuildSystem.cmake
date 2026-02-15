# ThemisDB Docker Build System - CMake Module
# Build Docker images using CMake custom targets

include_guard(GLOBAL)

# ============================================================================
# Configuration
# ============================================================================

set(THEMIS_DOCKER_REGISTRY "" 
    CACHE STRING "Docker registry for pushing images")

set(THEMIS_DOCKER_TAG "latest" 
    CACHE STRING "Docker image tag")

set(THEMIS_DOCKER_BUILDKIT ON 
    CACHE BOOL "Enable Docker BuildKit")

# Package store for pre-built packages
set(THEMIS_PACKAGE_STORE "${CMAKE_SOURCE_DIR}/vcpkg_packages")

# ============================================================================
# Find Docker
# ============================================================================

find_program(DOCKER_EXECUTABLE docker
    HINTS /usr/bin /usr/local/bin
    DOC "Docker executable"
)

if(NOT DOCKER_EXECUTABLE)
    message(WARNING "Docker not found - docker build targets will not work")
    return()
endif()

message(STATUS "Docker found: ${DOCKER_EXECUTABLE}")

# ============================================================================
# Docker Build Helper Function
# ============================================================================

function(add_docker_build_target)
    cmake_parse_arguments(
        ARGS
        ""  # Options
        "NAME;DOCKERFILE;TAG;EDITION;CONFIGURATION;BUILD_CONTEXT"  # Single-value
        "BUILD_ARGS"  # Multi-value
        ${ARGN}
    )
    
    if(NOT ARGS_NAME)
        message(FATAL_ERROR "NAME argument required")
    endif()
    
    if(NOT ARGS_DOCKERFILE)
        set(ARGS_DOCKERFILE "${CMAKE_SOURCE_DIR}/Dockerfile")
    endif()
    
    if(NOT ARGS_TAG)
        set(ARGS_TAG "themisdb:${ARGS_NAME}")
    endif()
    
    if(NOT ARGS_BUILD_CONTEXT)
        set(ARGS_BUILD_CONTEXT "${CMAKE_SOURCE_DIR}")
    endif()
    
    # Build docker command
    set(_docker_cmd
        "${DOCKER_EXECUTABLE}" build
        -t "${ARGS_TAG}"
        -f "${ARGS_DOCKERFILE}"
    )
    
    # Add build args
    if(ARGS_BUILD_ARGS)
        foreach(_arg ${ARGS_BUILD_ARGS})
            list(APPEND _docker_cmd --build-arg "${_arg}")
        endforeach()
    endif()
    
    # Add build context
    list(APPEND _docker_cmd "${ARGS_BUILD_CONTEXT}")
    
    # Create custom target
    add_custom_target(${ARGS_NAME}
        COMMAND ${CMAKE_COMMAND} -E env DOCKER_BUILDKIT=1 ${_docker_cmd}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Building Docker image: ${ARGS_TAG}"
        VERBATIM
    )
    
    message(STATUS "Added Docker build target: ${ARGS_NAME} → ${ARGS_TAG}")
endfunction()

# ============================================================================
# Pre-defined Docker Build Targets
# ============================================================================

# Helper: Check if pre-built packages exist
function(check_prebuilt_packages TRIPLET CONFIG RESULT_VAR)
    set(_package_dir "${THEMIS_PACKAGE_STORE}/${TRIPLET}/${CONFIG}")
    if(EXISTS "${_package_dir}" AND IS_DIRECTORY "${_package_dir}")
        file(GLOB_RECURSE _files "${_package_dir}/*")
        list(LENGTH _files _file_count)
        if(_file_count GREATER 0)
            set(${RESULT_VAR} TRUE PARENT_SCOPE)
            return()
        endif()
    endif()
    set(${RESULT_VAR} FALSE PARENT_SCOPE)
endfunction()

# Target: docker-build-community-release
check_prebuilt_packages("x64-linux" "release" _has_linux_release)
if(_has_linux_release)
    add_docker_build_target(
        NAME docker-build-community-release
        DOCKERFILE "${CMAKE_SOURCE_DIR}/Dockerfile.prebuilt"
        TAG "themisdb:community-release"
        EDITION "COMMUNITY"
        CONFIGURATION "release"
        BUILD_ARGS
            "THEMIS_EDITION=COMMUNITY"
            "BUILD_TYPE=Release"
            "ENABLE_LLM=ON"
            "ENABLE_GPU=OFF"
    )
else()
    message(STATUS "Skipping docker-build-community-release (no pre-built packages)")
endif()

# Target: docker-build-community-debug
check_prebuilt_packages("x64-linux" "debug" _has_linux_debug)
if(_has_linux_debug)
    add_docker_build_target(
        NAME docker-build-community-debug
        DOCKERFILE "${CMAKE_SOURCE_DIR}/Dockerfile.prebuilt"
        TAG "themisdb:community-debug"
        EDITION "COMMUNITY"
        CONFIGURATION "debug"
        BUILD_ARGS
            "THEMIS_EDITION=COMMUNITY"
            "BUILD_TYPE=Debug"
            "ENABLE_LLM=ON"
            "ENABLE_GPU=OFF"
    )
else()
    message(STATUS "Skipping docker-build-community-debug (no pre-built packages)")
endif()

# Target: docker-build-minimal-release
add_docker_build_target(
    NAME docker-build-minimal-release
    DOCKERFILE "${CMAKE_SOURCE_DIR}/Dockerfile"
    TAG "themisdb:minimal-release"
    BUILD_ARGS
        "THEMIS_EDITION=MINIMAL"
        "CMAKE_BUILD_TYPE=Release"
        "ENABLE_LLM=OFF"
        "ENABLE_GPU=OFF"
)

# Target: docker-build-enterprise-release
if(_has_linux_release)
    add_docker_build_target(
        NAME docker-build-enterprise-release
        DOCKERFILE "${CMAKE_SOURCE_DIR}/Dockerfile.prebuilt"
        TAG "themisdb:enterprise-release"
        EDITION "ENTERPRISE"
        CONFIGURATION "release"
        BUILD_ARGS
            "THEMIS_EDITION=ENTERPRISE"
            "BUILD_TYPE=Release"
            "ENABLE_LLM=ON"
            "ENABLE_GPU=ON"
    )
endif()

# ============================================================================
# Docker Compose Targets
# ============================================================================

find_program(DOCKER_COMPOSE_EXECUTABLE docker-compose
    HINTS /usr/bin /usr/local/bin
    DOC "Docker Compose executable"
)

if(DOCKER_COMPOSE_EXECUTABLE)
    message(STATUS "Docker Compose found: ${DOCKER_COMPOSE_EXECUTABLE}")
    
    # Target: docker-compose-up
    add_custom_target(docker-compose-up
        COMMAND "${DOCKER_COMPOSE_EXECUTABLE}" up -d
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Starting Docker Compose services"
        VERBATIM
    )
    
    # Target: docker-compose-down
    add_custom_target(docker-compose-down
        COMMAND "${DOCKER_COMPOSE_EXECUTABLE}" down
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Stopping Docker Compose services"
        VERBATIM
    )
    
    # Target: docker-compose-logs
    add_custom_target(docker-compose-logs
        COMMAND "${DOCKER_COMPOSE_EXECUTABLE}" logs -f
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Following Docker Compose logs"
        VERBATIM
        USES_TERMINAL
    )
endif()

# ============================================================================
# Status Output
# ============================================================================

message(STATUS "==========================================")
message(STATUS "Docker Build System")
message(STATUS "==========================================")
message(STATUS "Docker: ${DOCKER_EXECUTABLE}")
if(DOCKER_COMPOSE_EXECUTABLE)
    message(STATUS "Docker Compose: ${DOCKER_COMPOSE_EXECUTABLE}")
endif()
message(STATUS "BuildKit: ${THEMIS_DOCKER_BUILDKIT}")
message(STATUS "")
message(STATUS "Available Docker targets:")
if(_has_linux_release)
    message(STATUS "  docker-build-community-release  - COMMUNITY Release (pre-built packages)")
endif()
if(_has_linux_debug)
    message(STATUS "  docker-build-community-debug    - COMMUNITY Debug (pre-built packages)")
endif()
message(STATUS "  docker-build-minimal-release    - MINIMAL Release")
if(_has_linux_release)
    message(STATUS "  docker-build-enterprise-release - ENTERPRISE Release (pre-built packages)")
endif()
if(DOCKER_COMPOSE_EXECUTABLE)
    message(STATUS "  docker-compose-up               - Start services")
    message(STATUS "  docker-compose-down             - Stop services")
    message(STATUS "  docker-compose-logs             - Follow logs")
endif()
message(STATUS "==========================================")
