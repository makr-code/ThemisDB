# Miscellaneous Features and Utilities
# Plugin feedback system, graph buffering, HTTP adapters, sampling, and module loading

# Always-on miscellaneous sources
list(APPEND THEMIS_CORE_SOURCES
    ../src/index/graph_auto_buffer.cpp
    ../src/server/http_type_adapter.cpp
    ../src/base/module_loader.cpp
    ../src/base/module_sandbox.cpp
    ../src/base/hot_reload_manager.cpp
)

# LLM-adjacent features are only added when LLM is enabled
if(THEMIS_ENABLE_LLM)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/llm/feedback_plugin_basic.cpp
        ../src/llm/feedback_store.cpp
        ../src/llm/sampling_strategy.cpp
    )
endif()
