# src/distributed_knowledge/CMakeLists.txt
#
# Library target: themis_distributed_knowledge
# Version: Session 1 (2026-04-19)
#
# Sources: 4 implementation files for the Distributed Knowledge Federation module.
# Dependencies: nlohmann_json (header-only), themis_core (for governance + llm headers).

cmake_minimum_required(VERSION 3.16)

add_library(themis_distributed_knowledge
    adapter_capability_announcement.cpp
    lora_federation_coordinator.cpp
    federated_rag_merger.cpp
    cross_shard_feedback_sync.cpp
    ../governance/cross_border_transfer.cpp
    ../llm/decision_record_yaml_processor.cpp
)

target_include_directories(themis_distributed_knowledge PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(themis_distributed_knowledge
    PUBLIC
        nlohmann_json::nlohmann_json
        Threads::Threads
)

if(TARGET fmt::fmt)
    target_link_libraries(themis_distributed_knowledge PRIVATE fmt::fmt)
endif()

if(TARGET yaml-cpp::yaml-cpp)
    target_link_libraries(themis_distributed_knowledge PRIVATE yaml-cpp::yaml-cpp)
elseif(TARGET yaml-cpp)
    target_link_libraries(themis_distributed_knowledge PRIVATE yaml-cpp)
endif()

if(TARGET themis_core)
    target_link_libraries(themis_distributed_knowledge PRIVATE themis_core)
endif()

# Resolve symbols used by distributed federation flows:
# - governance::CrossBorderTransferPolicy (themis_security)
# DecisionRecordYamlProcessor is compiled in directly above to avoid pulling
# full themis_llm (which introduces duplicate distributed_knowledge symbols).
if(TARGET themis_security)
    target_link_libraries(themis_distributed_knowledge PRIVATE themis_security)
endif()

target_compile_features(themis_distributed_knowledge PUBLIC cxx_std_17)

set_target_properties(themis_distributed_knowledge PROPERTIES
    CXX_STANDARD          17
    CXX_STANDARD_REQUIRED ON
)
