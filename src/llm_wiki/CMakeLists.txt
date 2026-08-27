# LLM Wiki module - workspace state persistence and edition gating.
# Phase 3 implementation: WorkspaceStateManager + EditionGate.
#
# Provides: themis_llm_wiki (STATIC)
#   workspace_state_manager.cpp  - JSON state persistence with checksum validation
#   edition_gate.cpp             - Edition-aware access control

add_library(themis_llm_wiki STATIC
    workspace_state_manager.cpp
    edition_gate.cpp
    process_policy_manager.cpp
    wikipedia/wiki_workspace_orchestrator.cpp
    wikipedia/llm_wiki_plugin_impl.cpp
    wikipedia/llm_wiki_plugin_factory.cpp
    rocksdb_wiki_store.cpp
)

target_include_directories(themis_llm_wiki
    PUBLIC
        $<BUILD_INTERFACE:${THEMIS_ROOT_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(themis_llm_wiki
    PUBLIC
        themis_core
        spdlog::spdlog
)

# Wire RocksDB when available — activates the real backend (Wave-Next LW1).
if(TARGET rocksdb)
    target_compile_definitions(themis_llm_wiki PUBLIC THEMIS_USE_ROCKSDB=1)
    target_link_libraries(themis_llm_wiki PUBLIC rocksdb)
elseif(RocksDB_FOUND OR rocksdb_FOUND)
    target_compile_definitions(themis_llm_wiki PUBLIC THEMIS_USE_ROCKSDB=1)
    target_link_libraries(themis_llm_wiki PUBLIC ${RocksDB_LIBRARIES})
    target_include_directories(themis_llm_wiki PRIVATE ${RocksDB_INCLUDE_DIRS})
endif()

if(MSVC)
    target_compile_options(themis_llm_wiki PRIVATE /W4)
else()
    target_compile_options(themis_llm_wiki PRIVATE -Wall -Wextra)
endif()
