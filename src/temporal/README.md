> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Temporal Module

Time-series and temporal query support for ThemisDB.

## Module Purpose

Implements temporal and bitemporal data management for ThemisDB, enabling transaction-time and valid-time queries, time travel queries, and bitemporal data versioning.

## Subsystem Scope

**In scope:** Transaction-time tracking, valid-time management, bitemporal query operators, time travel queries, temporal data versioning and retention.

**Out of scope:** Time series storage (handled by timeseries module), event sourcing (handled by cdc module).

## Relevant Interfaces

- `temporal_query_engine.cpp` — time-travel query execution (`AS OF`, `FROM...TO`, `BETWEEN...AND`, bitemporal joins, SEQUENCED/NON-SEQUENCED semantics)
- `system_versioned_table.cpp` — automatic transaction-time versioning of all table rows
- `bi_temporal.cpp` — bitemporal record management (system time + valid time axes)
- `temporal_index.cpp` — period-based B-tree index for efficient time range queries
- `temporal_aggregator.cpp` — temporal aggregations (tumbling and sliding window)
- `temporal_conflict_resolver.cpp` — HLC-based conflict resolution for concurrent edits
- `snapshot_manager.cpp` — temporal snapshot creation, querying, and release
- `retention_manager.cpp` — automated expiry of old versions based on retention policy
- `interval_tree_index.cpp` — augmented interval tree, O(log n + k) overlap detection for valid-time period predicates
- `temporal_compressor.cpp` — DELTA/ZSTD/Gorilla/dictionary compression for historical version payloads
- `temporal_cdc.cpp` — versioned change data capture; typed ChangeEvent (INSERT/UPDATE/DELETE/VERSION_CREATED), pub/sub subscriptions, ring-buffer replay

## Public API Entry Points (`include/temporal`)

| Header | Primary responsibility |
|---|---|
| `temporal/system_versioned_table.h` | System-time history tracking for mutable rows |
| `temporal/bi_temporal.h` | Bi-temporal rows with valid-time + system-time APIs |
| `temporal/temporal_query_engine.h` | `AS OF`, `FROM...TO`, `BETWEEN...AND`, semantics-aware temporal queries |
| `temporal/retention_manager.h` | Time/count/storage based retention policy execution |
| `temporal/temporal_cdc.h` | Temporal change event streaming and replay |
| `temporal/temporal_compressor.h` | Compression/decompression for historical payloads |
| `temporal/snapshot_manager.h` | Consistent point-in-time snapshots across multiple tables |
| `temporal/temporal_migrator.h` | Migration tooling from non-temporal to temporal tables |

See [`../../include/temporal/README.md`](../../include/temporal/README.md) for usage snippets per header.

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — Transaction-time and valid-time tracking, time-travel queries, bitemporal joins, SEQUENCED/NON-SEQUENCED query semantics, temporal aggregations, conflict resolution, snapshot management, and retention policies are all fully implemented and production-ready. SQL `PERIOD FOR` DDL syntax is not yet supported.

Phase 4 (v1.5.0): IntervalTreeIndex, TemporalCompressor, and TemporalCDC are implemented and production-ready.

## Components

- Temporal Conflict Resolver: Resolves conflicts between temporal snapshots using HLC timestamps
- System-versioned tables: Track historical changes automatically
- Application-versioned tables: User-controlled time periods
- Time-travel query engine: Query data as it existed at specific points in time
- Temporal joins and aggregations: Join and aggregate across time dimensions
- Retention policies: Automated historical data cleanup
- IntervalTreeIndex: Augmented BST-based interval tree for efficient valid-time overlap detection
- TemporalCompressor: Compresses historical version payloads using DELTA, ZSTD, Gorilla, and DICTIONARY algorithms
- TemporalCDC: Version-aware change data capture; subscribe to table change streams and replay events by time range

## Features

### Temporal Tables
- **System-versioned tables**: Automatic tracking of all changes with system timestamps
- **Application-versioned tables**: User-defined valid time periods for bi-temporal support
- **Transaction-time tracking**: Record when data was stored in the database
- **Valid-time tracking**: Record when data is valid in the real world

### Time-Travel Queries
- **AS OF queries**: Retrieve data as it existed at a specific point in time
- **FROM...TO queries**: Retrieve all versions of data within a time range
- **BETWEEN...AND queries**: Query data valid during a specific period
- **Historical snapshots**: Create point-in-time snapshots for analysis

### Conflict Resolution
- **HLC-based ordering**: Use Hybrid Logical Clocks for distributed timestamp ordering
- **Multiple policies**: Last-write-wins, first-write-wins, node-priority, manual, CRDT-merge
- **Conflict detection**: Automatic detection of concurrent modifications
- **Resolution logging**: Track all conflict resolutions for audit purposes

### Temporal Operations
- **Temporal joins**: Join tables based on temporal overlap or specific time points
- **Temporal aggregations**: Aggregate data across time windows
- **Period operations**: Union, intersection, and difference of time periods
- **Temporal predicates**: OVERLAPS, CONTAINS, PRECEDES, SUCCEEDS

### Retention and Optimization
- **Configurable retention policies**: Automatically purge old historical data
- **Temporal indexes**: Specialized indexes for time-based queries
- **Compression**: Historical data compression to save storage
- **Partitioning**: Time-based partitioning for performance

## Architecture

```
TemporalModule
├─→ TemporalQueryEngine     (time-travel queries, bitemporal joins, SEQUENCED/NON-SEQUENCED semantics)
├─→ SystemVersionedTable    (automatic transaction-time versioning)
├─→ BiTemporalTable         (dual-axis: system time + valid time)
├─→ TemporalIndex           (period B-tree index for fast range lookups)
├─→ TemporalAggregator      (tumbling and sliding window aggregations)
├─→ TemporalConflictResolver (HLC-based conflict resolution with five policies)
├─→ TemporalSnapshotManager (consistent multi-table point-in-time snapshots)
├─→ RetentionManager        (time-based and count-based history cleanup)
├─→ IntervalTreeIndex       (augmented BST for O(log n + k) overlap detection)
├─→ TemporalCompressor      (DELTA/ZSTD/Gorilla/dictionary compression for history)
└─→ TemporalCDC             (pub/sub change data capture with ring-buffer replay)
```

## Use Cases

### Audit and Compliance
- Track all changes to sensitive data
- Comply with regulatory requirements (GDPR, HIPAA, SOX)
- Provide complete audit trails
- Support data lineage tracking

### Historical Analysis
- Analyze trends over time
- Compare current vs. historical data
- Identify patterns and anomalies
- Generate historical reports

### Point-in-Time Recovery
- Restore data to any previous state
- Undo unwanted changes
- Investigate data corruption issues
- Test against historical data

### Temporal Data Modeling
- Model real-world temporal relationships
- Support bi-temporal data (transaction time + valid time)
- Handle slowly changing dimensions
- Track entity lifecycles

## Performance Characteristics

- **Read queries**: Historical queries incur additional overhead for version filtering
- **Write queries**: System-versioned tables add ~10-20% write overhead for history tracking
- **Storage**: Historical data requires additional storage (configurable with retention policies)
- **Indexes**: Temporal indexes improve time-based query performance by 10-100x
- **Compression**: Historical data compresses well (typical 3-5x compression ratio)

## Configuration

> **Note:** The SQL DDL syntax shown below (`PERIOD FOR`, `WITH SYSTEM VERSIONING`,
> `FOR SYSTEM_TIME`, `ALTER TABLE … SET RETENTION_PERIOD`) is **not yet supported** in
> the AQL parser.  The same functionality is fully available through the C++ API
> (`SystemVersionedTable`, `BiTemporalTable`, `RetentionManager`).

### System-Versioned Table (C++ API)
```cpp
SystemVersionedTable employees("employees");
employees.insert("emp1", {{"name","Alice"},{"salary",90000}});
employees.update("emp1", {{"salary",95000}});
auto snapshot = employees.scan(as_of_timestamp);
```

### Planned DDL Syntax (not yet supported)
```sql
-- Target: Q3 2026
CREATE TABLE employees (
    id INTEGER PRIMARY KEY,
    name TEXT,
    salary DECIMAL,
    PERIOD FOR SYSTEM_TIME
)
WITH SYSTEM VERSIONING;
```

### Application-Versioned Table (C++ API)
```cpp
BiTemporalTable contracts("contracts");
contracts.insert("c1", doc, valid_from, valid_to);
auto rows = contracts.scanBiTemporal(sys_as_of, valid_at);
```

### Planned DDL Syntax (not yet supported)
```sql
-- Target: Q3 2026
CREATE TABLE contracts (
    id INTEGER PRIMARY KEY,
    customer_id INTEGER,
    valid_from DATE,
    valid_to DATE,
    PERIOD FOR APPLICATION_TIME (valid_from, valid_to)
);
```

### Retention Policy (C++ API)
```cpp
RetentionManager rm;
rm.setPolicy("employees", {RetentionPolicy::Type::TIME_BASED, 365 /* days */});
rm.enforceRetention("employees");
```

### Planned DDL Syntax (not yet supported)
```sql
-- Target: Q3 2026
ALTER TABLE employees
SET RETENTION_PERIOD = INTERVAL '1 YEAR';
```

### Time-Travel Query (C++ API)
```cpp
TemporalQueryEngine engine(table);
auto rows = engine.queryAsOf(target_ts);
auto history = engine.queryFromTo(t_start, t_end);
```

### Planned SQL Syntax (not yet supported)
```sql
-- Target: Q3 2026
SELECT * FROM employees
FOR SYSTEM_TIME AS OF '2024-01-01 00:00:00';

SELECT * FROM employees
FOR SYSTEM_TIME FROM '2024-01-01' TO '2024-12-31';
```

## Runtime Behavior, Error Cases, and Limits

- **Versioning model:** updates and deletes create/close versions instead of mutating history in place.
- **Concurrency:** conflict resolution uses HLC ordering policies; manual/conflict queues must be drained by operators when MANUAL policy is configured.
- **Retention behavior:** `RetentionManager` enforcement can run in background intervals; deletion/archive behavior depends on configured policy type.
- **Parser boundary:** SQL/AQL temporal syntax (`PERIOD FOR`, `FOR SYSTEM_TIME`, `FOR APPLICATION_TIME`) is still deferred; use C++ APIs for production usage today.
- **Operational limits:** history growth is workload-dependent and requires explicit retention + compression tuning to keep storage bounded.

## Integration Points

- **Storage Layer**: Extended key schema for version tracking
- **Query Engine**: Temporal query operators and predicates
- **Index Layer**: Specialized temporal indexes
- **Replication**: Temporal conflict resolution for distributed scenarios
- **Backup/Recovery**: Point-in-time restore capabilities

## Thread Safety

- Thread-safe conflict resolution with concurrent snapshot handling
- Lock-free temporal query execution for read-heavy workloads
- Coordinated version creation to prevent conflicts
- Safe retention policy enforcement with background cleanup

## Dependencies

- RocksDB: Underlying storage for temporal data
- HLC (Hybrid Logical Clock): Distributed timestamp ordering
- Replication Module: Multi-master conflict resolution
- Index Module: Temporal index support

## Documentation

For detailed implementation documentation, see:
- [Architecture Guide](ARCHITECTURE.md) - Temporal module architecture and data flow
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) - Planned features
- [ROADMAP](ROADMAP.md) - Implementation roadmap and status
- [German Overview](../../docs/de/temporal/README.md) - Modulzusammenfassung und Navigationshilfe

## Version History

- **v1.0.0**: HLC-based temporal conflict resolver with five resolution policies
- **v1.1.0**: System-versioned table with automatic transaction-time history
- **v1.2.0**: BiTemporalTable (system time + valid time), TemporalIndex (period B-tree), TemporalQueryEngine (`AS OF`, `FROM...TO`, `BETWEEN...AND`)
- **v1.3.0**: RetentionManager (time-based and count-based policies), TemporalAggregator (tumbling/sliding window), TemporalSnapshotManager
- **v1.4.0**: Bitemporal joins (`joinBiTemporal`), SEQUENCED/NON-SEQUENCED query semantics (`queryWithSemantics`)
- **v1.5.0**: IntervalTreeIndex (augmented BST, max-end tracking, O(log n + k) overlap queries), TemporalCompressor (DELTA/ZSTD/Gorilla/dictionary compression), TemporalCDC (pub/sub change events with ring-buffer replay)

## See Also

- [Header Documentation](../../include/temporal/README.md) - Public API definitions
- [Replication Module](../replication/README.md) - HLC and multi-master support
- [Storage Module](../storage/README.md) - Underlying storage layer
- [Query Module](../query/README.md) - Query execution integration

## Scientific References

1. Jensen, C. S., & Snodgrass, R. T. (1999). **Temporal Data Management**. *IEEE Transactions on Knowledge and Data Engineering*, 11(1), 36–44. https://doi.org/10.1109/69.755613

2. Snodgrass, R. T. (1987). **The Temporal Query Language TQuel**. *ACM Transactions on Database Systems*, 12(2), 247–298. https://doi.org/10.1145/22952.22956

3. Kulkarni, K., & Michels, J.-E. (2012). **Temporal Features in SQL:2011**. *ACM SIGMOD Record*, 41(3), 34–43. https://doi.org/10.1145/2380776.2380786

4. Lamport, L. (1978). **Time, Clocks, and the Ordering of Events in a Distributed System**. *Communications of the ACM*, 21(7), 558–565. https://doi.org/10.1145/359545.359563

5. Dalgaard, P., & Jensen, C. S. (2001). **On the Representation of Valid Time in a Temporal Relational Database**. *VLDB Journal*, 10(2–3), 188–205. https://doi.org/10.1007/s007780100041

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/temporal/README.md`](../../include/temporal/README.md) for the public API.

## Troubleshooting

- **No rows returned for past timestamps:** verify whether requested timestamp is within system-valid range (`sys_start`/`sys_end`) and whether retention already purged older versions.
- **Unexpected conflict winner:** validate HLC progression and configured `ConflictPolicy`.
- **High storage usage:** enable/use `TemporalCompressor` and enforce explicit retention rules (`RetentionManager`).
