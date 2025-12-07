# GPU Impact Analysis Plugin - CMake Build Configuration
cmake_minimum_required(VERSION 3.20)

# Don't create a separate project if we're included from parent
if(NOT PROJECT_NAME)
    project(ThemisGPUImpactAnalysis VERSION 1.0.0 LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(THEMIS_ENABLE_CUDA "Enable CUDA GPU acceleration" OFF)
option(THEMIS_ENABLE_VULKAN "Enable Vulkan GPU acceleration" OFF)
option(THEMIS_BUILD_PLUGIN_SHARED "Build as shared library (DLL/SO)" ON)

# Find dependencies
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Optional: CUDA support
if(THEMIS_ENABLE_CUDA)
    find_package(CUDAToolkit 11.0)
    if(CUDAToolkit_FOUND)
        enable_language(CUDA)
        add_compile_definitions(THEMIS_HAS_CUDA)
    endif()
endif()

# Optional: Vulkan support
if(THEMIS_ENABLE_VULKAN)
    find_package(Vulkan)
    if(Vulkan_FOUND)
        add_compile_definitions(THEMIS_HAS_VULKAN)
    endif()
endif()

# Source files
set(PLUGIN_SOURCES
    gpu_impact_analysis_plugin.cpp
)

set(PLUGIN_HEADERS
    ${CMAKE_SOURCE_DIR}/include/enterprise/gpu_impact_analysis_plugin.h
    ${CMAKE_SOURCE_DIR}/include/enterprise/analytics_plugins.h
)

# Build as shared library (Plugin DLL/SO)
if(THEMIS_BUILD_PLUGIN_SHARED)
    add_library(themis_gpu_impact_analysis SHARED 
        ${PLUGIN_SOURCES}
        ${PLUGIN_HEADERS}
    )
    
    # Export symbols
    target_compile_definitions(themis_gpu_impact_analysis PRIVATE
        THEMIS_ENTERPRISE_EXPORT
        THEMIS_PLUGIN_EXPORTS
    )
    
    # Platform-specific settings
    if(WIN32)
        set_target_properties(themis_gpu_impact_analysis PROPERTIES
            OUTPUT_NAME "themis_gpu_impact_analysis"
            PREFIX ""
            SUFFIX ".dll"
        )
    elseif(APPLE)
        set_target_properties(themis_gpu_impact_analysis PROPERTIES
            OUTPUT_NAME "themis_gpu_impact_analysis"
            PREFIX "lib"
            SUFFIX ".dylib"
        )
    else()
        set_target_properties(themis_gpu_impact_analysis PROPERTIES
            OUTPUT_NAME "themis_gpu_impact_analysis"
            PREFIX "lib"
            SUFFIX ".so"
        )
    endif()
    
else()
    # Build as static library
    add_library(themis_gpu_impact_analysis STATIC
        ${PLUGIN_SOURCES}
        ${PLUGIN_HEADERS}
    )
endif()

# Include directories
target_include_directories(themis_gpu_impact_analysis
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(themis_gpu_impact_analysis
    PRIVATE
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

# CUDA libraries
if(THEMIS_ENABLE_CUDA AND CUDAToolkit_FOUND)
    target_link_libraries(themis_gpu_impact_analysis
        PRIVATE
            CUDA::cudart
            CUDA::cufft
            CUDA::cublas
            CUDA::cusparse
            CUDA::curand
    )
endif()

# Vulkan libraries
if(THEMIS_ENABLE_VULKAN AND Vulkan_FOUND)
    target_link_libraries(themis_gpu_impact_analysis
        PRIVATE
            Vulkan::Vulkan
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(themis_gpu_impact_analysis PRIVATE /W4)
else()
    target_compile_options(themis_gpu_impact_analysis PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install rules
install(TARGETS themis_gpu_impact_analysis
    LIBRARY DESTINATION lib/themis/plugins/enterprise
    ARCHIVE DESTINATION lib/themis/plugins/enterprise
    RUNTIME DESTINATION bin/themis/plugins/enterprise
)

# Install configuration
install(FILES config.yaml
    DESTINATION share/themis/plugins/gpu_impact_analysis
)

# Install documentation
if(EXISTS ${CMAKE_SOURCE_DIR}/docs/enterprise/gpu_impact_analysis_plugin.md)
    install(FILES ${CMAKE_SOURCE_DIR}/docs/enterprise/gpu_impact_analysis_plugin.md
        DESTINATION share/doc/themis/plugins
    )
endif()

message(STATUS "GPU Impact Analysis Plugin configured")

