set(DISTRIBUTED_TENSOR_PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/tensor_artifact_classes.h
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/artifact_manifest.h
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/shard_placement.h
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/integrity_verification.h
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/recovery_manager.h
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/distributed_planner.h
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/graph_summary_projection.h
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/tensor_training_coordinator.h
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/tensor_infrastructure.h
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/tensor_storage_strategy.h
)

set(DISTRIBUTED_TENSOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tensor_artifact_classes.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/artifact_manifest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/artifact_manifest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/shard_placement.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrity_verification.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/recovery_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/distributed_planner.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/graph_summary_projection.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/distributed_planner.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tensor_training_coordinator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tensor_infrastructure.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tensor_delta_log.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manifest_store.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crash_recovery_checkpoint.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tensor_storage_strategy.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/snapshot_update_worker.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shard_summary_coordinator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/artifact_invalidation.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/exact_graph_fallback.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stale_artifact_detector.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/error_recovery_handler.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/distributed_lock_manager.cc
)

add_library(themis_distributed_tensor STATIC ${DISTRIBUTED_TENSOR_SOURCES})
target_include_directories(themis_distributed_tensor
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(themis_distributed_tensor
    PRIVATE
        themis_base
)

# Link core tensor infrastructure if available.
# This provides ShardSummary, SummaryFreshnessState, and other tensor types.
if(TARGET themis_core)
    target_link_libraries(themis_distributed_tensor PRIVATE themis_core)
endif()

# If the tensor module is available, link it so consumers of
# themis_distributed_tensor automatically get tensor symbols (e.g. ShardSummary).
if(TARGET themis_tensor)
    target_link_libraries(themis_distributed_tensor PRIVATE themis_tensor)
endif()

if(OpenSSL_FOUND)
    target_link_libraries(themis_distributed_tensor PRIVATE OpenSSL::Crypto)
    target_compile_definitions(themis_distributed_tensor
        PRIVATE THEMIS_DISTRIBUTED_TENSOR_WITH_OPENSSL=1
    )
endif()

target_compile_features(themis_distributed_tensor PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(themis_distributed_tensor PRIVATE /W4)
else()
    target_compile_options(themis_distributed_tensor PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(FILES ${DISTRIBUTED_TENSOR_PUBLIC_HEADERS}
    DESTINATION include/distributed_tensor
)

# Keep the transport bridge separate from the core module so neither sharding
# nor distributed-tensor storage acquires a reverse dependency on the other.
if(TARGET themis_sharding AND TARGET themis_shard_proto AND THEMIS_ENABLE_GRPC)
    add_library(themis_distributed_tensor_shard_rpc_adapter STATIC
        shard_rpc_fragment_fetcher.cc
        local_projection_publisher.cc)
    target_include_directories(themis_distributed_tensor_shard_rpc_adapter
        PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(themis_distributed_tensor_shard_rpc_adapter
        PUBLIC themis_distributed_tensor themis_sharding
        PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
    target_compile_features(themis_distributed_tensor_shard_rpc_adapter
        PUBLIC cxx_std_20)
    if(MSVC)
        target_compile_options(themis_distributed_tensor_shard_rpc_adapter
            PRIVATE /W4)
    else()
        target_compile_options(themis_distributed_tensor_shard_rpc_adapter
            PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    install(FILES
        ${CMAKE_SOURCE_DIR}/include/distributed_tensor/shard_rpc_fragment_fetcher.h
        ${CMAKE_SOURCE_DIR}/include/distributed_tensor/local_projection_publisher.h
        DESTINATION include/distributed_tensor)
endif()
