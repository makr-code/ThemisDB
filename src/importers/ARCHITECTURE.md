# Importers Module — Architecture Guide

**Version:** 2.1
**Last Updated:** 2026-04-06
**Module Path:** `src/importers/`

---

## 1. Overview

The Importers module provides one-time and incremental data import from external database
systems into ThemisDB. It handles schema discovery, schema mapping to ThemisDB's multi-model
layout, batch data transfer, incremental sync via change tracking, and Master Data Management
(MDM) with entity deduplication, conflict resolution, and golden-record selection.

Current connectors: PostgreSQL (v2.1), MySQL/MariaDB, MongoDB, SQLite, Oracle, Kafka,
S3/flat-file, BigQuery. Advanced capabilities include MDM engine, CDC logical replication,
adaptive import optimizer, polyglot persistence mapper, temporal support, schema inference,
blockchain integrity, federated learning coordinator, GraphQL federation, and GUI import wizard.

---

## 2. Design Principles

- **Schema Mapping First** – the importer translates source schema to ThemisDB schema
  before any data is transferred; this allows validation and preview without side effects.
- **Batch-Oriented** – large imports are split into configurable batches to limit memory
  usage and enable resumption on failure.
- **Incremental Support** – after initial import, incremental runs transfer only changed
  rows using source-side change tracking (last-modified timestamp or WAL-based CDC).
- **Source-Agnostic Pipeline** – `import_pipeline.cpp` orchestrates all importers; adding
  a new source requires only a new connector, not pipeline changes.
- **Quarantine on Error** – rows that fail validation or conversion are written to a
  quarantine table rather than causing the import to abort.
- **MDM-First Entity Resolution** – all imports can optionally flow through the MDM engine
  for probabilistic deduplication, golden-record selection, and immutable audit trail.
- **Plugin API** – third-party connectors register at static-init time via `REGISTER_IMPORTER_PLUGIN`;
  the stable C-linkage ABI (`THEMIS_IMPORTER_PLUGIN_V1`) is enforced from v1.9.0 onwards.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `postgres_importer.cpp` | PostgreSQL source connector: pg_dump parsing, FK preservation, schema mapping, CDC |
| `mysql_importer.cpp` | MySQL/MariaDB source connector with parameterised queries |
| `mongo_importer.cpp` | MongoDB source connector with change-stream support |
| `sqlite_importer.cpp` | SQLite `.dump` source connector with type-affinity mapping |
| `oracle_importer.cpp` | Oracle ODBC importer with CLOB/BLOB streaming |
| `kafka_importer.cpp` | Apache Kafka consumer (JSON, Avro, plaintext); requires `THEMIS_ENABLE_KAFKA` |
| `s3_importer.cpp` | S3-compatible object-storage source; requires `THEMIS_ENABLE_S3` |
| `flatfile_importer.cpp` | CSV/TSV/Parquet/NDJSON flat-file importer with BOM/encoding detection |
| `schema_inference.cpp` | Implicit FK discovery, semantic type detection, cardinality estimation |
| `schema_validator.cpp` | Strict/lenient schema validation before apply |
| `conflict_resolver.cpp` | Pluggable conflict resolution (SKIP, OVERWRITE, MERGE, ERROR) |
| `adaptive_import.cpp` | FK-topology-aware import ordering and runtime batch-size adaptation |
| `mdm_engine.cpp` | Probabilistic matching, deduplication, merge policy, golden-record selection |
| `mdm_audit_trail.cpp` | Immutable append-only MDM entity lifecycle event log |
| `mdm_metrics.cpp` | Prometheus-compatible MDM metrics exporter |
| `deterministic_matcher.cpp` | Exact-key entity matching for MDM (confidence = 1.0) |
| `semantic_matcher.cpp` | Embedding-based entity matching (HybridEntityMatcher, co-located impl) |
| `entity_linker.cpp` | Cross-source entity resolution and ID alignment |
| `canonical_resolver.cpp` | MDM golden-record selection and merge arbitration |
| `postgres_importer_mdm.cpp` | PostgreSQL import with MDM deduplication integration |
| `postgres_cdc.cpp` | PostgreSQL logical replication CDC (pgoutput); live stream requires `THEMIS_ENABLE_CDC` |
| `crdt_importer.cpp` | CRDT-based import with LWW, set-union, and multi-value register strategies |
| `data_quality.cpp` | NIST SP 800-188 six-dimension quality scoring |
| `audit_trail.cpp` | SOX/HIPAA-compliant Merkle-chained immutable audit log with SIEM export |
| `polyglot_mapper.cpp` | Recommend RELATIONAL/DOCUMENT/GRAPH/TIMESERIES/KEYVALUE/VECTORSPACE per table |
| `temporal_support.cpp` | SQL:2011 temporal detection and point-in-time query builder |
| `blockchain_integrity.cpp` | SHA-256 Merkle tree with optional blockchain anchoring |
| `federated_learning.cpp` | FedAvg + Gaussian ε-δ differential privacy for distributed schema stats |
| `graphql_federation.cpp` | Apollo Federation v2 SDL generation from relational schemas |
| `column_importance.cpp` | Shannon entropy, Gini impurity, information gain column ranking |
| `gui_import_wizard.cpp` | Step-by-step GUI import wizard with source configuration and dry-run preview |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                  Import API (src/server/)                                │
│   POST /import  { source: "postgres", config: {...} }                    │
│   GUI Import Wizard (gui_import_wizard.cpp) — step-by-step config       │
└──────────────────────────┬──────────────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────────────┐
│                     Import Pipeline (adaptive_import.cpp)                │
│                                                                          │
│  1. SchemaInference + SchemaMapper → ThemisDB collection schema          │
│  2. DataQuality assessment (NIST SP 800-188 six-dimension scoring)       │
│  3. Validate mapping (user confirmation or auto-approve)                 │
│  4. AdaptiveImportOptimizer: FK-topology sort + dynamic batch sizing     │
│  5. Batch iterator: fetch records in configurable batches                │
│  6. Transform + validate each record (schema_validator.cpp)              │
│  7. ConflictResolver: SKIP / OVERWRITE / MERGE / ERROR                  │
│  8a. MDM path → DeterministicMatcher → SemanticMatcher → MDMEngine      │
│      → CanonicalResolver → golden record → ThemisDB storage             │
│  8b. Direct path → write to ThemisDB storage/index                      │
│  9. Quarantine failed records; AuditTrail / MDMAuditTrail event log      │
└──────────────────────────┬──────────────────────────────────────────────┘
                           │ per-source connector
   ┌───────────────────────┼──────────────────────┐
   │                       │                      │
┌──▼──────────────┐  ┌─────▼──────────┐  ┌───────▼──────────────────────┐
│ PostgresImporter│  │ MySQLImporter  │  │ MongoImporter / SQLiteImporter│
│ v2.1: FK preserve│ │ MariaDB support│  │ OracleImporter / S3Importer   │
│ CDC (pgoutput)  │  │ parameterised  │  │ KafkaImporter / FlatFile      │
│ schema discovery│  │ bulk insert    │  │ BigQuery                      │
└─────────────────┘  └────────────────┘  └──────────────────────────────┘
         │
┌────────▼────────────────────────────────────────────────────────────────┐
│                  Advanced Capabilities (v2.1+)                           │
│  PolyglotMapper · TemporalSupport · BlockchainIntegrity                  │
│  FederatedLearning · GraphQLFederation · CRDTImporter                   │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Initial Import

```
ImportRequest { source: "postgres", dsn: "...", tables: ["users", "orders"] }
    │
    ▼
PostgresImporter::discoverSchema(tables)
    → {tables, columns, types, primary_keys, foreign_keys, check_constraints, ...}
    │
    ▼
SchemaInferenceEngine::inferImplicitRelationships() — Jaccard FK discovery
SchemaInferenceEngine::detectSemanticTypes() — EMAIL/UUID/IP/CURRENCY detection
    │
    ▼
DataQualityFramework::QualityAssessor::assessTable() — NIST SP 800-188 scoring
    │
    ▼
SchemaMapper::map(source_schema) → ThemisDB collection schema
    │
    ▼
AdaptiveImportOptimizer::optimizeImportPlan() — Kahn's topological FK sort
User confirms mapping (or auto-approve via config)
    │
    ▼
Import loop (adaptive batch size):
    batch_cursor.next(batch_size) → [rows]
    for each row:
        ├─ transform types (source → ThemisDB types)
        ├─ validate (not-null, type constraints, schema_validator.cpp)
        ├─ ConflictResolver: check for duplicates (SKIP/OVERWRITE/MERGE/ERROR)
        ├─ [optional] MDMEngine: probabilistic match → deduplicate → golden record
        ├─ valid → write to storage + AuditTrail event
        └─ invalid → write to quarantine_table
    │
    ▼
Import complete: {imported: N, quarantined: M, foreign_keys_preserved: K, duration: Xs}
```

### 4.2 Incremental Import / CDC

```
PostgreSQLCDC::subscribeToChanges(publication, replication_slot)  [THEMIS_ENABLE_CDC]
    │  logical decoding (pgoutput protocol)
    ▼
ChangeEvent { type: INSERT|UPDATE|DELETE, table, before, after, lsn }
    │
    ▼
Same transform/validate/MDM pipeline as initial import
    │
    ▼
PostgreSQLCDC::confirmLSN(lsn) — acknowledge processed events
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Writes to** | `src/storage/` | Batch writes via `RocksDBWrapper` |
| **Registers with** | `src/index/` | Secondary index updates during import |
| **Uses** | `src/metadata/` | Schema registration after mapping |
| **Consumed by** | `src/server/` | Import API endpoints + GUI wizard |
| **Exports to** | SIEM (Splunk/ELK) | `AuditedImporter::exportForSIEM()` |
| **Metrics to** | Prometheus/OTel | `ImportOptions::metrics_callback` |

---

## 6. Threading & Concurrency Model

- Each import job runs on a dedicated background thread.
- Concurrent imports from different sources are supported; each uses an independent connection.
- Batch writes use ThemisDB transactions for atomicity per batch.
- Quarantine writes are separate transactions to avoid blocking the main import.
- MDM engine uses shared-read / exclusive-write locking for golden-record updates.
- CRDT importer is designed for parallel ingestion; merge is deterministic (LWW by wall_clock_ns → lamport_clock → replica_id).
- Plugin sandbox enforces per-job memory limits and timeouts via a dedicated monitoring thread.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Batch fetching | Configurable batch size (default: 1000 rows); adaptive tuning via `adaptive_import.cpp` |
| Parallel table import | Multiple tables imported concurrently (configurable `ImportOptions::parallel_tables`) |
| Server-side cursor | PostgreSQL server-side cursor avoids loading entire table into memory |
| FK-topology ordering | `AdaptiveImportOptimizer` uses Kahn's topological sort to import parent tables first |
| Static regex cache | `postgres_importer.cpp` uses `thread_local static std::regex` to avoid recompilation |
| CRDT parallel merge | `crdt_importer.cpp` allows concurrent writers; merge is applied at read time |

---

## 8. Security Considerations

- Source database credentials are never logged; credential fields are masked in `audit_trail.cpp`.
- Connection strings are validated against an allow-list to prevent SSRF.
- All SQL importers use parameterised queries; string interpolation into SQL is prohibited.
- Flat-file paths are canonicalised with `realpath()` and checked against the configured base directory.
- Import is scoped to the authenticated user's tenant; cross-tenant imports are rejected.
- MDM records are validated against the golden-record schema before deduplication; low-confidence matches go to quarantine.
- See `src/importers/SECURITY.md` for the full threat model and security control matrix.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `importers.batch_size` | 1000 | Records per batch |
| `importers.parallel_tables` | 4 | Concurrent table imports |
| `importers.quarantine_enabled` | true | Enable quarantine on error |
| `importers.auto_approve_mapping` | false | Skip user confirmation for schema mapping |
| `importers.incremental.strategy` | `"timestamp"` | Incremental mode: `timestamp` / `wal-cdc` |
| `importers.preserve_foreign_keys` | true | Extract and embed FK metadata in entity JSON |
| `importers.mdm.enabled` | false | Route import through MDM deduplication pipeline |
| `importers.mdm.confidence_threshold` | 0.85 | Min match confidence for auto-merge; below → quarantine |
| `importers.data_quality.enabled` | false | Run NIST SP 800-188 quality assessment before import |
| `importers.adaptive.enabled` | true | Enable adaptive batch-size tuning |
| `importers.checkpoint_file` | `""` | Path to checkpoint JSON for resume-on-failure |
| `importers.enforce_utf8` | false | Reject rows with invalid UTF-8 encoding |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Connection failure | Abort import; return structured `ImportError` with `ImportErrorCode` |
| Schema discovery failure | Abort import; return structured error |
| Row transformation failure | Quarantine row (`continue_on_error=true`) or abort (`false`) |
| Batch write failure | Retry batch (up to 3 times); then abort import |
| MDM match below threshold | Write entity to MDM quarantine; continue import |
| Plugin sandbox violation | Terminate plugin job; return `PLUGIN_SANDBOX_VIOLATION` error |

---

## 11. Known Limitations & Future Work

- `postgres_cdc.cpp` LogicalDecoder provides the interface contract; the live libpq replication
  stream requires `THEMIS_ENABLE_CDC` and a live PostgreSQL server.
- `blockchain_integrity.cpp` uses `std::hash` by default; enable `THEMIS_ENABLE_OPENSSL`
  for FIPS 140-2 compliant SHA-256.
- `federated_learning.cpp` is experimental; it has not undergone a formal security review for
  gradient inversion attacks. Do not use with sensitive training data in production.
- No distributed parallel import across multiple ThemisDB nodes (single-node import only).
- Microsoft SQL Server importer is planned (Issue: #1845).
- GraphQL federation importer does not yet enforce query depth limits (open issue in AUDIT.md).

---

## 12. Version History

| Version | Date | Summary |
|---------|------|---------|
| 2.1 | 2026-03-21 | MDM engine, GUI wizard, CDC interface, adaptive optimizer, polyglot mapper, temporal support, blockchain integrity, federated learning, GraphQL federation, CRDT importer, data quality framework added; 31 source components documented |
| 1.0 | 2026-02-24 | Initial architecture guide; covered PostgreSQL, MySQL (in progress), and MongoDB connectors only |

---

## 13. References

- `src/importers/README.md` — module overview
- `src/importers/ROADMAP.md` — feature roadmap
- `src/importers/CHANGELOG.md` — version history
- `src/importers/AUDIT.md` — security audit findings
- `src/importers/SECURITY.md` — threat model and security controls
- `docs/importers_roadmap.md` — production-readiness assessment
- `docs/importers_runbook.md` — operational runbook
- `ARCHITECTURE.md` (root) — full system architecture
