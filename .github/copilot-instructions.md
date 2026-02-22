# ThemisDB – GitHub Copilot Instructions

> **Automatisch erkannt von GitHub Copilot.** Diese Datei steuert das Verhalten von Copilot für alle Contributors des ThemisDB-Projekts.
>
> Detaillierte Modul-Guides: [`.github/copilot/`](.github/copilot/)
> Governance & Labels: [`.github/GOVERNANCE.md`](.github/GOVERNANCE.md)

---

## Kurzreferenz: Deutsche Befehle

| Befehl | Aktion | Schrittfolge |
|--------|--------|--------------|
| `weiter` | Nächste offene Roadmap-Task implementieren | 1. `roadmap.md` lesen → 2. ersten `[ ]`-Task identifizieren → 3. Implementierung gemäß Phase-Modell starten |
| `check` | Code-Qualität prüfen | 1. `clang-tidy` → 2. `cppcheck` → 3. Google Test ausführen → 4. Maturity-Score bewerten |
| `fix` | Gefundenes Problem beheben | 1. Root cause analysieren → 2. minimale Änderung implementieren → 3. Regressionstests ergänzen → 4. PR erstellen |
| `doc` | Dokumentation erstellen/aktualisieren | 1. Doxygen-Header schreiben → 2. `docs/` Markdown aktualisieren → 3. `CHANGELOG.md` Eintrag ergänzen |
| `test` | Tests für geänderten Code schreiben | 1. Google Test Fixture anlegen → 2. Unit-Tests (Happy Path + Edge Cases) → 3. Integration-Tests → 4. `ctest` ausführen |
| `review` | Code-Review durchführen | 1. Diff prüfen → 2. Standards-Konformität → 3. Security-Check → 4. Performance-Impact → 5. Feedback formulieren |
| `plan` | Implementierungsplan erstellen | 1. Anforderungen klären → 2. Phasen 1-6 ausarbeiten → 3. Akzeptanzkriterien definieren → 4. Abhängigkeiten identifizieren |
| `status` | Aktuellen Projektstand ermitteln | 1. `roadmap.md` + `CURRENT_STATUS.md` lesen → 2. offene Issues prüfen → 3. Zusammenfassung ausgeben |

---

## 1) Roadmap-Driven Implementation

### 1.1 Ziel

Roadmap-Einträge müssen so konkret sein, dass Copilot **produktiven Sourcecode** statt Stub/Rumpf erzeugen kann. Keine Stub-Methoden ohne Produktionslogik. Kein „TODO-Code" als Endergebnis.

### 1.2 Pflichtstruktur für `roadmap.md` je Modul

Jede Modul-Roadmap MUSS diese Abschnitte enthalten:

1. `## Current Status`
2. `## In Progress` und/oder `## Planned Features`
3. `## Implementation Phases` mit `### Phase 1 ... ### Phase N`
4. `## Production Readiness Checklist`
5. `## Known Issues & Limitations`
6. `## Breaking Changes` (falls relevant)

#### Checkbox-Status

| Symbol | Bedeutung |
|--------|-----------|
| `[ ]` | offen |
| `[~]` | in Bearbeitung |
| `[x]` | erledigt |
| `[I]` | Issue vorhanden |
| `[P]` | Pull Request vorhanden |
| `[?]` | Human question/blockiert |
| `[!]` | unklarer/zu prüfender Zustand |

#### Aufgabenformat (Pflicht)

```markdown
- [ ] <konkrete technische Aufgabe> (Target: <Milestone/Quartal>)
  - Inputs: <Datentypen, Batch-Größen>
  - Outputs: <erwartete Ergebnisse>
  - Constraints: <Toleranzen, Limits>
  - Errors: <Fehlerfälle, Validierung>
  - Tests: <unit + integration + edge cases>
  - Perf: <messbares Ziel>
```

**Beispiel:**

```markdown
- [ ] CUDA geospatial distance and containment kernels (Target: Q3 2026)
  - Inputs: WGS84 points/polygons, batch-size up to 1e6
  - Outputs: distance matrix + containment bitset
  - Constraints: deterministic FP tolerance <= 1e-6
  - Errors: invalid geometry, NaN coordinates, overflow
  - Tests: unit + property-based + GPU/CPU parity
  - Perf: >= 8x speedup vs CPU baseline on RTX-class GPU
```

### 1.3 Phasenmodell (verbindlich)

| Phase | Inhalt | Deliverables |
|-------|--------|--------------|
| Phase 1 | Design / API-Vertrag | Header-Dateien, Interface-Definitionen |
| Phase 2 | Core-Implementierung | `.cpp`-Implementierungen, keine Stubs |
| Phase 3 | Fehlerbehandlung & Edge Cases | `std::expected`, Exception-Handling |
| Phase 4 | Tests | Google Test: Unit + Integration + Benchmarks |
| Phase 5 | Performance/Hardening | SIMD, Caching, Lock-free Datenstrukturen |
| Phase 6 | Dokumentation & Abnahme | Doxygen, `docs/`, `CHANGELOG.md` |

### 1.4 `future_enhancement.md` Struktur

```markdown
## <module-name>
### Scope
### Design Constraints
### Required Interfaces
### Implementation Notes
### Test Strategy
### Performance Targets
### Security / Reliability
```

Regel: keine vagen Formulierungen wie „improve", „optimize" ohne messbares Ziel.

### 1.5 Governance & Issue Metadaten (Pflicht)

Jedes neue Issue MUSS folgende Metadaten haben:

- `area:*` Label (z.B. `area:core`, `area:llm`, `area:security`)
- `priority:*` Label (critical, high, medium, low)
- `type:*` Label (feature, bug, test, documentation, refactor, chore)
- `status:*` Label (open, in_progress, blocked, ready, review)
- **Milestone** (Quartal Q1-Q4 2026 oder Version v1.x.x)
- **Relationship-Links:** `Relates to #123`, `Depends on #456`, `Fixes #789`

---

## 2) C++ Entwicklungsrichtlinien

### 2.1 Sprachstandard & Compiler

- **Standard:** C++17 (minimal), C++20 (bevorzugt für neue Module)
- **Compiler:** GCC 11+, Clang 14+, MSVC 2022+
- **Build System:** CMake 3.20+ mit vcpkg (offline-first)

### 2.2 Modern C++ Patterns

```cpp
// ✅ RAII für Resource Management
auto conn = pool->acquire();  // automatisch freigegeben

// ✅ Smart Pointers statt raw pointers
std::unique_ptr<StorageEngine> engine = StorageEngine::create(config);
std::shared_ptr<Index> idx = std::make_shared<VectorIndex>(dims);

// ✅ Structured Bindings (C++17)
for (const auto& [key, value] : result_map) { process(key, value); }

// ✅ std::optional / std::expected für nullable/fallible Ergebnisse
auto findUser(int id) -> std::optional<User>;
auto parseConfig(const std::string& path) -> std::expected<Config, ParseError>;

// ❌ Kein manuelles Memory Management
Connection* conn = new Connection();  // VERBOTEN
delete conn;                          // VERBOTEN
```

### 2.3 Namenskonventionen

| Kategorie | Stil | Beispiel |
|-----------|------|---------|
| Klassen, Structs, Enums | `PascalCase` | `VectorIndex`, `QueryResult` |
| Funktionen, Methoden | `camelCase` | `executeQuery()`, `getUser()` |
| Member-Variablen | `snake_case_` | `max_connections_`, `db_path_` |
| Lokale Variablen / Parameter | `snake_case` | `result_count`, `query_str` |
| Konstanten / constexpr | `UPPER_CASE` | `MAX_CONNECTIONS`, `BUFFER_SIZE` |
| Namespaces | `snake_case` | `themis::query::`, `themis::storage::` |

### 2.4 Namespace-Struktur

```
themis::core::*          - Initialization, logging, tracing
themis::storage::*       - RocksDB wrapper, blob, compression
themis::query::*         - Parser, optimizer, executor
themis::transaction::*   - ACID, SAGA, branching
themis::index::*         - HNSW, vector, graph indices
themis::server::*        - HTTP/gRPC handlers, API gateway
themis::security::*      - RBAC, encryption, PKI, audit
themis::llm::*           - Inference, embeddings, LoRA
themis::rag::*           - RAG pipeline, evaluation
themis::sharding::*      - Raft/Paxos, shard routing
```

### 2.5 Thread Safety Patterns

```cpp
// ✅ Standard Synchronisation
std::lock_guard<std::mutex> lock(mutex_);
std::shared_lock<std::shared_mutex> read_lock(rw_mutex_);
std::unique_lock<std::shared_mutex> write_lock(rw_mutex_);
std::atomic<int64_t> request_count_{0};
thread_local ConnectionPool local_pool;  // Lock-free Hot-Path
```

### 2.6 Error Handling

```cpp
// ✅ Exceptions nur für echte Ausnahmen
if (!file.exists()) throw FileNotFoundException(path);

// ✅ std::optional für nullable Ergebnisse
auto findUser(UserId id) -> std::optional<User>;

// ✅ Fehler-Codes für Performance-kritische Pfade
enum class StorageError { NotFound, Corrupted, IOError };
auto read(const Key& k) -> std::variant<Value, StorageError>;
```

### 2.7 Performance Guidelines

```cpp
// ✅ Const Reference für große Objekte
void process(const std::string& data);

// ✅ Move Semantics bei Ownership-Transfer
return std::move(large_object);

// ✅ Reserve für Container
std::vector<int> results;
results.reserve(expected_size);
```

---

## 3) Multi-Model Database Architecture

### 3.1 Datenbankmodelle

| Modell | Namespace | Schlüsselklassen |
|--------|-----------|-----------------|
| Relational (AQL) | `themis::query::` | `AqlParser`, `QueryOptimizer`, `QueryExecutor` |
| Document | `themis::storage::` | `DocumentStore`, `SchemaManager` |
| Graph | `themis::graph::` | `PropertyGraph`, `GraphIndex`, `PathConstraint` |
| Vector | `themis::index::` | `VectorIndex`, `HnswIndex`, `QuantizedIndex` |
| Time Series | `themis::timeseries::` | `TimeSeriesManager`, `GorillaEncoder` |
| Geospatial | `themis::geo::` | `SpatialBackend`, `GpuGeoBackend` |

### 3.2 ACID Transaction Support

```cpp
// Vollständige ACID-Transaktion
auto tx = transaction_manager->begin(IsolationLevel::Serializable);
try {
    tx->write(key1, value1);
    tx->commit();
} catch (const ConflictException& e) {
    tx->rollback();
    throw;
}

// SAGA Pattern für verteilte Transaktionen
auto saga = SagaManager::create();
saga->addStep("debit",  debit_fn,  compensate_debit_fn);
saga->addStep("credit", credit_fn, compensate_credit_fn);
saga->execute();
```

### 3.3 AI/LLM Integration Patterns

```cpp
// Embedding-Generierung
auto embedder = EmbeddedLlm::create(ModelConfig{.model = "nomic-embed-text"});
auto embedding = embedder->embed("search query text");

// RAG Pipeline
auto rag = RagPipeline::create(vector_index, llm_backend);
auto result = rag->query("Was ist ThemisDB?", {.top_k = 5, .rerank = true});

// AQL aus natürlicher Sprache
auto aql = LlmAqlHandler::create(llm)->naturalLanguageToAql("Finde alle Nutzer aus Berlin");
```

---

## 4) Modulspezifische Guidelines

### Foundation Layer

#### `src/core/` — Kern-Initialisierung

- **Namespace:** `themis::core::` | Enthält: `ConcernsContext`, `SecurityInit`
- `SecurityInit` muss vor allen anderen Modulen ausgeführt werden
- Logging via `themis::utils::Logger` — niemals `std::cout` in Produktionscode
- Keine Business-Logik in `core/` — nur Infrastructure-Setup

#### `src/storage/` — Storage Engine

- **Namespace:** `themis::storage::` | Enthält: `StorageEngine`, `BlobStorageManager`
- **PFLICHT:** Jede Schreiboperation transaktional oder mit WAL abgesichert
- Kompression: LZ4 (default), ZSTD (high ratio), Snappy (low latency)
- Performance-Ziel: < 1ms p99 für Point-Reads unter 10k QPS

#### `src/transaction/` — ACID & SAGA

- **Namespace:** `themis::transaction::` | Enthält: `TransactionManager`, `SagaManager`
- MVCC für Snapshot Isolation; SAGA-Kompensationen müssen idempotent sein
- Isolation Levels: ReadUncommitted, ReadCommitted, RepeatableRead, Serializable
- Deadlock-Detection: Wartegraph-Algorithmus, Timeout-basiertes Fallback

#### `src/query/` — AQL Query Engine

- **Namespace:** `themis::query::` | Pipeline: `AqlParser` → `Optimizer` → `Executor`
- Cost-Based Optimizer mit Kardinalitätsschätzungen; Query-Pläne cachen (LRU, max 10k)
- **Test-Pflicht:** Jede neue AQL-Funktion braucht Parser + Execution-Tests

#### `src/index/` — Indexing

- **Namespace:** `themis::index::` | Enthält: `HnswIndex`, `VectorIndex`, `GraphIndex`
- HNSW: M=16, ef_construction=200 als Defaults; thread-safe via Lock-Striping
- Distanzmetriken: Kosinus, L2, Inner Product — alle müssen getestet sein

### Query & Index Layer

#### `src/aql/` — AQL Handlers & LLM

- `LlmAqlHandler`: Natural Language → AQL; Eingabe-Sanitization gegen Injection-Angriffe
- Ergebnisse cachen wenn `is_deterministic == true`

#### `src/search/` — Hybrid Search

- Kombiniert Vector-Search (ANN) + Full-Text (BM25/TF-IDF) via RRF-Fusion
- `HybridSearch::query()` führt beide Pfade parallel aus (std::async/Thread-Pool)

#### `src/cache/` — Caching Layer

- `SemanticCache`: Cosinus-Threshold 0.95; `AdaptiveQueryCache`: LRU + Frequency
- Cache-Invalidierung bei Writes synchron; kein Caching von PII/Auth-Tokens

### Security & Auth

#### `src/security/` — Security Engine

- **Namespace:** `themis::security::` | **PFLICHT:** Field-Level Encryption für PII
- RBAC Least-Privilege; HSM-Integration für Key Management (Enterprise)
- **Audit-Log:** Jede privilegierte Operation muss geloggt werden
- Compliance: ISO 27001, NIST SP 800-53, SOC 2 Type II, GDPR

```cpp
// ✅ RBAC Check vor privilegierter Operation
if (!rbac->hasPermission(user_id, Permission::AdminWrite)) {
    throw AuthorizationException("Insufficient privileges");
}
```

#### `src/auth/` — Authentication

- JWT: RS256/ES256 (kein HS256 in Produktion); Token-Rotation: Access < 15min
- Rate-Limiting auf Auth-Endpoints: max. 10 Versuche/Minute/IP
- MFA: TOTP (RFC 6238), FIDO2/WebAuthn

### Server & Network

#### `src/server/` — HTTP/gRPC Server

- 40+ API Handler — jeder Handler in eigener Datei, max. 300 Zeilen
- `ApiGateway`: Auth-Middleware → Rate-Limiter → Handler
- **Kein** direkter DB-Zugriff in Handlern — nur über Service-Layer
- SSE für Changefeed-Streaming: max. 1000 concurrent connections

#### `src/network/` — Wire Protocol

- PostgreSQL Wire Protocol: Kompatibilität mit pg-Clients sicherstellen
- Alle Verbindungen: TLS 1.3 minimum; WebSocket Heartbeat alle 30s

### Intelligence Layer

#### `src/llm/` — LLM Integration

- **Namespace:** `themis::llm::` | Enthält: `EmbeddedLlm`, `LoraFramework`, `FlashAttention`
- Modell-Laden: Lazy Loading, max. VRAM-Budget konfigurierbar
- **Safety:** Output-Filtering für Jailbreaks (OWASP LLM Top 10)
- Inference Timeout: 30s default, konfigurierbar per Request

#### `src/rag/` — RAG Pipeline & Evaluation

- `RagJudge`: Faithfulness + Relevance Scoring (0.0–1.0)
- Bias-Detection auf demografische und politische Biases
- **Retrieval-Qualität:** Precision@5 >= 0.85 als Akzeptanzkriterium
- Chunk-Size: 512 tokens default, overlap 64 tokens

#### `src/voice/` — Voice Assistant

- STT → NLU → AQL-Generation → TTS Pipeline; Wake-Word lokal (kein Cloud)
- Audio-Buffer: 16kHz, 16-bit PCM; Latenz-Ziel: < 500ms End-to-End

#### `src/prompt_engineering/` — Prompt Management

- Prompt-Templates versionieren (SemVer); Injection-Schutz via Template-Slots
- A/B-Testing-Framework für Prompt-Varianten

### Operations

#### `src/observability/` — Metrics & Profiling

- Prometheus-Export: alle Metriken als `themis_*`-Präfix
- **SLO-Targets:** p99 < 100ms für einfache Queries, p99 < 1s für komplexe

#### `src/scheduler/` — Task Scheduling

- `TaskScheduler`: Priority Queues; Backpressure bei > 80% Queue-Füllstand
- `HybridRetentionManager`: TTL + Size-basierte Tier-Migration

#### `src/updates/` — Hot Reload

- `HotReloadEngine`: Rolling Updates ohne Downtime
- **Rollback-Garantie:** Bei fehlgeschlagenem Update automatisch zurück

### Data Integration

#### `src/cdc/` — Change Data Capture

- `ChangeFeed`: Debezium-kompatibles Event-Format; Ordered Buffer mit Exactly-Once
- At-least-once Delivery → Idempotente Consumer erforderlich

#### `src/importers/` — Data Import

- `PostgresImporter`: pgdump/pglogical Kompatibilität; Batch: max. 10k Rows, transaktional

#### `src/exporters/` — Data Export

- `JsonlLlmExporter`: JSONL für LLM-Training-Datasets; PII-Anonymisierung mandatory

#### `src/content/` — Multimodal Ingestion

- `AsyncIngestionWorker`: Worker-Pool, max. 16 concurrent; Virus-Scan-Hook (Enterprise)
- Chunk-Extraktion mit Metadaten (page, timestamp, bounding box)

#### `src/ingestion/` — Data Pipeline

- Streaming-Ingest: Kafka, MQTT, WebSocket; Dead-Letter-Queue für fehlgeschlagene Messages

### Distributed Systems

#### `src/sharding/` — Horizontal Scaling

- `RaftConsensus`: Leader Election, Log Replication (etcd-kompatibel)
- `ShardRouter`: Konsistentes Hashing, virtuelle Nodes (vnodes: 256)
- Split-Brain-Prävention: Quorum-basierte Writes (majority + 1)

#### `src/replication/` — Multi-Master Replication

- Asynchrone + synchrone Modi; Replikations-Lag-Alert bei > 10s
- Conflict Resolution: Last-Write-Wins oder Custom-Resolver

### Specialized Modules

#### `src/geo/` — Geospatial

- WGS84-Koordinatensystem; `SpatialBackend`: R-Tree-Index, PostGIS-kompatibel
- `GpuGeoBackend`: CUDA-Kernel für Batch-Distanzberechnungen; FP-Toleranz <= 1e-6

#### `src/graph/` — Property Graph

- Apache TinkerPop / Gremlin-kompatibel; CSR für effiziente Traversal
- Max. Traversal-Tiefe: 20 Hops (konfigurierbar, default: 10)

#### `src/temporal/` — Temporal Data

- Bitemporal Model: Transaction Time + Valid Time; SQL:2011 Temporal Tables

#### `src/timeseries/` — Time Series

- `GorillaEncoder`/`GorillaDecoder`: Facebook Gorilla-Kompression
- Ingestion-Rate: > 100k samples/s als Ziel; Hot/Warm/Cold Tier-Migration

#### `src/analytics/` — Analytics Engine

- `OlapEngine`: Columnstore, vectorized execution; `DiffEngine`: Schema/Daten-Diff

#### `src/acceleration/` — GPU Acceleration

- Backends: CUDA, HIP (ROCm), OpenCL, Vulkan, Metal
- L2-Distanz: immer squared distance zurückgeben (kein sqrt)
- Backend-Fallback: GPU → CPU automatisch; Cross-Backend-Konsistenz-Tests PFLICHT

#### `src/gpu/` — GPU Memory Management

- `GpuMemoryManager`: Unified Memory Pool; VRAM-Budget: Default 80% der freien VRAM

### Utility Modules

#### `src/utils/` — Utilities

- `Logger`: Structured Logging (JSON); `PiiDetector`: Regex + ML-basiert
- **Niemals** rohe `std::cout`/`printf` in Produktionscode

#### `src/chimera/` — Database Compatibility Adapters

- `IDatabaseAdapter`: Interface für PostgreSQL, MySQL, MongoDB Kompatibilität
- Kompatibilitäts-Tests gegen echte Reference-Implementierungen

#### `src/plugins/` — Plugin System

- `PluginManager`: Hot-Plugging ohne Neustart; stabile ABI über `extern "C"` Interface

#### `src/governance/` — Policy Engine

- `PolicyEngine`: OPA-kompatibel; `ComplianceReporter`: GDPR, HIPAA, SOC 2
- Data Lineage: vollständige Datenherkunft für Audit

#### `src/metadata/` — Schema Management

- `SchemaManager`: Schema-Versioning (SemVer); Backward-compatible Migration default

#### `src/config/` — Configuration

- YAML-basiert; Secrets via Env/Vault (niemals in Config-Dateien)
- Hot-Reload: Konfigurationsänderungen ohne Neustart möglich

#### `src/performance/` — Performance Primitives

- RCU für Lock-free Reads; LIRS Cache-Replacement; Lock-free SPSC/MPMC Queues

#### `src/api/` — GraphQL API

- GraphQL über HTTP/2; Introspection in Produktion deaktivieren

#### `src/base/` — Module Loader

- `ModuleLoader`: Dependency-ordered Initialization mit Circular-Dependency-Detection

#### `src/training/` — Model Training

- Fine-Tuning-Pipelines; Trainingsdaten-Qualitätsprüfung; Checkpoint-Management

---

## 5) Code Quality Standards

### 5.1 Maturity Levels

| Level | Score | Kriterien |
|-------|-------|-----------|
| 🟢 PRODUCTION-READY | 95–100 | Keine Stubs, keine TODOs, vollständige Tests, Doxygen |
| 🟡 RELEASE-CANDIDATE | 85–94 | Wenige nicht-kritische TODOs, > 80% Test-Coverage |
| 🟠 BETA | 70–84 | Bekannte Einschränkungen dokumentiert |
| 🔴 ALPHA | 50–69 | Proof-of-Concept, kritische Pfade fehlen |
| ⚫ DRAFT | 0–49 | Stub/Rumpf, nicht für Produktion |

**Ziel:** Durchschnittlicher Maturity-Score > 95/100 (aktuell: 96.6/100 laut `feature_enhancement.md`)

### 5.2 Code Quality Tools

```bash
clang-format -i src/**/*.cpp include/**/*.h        # Formatierung
clang-tidy src/**/*.cpp -- -std=c++17 -I include/  # Linting
cppcheck --enable=all --project=build/compile_commands.json
cd build && ctest --output-on-failure -j$(nproc)   # Tests
```

### 5.3 Security & Compliance Checkliste

| Standard | Prüfpunkte |
|----------|------------|
| OWASP Top 10 | Injection, Broken Auth, SSRF |
| OWASP LLM Top 10 | Prompt Injection, Data Leakage, Jailbreaks |
| ISO 27001 | Zugriffssteuerung, Audit-Logs, Datenschutz |
| NIST SP 800-53 | AES-256, RSA-4096, Key Management |
| SOC 2 Type II | Availability, Integrity, Confidentiality |
| GDPR | PII-Handling, Right to Erasure, Data Minimization |

**Security-Pflichten pro PR:**
- [ ] Keine neuen CVEs in Dependencies
- [ ] PII-Detection-Test für neue Felder
- [ ] Audit-Log für privilegierte Operationen
- [ ] Input-Validierung für alle externen Eingaben

### 5.4 Test Requirements

```cpp
// Pflicht-Teststruktur für jedes Modul
TEST(ModuleTest, HappyPath_BasicOperation) { ... }
TEST(ModuleTest, EdgeCase_EmptyInput) { ... }
TEST(ModuleTest, EdgeCase_LargeDataset) { ... }
TEST(ModuleTest, ErrorCase_InvalidInput) { ... }
TEST(ModuleTest, ErrorCase_NetworkFailure) { ... }
TEST(ModuleTest, ThreadSafety_ConcurrentAccess) { ... }

// Performance-Benchmark
BENCHMARK(BM_ModuleOperation)->Range(1<<10, 1<<20);
```

**Coverage-Ziele:**
- Core/Storage/Transaction: >= 90%
- Query/Index: >= 85%
- Security/Auth: >= 95% (kritisch)
- LLM/RAG: >= 80%

### 5.5 Dokumentations-Standard

```cpp
/**
 * @brief Executes an AQL query against the database
 *
 * @param query   The AQL query string (not sanitized — caller must validate)
 * @param timeout Query timeout in milliseconds (0 = no timeout)
 * @param context Execution context with tenant and auth info
 * @return QueryResult containing matched records and metadata
 * @throws QuerySyntaxException if query parsing fails
 * @throws AuthorizationException if context lacks read permissions
 *
 * @note Thread-safe. Multiple queries can execute concurrently.
 */
auto executeQuery(const std::string& query, int timeout,
                  const ExecContext& context) -> QueryResult;
```

---

## 6) Workflow Integration

### 6.1 PR Validation Checklist

- [ ] `clang-format` hat keine Änderungen produziert
- [ ] `clang-tidy` ohne neue Warnungen
- [ ] `ctest` läuft durch (alle Tests grün)
- [ ] Neuer Code hat Doxygen-Kommentare
- [ ] `CHANGELOG.md` aktualisiert
- [ ] Security-Checklist abgearbeitet (s. Abschnitt 5.3)
- [ ] Breaking Changes dokumentiert

### 6.2 Conventional Commits

```
feat(storage): add LZ4 compression for blob storage
fix(query): resolve optimizer cardinality estimation overflow
test(security): add RBAC permission matrix tests
docs(llm): document LoRA fine-tuning API
perf(index): reduce HNSW build time via parallel construction
refactor(transaction): extract SAGA coordinator into separate class
chore(deps): update RocksDB to 8.10.0
```

### 6.3 Branching Strategy

```
main                       ← Produktions-Branch (protected)
develop                    ← Integration-Branch
feature/<issue>-<desc>     ← Feature-Branches
fix/<issue>-<desc>         ← Bugfix-Branches
release/v<major>.<minor>   ← Release-Branches
```

### 6.4 Milestone-Übersicht

| Milestone | Schwerpunkt | Deadline |
|-----------|-------------|----------|
| Q1 2026 | Critical Fixes, Security Hardening | 2026-03-31 |
| Q2 2026 | Performance, GPU Acceleration | 2026-06-30 |
| Q3 2026 | Distributed Systems, Multi-Region | 2026-09-30 |
| Q4 2026 | Enterprise Features, Compliance, GA | 2026-12-31 |

---

## 7) Detaillierte Modul-Guides

| Guide | Inhalt |
|-------|--------|
| [CODE_STANDARDS.md](.github/copilot/CODE_STANDARDS.md) | C++ Style, Naming, Tools |
| [BUILD_GUIDE.md](.github/copilot/BUILD_GUIDE.md) | CMake Presets, vcpkg, Editions |
| [TESTING_GUIDE.md](.github/copilot/TESTING_GUIDE.md) | Google Test, Coverage, CI |
| [BRANCHING_GUIDE.md](.github/copilot/BRANCHING_GUIDE.md) | Git Flow, PR-Workflow |
| [CROSS_COMPILATION_CONTEXT.md](.github/copilot/CROSS_COMPILATION_CONTEXT.md) | ARM, Windows, Linux, Docker |
| [VSCODE_CONTEXT.md](.github/copilot/VSCODE_CONTEXT.md) | IDE Setup, IntelliSense |
