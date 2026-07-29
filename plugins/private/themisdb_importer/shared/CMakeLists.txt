add_library(themisdb_importer_shared STATIC
    src/schema_validator.cpp
    src/schema_inference.cpp
    src/data_quality.cpp
    src/audit_trail.cpp
    src/conflict_resolver.cpp
    src/canonical_resolver.cpp
    src/temporal_support.cpp
    src/crdt_importer.cpp
    src/adaptive_import.cpp
    src/polyglot_mapper.cpp
    src/entity_linker.cpp
)

target_include_directories(themisdb_importer_shared
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(themisdb_sdk_FOUND)
    target_link_libraries(themisdb_importer_shared PUBLIC themisdb_sdk::themisdb_sdk)
endif()

set_target_properties(themisdb_importer_shared PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
