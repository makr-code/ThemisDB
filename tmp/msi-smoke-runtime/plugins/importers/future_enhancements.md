# Importer Plugins – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

## Scope

- New importer source connectors: Oracle Database, Microsoft SQL Server, Apache Kafka (streaming), Apache Avro files, ORC files.
- Enhancements to existing connectors: PostgreSQL COPY protocol optimisation, MongoDB change streams, S3 multipart parallel read.
- Data quality validation pipeline: schema mismatch detection, null-rate checks, duplicate detection, transformation DSL.
- Entry-points: `plugins/importers/`; core implementation in `src/importers/`.

## Design Constraints

- [ ] All RDBMS importers MUST use parameterised queries exclusively; no string concatenation for SQL generation.
- [ ] Database credentials and connection strings MUST be sourced from environment variables or secret manager; never hardcoded or logged.
- [ ] Connection strings MUST be sanitised before inclusion in any error message (passwords replaced with `***`).
- [ ] Streaming importers (Kafka) MUST support exactly-once semantics via idempotent consumer group offsets.
- [ ] Importers MUST stream records; maximum in-process buffer ≤ 64 MB regardless of source table size.
- [ ] Data quality validation MUST run before records are written to ThemisDB; failed records are routed to a configurable dead-letter collection.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IImporter` | `ImporterPlugin`, ThemisDB ingest pipeline | `connect()`, `stream_records()`, `disconnect()` |
| `IConnectionConfig` | `IImporter` impls | Validates and redacts connection parameters at construction time |
| `IDataQualityValidator` | `ImporterPlugin` | Schema check, null-rate, duplicate hash; returns pass/fail per record |
| `IDeadLetterRouter` | `ImporterPlugin` | Routes failed-validation records to a named dead-letter collection |
| `ITransformationDSL` | `ImporterPlugin` | Field mapping, renaming, type casting; declarative YAML/JSON config |

## Idea Backlog

### Additional Source Systems

- [ ] **Oracle Database** – enterprise RDBMS importer.
- [ ] **Microsoft SQL Server** – via ODBC / FreeTDS.
- [ ] **Cassandra / ScyllaDB** – wide-column NoSQL importer.
- [ ] **DynamoDB** – AWS NoSQL importer.
- [ ] **Neo4j** – graph database importer (Cypher → ThemisDB AQL).
- [ ] **Apache Hive / Spark** – analytical data warehouse ingestion.
- [ ] **Salesforce / CRM APIs** – SaaS data source integration.

### File Formats

- [ ] **Parquet / ORC** – columnar file format ingestion.
- [ ] **Avro** – schema-aware binary format.
- [ ] **XML / JSON Lines** – structured text file ingestion.
- [ ] **Excel (XLSX)** – spreadsheet import.

### Streaming Ingestion

- [x] **Apache Kafka consumer** – consume and ingest Kafka topic messages in real time. (`src/importers/kafka_importer.cpp`)
- [ ] **MQTT** – IoT message ingestion.
- [ ] **WebSocket** – live data feed ingestion.

### Data Quality

- [ ] **Pre-import validation** – schema mismatch detection, null-check, referential integrity.
- [ ] **Duplicate detection** – hash-based deduplication during import.
- [ ] **Transformation DSL** – lightweight field mapping / renaming / type casting.

---

## Test Strategy

- SQL injection tests for all RDBMS importers: assert that input containing SQL metacharacters does not alter the executed query structure.
- Credential redaction tests: trigger a connection failure; assert the raw password does not appear in the error message or logs.
- Throughput benchmark tests: PostgreSQL import of 500,000 rows must complete within 10 s; recorded as a CI regression gate.
- Schema mismatch tests: change source table schema mid-import; assert `SCHEMA_MISMATCH` event and records routed to dead-letter collection.
- Duplicate detection tests: import the same 1,000 records twice; assert exactly 1,000 records in the target collection.
- Kafka exactly-once tests: restart consumer mid-stream; assert no duplicate or missing records after recovery.

## Performance Targets

- PostgreSQL import throughput ≥ 50,000 rows/s using COPY protocol (single table, 10 columns, VARCHAR/INT mix).
- MongoDB import throughput ≥ 20,000 documents/s (documents ≤ 2 KB each, single collection).
- S3 file import throughput ≥ 100 MB/s with 4 parallel part readers.
- Oracle/MSSQL import throughput ≥ 10,000 rows/s (JDBC/ODBC baseline; COPY-equivalent optimisation is a stretch goal).
- Data quality validation overhead ≤ 5 % of total import wall-clock time for standard checks.

## Security / Reliability

- SQL injection prevention is mandatory on all RDBMS importers; enforced via parameterised queries with no exceptions.
- Database credentials MUST never appear in log output, error responses, or metrics labels.
- Connection strings MUST be sanitised in all error messages: passwords replaced with `***` before any logging or surfacing to callers.
- Streaming importers MUST handle upstream disconnections with automatic reconnect and exponential back-off (max 60 s).
- Dead-letter routing MUST be atomic: a record is either in the target collection or the dead-letter collection, never in neither or both.

## Research / References

- T. Akidau et al., "The dataflow model: A practical approach to balancing correctness, latency, and cost in massive-scale, unbounded, out-of-order data processing," *Proc. VLDB Endow.*, vol. 8, no. 12, pp. 1792–1803, 2015. DOI: [10.14778/2824032.2824076](https://doi.org/10.14778/2824032.2824076)
- M. Stonebraker and L. A. Rowe, "The design of POSTGRES," in *Proc. 1986 ACM SIGMOD International Conf. Management of Data*, 1986, pp. 340–355. DOI: [10.1145/16894.16888](https://doi.org/10.1145/16894.16888)
- C. Mohan et al., "ARIES: A transaction recovery method supporting fine-granularity locking and partial rollbacks using write-ahead logging," *ACM Trans. Database Syst.*, vol. 17, no. 1, pp. 94–162, Mar. 1992. DOI: [10.1145/128765.128770](https://doi.org/10.1145/128765.128770)
- A. Reuter and H. Härder, "Principles of transaction-oriented database recovery," *ACM Comput. Surv.*, vol. 15, no. 4, pp. 287–317, Dec. 1983. DOI: [10.1145/289.291](https://doi.org/10.1145/289.291)
- M. J. Carey and D. J. DeWitt, "Of objects and databases: A decade of turmoil," in *Proc. 22nd International Conf. Very Large Data Bases (VLDB)*, 1996, pp. 3–14.
