# GPU Impact Analysis Plugin - CMake Build Configuration
cmake_minimum_required(VERSION 3.20)

project(ThemisGPUImpactAnalysis VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(THEMIS_ENABLE_CUDA "Enable CUDA GPU acceleration" ON)
option(THEMIS_ENABLE_VULKAN "Enable Vulkan GPU acceleration" ON)
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
    ../../../include/enterprise/gpu_impact_analysis_plugin.h
    ../../../include/enterprise/analytics_plugins.h
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
    
    # Set output name
    set_target_properties(themis_gpu_impact_analysis PROPERTIES
        OUTPUT_NAME "themis_gpu_impact_analysis"
        PREFIX ""  # No 'lib' prefix on Windows
        SUFFIX ".dll"  # Or .so on Linux
    )
    
    # Platform-specific settings
    if(WIN32)
        set_target_properties(themis_gpu_impact_analysis PROPERTIES
            SUFFIX ".dll"
        )
    elseif(APPLE)
        set_target_properties(themis_gpu_impact_analysis PROPERTIES
            SUFFIX ".dylib"
        )
    else()
        set_target_properties(themis_gpu_impact_analysis PROPERTIES
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
    target_compile_options(themis_gpu_impact_analysis PRIVATE /W4 /WX)
else()
    target_compile_options(themis_gpu_impact_analysis PRIVATE -Wall -Wextra -Wpedantic -Werror)
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
install(FILES ../../../docs/enterprise/gpu_impact_analysis_plugin.md
    DESTINATION share/doc/themis/plugins
)

# Testing
if(BUILD_TESTING)
    enable_testing()
    
    # TODO: Add unit tests
    # add_subdirectory(tests)
endif()

# Package information
set(CPACK_PACKAGE_NAME "themis-gpu-impact-analysis-plugin")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "ThemisDB Enterprise")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GPU-accelerated impact analysis plugin for ThemisDB")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

include(CPack)
