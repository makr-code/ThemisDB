# ThemisDB CHIMERA Module — Header Reference

**Version:** 0.0.47
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-04-16
**Module Path:** `include/chimera/`

---

## Installation

These headers are part of the ThemisDB build and are included automatically when you link against the `themis_chimera` CMake target.
No additional installation steps are required beyond the standard ThemisDB build.

---

## Module Purpose

The `include/chimera/` directory contains the public header files that define the CHIMERA adapter API for ThemisDB.
CHIMERA (**C**omprehensive **H**ybrid **I**nferencing & **M**ulti-model **E**valuation **R**esource **A**ssessment) is a benchmark suite for evaluating diverse database systems through a unified, vendor-neutral interface.

This directory provides:
- **Extended interface definitions** for streaming and prepared statements (shim header layered on top of the external `chimera` submodule).
- **The ThemisDB reference adapter** (`ThemisDBAdapter`), which implements all CHIMERA interfaces for ThemisDB in both in-process simulation mode and wired engine mode.

For implementation details see [`src/chimera/README.md`](../../src/chimera/README.md).

---

## Header Files

### 1. `database_adapter.hpp`

**Purpose:** Shim header that re-exports the upstream external CHIMERA adapter definitions and adds the extended `IStreamingAdapter` and `IPreparedStatementAdapter` interfaces.

These extensions were added after the external submodule was frozen at `v0.0.37` and are maintained here so that `ThemisDBAdapter` and all CHIMERA tests can use them without requiring a submodule update.

#### Extended Types

##### `StreamConfig`

Configuration hints for server-side cursor / streaming execution.

```cpp
struct StreamConfig {
    size_t   default_batch_size = 1000; // Rows fetched per network round-trip
    size_t   prefetch           = 2;    // Number of batches to prefetch
    uint32_t timeout_ms         = 30000;// Per-batch fetch timeout (ms)
};
```

##### `IResultStream`

Cursor interface for streaming large result sets row-by-row.

| Method | Description |
|--------|-------------|
| `has_more() const` | `true` while there are more rows to fetch |
| `next_batch(batch_size)` | Fetch the next batch of rows (up to `batch_size`) |
| `position() const` | Zero-based index of the next row to be returned |
| `total_size() const` | Total row count if known, `nullopt` otherwise |
| `close()` | Release server-side cursor resources |

##### `IStreamingAdapter`

Mixin that adds streaming query execution to any adapter.

| Method | Description |
|--------|-------------|
| `execute_query_stream(query, params)` | Execute a query and return a `IResultStream` cursor |
| `set_stream_config(config)` | Update the `StreamConfig` for subsequent stream queries |

##### `IPreparedStatement`

Handle for a server-side prepared (pre-parsed) statement.

| Method | Description |
|--------|-------------|
| `get_id() const` | Opaque server-side statement identifier (UUID) |
| `get_query() const` | Original query text passed to `prepare()` |
| `bind(name, value)` | Bind a named parameter (e.g. `@name` tokens) |
| `bind(position, value)` | Bind a positional parameter (1-based index) |
| `bind_all(params)` | Bind all named parameters at once |
| `execute()` | Execute with currently bound parameters; returns `RelationalTable` |
| `execute_async()` | Asynchronous variant of `execute()` |
| `reset()` | Clear all bound parameters for re-use with new values |
| `get_statistics() const` | Accumulated execution statistics for this statement |

##### `IPreparedStatementAdapter`

Mixin that adds prepared-statement support to any adapter.

| Method | Description |
|--------|-------------|
| `prepare(query)` | Parse and cache a query; returns a statement handle |
| `unprepare(statement_id)` | Release a cached prepared statement by its ID |
| `list_prepared()` | List all currently cached statement IDs |

**Thread Safety:** All extended interfaces are designed for single-thread use per instance unless the concrete implementation documents otherwise.

---

### 2. `themisdb_adapter.hpp`

**Purpose:** Declaration of the ThemisDB reference adapter — the production implementation of all CHIMERA interfaces for the ThemisDB engine.

#### Operating Modes

`ThemisDBAdapter` supports two distinct operating modes:

| Mode | Constructor | Description |
|------|-------------|-------------|
| **Simulation** | `ThemisDBAdapter()` | Default — all operations are served from lightweight in-memory collections. No live ThemisDB server required. Ideal for unit tests and CI. |
| **Engine-wired** | `ThemisDBAdapter(query_engine, vector_index, graph_index)` | Accepts optional ThemisDB engine pointers. Non-null pointers delegate operations to the real back-end; null pointers fall back to in-memory simulation for that subsystem. |

#### Implemented Interfaces

`ThemisDBAdapter` inherits from all four CHIMERA extension interfaces:

```
IDatabaseAdapter           — connection, relational, vector, graph, document, transactions
IAsyncDatabaseAdapter      — async variants of query, batch-insert, vector-search, cancel
IStreamingAdapter          — streaming cursor execution, stream config
IPreparedStatementAdapter  — prepare / unprepare / list_prepared
```

#### Key Classes

##### `ThemisDBResultStream`

In-memory simulation implementation of `IResultStream`. Stores a pre-fetched `RelationalTable` snapshot and serves rows via the cursor API. In production mode the back-end would replace the snapshot with a real server-side cursor.

```cpp
// Construct from a pre-fetched table snapshot
ThemisDBResultStream(RelationalTable table, StreamConfig config = {});
```

##### `ThemisDBPreparedStatement`

In-memory simulation implementation of `IPreparedStatement`. Stores the query text and named/positional parameter maps. On `execute()` parameters are substituted into the query text and delegated to the adapter's `execute_query()`. In production mode a real query-plan cache would replace textual substitution.

```cpp
// Construct a prepared statement for the given query
ThemisDBPreparedStatement(
    std::string       id,       // Unique server-side statement ID (UUID)
    std::string       query,    // Query text to prepare
    IDatabaseAdapter* adapter   // Non-owning; adapter outlives statement
);
```

##### `ThemisDBAdapter`

Full ThemisDB adapter. Implements connection management, all data-model operations (relational, vector, graph, document), transaction support with savepoints, streaming, async queries, and prepared statements.

**Selected API surface:**

```cpp
// Connection
Result<bool> connect(connection_string, options);
Result<bool> disconnect();
bool         is_connected() const;

// Relational
Result<RelationalTable>         execute_query(query, params);
Result<size_t>                  insert_row(table_name, row);
Result<size_t>                  batch_insert(table_name, rows);

// Vector
Result<std::string>                               insert_vector(collection, vector);
Result<size_t>                                    batch_insert_vectors(collection, vectors);
Result<std::vector<std::pair<Vector,double>>>      search_vectors(collection, query_vector, k, filters);
Result<bool>                                      create_index(collection, dimensions, index_params);

// Graph
Result<std::string>             insert_node(node);
Result<std::string>             insert_edge(edge);
Result<GraphPath>               shortest_path(source_id, target_id, max_depth);
Result<std::vector<GraphNode>>  traverse(start_id, max_depth, edge_labels);
Result<std::vector<GraphPath>>  execute_graph_query(query, params);

// Document
Result<std::string>              insert_document(collection, doc);
Result<size_t>                   batch_insert_documents(collection, docs);
Result<std::vector<Document>>    find_documents(collection, filter, limit);
Result<size_t>                   update_documents(collection, filter, updates);

// Transactions (with savepoints)
Result<std::string>  begin_transaction(options);
Result<bool>         commit_transaction(transaction_id);
Result<bool>         rollback_transaction(transaction_id);
Result<std::string>  create_savepoint(transaction_id, savepoint_name);
Result<bool>         rollback_to_savepoint(transaction_id, savepoint_name);
Result<bool>         release_savepoint(transaction_id, savepoint_name);

// System
Result<SystemInfo>    get_system_info() const;
Result<SystemMetrics> get_metrics() const;
bool                  has_capability(Capability cap) const;
std::vector<Capability> get_capabilities() const;

// Async
std::future<Result<RelationalTable>> execute_query_async(query, params, opts);
std::future<Result<size_t>>          batch_insert_async(table_name, rows, progress_cb, opts);
std::future<Result<std::vector<std::pair<Vector,double>>>> search_vectors_async(...);
Result<bool> cancel_async(operation_id);

// Streaming
Result<std::unique_ptr<IResultStream>> execute_query_stream(query, params);
Result<bool>                           set_stream_config(config);

// Prepared statements
Result<std::unique_ptr<IPreparedStatement>> prepare(query);
Result<bool>                                unprepare(statement_id);
Result<std::vector<std::string>>            list_prepared();
```

**Thread Safety:** All public methods are internally protected by `std::mutex`. Concurrent calls from multiple threads are safe. Keep critical sections short; avoid holding the adapter lock during long-running callbacks.

#### Usage Examples

**Simulation mode (unit tests / CI):**

```cpp
#include "chimera/themisdb_adapter.hpp"

chimera::ThemisDBAdapter adapter;  // simulation mode — no live server
adapter.connect("themisdb://localhost/test");

// Relational insert + query
adapter.insert_row("users", {{"id", "u1"}, {"name", "Alice"}});
auto result = adapter.execute_query("SELECT * FROM users");

// Vector similarity search
adapter.create_index("embeddings", 128);
adapter.insert_vector("embeddings", {/* 128-d float vector */});
auto hits = adapter.search_vectors("embeddings", query_vec, /*k=*/5);

// Streaming result set
auto stream = adapter.execute_query_stream("SELECT * FROM large_table");
while (stream->has_more()) {
    auto batch = stream->next_batch(256);
    // process batch ...
}

// Prepared statement
auto stmt = adapter.prepare("SELECT * FROM orders WHERE id = @id");
stmt->bind("id", chimera::Scalar{"o123"});
auto rows = stmt->execute();
stmt->reset();
```

**Engine-wired mode (production):**

```cpp
#include "chimera/themisdb_adapter.hpp"

themis::QueryEngine        engine;
themis::VectorIndexManager vim;
themis::GraphIndexManager  gim;

chimera::ThemisDBAdapter adapter(&engine, &vim, &gim);
adapter.connect("themisdb://prod-host:7070/mydb");
// All operations are now delegated to the real ThemisDB back-end.
```

---

## Known Limitations

| Limitation | Status |
|------------|--------|
| `Capability::CONNECTION_POOLING` declared as available but no dedicated pooling API is implemented in the adapter | Tracked in [`src/chimera/ROADMAP.md`](../../src/chimera/ROADMAP.md) — Target Q3 2026 |
| Engine-backed paths return `NOT_IMPLEMENTED` if `THEMISDB_ENGINE_AVAILABLE` build flag is off | By design — simulation mode is the safe default |
| `execute_query_stream` in production mode still uses the in-memory snapshot; real server-side cursor delegated to engine in a future release | See [`src/chimera/FUTURE_ENHANCEMENTS.md`](../../src/chimera/FUTURE_ENHANCEMENTS.md) |

---

## Related Documentation

| Resource | Path |
|----------|------|
| Source-level README | [`src/chimera/README.md`](../../src/chimera/README.md) |
| Architecture | [`src/chimera/ARCHITECTURE.md`](../../src/chimera/ARCHITECTURE.md) |
| Roadmap | [`src/chimera/ROADMAP.md`](../../src/chimera/ROADMAP.md) |
| Future Enhancements | [`src/chimera/FUTURE_ENHANCEMENTS.md`](../../src/chimera/FUTURE_ENHANCEMENTS.md) |
| Changelog | [`src/chimera/CHANGELOG.md`](../../src/chimera/CHANGELOG.md) |
| Secondary docs (DE) | [`docs/de/chimera/README.md`](../../docs/de/chimera/README.md) |
| Secondary docs (EN) | [`docs/en/chimera/README.md`](../../docs/en/chimera/README.md) |
| Streaming tests | [`tests/chimera/test_chimera_streaming.cpp`](../../tests/chimera/test_chimera_streaming.cpp) |
| Prepared-statement tests | [`tests/chimera/test_chimera_prepared_statements.cpp`](../../tests/chimera/test_chimera_prepared_statements.cpp) |
| External chimera headers | [`external/chimera/include/chimera/`](../../external/chimera/include/chimera/) |
