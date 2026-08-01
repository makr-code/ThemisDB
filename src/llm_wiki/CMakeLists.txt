# LLM Wiki module - workspace state persistence and edition gating.
# Phase 3 implementation: WorkspaceStateManager + EditionGate.
#
# Provides: themis_llm_wiki (STATIC)
#   workspace_state_manager.cpp  - JSON state persistence with checksum validation
#   edition_gate.cpp             - Edition-aware access control

add_library(themis_llm_wiki STATIC
    workspace_state_manager.cpp
    edition_gate.cpp
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

if(MSVC)
    target_compile_options(themis_llm_wiki PRIVATE /W4)
else()
    target_compile_options(themis_llm_wiki PRIVATE -Wall -Wextra)
endif()
