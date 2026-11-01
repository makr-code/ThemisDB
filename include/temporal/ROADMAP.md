# Temporal public-header status

Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors

Exact model backend not verifiable from final document text alone.

This file is a header inventory, not an independent release roadmap. The
canonical module plan is [`../../src/temporal/ROADMAP.md`](../../src/temporal/ROADMAP.md);
the root [`../../ROADMAP.md`](../../ROADMAP.md) controls aggregate maturity and
release readiness.

## Current inventory

The directory contains 17 headers: 16 runtime headers in `themisdb::temporal`
and the separate `temporal_api_contract.h` in `themis::temporal`. Header groups,
their public types, and contract boundaries are listed in
[`README.md`](README.md).

## Status notes

- The runtime headers declare system-versioned and bitemporal APIs, query/join
  helpers, indexes, aggregation, snapshots, retention, CDC, conflict handling,
  compression, migration, and tier/cold-store operations.
- `temporal_api_contract.h` is a distinct contract surface. Its declarations
  and written rules do not replace the runtime headers' millisecond and
  half-open-range semantics.
- The directory inventory does not establish API/ABI stability, SQL:2011
  conformance, CMake target availability, or benchmark/release-gate completion.
- No additional public-header commitment or delivery date is established by
  this document. **TODO:** record a requested interface change here only after
  it is approved in the canonical module roadmap.
