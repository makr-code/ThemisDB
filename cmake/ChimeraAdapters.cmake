# Chimera Adapter Framework Sources
# Vendor-neutral database adapter architecture for the CHIMERA benchmark suite.
# All adapters run in simulation mode (no external library dependencies).

list(APPEND THEMIS_CORE_SOURCES
    # Chimera Adapter Factory and Reference Implementation
    ../src/chimera/adapter_factory.cpp
    ../src/chimera/themisdb_adapter.cpp
    # Relational / document adapters
    ../src/chimera/mongodb_adapter.cpp
    ../src/chimera/postgresql_adapter.cpp
    # Full-text / vector search adapters
    ../src/chimera/elasticsearch_adapter.cpp
    ../src/chimera/pinecone_adapter.cpp
    ../src/chimera/qdrant_adapter.cpp
    ../src/chimera/weaviate_adapter.cpp
    # Graph database adapter
    ../src/chimera/neo4j_adapter.cpp
)
