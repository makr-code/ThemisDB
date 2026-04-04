# RPC and Service Implementation Sources
# Service layer implementations and RPC infrastructure

list(APPEND THEMIS_CORE_SOURCES
    # RPC service implementation
    ../src/server/rpc/rpc_service_impl.cpp
    
    # RPC service registry for service discovery
    ../src/plugins/rpc_service_registry.cpp
)
