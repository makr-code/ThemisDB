cmake_minimum_required(VERSION 3.20)

# Compatibility shim: runtime plugin implementation has been integrated at
# src/rpc_grpc. Keep this file for legacy paths under plugins/rpc/grpc.

if(NOT THEMIS_ROOT_DIR)
  get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(_rpc_src_dir "${THEMIS_ROOT_DIR}/src/rpc_grpc")

if(EXISTS "${_rpc_src_dir}/CMakeLists.txt")
  message(STATUS "plugins/rpc/grpc: delegating build to src/rpc_grpc")
  if(NOT TARGET themis_rpc_grpc)
    add_subdirectory("${_rpc_src_dir}" "${CMAKE_CURRENT_BINARY_DIR}/src_rpc_grpc")
  endif()
else()
  message(FATAL_ERROR "plugins/rpc/grpc: expected integrated source at ${_rpc_src_dir}")
endif()
