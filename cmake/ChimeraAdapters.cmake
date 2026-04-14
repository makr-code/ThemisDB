# Chimera Adapter Framework Sources
# Vendor-neutral database adapter architecture for the CHIMERA benchmark suite.
# All adapters run in simulation mode (no external library dependencies).

set(THEMIS_CHIMERA_SOURCES
    # Chimera Adapter Factory and Reference Implementation
    ${CMAKE_SOURCE_DIR}/external/chimera/src/chimera/adapter_factory.cpp
    ${CMAKE_SOURCE_DIR}/src/chimera/themisdb_adapter.cpp
    # Relational / document adapters
    ${CMAKE_SOURCE_DIR}/external/chimera/src/chimera/mongodb_adapter.cpp
    ${CMAKE_SOURCE_DIR}/external/chimera/src/chimera/postgresql_adapter.cpp
    # Full-text / vector search adapters
    ${CMAKE_SOURCE_DIR}/external/chimera/src/chimera/elasticsearch_adapter.cpp
    ${CMAKE_SOURCE_DIR}/external/chimera/src/chimera/pinecone_adapter.cpp
    ${CMAKE_SOURCE_DIR}/external/chimera/src/chimera/qdrant_adapter.cpp
    ${CMAKE_SOURCE_DIR}/external/chimera/src/chimera/weaviate_adapter.cpp
    # Graph database adapter
    ${CMAKE_SOURCE_DIR}/external/chimera/src/chimera/neo4j_adapter.cpp
)

list(APPEND THEMIS_CORE_SOURCES ${THEMIS_CHIMERA_SOURCES})
