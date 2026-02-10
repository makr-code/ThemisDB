# Analytics Module - ThemisDB

## Overview

The Analytics module provides advanced data analytics capabilities for ThemisDB, including OLAP (Online Analytical Processing) queries and data export functionality.

## Components

### 1. OLAP Engine (`olap.h`, `olap.cpp`)

The OLAP engine provides:
- Multi-dimensional queries with GROUP BY, CUBE, ROLLUP
- Aggregation functions (SUM, AVG, COUNT, MIN, MAX, etc.)
- Window functions for time-series analysis
- Columnar storage for efficient aggregations
- Materialized views for pre-computed results

**Usage Example:**
```cpp
#include "analytics/olap.h"

using namespace themis::analytics;

OLAPQuery query;
query.collection = "sales";
query.dimensions.push_back({"region", "", true});
query.measures.push_back({"total", "amount", Measure::Function::Sum});

OLAPEngine engine;
auto result = engine.execute(query);
```

See [OLAP Guide](../../docs/de/analytics/olap_guide.md) for detailed documentation.

### 2. Arrow Export (`arrow_export.h`, `analytics_export.h`)

**Status:** ⚠️ Stub Implementation (GAP-003)  
**Apache Arrow:** Optional (not required, can be enabled via `THEMIS_ENABLE_ARROW` flag)

The Arrow Export module provides interfaces for exporting analytics data to various formats, with a design that supports **optional** Apache Arrow integration.

**Current Features:**
- `ArrowRecordBatch`: Placeholder class representing columnar data
- `IAnalyticsExporter`: Interface for data export implementations
- `StubAnalyticsExporter`: Reference implementation supporting JSON and CSV export
- Export to file, string, or streaming with callbacks

**Supported Export Formats (WITHOUT Arrow dependency):**
- ✅ JSON (fully implemented, always available)
- ✅ CSV (fully implemented, always available)
- ⚠️ Arrow IPC (placeholder, will fallback to JSON if Arrow not enabled)
- ⚠️ Arrow Parquet (placeholder, will fallback to CSV if Arrow not enabled)
- ⚠️ Arrow Feather (placeholder, will fallback to JSON if Arrow not enabled)

**Optional Apache Arrow Integration (Phase 2):**
- When `THEMIS_ENABLE_ARROW=ON` is set, native Arrow formats will be used
- When `THEMIS_ENABLE_ARROW=OFF` (default), stub implementation provides JSON/CSV export
- The module remains fully functional without Apache Arrow

**Usage Example:**
```cpp
#include "analytics/arrow_export.h"
#include "analytics/analytics_export.h"

using namespace themis::analytics;

// Create a record batch
ArrowRecordBatch batch;
batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, true});
batch.addColumn({"value", ArrowRecordBatch::DataType::DOUBLE, true});

// Add data
batch.appendRow({int64_t(1), std::string("Alice"), 95.5});
batch.appendRow({int64_t(2), std::string("Bob"), 87.3});

// Export
auto exporter = ExporterFactory::createDefaultExporter();

ExportOptions options;
options.format = ExportFormat::JSON;

auto result = exporter->exportToFile(batch, "output.json", options);
if (result.status == ExportStatus::SUCCESS) {
    std::cout << "Exported " << result.rows_exported << " rows\n";
}
```

**Future Development (Optional):**
- Real Apache Arrow integration via `THEMIS_ENABLE_ARROW` flag (optional dependency)
- Zero-copy data transfer (only with Arrow enabled)
- Parquet file format support (only with Arrow enabled)
- Inter-process communication with Arrow IPC (only with Arrow enabled)
- Integration with external analytics tools like Pandas, DuckDB (only with Arrow enabled)
- **Important:** All core functionality remains available without Apache Arrow

See [GAP-003 Documentation](../../docs/de/analytics/GAP_003_ARROW_ANALYTICS.md) for the implementation plan with optional Arrow integration.

### 3. Process Mining (`process_mining.h`, `process_mining.cpp`)

Analyze process flows and patterns in event data:
- Event log processing
- Process discovery
- Conformance checking
- Performance analysis

See [Process Mining Guide](../../docs/de/analytics/process_mining_guide.md) for details.

### 4. Text Analytics (`nlp_text_analyzer.h`, `nlp_text_analyzer.cpp`)

NLP-based text analysis:
- Text classification
- Sentiment analysis
- Entity extraction
- Text summarization

See [NLP Text Analyzer](../../docs/de/analytics/NLP_TEXT_ANALYZER.md) for details.

## Building

The analytics module is built as part of ThemisDB. No special configuration is required.

```bash
cmake -B build -S .
cmake --build build
```

## Testing

Run analytics tests:
```bash
cd build
ctest -R analytics --verbose
```

Or run specific tests:
```bash
./build/tests/test_olap
./build/tests/analytics/test_arrow_export
./build/tests/analytics/test_process_mining_llm
```

## Architecture

```
src/analytics/
├── olap.cpp                    # OLAP engine implementation
├── arrow_export.cpp            # Arrow RecordBatch placeholder
├── analytics_export.cpp        # Export interface implementation
├── process_mining.cpp          # Process mining engine
├── nlp_text_analyzer.cpp       # Text analytics
├── llm_process_analyzer.cpp    # LLM-based analysis
└── diff_engine.cpp             # Diff analysis

include/analytics/
├── olap.h                      # OLAP interfaces
├── arrow_export.h              # Arrow RecordBatch placeholder
├── analytics_export.h          # Export interfaces
├── process_mining.h            # Process mining interfaces
└── ...

tests/analytics/
├── test_arrow_export.cpp       # Arrow export tests
└── test_process_mining_llm.cpp # Process mining tests

docs/de/analytics/
├── olap_guide.md               # OLAP documentation
├── process_mining_guide.md     # Process mining guide
└── GAP_003_ARROW_ANALYTICS.md  # Arrow integration plan
```

## Performance Considerations

### OLAP Queries
- Use materialized views for frequently accessed aggregations
- Create indexes on dimension columns
- Use columnar storage for large analytical workloads
- Limit result sets with LIMIT clauses

### Data Export
- Use streaming export for large datasets
- Enable compression for network transfers
- Batch exports to reduce overhead
- Consider Parquet format for archival storage (when implemented)

## Integration with Other Modules

### With Query Engine
- OLAP queries can be triggered from AQL
- Analytics results can feed back into the query engine

### With Storage Engine
- Direct access to columnar data structures
- Efficient batch reads for analytics workloads

### With Observability
- Export metrics and traces for external analysis
- Integration with Prometheus and Grafana

## Roadmap

### Phase 1: Foundation (✅ Complete)
- [x] OLAP engine with aggregations
- [x] Basic export interfaces
- [x] JSON and CSV export

### Phase 2: Optional Arrow Integration (⚠️ In Progress - GAP-003)
- [x] Arrow RecordBatch placeholder
- [x] Export interface design
- [x] Stub implementation (always available)
- [ ] Optional Apache Arrow C++ integration via `THEMIS_ENABLE_ARROW` flag
- [ ] Arrow IPC format support (optional, requires Arrow flag)
- [ ] Parquet writer integration (optional, requires Arrow flag)
- **Note:** Core functionality remains available without Apache Arrow

### Phase 3: Advanced Features (📋 Planned)
- [ ] Zero-copy data transfer (optional, with Arrow)
- [ ] Streaming aggregations (available without Arrow)
- [ ] Incremental materialized views (available without Arrow)
- [ ] GPU-accelerated analytics (optional feature)
- [ ] Integration with external analytics tools (optional, enhanced with Arrow)

## Contributing

When contributing to the analytics module:

1. Add tests for new functionality
2. Update documentation
3. Follow the existing code structure
4. Consider performance implications
5. Add integration tests for cross-module features

## License

Part of ThemisDB. See LICENSE file in the root directory.

## See Also

- [OLAP Guide](../../docs/de/analytics/olap_guide.md)
- [Process Mining Guide](../../docs/de/analytics/process_mining_guide.md)
- [GAP-003: Arrow Analytics](../../docs/de/analytics/GAP_003_ARROW_ANALYTICS.md)
- [Features: OLAP Analytics](../../docs/de/features/features_olap_analytics.md)
