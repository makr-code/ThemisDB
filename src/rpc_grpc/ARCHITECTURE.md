> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# gRPC Plugin — Architecture Guide

**Version:** 0.0.1
**Last Updated:** 2026-04-06
**Module Path:** `src/rpc_grpc/`

---

## 1. Overview

The gRPC plugin delivers an RPC transport layer for ThemisDB using the gRPC C++ library.
It provides `GRPCServer` (the server lifecycle manager) and `GRPCPlugin` (the
`IThemisPlugin` factory). Service implementations (protobuf `.grpc.pb.h` classes) are
registered with the server as `grpc::Service*` pointers and owned by the caller.

---

## 2. Design Principles

- **Fail-closed TLS** — if TLS is configured and certificate files cannot be loaded,
  `configureCredentials()` throws; the server refuses to start rather than silently
  falling back to insecure mode.
- **Service registration decoupling** — `GRPCServer` only stores `grpc::Service*`
  pointers; service implementation lifetime is managed by the caller.
- **Atomic running state** — `std::atomic<bool> running_` allows `isRunning()` to be
  called from any thread without locking.
- **Separate plugin and server lifetimes** — `GRPCPlugin` is stateless after
  `initialize()`; each `createServer()` call produces an independent server instance.

---

## 3. Component Architecture

### 3.1 Component Diagram

```
┌──────────────────────────────────────────────────────┐
│  GRPCPlugin                                          │
│  (IRPCPlugin + IThemisPlugin)                        │
│                                                      │
│  getName()         → "grpc"                          │
│  getDefaultPort()  → 50051                           │
│  getProtocol()     → RPCProtocol::GRPC              │
│  createServer()    → unique_ptr<IRPCServer>         │
└──────────────────────┬───────────────────────────────┘
                       │ createServer()
                       ▼
┌──────────────────────────────────────────────────────┐
│  GRPCServer                                          │
│  (IRPCServer)                                        │
│                                                      │
│  initialize(config)  ─ sets server_address_         │
│  registerService(svc) ─ appends to services_        │
│  start()             ─ builds and starts server     │
│  stop()              ─ server->Shutdown()           │
│  isRunning()         ─ atomic<bool>                 │
│  getStats()          ─ mutex-protected stats        │
└──────────────────────┬───────────────────────────────┘
                       │
          ┌────────────▼────────────────────┐
          │  grpc::ServerBuilder            │
          │  .AddListeningPort(addr, creds) │
          │  .RegisterService(svc)          │
          │  .SetMaxReceiveMessageSize(100M)│
          │  .SetMaxSendMessageSize(100M)   │
          │  .BuildAndStart()               │
          └────────────┬────────────────────┘
                       │
          ┌────────────▼────────────────────┐
          │  configureCredentials()         │
          │                                 │
          │  tls_enabled=false:             │
          │    InsecureServerCredentials()  │
          │                                 │
          │  tls_enabled=true:              │
          │    loadFile(cert), loadFile(key)│
          │    loadFile(ca_cert)            │
          │    auth_required=true:          │
          │      mTLS (require + verify)    │
          │    auth_required=false:         │
          │      TLS (server cert only)     │
          │    SslServerCredentials(opts)   │
          │                                 │
          │  On failure: throw → server     │
          │  refuses to start (fail-closed) │
          └─────────────────────────────────┘
```

### 3.2 Class Responsibilities

| Class | File | Responsibility |
|-------|------|----------------|
| `GRPCPlugin` | `grpc_plugin.h/.cpp` | `IThemisPlugin` + `IRPCPlugin` factory; stateless after init |
| `GRPCServer` | `grpc_plugin.h/.cpp` | Server lifecycle: init, register, start, stop, stats |

---

## 4. Data Flow

### 4.1 Server Startup

```
GRPCPlugin::createServer()
  → new GRPCServer()

GRPCServer::initialize(config)
  → server_address_ = host:port

GRPCServer::registerService(svc)
  → services_.push_back(svc)

GRPCServer::start()
  → grpc::ServerBuilder builder
  → builder.AddListeningPort(server_address_, configureCredentials())
  → for svc in services_: builder.RegisterService(svc)
  → builder.SetMaxReceiveMessageSize(100 MB)
  → builder.SetMaxSendMessageSize(100 MB)
  → server_ = builder.BuildAndStart()
  → running_ = true
```

### 4.2 Credentials Configuration

```
configureCredentials():
  if !tls_enabled:
    return InsecureServerCredentials()     // DEV ONLY

  try:
    server_cert = loadFile(tls_cert_path)
    server_key  = loadFile(tls_key_path)
    ca_cert     = loadFile(tls_ca_cert_path)

    ssl_opts.pem_root_certs = ca_cert
    ssl_opts.pem_key_cert_pairs = [{server_key, server_cert}]

    if auth_required:
      ssl_opts.client_certificate_request =
        GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY
    else:
      ssl_opts.client_certificate_request =
        GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE

    return SslServerCredentials(ssl_opts)

  catch:
    throw std::runtime_error("TLS configuration failed - aborting for security")
    // FAIL-CLOSED: never falls back to insecure mode
```

---

## 5. Integration Points

| Direction | Module | Interface |
|-----------|--------|-----------|
| **Implements** | `plugins/rpc_plugin_interface.h` | `IRPCPlugin` + `IThemisPlugin` |
| **Implements** | `plugins/rpc_plugin_interface.h` | `IRPCServer` |
| **Provides to** | `src/server/` | gRPC transport for service handlers |
| **Exported via** | `extern "C" createPlugin()` | Dynamic plugin loader |

---

## 6. Threading & Concurrency

- `running_` is `std::atomic<bool>`; safe to read from any thread.
- `stats_mutex_` serialises `getStats()` and `resetStats()`.
- gRPC's internal thread pool handles connection I/O and handler dispatch.
- `services_` is populated before `start()`; no concurrent write during operation.

---

## 7. Configuration

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `host` | string | `"0.0.0.0"` | Bind address |
| `port` | uint16 | `50051` | Bind port |
| `tls_enabled` | bool | `false` | Enable TLS / mTLS |
| `tls_cert_path` | string | — | Server certificate path |
| `tls_key_path` | string | — | Server private key path |
| `tls_ca_cert_path` | string | — | CA certificate path |
| `auth_required` | bool | `false` | Require client cert (mTLS) |

---

## 8. Error Handling

| Scenario | Behaviour |
|----------|-----------|
| `start()` while already running | Returns `false`; logs error |
| `builder.BuildAndStart()` fails | Returns `false`; logs "Failed to start gRPC server" |
| Certificate file not found | `configureCredentials()` throws; `start()` propagates |
| `registerService(nullptr)` | Logs error; service not added |

---

## 9. Known Limitations

- `RPCServerStats::uptime_seconds` is not updated after start (incremental tracking planned).
- No health-check service (`grpc.health.v1.Health`) is auto-registered.
- Single-port only; SNI-based multi-host TLS is not supported.
