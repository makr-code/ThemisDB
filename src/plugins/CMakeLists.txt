# ThemisDB Plugin Implementations – src/plugins
# This file is included from cmake/CMakeLists.txt when
# THEMIS_BUILD_ENTERPRISE_PLUGINS is ON.
#
# Each sub-plugin is guarded by its own feature flag defined in
# cmake/features/PluginFeatures.cmake, allowing fine-grained
# compile-time enable/disable via  -DTHEMIS_PLUGIN_<NAME>=ON|OFF.

cmake_minimum_required(VERSION 3.20)

# THEMIS_ROOT_DIR must be set by the parent (cmake/CMakeLists.txt)
if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

message(STATUS "Building ThemisDB Plugin Implementations (src/plugins)")

# -------------------------------------------------------------------------
# Ethics AI Plugin (v1.4.0+)
# Flag: THEMIS_PLUGIN_ETHICS_AI
# -------------------------------------------------------------------------
if(THEMIS_PLUGIN_ETHICS_AI)
    add_subdirectory(ethics_ai)
endif()

# -------------------------------------------------------------------------
# gRPC RPC Plugin (v1.3.0+)
# Flag: THEMIS_PLUGIN_RPC_GRPC
# -------------------------------------------------------------------------
if(THEMIS_PLUGIN_RPC_GRPC)
    add_subdirectory(rpc/grpc)
endif()

# -------------------------------------------------------------------------
# Image Analysis – ONNX CLIP Plugin
# Flag: THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX
# -------------------------------------------------------------------------
if(THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX)
    add_subdirectory(image_analysis)
endif()

# -------------------------------------------------------------------------
# User Storage Encrypted Plugin (v1.5.0+)
# Flag: THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED
# -------------------------------------------------------------------------
if(THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED)
    add_subdirectory(user_storage_encrypted)
endif()
