<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — gRPC RPC Plugin

## Module Overview

The gRPC plugin provides `GRPCServer` (`IRPCServer`) and `GRPCPlugin`
(`IRPCPlugin` + `IThemisPlugin`). It uses gRPC C++ with HTTP/2, Protocol Buffers,
mTLS, and bidirectional streaming. Default port: 50051.

---

## Source File Inventory

| # | File | Description | Lines | Status |
|---|------|-------------|-------|--------|
| 1 | `grpc_plugin.h` | `GRPCServer` + `GRPCPlugin` declarations; `IRPCServer`/`IRPCPlugin` compliance | 122 | ✅ Complete |
| 2 | `grpc_plugin.cpp` | Full implementation: lifecycle, credentials, service registration, stats | 284 | ✅ Complete |
| 3 | `CMakeLists.txt` | Build configuration; links gRPC++ and protobuf | — | ✅ Complete |

**Total: 3 files**

---

## Test Coverage Summary

| Test Target | Scope | Status |
|-------------|-------|--------|
| `GRPCServer::initialize()` | Sets `server_address_` from host:port | ⚠️ Not confirmed |
| `GRPCServer::start()` (insecure) | Binds on localhost, `isRunning()` = true | ⚠️ Not confirmed |
| `GRPCServer::stop()` | Server shutdown, `isRunning()` = false | ⚠️ Not confirmed |
| `GRPCServer::registerService()` | Null pointer guard, valid service appended | ⚠️ Not confirmed |
| `configureCredentials()` TLS | Cert load, mTLS vs TLS mode | ⚠️ Not confirmed |
| `configureCredentials()` fail-closed | Throws on bad cert path | ⚠️ Not confirmed |
| `GRPCPlugin::createServer()` | Returns non-null `IRPCServer` | ⚠️ Not confirmed |
| `GRPCPlugin::getDefaultPort()` | Returns 50051 | ⚠️ Not confirmed |
| `getStats()` / `resetStats()` | Counter correctness, mutex protection | ⚠️ Not confirmed |

---

## Open Items

| ID | Description | Priority | Target |
|----|-------------|----------|--------|
| GRPC-OPEN-01 | Unit and integration tests not confirmed | High | Q3 2026 |
| GRPC-OPEN-02 | `RPCServerStats::uptime_seconds` not incremented after start | Medium | Q3 2026 |
| GRPC-OPEN-03 | gRPC health-check service not auto-registered | Medium | Q4 2026 |
| GRPC-OPEN-04 | No server-side request/response interceptors for telemetry | Low | Q4 2026 |

---

## Audit Sign-off

| Date | Auditor | Verdict |
|------|---------|---------|
| 2026-03-22 | Initial module audit | Passed — 4 open items tracked above |
