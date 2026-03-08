# API Module - Future Header Enhancements

## Scope

- `IHttpHandler` interface extensions for versioned routing and middleware chaining
- GraphQL schema builder interface (`IGraphQLSchemaBuilder`) for runtime schema composition
- WebSocket upgrade API (`IWebSocketHandler`) for real-time streaming endpoints
- gRPC bridge interface (`IGRPCBridge`) mapping HTTP/2 RPC calls to internal handler contracts
- API version router interface (`IAPIVersionRouter`) enforcing backward-compatible `/v2/` routing
- Correlation ID propagation API for end-to-end request tracing across handler boundaries

## Design Constraints

- `[ ]` All request handlers must implement `IHttpHandler`; no ad-hoc handler registration bypassing the interface
- `[ ]` GraphQL schema is generated exactly once at server initialization; post-init schema mutation is not permitted
- `[ ]` WebSocket upgrade is non-blocking; `IWebSocketHandler::upgrade()` returns immediately and delivers frames via callback
- `[ ]` API version selection must be backward-compatible; a `/v2/` handler must not remove endpoints present in `/v1/`
- `[ ]` Correlation IDs are generated at the interface boundary; downstream handlers receive IDs as opaque `CorrelationId` values
- `[ ]` All public header types are header-only value types or pure-virtual interfaces; no implementation in headers

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

- `[ ]` Define `IGraphQLSchemaBuilder` with `addType()`, `addQuery()`, `addMutation()`, and `build()` methods
- `[ ]` Expose `GraphQLTypeDescriptor` as a plain-data struct usable without a schema library dependency
- `[ ]` Add `SchemaValidationResult` value type returned by `build()`; carries structured error list on failure
- `[ ]` Document that `build()` is callable exactly once; subsequent calls return `Result::error(SchemaAlreadyBuilt)`

### WebSocket Endpoint Interface

- `[ ]` Define `IWebSocketHandler` with `upgrade(HttpRequest&) -> Result<WebSocketSession>`
- `[ ]` Expose `WebSocketSession` as an opaque handle with `send(Frame)` and `close(CloseCode)` methods
- `[ ]` Add `IWebSocketFrameCallback` pure-virtual interface for incoming frame dispatch; callbacks must be `noexcept`
- `[ ]` Define `WebSocketCloseCode` as a strongly-typed enum class aligned with RFC 6455 close codes

### API Version Router

- `[ ]` Define `IAPIVersionRouter::route(HttpRequest&) -> IHttpHandler&` with version extracted from URL path
- `[ ]` Add `VersionDescriptor` value type carrying major/minor version and deprecation date fields
- `[ ]` Expose `registerVersion(VersionDescriptor, HandlerSet)` to allow multi-version registration at init time
- `[ ]` Guarantee that `route()` never returns a null handler; unknown versions resolve to an error handler

### gRPC Bridge Interface

- `[ ]` Define `IGRPCBridge` with `registerService(ServiceDescriptor, IHttpHandler&)` and `dispatch(GRPCRequest&)`
- `[ ]` Expose `ServiceDescriptor` as a plain-data struct (service name, method names, serialization format)
- `[ ]` Add `GRPCMetadata` value type for propagating gRPC headers into the `IHttpHandler` invocation context
- `[ ]` Document serialization contract: bridge converts Protobuf wire format to internal `RequestBody` before dispatch

### Request Correlation ID API

- `[ ]` Define `ICorrelationIDProvider::generate() -> CorrelationId` as a factory method
- `[ ]` Expose `CorrelationId` as a fixed-width (16-byte UUID) value type with string serialization support
- `[ ]` Add `ICorrelationIDProvider::extract(HttpRequest&) -> CorrelationId` to parse incoming `X-Correlation-ID` headers
- `[ ]` Document that `CorrelationId` serialization never includes PII; format is opaque UUID only

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
