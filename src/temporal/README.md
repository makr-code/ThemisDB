# Temporal Module

Time-series and temporal query support for ThemisDB.

## Module Purpose

Implements temporal and bitemporal data management for ThemisDB, enabling transaction-time and valid-time queries, time travel queries, and bitemporal data versioning.

## Subsystem Scope

**In scope:** Transaction-time tracking, valid-time management, bitemporal query operators, time travel queries, temporal data versioning and retention.

**Out of scope:** Time series storage (handled by timeseries module), event sourcing (handled by cdc module).

## Relevant Interfaces

- `temporal_manager.cpp` — bitemporal data management
- `time_travel_query.cpp` — historical query execution
- `bitemporal_index.cpp` — temporal period indexing
- `temporal_retention.cpp` — temporal data retention

## Current Delivery Status

**Maturity:** 🟡 Beta — Transaction-time tracking and time travel queries operational; bitemporal query operators in progress.

## Components

- Temporal Conflict Resolver: Resolves conflicts between temporal snapshots using HLC timestamps
- System-versioned tables: Track historical changes automatically
- Application-versioned tables: User-controlled time periods
- Time-travel query engine: Query data as it existed at specific points in time
- Temporal joins and aggregations: Join and aggregate across time dimensions
- Retention policies: Automated historical data cleanup

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
├─→ TemporalConflictResolver (HLC-based conflict resolution)
├─→ TemporalQueryEngine (Time-travel query execution)
├─→ TemporalIndexManager (Time-based index optimization)
├─→ RetentionManager (Historical data lifecycle management)
└─→ VersionManager (Version tracking and storage)
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

### System-Versioned Table
```sql
CREATE TABLE employees (
    id INTEGER PRIMARY KEY,
    name TEXT,
    salary DECIMAL,
    PERIOD FOR SYSTEM_TIME
)
WITH SYSTEM VERSIONING;
```

### Application-Versioned Table
```sql
CREATE TABLE contracts (
    id INTEGER PRIMARY KEY,
    customer_id INTEGER,
    valid_from DATE,
    valid_to DATE,
    PERIOD FOR APPLICATION_TIME (valid_from, valid_to)
);
```

### Retention Policy
```sql
ALTER TABLE employees
SET RETENTION_PERIOD = INTERVAL '1 YEAR';
```

### Time-Travel Query
```sql
-- Query data as of specific time
SELECT * FROM employees
FOR SYSTEM_TIME AS OF '2024-01-01 00:00:00';

-- Query all versions in time range
SELECT * FROM employees
FOR SYSTEM_TIME FROM '2024-01-01' TO '2024-12-31';
```

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
- [Temporal Conflict Resolver](../../docs/temporal/temporal_conflict_resolver.md)
- [Temporal Query Engine](../../docs/temporal/temporal_query_engine.md)
- [Retention Policies](../../docs/temporal/retention_policies.md)
- [Architecture Guide](../../ARCHITECTURE.md) - Temporal data model
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) - Planned features

## Version History

- **v1.0.0**: Initial temporal conflict resolver with HLC support
- **v1.1.0**: Planned - System-versioned table support
- **v1.2.0**: Planned - Time-travel query engine
- **v1.3.0**: Planned - Retention policy automation

## See Also

- [Header Documentation](../../include/temporal/README.md) - Public API definitions
- [Replication Module](../replication/README.md) - HLC and multi-master support
- [Storage Module](../storage/README.md) - Underlying storage layer
- [Query Module](../query/README.md) - Query execution integration
