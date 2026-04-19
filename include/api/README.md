> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# API Module — Public Headers

**Module Path:** `include/api/`
**Implementation:** `../../src/api/`
**Status:** ✅ Production Ready

This directory contains the public C++ header files (`.h`) for the `api` module.
All headers are `#pragma once` guarded and contain no implementation code.

## Header Overview

| File | Purpose |
|------|---------|
| [`graphql.h`](graphql.h) | GraphQL `Value`, `Field`, `SelectionSet`, `Parser`, `QueryLimits` and `Executor` types |
| [`graphql_cache.h`](graphql_cache.h) | LRU query-plan cache (`GraphQLCache`) for parsed GraphQL documents |
| [`graphql_metrics.h`](graphql_metrics.h) | Prometheus-style metrics collector for GraphQL requests and errors |
| [`grpc_server.h`](grpc_server.h) | `GrpcApiServer` and `GrpcServerConfig` — gRPC server lifecycle (compiled only when `THEMIS_ENABLE_GRPC` is defined) |
| [`themisdb_grpc_service.h`](themisdb_grpc_service.h) | `ThemisDBService` gRPC handler — document CRUD and AQL execution over protobuf |
| [`ws_handler.h`](ws_handler.h) | `WsChangeHandler` — WebSocket upgrade validator and frame dispatcher for `/v2/changes` and `/v2/cdc/stream` |
| [`tracing_middleware.h`](tracing_middleware.h) | `TracingMiddleware` — `X-Correlation-ID` propagation and thread-local request context |
| [`rate_limiter.h`](rate_limiter.h) | Token-bucket rate limiter interface for per-client/per-tenant request throttling |
| [`audit_logger.h`](audit_logger.h) | Audit event logging interface for security-relevant API operations |
| [`persisted_queries.h`](persisted_queries.h) | Persisted GraphQL query store — hash-keyed lookup to avoid repeated parsing |
| [`geo_index_hooks.h`](geo_index_hooks.h) | GeoJSON validation hooks wired into the storage write/delete path |

## Build Conditionals

| Symbol | Guards |
|--------|--------|
| `THEMIS_ENABLE_GRPC` | `grpc_server.h`, `themisdb_grpc_service.h` |
| `THEMIS_ENABLE_WEBSOCKET` | `ws_handler.h` |

## Documentation

- Module overview: [`../../src/api/README.md`](../../src/api/README.md)
- Architecture guide: [`../../src/api/ARCHITECTURE.md`](../../src/api/ARCHITECTURE.md)
- Roadmap: [`../../src/api/ROADMAP.md`](../../src/api/ROADMAP.md)
- Future enhancements: [`../../src/api/FUTURE_ENHANCEMENTS.md`](../../src/api/FUTURE_ENHANCEMENTS.md)
- Planned header interfaces: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "api/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
