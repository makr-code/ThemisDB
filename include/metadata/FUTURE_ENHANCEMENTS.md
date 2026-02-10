# Metadata Module API - Future Enhancements

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

## See Also

- [Current API](README.md)
- [Implementation FUTURE_ENHANCEMENTS](../../src/metadata/FUTURE_ENHANCEMENTS.md)

---

*Last Updated: February 2026*  
*Target API Version: v1.6.0*
