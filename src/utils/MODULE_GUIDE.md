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

Exact model backend not verifiable from final document text alone.

# Utils Module Developer Guide

## Start here

The utils module is shared infrastructure, not a product-domain layer.
Start with [`README.md`](README.md) for scope,
[`ARCHITECTURE.md`](ARCHITECTURE.md) for responsibility boundaries, and
[`API.md`](API.md) for consumer contracts. Read the matching header in
[`include/utils`](../../include/utils/README.md) before changing an interface;
then inspect the `.cpp` implementation and focused tests.

## Find the owner

| Need | Start with |
|---|---|
| Logging, audit, tracing | `audit_logger`, `logger`, `saga_logger`, `tracing` |
| PII detection or redaction | `pii_detector`, engine headers, `pii_stream_scanner`, `pii_pseudonymizer` |
| Keys, signatures, or certificates | `hkdf_helper`, `hkdf_cache`, `lek_manager`, `pki_client` |
| Compression or serialization | `zstd_codec`, `lz4_codec`, `serialization` |
| Scheduling/concurrency | `thread_pool_manager`, `rate_limiter`, `cron_parser`, connection pools |
| Numeric/general helpers | `simd_distance`, `geometric_distances`, `bloom_filter`, `consistent_hash`, `timestamp_utils` |

Implementation files live in `src/utils`; public declarations live in
`include/utils`; component tests live in `tests/utils` (with a few shared
utility tests at `tests/`). Source ownership and compatibility expectations
are described in [`IMPLEMENTATION.md`](IMPLEMENTATION.md).

## Safe integration sequence

1. Construct the helper with valid dependencies and explicit resource limits.
2. Check construction, `Result`, `optional`, and boolean outcomes before
   using derived data or considering an operation successful.
3. Set bounded timeouts for waits and external calls. Handle overload and
   shutdown as expected outcomes, not silent success.
4. Avoid placing raw secrets or unredacted PII in logs, traces, or diagnostic
   payloads. Treat detection output as advisory unless the specific caller
   contract says otherwise.
5. Shut down background workers before destroying dependencies; preserve
   storage and key-provider lifetimes required by the helper.

## Testing and review

Focused C++ suites and their registration are maintained in
[`tests/utils/CMakeLists.txt`](../../tests/utils/CMakeLists.txt); the existing
test workflow is documented in
[`PHASE_4_TEST_EXECUTION_GUIDE.md`](../../tests/utils/PHASE_4_TEST_EXECUTION_GUIDE.md).
Choose tests by changed contract: detection and Unicode, key rotation, audit
concurrency, codec corruption/bounds, rate limiting, or thread-pool
saturation/shutdown. Add failure-path assertions whenever a change affects
resource bounds, configuration reload, or external dependencies.

For new benchmarks or a hot-path change, use the cases and release thresholds
in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md). Do not claim a
performance improvement without comparable measurements. For incidents, use
[`RUNBOOK.md`](RUNBOOK.md) and [`OPERATIONS.md`](OPERATIONS.md).
