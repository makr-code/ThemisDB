# Chimera Adapter Framework Sources
# Vendor-neutral database adapter architecture for the CHIMERA benchmark suite.
# All adapters run in simulation mode (no external library dependencies).

list(APPEND THEMIS_CORE_SOURCES
    # Chimera Adapter Factory and Reference Implementation
    ../external/chimera/src/chimera/adapter_factory.cpp
    ../src/chimera/themisdb_adapter.cpp
    # Relational / document adapters
    ../external/chimera/src/chimera/mongodb_adapter.cpp
    ../external/chimera/src/chimera/postgresql_adapter.cpp
    # Full-text / vector search adapters
    ../external/chimera/src/chimera/elasticsearch_adapter.cpp
    ../external/chimera/src/chimera/pinecone_adapter.cpp
    ../external/chimera/src/chimera/qdrant_adapter.cpp
    ../external/chimera/src/chimera/weaviate_adapter.cpp
    # Graph database adapter
    ../external/chimera/src/chimera/neo4j_adapter.cpp
)
