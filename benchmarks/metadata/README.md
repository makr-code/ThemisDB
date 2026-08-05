# benchmarks/metadata

Benchmark folder for `src/metadata` — measures hot-path performance for the
metadata module and provides release-gate assertions.

## Gate Table

| ID          | Benchmark                                     | Gate          | File                                         |
|-------------|-----------------------------------------------|---------------|----------------------------------------------|
| GATE-MET-01 | MetaError cast throughput                     | ≥ 50M ops/s   | bench_metadata_release_gates.cpp             |
| GATE-MET-02 | MetaError switch dispatch                     | ≥ 50M ops/s   | bench_metadata_release_gates.cpp             |
| GATE-MET-03 | MetaError range check                         | ≥ 50M ops/s   | bench_metadata_release_gates.cpp             |
| GATE-MET-04 | MetaError batch cast                          | ≥ 1M ops/s    | bench_metadata_release_gates.cpp             |
| GATE-MCL-01 | ConsistencyIssue::toJSON() throughput         | ≥ 10M ops/s   | bench_metadata_consistency_lineage_gates.cpp |
| GATE-MCL-02 | ColumnRef::toString() throughput              | ≥ 50M ops/s   | bench_metadata_consistency_lineage_gates.cpp |
| GATE-MCL-03 | ColumnRef toJSON / fromJSON round-trip        | ≥ 5M ops/s    | bench_metadata_consistency_lineage_gates.cpp |
| GATE-MCL-04 | TransformationType string conversion          | ≥ 20M ops/s   | bench_metadata_consistency_lineage_gates.cpp |

## Cache benchmarks

`bench_metadata_cache.cpp` covers the cache-centric SchemaManager hot paths
(cold scan, warm hit, concurrent reads, adaptive TTL).

