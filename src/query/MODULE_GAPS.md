# query Module — Implementation Gap Analysis

**Status:** Active  
**Last Updated:** 2026-05-26

---

## 📊 Gap Summary

### oop_design — Override destructors

**Status (v1.22.0-pre — W1-L03c):** 399 missing `override` destructors added to all concrete
`IFunction` subclasses across 28 query header files:

| File | Classes fixed |
|------|--------------|
| `include/query/functions/ai_ml_functions.h` | 6 |
| `include/query/functions/array_functions.h` | 18 |
| `include/query/functions/collection_functions.h` | 39 |
| `include/query/functions/crs_functions.h` | 9 |
| `include/query/functions/date_functions.h` | 54 |
| `include/query/functions/document_functions.h` | 21 |
| `include/query/functions/ethics_functions.h` | 12 |
| `include/query/functions/file_functions.h` | 20 |
| `include/query/functions/geo_functions.h` | 25 |
| `include/query/functions/graph_extensions.h` | 12 |
| `include/query/functions/graph_functions.h` | 16 |
| `include/query/functions/graphql_functions.h` | 1 |
| `include/query/functions/json_path_functions.h` | 8 |
| `include/query/functions/lora_functions.h` | 11 |
| `include/query/functions/math_functions.h` | 25 |
| `include/query/functions/process_functions.h` | 11 |
| `include/query/functions/process_mining_functions.h` | 14 |
| `include/query/functions/relational_functions.h` | 21 |
| `include/query/functions/retention_functions.h` | 8 |
| `include/query/functions/security_functions.h` | 15 |
| `include/query/functions/string_functions.h` | 17 |
| `include/query/functions/vector_functions.h` | 23 |
| `include/query/approximate_aggregator.h` | 3 |
| `include/query/continuous_query_engine_impl.h` | 1 |
| `include/query/query_engine.h` | 1 |
| `include/query/query_profiler.h` | 2 |
| `include/query/query_rewrite_rule.h` | 5 |
| `include/query/result_stream.h` | 1 |

All concrete `IFunction`-derived classes (and other derived classes in the query module)
now have explicit `~ClassName() override = default;` virtual destructors.

---

## 🚀 How to Use This Documentation

Run the gap audit to re-populate this document with fresh analysis:

```bash
python tools/gap_audit_pipeline_v2.py
```

---

**Format:** THEMIS_MODULE_GAPS_v1  
**Generator:** ThemisDB Gap Audit Pipeline v2  
**Manually Updated:** Yes
