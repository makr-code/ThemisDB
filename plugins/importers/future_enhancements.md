# Importer Plugins – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

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

- [ ] **Apache Kafka consumer** – consume and ingest Kafka topic messages in real time.
- [ ] **MQTT** – IoT message ingestion.
- [ ] **WebSocket** – live data feed ingestion.

### Data Quality

- [ ] **Pre-import validation** – schema mismatch detection, null-check, referential integrity.
- [ ] **Duplicate detection** – hash-based deduplication during import.
- [ ] **Transformation DSL** – lightweight field mapping / renaming / type casting.

---

## Research / References

- [ ] TODO: Add reference – *Debezium: Change Data Capture* (URL placeholder)
- [ ] TODO: Add reference – *The Data Warehouse Toolkit* – Kimball (ISBN placeholder)
- [ ] TODO: Add reference – *Streaming Systems* – Akidau et al. (ISBN placeholder)
