# ThemisDB Module Documentation Index

This directory contains comprehensive documentation for all ThemisDB modules, consolidated from the source code directories.

## Module Categories

### Foundation Layer (6 modules)

- **[core](core/)** - Core database functionality
  - [README](core/README.md) | [FUTURE_ENHANCEMENTS](core/FUTURE_ENHANCEMENTS.md) | [Header Docs](core/include_README.md)
- **[storage](storage/)** - Storage engine and persistence
  - [README](storage/README.md) | [FUTURE_ENHANCEMENTS](storage/FUTURE_ENHANCEMENTS.md) | [Header Docs](storage/include_README.md)
- **[transaction](transaction/)** - Transaction management
  - [README](transaction/README.md) | [FUTURE_ENHANCEMENTS](transaction/FUTURE_ENHANCEMENTS.md) | [Header Docs](transaction/include_README.md)
- **[themis](themis/)** - Core Themis functionality
  - [README](themis/README.md) | [FUTURE_ENHANCEMENTS](themis/FUTURE_ENHANCEMENTS.md) | [Header Docs](themis/include_README.md)
- **[base](base/)** - Base types and utilities
  - [README](base/README.md) | [FUTURE_ENHANCEMENTS](base/FUTURE_ENHANCEMENTS.md)
- **[utils](utils/)** - General utilities
  - [README](utils/README.md) | [Header Docs](utils/include_README.md)

### Query & Index (6 modules)

- **[query](query/)** - Query engine
  - [README](query/README.md) | [FUTURE_ENHANCEMENTS](query/FUTURE_ENHANCEMENTS.md) | [Header Docs](query/include_README.md)
- **[aql](aql/)** - AQL (Advanced Query Language)
  - [README](aql/README.md) | [FUTURE_ENHANCEMENTS](aql/FUTURE_ENHANCEMENTS.md) | [Header Docs](aql/include_README.md)
- **[index](index/)** - Indexing system
  - [README](index/README.md) | [FUTURE_ENHANCEMENTS](index/FUTURE_ENHANCEMENTS.md) | [Header Docs](index/include_README.md)
- **[search](search/)** - Search functionality
  - [README](search/README.md) | [FUTURE_ENHANCEMENTS](search/FUTURE_ENHANCEMENTS.md) | [Header Docs](search/include_README.md)
- **[temporal](temporal/)** - Temporal data handling
  - [README](temporal/README.md) | [FUTURE_ENHANCEMENTS](temporal/FUTURE_ENHANCEMENTS.md) | [Header Docs](temporal/include_README.md)
- **[timeseries](timeseries/)** - Time series data
  - [README](timeseries/README.md) | [Header Docs](timeseries/include_README.md)

### Security (3 modules)

- **[security](security/)** - Security framework
  - [README](security/README.md) | [FUTURE_ENHANCEMENTS](security/FUTURE_ENHANCEMENTS.md) | [Header Docs](security/include_README.md)
- **[auth](auth/)** - Authentication and authorization
  - [README](auth/README.md) | [FUTURE_ENHANCEMENTS](auth/FUTURE_ENHANCEMENTS.md) | [Header Docs](auth/include_README.md)
- **[governance](governance/)** - Data governance
  - [README](governance/README.md) | [Header Docs](governance/include_README.md)

### Server & Network (4 modules)

- **[server](server/)** - Server implementation
  - [README](server/README.md) | [FUTURE_ENHANCEMENTS](server/FUTURE_ENHANCEMENTS.md) | [Header Docs](server/include_README.md)
- **[network](network/)** - Network layer
  - [README](network/README.md) | [FUTURE_ENHANCEMENTS](network/FUTURE_ENHANCEMENTS.md) | [Header Docs](network/include_README.md)
- **[api](api/)** - API layer
  - [README](api/README.md) | [Header Docs](api/include_README.md)
- **[sharding](sharding/)** - Data sharding
  - [README](sharding/README.md) | [Header Docs](sharding/include_README.md)

### Intelligence (4 modules)

- **[rag](rag/)** - Retrieval-Augmented Generation
  - [README](rag/README.md) | [FUTURE_ENHANCEMENTS](rag/FUTURE_ENHANCEMENTS.md) | [Header Docs](rag/include_README.md)
- **[llm](llm/)** - Large Language Model integration
  - [README](llm/README.md) | [Header Docs](llm/include_README.md)
- **[analytics](analytics/)** - Analytics engine
  - [README](analytics/README.md) | [FUTURE_ENHANCEMENTS](analytics/FUTURE_ENHANCEMENTS.md) | [Header Docs](analytics/include_README.md)
- **[voice](voice/)** - Voice processing
  - [README](voice/README.md) | [FUTURE_ENHANCEMENTS](voice/FUTURE_ENHANCEMENTS.md) | [Header Docs](voice/include_README.md)

### Operations (4 modules)

- **[performance](performance/)** - Performance monitoring
  - [README](performance/README.md) | [FUTURE_ENHANCEMENTS](performance/FUTURE_ENHANCEMENTS.md) | [Header Docs](performance/include_README.md)
- **[observability](observability/)** - Observability tools
  - [README](observability/README.md) | [FUTURE_ENHANCEMENTS](observability/FUTURE_ENHANCEMENTS.md) | [Header Docs](observability/include_README.md)
- **[updates](updates/)** - Update management
  - [README](updates/README.md) | [FUTURE_ENHANCEMENTS](updates/FUTURE_ENHANCEMENTS.md) | [Header Docs](updates/include_README.md)
- **[scheduler](scheduler/)** - Task scheduling
  - [README](scheduler/README.md) | [FUTURE_ENHANCEMENTS](scheduler/FUTURE_ENHANCEMENTS.md) | [Header Docs](scheduler/include_README.md)

### Data Integration (4 modules)

- **[importers](importers/)** - Data import functionality
  - [README](importers/README.md) | [Header Docs](importers/include_README.md)
- **[exporters](exporters/)** - Data export functionality
  - [README](exporters/README.md) | [Header Docs](exporters/include_README.md)
- **[cdc](cdc/)** - Change Data Capture
  - [README](cdc/README.md) | [Header Docs](cdc/include_README.md)
- **[plugins](plugins/)** - Plugin system
  - [README](plugins/README.md) | [Header Docs](plugins/include_README.md)

### Distributed (2 modules)

- **[replication](replication/)** - Data replication
  - [README](replication/README.md) | [FUTURE_ENHANCEMENTS](replication/FUTURE_ENHANCEMENTS.md) | [Header Docs](replication/include_README.md)
- **[sharding](sharding/)** - Distributed sharding (see Server & Network)

### Specialized (4 modules)

- **[graph](graph/)** - Graph database functionality
  - [README](graph/README.md) | [FUTURE_ENHANCEMENTS](graph/FUTURE_ENHANCEMENTS.md) | [Header Docs](graph/include_README.md)
- **[chimera](chimera/)** - Chimera multi-model engine
  - [README](chimera/README.md) | [FUTURE_ENHANCEMENTS](chimera/FUTURE_ENHANCEMENTS.md) | [Header Docs](chimera/include_README.md)
- **[geo](geo/)** - Geospatial data handling
  - [README](geo/README.md) | [Header Docs](geo/include_README.md)
- **[acceleration](acceleration/)** - Hardware acceleration
  - [README](acceleration/README.md) | [Header Docs](acceleration/include_README.md)

### Utility (4 modules)

- **[metadata](metadata/)** - Metadata management
  - [README](metadata/README.md) | [FUTURE_ENHANCEMENTS](metadata/FUTURE_ENHANCEMENTS.md) | [Header Docs](metadata/include_README.md)
- **[gpu](gpu/)** - GPU acceleration
  - [README](gpu/README.md) | [FUTURE_ENHANCEMENTS](gpu/FUTURE_ENHANCEMENTS.md)
- **[cache](cache/)** - Caching system
  - [README](cache/README.md) | [Header Docs](cache/include_README.md)
- **[content](content/)** - Content processing
  - [README](content/README.md) | [Header Docs](content/include_README.md)

## Documentation Structure

Each module directory contains:

- **README.md** - Overview of the module from `src/<module>/README.md`
- **FUTURE_ENHANCEMENTS.md** - Planned enhancements from `src/<module>/FUTURE_ENHANCEMENTS.md` (where applicable)
- **include_README.md** - Header file documentation from `include/<module>/README.md`

## Contributing

When updating module documentation:
1. Update the source files in `src/<module>/` or `include/<module>/`
2. Copy changes to the corresponding files in `docs/de/src/<module>/`
3. Keep the documentation synchronized

## Related Documentation

- [Main Documentation Index](../INDEX.md)
- [Architecture Documentation](../architecture/)
- [API Documentation](../api/)
- [Development Guides](../development/)
