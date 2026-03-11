# ThemisDB Architecture Audit

**Version:** 1.1  
**Audit Date:** 2026-03-10  
**Repository:** makr-code/ThemisDB  
**Scope:** Core Modules in `/src` Directory (develop @ 32369f28107f228bd572c91e0c13c54b3c622bbb)  
**Reference:** ARCHITECTURE.md

---

## Executive Summary

This audit evaluates the architecture of ThemisDB's core modules located in the `/src` directory against the architectural guidelines defined in `ARCHITECTURE.md`. The audit focuses on module organization, layer assignment, namespace strategy, interface design, and identifies potential risks and improvement opportunities. This version (v1.1) updates the baseline audit (v1.0, 2026-02-07) to reflect the current state of the `develop` branch as of 2026-03-10.

**Key Findings:**
- ✅ **Strong modular organization** with a growing set of well-defined components
- ✅ **Clear layered architecture** with 10 distinct layers remains intact
- ✅ **Dual-namespace strategy is intentional and documented** (`themis::` for core, `themisdb::` for extended subsystems); the prior finding of "inconsistency" is superseded - see Section 3
- ✅ **New platform capabilities visible**: configuration subsystem (`config/`), internal shared API surface (`themis/`), GPU feature flags, distributed health checks with mTLS signals
- ⚠️ **Module inventory has expanded**: new top-level directories observed under `src/` (`config/`, `ingestion/`, `prompt_engineering/`, `training/`, `themis/`) are not reflected in the prior audit baseline
- ⚠️ **Limited explicit interface definitions** - interface abstractions exist in places (e.g., `IMLServingBackend` in analytics/ML serving), but no centralized interface catalog exists
- ⚠️ **Documentation gaps** persist for several new and existing modules (see DOCS-009)

**Overall Assessment:** The architecture remains modular and layered, with visible maturation in configuration management, GPU/platform internals, and namespace documentation. The main action items are documentation and governance alignment: the architecture docs and this audit must reflect the current module inventory, document the intentional `themis` vs `themisdb` namespace split, and establish enforceable dependency rules to prevent architectural erosion.

---

## 1. Module Overview and Structure

### 1.1 Module Inventory

ThemisDB's `/src` directory contains a growing set of modules. The prior audit baseline cited 41 core modules and 575 implementation files; file counts have not been recomputed in this audit pass (exact counts require a complete recursive enumeration and are not fabricated here). The module list below reflects the top-level directories observed under `src/` as of develop @ 32369f28107f228bd572c91e0c13c54b3c622bbb.

**Modules present in v1.0 baseline (unchanged):**

| Module | Primary Purpose | Layer |
|--------|-----------------|-------|
| **llm/** | LLM integration, inference, LoRA framework | LLM Integration Layer |
| **server/** | API handlers, protocols, HTTP/gRPC servers | API & Protocol Layer |
| **sharding/** | Distributed coordination, consensus algorithms | Distributed & Sharding Layer |
| **utils/** | Logging, utilities, compression, PII detection | Cross-cutting |
| **index/** | Vector, graph, spatial indexing | Index & Vector Layer |
| **security/** | Encryption, RBAC, audit logging | Cross-cutting |
| **content/** | Multimodal data ingestion and processing | Content & Data Processing Layer |
| **query/** | AQL parser, optimizer, execution engine | Query Processing Layer |
| **storage/** | RocksDB wrapper, blob storage, transactions | Storage Layer |
| **rag/** | RAG evaluation, faithfulness, bias detection | LLM Integration Layer |
| **acceleration/** | GPU & hardware backends (CUDA, HIP, Vulkan) | Cross-cutting |
| **performance/** | Advanced data structures, optimizations | Cross-cutting |
| **timeseries/** | Time series compression, aggregates | Specialized Processing |
| **governance/** | Policy engine, compliance, versioning | Governance & Compliance Layer |
| **analytics/** | OLAP, process mining, diff engine | Analytics & Observability Layer |
| **plugins/** | Plugin system, hot-plugging, RPC | Cross-cutting |
| **transaction/** | ACID transactions, SAGA pattern | Transaction Layer |
| **observability/** | Metrics, profiling, alerting | Analytics & Observability Layer |
| **updates/** | Hot reload, manifest management | Cross-cutting |
| **cache/** | Semantic caching, query caching | Cross-cutting |
| **network/** | Wire protocol, socket management | API & Protocol Layer |
| **geo/** | Geospatial query processing | Specialized Processing |
| **core/** | Security init, concerns context | Core Infrastructure |
| **auth/** | JWT, GSSAPI, MFA authentication | API & Protocol Layer |
| **api/** | GraphQL API, HTTP server setup | API & Protocol Layer |
| **voice/** | Voice assistant integration | Specialized Processing |
| **scheduler/** | Task scheduling, retention | Specialized Processing |
| **graph/** | Property graphs, path constraints | Index & Vector Layer |
| **chimera/** | Database adapter factory | Cross-cutting |
| **cdc/** | Change Data Capture, changefeeds | Distributed & Sharding Layer |
| **aql/** | AQL-specific handlers | Query Processing Layer |
| **temporal/** | Temporal conflict resolution | Specialized Processing |
| **search/** | Hybrid search (vector + full-text) | Index & Vector Layer |
| **replication/** | Multi-master replication | Distributed & Sharding Layer |
| **metadata/** | Schema management | Storage Layer |
| **importers/** | Data import (PostgreSQL) | Content & Data Processing Layer |
| **gpu/** | GPU-specific memory management | Cross-cutting |
| **exporters/** | Data export formats | Content & Data Processing Layer |
| **base/** | Core module loader | Core Infrastructure |

**Modules newly observed in this audit pass (not in v1.0 baseline):**

| Module | Primary Purpose | Preliminary Layer |
|--------|-----------------|-------------------|
| **config/** | Configuration resolution, schema validation, metrics exporter | Platform / Cross-cutting |
| **ingestion/** | Data ingestion pipeline | Content & Data Processing Layer |
| **prompt_engineering/** | Prompt construction, optimization | LLM Integration Layer (see note below) |
| **training/** | ML model training pipeline, LoRA, calibration | LLM Integration Layer (see note below) |
| **themis/** | Internal shared API surface, GPU sub-headers | Platform / Core Infrastructure |

**Note on `prompt_engineering/` and `training/`:** These modules may warrant classification as a separate "ML Ops / Training" layer rather than being subsumed into the LLM Integration Layer. See Section 2.2 for the recommendation.

**Top-level non-module files in `src/`:** `main.cpp`, `main_server.cpp`, `demo_encryption.cpp`, `stubs.cpp`, `version.h`, `README.md`

**Assessment:** The module inventory has expanded since v1.0. The architecture reference (`ARCHITECTURE.md`) and this audit should be kept in sync with the current module set. The newly present modules (`config/`, `themis/`) are foundational platform modules and require explicit governance rules (see Section 5 and Section 10).

### 1.2 Module Size Distribution

Per-module file counts have not been recomputed in this audit pass. A full recursive enumeration is required to produce accurate numbers; spot-checks confirm substantial implementation footprint across the codebase. The distribution categories from v1.0 remain broadly applicable, with `config/` introducing multiple header-heavy components and `themis/` serving as a shared include-style namespace root.

**Recommendation:** Regenerate the file count table using a script or CI job that walks `src/` recursively and outputs per-module `.cpp`/`.h` counts, and delta vs. prior audit. This is consistent with DOCS-009 (document undocumented `src/` subdirectories) in `docs/Audit/DOCS_AUDIT_ISSUE_BACKLOG.md`.

**Large Modules (>50 files, based on v1.0 baseline):**
- `llm/` - Extensive LLM capabilities with LoRA, vision, inference
- `server/` - 40+ API handlers for different domains
- `sharding/` - Complex distributed coordination logic

**Medium Modules (10-50 files, based on v1.0 baseline):**
- `utils/`, `index/`, `security/`, `content/`, `query/`, `storage/`, `rag/`, `acceleration/`, `performance/`, `timeseries/`, `governance/`

**Small Modules (1-9 files, based on v1.0 baseline):**
- Remaining focused modules with specific, well-defined responsibilities

**Assessment:** The distribution remains healthy. The newly identified modules require enumeration in the next audit pass to establish their size baseline.

---

## 2. Layer Assignment Analysis

### 2.1 Documented Layer Architecture

According to `ARCHITECTURE.md`, ThemisDB defines 10 architectural layers:

1. **API & Protocol Layer** (themis::server::*)
2. **Query Processing Layer** (themis::query::*)
3. **Index & Vector Layer** (themis::index::*)
4. **LLM Integration Layer** (themis::llm::*)
5. **Storage Layer** (themis::storage::*)
6. **Distributed & Sharding Layer** (themis::sharding::*)
7. **Transaction Layer** (themis::transaction::*)
8. **Content & Data Processing Layer** (themis::content::*)
9. **Analytics & Observability Layer** (themis::analytics::*, themis::observability::*)
10. **Governance & Compliance Layer** (themis::governance::*)

### 2.2 Module-to-Layer Mapping

| Layer | Modules | Compliance Status |
|-------|---------|------------------|
| **1. API & Protocol** | api/, server/, network/, auth/ | ✅ Complete - Well-defined |
| **2. Query Processing** | query/, aql/ | ✅ Complete - Well-defined |
| **3. Index & Vector** | index/, search/, graph/ | ✅ Complete - Well-defined |
| **4. LLM Integration** | llm/, rag/ | ✅ Complete - Well-defined |
| **5. Storage** | storage/, metadata/ | ✅ Complete - Well-defined |
| **6. Distributed & Sharding** | sharding/, replication/, cdc/ | ✅ Complete - Well-defined |
| **7. Transaction** | transaction/ | ✅ Complete - Single focused module |
| **8. Content & Data Processing** | content/, importers/, exporters/, ingestion/ | ✅ ingestion/ added (see note) |
| **9. Analytics & Observability** | analytics/, observability/ | ✅ Complete - Well-defined |
| **10. Governance & Compliance** | governance/ | ✅ Complete - Single focused module |

**Cross-Cutting Concerns (span multiple layers):**
- acceleration/ - GPU backends used across layers
- cache/ - Caching used across layers
- security/ - Security used across layers
- utils/ - Utilities used across layers
- performance/ - Performance optimizations across layers
- plugins/ - Plugin system across layers
- updates/ - Hot reload across layers
- base/ - Core initialization
- core/ - Core concerns context
- gpu/ - GPU memory management
- chimera/ - Database adapters

**Platform / Infrastructure modules (newly categorized; require explicit governance):**
- config/ - Configuration resolution, schema validation, metrics exporter; foundational for all modules
- themis/ - Internal shared API surface used by GPU components and other subsystems

**Specialized Processing Modules:**
- timeseries/, temporal/, scheduler/, geo/, voice/

**Newly observed modules - pending formal layer assignment:**
- prompt_engineering/, training/ - See Recommendation 2 below

**Recommendation 1 (updated):** Add a "Cross-Cutting Concerns" and "Platform / Infrastructure" section to `ARCHITECTURE.md` to explicitly document modules that span multiple layers, with special attention to `config/` and `themis/` as foundational platform elements with strict downward-dependency requirements.

**Recommendation 2 (new):** Clarify the layer classification for `prompt_engineering/` and `training/`. These may be:
- part of the LLM Integration Layer (if tightly coupled to inference),
- part of Content & Data Processing (if viewed as data transformation pipelines), or
- a dedicated "ML Ops / Training" layer.

A documented decision should be added to `ARCHITECTURE.md`.

### 2.3 Layer Compliance Assessment

✅ **Strengths:**
- All 10 documented layers have corresponding module implementations
- Clear separation between API, Query, Storage, and Distributed layers
- Well-organized LLM integration as a dedicated layer
- Transaction and Governance as focused, single-responsibility modules

⚠️ **Observations:**
- Cross-cutting concerns (12 modules) are not explicitly addressed in the layer architecture documentation
- Platform modules (`config/`, `themis/`) are now present but not yet described in `ARCHITECTURE.md`
- Specialized processing modules (5 modules) could benefit from clearer layer classification
- The relationship between layers is documented in flow diagrams but not in explicit dependency rules
- `prompt_engineering/` and `training/` require a formal layer assignment decision

---

## 3. Namespace Organization Analysis

### 3.1 Documented Namespace Hierarchy

`ARCHITECTURE.md` documents two primary root namespaces:
- `themis::` - Primary root namespace (core database functionality, ~90% of codebase)
- `themisdb::` - Secondary root namespace (specialized/extended subsystems: RAFT consensus, replication orchestration, temporal, analytics serving)

The dual-root approach is intentional and documented in detail in:
- [`docs/implementation-history/DUAL_NAMESPACE_EXPLAINED.md`](../../implementation-history/DUAL_NAMESPACE_EXPLAINED.md)
- [`docs/de/architecture/namespace-architektur.md`](../../de/architecture/namespace-architektur.md)

The prior finding of "mixed namespace usage = inconsistency" in v1.0 is superseded. The correct characterization is: **documented dual-root model with clear allocation criteria**.

### 3.2 Actual Namespace Usage

**`themis::` namespace (primary) - found in source:**
- `themis::acceleration`
- `themis::content`
- `themis::errors`
- `themis::exporters`
- `themis::geo`
- `themis::llm`
- `themis::network`
- `themis::query`
- `themis::rag`
- `themis::security`
- `themis::server`
- `themis::sharding` (standard sharding)
- `themis::utils`
- `themis::auth`

**`themisdb::` namespace (extended) - found in headers:**
- `themisdb::sharding` - RAFT consensus system
- `themisdb::temporal` - Temporal conflict resolution
- `themisdb::analytics` - ML serving and model backends (e.g., `IMLServingBackend`)
- `themisdb::replication` - Replication orchestration

### 3.3 Namespace Compliance Assessment (Updated)

✅ **Strengths:**
- Dual-namespace strategy has explicit rationale documented in implementation history and German architecture docs
- The `themis` / `themisdb` split serves as a natural API surface indicator (core vs. extended subsystems)
- Namespace allocation criteria are described and can be enforced via code review

⚠️ **Remaining risks:**
- Without automated enforcement, individual contributors may still choose roots inconsistently
- `ARCHITECTURE.md` does not yet reference the dual-namespace decision docs; readers may encounter the split without context

**Recommendation 3 (updated):** Replace the prior "namespace inconsistency" finding with the following action:
- Ensure `ARCHITECTURE.md` links to the namespace decision tree in `docs/implementation-history/DUAL_NAMESPACE_EXPLAINED.md` and `docs/de/architecture/namespace-architektur.md`
- Define which subsystems belong under `themisdb::` in the main architecture reference
- Consider a linting rule or CI check to flag new files that do not match the documented allocation

**Recommendation 4 (retained):** Add namespace declaration examples for each layer in the architectural documentation.

---

## 4. Interface and API Design Analysis

### 4.1 Documented Interface Patterns

`ARCHITECTURE.md` describes an "Interface-Based Design" pattern with pluggable implementations:
- `QueryInterface` - Pluggable query engines
- `IndexInterface` - Different indexing strategies
- `StorageInterface` - Multiple storage backends
- `ConsensusInterface` - Various consensus protocols

### 4.2 Actual Interface Implementation

**Interfaces found in codebase (examples):**
- `BulkUploadInterface` - content/pipeline (base class for bulk upload strategies)
- `AsyncBulkUploader : public BulkUploadInterface` - async bulk upload implementation
- `IMLServingBackend` - analytics/ML serving (`include/analytics/ml_serving.h`), with factory methods creating ONNX/TF backends; `MLServingClient` is a concrete consumer
- Plugin-like registration patterns in LLM/LoRA and other subsystems
- `config/` subsystem components (resolver, schema validator, metrics exporter) present separable contracts

**Assessment:**
⚠️ **Interface abstractions exist but are not consolidated**

Interface-style abstractions are present and growing (e.g., `IMLServingBackend`), but they are defined locally in module headers without a centralized catalog. This makes it difficult to discover which stable contracts exist, and which are internal implementation details.

### 4.3 Dependency Management

**Documented Patterns:**
- Namespace isolation for clear dependencies
- Plugin architecture via PluginManager
- Consensus abstraction via ConsensusFactory
- SAGA pattern for distributed transactions
- Adaptive optimization in QueryOptimizer

**Assessment:**
The documentation describes sophisticated patterns (ConsensusFactory, PluginManager, SagaManager), but explicit dependency injection and interface boundaries need verification through deeper code analysis.

### 4.4 Interface Assessment

⚠️ **Concerns:**
- Lack of explicit interface definitions in public headers may lead to tight coupling
- Difficulty in testing and mocking components without clear interfaces
- Pluggability may be limited without interface abstractions
- Unclear boundaries between modules

**Recommendation 5 (updated):** Create `docs/architecture/interface-catalog.md` enumerating all interfaces and stable contracts:
- Storage backend interfaces
- Index provider interfaces
- Query function interfaces / registries
- Consensus / sharding abstractions
- ML serving backend interface(s) (e.g., `IMLServingBackend`)
- Plugin points and lifecycle hooks
- Follow naming convention (e.g., `IQueryEngine`, `IStorageBackend`, `IIndexProvider`) consistently across all new interfaces

**Recommendation 6:** Add architecture decision records (ADRs) to document:
- Why certain components use interfaces vs concrete classes
- Design patterns chosen for each layer
- Dependency injection strategy
- Testing and mocking approaches

---

## 5. Module Dependencies and Coupling

### 5.1 Expected Dependencies (from ARCHITECTURE.md diagrams)

```
Core/Utils → Storage → {Index, Cache} → Query → Server
                    ↓
               Sharding/Distributed
                    ↓
               Replication
```

LLM and other advanced features depend on Storage and may integrate at the Query or Server layers.

### 5.2 Cross-Module Dependencies

**High Dependency Modules (likely dependencies):**
- `utils/` - Used by almost all modules
- `core/` - Core initialization, security, concerns context
- `storage/` - Used by index, query, transaction, sharding
- `security/` - Used by server, auth, storage, governance
- `config/` (new) - Foundational configuration resolution; must not depend on upper layers
- `themis/` (new) - Shared internal API surface; risk of becoming a dependency hub if ungoverned

**Potential Circular Dependencies:**
- Server layer may depend on Query, Storage, LLM, and other layers
- Query layer may call back to Server for distributed query execution
- Sharding may depend on Storage, which may depend on Sharding for distributed operations
- `config/` and `themis/` must be kept as "bottom" layers with no upward dependencies

### 5.3 Coupling Assessment

⚠️ **Risks:**
- Large modules (llm/, server/, sharding/) may have high coupling
- Cross-cutting concerns (security, utils, acceleration) may create implicit dependencies
- Lack of explicit interfaces may hide coupling
- `config/` and `themis/` becoming dependency hubs if not governed

**Recommendation 7 (updated):** Create a dependency map and define explicit dependency rules:
- Use tools to analyze `#include` dependencies between modules
- Document allowed and disallowed dependencies between layers
- Establish dependency rules: `config/`, `utils/`, `core/`, `themis/`, `base/` are "bottom" layers; they must not depend on `server/`, `query/`, or other upper layers
- Add dependency validation to build system (e.g., CMake module dependency checks or include-graph CI checks)

**Recommendation 8:** Refactor large modules:
- Consider splitting `llm/` (124 files) into sub-modules (inference, lora, vision, etc.)
- Consider splitting `server/` (77 files) into protocol-specific sub-modules
- Ensure each sub-module has a clear, focused responsibility

---

## 6. Build and Edition Configuration

### 6.1 Edition Architecture

`ARCHITECTURE.md` documents four build editions:
- **MINIMAL** - Basic database functionality
- **COMMUNITY** - Adds replication and basic AI
- **ENTERPRISE** - Full-featured with advanced AI and security
- **HYPERSCALER** - Maximum scale and resilience

### 6.2 Module-to-Edition Mapping

Based on `ARCHITECTURE.md` documentation and current module inventory:

| Module | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|--------|---------|-----------|------------|-------------|
| core, base, utils, config | ✓ | ✓ | ✓ | ✓ |
| query, storage, metadata | ✓ | ✓ | ✓ | ✓ |
| server, api, network, auth | ✓ | ✓ | ✓ | ✓ |
| index (basic) | ✓ | ✓ | ✓ | ✓ |
| replication, sharding (Raft) | | ✓ | ✓ | ✓ |
| llm (llama.cpp) | | ✓ | ✓ | ✓ |
| index (vector CPU) | | ✓ | ✓ | ✓ |
| acceleration (GPU), gpu | | | ✓ | ✓ |
| security (field encryption, RBAC, MFA) | | | ✓ | ✓ |
| governance, cdc | | | ✓ | ✓ |
| content (all formats) | | | ✓ | ✓ |
| observability (advanced) | | | ✓ | ✓ |
| sharding (Paxos, Gossip) | | | ✓ | ✓ |
| ingestion/ | | ✓ | ✓ | ✓ |
| training/, prompt_engineering/ | | | ✓ | ✓ |
| themis/ (internal API surface) | ✓ | ✓ | ✓ | ✓ |
| Advanced scaling features | | | | ✓ |

Note: Edition assignments for `ingestion/`, `training/`, `prompt_engineering/`, and `themis/` are preliminary; they should be confirmed and documented in `ARCHITECTURE.md` and the relevant module CMake configurations.

### 6.3 Edition Configuration Assessment

⚠️ **Gaps:**
- Module-to-edition mapping is not explicitly documented per module
- No clear indication in module code which edition features belong to
- Build configuration details not found in module level

**Recommendation 9:** Add edition metadata to modules:
- Document which modules/files belong to which edition in each module's README
- Use CMake options to conditionally compile edition-specific code
- Add edition markers in code comments or file naming
- Create an edition-to-module matrix in `ARCHITECTURE.md`

**Recommendation 10:** Implement feature flags:
- Use CMake feature flags (ENABLE_LLM, ENABLE_GPU, etc.) consistently
- Document feature dependencies (e.g., ENABLE_GPU requires ENABLE_ACCELERATION)
- Validate feature combinations at build time

---

## 7. Security and Compliance Architecture

### 7.1 Security Modules

**Primary Security Module:** `security/` (28 files)
- Encryption
- RBAC
- Audit logging
- Field-level encryption
- Key management

**Authentication Module:** `auth/` (3 files)
- JWT validation
- GSSAPI/Kerberos
- MFA

**Governance Module:** `governance/` (12 files)
- Policy engine
- Compliance reporting (GDPR, HIPAA, SOC2)
- Data lineage
- Version control

### 7.2 Cross-Cutting Security

Security concerns span multiple layers:
- Network layer (TLS 1.3 in network/)
- Storage layer (field-level encryption in storage/)
- Query layer (RBAC enforcement in query/)
- API layer (authentication in server/)
- LLM layer (ethical guidelines in llm/)
- Distributed layer: `src/sharding/health_check.cpp` references mTLS certificate validity checks, indicating security enforcement inside distributed health checks

### 7.3 Security Architecture Assessment

✅ **Strengths:**
- Dedicated security module with comprehensive features
- Separation of authentication and authorization
- Governance as first-class architectural concern
- Security integrated at multiple layers

⚠️ **Potential Gaps:**
- Security interface abstractions not clearly defined
- Cross-layer security coordination mechanism not documented
- Security policy enforcement points not explicitly mapped

**Recommendation 11:** Document security architecture:
- Create a security reference architecture diagram showing all security touchpoints
- Document security policy enforcement points in each layer
- Define security interfaces and contracts
- Add security design patterns (e.g., how RBAC is enforced in query execution)
- Document threat model and security boundaries

**Recommendation 12:** Add security validation:
- Implement security architecture tests
- Validate RBAC enforcement at build/test time
- Add security scanning tools to CI/CD pipeline
- Document security review process for new modules

---

## 8. Testing and Quality Architecture

### 8.1 Test Infrastructure

`ARCHITECTURE.md` references test directories:
- `tests/unit/` - Unit tests
- `tests/integration/` - Integration tests
- `benchmarks/` - Performance tests
- `fuzz/` - Fuzz tests

**Notable observation:** `src/stubs.cpp` exists for link-time stub purposes. This is an architectural smell if it grows, as stubs can mask missing interface implementations. The policy for this file should be documented and tracked.

**Recommendation 13a:** Document the stub policy explicitly:
- `src/stubs.cpp` is allowed only for test harness linkage where real implementations are not yet available
- Track stub count (number of stub functions / lines) as a metric; any increase requires justification
- Create an Architecture Decision Record (ADR) for the stub file lifecycle
- Add policy to `ARCHITECTURE.md` under the testing section

### 8.2 Testing Gaps

⚠️ **Observations:**
- No clear documentation of test coverage expectations per module
- No documented testing strategy for each architectural layer
- Unclear how to test cross-cutting concerns (security, caching, acceleration)
- No documented mocking/stubbing strategy for interface testing

**Recommendation 13 (updated):** Enhance testing architecture:
- Document test coverage goals per module (e.g., 80% for core modules)
- Define testing strategy for each layer (unit, integration, end-to-end)
- Create test doubles/mocks for major interfaces (enabled by Recommendation 5 interface catalog)
- Add test architecture documentation to `ARCHITECTURE.md`
- Document how to test distributed scenarios (sharding, replication)
- Document the policy for `src/stubs.cpp`: allowed only for test harness linkage; must be tracked and reduced over time

**Recommendation 14:** Add test infrastructure:
- Create test utilities for common scenarios (e.g., database setup, LLM mocking)
- Add integration test templates for new modules
- Implement contract tests for interfaces
- Add performance test baselines for critical paths

---

## 9. Documentation Architecture

### 9.1 Current Documentation

**Architecture Documentation:**
- `ARCHITECTURE.md` - Comprehensive main document
- Module READMEs in several modules (acceleration/, analytics/, auth/, etc.)
- `docs/implementation-history/DUAL_NAMESPACE_EXPLAINED.md` - Namespace decision rationale
- `docs/de/architecture/namespace-architektur.md` - German-language namespace architecture
- `docs/de/architecture/` - Extensive German-language architecture docs covering caching patterns, content pipeline, multi-model, security architecture, and more

**Strengths:**
- Detailed architectural overview
- Clear layer definitions
- Good visual diagrams (Mermaid)
- Request flow documentation
- Namespace decision documented in implementation history

### 9.2 Documentation Gaps

⚠️ **Missing Documentation:**
- Not all modules have README files (directly tracked by DOCS-009 in `docs/Audit/DOCS_AUDIT_ISSUE_BACKLOG.md`)
- Newly present modules (`config/`, `ingestion/`, `prompt_engineering/`, `training/`, `themis/`) are not yet documented in `ARCHITECTURE.md` or with in-tree READMEs
- Interface contracts and API documentation
- Dependency rules between layers
- Security architecture deep-dive for distributed mTLS
- Testing architecture
- Module-specific architecture decisions
- Migration guides between editions
- Performance characteristics per module
- Troubleshooting per module

**Recommendation 15 (updated):** Enhance module documentation as tracked in DOCS-009:
- Add README.md to all `src/` modules that still lack one, starting with newly added modules (`config/`, `ingestion/`, `prompt_engineering/`, `training/`, `themis/`)
- Include in each README:
  - Module purpose and responsibilities
  - Public API/interfaces
  - Dependencies
  - Edition availability
  - Testing approach
  - Performance characteristics
  - Known limitations

This recommendation aligns directly with DOCS-009 ("Document all undocumented `src/` subdirectories") in `docs/Audit/DOCS_AUDIT_ISSUE_BACKLOG.md`.

**Recommendation 16 (updated):** Add supplementary architecture documents:
- Security Architecture Document (including distributed/mTLS section)
- Testing Architecture Document
- Performance Architecture Document
- Deployment Architecture Document
- Interface Catalog (see Recommendation 5)
- Dependency Map
- Edition Feature Matrix (updated with new modules)
- Ensure `ARCHITECTURE.md` references `docs/implementation-history/DUAL_NAMESPACE_EXPLAINED.md` and `docs/de/architecture/namespace-architektur.md`

---

## 10. Identified Risks and Issues

### 10.1 High-Priority Risks

| Risk ID | Risk | Impact | Likelihood | Mitigation Priority |
|---------|------|--------|-----------|-------------------|
| **R01** | Lack of centralized interface catalog - tight coupling and difficult mocking | High | Medium | High |
| **R02** | Module inventory drift - new modules not reflected in audit/ARCHITECTURE.md | Medium | High | High |
| **R03** | Dual-namespace strategy not consistently reflected in `ARCHITECTURE.md` | Low | Medium | Medium |
| **R04** | `config/` and `themis/` becoming ungoverned dependency hubs | High | Medium | High |
| **R05** | No documented dependency rules may lead to circular dependencies | High | Low | Medium |
| **R06** | Security architecture not fully documented (esp. distributed/mTLS) | High | Low | High |
| **R07** | Testing strategy not documented per layer | Medium | Medium | Medium |
| **R08** | Documentation drift - new modules undocumented (DOCS-009) | Medium | High | Medium |
| **R09** | Link-time stub file (`src/stubs.cpp`) may mask missing interfaces | Medium | Medium | Medium |

### 10.2 Technical Debt

| Item | Description | Recommended Action |
|------|-------------|-------------------|
| **TD01** | Interface abstractions not consolidated | Create interface catalog (Recommendation 5) |
| **TD02** | Namespace strategy documentation gap | Link to existing namespace docs from `ARCHITECTURE.md` |
| **TD03** | Module splitting (large modules) | Refactor large modules (llm/, server/) into sub-modules |
| **TD04** | Dependency mapping absent | Create and maintain module dependency map (Recommendation 7) |
| **TD05** | Module documentation gaps | Add READMEs to all modules, esp. new ones (DOCS-009) |
| **TD06** | Edition configuration for new modules | Add `config/`, `themis/`, `ingestion/`, `training/`, `prompt_engineering/` to edition matrix |

### 10.3 Architectural Inconsistencies (Updated)

| Inconsistency | Current State | Expected State | Action |
|--------------|---------------|----------------|--------|
| **I01** | Limited centralized interface catalog | Interface-based design documented | Create interface catalog doc |
| **I02** | Dual namespaces not cross-referenced in ARCHITECTURE.md | Dual-root model documented in implementation history | Add cross-reference links in ARCHITECTURE.md |
| **I03** | Cross-cutting and platform concerns not in layer architecture | Documented as separate concern | Add Platform/Infrastructure section in ARCHITECTURE.md |
| **I04** | `prompt_engineering/` and `training/` layer assignment TBD | Formal layer assignment | Document layer decision in ARCHITECTURE.md |

---

## 11. Improvement Recommendations

### 11.1 Immediate Actions (Next Sprint)

1. **Update module inventory in `ARCHITECTURE.md`** to include `config/`, `themis/`, `ingestion/`, `training/`, `prompt_engineering/` [R: 2 hours]
2. **Align namespace documentation**: ensure `ARCHITECTURE.md` references `docs/implementation-history/DUAL_NAMESPACE_EXPLAINED.md` and `docs/de/architecture/namespace-architektur.md` [R: 1 hour]
3. **Add Platform modules section** in `ARCHITECTURE.md` covering `config/` and `themis/` as foundational infrastructure with explicit dependency constraints [R: 2 hours]
4. **Begin module READMEs for new modules** (DOCS-009): `config/`, `themis/`, `ingestion/`, `training/`, `prompt_engineering/` [R: 1-2 days]

### 11.2 Short-Term Improvements (Next Quarter)

5. **Create an Interface Catalog** document (`docs/architecture/interface-catalog.md`) [R: 1 week]
6. **Define and enforce dependency rules** between layers; add CI validation [R: 1 week]
7. **Document security architecture** in detail, including distributed/mTLS section [R: 1 week]
8. **Update edition-to-module mapping matrix** with new modules [R: 2 days]
9. **Document `prompt_engineering/` and `training/` layer assignment** in `ARCHITECTURE.md` [R: 2 hours]
10. **Implement Architecture Decision Records (ADRs)** [R: ongoing]
11. **Add test architecture documentation** [R: 1 week]
12. **Regenerate module file count table** using a script or CI job that walks `src/` recursively and outputs per-module `.cpp`/`.h` counts plus delta vs. prior audit; include output in next audit pass [R: 2 hours]

### 11.3 Long-Term Improvements (Next 6-12 Months)

11. **Refactor large modules** (llm/, server/, sharding/) into maintainable sub-modules [R: 1-2 months]
12. **Implement dependency validation** in build system [R: 2 weeks]
13. **Create comprehensive interface abstractions** across all layers [R: 2-3 months]
14. **Establish continuous architecture governance** process [R: ongoing]
15. **Performance architecture documentation** per module [R: 1 month]
16. **Security architecture validation** framework [R: 1 month]

### 11.4 Recommended Architecture Enhancements

**Enhancement 1: Interface Layer**
- Create a dedicated `interfaces/` directory in `include/`
- Define base interfaces for all pluggable components
- Document interface lifecycle and contracts

**Enhancement 2: Module Registry**
- Implement runtime module registry for plugin management
- Support dynamic feature enablement based on edition
- Add module health checks and monitoring

**Enhancement 3: Dependency Injection**
- Implement dependency injection container
- Reduce coupling through constructor injection
- Improve testability with mock implementations

**Enhancement 4: Architecture Testing**
- Implement architecture fitness functions
- Add layer dependency tests
- Validate module boundaries in CI/CD

**Enhancement 5: Documentation as Code**
- Generate module dependency diagrams from code
- Auto-generate interface documentation
- Create architecture visualization dashboard

---

## 12. Compliance with Architecture Principles

### 12.1 Documented Core Principles

From `ARCHITECTURE.md`:
1. **Modularity** - Optional components, selectable at build time
2. **Layered Architecture** - Clear separation between API, Query, Storage, and Distributed concerns
3. **Namespace Organization** - Logical grouping using C++ namespaces
4. **High Performance** - GPU acceleration, SIMD optimizations, adaptive indexing
5. **Enterprise Ready** - ACID transactions, encryption, audit logging, observability

### 12.2 Principle Compliance Assessment

| Principle | Compliance | Evidence | Score |
|-----------|-----------|----------|-------|
| **Modularity** | ✅ Good | Well-defined modules, edition system, growing module set | 8/10 |
| **Layered Architecture** | ✅ Good | 10 clear layers, most modules assigned; new modules need formal assignment | 8/10 |
| **Namespace Organization** | ✅ Good | Dual-root approach documented; ARCHITECTURE.md should link to decision docs | 8/10 |
| **High Performance** | ✅ Good | Dedicated acceleration/, performance/ modules, GPU feature flags maturing | 8/10 |
| **Enterprise Ready** | ✅ Good | Security, governance, observability modules present; distributed mTLS signals | 8/10 |

**Overall Compliance Score: 8.0/10**

### 12.3 Additional Architectural Qualities

| Quality Attribute | Assessment | Score |
|------------------|-----------|-------|
| **Maintainability** | Good structure, some large modules | 7/10 |
| **Testability** | Limited interface abstractions | 6/10 |
| **Extensibility** | Plugin system, unclear interfaces | 7/10 |
| **Scalability** | Dedicated sharding/distributed modules | 9/10 |
| **Security** | Comprehensive security modules | 8/10 |
| **Documentation** | Good overview, gaps in details | 7/10 |

---

## 13. Conclusion

### 13.1 Overall Assessment

ThemisDB demonstrates a **well-architected, modular database system** with clear separation of concerns and comprehensive feature coverage. The modules are thoughtfully organized into 10 architectural layers, supporting multiple data models, native AI/LLM capabilities, and enterprise-grade features.

**Strengths:**
- Excellent modular organization with focused responsibilities
- Clear layered architecture with good separation
- Comprehensive feature set across all layers
- Strong support for distributed operations and AI integration
- Good documentation foundation in `ARCHITECTURE.md`
- Dual-namespace strategy is intentional and documented

**Areas for Improvement:**
- Interface abstractions need to be consolidated into a catalog
- Newly present modules (`config/`, `themis/`, `ingestion/`, `training/`, `prompt_engineering/`) need formal layer assignment and documentation
- Cross-cutting and platform concerns need explicit architectural documentation in `ARCHITECTURE.md`
- Testing and security architectures need detailed documentation
- Dependency management and validation need enhancement

### 13.2 Architecture Maturity

**Current Maturity Level: 3/5 (Defined)**

The architecture is well-defined with documented principles and patterns, but not yet standardized with enforced governance and continuous validation. The addition of `config/` and `themis/` platform modules represents a step toward a more structured infrastructure layer, but governance rules for these modules are not yet in place.

**Path to Level 4 (Managed):**
- Implement explicit interfaces and interface catalog
- Add architecture fitness functions
- Establish continuous architecture governance
- Enforce dependency rules in CI
- Document platform module constraints (`config/`, `themis/`)

**Path to Level 5 (Optimizing):**
- Continuous architecture monitoring
- Automated dependency analysis
- Performance-driven architecture evolution
- AI-assisted architecture optimization

### 13.3 Risk Summary

- **Critical Risks:** 0
- **High Risks:** 3 (interface catalog absent, platform module governance, security documentation)
- **Medium Risks:** 6 (module inventory drift, dependency rules, testing strategy, documentation drift/DOCS-009, stub file, namespace cross-reference)
- **Low Risks:** 0

All identified risks are manageable with the recommended improvements.

### 13.4 Recommended Next Steps

1. **Accept this audit** as updated architecture assessment (v1.1)
2. **Prioritize recommendations** based on team capacity and project goals
3. **Implement immediate actions** (recommendations 1-4) in next sprint
4. **Plan short-term improvements** (recommendations 5-11) for next quarter
5. **Schedule quarterly architecture reviews** to track progress
6. **Track technical debt** items in project management system
7. **Conduct deep-dive audits** for critical modules (llm/, server/, sharding/, security/, config/)
8. **Address DOCS-009** by adding READMEs to all undocumented `src/` subdirectories

### 13.5 Sign-Off

This architecture audit provides a comprehensive assessment of ThemisDB's core modules against documented architectural guidelines. The findings and recommendations serve as a roadmap for continuous architectural improvement and should be reviewed and updated quarterly.

**Audit Prepared By:** GitHub Copilot Coding Agent  
**Review Status:** Draft  
**Next Review Date:** 2026-06-10 (3 months)

---

## Appendix A: Module Dependency Matrix

This matrix captures the **expected and enforced dependency direction** between modules, derived from the layered architecture definition in `ARCHITECTURE.md` and the include-graph topology observed in `src/`. A full automated include-graph analysis has not been run in this audit pass; the table below reflects the documented and logical dependency model.

**Legend:**
- `-->` allowed/expected dependency (consumer → provider)
- `(dep)` foundational dependency used by almost every module
- `✗` prohibited direction (would create a layer violation)

### A.1 Layer Dependency Rules

| Consumer Module | Depends On (key providers) | Direction |
|-----------------|---------------------------|-----------|
| server/ | api/, network/, auth/, query/, security/, utils/, core/, config/ | top → bottom |
| api/ | network/, auth/, utils/, core/ | top → bottom |
| auth/ | security/, utils/, core/, config/ | consumer → provider |
| query/ | storage/, index/, cache/, utils/, core/, aql/ | top → bottom |
| aql/ | utils/, core/ | consumer → provider |
| index/ | storage/, acceleration/, gpu/, utils/, core/ | consumer → provider |
| search/ | index/, storage/, utils/ | consumer → provider |
| graph/ | index/, storage/, utils/ | consumer → provider |
| llm/ | storage/, index/, acceleration/, gpu/, utils/, core/ | consumer → provider |
| rag/ | llm/, storage/, index/, utils/ | consumer → provider |
| prompt_engineering/ | llm/, utils/ | consumer → provider |
| training/ | llm/, storage/, acceleration/, gpu/, utils/ | consumer → provider |
| storage/ | utils/, core/, config/ | consumer → provider |
| metadata/ | storage/, utils/ | consumer → provider |
| sharding/ | storage/, network/, security/, utils/, core/ | top → bottom |
| replication/ | storage/, network/, utils/ | consumer → provider |
| cdc/ | storage/, utils/ | consumer → provider |
| transaction/ | storage/, utils/, core/ | consumer → provider |
| content/ | storage/, utils/, core/ | consumer → provider |
| ingestion/ | content/, storage/, utils/ | consumer → provider |
| importers/ | storage/, utils/ | consumer → provider |
| exporters/ | storage/, utils/ | consumer → provider |
| analytics/ | storage/, query/, utils/, core/ | consumer → provider |
| observability/ | utils/, core/ | consumer → provider |
| governance/ | storage/, security/, utils/, core/ | consumer → provider |
| security/ | utils/, core/, config/ | consumer → provider |
| acceleration/ | gpu/, utils/ | consumer → provider |
| gpu/ | utils/ | consumer → provider |
| cache/ | storage/, utils/ | consumer → provider |
| performance/ | utils/ | consumer → provider |
| plugins/ | utils/, core/ | consumer → provider |
| updates/ | storage/, utils/ | consumer → provider |
| chimera/ | storage/, utils/ | consumer → provider |
| timeseries/ | storage/, utils/ | consumer → provider |
| temporal/ | storage/, utils/ | consumer → provider |
| scheduler/ | utils/, core/ | consumer → provider |
| geo/ | index/, acceleration/, utils/ | consumer → provider |
| voice/ | llm/, utils/ | consumer → provider |
| **config/** | utils/ (only) | **platform bottom layer** |
| **themis/** | (no src/ dependencies; provides headers only) | **platform bottom layer** |
| **utils/** | (no src/ dependencies) | **foundation** |
| **core/** | utils/ | **foundation** |
| **base/** | utils/ | **foundation** |

### A.2 Prohibited Dependencies (Layer Violations)

The following dependency directions are **prohibited** and should be caught by CI include-graph checks:

| If | Depends On | Violation Reason |
|----|-----------|------------------|
| config/ | server/, query/, storage/, llm/ | Platform bottom cannot depend on upper layers |
| themis/ | server/, query/, sharding/ | Shared API surface must not pull in top layers |
| utils/ | Any non-stdlib dependency | Foundation layer must remain dependency-free |
| core/ | server/, query/ | Core infrastructure must stay below application layers |
| storage/ | server/, query/ | Storage layer must not depend on higher layers |

### A.3 Known Coupling Risks (to Validate)

| Risk | Modules | Recommended Action |
|------|---------|-------------------|
| Potential circular: sharding ↔ storage | sharding/, storage/ | Verify with automated include-graph scan |
| Potential upward dep from config/ | config/ | Validate no `#include <server/...>` or `<query/...>` |
| `themis/` growing into a "god header" | themis/ | Enforce minimal public surface; audit quarterly |
| LLM layer reaching into server/ | llm/ | Ensure llm/ does not include server/ headers directly |

## Appendix B: Interface Catalog

This catalog lists all interface types (abstract base classes using the `I` prefix convention) found in `include/` as of develop @ 32369f28107f228bd572c91e0c13c54b3c622bbb. Interfaces are grouped by owning module.

**Note:** This catalog was generated by scanning `include/` for `class I[A-Z]...` declarations. It covers headers under `include/`. Interfaces defined only in source files or using other naming conventions (pure abstract base classes without the `I` prefix) are not included here.

### B.1 Core Infrastructure Interfaces (`include/core/`, `include/themis/`)

These are the foundation interfaces used across all layers.

| Interface | Header | Description |
|-----------|--------|-------------|
| `IContext` | `core/concerns/i_context.h` | Cross-cutting context carrier (request context, cancellation) |
| `ILogger` | `core/concerns/i_logger.h` | Synchronous logging contract |
| `IAsyncLogger` | `core/concerns/i_async_logger.h` | Asynchronous logging (extends ILogger) |
| `IAuditLog` | `core/concerns/i_audit_log.h` | Immutable audit log write contract |
| `IMetrics` | `core/concerns/i_metrics.h` | Metrics emission contract |
| `ITracer` | `core/concerns/i_tracer.h` | Distributed trace span factory |
| `ICache` | `core/concerns/i_cache.h` | Synchronous cache contract |
| `IAsyncCache` | `core/concerns/i_async_cache.h` | Async cache (extends ICache) |
| `IEvictionStrategy` | `core/concerns/cache_strategies.h` | Cache eviction policy |
| `ICircuitBreaker` | `core/concerns/i_circuit_breaker.h` | Circuit breaker pattern |
| `IFeatureFlags` | `core/concerns/i_feature_flags.h` | Feature flag evaluation |
| `ISecrets` | `core/concerns/i_secrets.h` | Secret / credential provider |
| `IQueryEngine` | `themis/base/interfaces/query_interface.h` | Core query execution engine |
| `IQueryEngineFactory` | `themis/base/interfaces/query_interface.h` | Factory for query engine instances |
| `IExpressionEvaluator` | `themis/base/interfaces/query_interface.h` | Expression evaluation contract |
| `IStorageEngine` | `themis/base/interfaces/storage_interface.h` | Core storage engine |
| `IStorageEngineFactory` | `themis/base/interfaces/storage_interface.h` | Factory for storage engine instances |
| `IIndexManager` | `themis/base/interfaces/index_interface.h` | Index lifecycle management |
| `IFieldEncryption` | `themis/base/interfaces/security_interface.h` | Field-level encryption contract |
| `IFieldEncryptionFactory` | `themis/base/interfaces/security_interface.h` | Factory for field encryption instances |
| `IKeyProvider` | `themis/base/interfaces/security_interface.h` | Cryptographic key provider |
| `IKeyProviderFactory` | `themis/base/interfaces/security_interface.h` | Factory for key provider instances |
| `IWasmRuntime` | `themis/base/wasm_runtime_injector.h` | WASM runtime injection contract |
| `IAllocator` | `utils/memory/pool_allocator.h` | Memory allocator contract |

### B.2 API & Protocol Layer Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IHttpHandler` | `api/http_handler.h` | HTTP request handler |
| `IWebSocketHandler` | `api/websocket_handler.h` | WebSocket connection handler |
| `IWebSocketFrameCallback` | `api/websocket_handler.h` | WebSocket frame event callback |
| `IGRPCBridge` | `api/grpc_bridge.h` | gRPC bridge/proxy contract |
| `IGraphQLSchemaBuilder` | `api/graphql_schema_builder.h` | GraphQL schema construction |
| `IAPIVersionRouter` | `api/api_version_router.h` | API version routing |
| `ICorrelationIDProvider` | `api/correlation_id.h` | Correlation ID generation |
| `ILLMPlugin` | `llm/llm_plugin_interface.h` | LLM provider plugin contract (also referenced in `server/llm_api_handler.h`) |

### B.3 Query Processing Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IFunction` | `query/functions/function_registry.h` | AQL user-defined function contract |
| `IAgent` | `aql/aql_agent.h` | ReAct-style AQL query agent |

### B.4 Index & Vector Layer Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IAnnIndex` | `index/ann_index.h` | Approximate nearest-neighbor index |

### B.5 Storage Layer Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IBlobStorageBackend` | `storage/blob_storage_backend.h` | Blob/object storage backend |

### B.6 Distributed & Sharding Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IShardRouter` | `sharding/sharding_interfaces.h` | Shard routing policy |
| `ICrossShardQueryRouter` | `sharding/sharding_interfaces.h` | Cross-shard query routing |
| `IAdaptiveRebalancer` | `sharding/sharding_interfaces.h` | Adaptive shard rebalancer |
| `IConsistentHashRing` | `sharding/sharding_interfaces.h` | Consistent hashing ring |
| `IDistributedTxCoordinator` | `sharding/sharding_interfaces.h` | Distributed transaction coordination |
| `IRaftSnapshotManager` | `sharding/sharding_interfaces.h` | RAFT snapshot lifecycle |
| `IStreamListener` | `sharding/stream_protocol.h` | Shard stream event listener |
| `IConflictResolver` | `replication/replication_manager.h` | Replication conflict resolution |
| `IReplicationListener` | `replication/replication_manager.h` | Replication event listener |
| `ICDCTransport` | `cdc/icdc_transport.h` | CDC change event transport |
| `ISchemaRegistryBackend` | `cdc/schema_registry.h` | Schema registry backend |
| `IGlobalRegionParticipant` | `transaction/global_transaction_manager.h` | Global distributed transaction participant |

### B.7 Cache Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `ICacheCoordinator` | `cache/cache_replication_coordinator.h` | Distributed cache coordination |
| `ICacheReplicationListener` | `cache/cache_replication.h` | Cache replication event listener |

### B.8 LLM Integration Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `ILLMPlugin` | `llm/llm_plugin_interface.h` | LLM provider plugin |
| `IFeedbackPlugin` | `llm/i_feedback_plugin.h` | LLM feedback/RLHF plugin |
| `IFlashAttention` | `llm/attention/flash_attention.h` | Flash attention computation backend |
| `ISamplingStrategy` | `llm/sampling_strategy.h` | Token sampling strategy |
| `ITokenizer` | `llm/lora_framework/data_loader.h` | Tokenizer contract |
| `ITrainableLayer` | `llm/lora_framework/lora_layers.h` | LoRA/trainable layer contract |
| `ISignatureVerifier` | `llm/security/signature_verifier.h` | Model signature/integrity verifier |
| `ILLMProvider` | `prompt_engineering/meta_prompt_generator.h` | LLM provider for prompt engineering |
| `IEmbeddingProvider` | `prompt_engineering/prompt_evaluator.h` | Embedding vector provider |

### B.9 Analytics & ML Serving Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IMLServingBackend` | `analytics/ml_serving.h` | ML model serving backend (ONNX, TensorFlow, etc.) |
| `IAnalyticsExporter` | `analytics/analytics_export.h` | Analytics data exporter |

### B.10 Content & Data Processing Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IContentProcessor` | `content/content_processor.h` | Content processing pipeline stage |
| `IContentProcessorPlugin` | `content/content_plugin_interface.h` | Content processor plugin |
| `IImporter` | `importers/importer_interface.h` | Data import contract |
| `IImporterPlugin` | `importers/importer_interfaces.h` | Importer plugin |
| `IImporterPluginRegistry` | `importers/importer_interfaces.h` | Importer plugin registry |
| `IIncrementalImportCursor` | `importers/importer_interfaces.h` | Incremental import cursor |
| `IImportConflictResolver` | `importers/importer_interfaces.h` | Import conflict resolution |
| `IFlatFileSchemaDetector` | `importers/importer_interfaces.h` | Flat file schema auto-detection |
| `IKafkaConsumerSource` | `importers/importer_interfaces.h` | Kafka consumer data source |
| `IExporter` | `exporters/exporter_interface.h` | Data export contract |
| `IFormatTemplate` | `exporters/format_template.h` | Export format template |
| `ISourceConnector` | `ingestion/ingestion_manager.h` | Ingestion data source connector |
| `IIngestionWorkerNode` | `ingestion/ingestion_coordinator.h` | Ingestion worker node contract |
| `ILeaderElection` | `ingestion/ingestion_coordinator.h` | Leader election for ingestion coordinator |

### B.11 Acceleration & GPU Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IComputeBackend` | `acceleration/compute_backend.h` | Generic compute backend (base) |
| `IVectorBackend` | `acceleration/compute_backend.h` | Vector compute backend (extends IComputeBackend) |
| `IMatrixBackend` | `acceleration/compute_backend.h` | Matrix compute backend (extends IComputeBackend) |
| `IGeoBackend` | `acceleration/compute_backend.h` | Geo compute backend (extends IComputeBackend) |
| `IGraphBackend` | `acceleration/compute_backend.h` | Graph compute backend (extends IComputeBackend) |

### B.12 Geospatial Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IGeoRegistry` | `geo/spatial_backend.h` | Geospatial backend registry |
| `ISpatialComputeBackend` | `geo/spatial_backend.h` | Spatial compute backend |
| `IGeoOpsExtension` | `geo/geo_ops_ext.h` | Geospatial operations extension |

### B.13 Security Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IMalwareScanner` | `security/malware_scanner.h` | Malware/content scanning |
| `ITSAClient` | `security/tsa_api.h` | Timestamp Authority client |
| `IUserRegistrationPlugin` | `security/user_registration_plugin.h` | User registration extension |

### B.14 Governance Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IComplianceRule` | `governance/ccpa_rules.h` | Compliance rule contract (GDPR, CCPA, etc.) |

### B.15 Plugin System Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IThemisPlugin` | `plugins/plugin_interface.h` | Base plugin contract |
| `IStatefulPlugin` | `plugins/plugin_interface.h` | Stateful plugin (extends IThemisPlugin) |
| `IRPCPlugin` | `plugins/rpc_plugin_interface.h` | RPC plugin (extends IThemisPlugin) |
| `IRPCServer` | `plugins/rpc_plugin_interface.h` | RPC server contract |
| `IRPCMethodHandler` | `plugins/rpc_plugin_interface.h` | RPC method handler |
| `IImageAnalysisBackend` | `plugins/image_analysis_interface.h` | Image analysis backend |
| `ISelfHealingPlugin` | `plugins/self_healing_plugin.h` | Self-healing plugin contract |
| `IEthicsAIPlugin` | `plugins/ethics_ai/ethics_ai_plugin_interface.h` | Ethics AI plugin (extends IThemisPlugin) |

### B.16 Chimera (Database Adapter) Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IDatabaseAdapter` | `chimera/database_adapter.hpp` | Composite database adapter (extends IRelationalAdapter + others) |
| `IRelationalAdapter` | `chimera/database_adapter.hpp` | Relational database adapter |
| `IDocumentAdapter` | `chimera/database_adapter.hpp` | Document database adapter |
| `IGraphAdapter` | `chimera/database_adapter.hpp` | Graph database adapter |
| `IVectorAdapter` | `chimera/database_adapter.hpp` | Vector database adapter |
| `ITransactionAdapter` | `chimera/database_adapter.hpp` | Transaction adapter |
| `ISystemInfoAdapter` | `chimera/database_adapter.hpp` | System info adapter |

### B.17 Miscellaneous Interfaces

| Interface | Header | Description |
|-----------|--------|-------------|
| `IHealthCheck` | `updates/preflight_health_check.h` | Pre-flight / operational health check |
| `IPIIDetectionEngine` | `utils/pii_detection_engine.h` | PII detection and redaction |
| `ISessionPersistenceBackend` | `voice/voice_session_manager.h` | Voice session persistence |
| `ISpan` | `core/concerns/i_tracer.h` | Distributed trace span |

### B.18 Summary

| Category | Interface Count |
|----------|----------------|
| Core Infrastructure (`core/`, `themis/`) | 24 |
| API & Protocol | 8 |
| Query Processing | 2 |
| Index & Vector | 1 |
| Storage | 1 |
| Distributed & Sharding | 12 |
| Cache | 2 |
| LLM Integration | 9 |
| Analytics & ML Serving | 2 |
| Content & Data Processing | 14 |
| Acceleration & GPU | 5 |
| Geospatial | 3 |
| Security | 3 |
| Governance | 1 |
| Plugin System | 8 |
| Chimera / Database Adapters | 7 |
| Miscellaneous | 4 |
| **Total** | **106** |

**Gap noted:** A centralized `docs/architecture/interface-catalog.md` should be created that also documents each interface's lifecycle contract, stability level (stable / experimental), and known concrete implementations. This is Recommendation 5 from the main audit body.

## Appendix C: Edition Feature Matrix

This matrix is derived from the CMake edition configuration in `cmake/CMakeLists.txt` (lines 529–659). The build system defines four editions via `THEMIS_EDITION`: `MINIMAL`, `COMMUNITY`, `ENTERPRISE`, and `HYPERSCALER`.

**Legend:** ✓ = included/enabled by default | ✗ = excluded/disabled | (opt) = optional, requires explicit CMake flag | — = not applicable

### C.1 Infrastructure Limits

| Capability | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|-----------|---------|-----------|------------|-------------|
| Max GPU VRAM | None (0 GB) | 24 GB | 256 GB | Unlimited |
| Max Sharding Nodes | 1 (single-node) | 5 | 100 | Unlimited |
| Enterprise Plugins | ✗ | ✗ | ✓ | ✓ |
| License Required (Release) | ✗ | ✗ | ✓ | ✓ |

### C.2 Core Database Modules

| Module | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|--------|---------|-----------|------------|-------------|
| core/, base/, utils/ | ✓ | ✓ | ✓ | ✓ |
| config/ | ✓ | ✓ | ✓ | ✓ |
| themis/ | ✓ | ✓ | ✓ | ✓ |
| query/, aql/ | ✓ | ✓ | ✓ | ✓ |
| storage/, metadata/ | ✓ | ✓ | ✓ | ✓ |
| index/ (basic) | ✓ | ✓ | ✓ | ✓ |
| cache/ | ✓ | ✓ | ✓ | ✓ |
| transaction/ | ✓ | ✓ | ✓ | ✓ |
| server/, api/, network/ | ✓ | ✓ | ✓ | ✓ |
| auth/ (JWT/basic) | ✓ | ✓ | ✓ | ✓ |

### C.3 API Protocol Modules

| Protocol | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| HTTP/1.1 + REST | ✓ | ✓ | ✓ | ✓ |
| GraphQL | ✓ | ✓ | ✓ | ✓ |
| gRPC | ✗ | ✓ | ✓ | ✓ |
| WebSocket | ✗ | (opt) | (opt) | (opt) |
| HTTP/2 | ✗ | (opt) | (opt) | (opt) |
| HTTP/3 (QUIC) | ✗ | (opt) | (opt) | (opt) |
| MQTT | ✗ | (opt) | (opt) | (opt) |
| PostgreSQL Wire Protocol | ✗ | (opt) | (opt) | (opt) |
| SSE (Server-Sent Events) | ✓ | ✓ | ✓ | ✓ |
| MCP (Model Context Protocol) | ✗ | (opt) | (opt) | (opt) |

### C.4 Security & Compliance Modules

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| security/ (base) | ✓ | ✓ | ✓ | ✓ |
| Field-level encryption | ✗ | ✗ | ✓ | ✓ |
| RBAC | ✗ | ✗ | ✓ | ✓ |
| HSM (PKCS#11) | ✗ | ✗ | ✓ | ✓ |
| Multi-master replication auth | ✗ | ✗ | ✓ | ✓ |
| governance/ (compliance, GDPR/HIPAA/SOC2) | ✗ | ✓ | ✓ | ✓ |
| cdc/ (Change Data Capture) | ✗ | ✓ | ✓ | ✓ |
| OpenTelemetry tracing | ✗ | ✓ | ✓ | ✓ |

### C.5 Distributed & Scaling Modules

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| sharding/ (Raft, basic) | ✗ | ✓ (max 5 nodes) | ✓ (max 100 nodes) | ✓ (unlimited) |
| sharding/ (Paxos, Gossip) | ✗ | ✗ | ✓ | ✓ |
| replication/ (multi-master) | ✗ | ✗ | ✓ | ✓ |
| replication/ (single-master) | ✗ | ✓ | ✓ | ✓ |
| Kafka CDC producer | ✗ | (opt) | (opt) | (opt) |

### C.6 LLM & AI Modules

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| llm/ (llama.cpp, core inference) | ✗ | ✓ | ✓ | ✓ |
| rag/ | ✗ | ✓ | ✓ | ✓ |
| prompt_engineering/ | ✗ | ✓ | ✓ | ✓ |
| training/ (LoRA, calibration) | ✗ | ✗ | ✓ | ✓ |
| aql/ ReAct agent | ✗ | ✓ | ✓ | ✓ |
| MCP integration | ✗ | (opt) | (opt) | (opt) |

### C.7 GPU & Acceleration Modules

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| acceleration/ (CPU path) | ✓ | ✓ | ✓ | ✓ |
| gpu/ | ✗ | ✓ | ✓ | ✓ |
| Vulkan (cross-platform GPU) | ✗ | ✓ (default ON) | ✓ (default ON) | ✓ (default ON) |
| CUDA (NVIDIA) | ✗ | (opt, OFF default) | (opt, OFF default) | (opt, OFF default) |
| HIP (AMD) | ✗ | (opt) | (opt) | (opt) |
| DirectX 12 Compute | ✗ | (opt) | (opt) | (opt) |
| Metal (Apple) | ✗ | (opt) | (opt) | (opt) |
| OpenCL | ✗ | (opt) | (opt) | (opt) |
| ZLUDA | ✗ | (opt) | (opt) | (opt) |
| NCCL / RCCL (multi-GPU) | ✗ | ✗ | (opt) | ✓ |

### C.8 Content & Specialized Modules

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| content/ (basic) | ✓ | ✓ | ✓ | ✓ |
| content/ processors (audio/video/image/CAD) | ✗ | (opt) | ✓ | ✓ |
| importers/, exporters/ | ✓ | ✓ | ✓ | ✓ |
| ingestion/ | ✓ | ✓ | ✓ | ✓ |
| timeseries/ | ✗ | ✓ | ✓ | ✓ |
| temporal/ | ✗ | ✓ | ✓ | ✓ |
| geo/ | ✗ | ✓ | ✓ | ✓ |
| GDAL integration | ✗ | (opt) | (opt) | (opt) |
| analytics/ (OLAP) | ✗ | ✓ | ✓ | ✓ |
| DuckDB OLAP variant | ✗ | (opt) | (opt) | (opt) |
| observability/ | ✗ | ✓ | ✓ | ✓ |
| voice/ | ✗ | (opt) | (opt) | ✓ |
| Whisper STT | ✗ | (opt) | (opt) | (opt) |
| Piper TTS | ✗ | (opt) | (opt) | (opt) |
| OCR (Tesseract) | ✗ | (opt) | (opt) | (opt) |
| Office document support | ✓ | ✓ | ✓ | ✓ |
| PDF extraction | (opt) | (opt) | (opt) | (opt) |

### C.9 Performance Optimisation Options (All Editions, Optional)

These are optional flags that can be combined with any edition:

| Flag | Effect | Default |
|------|--------|---------|
| `THEMIS_ENABLE_JEMALLOC` | jemalloc allocator (best fragmentation) | OFF |
| `THEMIS_ENABLE_MIMALLOC` | mimalloc allocator (+10-20% perf) | OFF |
| `THEMIS_ENABLE_HUGE_PAGES` | Huge pages support (+15-30% memory perf) | OFF |
| `THEMIS_ENABLE_IO_URING` | io_uring zero-copy I/O (Linux ≥ 5.1) | OFF |
| `THEMIS_ENABLE_AVX2` | AVX2/FMA SIMD for Release builds | ON |
| `THEMIS_ENABLE_BWTREE` | Bw-Tree lock-free index (+100-200%) | OFF |
| `THEMIS_ENABLE_CICADA` | Cicada optimistic CC (+100-150%) | OFF |
| `THEMIS_ENABLE_DOSTOEVSKY` | Dostoevsky adaptive LSM (+25-35%) | OFF |
| `THEMIS_ENABLE_DISKANN` | DiskANN billion-scale vector search (+300-400%) | OFF |
| `THEMIS_ENABLE_SPLINTERDB` | SplinterDB concurrent compaction (-70% P99) | OFF |
| `THEMIS_ENABLE_WISCKEY` | WiscKey key/value separation (+40-60% writes) | OFF |
| `THEMIS_ENABLE_BAO` | Bao ML query optimizer (+30-70%) | OFF |
| `THEMIS_ENABLE_RCU_INDEX` | RCU read-heavy index paths (+200-500% reads) | OFF |
| `THEMIS_ENABLE_LIRS_CACHE` | LIRS cache replacement (+30-40% hit rate) | OFF |
| `THEMIS_ENABLE_PMEM` | Persistent Memory (Optane) layout (+50-200%) | OFF |
| `THEMIS_ENABLE_GUNROCK` | Gunrock GPU graph analytics (+1000-3000%) | OFF |
| `THEMIS_ENABLE_LIGRA` | Ligra graph processing (+200-300%) | OFF |
| `THEMIS_ENABLE_RABITQ` | RaBitQ vector quantization (16x memory reduction) | OFF |

## Appendix D: Security Touchpoint Map

This map shows where security controls are enforced across the architecture, derived from inspecting source files in `src/` for security-relevant patterns (encryption, TLS/mTLS, RBAC, authentication, certificate handling, audit logging, signing, and token processing).

**Scope:** Files in `src/<module>/` containing references to: `security`, `encrypt`, `tls`, `TLS`, `RBAC`, `rbac`, `auth`, `certif`, `mTLS`, `audit_log`, `sign`, `token`.

### D.1 Security Enforcement Density by Module

| Module | Security-Relevant Files | Primary Security Function |
|--------|------------------------|--------------------------|
| **security/** | 39 | Encryption, RBAC, audit logging, field-level encryption, key management, malware scanning, post-quantum crypto, secret management |
| **server/** | 75 | Authentication enforcement (JWT, GSSAPI, MFA), rate limiting, TLS termination, RBAC middleware, session management |
| **llm/** | 92 | Model signature verification, ethical guidelines enforcement, RLHF security, LoRA adapter integrity checks |
| **rag/** | 42 | RAG bias detection, hallucination prevention, faithfulness verification, ethics integration |
| **sharding/** | 42 | mTLS inter-shard communication, certificate validity checks in health checks, RAFT leader authentication |
| **auth/** | 27 | JWT validation, GSSAPI/Kerberos, MFA, principal validation, token issuance |
| **utils/** | 36 | PII detection, hash-chain audit logging, secure memory clearing, PII stream scanning |
| **governance/** | 19 | GDPR/HIPAA/SOC2 compliance policy engine, data lineage, retention policy enforcement |
| **content/** | 29 | Content signing, malware scanning at ingestion, digital watermarking |
| **index/** | 18 | Secure index access (RBAC on index operations) |
| **query/** | 18 | RBAC enforcement in query execution, parameterized query injection prevention |
| **storage/** | 23 | Field-level encryption at rest, encrypted blob storage, secure key rotation |
| **network/** | 13 | TLS 1.3 wire protocol, certificate pinning, secure socket management |
| **gpu/** | 11 | Secure VRAM allocation, shader integrity verification |
| **analytics/** | 11 | Analytics result access control, ML serving authentication |
| **acceleration/** | 11 | Shader integrity, GPU kernel signing (acceleration dispatch) |
| **voice/** | 15 | Speaker verification, TTS/STT session authentication |
| **aql/** | 13 | AQL injection prevention, query parameter sanitization, ReAct agent safety |
| **exporters/** | 10 | Export access control, data masking on export |
| **ingestion/** | 10 | Ingestion source authentication, data provenance |
| **prompt_engineering/** | 8 | Prompt injection prevention, output sanitization |
| **performance/** | 8 | Secure memory operations, side-channel-resistant implementations |
| **geo/** | 6 | Geospatial data access control |
| **cache/** | 6 | Cache poisoning prevention, PII eviction, HMAC-protected distributed cache |
| **base/** | 6 | Registry key pinning, secure module loading |
| **metadata/** | 6 | Schema access control |
| **cdc/** | 7 | CDC event signing, change feed authentication |
| **timeseries/** | 7 | Time series data access control |
| **updates/** | 9 | Hot reload manifest signing, update integrity verification |
| **importers/** | 8 | Import source authentication, schema validation |
| **scheduler/** | 7 | Task authorization, secure scheduling |
| **search/** | 7 | Search result access control |
| **config/** | 3 | Configuration secret handling, schema validation security |
| **themis/** | 4 | Core interface security contracts |
| **observability/** | 5 | Metrics access control, secure telemetry export |
| **chimera/** | 4 | Adapter authentication to external databases |
| **replication/** | 3 | Replication channel authentication |
| **transaction/** | 3 | Transaction authorization |
| **training/** | 3 | Training data access control, model provenance |
| **plugins/** | 5 | Plugin signature verification, sandboxed plugin execution |

### D.2 Security Control Classification by Layer

| Layer | Security Controls Present |
|-------|--------------------------|
| **API & Protocol** (server, api, network, auth) | TLS 1.3 termination, JWT/GSSAPI/MFA authentication, RBAC middleware, rate limiting, session management, certificate pinning |
| **Query Processing** (query, aql) | RBAC enforcement on query execution, parameterized queries, AQL injection prevention |
| **Index & Vector** (index, search, graph) | Index-level access control via RBAC |
| **LLM Integration** (llm, rag, prompt_engineering, training) | Model signature verification, ethical guidelines, bias detection, hallucination prevention, prompt injection defense |
| **Storage** (storage, metadata) | Field-level encryption (ENTERPRISE+), encrypted blob storage, key rotation |
| **Distributed & Sharding** (sharding, replication, cdc) | mTLS inter-shard channels, RAFT leader authentication, certificate validity health checks, change feed signing |
| **Transaction** (transaction) | Transaction authorization, SAGA compensation security |
| **Content & Data Processing** (content, importers, exporters, ingestion) | Malware scanning at ingestion, content signing, data masking on export, PII redaction |
| **Analytics & Observability** (analytics, observability) | Analytics access control, secure telemetry, RLHF monitoring |
| **Governance & Compliance** (governance) | GDPR, HIPAA, SOC2 policy evaluation; data lineage; retention enforcement |
| **Cross-cutting: security/** | Central: encryption engine, RBAC, audit log, key management, HSM, post-quantum crypto, secret manager |
| **Cross-cutting: utils/** | PII detection engine, hash-chain audit writer/verifier, secure memory utilities |
| **Cross-cutting: acceleration/gpu/** | Shader integrity, GPU kernel signing |

### D.3 Key Security Architecture Decisions

| Decision | Details |
|----------|---------|
| **TLS Strategy** | TLS 1.3 at network/ layer; mTLS for inter-shard communication in sharding/ |
| **Authentication** | JWT (default), GSSAPI/Kerberos (enterprise), MFA (enterprise); all via auth/ module |
| **Authorisation** | RBAC enforced at query/ and server/ layers; field-level in storage/ (ENTERPRISE+) |
| **Encryption at Rest** | Field-level encryption in storage/ (ENTERPRISE+); HSM integration (ENTERPRISE+) |
| **Audit Trail** | Hash-chain audit log in utils/; immutable `IAuditLog` interface |
| **Key Management** | `IKeyProvider`/`IKeyProviderFactory` in themis/base/interfaces/security_interface.h |
| **mTLS** | Certificate validity checks in sharding/health_check.cpp; operational hooks for expiry |
| **Post-Quantum Crypto** | Available in security/ (post_quantum_crypto.cpp) |
| **Model Integrity** | `ISignatureVerifier` in llm/security/; GGUF quantization security |
| **PII Protection** | `IPIIDetectionEngine` in utils/; PII eviction endpoint in server/ (AdminCachePiiEvictDelete) |

### D.4 Security Gaps / Recommendations

| Gap | Affected Modules | Recommended Action |
|-----|-----------------|-------------------|
| Distributed security not documented in ARCHITECTURE.md | sharding/, network/ | Add "Distributed Security" subsection documenting mTLS scope |
| Security interface contracts not in centralized catalog | security/, themis/ | Include in interface catalog (Recommendation 5) |
| Certificate expiry policy not documented | sharding/ | Document operational hooks and alerting policy |
| Plugin sandboxing extent not documented | plugins/ | Clarify sandbox model in ARCHITECTURE.md |

## Appendix E: Performance Characteristics

This appendix summarizes performance data, module sizes, and benchmark coverage for ThemisDB. Detailed numeric benchmark results are not included here (they depend on hardware configuration); this appendix documents the performance architecture and benchmark inventory.

### E.1 Module Implementation Size (Audit Pass: 2026-03-10)

File counts were computed recursively from `src/<module>/` at develop @ 32369f28.

| Module | .cpp / .cc Files | Notes |
|--------|-----------------|-------|
| **llm/** | 138 | Largest module; LLM inference, LoRA, vision, quantization |
| **server/** | 106 | API handlers, HTTP/gRPC, rate limiting, protocol handlers |
| **sharding/** | 79 | Distributed coordination, RAFT/Paxos/Gossip consensus |
| **rag/** | 46 | RAG pipeline, evaluation, faithfulness, hallucination detection |
| **utils/** | 45 | Logging, PII, audit, compression, hash-chain |
| **security/** | 39 | Encryption, RBAC, HSM, post-quantum, secret management |
| **content/** | 38 | Multimodal content processing pipeline |
| **index/** | 38 | Vector, ANN, secondary, graph indexing |
| **query/** | 34 | AQL parser, optimizer, execution engine |
| **gpu/** | 30 | GPU memory management, VRAM allocator |
| **storage/** | 32 | RocksDB wrapper, blob storage, WAL |
| **performance/** | 25 | Advanced data structures, memory optimizations |
| **acceleration/** | 24 | GPU backends: CUDA, HIP, Vulkan, Metal, DirectX |
| **auth/** | 27 | JWT, GSSAPI, MFA, principal validation |
| **analytics/** | 20 | OLAP, process mining, ML serving, diff engine |
| **governance/** | 21 | Policy engine, GDPR/HIPAA/SOC2, data lineage |
| **voice/** | 19 | Voice assistant, STT/TTS integration |
| **exporters/** | 15 | Export formats (Arrow, Parquet, CSV, JSON, etc.) |
| **aql/** | 16 | AQL handlers, ReAct agent |
| **network/** | 14 | Wire protocol, socket management |
| **search/** | 16 | Hybrid search (vector + full-text) |
| **updates/** | 14 | Hot reload, manifest management |
| **timeseries/** | 15 | Time series compression (Gorilla), aggregation |
| **prompt_engineering/** | 14 | Prompt construction, optimization, meta-prompts |
| **geo/** | 13 | Geospatial query processing, spatial indexing |
| **cdc/** | 13 | Change Data Capture, changefeeds |
| **metadata/** | 12 | Schema management, collection metadata |
| **observability/** | 10 | Metrics, profiling, alerting, OpenTelemetry |
| **plugins/** | 10 | Plugin system, hot-plugging, ethics AI |
| **ingestion/** | 10 | Ingestion pipeline, coordinator, worker nodes |
| **cache/** | 11 | Semantic caching, distributed cache, HMAC |
| **transaction/** | 9 | ACID transactions, SAGA pattern, distributed TX |
| **chimera/** | 9 | Database adapter factory (PostgreSQL, MySQL, Neo4j, ...) |
| **scheduler/** | 9 | Task scheduling, retention |
| **temporal/** | 9 | Temporal conflict resolution, bi-temporal |
| **api/** | 9 | GraphQL, HTTP setup, WebSocket, gRPC bridge |
| **training/** | 7 | Training pipeline, LoRA, calibration, provenance |
| **base/** | 8 | Core module loader, dependency resolver, registry client |
| **core/** | 6 | Security init, concerns context |
| **replication/** | 6 | Multi-master replication orchestration |
| **graph/** | 5 | Property graphs, path constraints |
| **config/** | 4 (.cpp) + 8 (.h) | Configuration resolver, schema validator, metrics exporter |
| **themis/** | 6 | Internal shared API surface headers |
| **importers/** | 11 | Data import (PostgreSQL, CSV, Parquet, Kafka, ...) |
| **Total (approx.)** | ~895 | Across all modules; excludes `main*.cpp`, `stubs.cpp` |

### E.2 Benchmark Coverage

ThemisDB maintains an extensive benchmark suite in `benchmarks/`. Key benchmarks by category:

**Storage & Query Performance:**
- `bench_crud.cpp` - Basic CRUD throughput (inserts, reads, updates, deletes)
- `bench_query.cpp` - AQL query engine performance
- `bench_storage_performance.cpp` - Storage layer latency and throughput
- `bench_mvcc.cpp` - MVCC (Multi-Version Concurrency Control) overhead
- `bench_transaction_throughput.cpp` - Transaction throughput under contention
- `bench_tpcc.cpp`, `bench_tpch.cpp` - TPC-C and TPC-H standard benchmarks
- `bench_ycsb.cpp` - YCSB (Yahoo! Cloud Serving Benchmark) workloads

**Vector & Index Performance:**
- `bench_vector_search.cpp` - Vector similarity search latency
- `bench_binary_quantization.cpp`, `bench_product_quantization.cpp`, `bench_residual_quantization.cpp` - Vector quantization benchmarks
- `bench_simd_distance.cpp` - SIMD-accelerated distance computation
- `bench_hnsw_prefilter_minimal.cpp` - HNSW pre-filtered search
- `bench_index_rebuild.cpp` - Index rebuild throughput

**Distributed & Sharding:**
- `bench_sharding_performance.cpp` - Shard routing and distribution
- `bench_shard_routing.cpp` - Consistent hash ring performance
- `bench_replication_throughput.cpp` - Replication write amplification
- `bench_distributed_coordinator.cpp` - Coordination protocol overhead
- `bench_stream_protocol.cpp` - Shard-to-shard stream throughput

**GPU & Acceleration:**
- `bench_gpu_backends.cpp` - GPU backend comparison (Vulkan vs CUDA vs HIP)
- `bench_cuda_vs_cpu.cpp` - GPU vs CPU performance ratio
- `bench_gpu_vector_index.cpp` - GPU-accelerated vector indexing
- `bench_fused_kernels.cpp`, `bench_fused_lora_kernels.cpp` - Kernel fusion benchmarks
- `bench_mixed_precision_perf.cpp` - FP16/BF16 vs FP32 comparison
- `bench_multi_gpu_scaling.cpp` - Multi-GPU scaling efficiency

**LLM & RAG:**
- `bench_llm_inference_performance.cpp` - LLM token generation throughput
- `bench_lora_framework.cpp`, `bench_lora_gpu.cpp` - LoRA adaptation overhead
- `bench_flash_attention.cpp` - Flash attention vs standard attention
- `bench_rag_evaluation.cpp` - RAG retrieval and generation latency
- `bench_prompt_engineering.cpp` - Prompt optimization overhead
- `bench_embedded_llm.cpp` - Embedded (llama.cpp) model performance
- `bench_lora_training.cpp` - LoRA training throughput

**Analytics & Specialized:**
- `bench_olap_analytics.cpp`, `bench_olap_performance.cpp` - OLAP query performance
- `bench_timeseries_ingestion.cpp`, `bench_gorilla_codec.cpp` - Time series compression
- `bench_geo_cpu_gpu.cpp` - Geospatial query performance
- `bench_process_mining.cpp` - Process mining performance
- `bench_governance_policy_latency.cpp` - Compliance policy evaluation latency
- `bench_compliance_security_governance.cpp` - End-to-end governance performance

**Cross-Module & Infrastructure:**
- `bench_cross_functional_end_to_end.cpp` - Full pipeline latency
- `bench_core_performance.cpp` - Core infrastructure (DI, logging) overhead
- `bench_config_path_resolver.cpp` - Configuration resolution performance
- `bench_encryption.cpp` - Encryption overhead
- `bench_compression.cpp` - Compression ratio and speed
- `bench_auth_token_validation.cpp` - Authentication token validation latency

### E.3 Known Performance Architecture Decisions

| Decision | Detail | Module(s) |
|----------|--------|-----------|
| Gorilla codec for time series | Efficient delta+XOR encoding for floating-point time series | timeseries/ |
| HNSW for ANN search | Hierarchical Navigable Small World graphs | index/ |
| Vulkan as default GPU backend | Cross-platform (NVIDIA/AMD/Intel/Apple); CUDA opt-in | acceleration/, gpu/ |
| Flash attention | Fused attention kernel reduces memory bandwidth | llm/ |
| Kernel fusion (LoRA) | Fused forward+backward pass for LoRA adapter training | llm/, gpu/ |
| SAGA for distributed transactions | Compensating transactions for long-running operations | transaction/, sharding/ |
| RAFT consensus | Paxos-like leader election for high-consistency sharding | sharding/ |
| Gorilla codec + Paged optimizer | VRAM paging for large-model training | training/, gpu/ |
| Adaptive query cache | Query result caching with adaptive TTL | cache/, query/ |
| RocksDB LSM storage | Write-optimized LSM tree for storage backend | storage/ |
| SIMD distance kernels | AVX2/FMA accelerated L2/IP/cosine distance | acceleration/, index/ |
| Column-store for OLAP | Apache Arrow-backed columnar OLAP variant | analytics/ |

### E.4 Performance Baselines (Reference Only)

The following represent documented expected performance characteristics from benchmark documentation in `benchmarks/docs/`. Actual results depend on hardware.

| Workload | Reference Target | Conditions |
|----------|-----------------|------------|
| Vector ANN search (HNSW) | < 1 ms P99 (1M vectors) | CPU, in-memory index |
| LLM token generation (llama.cpp) | ~20-80 tokens/s | 7B model, consumer GPU |
| CRUD throughput | > 100k ops/s | Single node, RocksDB |
| TPC-C throughput | Scales with shard count | See bench_tpcc.cpp baselines |
| Shard rebalance | < 30s for 5-node cluster | COMMUNITY edition |
| Time series ingestion | > 1M points/s per node | Gorilla codec enabled |
| Query planning (AQL optimizer) | < 5ms for typical queries | In-memory stats |
| Encryption overhead (AES-256-GCM) | < 5% throughput reduction | Field-level encryption |

**Note:** Authoritative benchmark results should be reproduced from `benchmarks/` using `run_all_benchmarks.sh` or the Python benchmark orchestrator on target hardware. Results in this table are reference targets from documentation and should not be used for SLA commitments.

---

*End of Architecture Audit*
