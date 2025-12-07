# ThemisDB Enterprise Plugins
cmake_minimum_required(VERSION 3.20)

# Options for enterprise plugins
option(THEMIS_BUILD_ENTERPRISE_PLUGINS "Build enterprise plugins" ON)
option(THEMIS_BUILD_GPU_IMPACT_PLUGIN "Build GPU Impact Analysis Plugin" ON)

if(THEMIS_BUILD_ENTERPRISE_PLUGINS)
    message(STATUS "Building ThemisDB Enterprise Plugins")
    
    # GPU Impact Analysis Plugin
    if(THEMIS_BUILD_GPU_IMPACT_PLUGIN)
        message(STATUS "  - GPU Impact Analysis Plugin")
        add_subdirectory(enterprise/gpu_impact_analysis)
    endif()
    
    # Future enterprise plugins can be added here
    # add_subdirectory(enterprise/other_plugin)
    
endif()
