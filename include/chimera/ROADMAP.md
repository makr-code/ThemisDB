---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-11-19
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera public-header roadmap notes

**Status basis:** current header and CMake inspection only. No public API gate, compatibility build, or release review was run.

## Current state

Headers declare adapter, retry, batch, transaction, streaming, and prepared-statement surfaces. `database_adapter.hpp` selects an external header when available and otherwise defines a minimal fallback shim. The checked-in external v0.0.37 header has no unique prepared-statement or connection-pool capability tokens; local aliases exist for compilation but are not reported as distinct capabilities. The retry policy requires an idempotency assertion and classifies concrete error codes, but an operation deadline remains undefined.

## Candidate work

- [ ] Record and test the supported external CHIMERA header revision and fallback behavior.
- [ ] Add evidence-backed per-adapter capability and build-configuration documentation.
- [ ] Validate public headers and actual implementation signatures in each supported configuration.
- [ ] Establish ABI/API compatibility policy from the real dependency and distribution requirements.

## Readiness checklist and known issues

- [ ] Verify external-header and fallback-shim configurations.
- [ ] Confirm compiled adapter implementations match the selected public declarations.
- [ ] Record API and dependency compatibility evidence for each supported build.

Known issue: the public base contract differs according to header selection. No breaking-change plan is established by the current source review.

These are candidates, not dated commitments. See [`CHIMERA_ADAPTER_CONTRACT.md`](CHIMERA_ADAPTER_CONTRACT.md) and [`../../src/chimera/ROADMAP.md`](../../src/chimera/ROADMAP.md).

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
