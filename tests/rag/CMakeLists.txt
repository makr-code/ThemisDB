# RAG focused module tests (flat hierarchy under tests/rag)

file(GLOB RAG_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS RAG_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_rag_${_stem}_focused")
    set(_ctest "${_stem}_RagFocusedTests")

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    set(_extra_sources)
    if("${_stem}" STREQUAL "test_rag_adaptive_retrieval")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/adaptive_retrieval.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_rlaif_trainer")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/rlaif_trainer.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_multi_hop_reasoner")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/multi_hop_reasoner.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_replug_retriever")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/replug_retriever.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_context_assembler")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/rag_context_assembler.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_context_engine")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/ethics_ai/argument_store.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/rag_context_engine.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_ai_types.cpp
        )
    endif()
    if(_extra_sources)
        target_sources(${_target} PRIVATE ${_extra_sources})
    endif()
    if("${_stem}" STREQUAL "test_tensor_rag_provenance")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/query/tensor_aware_query_optimizer.cpp
            ${THEMIS_ROOT_DIR}/src/rag/targ_retrieval.cpp
            ${THEMIS_ROOT_DIR}/src/rag/flare_retrieval.cpp
            ${THEMIS_ROOT_DIR}/src/rag/tensor_rag_pipeline.cpp
        )
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE rag
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
endforeach()

# AUTOGEN PREFIX BLOCK: rag
file(GLOB RAG_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_rag_*.cpp"
)

foreach(_src IN LISTS RAG_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_rag_${_stem}_autofocused")
    set(_ctest "${_stem}_rag_AutoFocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    set(_extra_sources)
    if("${_stem}" STREQUAL "test_rag_adaptive_retrieval")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/adaptive_retrieval.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_rlaif_trainer")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/rlaif_trainer.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_multi_hop_reasoner")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/multi_hop_reasoner.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_replug_retriever")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/replug_retriever.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_context_assembler")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/rag_context_assembler.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_context_engine")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/ethics_ai/argument_store.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/rag_context_engine.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_ai_types.cpp
        )
    endif()
    if(_extra_sources)
        target_sources(${_target} PRIVATE ${_extra_sources})
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE rag
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS rag autogen
    )
endforeach()

# ─────────────────────────────────────────────────────────────────────────────
# Wave 5 Phase 2 RAG hardening test (explicit entry — not caught by GLOB when
# file is added after initial configure)
# ─────────────────────────────────────────────────────────────────────────────
set(_wave5_src "${CMAKE_CURRENT_SOURCE_DIR}/test_wave5_rag_hardening.cpp")
if(EXISTS "${_wave5_src}" AND NOT TARGET module_rag_test_wave5_rag_hardening_focused)
    add_executable(module_rag_test_wave5_rag_hardening_focused "${_wave5_src}")
    target_include_directories(module_rag_test_wave5_rag_hardening_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    # wiki_index_store.cpp is in themis_core when THEMIS_ENABLE_LLM=ON.
    # If not (community build without LLM), supply it directly.
    if(NOT THEMIS_ENABLE_LLM)
        target_sources(module_rag_test_wave5_rag_hardening_focused PRIVATE
            ${THEMIS_ROOT_DIR}/src/rag/wiki_index_store.cpp
            ${THEMIS_ROOT_DIR}/src/rag/rlaif_trainer.cpp
            ${THEMIS_ROOT_DIR}/src/rag/distributed_rag_evaluator.cpp
        )
    endif()
    target_link_libraries(module_rag_test_wave5_rag_hardening_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(module_rag_test_wave5_rag_hardening_focused PRIVATE
        THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(
        MODULE rag
        NAME test_wave5_rag_hardening_RagFocusedTests
        TARGET module_rag_test_wave5_rag_hardening_focused
        TIER unit
        TIMEOUT 120
    )
endif()