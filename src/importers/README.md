# Importers Module

Data import functionality for ThemisDB.

## Module Purpose

Provides data import functionality for ThemisDB, supporting PostgreSQL, MySQL/MariaDB,
MongoDB, and SQLite with schema mapping, batch import, and incremental import support.

## Subsystem Scope

**In scope:** Database dump import with schema mapping, batch import operations,
incremental import via change tracking, streaming row callbacks, dry-run mode.

**Out of scope:** Data transformation beyond schema mapping (handled by content module),
export functionality (handled by exporters module), CDC-based ongoing sync (handled by
cdc module).

## Relevant Interfaces

- `postgres_importer.cpp` — PostgreSQL pg_dump source connector with schema mapping,
  COPY-protocol support, checkpoint/resume, conflict resolution, and quarantine
- `mysql_importer.cpp` — MySQL / MariaDB mysqldump source connector
- `mongo_importer.cpp` — MongoDB mongoexport JSON/NDJSON source connector
- `sqlite_importer.cpp` — SQLite `.dump` source connector (type-affinity mapping,
  single-quoted + hex literal parsing, BEGIN TRANSACTION / COMMIT / PRAGMA handling)
- `conflict_resolver.cpp` — pluggable conflict resolution strategies (skip, overwrite, merge)
- `import_pipeline.cpp` — import orchestration and batching
- `schema_mapper.cpp` — source-to-ThemisDB schema translation

## Current Delivery Status

**Maturity:** 🟢 Production — PostgreSQL, MySQL/MariaDB, MongoDB, and SQLite importers
operational. CSV/TSV/Parquet and cloud-storage importers planned.

## Components

- PostgreSQL importer
- MySQL / MariaDB importer
- MongoDB importer
- SQLite importer
- Conflict resolver
- Custom import format handlers
- Import pipeline

## Features

- Import data from PostgreSQL, MySQL/MariaDB, MongoDB, and SQLite
- Schema mapping and type-affinity transformation
- Batch import operations with configurable chunk size
- Incremental import support (watermark-based change tracking, checkpoint/resume)
- Dry-run mode (validate without writing data)
- Streaming row callback for real-time progress
- Include/exclude table filtering
- Permission-check callback (ACL enforcement)
- Metrics and distributed-tracing observability hooks

## Documentation

For importer documentation, see:
- [PostgreSQL Importer](../../docs/importers/POSTGRES_IMPORTER.md)
- [Importers Runbook](../../docs/importers_runbook.md)
- [Importers Roadmap](../../docs/importers_roadmap.md)

## Scientific References

1. Vassiliadis, P., Simitsis, A., & Skiadopoulos, S. (2002). **Conceptual Modeling for ETL Processes**. *Proceedings of the 5th ACM International Workshop on Data Warehousing and OLAP (DOLAP)*, 14–21. https://doi.org/10.1145/583890.583893

2. Kimball, R., & Caserta, J. (2004). **The Data Warehouse ETL Toolkit: Practical Techniques for Extracting, Cleaning, Conforming, and Delivering Data**. Wiley. ISBN: 978-0-764-57923-5

3. Stonebraker, M., Bruckner, D., Ilyas, I. F., Beschastnikh, I., Cherniack, M., & Xu, N. (2013). **Data Curation at Scale: The Data Tamer System**. *Proceedings of the 6th Biennial Conference on Innovative Data Systems Research (CIDR)*. https://www.cidrdb.org/cidr2013/Papers/CIDR13_Paper28.pdf

4. Doan, A., Halevy, A., & Ives, Z. (2012). **Principles of Data Integration**. Morgan Kaufmann. ISBN: 978-0-124-16248-4
