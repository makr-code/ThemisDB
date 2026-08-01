# LLM Wiki module - guardrails and plugin support infrastructure
# (Phase 3-4 implementation)

file(GLOB LLM_WIKI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Header-only libraries (guardrail_patterns.h is included by tests)
# No explicit target needed; included headers are available to dependent targets

# For now, this directory provides:
# - guardrail_patterns.h: Pattern matching for prompt injection detection
# - (Future Phase 4+) llm_wiki_plugin_impl.cpp when private plugin is ready
