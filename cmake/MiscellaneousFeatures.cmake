# Miscellaneous Features and Utilities
# Plugin feedback system, graph buffering, HTTP adapters, sampling, and module loading

# Always-on miscellaneous sources
list(APPEND THEMIS_CORE_SOURCES
    ../src/index/graph_auto_buffer.cpp
    ../src/server/http_type_adapter.cpp
    # Module loader – migrated to src/themis/ (v1.7.0)
    ../src/themis/module_loader.cpp
    ../src/themis/module_loader_win32.cpp
    ../src/themis/module_loader_linux.cpp
    ../src/themis/module_security.cpp
    ../src/base/module_sandbox.cpp
    ../src/base/hot_reload_manager.cpp
    ../src/base/ab_test_manager.cpp
    ../src/base/wasm_plugin_sandbox.cpp
    ../src/base/remote_registry_client.cpp
    ../src/base/plugin_dependency_graph.cpp
    ../src/base/wasm_runtime_injector.cpp
    ../src/themis/module_dependency_resolver.cpp
    ../src/themis/module_hash_verifier.cpp
    ../src/themis/module_signature_verifier.cpp
    ../src/themis/edition_manager.cpp
)

# LLM-adjacent features are only added when LLM is enabled
if(THEMIS_ENABLE_LLM)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/llm/feedback_plugin_basic.cpp
        ../src/llm/feedback_store.cpp
        ../src/llm/sampling_strategy.cpp
    )
endif()
