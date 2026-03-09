# OLAP Analytics Usage Guide

**Version:** v1.3.0 Phase 2  
**Status:** Production-Ready  
**Last Updated:** December 22, 2025

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

See full documentation at https://github.com/makr-code/ThemisDB

---

**Last Updated:** December 22, 2025  
**Version:** v1.3.0 Phase 2  
**Status:** Production-Ready
