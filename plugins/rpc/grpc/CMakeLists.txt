# gRPC Plugin for ThemisDB RPC Framework
# Part of ThemisDB v1.3.0

cmake_minimum_required(VERSION 3.20)

message(STATUS "Configuring gRPC Plugin")

# Find gRPC package
find_package(gRPC CONFIG)
if(NOT gRPC_FOUND)
    find_package(gRPC)
endif()

if(NOT gRPC_FOUND)
    message(WARNING "gRPC not found - skipping gRPC plugin build")
    message(WARNING "Install with: apt install libgrpc++-dev or brew install grpc or vcpkg install grpc")
    return()
endif()

message(STATUS "  gRPC found: ${gRPC_VERSION}")

# Find Protobuf (required by gRPC)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "  Protobuf found: ${Protobuf_VERSION}")

# Build gRPC plugin as shared library
add_library(themis_rpc_grpc SHARED
    grpc_plugin.cpp
    grpc_plugin.h
)

target_include_directories(themis_rpc_grpc PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(themis_rpc_grpc PRIVATE
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
)

# Set output directory
set_target_properties(themis_rpc_grpc PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/rpc"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/rpc"
    OUTPUT_NAME "themis_rpc_grpc"
)

# Install plugin
install(TARGETS themis_rpc_grpc
    LIBRARY DESTINATION lib/themis/plugins/rpc
    RUNTIME DESTINATION lib/themis/plugins/rpc
)

# Create plugin manifest
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/plugin.json"
"{
  \"name\": \"grpc\",
  \"version\": \"1.0.0\",
  \"type\": \"rpc_server\",
  \"description\": \"gRPC server plugin for ThemisDB\",
  \"binary\": {
    \"windows\": \"themis_rpc_grpc.dll\",
    \"linux\": \"themis_rpc_grpc.so\",
    \"macos\": \"themis_rpc_grpc.dylib\"
  },
  \"dependencies\": [
    \"grpc++ (>=1.50.0)\",
    \"protobuf (>=3.21.0)\"
  ],
  \"capabilities\": {
    \"streaming\": true,
    \"batching\": true,
    \"thread_safe\": true
  },
  \"config_schema\": {
    \"host\": {\"type\": \"string\", \"default\": \"0.0.0.0\"},
    \"port\": {\"type\": \"number\", \"default\": 50051},
    \"tls_enabled\": {\"type\": \"boolean\", \"default\": true},
    \"tls_cert_path\": {\"type\": \"string\"},
    \"tls_key_path\": {\"type\": \"string\"},
    \"tls_ca_cert_path\": {\"type\": \"string\"},
    \"max_connections\": {\"type\": \"number\", \"default\": 1000},
    \"auth_required\": {\"type\": \"boolean\", \"default\": true}
  },
  \"auto_load\": true,
  \"load_priority\": 20
}
")

# Install manifest
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/plugin.json"
    DESTINATION lib/themis/plugins/rpc
)

message(STATUS "gRPC Plugin configured successfully")
