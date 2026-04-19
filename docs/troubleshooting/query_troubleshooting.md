# Query Troubleshooting Guide

The `query` module implements ThemisDB's AQL query engine, including the parser, adaptive cost-based optimizer, CTE materialization, window functions, federated queries, and query plan visualization.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `AQL parse error: unexpected token` | Syntax error in AQL query | Review AQL syntax; enable `query.debug.print_tokens: true` |
| Query runs full collection scan unexpectedly | Missing index or stale statistics | Run `ANALYZE collection_name`; check index existence |
| `QueryEngine: timeout after 30000ms` | Query too complex or missing index | Add index; increase `query.timeout_ms` for heavy analytics |
| CTE query results differ from subquery equivalent | CTE cache serving stale results | Set `query.cte.cache_ttl_ms: 0` to disable; `INVALIDATE CTE` |
| `query_federation: remote shard timeout` | Network issue or shard overloaded | Check shard health; tune `query.federation.shard_timeout_ms` |
| Memory usage spikes during large joins | Hash join spills to disk | Enable `query.join.allow_disk_spill: true` |
| `optimizer: plan cost is infinite` | Missing table statistics | Run `ANALYZE` on all involved collections |
| Window function returns wrong partition boundaries | Missing `ORDER BY` in window spec | Always specify `ORDER BY` in `OVER()` clause |
| Query plan shows `SeqScan` instead of `IndexScan` | Optimizer underestimates selectivity | Update statistics; increase `query.optimizer.stats_weight` |
| Federated query partially fails silently | Remote shard returns partial results | Set `query.federation.fail_on_partial_results: true` |

## Common Issues

### Issue 1: AQL Parser Rejects Valid Query

**Description:** A syntactically correct AQL query is rejected by the parser.

**Symptoms:**
- Error: `AqlParser: syntax error at line 3, col 18: unexpected FOR`
- Query works in AQL simulator but not in production

**Cause:** Nested `FOR` loops at depth > `query.parser.max_for_depth` (default: 10), or reserved keyword used as identifier.

**Solution:**
```bash
# Enable verbose parse output for debugging
themisdb-admin query parse --query "FOR doc IN users RETURN doc" --trace
```
```yaml
query:
  parser:
    max_for_depth: 20          # increase for deeply nested queries
    strict_reserved_words: false   # allow some reserved words as identifiers
```

---

### Issue 2: Full Collection Scan Instead of Index Scan

**Description:** The optimizer chooses a sequential scan even though a suitable index exists.

**Symptoms:**
- `EXPLAIN FOR u IN users FILTER u.email == 'x@y.com' RETURN u` shows `CollectionScan`
- Query is slow (>1s) on large collections

**Cause:** Index statistics are stale; selectivity estimate is too low for the optimizer to prefer the index.

**Solution:**
```sql
-- Update statistics
ANALYZE users;

-- Force index hint if needed (emergency)
FOR u IN users
  OPTIONS { indexHint: "idx_email", forceIndexHint: true }
  FILTER u.email == @email
  RETURN u
```
```bash
# Check index status
themisdb-admin index list --collection users

# Check optimizer decision
themisdb-admin query explain \
  --query "FOR u IN users FILTER u.email == 'x' RETURN u"
```

---

### Issue 3: Query Timeout on Complex Aggregation

**Description:** Analytics queries time out under the default 30-second threshold.

**Symptoms:**
- Error: `QueryEngine: query aborted: timeout after 30000ms`
- Prometheus metric `themisdb_query_duration_seconds` shows P99 > 30s

**Cause:** Default timeout is too short for OLAP-style queries on large datasets.

**Solution:**
```yaml
query:
  timeout_ms: 300000           # 5 minutes for analytics workloads
  adaptive_optimizer:
    max_optimization_time_ms: 2000
```
```sql
-- Per-query timeout override
FOR order IN orders
  COLLECT month = DATE_MONTH(order.created_at) WITH COUNT INTO total
  OPTIONS { maxRuntime: 120 }
  RETURN { month, total }
```

---

### Issue 4: CTE Cache Returns Stale Results

**Description:** Common Table Expressions return outdated data after the underlying collection is modified.

**Symptoms:**
- CTE query returns rows that were deleted
- Log: `CteCache: serving cached result for cte=monthly_totals (age=45s)`

**Cause:** CTE cache TTL too long; writes are not invalidating the cache.

**Solution:**
```yaml
query:
  cte:
    cache_enabled: true
    cache_ttl_ms: 5000       # reduce from default 60000
    invalidate_on_write: true
```
```sql
-- Manually invalidate a named CTE
INVALIDATE CTE monthly_totals;
```

---

### Issue 5: Window Function Returns Wrong Results

**Description:** `WINDOW` functions produce incorrect aggregates or unexpected `null` boundaries.

**Symptoms:**
- `ROW_NUMBER()` resets unexpectedly
- Partition subtotals do not sum to collection total

**Cause:** Missing `ORDER BY` inside `OVER()` causes undefined ordering; `PARTITION BY` key has type mismatch.

**Solution:**
```sql
-- Correct window function syntax
FOR sale IN sales
  LET row_num = NTH_RANK(
    WINDOW { partitionBy: ["region"], orderBy: [{ field: "amount", order: "DESC" }] }
  )
  RETURN { sale, row_num }
```
```yaml
query:
  window_functions:
    strict_order_required: true     # reject window specs without ORDER BY
```

---

### Issue 6: Federated Query Partially Fails

**Description:** A query spanning multiple shards returns partial results without error.

**Symptoms:**
- Result set is smaller than expected
- Log: `QueryFederation: shard-03 timed out; partial results returned`

**Cause:** One shard is overloaded or unreachable; federation returns best-effort results by default.

**Solution:**
```yaml
query:
  federation:
    fail_on_partial_results: true    # fail fast instead of partial return
    shard_timeout_ms: 10000
    retry_on_timeout: true
    retry_count: 2
```

---

### Issue 7: Optimizer Plan Cost Is Infinite

**Description:** The cost-based optimizer cannot produce a finite plan estimate.

**Symptoms:**
- Log: `AdaptiveOptimizer: plan cost overflow for query_id=abc123`
- Query falls back to naive sequential execution

**Cause:** Missing or corrupt table statistics (zero rows reported by `ANALYZE`).

**Solution:**
```bash
# Re-run ANALYZE
themisdb-admin query analyze --collection orders --full

# Check statistics
themisdb-admin stats show --collection orders
```
```yaml
query:
  optimizer:
    stats_weight: 1.0
    fallback_to_heuristic: true     # use heuristic plan when stats missing
    cost_overflow_threshold: 1e15
```

---

### Issue 8: Memory Spike During Hash Join

**Description:** ThemisDB RSS spikes to several GB during joins on large collections.

**Symptoms:**
- Process OOM-killed during join
- Log: `QueryEngine: hash join build phase exceeded memory_limit_mb`

**Cause:** In-memory hash join exceeds `query.join.memory_limit_mb`; disk spill not enabled.

**Solution:**
```yaml
query:
  join:
    memory_limit_mb: 2048
    allow_disk_spill: true
    spill_dir: /var/lib/themisdb/tmp/join-spill
    prefer_merge_join_above_rows: 1000000   # use merge join for large sets
```

---

### Issue 9: Query Plan Visualizer Returns Empty Output

**Description:** `EXPLAIN` with `visualization: true` returns an empty SVG or JSON.

**Symptoms:**
- `themisdb-admin query explain --visualize` outputs `{}`
- Log: `QueryPlanVisualizer: plan has 0 nodes`

**Cause:** Query was already cached; visualizer only works on freshly planned queries.

**Solution:**
```bash
# Bypass cache for EXPLAIN
themisdb-admin query explain \
  --query "FOR u IN users RETURN u" \
  --no-cache \
  --visualize \
  --format dot > plan.dot

dot -Tsvg plan.dot -o plan.svg
```

---

### Issue 10: `LET` Evaluator Returns Wrong Type

**Description:** A `LET` expression that should return a number returns a string, causing downstream type errors.

**Symptoms:**
- Error: `LetEvaluator: type mismatch: expected number, got string`
- Only occurs on some documents (heterogeneous collection)

**Cause:** Some documents store the field as a string instead of a number.

**Solution:**
```sql
-- Defensive type coercion in AQL
FOR doc IN sales
  LET amount = TO_NUMBER(doc.amount)
  FILTER amount > 100
  RETURN { doc._key, amount }
```

## Diagnostic Commands

```bash
# Parse and explain a query
themisdb-admin query explain \
  --query "FOR u IN users FILTER u.active == true RETURN u"

# Show query statistics for running queries
themisdb-admin query running-queries

# Kill a long-running query
themisdb-admin query kill --id <query_id>

# Check optimizer statistics for a collection
themisdb-admin stats show --collection users

# Update statistics
themisdb-admin query analyze --collection users

# Show query plan cache hit rate
curl -s http://localhost:9100/metrics | grep themisdb_query_cache

# Tail query engine logs
journalctl -u themisdb -f | grep -E "query|optimizer|CTE|federation|timeout"
```

## Configuration Reference

```yaml
query:
  timeout_ms: 30000
  max_result_size_mb: 256
  parser:
    max_for_depth: 10
    max_query_string_length: 65536
  adaptive_optimizer:
    enabled: true
    max_optimization_time_ms: 1000
    stats_weight: 1.0
    fallback_to_heuristic: true
  cte:
    cache_enabled: true
    cache_ttl_ms: 10000
    invalidate_on_write: true
  federation:
    shard_timeout_ms: 5000
    fail_on_partial_results: false
    retry_count: 1
  join:
    memory_limit_mb: 1024
    allow_disk_spill: false
  window_functions:
    enabled: true
    strict_order_required: false
  debug:
    print_tokens: false
    print_ast: false
    print_plan: false
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `timeout_ms` | `30000` for OLAP | `300000` |
| `cte.cache_ttl_ms` | `300000` | `5000–30000` |
| `join.allow_disk_spill` | `false` | `true` for large joins |
| `federation.fail_on_partial_results` | `false` | `true` in production |

## Known Limitations

- Window functions do not yet support `GROUPS` framing (only `ROWS` and `RANGE`).
- Federated queries across more than 128 shards may exceed internal plan serialization limits.
- `ANALYZE` on collections >500 GB uses sampling (5%) rather than full scan; accuracy may vary.
- The query plan visualizer requires Graphviz `dot` installed on the ThemisDB host for SVG output.
- Adaptive optimizer statistics are reset on server restart unless `query.optimizer.persist_stats: true`.

## Related Documentation

- [Query Engine Implementation Guide](../architecture/QUERYENGINE_IMPLEMENTATION_GUIDE.md)
- [AQL Roadmap](../de/roadmap/aql_roadmap.md)
- [Distributed Transactions](../DISTRIBUTED_TRANSACTIONS.md)
- [Phase 3 Query Engine DI Architecture](../PHASE3_QUERYENGINE_DI_ARCHITECTURE.md)
