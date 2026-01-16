# Edition-Specific Features
# Plugin system, sharding, and GPU memory management for specific editions

# Plugin system for enterprise
list(APPEND THEMIS_CORE_SOURCES
    ../src/plugins/plugin_system_edition.cpp
)

# Enterprise sharding manager
if(THEMIS_EDITION STREQUAL "ENTERPRISE" OR THEMIS_EDITION STREQUAL "HYPERSCALER")
    list(APPEND THEMIS_CORE_SOURCES
        ../src/sharding/sharding_manager_edition.cpp
    )
endif()

# GPU memory management for enterprise
if(THEMIS_ENABLE_GPU AND (THEMIS_EDITION STREQUAL "ENTERPRISE" OR THEMIS_EDITION STREQUAL "HYPERSCALER"))
    list(APPEND THEMIS_CORE_SOURCES
        ../src/gpu/gpu_memory_manager_edition.cpp
    )
endif()
