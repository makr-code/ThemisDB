# ThemisDB Enterprise Plugins
cmake_minimum_required(VERSION 3.20)

# Options for enterprise plugins
option(THEMIS_BUILD_ENTERPRISE_PLUGINS "Build enterprise plugins" ON)
option(THEMIS_BUILD_GPU_IMPACT_PLUGIN "Build GPU Impact Analysis Plugin" ON)
option(THEMIS_BUILD_RPC_FRAMEWORK "Build RPC Framework plugins" ON)

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

# RPC Framework plugins (v1.3.0+)
if(THEMIS_BUILD_RPC_FRAMEWORK)
    message(STATUS "Building RPC Framework Plugins")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rpc/grpc")
        message(STATUS "  - gRPC Plugin")
        add_subdirectory(rpc/grpc)
    endif()
endif()
