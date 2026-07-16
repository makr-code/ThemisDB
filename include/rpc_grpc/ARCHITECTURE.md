> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/rpc_grpc/ARCHITECTURE.md -->

# RPC/gRPC Module — Public Header Architecture

**Module Path:** `include/rpc_grpc/`  
**Implementation:** `../../src/rpc_grpc/`  
**Canonical architecture doc:** [`../../src/rpc_grpc/ARCHITECTURE.md`](../../src/rpc_grpc/ARCHITECTURE.md)

---

## 1. Overview

`include/rpc_grpc/` defines the **public gRPC service definitions and generated stubs for ThemisDB distributed communication API contract** for ThemisDB.

> **Note:** No standalone public C++ headers in this module; gRPC stubs are generated from `.proto` files and consumed via the `server` and `sharding` modules.
