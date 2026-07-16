# ThemisDB Core Modularization Plan (Post v1.3.0)

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🧩 Architecture  
**Status:** Planned for implementation after v1.3.0 release  
**Target Version:** 1.4.0+  
**Estimated Effort:** 2-4 weeks

---

## 📑 Table of Contents

- [Executive Summary](#executive-summary)
- [Current Architecture Issues](#current-architecture-issues)
- [Proposed Modular Architecture](#proposed-modular-architecture)
- [Benefits](#benefits)
- [Challenges & Solutions](#challenges--solutions)

## Executive Summary

ThemisDB currently builds as a single monolithic library (`themis_core`) containing 69,000+ exported symbols, which exceeds the Windows COFF object file limit of 65,535 symbols. This document outlines a plan to split `themis_core` into smaller, thematically organized libraries.

## Current Architecture Issues

### The COFF Symbol Limit Problem
- **Current State**: `themis_core` has 69,000+ exported symbols
- **Windows COFF Limit**: 65,535 symbols per object file
- **Impact**: Build failures on Windows when using static libraries
- **Workaround**: Currently using `THEMIS_CORE_SHARED=ON` (DLL build)

### Other Challenges
- **Build Times**: Incremental builds require recompiling entire core
- **Code Organization**: All features bundled together
- **Optional Features**: Cannot selectively disable LLM/Geo/GPU features
- **Testing**: Difficult to isolate and test individual subsystems

## Proposed Modular Architecture

### Module Overview

The proposed split divides `themis_core` into 11 focused libraries:

| Module | Estimated Symbols | Dependencies | Description |
|--------|------------------|--------------|-------------|
| `themis_base` | ~3,000 | None | Common types, interfaces, status codes |
| `themis_storage` | ~15,000 | base, RocksDB | Core storage engine, indexes |
| `themis_query` | ~12,000 | base, storage | AQL parser, query engine, functions |
| `themis_security` | ~8,000 | base, OpenSSL | Encryption, PKI, RBAC, JWT, audit |
| `themis_sharding` | ~10,000 | base, storage, query | Distributed system, Raft, gossip |
| `themis_llm` | ~6,000 | base, storage | Model inference, LoRA, KV cache |
| `themis_content` | ~5,000 | base, storage | Content management, MIME, malware |
| `themis_timeseries` | ~4,000 | base, storage | Time-series store, Gorilla compression |
| `themis_network` | ~5,000 | base | HTTP/Wire protocol servers, SSE |
| `themis_geo` | ~2,000 | base, storage | Geospatial index and operations |
| `themis_graph` | ~2,000 | base, storage | Graph index and analytics |

**Total**: ~72,000 symbols → Each module well under 65,535 limit ✓

### Module Dependency Graph

```
                    themis_base
                        │
         ┌──────────────┼──────────────┐
         │              │              │
    themis_storage  themis_security  themis_network
         │              │              
         ├──────┬───────┼──────┬───────┤
         │      │       │      │       │
    themis_  themis_  themis_ themis_ themis_
    query    geo      content llm     sharding
         │      │       │      │       │
         └──────┴───────┴──────┴───────┘
                    themis_graph
                    themis_timeseries
```

## Benefits

### ✅ Technical Benefits
1. **Solves COFF Limit**: Each module stays under 65,535 symbols
2. **Faster Incremental Builds**: Only rebuild changed modules
3. **Better Code Organization**: Clear separation of concerns
4. **Optional Features**: Build without LLM/Geo/GPU if not needed
5. **Independent Testing**: Test modules in isolation
6. **Parallel Compilation**: Build multiple modules simultaneously

### ✅ Architectural Benefits
1. **Maintainability**: Smaller, focused codebases per module
2. **Modularity**: Pluggable architecture for optional features
3. **Documentation**: Easier to document individual modules
4. **Onboarding**: New developers can focus on specific modules

## Challenges & Solutions

### ⚠️ Challenge 1: Circular Dependencies
**Problem**: Query ↔ Storage ↔ Security potential cycles

**Solution**: 
- Use dependency inversion principle
- Define interfaces in `themis_base`
- Implement in respective modules
- Use factory patterns for cross-module instantiation

### ⚠️ Challenge 2: Export Management
**Problem**: Each module needs its own DLL export macros

**Solution**:
```cpp
// include/themis/base/export.h
#define THEMIS_BASE_EXPORT   /* platform-specific */
#define THEMIS_STORAGE_EXPORT /* platform-specific */
// ... one per module
```

### ⚠️ Challenge 3: Header Organization
**Problem**: Shared vs. private headers must be clearly separated

**Solution**:
```
include/themis/
  base/          # Public API headers
    export.h
    types.h
  storage/       # Public API headers
    rocksdb_wrapper.h
src/
  base/          # Implementation + private headers
  storage/       # Implementation + private headers
```

### ⚠️ Challenge 4: CMake Complexity
**Problem**: Managing 11+ library targets instead of 1

**Solution**:
- Use helper functions/macros for common patterns
- Maintain clear dependency declarations
- Use target-based propagation (INTERFACE/PUBLIC/PRIVATE)

### ⚠️ Challenge 5: DLL Security & Corruption Prevention
**Problem**: Modular DLLs could be corrupted, tampered with, or replaced with malicious code

**Solution**: Comprehensive module verification system (IMPLEMENTED)
- **SHA-256 Hash Verification**: Calculate and verify file integrity before loading
- **Digital Signature Verification**: X.509 certificate-based signing (RSA/ECDSA)
- **Certificate Chain Validation**: Verify trusted issuers
- **Blacklist/Whitelist Support**: Block known malicious or allow known good hashes
- **Audit Logging**: Log all module load attempts and verification results
- **Production Mode**: Mandatory signature verification (unsigned modules rejected)
- **Development Mode**: Optional signatures for local development

**Implementation**:
```cpp
// themis_base includes ModuleLoader with security verification
#include "themis/base/module_loader.h"

// Load modules with automatic verification
themis::modules::ModuleLoader loader;
auto result = loader.loadModule("themis_storage.dll", "themis_storage");

if (!result.success) {
    // Module verification failed - security issue detected
    spdlog::critical("Module verification failed: {}", result.errorMessage);
    // Module NOT loaded - system remains secure
}
```

**Security Features**:
- Reuses existing `PluginSecurityVerifier` infrastructure
- Production builds require valid signatures
- Development builds allow unsigned modules for testing
- Tampered DLLs are detected via hash mismatch
- Expired certificates are rejected
- All security events logged to audit trail

## Implementation Plan

### Phase 1: Foundation (Week 1)
- [ ] Create `themis_base` module with common types
- [ ] Extract `BaseEntity`, status codes, core interfaces
- [ ] Define export macros for all modules
- [ ] Update CMakeLists.txt with modular structure
- [ ] Ensure existing tests still pass

### Phase 2: Core Modules (Week 2)
- [ ] Split out `themis_storage` (RocksDB, indexes)
- [ ] Split out `themis_query` (AQL, query engine)
- [ ] Split out `themis_security` (encryption, PKI, RBAC)
- [ ] Split out `themis_network` (HTTP/Wire servers)
- [ ] Run full test suite

### Phase 3: Feature Modules (Week 3)
- [ ] Split out `themis_timeseries` (TSStore, Gorilla)
- [ ] Split out `themis_geo` (spatial index)
- [ ] Split out `themis_graph` (graph index)
- [ ] Split out `themis_content` (content management)
- [ ] Split out `themis_llm` (model inference)
- [ ] Make feature modules optional via CMake options

### Phase 4: Distributed System (Week 4)
- [ ] Split out `themis_sharding` (Raft, gossip, WAL)
- [ ] Resolve any remaining circular dependencies
- [ ] Run full test suite including benchmarks
- [ ] Update documentation
- [ ] Validate Windows builds (COFF limit resolved)

### Phase 5: Testing & Validation
- [ ] Run all unit tests
- [ ] Run all integration tests
- [ ] Run all benchmarks
- [ ] Verify build times improvement
- [ ] Test optional module configurations
- [ ] Update CI/CD pipelines

## CMake Configuration (Preview)

```cmake
# New build options for modular architecture
option(THEMIS_BUILD_MODULAR "Build as modular libraries instead of monolithic core" ON)
option(THEMIS_MODULE_LLM "Include LLM inference module (optional)" ON)
option(THEMIS_MODULE_GEO "Include geospatial module (optional)" ON)
option(THEMIS_MODULE_GRAPH "Include graph analytics module (optional)" ON)
option(THEMIS_MODULE_CONTENT "Include content processors module (optional)" OFF)

# Core modules (always required)
add_library(themis_base SHARED ${THEMIS_BASE_SOURCES})
add_library(themis_storage SHARED ${THEMIS_STORAGE_SOURCES})
add_library(themis_query SHARED ${THEMIS_QUERY_SOURCES})
add_library(themis_security SHARED ${THEMIS_SECURITY_SOURCES})
add_library(themis_network SHARED ${THEMIS_NETWORK_SOURCES})
add_library(themis_sharding SHARED ${THEMIS_SHARDING_SOURCES})
add_library(themis_timeseries SHARED ${THEMIS_TIMESERIES_SOURCES})

# Optional modules
if(THEMIS_MODULE_LLM)
    add_library(themis_llm SHARED ${THEMIS_LLM_SOURCES})
endif()

if(THEMIS_MODULE_GEO)
    add_library(themis_geo SHARED ${THEMIS_GEO_SOURCES})
endif()

if(THEMIS_MODULE_GRAPH)
    add_library(themis_graph SHARED ${THEMIS_GRAPH_SOURCES})
endif()

if(THEMIS_MODULE_CONTENT)
    add_library(themis_content SHARED ${THEMIS_CONTENT_SOURCES})
endif()

# Main executable links all required modules
target_link_libraries(themis_server PRIVATE
    themis_base
    themis_storage
    themis_query
    themis_security
    themis_network
    themis_sharding
    themis_timeseries
    $<$<BOOL:${THEMIS_MODULE_LLM}>:themis_llm>
    $<$<BOOL:${THEMIS_MODULE_GEO}>:themis_geo>
    $<$<BOOL:${THEMIS_MODULE_GRAPH}>:themis_graph>
    $<$<BOOL:${THEMIS_MODULE_CONTENT}>:themis_content>
)
```

## Source File Organization (Preview)

### themis_base (~3,000 symbols)
```
src/base/
  base_entity.cpp
  status_codes.cpp
  common_interfaces.cpp
  
include/themis/base/
  export.h
  base_entity.h
  status.h
  types.h
```

### themis_storage (~15,000 symbols)
```
src/storage/
  rocksdb_wrapper.cpp
  key_schema.cpp
  backup_manager.cpp
  security_signature.cpp
  security_signature_manager.cpp
  
src/index/
  secondary_index.cpp
  adaptive_index.cpp
  spatial_index.cpp
  
include/themis/storage/
  export.h
  rocksdb_wrapper.h
  key_schema.h
  backup_manager.h
```

### themis_query (~12,000 symbols)
```
src/query/
  query_engine.cpp
  query_optimizer.cpp
  aql_parser.cpp
  aql_translator.cpp
  aql_runner.cpp
  let_evaluator.cpp
  window_evaluator.cpp
  cte_subquery.cpp
  cte_cache.cpp
  statistical_aggregator.cpp
  semantic_cache.cpp
  functions/function_registry.cpp
  
include/themis/query/
  export.h
  query_engine.h
  aql_parser.h
```

### themis_security (~8,000 symbols)
```
src/security/
  mock_key_provider.cpp
  vault_key_provider.cpp
  key_cache.cpp
  keyprovider_signing.cpp
  vault_signing_provider.cpp
  field_encryption.cpp
  encrypted_field.cpp
  malware_scanner.cpp
  pki_key_provider.cpp
  cms_signing.cpp
  rbac.cpp
  hsm_provider.cpp
  hsm_provider_pkcs11.cpp
  timestamp_authority.cpp
  timestamp_authority_openssl.cpp
  vcc_pki_client.cpp
  
src/auth/
  jwt_validator.cpp
  
include/themis/security/
  export.h
  encryption.h
  pki.h
  rbac.h
```

### themis_sharding (~10,000 symbols)
```
src/sharding/
  urn.cpp
  consistent_hash.cpp
  shard_topology.cpp
  urn_resolver.cpp
  pki_shard_certificate.cpp
  mtls_client.cpp
  signed_request.cpp
  remote_executor.cpp
  shard_router.cpp
  rebalance_operation.cpp
  data_migrator.cpp
  shard_load_detector.cpp
  auto_rebalancer.cpp
  prometheus_metrics.cpp
  metrics_registry.cpp
  health_check.cpp
  admin_api.cpp
  cloud_agent.cpp
  circuit_breaker.cpp
  gossip_protocol.cpp
  raft_configuration.cpp
  raft_log.cpp
  raft_state.cpp
  raft_wal_integration.cpp
  stream_protocol.cpp
  wal_applier.cpp
  wal_manager.cpp
  wal_shipper.cpp
  truetime.cpp
  distributed_transaction.cpp
  
include/themis/sharding/
  export.h
  shard_router.h
  raft.h
```

### themis_llm (~6,000 symbols)
```
src/llm/
  llm_interaction_store.cpp
  prompt_manager.cpp
  
include/themis/llm/
  export.h
  llm_interaction_store.h
  prompt_manager.h
```

### themis_content (~5,000 symbols)
```
src/content/
  content_type.cpp
  content_manager.cpp
  text_processor.cpp
  mock_clip_processor.cpp
  mime_detector.cpp
  content_policy.cpp
  content_fs.cpp
  version_manager.cpp
  
include/themis/content/
  export.h
  content_manager.h
  content_policy.h
```

### themis_timeseries (~4,000 symbols)
```
src/timeseries/
  timeseries.cpp
  tsstore.cpp
  gorilla.cpp
  retention.cpp
  continuous_agg.cpp
  aggregate_scheduler.cpp
  aggregate_scheduler_helper.cpp
  query_optimizer.cpp
  
include/themis/timeseries/
  export.h
  tsstore.h
  gorilla.h
```

### themis_network (~5,000 symbols)
```
src/network/
  wire_protocol_server.cpp
  
src/server/
  http_server.cpp
  sse_connection_manager.cpp
  audit_api_handler.cpp
  pki_api_handler.cpp
  saga_api_handler.cpp
  pii_api_handler.cpp
  retention_api_handler.cpp
  keys_api_handler.cpp
  classification_api_handler.cpp
  reports_api_handler.cpp
  ranger_adapter.cpp
  policy_engine.cpp
  auth_middleware.cpp
  rate_limiter.cpp
  rate_limiter_v2.cpp
  load_shedder.cpp
  update_api_handler.cpp
  hot_reload_api_handler.cpp
  export_api_handler.cpp
  tenant_manager.cpp
  sharding_metrics_handler.cpp
  
include/themis/network/
  export.h
  http_server.h
  wire_protocol.h
```

### themis_geo (~2,000 symbols)
```
src/geo/
  cpu_backend.cpp
  gpu_backend_stub.cpp
  boost_cpu_exact_backend.cpp
  
src/api/
  geo_index_hooks.cpp
  
src/utils/geo/
  ewkb.cpp
  
include/themis/geo/
  export.h
  spatial.h
```

### themis_graph (~2,000 symbols)
```
src/index/
  graph_index.cpp
  temporal_graph.cpp
  property_graph.cpp
  edge_types.cpp
  process_graph.cpp
  gnn_embeddings.cpp
  graph_analytics.cpp
  
include/themis/graph/
  export.h
  graph_index.h
  temporal_graph.h
```

## Migration Strategy

### Backward Compatibility
To ensure smooth transition, support both build modes during migration:

```cmake
if(THEMIS_BUILD_MODULAR)
    # New modular build
    add_subdirectory(src/base)
    add_subdirectory(src/storage)
    # ...
else()
    # Legacy monolithic build (deprecated)
    add_library(themis_core SHARED ${THEMIS_CORE_SOURCES})
endif()
```

### Testing Strategy
1. **Unit Tests**: Update to link specific modules
2. **Integration Tests**: Update to link all required modules
3. **Regression Tests**: Ensure identical behavior
4. **Performance Tests**: Verify no performance regression

## Success Criteria

- [ ] All modules have < 65,535 symbols
- [ ] Windows builds succeed without COFF errors
- [ ] All existing tests pass
- [ ] Build time improves by at least 30% for incremental builds
- [ ] Optional modules can be disabled
- [ ] Documentation is updated
- [ ] CI/CD pipelines work with new structure

## Rollback Plan

If modularization causes issues:
1. Set `THEMIS_BUILD_MODULAR=OFF` to use legacy build
2. Fix issues in a controlled manner
3. Re-enable modular build once resolved

## Timeline

**Pre-requisite**: v1.3.0 must be released first

| Timeframe | Milestone |
|-----------|-----------|
| Week 1 | Foundation setup (base module, CMake structure) |
| Week 2 | Core modules (storage, query, security, network) |
| Week 3 | Feature modules (timeseries, geo, graph, content, llm) |
| Week 4 | Distributed system (sharding), testing, validation |
| Week 5 | Buffer for issues, documentation, review |

## References

- Windows COFF format: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
- CMake target-based design: https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html
- Current issue: themis_core exceeds 65,535 symbol limit (69,000+ symbols)

## Conclusion

This modularization is a significant architectural upgrade that will:
1. **Solve immediate technical issues** (COFF limit on Windows)
2. **Improve developer experience** (faster builds, clearer code organization)
3. **Enable future growth** (optional modules, plugin architecture)

The effort is substantial (2-4 weeks) but provides long-term benefits for maintainability and scalability.

**Action**: This plan will be implemented **after** the v1.3.0 release.
