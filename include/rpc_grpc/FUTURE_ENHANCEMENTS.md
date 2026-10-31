---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# RPC gRPC Public Header — Future Enhancements

## Scope

This page covers only `include/rpc_grpc/`. The directory currently contains
`rpc_grpc_api_contract.h` and `circuit_breaker.h`; it does not contain generated
service stubs. Runtime work is tracked in
[`../../src/rpc_grpc/FUTURE_ENHANCEMENTS.md`](../../src/rpc_grpc/FUTURE_ENHANCEMENTS.md).

## Current status

No public-header enhancement in this page is an accepted implementation
commitment. Earlier proposals about Graph-layer association, C++20 module
wrappers, and a header-only server path are not supported by the current module
layout and are withdrawn pending a concrete design and issue.

## Change constraints

- Keep the contract header distinct from the `IRPCServer` runtime interface.
- Do not imply that descriptive contract types or constants are enforced unless
  the implementation consumes them.
- Document any public API or ABI impact and add focused tests with a proposed
  header change.

## Test references

The circuit-breaker API is covered by
[`../../tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`](../../tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp).
Contract constants and types are checked by
[`../../tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`](../../tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp).
