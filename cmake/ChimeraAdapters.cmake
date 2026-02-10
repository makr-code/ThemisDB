# Chimera Adapter Framework Sources
# Adapter lifecycle management for LLM integration

if(THEMIS_ENABLE_LLM)
    list(APPEND THEMIS_CORE_SOURCES
        # Chimera Adapter Factory and Lifecycle Management
        ../src/chimera/adapter_factory.cpp
        ../src/chimera/themisdb_adapter.cpp
    )
endif()
