# ThemisDB Source Code Directory Guide

**Version:** 1.8.0-rc1  
**Last Updated:** 2026-04-06  
**Status:** Complete

---

## Overview

This document provides a comprehensive guide to all **35 directories** in the `src/` folder of ThemisDB. It serves as a roadmap for developers to understand the codebase organization, locate specific functionality, and determine where to add new code.

### Organization Philosophy

The ThemisDB source code is organized into functionally-cohesive modules that represent distinct layers and capabilities of the database system:

- **Core Infrastructure** - Base utilities, networking, and common functionality
- **Storage Layer** - Data persistence, caching, and transactions
- **Query & Processing** - AQL parser, query execution, and data processing
- **Index & Search** - Vector, graph, spatial, and full-text indexing
- **Server & API** - HTTP server, protocol handlers, and API endpoints
- **Security & Auth** - Encryption, authentication, and access control
- **LLM & AI** - Language models, embeddings, and AI features
- **Distributed Systems** - Sharding, replication, and coordination
- **Observability** - Metrics, monitoring, and governance
- **Data Integration** - Import/export, CDC, and content processing

---

## Directory Structure

### Core Infrastructure

#### src/base/
**Purpose:** Base classes, common types, and foundational utilities used across the entire codebase.

**Key Files:**
- `module_loader.cpp` - Dynamic module loading system

**Dependencies:** None (foundation layer)

**Feature Flags:** None (always compiled)

**Documentation:**
- Core infrastructure is documented throughout the codebase

**Example Usage:**
```cpp
// Foundation classes and types used by all other modules
#include "base/types.h"
```

---

#### src/utils/
**Purpose:** Utility functions, helpers, and cross-cutting concerns used throughout ThemisDB.

**Key Files:** (29 files total)
- `audit_logger.cpp` - Audit logging functionality
- `build_info.cpp` - Build information and version tracking
- `cursor.cpp` - Database cursor implementation
- `error_codes.cpp` - Error code management
- `json_utils.cpp` - JSON manipulation utilities
- `logging.cpp` - Logging infrastructure
- `string_utils.cpp` - String manipulation helpers
- `time_utils.cpp` - Time and date utilities

**Dependencies:**
- src/base/ - Base types and classes

**Feature Flags:** None (always compiled)

**Documentation:**
- [Utilities README](../../src/utils/README.md)

**Example Usage:**
```cpp
#include "utils/logging.h"
#include "utils/json_utils.h"
#include "utils/error_codes.h"

Logger::info("Operation completed successfully");
```

---

#### src/network/
**Purpose:** Network communication layer and wire protocol implementations.

**Key Files:**
- `wire_protocol_server.cpp` - ThemisDB wire protocol server
- `themis_wire_v1.proto` - Protocol buffer definitions for wire protocol

**Dependencies:**
- src/base/
- src/utils/

**Feature Flags:** None (core networking)

**Documentation:**
- [Wire Protocol Documentation](../../docs/network/)

**Example Usage:**
```cpp
#include "network/wire_protocol_server.h"

// Network protocol implementation
WireProtocolServer server(port);
server.start();
```

---

### Storage Layer

#### src/storage/
**Purpose:** Core storage engine - RocksDB wrapper, key-value storage, blob storage backends.

**Key Files:** (14 files total)
- `base_entity.cpp` - BaseEntity storage pattern (fundamental ThemisDB concept)
- `backup_manager.cpp` - Backup and restore functionality
- `blob_backend_azure.cpp` - Azure Blob Storage backend
- `blob_backend_filesystem.cpp` - Filesystem blob storage
- `blob_backend_s3.cpp` - AWS S3 blob storage backend
- `rocks_wrapper.cpp` - RocksDB abstraction layer
- `snapshot_isolation.cpp` - Snapshot isolation implementation
- `storage_engine.cpp` - Main storage engine

**Dependencies:**
- src/base/
- src/utils/
- RocksDB (external)

**Feature Flags:**
- Various blob storage backends may have feature flags

**Documentation:**
- [Storage README](../../src/storage/README.md)
- [BaseEntity Principle](../../BASEENTITY_PRINCIPLE.md)
- [Storage Architecture](../../docs/architecture/)

**Example Usage:**
```cpp
#include "storage/storage_engine.h"
#include "storage/base_entity.h"

StorageEngine engine;
BaseEntity entity;
engine.put(key, entity.serialize());
```

---

#### src/cache/
**Purpose:** Caching implementations including semantic cache and embedding cache.

**Key Files:**
- `embedding_cache.cpp` - Cache for vector embeddings
- `semantic_cache.cpp` - Semantic-aware query result caching

**Dependencies:**
- src/storage/ - For persistent cache backing
- src/index/ - For vector similarity in semantic cache

**Feature Flags:** None (core feature)

**Documentation:**
- [Cache README](../../src/cache/README.md)
- [Semantic Cache Documentation](../../docs/semantic_cache.md)

**Example Usage:**
```cpp
#include "cache/semantic_cache.h"

SemanticCache cache;
auto result = cache.lookup(query_embedding);
if (!result) {
    result = execute_query(query);
    cache.store(query_embedding, result);
}
```

---

#### src/transaction/
**Purpose:** Transaction management, MVCC, snapshot isolation, and SAGA patterns.

**Key Files:**
- `transaction_manager.cpp` - Main transaction coordinator
- `snapshot_manager.cpp` - Snapshot isolation management
- `saga.cpp` - SAGA distributed transaction pattern

**Dependencies:**
- src/storage/ - Storage layer integration
- src/utils/ - Logging and error handling

**Feature Flags:** None (core feature)

**Documentation:**
- [Transaction README](../../src/transaction/README.md)
- [SAGA Pattern Documentation](../../docs/saga/)

**Example Usage:**
```cpp
#include "transaction/transaction_manager.h"

TransactionManager tm;
auto txn = tm.begin();
// ... perform operations ...
tm.commit(txn);
```

---

#### src/replication/
**Purpose:** Data replication logic for high availability and disaster recovery.

**Key Files:**
- `replication_manager.cpp` - Replication coordination

**Dependencies:**
- src/storage/ - Data access
- src/network/ - Network communication
- src/sharding/ - Distributed coordination

**Feature Flags:** None (core feature)

**Documentation:**
- [Replication Documentation](../../docs/replication/)

**Example Usage:**
```cpp
#include "replication/replication_manager.h"

ReplicationManager repl;
repl.replicate(data, target_nodes);
```

---

### Query & Processing

#### src/query/
**Purpose:** AQL (Analytics Query Language) parser, query engine, and execution.

**Key Files:** (14 files total)
- `aql_parser.cpp` - Main AQL parser
- `aql_parser_json.cpp` - JSON query format parser
- `aql_runner.cpp` - Query execution engine
- `aql_translator.cpp` - Query translation and optimization
- `aql_validator.cpp` - Query validation
- `query_optimizer.cpp` - Query optimization
- `query_planner.cpp` - Query plan generation

**Dependencies:**
- src/storage/ - Data access
- src/index/ - Index utilization
- src/utils/ - Parsing utilities

**Feature Flags:** None (core feature)

**Documentation:**
- [Query README](../../src/query/README.md)
- [AQL Reference](../../LORA_AQL_REFERENCE.md)
- [Query Engine Documentation](../../docs/query/)

**Example Usage:**
```cpp
#include "query/aql_parser.h"
#include "query/aql_runner.h"

AQLParser parser;
auto query = parser.parse("FOR doc IN users FILTER doc.age > 21 RETURN doc");
AQLRunner runner;
auto result = runner.execute(query);
```

---

#### src/aql/
**Purpose:** AQL-specific functionality including LLM integration and documentation assistance.

**Key Files:**
- `llm_aql_handler.cpp` - LLM-powered AQL query assistance
- `docs_assistant_functions.cpp` - Documentation assistant functions

**Dependencies:**
- src/query/ - AQL parsing and execution
- src/llm/ - LLM integration

**Feature Flags:**
- `THEMIS_ENABLE_LLM` - Required for LLM-powered features

**Documentation:**
- [AQL LLM Integration](../../docs/llm/)

**Example Usage:**
```cpp
#include "aql/llm_aql_handler.h"

LLMAQLHandler handler;
auto suggested_query = handler.suggest_query("find all users over 21");
```

---

#### src/analytics/
**Purpose:** Real-time analytics, OLAP, metrics aggregation, and process mining.

**Key Files:**
- `olap.cpp` - OLAP query processing
- `diff_engine.cpp` - Data difference analysis
- `llm_process_analyzer.cpp` - LLM interaction process analysis
- `nlp_text_analyzer.cpp` - Natural language text analysis
- `process_mining.cpp` - Process mining functionality

**Dependencies:**
- src/storage/ - Data access
- src/query/ - Query execution
- src/llm/ - LLM analytics

**Feature Flags:**
- `THEMIS_ENABLE_LLM` - For LLM-related analytics

**Documentation:**
- [Analytics Guide](../../docs/features/analytics.md)

**Example Usage:**
```cpp
#include "analytics/olap.h"

OLAPEngine engine;
auto stats = engine.aggregate(dataset, dimensions);
```

---

### Index & Search

#### src/index/
**Purpose:** Vector indexes, graph indexes, spatial indexes, and advanced indexing structures.

**Key Files:** (16 files total)
- `adaptive_index.cpp` - Adaptive index selection
- `advanced_vector_index.cpp` - Advanced vector similarity search
- `edge_types.cpp` - Graph edge type definitions
- `gnn_embeddings.cpp` - Graph neural network embeddings
- `graph_index.cpp` - Graph indexing
- `hnsw_index.cpp` - Hierarchical Navigable Small World index
- `spatial_index.cpp` - Geospatial indexing
- `vector_index.cpp` - Basic vector index

**Dependencies:**
- src/storage/ - Data persistence
- src/acceleration/ - GPU acceleration for vector ops

**Feature Flags:**
- `THEMIS_ENABLE_DISKANN` - DiskANN vector index
- `THEMIS_ENABLE_RABITQ` - RaBitQ quantization

**Documentation:**
- [Index README](../../src/index/README.md)
- [Vector Index Documentation](../../docs/indexes/vector.md)
- [Graph Index Documentation](../../docs/indexes/graph.md)

**Example Usage:**
```cpp
#include "index/vector_index.h"

VectorIndex index(dimensions);
index.add(id, embedding);
auto results = index.search(query_embedding, k);
```

---

#### src/search/
**Purpose:** Hybrid search combining full-text, vector, and graph search.

**Key Files:**
- `hybrid_search.cpp` - Unified hybrid search implementation

**Dependencies:**
- src/index/ - Various index types
- src/query/ - Query integration

**Feature Flags:** None (core feature)

**Documentation:**
- [Hybrid Search Documentation](../../docs/search/)

**Example Usage:**
```cpp
#include "search/hybrid_search.h"

HybridSearch search;
auto results = search.query(text_query, vector_query, graph_constraints);
```

---

### Server & API

#### src/server/
**Purpose:** HTTP server implementation and API endpoint handlers.

**Key Files:** (58 files total)
- `http_server.cpp` - Main HTTP server
- `admin_api_handler.cpp` - Administrative API endpoints
- `audit_api_handler.cpp` - Audit log API
- `auth_middleware.cpp` - Authentication middleware
- `classification_api_handler.cpp` - Data classification API
- `keys_api_handler.cpp` - Key management API
- `pii_api_handler.cpp` - PII detection/masking API
- `policy_engine.cpp` - Policy enforcement engine
- `saga_api_handler.cpp` - SAGA transaction API
- `sse_connection_manager.cpp` - Server-Sent Events manager

**Dependencies:**
- src/storage/ - Data access
- src/query/ - Query execution
- src/auth/ - Authentication
- src/security/ - Security features

**Feature Flags:**
- `THEMIS_ENABLE_SSE` - Server-Sent Events
- `THEMIS_ENABLE_WEBSOCKET` - WebSocket support

**Documentation:**
- [Server README](../../src/server/README.md)
- [HTTP API Documentation](../../docs/api/)
- [API Reference](../api/API_REFERENCE.md)

**Example Usage:**
```cpp
#include "server/http_server.h"

HTTPServer server(8080);
server.register_handler("/api/query", query_handler);
server.start();
```

---

#### src/api/
**Purpose:** High-level API implementations including GraphQL and geo-indexing hooks.

**Key Files:**
- `http_server.cpp` - HTTP server implementation
- `graphql.cpp` - GraphQL query interface
- `geo_index_hooks.cpp` - Geospatial index integration hooks

**Dependencies:**
- src/server/ - Server infrastructure
- src/query/ - Query processing
- src/geo/ - Geospatial functionality

**Feature Flags:**
- `THEMIS_ENABLE_GRAPHQL` - GraphQL API (ON by default)

**Documentation:**
- [API README](../../src/api/README.md)
- [GraphQL Schema](../../docs/api/graphql_schema.md)

**Example Usage:**
```cpp
#include "api/graphql.h"

GraphQLEngine engine;
auto result = engine.execute(graphql_query);
```

---

### Security & Auth

#### src/security/
**Purpose:** Encryption, Hardware Security Modules (HSM), PKI, and security features.

**Key Files:** (18 files total)
- `field_encryption.cpp` - Field-level encryption
- `encrypted_field.cpp` - Encrypted field implementation
- `hsm_provider.cpp` - HSM integration
- `cms_signing.cpp` - CMS/PKCS#7 signing
- `key_manager.cpp` - Key management system
- `pki_manager.cpp` - Public Key Infrastructure
- `tls_config.cpp` - TLS configuration

**Dependencies:**
- src/storage/ - Encrypted storage
- src/utils/ - Crypto utilities

**Feature Flags:**
- `THEMIS_ENABLE_HSM_REAL` - Real HSM provider (vs. mock)

**Documentation:**
- [Security README](../../src/security/README.md)
- [Security Documentation](../../docs/security/)
- [SECURITY.md](../../SECURITY.md)

**Example Usage:**
```cpp
#include "security/field_encryption.h"

FieldEncryption enc(key);
auto encrypted = enc.encrypt(plaintext);
auto decrypted = enc.decrypt(encrypted);
```

---

#### src/auth/
**Purpose:** Authentication mechanisms including JWT, Kerberos/GSSAPI.

**Key Files:**
- `jwt_validator.cpp` - JWT token validation
- `gssapi_authenticator.cpp` - Kerberos/GSSAPI authentication

**Dependencies:**
- src/security/ - Crypto primitives
- src/utils/ - Utilities

**Feature Flags:**
- `THEMIS_ENABLE_KERBEROS` - Kerberos/GSSAPI support

**Documentation:**
- [Auth README](../../src/auth/README.md)
- [Authentication Guide](../../docs/security/authentication.md)
- [Kerberos Implementation](../../KERBEROS_IMPLEMENTATION_SUMMARY.md)

**Example Usage:**
```cpp
#include "auth/jwt_validator.h"

JWTValidator validator(secret);
if (validator.validate(token)) {
    // Token is valid
}
```

---

#### src/governance/
**Purpose:** Data governance, policy management, and compliance.

**Key Files:**
- `policy_engine.cpp` - Policy enforcement engine

**Dependencies:**
- src/storage/ - Data access
- src/security/ - Security integration

**Feature Flags:** None (enterprise feature)

**Documentation:**
- [Governance README](../../src/governance/README.md)
- [Data Governance Guide](../../docs/governance/)

**Example Usage:**
```cpp
#include "governance/policy_engine.h"

PolicyEngine engine;
engine.enforce_policy(data, policy_rules);
```

---

### LLM & AI

#### src/llm/
**Purpose:** Large Language Model integration, inference, embeddings, and AI features.

**Key Files:** (37 files total)
- `ai_decision_auditor.cpp` - AI decision auditing
- `async_inference_engine.cpp` - Asynchronous LLM inference
- `block_table.cpp` - Memory block management for paged attention
- `embedding_handler.cpp` - Embedding generation
- `grammar_checker.cpp` - Grammar-based output validation
- `llama_plugin.cpp` - LLaMA model integration
- `lora_manager.cpp` - LoRA adapter management
- `multimodal_processor.cpp` - Multimodal input processing
- `paged_attention.cpp` - Paged attention mechanism
- `prefix_cache.cpp` - KV cache prefix caching
- `vision_processor.cpp` - Vision model processing (LLaVA)

**Dependencies:**
- src/storage/ - Model and cache storage
- src/acceleration/ - GPU acceleration
- src/cache/ - Embedding cache
- llama.cpp (external)

**Feature Flags:**
- `THEMIS_ENABLE_LLM` - Enable LLM features
- `THEMIS_ENABLE_VISION` - Enable vision/multimodal support
- `THEMIS_ENABLE_GPU` - GPU acceleration for inference

**Documentation:**
- [LLM README](../../src/llm/README.md)
- [LLM Documentation Hub](../../docs/llm/)
- [LoRA Guide](../../LORA_MULTIMODEL_GUIDE.md)
- [LLM Build Guide](../../LORA_BUILD_GUIDE.md)

**Example Usage:**
```cpp
#include "llm/async_inference_engine.h"

AsyncInferenceEngine engine;
auto future = engine.generate_async(prompt);
auto response = future.get();
```

---

#### src/acceleration/
**Purpose:** GPU and hardware acceleration abstraction layer for compute-intensive operations.

**Key Files:** (18 files total)
- `backend_registry.cpp` - Backend registration system
- `cpu_backend.cpp` - CPU fallback implementation
- `cpu_backend_mt.cpp` - Multi-threaded CPU backend
- `cpu_backend_tbb.cpp` - Intel TBB CPU backend
- `cuda_backend.cpp` - NVIDIA CUDA backend
- `vulkan_backend.cpp` - Vulkan compute backend
- `hip_backend.cpp` - AMD HIP backend

**Dependencies:**
- src/base/ - Base types
- CUDA, Vulkan, HIP (external, optional)

**Feature Flags:**
- `THEMIS_ENABLE_GPU` - Enable GPU acceleration
- `THEMIS_ENABLE_CUDA` - NVIDIA CUDA backend
- `THEMIS_ENABLE_HIP` - AMD HIP backend

**Documentation:**
- [Acceleration README](../../src/acceleration/README.md)
- [GPU Acceleration Guide](../../docs/performance/GPU_ACCELERATION_PLAN.md)
- [CUDA Backend](../../docs/performance/CUDA_BACKEND.md)

**Example Usage:**
```cpp
#include "acceleration/backend_registry.h"

auto backend = BackendRegistry::get_instance().get_backend("cuda");
backend->vector_similarity(vectors, query);
```

---

#### src/gpu/
**Purpose:** GPU memory management specific to edition-based resource control.

**Key Files:**
- `gpu_memory_manager_edition.cpp` - Edition-aware GPU memory allocation

**Dependencies:**
- src/acceleration/ - GPU backends
- src/llm/ - LLM memory requirements

**Feature Flags:**
- `THEMIS_ENABLE_GPU` - Required

**Documentation:**
- [GPU Memory Management](../../docs/performance/)

**Example Usage:**
```cpp
#include "gpu/gpu_memory_manager_edition.h"

GPUMemoryManager mgr;
auto allocation = mgr.allocate(size, edition_limits);
```

---

### Distributed Systems

#### src/sharding/
**Purpose:** Distributed sharding, cluster coordination, and RAID-style data distribution.

**Key Files:** (44 files total)
- `admin_api.cpp` - Shard administration API
- `auto_rebalancer.cpp` - Automatic shard rebalancing
- `circuit_breaker.cpp` - Circuit breaker pattern
- `cloud_agent.cpp` - Cloud provider integration
- `coordinator.cpp` - Cluster coordination
- `gossip_protocol.cpp` - Gossip-based cluster communication
- `raid_orchestrator.cpp` - RAID orchestration
- `shard_manager.cpp` - Shard lifecycle management
- `shard_router.cpp` - Query routing to shards

**Dependencies:**
- src/storage/ - Data storage
- src/network/ - Network communication
- src/replication/ - Data replication

**Feature Flags:** None (core distributed feature)

**Documentation:**
- [Sharding README](../../src/sharding/README.md)
- [RAID Documentation Hub](../../docs/RAID_DOCUMENTATION_HUB.md)
- [RAID Orchestration Architecture](../../docs/RAID_ORCHESTRATION_ARCHITECTURE.md)

**Example Usage:**
```cpp
#include "sharding/shard_manager.h"

ShardManager manager;
manager.create_shard(shard_config);
manager.route_query(query, target_shards);
```

---

#### src/scheduler/
**Purpose:** Task scheduling, retention management, and background job processing.

**Key Files:**
- `task_scheduler.cpp` - General task scheduling
- `hybrid_retention_manager.cpp` - Data retention policy management

**Dependencies:**
- src/storage/ - Data access
- src/utils/ - Time utilities

**Feature Flags:** None (core feature)

**Documentation:**
- [Scheduler README](../../src/scheduler/README.md)
- [Retention Policy Guide](../../docs/retention/)

**Example Usage:**
```cpp
#include "scheduler/task_scheduler.h"

TaskScheduler scheduler;
scheduler.schedule(task, interval);
```

---

### Observability

#### src/observability/
**Purpose:** Metrics collection, monitoring, and observability infrastructure.

**Key Files:**
- `metrics_collector.cpp` - System metrics collection

**Dependencies:**
- src/utils/ - Logging
- Prometheus (external, optional)

**Feature Flags:**
- `THEMIS_ENABLE_TRACING` - OpenTelemetry tracing (ON by default)

**Documentation:**
- [Observability Guide](../../docs/observability/)
- [Metrics Documentation](../../docs/features/metrics.md)

**Example Usage:**
```cpp
#include "observability/metrics_collector.h"

MetricsCollector collector;
collector.record_metric("query_time", duration_ms);
```

---

### Data Integration

#### src/content/
**Purpose:** Content management, ingestion pipeline, and multi-format processing.

**Key Files:** (22 files total)
- `archive_processor.cpp` - Archive file processing
- `async_ingestion_worker.cpp` - Asynchronous data ingestion
- `audio_processor.cpp` - Audio file processing
- `cad_processor.cpp` - CAD file processing
- `content_manager.cpp` - Content lifecycle management
- `document_processor.cpp` - Document processing
- `image_processor.cpp` - Image processing
- `pdf_processor.cpp` - PDF processing
- `video_processor.cpp` - Video processing

**Dependencies:**
- src/storage/ - Content storage
- src/index/ - Content indexing
- External libraries for format support

**Feature Flags:**
- `THEMIS_ENABLE_CONTENT_PROCESSORS` - Enable content processing

**Documentation:**
- [Content README](../../src/content/README.md)
- [Content Architecture](../../docs/content_architecture.md)

**Example Usage:**
```cpp
#include "content/content_manager.h"

ContentManager manager;
manager.ingest(file_path, content_type);
```

---

#### src/importers/
**Purpose:** Data import from external systems and databases.

**Key Files:**
- `postgres_importer.cpp` - PostgreSQL data import

**Dependencies:**
- src/storage/ - Target storage
- src/query/ - Data transformation

**Feature Flags:** None

**Documentation:**
- [Importers README](../../src/importers/README.md)
- [Import Guide](../../docs/integration/import.md)

**Example Usage:**
```cpp
#include "importers/postgres_importer.h"

PostgresImporter importer(connection_string);
importer.import(table_name);
```

---

#### src/exporters/
**Purpose:** Data export to external formats and systems.

**Key Files:**
- `jsonl_llm_exporter.cpp` - JSONL export for LLM training

**Dependencies:**
- src/storage/ - Source data
- src/query/ - Data filtering

**Feature Flags:** None

**Documentation:**
- [Exporters README](../../src/exporters/README.md)
- [Export Guide](../../docs/integration/export.md)

**Example Usage:**
```cpp
#include "exporters/jsonl_llm_exporter.h"

JSONLLLMExporter exporter;
exporter.export_to_file(query, output_path);
```

---

#### src/cdc/
**Purpose:** Change Data Capture (CDC) for real-time data change streaming.

**Key Files:**
- `changefeed.cpp` - Changefeed implementation
- `changefeed_buffer.cpp` - Buffering for change events

**Dependencies:**
- src/storage/ - Data change detection
- src/server/ - SSE for streaming changes

**Feature Flags:**
- `THEMIS_ENABLE_SSE` - Required for changefeed streaming

**Documentation:**
- [CDC README](../../src/cdc/README.md)
- [Changefeed Guide](../../docs/features/changefeed.md)

**Example Usage:**
```cpp
#include "cdc/changefeed.h"

Changefeed feed("users");
feed.subscribe([](const Change& change) {
    // Handle change event
});
```

---

### Specialized Features

#### src/geo/
**Purpose:** Geospatial functionality - indexing, queries, and processing.

**Key Files:**
- `cpu_backend.cpp` - CPU-based geo operations
- `gpu_backend_stub.cpp` - GPU geo operations (stub)
- `boost_cpu_exact_backend.cpp` - Boost.Geometry exact calculations

**Dependencies:**
- src/index/ - Spatial indexing
- src/acceleration/ - GPU acceleration
- Boost.Geometry, GDAL (external)

**Feature Flags:** None (geo is core feature)

**Documentation:**
- [Geo README](../../src/geo/README.md)
- [Geospatial Guide](../../docs/features/geo.md)
- [GDAL Integration](../ARCHIVED/implementation-summaries/GDAL_INTEGRATION_SUMMARY.md)

**Example Usage:**
```cpp
#include "geo/cpu_backend.h"

GeoCPUBackend geo;
auto results = geo.spatial_query(point, radius);
```

---

#### src/timeseries/
**Purpose:** Time-series data management, continuous aggregates, and temporal queries.

**Key Files:** (12 files total)
- `aggregates.cpp` - Time-series aggregation functions
- `continuous_agg.cpp` - Continuous aggregates
- `aggregate_scheduler.cpp` - Scheduled aggregate updates
- `downsample.cpp` - Data downsampling
- `retention_policy.cpp` - Time-series retention
- `timeseries_index.cpp` - Time-series indexing

**Dependencies:**
- src/storage/ - Data storage
- src/scheduler/ - Aggregate scheduling
- src/query/ - Time-series queries

**Feature Flags:** None (core feature)

**Documentation:**
- [Timeseries README](../../src/timeseries/README.md)
- [Time-Series Guide](../../docs/features/timeseries.md)

**Example Usage:**
```cpp
#include "timeseries/continuous_agg.h"

ContinuousAggregate agg("avg_temperature");
agg.configure(interval, aggregation_func);
```

---

#### src/temporal/
**Purpose:** Temporal consistency, conflict resolution, and version control.

**Key Files:**
- `temporal_conflict_resolver.cpp` - Multi-version conflict resolution

**Dependencies:**
- src/storage/ - Versioned data
- src/transaction/ - Transaction context

**Feature Flags:** None

**Documentation:**
- [Temporal Consistency](../../docs/temporal/)

**Example Usage:**
```cpp
#include "temporal/temporal_conflict_resolver.h"

TemporalConflictResolver resolver;
auto resolved = resolver.resolve(version1, version2);
```

---

#### src/voice/
**Purpose:** Voice assistant integration and voice command processing.

**Key Files:**
- `voice_assistant.cpp` - Voice assistant implementation
- `voice_assistant_llm.cpp` - LLM integration for voice

**Dependencies:**
- src/llm/ - LLM for natural language understanding
- Whisper, Piper (external, for speech recognition/synthesis)

**Feature Flags:**
- `THEMIS_ENABLE_LLM` - Required for voice understanding

**Documentation:**
- [Voice Assistant Guide](../../docs/features/voice_assistant.md)
- [Voice Example](../../examples/voice_assistant_example.py)

**Example Usage:**
```cpp
#include "voice/voice_assistant.h"

VoiceAssistant assistant;
assistant.process_voice_command(audio_data);
```

---

### System Features

#### src/plugins/
**Purpose:** Plugin system for extending ThemisDB with custom functionality.

**Key Files:**
- `plugin_manager.cpp` - Plugin lifecycle management
- `plugin_system_edition.cpp` - Edition-aware plugin loading
- `rpc_service_registry.cpp` - RPC service registration for plugins

**Dependencies:**
- src/base/ - Module loading
- src/server/ - RPC integration

**Feature Flags:** None (plugin system is core)

**Documentation:**
- [Plugins README](../../src/plugins/README.md)
- [Plugin Development Guide](../../docs/development/plugins.md)

**Example Usage:**
```cpp
#include "plugins/plugin_manager.h"

PluginManager manager;
manager.load_plugin("my_plugin.so");
```

---

#### src/updates/
**Purpose:** Hot reload, version updates, and manifest-based update system.

**Key Files:**
- `hot_reload_engine.cpp` - Hot reload without downtime
- `manifest_database.cpp` - Update manifest management
- `release_manifest.cpp` - Release version manifests
- `updates_config.cpp` - Update configuration

**Dependencies:**
- src/storage/ - Manifest storage
- src/network/ - Update downloads

**Feature Flags:** None

**Documentation:**
- [Hot Reload Guide](../../docs/features/hot_reload.md)
- [Update System](../../docs/updates/)

**Example Usage:**
```cpp
#include "updates/hot_reload_engine.h"

HotReloadEngine engine;
engine.reload_configuration();
```

---

#### src/performance/
**Purpose:** Performance optimization features including advanced storage and indexing structures.

**Key Files:**
- `cicada.cpp` - Cicada MVCC protocol
- `dostoevsky.cpp` - Dostoevsky LSM-tree optimization
- `ligra.cpp` - Ligra graph processing
- `phase2_feature_flags.cpp` - Phase 2 performance features
- `phase3/` - Phase 3 experimental features

**Dependencies:**
- src/storage/ - Storage integration
- src/index/ - Index optimization

**Feature Flags:**
- `THEMIS_ENABLE_WISCKEY` - WiscKey LSM optimization
- `THEMIS_ENABLE_DOSTOEVSKY` - Dostoevsky LSM
- `THEMIS_ENABLE_CICADA` - Cicada MVCC
- `THEMIS_ENABLE_LIGRA` - Ligra graph processing
- `THEMIS_ENABLE_DISKANN` - DiskANN index
- `THEMIS_ENABLE_BWTREE` - Bw-Tree index
- `THEMIS_ENABLE_SPLINTERDB` - SplinterDB storage
- `THEMIS_ENABLE_GUNROCK` - Gunrock graph processing

**Documentation:**
- [Performance Optimization Plan](../../docs/performance/)
- [Phase 2 Features](../../docs/performance/phase2.md)
- [Phase 3 Features](../../docs/performance/phase3.md)

**Example Usage:**
```cpp
// These are typically integrated at the storage/index layer
// and selected via configuration
```

---

#### src/metadata/
**Purpose:** Database schema management and metadata catalog.

**Key Files:**
- `schema_manager.cpp` - Schema definition and management

**Dependencies:**
- src/storage/ - Metadata persistence

**Feature Flags:** None (core feature)

**Documentation:**
- [Schema Management](../../docs/schema/)

**Example Usage:**
```cpp
#include "metadata/schema_manager.h"

SchemaManager schema;
schema.create_collection("users", schema_definition);
```

---

## Adding New Code

### Where Should My Code Go?

Use this decision tree to determine the appropriate directory:

1. **Is it a new protocol/API?** → `src/server/` or `src/api/`
2. **Is it a storage engine component?** → `src/storage/`
3. **Is it an index/search feature?** → `src/index/` or `src/search/`
4. **Is it query-related?** → `src/query/` or `src/aql/`
5. **Is it LLM/AI functionality?** → `src/llm/` or `src/acceleration/`
6. **Is it security-related?** → `src/security/` or `src/auth/`
7. **Is it distributed systems?** → `src/sharding/` or `src/replication/`
8. **Is it data integration?** → `src/importers/`, `src/exporters/`, or `src/content/`
9. **Is it monitoring/observability?** → `src/observability/`
10. **Is it a utility/helper?** → `src/utils/`
11. **Is it foundational/base?** → `src/base/`

### Guidelines

- **Single Responsibility:** Each directory should have a clear, focused purpose
- **Minimal Dependencies:** Prefer depending on lower layers (base, utils, storage)
- **Feature Flags:** Add feature flags in `CMakeLists.txt` for optional/experimental code
- **Documentation:** Update this guide when adding new directories
- **README Files:** Add a README.md in the directory explaining its purpose

---

## Cross-Cutting Concerns

Some functionality spans multiple directories. Here's how to handle common cases:

### Logging
- **Implementation:** `src/utils/logging.cpp`
- **Usage:** Include from any module
- **Storage:** Logs can integrate with `src/observability/`

### Authentication & Authorization
- **Authentication:** `src/auth/` - User identity verification
- **Authorization:** `src/governance/` - Access control policies
- **Integration:** `src/server/auth_middleware.cpp` - HTTP layer

### Caching
- **Infrastructure:** `src/cache/` - Cache implementations
- **Query Cache:** `src/query/` - Query result caching
- **LLM Cache:** `src/llm/prefix_cache.cpp` - KV cache for LLM

### Metrics & Monitoring
- **Collection:** `src/observability/metrics_collector.cpp`
- **Export:** `src/exporters/` - Prometheus/metrics export
- **Storage:** Can persist to `src/storage/` or external systems

---

## Dependency Graph

### Core Dependencies (Bottom Layer)
```
src/base/  ←  (Foundation - no dependencies)
   ↑
src/utils/  ←  (Utilities - depends on base)
   ↑
src/network/  (Network - depends on base, utils)
```

### Storage Layer
```
src/storage/  ←  (Core storage - depends on base, utils)
   ↑
├── src/cache/  (Caching - depends on storage)
├── src/transaction/  (Transactions - depends on storage)
└── src/replication/  (Replication - depends on storage, network)
```

### Query & Processing Layer
```
src/query/  ←  (Depends on storage, index)
   ↑
├── src/aql/  (AQL extensions - depends on query, llm)
├── src/analytics/  (Analytics - depends on query, storage)
└── src/search/  (Search - depends on index, query)
```

### Index Layer
```
src/index/  ←  (Depends on storage, acceleration)
   ↑
├── src/geo/  (Geo indexes - depends on index)
└── src/timeseries/  (Time-series - depends on index, storage)
```

### API & Server Layer
```
src/server/  ←  (Depends on query, storage, auth)
   ↑
├── src/api/  (High-level APIs - depends on server, query)
└── src/cdc/  (CDC - depends on storage, server)
```

### AI & Acceleration
```
src/acceleration/  ←  (Hardware acceleration - minimal deps)
   ↑
├── src/llm/  (LLM - depends on acceleration, storage, cache)
└── src/gpu/  (GPU memory - depends on acceleration)
```

### Security Layer
```
src/security/  ←  (Depends on storage, utils)
   ↑
├── src/auth/  (Authentication - depends on security)
└── src/governance/  (Governance - depends on security, storage)
```

---

## Feature Flags Reference

| Directory | Primary Feature Flag | Status |
|-----------|---------------------|--------|
| src/llm/ | `THEMIS_ENABLE_LLM` | Optional (OFF) |
| src/acceleration/ | `THEMIS_ENABLE_GPU` | Optional (OFF) |
| src/acceleration/ | `THEMIS_ENABLE_CUDA` | Optional (OFF) |
| src/auth/ | `THEMIS_ENABLE_KERBEROS` | Optional (OFF) |
| src/content/ | `THEMIS_ENABLE_CONTENT_PROCESSORS` | Optional (OFF) |
| src/security/ | `THEMIS_ENABLE_HSM_REAL` | Optional (OFF) |
| src/api/ | `THEMIS_ENABLE_GRAPHQL` | Default (ON) |
| src/server/ | `THEMIS_ENABLE_SSE` | Optional (OFF) |
| src/server/ | `THEMIS_ENABLE_WEBSOCKET` | Optional (OFF) |
| src/server/ | `THEMIS_ENABLE_MQTT` | Optional (OFF) |
| src/server/ | `THEMIS_ENABLE_POSTGRES_WIRE` | Optional (OFF) |
| src/performance/ | `THEMIS_ENABLE_WISCKEY` | Experimental (OFF) |
| src/performance/ | `THEMIS_ENABLE_DOSTOEVSKY` | Experimental (OFF) |
| src/performance/ | `THEMIS_ENABLE_CICADA` | Experimental (OFF) |
| src/performance/ | `THEMIS_ENABLE_LIGRA` | Experimental (OFF) |
| src/index/ | `THEMIS_ENABLE_DISKANN` | Experimental (OFF) |
| src/index/ | `THEMIS_ENABLE_RABITQ` | Experimental (OFF) |

For complete feature flag documentation, see [FEATURE_FLAGS_REFERENCE.md](FEATURE_FLAGS_REFERENCE.md).

---

## Deprecated Directories

There are currently **no deprecated directories** in the src/ folder. All 35 directories are actively maintained.

If a directory becomes deprecated in the future, it will be listed here with:
- Deprecation date
- Reason for deprecation
- Migration path
- Removal timeline

---

## Maintenance

### Keeping This Guide Updated

This guide should be updated when:
- A new directory is added to `src/`
- A directory's purpose changes significantly
- Major refactoring occurs
- Feature flags are added/removed
- Dependencies between modules change

### Ownership

- **Primary Maintainer:** Architecture team
- **Review Frequency:** Quarterly
- **Update Process:** Submit PR with changes to this document

### Validation

To validate this guide remains accurate:
```bash
# Check all directories are documented
find src -maxdepth 1 -type d | wc -l  # Should match documented count + src itself

# List any new directories
comm -13 <(grep "^#### src/" docs/architecture/SOURCE_DIRECTORY_GUIDE.md | cut -d'/' -f2 | sort) \
         <(ls -1 src | grep -v -E '(^README.md|^main|^demo|^version)' | sort)
```

---

## Quick Reference

| Need | Directory |
|------|-----------|
| Add API endpoint | `src/server/` |
| Add new index type | `src/index/` |
| Add data processor | `src/content/` |
| Add LLM feature | `src/llm/` |
| Add security feature | `src/security/` |
| Add utility function | `src/utils/` |
| Add storage backend | `src/storage/` |
| Add query operator | `src/query/` |
| Add monitoring metric | `src/observability/` |
| Add distributed feature | `src/sharding/` |

---

## Related Documentation

- [CMAKE_MODULAR_ARCHITECTURE.md](CMAKE_MODULAR_ARCHITECTURE.md) - Build system architecture
- [FEATURE_FLAGS_REFERENCE.md](FEATURE_FLAGS_REFERENCE.md) - Complete feature flag documentation
- [BASEENTITY_PRINCIPLE.md](BASEENTITY_PRINCIPLE.md) - Core data model pattern
- [../CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution guidelines
- [../CODING_STANDARDS.md](../CODING_STANDARDS.md) - Code style standards
- [Documentation Index](../00_DOCUMENTATION_INDEX.md) - Complete documentation index

---

**Document Status:** ✅ Complete  
**Coverage:** 35/35 directories (100%)  
**Last Validated:** 2026-01-12
