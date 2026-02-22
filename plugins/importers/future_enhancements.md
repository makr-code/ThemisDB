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

- T. Akidau et al., "The dataflow model: A practical approach to balancing correctness, latency, and cost in massive-scale, unbounded, out-of-order data processing," *Proc. VLDB Endow.*, vol. 8, no. 12, pp. 1792–1803, 2015. DOI: [10.14778/2824032.2824076](https://doi.org/10.14778/2824032.2824076)
- M. Stonebraker and L. A. Rowe, "The design of POSTGRES," in *Proc. 1986 ACM SIGMOD International Conf. Management of Data*, 1986, pp. 340–355. DOI: [10.1145/16894.16888](https://doi.org/10.1145/16894.16888)
- C. Mohan et al., "ARIES: A transaction recovery method supporting fine-granularity locking and partial rollbacks using write-ahead logging," *ACM Trans. Database Syst.*, vol. 17, no. 1, pp. 94–162, Mar. 1992. DOI: [10.1145/128765.128770](https://doi.org/10.1145/128765.128770)
- A. Reuter and H. Härder, "Principles of transaction-oriented database recovery," *ACM Comput. Surv.*, vol. 15, no. 4, pp. 287–317, Dec. 1983. DOI: [10.1145/289.291](https://doi.org/10.1145/289.291)
- M. J. Carey and D. J. DeWitt, "Of objects and databases: A decade of turmoil," in *Proc. 22nd International Conf. Very Large Data Bases (VLDB)*, 1996, pp. 3–14.
