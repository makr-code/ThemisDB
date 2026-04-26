# OLAP Analytics Usage Guide

**Version:** v1.7.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-04-06

---

## Overview

ThemisDB's OLAP (Online Analytical Processing) Analytics engine provides powerful analytical capabilities for complex data analysis tasks. The engine supports GROUP BY operations, CUBE/ROLLUP queries, window functions, and advanced aggregations with columnar processing for optimal performance.

## Key Features

### 1. **Flexible Grouping Operations**

- Simple GROUP BY with single or multiple dimensions
- CUBE for generating all possible grouping combinations
- ROLLUP for hierarchical aggregations
- GROUPING SETS for custom grouping combinations

### 2. **Window Functions**

- ROW_NUMBER: Sequential row numbering within partitions
- RANK: Ranking with gaps for ties
- DENSE_RANK: Ranking without gaps
- LAG: Access previous row values
- LEAD: Access next row values

### 3. **Advanced Aggregations**

- Standard: COUNT, SUM, AVG, MIN, MAX
- Statistical: STDDEV, VARIANCE, MEDIAN, PERCENTILE
- Specialized: COUNT_DISTINCT, FIRST, LAST

### 4. **Performance Optimization**

- Columnar storage for efficient scanning
- Query plan optimization
- Apache Arrow integration (optional)
- Parallel processing support

---

## Related Documentation

- [Analytics Docs Hub](./README.md)
- [Forecasting Guide](./forecasting_guide.md)
- [Process Mining Guide](./process_mining_guide.md)
- [CEP Guide](./cep_guide.md)
- [API Reference](../../../include/analytics/README.md)
- [Implementation Overview](../../../src/analytics/README.md)
- [Roadmap](../../../src/analytics/ROADMAP.md)

---

**Last Updated:** 2026-04-06
**Version:** v1.7.0
**Status:** 🟢 Production-Ready
