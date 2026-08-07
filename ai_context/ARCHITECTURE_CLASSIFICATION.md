# ThemisDB Module Classification: Core, Integrated Modules, and Plugins

**Datum:** 2026-08-07  
**Status:** Active (Wave-1 Private Plugin Strategy)  
**Primary (Quelle der Wahrheit):** ARCHITECTURE.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md, src/plugins/ROADMAP.md  
**Bezug:** Architecture division clarification, Wave-1 plugin externalization

---

## Overview

ThemisDB is organized into three architectural tiers (T0–T5 security/trust model) plus an optional private plugin layer. This document clarifies the division and intended build-time behavior.

**Key Principle:** Integrated modules are built statically and linked into the core engine by default. Plugins are optionally dynamically loaded at runtime. Private plugins are externalized to separate repositories and optional based on edition and license features.

---

## Tier Model (T0–T5 Trust Boundary)

This document uses a **simplified 3-tier classification** aligned with the more detailed security/functional tier model defined in `ARCHITECTURE.md`.

**Simplified Classification (This Document):**

| Tier | Name | Grouping | Purpose |
|------|------|----------|---------|
| **T0** | Trusted Core | `src/core/`, `src/base/`, `src/themis/`, `src/utils/`, `src/plugins/` | Minimal bootstrap, lifecycle, RAII primitives, plugin system |
| **T1–T2** | Core Engine | `src/aql/`, `src/query/`, `src/execution/`, `src/storage/`, `src/index/`, `src/cache/`, `src/metadata/` | Query processing, storage, indexing, transactionality |
| **T3–T4** | Infrastructure & Governance | All remaining `src/` modules (auth, security, replication, sharding, observability, etc.) | Cross-cutting infrastructure, compliance, distribution |
| **T5** | Plugin Boundary | `plugins/` (public), `plugins/private/` (private) | Dynamically loaded extensions, optional backends, adapters |

**Detailed Security/Functional Model (ARCHITECTURE.md):**

For a more granular security and functional analysis, see the T0–T5 tier definitions in `ARCHITECTURE.md` (§ Security & Hardening Tiering Model):

- **T0: Trusted Core** — Minimal trusted computing base, bootstrapping, memory/lifecycle primitives
- **T1: Security & Platform Services** — Identity, cryptography, policy, config validation, audit
- **T2: Data Plane Engines** — Query, transaction, storage, index, sharding/replication logic
- **T3: Interface & Protocol Edge** — External protocol handling and request ingress
- **T4: Managed Extension Runtime** — In-tree feature extensions with controlled interfaces
- **T5: Plugin Boundary (Least Trusted)** — Dynamically loaded or remotely controlled plugin code

Both models are compatible; this document uses the simplified 3-tier grouping for module classification and development organization, while `ARCHITECTURE.md` provides the detailed security/hardening baseline per tier.

---

## Module Inventory by Classification

### Core (T0) — Minimal Trusted Base

These modules are always compiled and linked statically. They provide foundational abstractions that all other code depends on.

| Module | Location | Namespace | Purpose |
|--------|----------|-----------|---------|
| **base** | `src/base/` | `themis::resource` | Core types, RAII primitives, module loader |
| **core** | `src/core/` | `themis::core` | Security initialization, concerns context (logging/tracing) |
| **plugins** | `src/plugins/` | `themis::plugins` | Plugin system, runtime manager, manifest loader |
| **themis** | `src/themis/` | `themis` | Root namespace, edition/license, module loader, wire protocol |
| **utils** | `src/utils/` | `themis::utils` | Logging, UUID, compression, PII detection, basic utilities |

### Integrated Modules (T1–T4) — Database Functionality

These 62 modules provide complete database functionality and are built statically by default. Some (geo, timeseries) can optionally be externalized as public plugins with fallback to integrated source.

**T1–T2: Core Engine** (7 modules)
- aql, cache, execution, index, metadata, query, storage

**T3–T4: Infrastructure & Governance** (55 modules)
- acceleration, access_model, ai, analytics, api, auth, cdc, chaos, chimera, config, content, distributed_knowledge, distributed_tensor, document, ethics_ai, evaluation, exporters, failover, geo, governance, gpu, graph, importers, ingestion, llama_cpp, llm, llm_wiki, maintenance, network, observability, onnx_clip, performance, process, projects, prompt_engineering, rag, replication, retrieval, rpc_grpc, scheduler, scraper, search, security, server, sharding, stable_diffusion, temporal, tensor, timeseries, toolbox, training, transaction, updates, user_storage_encrypted, voice, whisper

**Wave-1 Candidates for Externalization (Private Plugins):**
- `ethics_ai` → `themisdb_ethic_ai` (private plugin)
- `user_storage_encrypted` → part of `themisdb_storage` (private plugin aggregate)
- `importers` → `themisdb_importer` (private plugin aggregate with mysql, mongo, kafka, s3)
- `llm_wiki` → `themisdb_llm_wiki` (private plugin)

**Wave-1 Candidates for Externalization (Public Plugins):**
- `geo` → `themisdb_geo` (public optional plugin, integrated fallback)
- `timeseries` → `themisdb_timeseries` (public optional plugin, integrated fallback)

---

## Public Plugins (T5) — Optional Runtime-Loadable Extensions

These modules are built as optional, dynamically loadable shared libraries. The public plugin SDK is defined in `include/plugins/` and enforced via manifest metadata.

| Plugin Name | Location | Purpose | Edition Availability |
|-------------|----------|---------|----------------------|
| **blob_storage** | `plugins/blob_storage/` | Generic blob storage (S3, Azure, local) | Community+ |
| **cuda** | `plugins/cuda/` | CUDA acceleration backend | Enterprise+ |
| **ethics_ai** | `plugins/ethics_ai/` | Reference ethics evaluation impl | Community+ (private version: Enterprise+) |
| **exporters** | `plugins/exporters/` | Export backends (Parquet, Arrow, CSV, JSON, JSONL) | Community+ |
| **huggingface** | `plugins/huggingface/` | HuggingFace model ingestion | Community+ |
| **image_analysis** | `plugins/image_analysis/` | Image analysis backends (ONNX CLIP reference) | Community+ |
| **importers** | `plugins/importers/` | Import drivers (PostgreSQL, MySQL, etc.) | Community+ (private version: Enterprise+) |
| **llama_cpp** | `plugins/llama_cpp/` | llama.cpp inference backend | Community+ (private version optimized: Enterprise+) |
| **rpc** | `plugins/rpc/` | Adapter factory for database adapters | Community+ |
| **saga** | `plugins/saga/` | SAGA transaction coordinator | Community+ |
| **scraper** | `plugins/scraper/` | Web scraper integrations | Community+ (private version: Enterprise+) |
| **stable_diffusion** | `plugins/stable_diffusion/` | Stable Diffusion image generation | Community+ (private version: Enterprise+) |
| **user_storage_encrypted** | `plugins/user_storage_encrypted/` | Reference field-level encryption impl | Community+ (private version: Enterprise+) |
| **whisper** | `plugins/whisper/` | OpenAI Whisper STT backend | Community+ (private version: Enterprise+) |
| **themisdb_plugin_signer** | `plugins/themisdb_plugin_signer/` | Plugin manifest signing/verification | Enterprise+ |

---

## Externalized Plugins (Wave-1 & Later)

Both private and certain public plugins are maintained in separate repositories and included as optional commit-pinned submodules. They are referenced in `.gitmodules` and loaded conditionally based on edition and build flags.

### Private Plugins (Wave-1 Externalized)

These are only available in Enterprise, Hyperscaler, and Military editions.

| Repository | Submodule Path | Purpose | Edition | Wave |
|------------|----------------|---------|---------|------|
| `makr-code/themisdb_ethic_ai` | `plugins/themisdb_ethic_ai/` | Private ethics_ai plugin implementation (optimized) | Enterprise+ | Wave 1 |
| `makr-code/themisdb_storage` | `plugins/themisdb_storage/` | Aggregate: user_storage_encrypted, azure_blob_storage, s3_blob_storage (optimized) | Enterprise+ | Wave 1 |
| `makr-code/themisdb_importer` | `plugins/themisdb_importer/` | Aggregate: mysql_importer, mongo_importer, kafka_importer, s3_importer (optimized) | Enterprise+ | Wave 1 |
| `makr-code/themisdb_llm_wiki` | `plugins/themisdb_llm_wiki/` | Private LLM Wiki plugin implementation (optimized) | Enterprise+ | Wave 1 |
| *(gpu-impact-analysis)* | *(planned)* | Private GPU impact analysis (deferred) | Enterprise+ | Wave 2+ |

### Public Plugins (Externalized for Better Versioning)

These are available in all editions via external repository management.

| Repository | Submodule Path | Purpose | Edition | Wave |
|------------|----------------|---------|---------|------|
| `makr-code/themisdb_plugin_signer` | `plugins/themisdb_plugin_signer/` | Plugin manifest signing/verification tool | Enterprise+ | Wave 1 |
| `makr-code/themisdb_geo` | `plugins/themisdb_geo/` | Public optional geospatial plugin (externalized from geo module) | Community+ | Wave 1 |
| `makr-code/themisdb_timeseries` | `plugins/themisdb_timeseries/` | Public optional timeseries plugin (externalized from timeseries module) | Community+ | Wave 1 |

**Plugin Strategy:**
- Private plugins use the same public SDK interface defined in `include/plugins/`
- CMake honors `WITH_PRIVATE_*` toggles (defaults to `OFF` for Community/Minimal editions)
- Private manifests declare `visibility: private`, `allowed_editions: [enterprise, hyperscaler, military]`
- Integrated sources (in `src/`) provide reference implementations for all editions
- Externalized plugins are commit-pinned via `.gitmodules` and updated with submodule commands
- Edition-specific builds selectively include/exclude private plugins at configuration time

---

## Module Duplication: Integrated vs. Plugin Versions

Some modules have both integrated (in `src/`) and plugin (in `plugins/` or externalized) implementations, enabling flexible deployment models:

| Module | Integrated Source | Public Plugin | Private Plugin | Usage Model |
|--------|-------------------|---------------|----------------|-------------|
| **ethics_ai** | `src/ethics_ai/` | `plugins/ethics_ai/` (ref impl) | `plugins/themisdb_ethic_ai/` | Integrated by default; private optimized version in Enterprise+ |
| **geo** | `src/geo/` | `plugins/themisdb_geo/` (externalized) | *(none)* | Integrated by default; public externalized for independent versioning (Community+) |
| **user_storage_encrypted** | `src/user_storage_encrypted/` | `plugins/user_storage_encrypted/` (ref impl) | `plugins/themisdb_storage/` | Integrated by default; private optimized in Enterprise+ |
| **importers** | `src/importers/` | `plugins/importers/` (PostgreSQL ref) | `plugins/themisdb_importer/` | Integrated by default; private MySQL/Mongo/Kafka/S3 in Enterprise+ |
| **exporters** | `src/exporters/` | `plugins/exporters/` (Parquet/Arrow/CSV/JSON) | *(none)* | Integrated by default; also available as runtime plugin |
| **llama_cpp** | `src/llama_cpp/` | `plugins/llama_cpp/` (ref binding) | *(optimized)* | Integrated by default; better versioning as plugin |
| **llm_wiki** | `src/llm_wiki/` | *(none)* | `plugins/themisdb_llm_wiki/` | Integrated by default; private optimized in Enterprise+ |
| **scraper** | `src/scraper/` | `plugins/scraper/` (ref impl) | `plugins/themisdb_*` (future) | Integrated by default; plugin isolation for security |
| **stable_diffusion** | `src/stable_diffusion/` | `plugins/stable_diffusion/` (ref impl) | `plugins/themisdb_*` (future) | Integrated by default; plugin isolation for safety |
| **timeseries** | `src/timeseries/` | `plugins/themisdb_timeseries/` (externalized) | *(none)* | Integrated by default; public externalized for independent versioning (Community+) |
| **whisper** | `src/whisper/` | `plugins/whisper/` (ref impl) | `plugins/themisdb_*` (future) | Integrated by default; plugin isolation for versioning |

**Rationale for Duplication:**
1. **Reference Implementations**: Public plugins in `plugins/` serve as reference implementations for the public SDK
2. **Integrated Fallback**: Core `src/` versions ensure Community/Minimal can function without runtime plugins
3. **Private Optimization**: Private repos in Wave 1 provide Enterprise-exclusive optimizations or features
4. **Public Externalization**: Some modules (geo, timeseries) are externalized for independent versioning while maintaining integrated fallbacks
5. **Runtime Flexibility**: Users can choose to load optimized or externalized plugin versions at runtime instead of static linking

---

## Build Configuration

### CMake Flags & Edition Strategy

**Core Tier (Always Built):**
```cmake
# T0 core is always available, non-optional
add_library(themisdb_core ...)
```

**Integrated Modules (Configurable):**
```cmake
# T1–T4 integrated modules are built by default, optional per-module control
option(WITH_GEO "Enable integrated geo module" ON)
option(WITH_TIMESERIES "Enable integrated timeseries module" ON)
option(THEMIS_EXTERNALIZE_GEO_PLUGIN "Use geo as optional plugin instead" OFF)
option(THEMIS_EXTERNALIZE_TIMESERIES_PLUGIN "Use timeseries as optional plugin instead" OFF)
```

**Public Plugins (Optional):**
```cmake
# Public plugins are optional, controlled by edition/build variant
option(BUILD_PLUGINS "Build public plugin ecosystem" ON)
option(BUILD_PLUGIN_BLOB_STORAGE "Build blob storage plugin" ON)
# ... per-plugin toggles
```

**Private Plugins (Optional, Edition-Gated):**
```cmake
# Private plugins are optional, only active in Enterprise/Hyperscaler/Military
option(WITH_PRIVATE_ETHICS_AI "Enable private ethics_ai plugin" OFF)
option(WITH_PRIVATE_STORAGE "Enable private storage/encryption plugin" OFF)
option(WITH_PRIVATE_IMPORTER "Enable private importer suite" OFF)
# Automatically OFF for Community/Minimal editions
```

### Edition Availability

| Tier | Minimal | Community | Enterprise | Hyperscaler | Military |
|------|---------|-----------|------------|-------------|----------|
| **T0 Core** | ✅ All | ✅ All | ✅ All | ✅ All | ✅ All |
| **T1–T4 Integrated** | ✅ Subset (geo, timeseries optional) | ✅ All | ✅ All | ✅ All | ✅ All |
| **T5 Public Plugins** | ⚠️ Limited | ✅ All | ✅ All | ✅ All | ✅ All |
| **Private Plugins** | ❌ None | ❌ None | ✅ All Wave 1 | ✅ All Wave 1 | ✅ All Wave 1 |

---

## Source Code Organization

### Directory Structure Semantics

```
src/                          # Integrated modules (T0–T4)
  core/                       # T0 trusted bootstrap
  base/                       # T0 RAII & resource primitives
  themis/                     # T0 root namespace, edition logic
  utils/                      # T0 utilities
  plugins/                    # T0 plugin system
  
  # T1–T4 database functionality (57 modules)
  query/                      # T1 query engine
  storage/                    # T1 storage backend
  index/                      # T1 indexing
  # ... all other modules

include/                      # Public headers (both core + integrated)
  core/                       # Core headers
  plugins/                    # Public plugin SDK
  <module>/                   # Public API per integrated module

plugins/                      # T5 public plugins (optional, runtime-loaded)
  blob_storage/               # Public reference implementations
  exporters/
  importers/
  # ... other public plugins
  
  private/                    # Private plugin submodules (Wave 1+)
    themisdb_ethic_ai/       # Private submodule (if checked out)
    themisdb_storage/        # Private submodule (if checked out)
    # ... other private submodules
```

---

## Plugin SDK & Interface Boundaries

### Public Plugin SDK

**Location:** `include/plugins/plugin_interface.h`, `include/plugins/manifest_schema_v2.json`

**Principles:**
1. All plugins (public and private) implement the same C++ interface contract
2. Manifest metadata declares visibility, edition allowance, and license features
3. Plugin manager applies fail-closed validation: unsupported edition → plugin disabled
4. No implicit trust of plugin manifests; explicit capability checks required

**SDK Hierarchy:**
```cpp
// Public interface (language-agnostic C linkage)
extern "C" {
  IPlugin* create_plugin(const PluginInitRequest& req);
  void destroy_plugin(IPlugin* plugin);
}

// C++ interface
namespace themis::plugins {
  class IPlugin {
  public:
    virtual Status initialize(const PluginConfig& cfg) = 0;
    virtual PluginCapabilities capabilities() const = 0;
    virtual Status shutdown() = 0;
    // ... plugin-specific methods
  };
}
```

### Manifest Metadata (v2 Schema)

```json
{
  "name": "themisdb_ethic_ai",
  "version": "1.0.0",
  "visibility": "private",  // or "public"
  "allowed_editions": ["enterprise", "hyperscaler", "military"],
  "min_themisdb_version": "2.4.0",
  "required_features": ["llm_inference", "knowledge_graph"],
  "license_features": ["AI_ETHICS_EVAL"],
  "core_abi_compatible": "2.4.x",
  "dependencies": ["themisdb_llm_wiki>=1.0.0"]
}
```

---

## Plugin Loading Strategy

### Public Plugins (Community+)

1. Plugins in `plugins/` are discovered at startup
2. Manifest is validated; if visibility is "private" and edition is Community/Minimal, plugin is disabled
3. Capability checks determine which operations are allowed
4. Plugins load in sandboxed RPC context with fail-closed error handling

### Private Plugins (Enterprise+)

1. If `.gitmodules` includes private submodule and `WITH_PRIVATE_*=ON`, submodule is checked out
2. Private manifest declares `visibility: private` and `allowed_editions: [enterprise, ...]`
3. Plugin manager loads private plugin with higher trust (internal verified signatures)
4. Private plugins have access to proprietary features (e.g., Azure credentials, private APIs)

### Fallback Behavior

- If private plugin is absent or disabled, integrated `src/` version is used
- If integrated version is absent/disabled, fallback behavior (error or stub) is defined per module
- Community/Minimal editions never attempt to load private plugins

---

## Migration Path: Integrated → Plugin

**Goal:** Progressively externalize Wave-1 modules (ethics_ai, user_storage_encrypted, importers, llm_wiki) into private plugins without breaking Community.

**Phase 1 (Current):**
- Integrated source remains in `src/` (always compiled)
- Public reference plugins in `plugins/` (optional at runtime)
- Private plugins planned/provisioned in separate repos

**Phase 2 (Q4 2026):**
- Integrated `src/ethics_ai`, `src/user_storage_encrypted` → mostly CAI/LLM seams
- Private `themisdb_ethic_ai`, `themisdb_storage` fully featured and commit-pinned
- Externalization flags (`THEMIS_EXTERNALIZE_*`) default to `OFF` but configurable

**Phase 3 (Q1 2027):**
- Community/Minimal can omit integrated versions if private submodules not checked out
- Enterprise/Hyperscaler default to private submodule versions
- Integrated fallback seams removed once private repos mature

---

## Design Decisions & Rationales

### Q: Why have duplicates (same module in `src/` and `plugins/`)?

**A:** 
1. **Reference Implementations**: Public plugins in `plugins/` show users how to extend ThemisDB
2. **Integrated Fallback**: Community/Minimal editions need no runtime plugin loading; static linking is simpler
3. **Gradual Externalization**: Private repos can evolve independently while core remains stable
4. **Version Flexibility**: Users can run newer/older plugin versions without rebuilding core

### Q: Why externalize ethics_ai and user_storage_encrypted?

**A:**
1. These are compliance/governance/security modules best served by licensed, audited vendors
2. Private plugins allow enterprise customers to substitute their own implementations
3. Keeps core engine neutral; policy decisions belong to deployment layer

### Q: Why keep geo/timeseries integrated by default?

**A:**
1. Core spatial/temporal queries are essential; no fallback strategy is acceptable
2. Integrated versions are highly optimized and tightly coupled to query engine
3. Externalization is opt-in (`THEMIS_EXTERNALIZE_GEO_PLUGIN=ON`) for users who want to swap backends

### Q: How does Community avoid private plugin loading?

**A:**
- Private manifest declares `allowed_editions: [enterprise, ...]`
- Plugin manager checks edition at load time; if mismatch, plugin is disabled
- `.gitmodules` only includes private submodules if explicitly checked out (not by default)
- CI gates prevent Community release lanes from including private artefacts or credentials

---

## Reference Documents

- **ARCHITECTURE.md** — System-wide layered architecture (API, Query, Storage, Distributed)
- **ROADMAP.md** — Feature milestones; Wave 7 baseline; private plugin externalization timeline
- **FUTURE_ENHANCEMENTS.md** — Open enhancement backlog; private plugin phase work
- **src/plugins/ROADMAP.md** — Plugin system design and roadmap
- **plugins/README.md** — Plugin development guide
- **BRANCHING_STRATEGY.md** — Branch/edition governance
- **VERSIONING.md** — Semantic versioning and release types
- **RELEASE_STRATEGY.md** — Release lane and packaging strategy

---

**Zuletzt geprueft (Architecture Classification):** 2026-08-07
