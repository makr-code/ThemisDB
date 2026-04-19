# Importers Module

Data import functionality for ThemisDB.

## Module Purpose

Provides data import functionality for ThemisDB, supporting PostgreSQL (v2.1), MySQL/MariaDB,
MongoDB, SQLite, Oracle, Apache Kafka, S3-compatible object storage, BigQuery, and CSV/TSV/Parquet
flat files with schema mapping, batch import, incremental import, MDM entity deduplication,
and advanced data quality capabilities.

## Subsystem Scope

**In scope:** Database dump import with schema mapping, batch import operations,
incremental import via change tracking, streaming row callbacks, dry-run mode,
MDM entity deduplication and golden-record selection, CDC logical replication,
data quality assessment, adaptive batch optimization, CRDT-based parallel import,
GUI import wizard, plugin API for third-party connectors.

**Out of scope:** Data transformation beyond schema mapping (handled by content module),
export functionality (handled by exporters module), CDC-based ongoing sync as a primary
interface (CDC module handles long-running replication; `postgres_cdc.cpp` here is for
import-time CDC bootstrapping).

## Relevant Interfaces

- `postgres_importer.cpp` — PostgreSQL pg_dump source connector with schema mapping,
  FK preservation (v2.0+), relationship mapping, COPY-protocol support, checkpoint/resume
- `mysql_importer.cpp` — MySQL/MariaDB mysqldump source connector with parameterised queries
- `mongo_importer.cpp` — MongoDB mongoexport JSON/NDJSON source connector
- `sqlite_importer.cpp` — SQLite `.dump` source connector (type-affinity mapping)
- `oracle_importer.cpp` — Oracle ODBC importer with CLOB/BLOB streaming
- `kafka_importer.cpp` — Apache Kafka consumer (JSON, Avro, plaintext); requires `THEMIS_ENABLE_KAFKA`
- `s3_importer.cpp` — S3-compatible object-storage source; requires `THEMIS_ENABLE_S3`
- `flatfile_importer.cpp` — CSV/TSV/Parquet/NDJSON flat-file importer with BOM/encoding detection
- `conflict_resolver.cpp` — pluggable conflict resolution strategies (SKIP, OVERWRITE, MERGE, ERROR)
- `adaptive_import.cpp` — FK-topology-aware import ordering and runtime batch-size adaptation
- `mdm_engine.cpp` — probabilistic matching, deduplication, merge policy, golden-record selection
- `mdm_audit_trail.cpp` — immutable append-only MDM entity lifecycle event log
- `mdm_metrics.cpp` — Prometheus-compatible MDM metrics exporter
- `deterministic_matcher.cpp` — exact-key entity matching for MDM deduplication
- `entity_linker.cpp` — cross-source entity resolution and ID alignment
- `canonical_resolver.cpp` — MDM golden-record selection and merge arbitration
- `postgres_cdc.cpp` — PostgreSQL logical replication CDC (pgoutput); live stream requires `THEMIS_ENABLE_CDC`
- `crdt_importer.cpp` — CRDT-based import with LWW, set-union, multi-value register strategies
- `data_quality.cpp` — NIST SP 800-188 six-dimension quality scoring
- `audit_trail.cpp` — SOX/HIPAA-compliant Merkle-chained immutable audit log
- `schema_inference.cpp` — implicit FK discovery, semantic type detection, cardinality estimation
- `schema_validator.cpp` — strict/lenient schema validation before apply
- `polyglot_mapper.cpp` — recommend optimal data model per table
- `temporal_support.cpp` — SQL:2011 temporal detection and point-in-time query builder
- `blockchain_integrity.cpp` — SHA-256 Merkle tree with optional blockchain anchoring
- `federated_learning.cpp` — FedAvg + Gaussian ε-δ differential privacy (experimental)
- `graphql_federation.cpp` — Apollo Federation v2 SDL generation
- `column_importance.cpp` — Shannon entropy, Gini impurity, information gain column ranking
- `gui_import_wizard.cpp` — step-by-step GUI import wizard with source configuration and dry-run preview

## Current Delivery Status

**Maturity:** 🟢 Production — PostgreSQL (v2.1), MySQL/MariaDB, MongoDB, SQLite, Oracle,
Kafka, S3, and flat-file importers operational. MDM engine, GUI wizard, CDC interface, and
advanced analytics capabilities (v2.1+) production-ready.

## Components

- PostgreSQL importer (v2.1): FK preservation, relationship mapping, extended constraints
- MySQL/MariaDB importer
- MongoDB importer
- SQLite importer
- Oracle importer
- Apache Kafka consumer importer (real-time streaming; requires `THEMIS_ENABLE_KAFKA`)
- S3-compatible object-storage source connector (requires `THEMIS_ENABLE_S3`)
- CSV/TSV/Parquet/NDJSON flat-file importer
- Conflict resolver (SKIP, OVERWRITE, MERGE, ERROR)
- Adaptive import optimizer (FK-topology ordering, dynamic batch sizing)
- MDM engine (probabilistic matching, deduplication, golden-record selection)
- MDM audit trail and metrics
- Deterministic and semantic entity matchers
- Entity linker and canonical resolver
- PostgreSQL CDC logical replication interface
- CRDT-based parallel import
- Data quality framework (NIST SP 800-188)
- SOX/HIPAA audit trail
- Schema inference engine
- Schema validator
- Polyglot persistence mapper
- Temporal database support
- Blockchain integrity verifier
- Federated learning coordinator (experimental)
- GraphQL federation SDL generator
- Column importance analyzer
- GUI import wizard

## Features

- Import data from PostgreSQL, MySQL/MariaDB, MongoDB, SQLite, Oracle, Kafka, S3, and flat files
- Real-time streaming ingestion from Apache Kafka topics (JSON, Avro, plaintext)
- Foreign key preservation and relationship mapping (PostgreSQL v2.0+)
- Schema mapping, type-affinity transformation, and inference
- Batch import operations with configurable chunk size and adaptive tuning
- Incremental import support (watermark-based change tracking, checkpoint/resume)
- CDC logical replication interface for PostgreSQL (`THEMIS_ENABLE_CDC`)
- MDM entity deduplication, golden-record selection, immutable audit trail
- CRDT-based conflict-free parallel import
- NIST SP 800-188 data quality assessment
- Dry-run mode (validate without writing data)
- Streaming row callback for real-time progress
- Include/exclude table filtering
- Permission-check callback (ACL enforcement)
- Plugin API for third-party importers (stable C-linkage ABI `THEMIS_IMPORTER_PLUGIN_V1`)
- GUI step-by-step import wizard with source configuration and dry-run preview
- Metrics and distributed-tracing observability hooks

## Documentation

For importer documentation, see:
- [Architecture Guide](ARCHITECTURE.md)
- [Roadmap](ROADMAP.md)
- [Changelog](CHANGELOG.md)
- [Security](SECURITY.md)
- [Audit Report](AUDIT.md)
- [PostgreSQL Importer](../../docs/importers/POSTGRES_IMPORTER.md)
- [Importers Runbook](../../docs/importers_runbook.md)
- [Importers Roadmap](../../docs/importers_roadmap.md)

## Scientific References

1. Vassiliadis, P., Simitsis, A., & Skiadopoulos, S. (2002). **Conceptual Modeling for ETL Processes**. *Proceedings of the 5th ACM International Workshop on Data Warehousing and OLAP (DOLAP)*, 14–21. https://doi.org/10.1145/583890.583893

2. Kimball, R., & Caserta, J. (2004). **The Data Warehouse ETL Toolkit: Practical Techniques for Extracting, Cleaning, Conforming, and Delivering Data**. Wiley. ISBN: 978-0-764-57923-5

3. Stonebraker, M., Bruckner, D., Ilyas, I. F., Beschastnikh, I., Cherniack, M., & Xu, N. (2013). **Data Curation at Scale: The Data Tamer System**. *Proceedings of the 6th Biennial Conference on Innovative Data Systems Research (CIDR)*. https://www.cidrdb.org/cidr2013/Papers/CIDR13_Paper28.pdf

4. Doan, A., Halevy, A., & Ives, Z. (2012). **Principles of Data Integration**. Morgan Kaufmann. ISBN: 978-0-124-16248-4

5. Shapiro, M., Preguiça, N., Baquero, C., & Zawirski, M. (2011). **Conflict-Free Replicated Data Types**. *Proceedings of the 13th International Conference on Stabilization, Safety, and Security of Distributed Systems (SSS)*, 386–400. https://doi.org/10.1007/978-3-642-24550-3_29

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/importers/README.md`](../../include/importers/README.md) for the public API.
