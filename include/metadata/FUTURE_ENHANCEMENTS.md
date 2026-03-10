# Metadata Module API - Future Enhancements
<!-- status: current | validated: 2026-03-10 | commit: 4c1a2dfc1 -->

## Scope

- API-level enhancements to `include/metadata/` public C++ headers
- Statistics collector API for table and column statistics used by the query planner
- Schema version tracking interface with rollback support
- INFORMATION_SCHEMA view registration API for system catalog introspection
- Live schema change API for non-blocking column additions
- `SchemaManager` catalog persistence extensions for metadata persistence and lookup

## Design Constraints

- [ ] Statistics are updated asynchronously; `StatisticsCollector` API must not block write paths
- [ ] Schema change API is transactional; partial schema changes must roll back atomically on error
- [ ] `SchemaManager` catalog interface is append-only for new columns; existing column descriptors are immutable
- [ ] `InformationSchema` views are read-only; no mutation methods exposed via this interface
- [ ] `SchemaVersionManager` exposes version history as an immutable sequence; deletions not permitted
- [ ] All public metadata APIs return `Result<T>`; no raw exceptions cross header boundaries

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `StatisticsCollector` | Query planner, optimizer | Async stats, no write blocking |
| `InformationSchema` | SQL layer, admin tools | Read-only catalog views |
| `SchemaVersionManager` | Migration tooling, admin | Transactional rollback support |
| `SchemaManager` (catalog API) | Storage engine, DDL layer | Append-only column metadata |

## Planned API Extensions

### Statistics API
**Priority:** High  
**Target Version:** v1.6.0

```cpp
// metadata/statistics.h
namespace themis {

class StatisticsCollector {
public:
    struct TableStats {
        size_t row_count;
        uint64_t size_bytes;
        std::map<std::string, ColumnStats> columns;
    };
    
    struct ColumnStats {
        size_t distinct_count;
        size_t null_count;
        double selectivity;
    };
    
    Result<TableStats> getStats(const std::string& table_name);
    Result<bool> updateStats(const std::string& table_name);
};

}
```

---

### Information Schema API
**Priority:** High  
**Target Version:** v1.7.0

```cpp
// metadata/information_schema.h
namespace themis {

class InformationSchema {
public:
    Result<std::vector<Table>> getTables();
    Result<std::vector<Column>> getColumns(const std::string& table_name);
    Result<std::vector<Index>> getIndexes(const std::string& table_name);
};

}
```

---

### Schema Version API
**Priority:** Medium  
**Target Version:** v1.8.0

```cpp
// metadata/schema_version.h
namespace themis {

class SchemaVersionManager {
public:
    Result<uint64_t> getCurrentVersion(const std::string& table_name);
    Result<std::vector<SchemaChange>> getHistory(const std::string& table_name);
    Result<bool> rollbackToVersion(const std::string& table_name, uint64_t version);
};

}
```

---

## Test Strategy

- Unit tests for `StatisticsCollector`: verify stats are updated asynchronously and do not block concurrent writes
- Integration tests for `SchemaVersionManager`: rollback leaves the catalog in a consistent pre-change state
- Contract tests for `InformationSchema`: `getTables()` and `getColumns()` reflect all DDL changes within one transaction
- Header-only compilation tests: each planned header must compile in isolation without `src/` includes
- Regression tests ensuring `SchemaManager` catalog append-only contract: existing column descriptors unchanged after `addColumn()`
- Fuzz tests for schema change API with malformed column descriptors

## Performance Targets

- `StatisticsCollector::getStats`: ≤ 1 ms p99 for tables up to 1B rows
- `SchemaManager` catalog schema lookup: ≤ 100 µs p99 (in-memory cache hit path)
- `SchemaVersionManager::getCurrentVersion`: ≤ 10 µs (single atomic read)
- `InformationSchema::getTables`: ≤ 500 µs for catalogs with up to 10K tables
- Async statistics update: zero measurable impact on write throughput (< 1% overhead)
- `SchemaVersionManager::rollbackToVersion`: ≤ 50 ms for schemas with ≤ 100 column changes

## Security / Reliability

- Schema change API enforces admin privilege check at header API entry; non-admin calls return `Error::PermissionDenied`
- Statistics data structures never include row values or column data; only aggregate counts and distributions
- `SchemaManager` catalog mutations are journaled; a crash during a schema change results in automatic rollback on restart
- `InformationSchema` read path is isolated from write path; no lock contention with schema changes
- Schema version history is append-only and tamper-evident; no version entries may be deleted via the public API
- All column descriptor fields validated for length and character set before acceptance into the catalog

## Scientific References (IEEE)

The following references underpin the planned API design decisions and design constraints in this module.

[1] Y. E. Ioannidis and V. Poosala, "Balancing Histogram Optimality and Practicality for Query Result Size Estimation," in *Proc. ACM SIGMOD Int. Conf. Management of Data*, San Jose, CA, USA, 1995, pp. 233–244. https://doi.org/10.1145/223784.223816

[2] S. Chaudhuri and V. R. Narasayya, "An Efficient Cost-Driven Index Selection Tool for Microsoft SQL Server," in *Proc. 23rd Int. Conf. Very Large Data Bases (VLDB)*, Athens, Greece, 1997, pp. 146–155. https://dl.acm.org/doi/10.5555/645923.671506

[3] J. F. Roddick, "A Survey of Schema Versioning Issues for Database Systems," *Information and Software Technology*, vol. 37, no. 7, pp. 383–393, 1995. https://doi.org/10.1016/0950-5849(95)91245-R

[4] P. Buneman, S. Khanna, and W. C. Tan, "Why and Where: A Characterization of Data Provenance," in *Proc. 8th Int. Conf. Database Theory (ICDT)*, London, UK, 2001, pp. 316–330. https://doi.org/10.1007/3-540-44503-X_20

[5] W3C, "PROV-DM: The PROV Data Model," W3C Recommendation, Apr. 2013. https://www.w3.org/TR/prov-dm/

[6] ISO/IEC, *Information Technology – Database Languages – SQL – Part 11: Information and Definition Schemas (SQL/Schemata)*, ISO/IEC 9075-11:2016, 2016. https://www.iso.org/standard/63556.html

[7] M. Stonebraker and J. Hellerstein, Eds., *Readings in Database Systems*, 4th ed. Cambridge, MA, USA: MIT Press, 2005. ISBN: 978-0-262-69314-1

## See Also

- [Current API](README.md)
- [Implementation FUTURE_ENHANCEMENTS](../../src/metadata/FUTURE_ENHANCEMENTS.md)

---

*Last Updated: March 2026*  
*Target API Version: v1.6.0*
