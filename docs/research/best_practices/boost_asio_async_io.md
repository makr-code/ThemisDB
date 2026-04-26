# Boost.Asio Proactor Pattern for Scalable Async I/O

**Metadaten:**
- Source: Boost.Asio Documentation (Christopher Kohlhoff, 2003+); POSA2 — "Pattern-Oriented Software Architecture Vol. 2: Patterns for Concurrent and Networked Objects" (Proactor pattern)
- URL: https://www.boost.org/doc/libs/release/libs/asio/ | https://www.dre.vanderbilt.edu/~schmidt/POSA/
- Tags: io, performance, networking
- ThemisDB-Versionen: v1.0.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

Blocking I/O models (one thread per connection) do not scale beyond a few thousand concurrent connections due to thread context-switch overhead and stack memory usage. The Proactor pattern (documented in POSA2 and implemented by Boost.Asio) inverts control: the application initiates asynchronous operations and provides completion handlers; the I/O framework calls the handlers on a shared thread pool when operations complete. This allows thousands of concurrent connections to be managed by a handful of OS threads, with no blocking and no per-connection stacks.

ThemisDB's HTTP server, WebSocket server, SSE broadcaster, and MQTT client all use Boost.Asio as their I/O substrate, adopting the proactor pattern with a single `io_context` per connection group. Long-lived connections use `keepalive_timer` and `reconnect_timer` (`steady_timer`) for timeout management without additional threads.

## 🎯 Core Principles

- **Single io_context per connection group**: Rather than one io_context per connection, ThemisDB uses one io_context per logical server component (HTTP, WebSocket, MQTT), shared by all connections in that group, to maximise I/O readiness batching.
- **No blocking calls in completion handlers**: Completion handlers must never block (no `std::this_thread::sleep_for`, no synchronous file I/O, no mutex waits longer than a few microseconds). Blocking work is dispatched to a separate `thread_pool`.
- **strand for per-connection serialisation**: `boost::asio::strand<io_context::executor_type>` ensures that completion handlers for a single connection are never executed concurrently, even on a multi-threaded io_context, eliminating per-connection mutexes.
- **Steady timer for all timeouts**: `boost::asio::steady_timer` replaces `sleep` + a separate timeout thread; timers are cancelled explicitly when the connection closes.
- **Shared_ptr for lifetime management**: Async operations capture `shared_ptr<Session>` (via `shared_from_this`) to extend the session's lifetime until all outstanding handlers have completed.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/server/mqtt_client_service.cpp` — `MqttClientService::AsioImpl` (PIMPL) holds `io_context&`, `tcp::socket socket_`, `steady_timer keepalive_timer_`, `steady_timer reconnect_timer_`; all MQTT I/O is asynchronous.
- `src/server/http_server.cpp` — `HttpServer` owns an `io_context` and a `thread_pool`; `async_accept` loop starts new `HttpSession` (shared_ptr) for each accepted connection.
- `src/server/websocket_session.cpp` — `WebSocketSession` uses `beast::websocket::stream<tcp::socket>` with `async_read`/`async_write`; keepalive ping sent via `steady_timer`.
- `src/server/sse_broadcaster.cpp` — SSE connections are managed as `shared_ptr<SseSession>` with an `async_write` queue drained by a `strand`-serialised handler chain.

### What Was Adopted?

- `boost::asio::io_context ioc;` created at server startup; `ioc.run()` called from a configurable number of worker threads (default: `std::thread::hardware_concurrency()`).
- `boost::asio::make_strand(ioc)` used as the executor for all per-session operations.
- `async_read` / `async_write` / `async_accept` chains: each completion handler initiates the next async operation, forming a chain that runs until the session closes.
- `steady_timer::async_wait` used for keepalive (send MQTT PINGREQ if no I/O within `keepalive_interval_`), connection timeout (close idle connections), and reconnect delay.
- `io_context::work_guard` held for the lifetime of the server to prevent `ioc.run()` from returning when the accept queue is temporarily empty.
- Graceful shutdown: `acceptor.close()` + `ioc.stop()` + joining all worker threads; in-flight handlers complete before thread exit.

### Deviations & Rationale

- **Beast (Boost.Beast) for HTTP/WebSocket**: Boost.Beast wraps Boost.Asio with HTTP/1.1 and WebSocket protocol layers. This is the canonical Boost-stack approach and not a deviation from Asio principles; Beast uses the same async model.
- **Thread pool size is configurable, not auto**: The default `hardware_concurrency()` worker count is overridable via `THEMISDB_IO_THREADS` environment variable, allowing deployment on I/O-bound or CPU-bound hardware to tune accordingly.
- **Blocking DB calls dispatched via post()**: Storage operations (RocksDB reads/writes) are dispatched with `boost::asio::post(thread_pool_, handler)` rather than called directly in I/O completion handlers. This keeps I/O threads available for network operations.
- **No coroutines (C++20) in v1.x**: Boost.Asio supports C++20 coroutines (`co_await`), which would simplify the async chains. The codebase targets C++17; coroutine migration is planned for v3.0.

## ⚠️ Trade-offs & Limitations

- **Completion handler chains are hard to follow**: Deeply nested async chains (read → process → write → next-read) are harder to debug than sequential blocking code. Strand serialisation eliminates data races but adds a layer of indirection.
- **Exception handling in handlers**: Exceptions thrown from a completion handler terminate `ioc.run()`. All handlers must catch exceptions internally and convert them to error codes; uncaught exceptions are a reliability hazard.
- **Thundering herd on io_context**: With many worker threads sharing one `io_context`, a burst of ready I/O events can wake all threads simultaneously, causing contention on the `io_context` internal readiness queue. `thread::hardware_concurrency()` workers is empirically a good default.
- **Beast HTTP parser limits**: `beast::http::parser` has a configurable body size limit (`body_limit()`). The default 8 MiB must be raised for endpoints accepting large binary payloads (bulk insert, bundle import).

## 🔬 Validation

- [x] Code reviewed against Boost.Asio documentation and POSA2 Proactor pattern
- [x] Load test (`benchmarks/server/http_load_bench.cpp`) verifies >10,000 concurrent HTTP connections with 4 io_context worker threads
- [x] Valgrind/TSan passes with no data race reports on session-level state
- [x] Graceful shutdown test verifies all in-flight requests complete before process exit
- [x] Module README linked (`src/server/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [PIMPL ABI Stability](pimpl_abi_stability.md)
- [Shared Mutex Read-Write Locks](shared_mutex_read_write_locks.md)

---
**Last Updated:** 2026-04-06
