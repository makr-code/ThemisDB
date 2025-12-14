# Enterprise modules build configuration
# These modules are built as separate DLLs/shared libraries

cmake_minimum_required(VERSION 3.20)

# Build options for each enterprise module
option(THEMIS_ENTERPRISE_SHARDING "Build enterprise sharding module" OFF)
option(THEMIS_ENTERPRISE_GPU "Build enterprise GPU acceleration module" OFF)
option(THEMIS_ENTERPRISE_ANALYTICS "Build enterprise analytics module" OFF)
option(THEMIS_ENTERPRISE_REPLICATION "Build enterprise replication module" OFF)
option(THEMIS_ENTERPRISE_SECURITY "Build enterprise security module" OFF)
option(THEMIS_ENTERPRISE_MANAGEMENT "Build enterprise management module" OFF)
option(THEMIS_ENTERPRISE_CONTENT "Build enterprise content processors" OFF)

# Helper macro to build enterprise modules
macro(themis_add_enterprise_module MODULE_NAME SOURCE_FILES)
    # Create shared library
    add_library(themis_enterprise_${MODULE_NAME} SHARED ${SOURCE_FILES})
    
    # Set compile definition for DLL export
    target_compile_definitions(themis_enterprise_${MODULE_NAME} 
        PRIVATE THEMIS_ENTERPRISE_EXPORTS)
    
    # Include directories
    target_include_directories(themis_enterprise_${MODULE_NAME} 
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )
    
    # Link dependencies
    target_link_libraries(themis_enterprise_${MODULE_NAME}
        PRIVATE
            spdlog::spdlog
            nlohmann_json::nlohmann_json
    )
    
    # Set output directory
    set_target_properties(themis_enterprise_${MODULE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/enterprise
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/enterprise
    )
    
    # Install
    install(TARGETS themis_enterprise_${MODULE_NAME}
        RUNTIME DESTINATION bin/enterprise
        LIBRARY DESTINATION lib/enterprise
        ARCHIVE DESTINATION lib/enterprise
    )
    
    message(STATUS "Enterprise module enabled: ${MODULE_NAME}")
endmacro()

# Common plugin loader (always built if any enterprise module is enabled)
if(THEMIS_ENTERPRISE_SHARDING OR 
   THEMIS_ENTERPRISE_GPU OR 
   THEMIS_ENTERPRISE_ANALYTICS OR
   THEMIS_ENTERPRISE_REPLICATION OR
   THEMIS_ENTERPRISE_SECURITY OR
   THEMIS_ENTERPRISE_MANAGEMENT OR
   THEMIS_ENTERPRISE_CONTENT)
    
    add_library(themis_enterprise_common STATIC
        common/plugin_loader.cpp
    )
    
    target_include_directories(themis_enterprise_common
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    
    target_link_libraries(themis_enterprise_common
        PUBLIC
            spdlog::spdlog
            nlohmann_json::nlohmann_json
    )
    
    # Add platform-specific libraries for dynamic loading
    if(WIN32)
        # Windows uses LoadLibrary (no extra libs needed)
    else()
        target_link_libraries(themis_enterprise_common
            PUBLIC ${CMAKE_DL_LIBS}
        )
    endif()
endif()

# Build individual enterprise modules
if(THEMIS_ENTERPRISE_SHARDING)
    themis_add_enterprise_module(sharding
        sharding/sharding_plugin.cpp
    )
endif()

if(THEMIS_ENTERPRISE_GPU)
    themis_add_enterprise_module(gpu
        gpu/gpu_plugin.cpp
    )
    
    # Link GPU-specific dependencies
    if(THEMIS_ENABLE_CUDA)
        target_link_libraries(themis_enterprise_gpu PRIVATE CUDA::cudart)
    endif()
endif()

if(THEMIS_ENTERPRISE_ANALYTICS)
    themis_add_enterprise_module(analytics
        analytics/analytics_plugin.cpp
    )
    
    # Link Arrow for columnar analytics
    find_package(Arrow QUIET)
    if(Arrow_FOUND)
        target_link_libraries(themis_enterprise_analytics PRIVATE arrow_shared)
    endif()
endif()

if(THEMIS_ENTERPRISE_REPLICATION)
    themis_add_enterprise_module(replication
        replication/replication_plugin.cpp
    )
endif()

if(THEMIS_ENTERPRISE_SECURITY)
    themis_add_enterprise_module(security
        security/security_plugin.cpp
    )
    
    # Link OpenSSL for HSM integration
    target_link_libraries(themis_enterprise_security PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(THEMIS_ENTERPRISE_MANAGEMENT)
    themis_add_enterprise_module(management
        management/management_plugin.cpp
    )
endif()

if(THEMIS_ENTERPRISE_CONTENT)
    themis_add_enterprise_module(content
        content/content_plugin.cpp
    )
    
    # Link content processing dependencies
    # These would be found via vcpkg or system packages
    # find_package(poppler QUIET)  # PDF
    # find_package(FFmpeg QUIET)   # Video/Audio
    # find_package(GDAL QUIET)     # Geo
endif()

# Create convenience target to build all enterprise modules
if(THEMIS_ENTERPRISE_SHARDING OR 
   THEMIS_ENTERPRISE_GPU OR 
   THEMIS_ENTERPRISE_ANALYTICS OR
   THEMIS_ENTERPRISE_REPLICATION OR
   THEMIS_ENTERPRISE_SECURITY OR
   THEMIS_ENTERPRISE_MANAGEMENT OR
   THEMIS_ENTERPRISE_CONTENT)
    
    add_custom_target(themis_enterprise_all
        DEPENDS
            $<$<BOOL:${THEMIS_ENTERPRISE_SHARDING}>:themis_enterprise_sharding>
            $<$<BOOL:${THEMIS_ENTERPRISE_GPU}>:themis_enterprise_gpu>
            $<$<BOOL:${THEMIS_ENTERPRISE_ANALYTICS}>:themis_enterprise_analytics>
            $<$<BOOL:${THEMIS_ENTERPRISE_REPLICATION}>:themis_enterprise_replication>
            $<$<BOOL:${THEMIS_ENTERPRISE_SECURITY}>:themis_enterprise_security>
            $<$<BOOL:${THEMIS_ENTERPRISE_MANAGEMENT}>:themis_enterprise_management>
            $<$<BOOL:${THEMIS_ENTERPRISE_CONTENT}>:themis_enterprise_content>
    )
    
    message(STATUS "Enterprise build enabled - use 'themis_enterprise_all' target to build all modules")
endif()
