---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Acceleration module guide

## Audience and boundaries

Use this guide to orient a developer integrating or changing acceleration behavior. The module owns backend contracts, runtime discovery, capability selection, dispatch, and fallback. It does not own consumer business logic, vendor SDK internals, embedding model training, or vector-index storage. The public/internal/test ownership split is:

- Public contract: [`include/acceleration/`](../../include/acceleration/)
- Runtime implementation: this directory
- Focused tests: [`tests/acceleration/`](../../tests/acceleration/)
- Benchmark expectations: [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md)

Start with [ARCHITECTURE.md](./ARCHITECTURE.md) for runtime boundaries and [API.md](./API.md) before calling a public interface.

## Typical integration sequence

1. Define required operation, precision, and metric capabilities.
2. Ask the registry for the matching typed backend, or use the existing consumer dispatch surface. Treat a null backend as an expected capability failure.
3. Validate input shape, pointers, dimensions, and ownership before dispatch. Keep caller-owned buffers valid until the synchronous API returns.
4. Inspect the result's status/error fields and selected backend. Do not infer success from device presence alone.
5. Keep a CPU baseline and test behavior with optional hardware absent, unhealthy, and available.

For AI embeddings and similarity requests, use the specific contracts in [EMBEDDING.md](./EMBEDDING.md); the generic CPU inference fallback copies input and is not an embedding model.

## Build and focused validation

The existing acceleration test instructions use:

```sh
cmake --preset linux-release
cmake --build --preset linux-release
```

For a focused test, list registered tests in the configured build directory first, then select the matching acceleration test by its registered CTest name:

```sh
ctest --test-dir <build-directory> -N
ctest --test-dir <build-directory> -R '<acceleration-test-name>' --output-on-failure
```

Relevant coverage includes registry selection, dispatch/fallback, AI routing, failure handling, thread safety, plugin security, and vLLM resource handling; the exact file list and hardware caveats are in [tests/acceleration/README.md](../../tests/acceleration/README.md). Hardware-gated tests and benchmarks are not evidence of hardware behavior when the runtime/device is unavailable.

## Change checklist

- Keep public declarations and Doxygen contracts synchronized.
- Preserve CPU fallback and explicit error reporting.
- Add regression coverage for new inputs, capability mismatch, and failure paths.
- Use benchmark evidence for performance assertions; do not convert an unverified target into a guaranteed capability.
- Update the relevant [runbook](./RUNBOOK.md), [operations guide](./OPERATIONS.md), or [trust guide](./TRUST.md) when operational or security behavior changes.

## Further reading

- [Implementation flow](./IMPLEMENTATION.md)
- [Capability model](./MODELING.md)
- [Runtime diagnosis](./RUNBOOK.md)
- [Operational signals](./OPERATIONS.md)
- [Trust boundaries](./TRUST.md)

**Exact model backend not verifiable from final document text alone.**
