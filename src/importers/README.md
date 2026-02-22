# Importers Module

Data import functionality for ThemisDB.

## Module Purpose

Provides data import functionality for ThemisDB, currently supporting PostgreSQL with schema mapping, batch import, and incremental import support.

## Subsystem Scope

**In scope:** PostgreSQL database import with schema mapping, batch import operations, incremental import via change tracking.

**Out of scope:** Data transformation beyond schema mapping (handled by content module), export functionality (handled by exporters module), CDC-based ongoing sync (handled by cdc module).

## Relevant Interfaces

- `postgres_importer.cpp` — PostgreSQL source connector with schema mapping
- `import_pipeline.cpp` — import orchestration and batching
- `schema_mapper.cpp` — source-to-ThemisDB schema translation

## Current Delivery Status

**Maturity:** 🟡 Beta — PostgreSQL importer operational; MySQL, MongoDB, and flat-file importers planned.

## Components

- PostgreSQL importer
- Custom import format handlers
- Import pipeline

## Features

- Import data from PostgreSQL databases
- Schema mapping and transformation
- Batch import operations
- Incremental import support

## Documentation

For importer documentation, see:
- [PostgreSQL Importer](../../docs/importers/POSTGRES_IMPORTER.md)

## Scientific References

1. Vassiliadis, P., Simitsis, A., & Skiadopoulos, S. (2002). **Conceptual Modeling for ETL Processes**. *Proceedings of the 5th ACM International Workshop on Data Warehousing and OLAP (DOLAP)*, 14–21. https://doi.org/10.1145/583890.583893

2. Kimball, R., & Caserta, J. (2004). **The Data Warehouse ETL Toolkit: Practical Techniques for Extracting, Cleaning, Conforming, and Delivering Data**. Wiley. ISBN: 978-0-764-57923-5

3. Stonebraker, M., Bruckner, D., Ilyas, I. F., Beschastnikh, I., Cherniack, M., & Xu, N. (2013). **Data Curation at Scale: The Data Tamer System**. *Proceedings of the 6th Biennial Conference on Innovative Data Systems Research (CIDR)*. https://www.cidrdb.org/cidr2013/Papers/CIDR13_Paper28.pdf

4. Doan, A., Halevy, A., & Ives, Z. (2012). **Principles of Data Integration**. Morgan Kaufmann. ISBN: 978-0-124-16248-4
