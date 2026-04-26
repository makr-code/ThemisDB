> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Importers Module Headers

This directory contains header files (.h, .hpp) for the importers module.

## Purpose

Public interfaces and declarations for importers functionality.

## Public Headers

| Header | Class / Interface | Description |
|--------|-------------------|-------------|
| `importer_interface.h` | `IImporter`, `ImportOptions`, `ImportStats` | Core importer interface |
| `importer_interfaces.h` | `IImporterV2` | Extended importer interface v2 |
| `importer_plugin.h` | `ImporterPlugin` | Plugin base for importer extensions |
| `importer_plugin_api.h` | `ImporterPluginAPI` | Plugin registration and lifecycle API |
| `flatfile_importer.h` | `FlatFileImporter` | CSV/TSV/fixed-width flat file ingestion |
| `kafka_importer.h` | `KafkaImporter` | Kafka topic consumer importer |
| `mongo_importer.h` | `MongoImporter` | MongoDB collection importer |
| `mysql_importer.h` | `MySQLImporter` | MySQL table importer |
| `oracle_importer.h` | `OracleImporter` | Oracle DB importer |
| `postgres_importer.h` | `PostgresImporter` | PostgreSQL table importer |
| `postgres_importer_mdm.h` | `PostgresImporterMDM` | PostgreSQL importer with MDM enrichment |
| `postgres_cdc.h` | `PostgresCDC` | PostgreSQL change-data-capture connector |
| `sqlite_importer.h` | `SQLiteImporter` | SQLite database importer |
| `s3_importer.h` | `S3Importer` | AWS S3 object importer |
| `graphql_federation.h` | `GraphQLFederationImporter` | GraphQL federation source importer |
| `schema_inference.h` | `SchemaInference` | Automatic schema detection from raw data |
| `schema_validator.h` | `SchemaValidator` | Import-time schema validation |
| `conflict_resolver.h` | `ConflictResolver` | Merge-conflict resolution strategies |
| `canonical_resolver.h` | `CanonicalResolver` | Entity canonicalization resolver |
| `entity_linker.h` | `EntityLinker` | Cross-source entity linkage |
| `entity_matcher.h` | `EntityMatcher` | Fuzzy entity matching engine |
| `relationship_mapper.h` | `RelationshipMapper` | Relationship extraction and mapping |
| `polyglot_mapper.h` | `PolyglotMapper` | Multi-format field mapping |
| `data_quality.h` | `DataQuality` | Import-time data quality checks |
| `column_importance.h` | `ColumnImportance` | Feature/column importance scoring |
| `adaptive_import.h` | `AdaptiveImport` | Self-tuning import rate controller |
| `temporal_support.h` | `TemporalSupport` | Bitemporal import annotations |
| `crdt_importer.h` | `CrdtImporter` | CRDT-based conflict-free import |
| `federated_learning.h` | `FederatedLearningImporter` | Federated learning data source integration |
| `mdm_engine.h` | `MDMEngine` | Master data management engine |
| `mdm_audit_trail.h` | `MDMAuditTrail` | MDM operation audit log |
| `mdm_metrics.h` | `MDMMetrics` | MDM throughput and quality metrics |
| `audit_trail.h` | `AuditTrail` | General import audit trail |
| `blockchain_integrity.h` | `BlockchainIntegrity` | Blockchain-backed data integrity proofs |
| `gui_import_wizard.h` | `GUIImportWizard` | GUI wizard integration interface |
| `ozg_service_registry.h` | `OZGServiceRegistry` | OZG (German e-government) service registry connector |
| `xoev_importer.h` | `XOEVImporter` | XÖV standard data format importer |

## Documentation

See `../../docs/src/importers/` for detailed module documentation.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "importers/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
