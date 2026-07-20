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
    if("${_stem}" STREQUAL "test_tensor_rag_provenance")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/query/tensor_aware_query_optimizer.cpp
            ${THEMIS_ROOT_DIR}/src/rag/targ_retrieval.cpp
            ${THEMIS_ROOT_DIR}/src/rag/flare_retrieval.cpp
            ${THEMIS_ROOT_DIR}/src/rag/tensor_rag_pipeline.cpp
        )
    endif()
    if("${_stem}" STREQUAL "test_rag_rlaif_trainer")
        # Compile the RLAIF trainer implementation into an OBJECT library
        # and link its objects into the focused test. This guarantees the
        # definitions (HeuristicAIJudge, RLAIFTrainer, etc.) are present
        # for the test executable's link step.
        add_library(rag_rlaif_impl_objs OBJECT
            ${THEMIS_ROOT_DIR}/src/rag/rlaif_trainer.cpp
        )
        target_include_directories(rag_rlaif_impl_objs PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )
        # Match the same compile-time defines used across the project so the
        # object compilation does not instantiate header-only implementations
        # (e.g., spdlog header vs compiled library mismatch).
        target_compile_definitions(rag_rlaif_impl_objs PRIVATE
            THEMIS_TEST_BUILD=1
            SPDLOG_COMPILED_LIB
            SPDLOG_SHARED_LIB
            SPDLOG_FMT_EXTERNAL
        )
        target_link_libraries(${_target} PRIVATE $<TARGET_OBJECTS:rag_rlaif_impl_objs>)
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    if(TARGET themis_llm)
        target_link_libraries(${_target} PRIVATE themis_llm)
    endif()
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