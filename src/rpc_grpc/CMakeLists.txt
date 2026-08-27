# gRPC RPC Plugin – src/rpc_grpc
# Implements IRPCServer (include/plugins/rpc_plugin_interface.h) using gRPC.
# Source and implementation header live together here.
# This is a runtime-loadable plugin, distinct from the built-in API gRPC server
# in src/api/grpc_server.cpp (themis::api namespace).

if(NOT THEMIS_PLUGIN_RPC_GRPC)
    return()
endif()

message(STATUS "Configuring gRPC RPC Plugin (src/rpc_grpc)")

find_package(gRPC CONFIG QUIET)
if(NOT gRPC_FOUND)
    find_package(gRPC QUIET)
endif()

if(NOT gRPC_FOUND)
    message(WARNING "gRPC not found – skipping gRPC plugin build")
    message(WARNING "Install with: apt install libgrpc++-dev  OR  vcpkg install grpc")
    return()
endif()
message(STATUS "  gRPC found: ${gRPC_VERSION}")

find_package(Protobuf CONFIG REQUIRED)
message(STATUS "  Protobuf found: ${Protobuf_VERSION}")

add_library(themis_rpc_grpc SHARED
    grpc_plugin.cpp
)

target_include_directories(themis_rpc_grpc
    PUBLIC
        $<BUILD_INTERFACE:${THEMIS_ROOT_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(themis_rpc_grpc PRIVATE
    gRPC::grpc++
    protobuf::libprotobuf
)

if(TARGET gRPC::grpc++_reflection)
    target_link_libraries(themis_rpc_grpc PRIVATE gRPC::grpc++_reflection)
else()
    message(WARNING "gRPC reflection target (gRPC::grpc++_reflection) not found; building themis_rpc_grpc without reflection")
endif()

set_target_properties(themis_rpc_grpc PROPERTIES
    OUTPUT_NAME "themis_rpc_grpc"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/rpc"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/rpc"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

if(WIN32)
    target_compile_definitions(themis_rpc_grpc PRIVATE THEMIS_PLUGIN_EXPORTS)
endif()

install(TARGETS themis_rpc_grpc
    LIBRARY DESTINATION lib/themis/plugins/rpc
    RUNTIME DESTINATION lib/themis/plugins/rpc
)

# Generate plugin manifest
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
    \"max_connections\": {\"type\": \"number\", \"default\": 1000},
    \"auth_required\": {\"type\": \"boolean\", \"default\": true}
  },
  \"auto_load\": true,
  \"load_priority\": 20
}
")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/plugin.json"
    DESTINATION lib/themis/plugins/rpc
)

message(STATUS "gRPC RPC Plugin configuration complete")
