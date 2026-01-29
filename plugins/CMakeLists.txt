# ThemisDB Enterprise Plugins
cmake_minimum_required(VERSION 3.20)

# Options for enterprise plugins
option(THEMIS_BUILD_ENTERPRISE_PLUGINS "Build enterprise plugins" ON)
option(THEMIS_BUILD_GPU_IMPACT_PLUGIN "Build GPU Impact Analysis Plugin" ON)
option(THEMIS_BUILD_RPC_FRAMEWORK "Build RPC Framework plugins" ON)
option(THEMIS_BUILD_ETHICS_AI_PLUGIN "Build Ethics AI Plugin" ON)

if(THEMIS_BUILD_ENTERPRISE_PLUGINS)
    message(STATUS "Building ThemisDB Enterprise Plugins")
    
    # GPU Impact Analysis Plugin
    if(THEMIS_BUILD_GPU_IMPACT_PLUGIN)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/enterprise/gpu_impact_analysis/CMakeLists.txt")
            message(STATUS "  - GPU Impact Analysis Plugin")
            add_subdirectory(enterprise/gpu_impact_analysis)
        else()
            message(WARNING "GPU Impact Analysis Plugin source not found at enterprise/gpu_impact_analysis")
        endif()
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

# Ethics AI Plugin (v1.4.0+)
if(THEMIS_BUILD_ETHICS_AI_PLUGIN)
    message(STATUS "Building Ethics AI Plugin")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ethics_ai")
        message(STATUS "  - Ethics AI Plugin (Native C++)")
        add_subdirectory(ethics_ai)
    endif()
endif()
