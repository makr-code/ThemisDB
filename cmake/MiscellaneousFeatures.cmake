# Miscellaneous Features and Utilities
# Plugin feedback system, graph buffering, HTTP adapters, sampling, and module loading

list(APPEND THEMIS_CORE_SOURCES
    # Feedback system for plugins
    ../src/llm/feedback_plugin_basic.cpp
    ../src/llm/feedback_store.cpp
    
    # Graph auto-buffering optimization
    ../src/index/graph_auto_buffer.cpp
    
    # HTTP type adapter for protocol handling
    ../src/server/http_type_adapter.cpp
    
    # Sampling strategies for data processing
    ../src/llm/sampling_strategy.cpp
    
    # Module loader for dynamic loading
    ../src/base/module_loader.cpp
)
