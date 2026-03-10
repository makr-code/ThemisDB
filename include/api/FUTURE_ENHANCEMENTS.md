# API Module - Future Header Enhancements

## Scope

- `IHttpHandler` interface extensions for versioned routing and middleware chaining
- GraphQL schema builder interface (`IGraphQLSchemaBuilder`) for runtime schema composition
- WebSocket upgrade API (`IWebSocketHandler`) for real-time streaming endpoints
- gRPC bridge interface (`IGRPCBridge`) mapping HTTP/2 RPC calls to internal handler contracts
- API version router interface (`IAPIVersionRouter`) enforcing backward-compatible `/v2/` routing
- Correlation ID propagation API for end-to-end request tracing across handler boundaries

## Design Constraints

- `[x]` All request handlers must implement `IHttpHandler`; no ad-hoc handler registration bypassing the interface. (**Implemented** — `IHttpHandler` pure-virtual interface defined in `include/api/http_handler.h`.)
- `[x]` GraphQL schema is generated exactly once at server initialization; post-init schema mutation is not permitted. (**Enforced** — `IGraphQLSchemaBuilder::build()` returns error on second call; `isBuilt()` guards all `add*()` methods.)
- `[x]` WebSocket upgrade is non-blocking; `IWebSocketHandler::upgrade()` returns immediately and delivers frames via callback. (**Enforced** — `IWebSocketHandler::upgrade()` returns a `WebSocketSession*` immediately; frames arrive via `IWebSocketFrameCallback::onFrame()`.)
- `[x]` API version selection must be backward-compatible; a `/v2/` handler must not remove endpoints present in `/v1/`. (**Documented** — `IAPIVersionRouter` contract in `include/api/api_version_router.h` mandates superset registration.)
- `[x]` Correlation IDs are generated at the interface boundary; downstream handlers receive IDs as opaque `CorrelationId` values. (**Implemented** — `CorrelationId` 16-byte value type in `include/api/correlation_id.h`; `ICorrelationIDProvider::extract()` returns opaque IDs.)
- `[x]` All public header types are header-only value types or pure-virtual interfaces; no implementation in headers. (**Enforced** — all six new interface headers contain only `= 0` declarations, inline value types, and inline trivial helpers.)

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IHttpHandler` | `APIRouter`, `MiddlewareChain`, `gRPCBridge` | Base interface for all HTTP request handlers; `handle()` returns `HttpResponse` via `Result<T>` |
| `IGraphQLSchemaBuilder` | `GraphQLEndpoint`, `SchemaRegistry` | Builds and validates GraphQL schema at init time; immutable after `build()` is called |
| `IWebSocketHandler` | `RealtimeStreamEndpoint`, `SubscriptionManager` | Manages WebSocket lifecycle; frame callbacks are `noexcept` |
| `IAPIVersionRouter` | `APIGateway`, `VersionNegotiator` | Routes requests to versioned handler sets; enforces backward-compatibility contract |
| `IGRPCBridge` | `gRPCServer`, `APIGateway` | Maps gRPC service methods to `IHttpHandler` implementations |
| `ICorrelationIDProvider` | `MiddlewareChain`, `ObservabilityLayer` | Generates and propagates `CorrelationId` across handler invocations |

## Planned Features

### GraphQL Schema Extension API

- `[x]` Define `IGraphQLSchemaBuilder` with `addType()`, `addQuery()`, `addMutation()`, and `build()` methods. (**Implemented** in `include/api/graphql_schema_builder.h`.)
- `[x]` Expose `GraphQLTypeDescriptor` as a plain-data struct usable without a schema library dependency. (**Implemented** — `GraphQLTypeDescriptor` + `GraphQLFieldDescriptor` plain-data structs in `include/api/graphql_schema_builder.h`.)
- `[x]` Add `SchemaValidationResult` value type returned by `build()`; carries structured error list on failure. (**Implemented** — `SchemaValidationResult` + `SchemaValidationError` in `include/api/graphql_schema_builder.h`.)
- `[x]` Document that `build()` is callable exactly once; subsequent calls return `Result::error(SchemaAlreadyBuilt)`. (**Documented** — contract in `IGraphQLSchemaBuilder::build()` docstring in `include/api/graphql_schema_builder.h`.)

### WebSocket Endpoint Interface

- `[x]` Define `IWebSocketHandler` with `upgrade(HttpRequest&) -> Result<WebSocketSession>`. (**Implemented** — `IWebSocketHandler::upgrade()` returns `Result<WebSocketSession*>` in `include/api/websocket_handler.h`.)
- `[x]` Expose `WebSocketSession` as an opaque handle with `send(Frame)` and `close(CloseCode)` methods. (**Implemented** — `WebSocketSession` pure-virtual class with `send()` and `close()` in `include/api/websocket_handler.h`.)
- `[x]` Add `IWebSocketFrameCallback` pure-virtual interface for incoming frame dispatch; callbacks must be `noexcept`. (**Implemented** — `IWebSocketFrameCallback::onFrame()` and `onClose()` are `noexcept` in `include/api/websocket_handler.h`.)
- `[x]` Define `WebSocketCloseCode` as a strongly-typed enum class aligned with RFC 6455 close codes. (**Implemented** — `enum class WebSocketCloseCode : uint16_t` with all RFC 6455 codes in `include/api/websocket_handler.h`.)

### API Version Router

- `[x]` Define `IAPIVersionRouter::route(HttpRequest&) -> IHttpHandler&` with version extracted from URL path. (**Implemented** — `IAPIVersionRouter::route(method, path, out_deprecation_headers)` in `include/api/api_version_router.h`.)
- `[x]` Add `VersionDescriptor` value type carrying major/minor version and deprecation date fields. (**Implemented** — `VersionDescriptor` with `major_version`, `minor_version`, `deprecation_date`, `sunset_date`, `successor_url` in `include/api/api_version_router.h`.)
- `[x]` Expose `registerVersion(VersionDescriptor, HandlerSet)` to allow multi-version registration at init time. (**Implemented** — `IAPIVersionRouter::registerVersion()` in `include/api/api_version_router.h`.)
- `[x]` Guarantee that `route()` never returns a null handler; unknown versions resolve to an error handler. (**Documented** — `route()` contract in `include/api/api_version_router.h` mandates non-null return.)

### gRPC Bridge Interface

- `[x]` Define `IGRPCBridge` with `registerService(ServiceDescriptor, IHttpHandler&)` and `dispatch(GRPCRequest&)`. (**Implemented** in `include/api/grpc_bridge.h`.)
- `[x]` Expose `ServiceDescriptor` as a plain-data struct (service name, method names, serialization format). (**Implemented** — `ServiceDescriptor` plain-data struct in `include/api/grpc_bridge.h`.)
- `[x]` Add `GRPCMetadata` value type for propagating gRPC headers into the `IHttpHandler` invocation context. (**Implemented** — `GRPCMetadata` value type with `authority`, `method_path`, `content_type`, `deadline`, `user_metadata` in `include/api/grpc_bridge.h`.)
- `[x]` Document serialization contract: bridge converts Protobuf wire format to internal `RequestBody` before dispatch. (**Documented** — `IGRPCBridge::dispatch()` contract in `include/api/grpc_bridge.h` states conversion from Protobuf to `HttpRequest::body`.)

### Request Correlation ID API

- `[x]` Define `ICorrelationIDProvider::generate() -> CorrelationId` as a factory method. (**Implemented** — `ICorrelationIDProvider::generate()` pure-virtual method in `include/api/correlation_id.h`.)
- `[x]` Expose `CorrelationId` as a fixed-width (16-byte UUID) value type with string serialization support. (**Implemented** — `CorrelationId` 16-byte value type with `parse()`, `toString()`, `bytes()`, `isNil()`, `std::hash` in `include/api/correlation_id.h`.)
- `[x]` Add `ICorrelationIDProvider::extract(HttpRequest&) -> CorrelationId` to parse incoming `X-Correlation-ID` headers. (**Implemented** — `ICorrelationIDProvider::extract(const std::unordered_map<string,string>&)` in `include/api/correlation_id.h`.)
- `[x]` Document that `CorrelationId` serialization never includes PII; format is opaque UUID only. (**Documented** — `CorrelationId` and `ICorrelationIDProvider` contracts in `include/api/correlation_id.h` explicitly prohibit PII.)

## Test Strategy

- Compile-time tests verify that all handler types implement `IHttpHandler` using `static_assert` traits
- Unit tests mock `IHttpHandler` and verify that `IAPIVersionRouter` dispatches to the correct version for `/v1/` and `/v2/` prefixes
- WebSocket upgrade tests use a loopback transport to assert non-blocking behavior of `upgrade()` under concurrent load
- GraphQL schema builder tests verify that duplicate type registration returns a structured `SchemaValidationResult` error
- Correlation ID tests assert that `extract()` returns a deterministic ID for a given `X-Correlation-ID` header value
- Integration tests exercise the full `IGRPCBridge` dispatch path with a mock Protobuf service descriptor

## Performance Targets

- `IHttpHandler::handle()` dispatch overhead (router lookup + handler invocation) ≤ 5 µs per request at 10k RPS
- `IGraphQLSchemaBuilder` type-lookup during query planning ≤ 1 µs per field resolution
- WebSocket frame dispatch overhead per frame via `IWebSocketFrameCallback` ≤ 10 µs
- `IAPIVersionRouter::route()` version extraction and handler resolution ≤ 2 µs
- `ICorrelationIDProvider::generate()` UUID generation ≤ 500 ns per call
- `IGRPCBridge::dispatch()` Protobuf-to-internal-body conversion overhead ≤ 20 µs per RPC call

## Security / Reliability

- All endpoints registered via `IHttpHandler` require authentication by default; opt-out must be explicit and documented in the handler declaration
- CORS policy is enforced at the `IHttpHandler` interface boundary; handlers cannot bypass CORS headers
- `CorrelationId` values are opaque UUIDs; the interface contract explicitly prohibits embedding PII, IP addresses, or user identifiers
- `IGraphQLSchemaBuilder::build()` validates schema for injection-vulnerable field name patterns before accepting registration
- `IWebSocketHandler::upgrade()` validates the `Origin` header against a configured allow-list before returning a `WebSocketSession`
- gRPC bridge rejects requests with unknown service names at `IGRPCBridge::dispatch()` rather than forwarding to an unregistered handler
