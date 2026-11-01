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

# RPC gRPC Public Header Roadmap

## Current status

The public include directory contains:

- `circuit_breaker.h`: standalone thread-safe circuit-breaker types.
- `rpc_grpc_api_contract.h`: contract enums, shared size defaults, and
  descriptive service metadata.

These are not generated gRPC service definitions or stubs. The plugin runtime
and its `GRPCServer` implementation live under `src/rpc_grpc/`.

## Planned work

There are no accepted public-header roadmap items at this time. Add a scoped
issue and define runtime enforcement, compatibility impact, and tests before
planning new public types or changing the contract.

## References

- [Header architecture](ARCHITECTURE.md)
- [Header change constraints](FUTURE_ENHANCEMENTS.md)
- [Runtime roadmap](../../src/rpc_grpc/ROADMAP.md)
- [Focused tests](../../tests/rpc_grpc/README.md)
