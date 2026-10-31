---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Operations Guide

## Purpose

This document records the limited callable behavior and operational boundaries of the current code. It is not a deployment runbook or an assertion that the module is operating in production.

## Operational responsibilities

The checked-in code exposes these narrow operations:

1. keep packages and products in a process-local, mutex-protected map,
2. allow callers to verify manifest hashes and optionally invoke a signature callback,
3. compute metrics from explicitly supplied result and ground-truth vectors.

These APIs do not enforce artifact governance or perform live retrieval validation.

## Runtime inputs and outputs

### Package registry lifecycle

Inputs to the store include:

- package metadata,
- provenance data,
- architecture compatibility metadata,
- integrity fields and signature metadata,
- status and timestamps.

Outputs include:

- package lookup and listing,
- product lifecycle status,
- verification booleans,
- export/import payloads.

### Dual-read metrics helper

Inputs include:

- for `ValidateQuery()`: query text and top-k, with judgments previously supplied through `SetGroundTruth()`;
- for metric helpers: a caller-provided result vector and ground-truth vector.

Outputs include:

- current-result recall / nDCG / MRR values,
- previous result vectors (currently empty because `ExecuteQuery()` is a placeholder),
- batch averages for nonempty batches.

`ValidateQuery()` does not return the previous metrics or a current-versus-previous delta. `RetrievalMetrics::HasRegression()` and `ComputeDeltas()` are separate helpers that callers can invoke with metrics they already have.

## Monitoring signals

The source logs method-level diagnostics and returns helper results; it does not provide production monitoring or audit signals.

Key signals to watch:

- a false result from explicit manifest verification,
- a warning when a query has no cached ground truth,
- metric helper results supplied to `HasRegression()` by a caller.

The store does not emit lifecycle-transition or audit events. The signature callback is only invoked when configured and a signature is non-empty; absence of either does not fail a matching manifest hash.

## Safety posture

The module does not implement a general fail-closed operational policy:

- selected missing/malformed JSON fields can throw, while many fields remain optional,
- explicit hash verification returns false for missing records, empty manifest hashes, or hash mismatches,
- metrics are only informative when supplied with real results and judgments,
- no automatic promotion/rollback decision is implemented.

## Caller-side considerations (not module controls)

If an application builds on these APIs, it must independently decide:

- how artifact data is persisted and protected,
- whether signatures are mandatory and which trusted verifier is used,
- how lifecycle/deployment authorization is enforced,
- how real retrieval results and ground truth are obtained,
- how verification failures and operational recovery are handled.

## Failure handling

The following are application-owner actions, not behavior implemented by this module:

1. stop trust progression for the impacted artifact or index,
2. capture the exact mismatch or metric delta,
3. compare to the last known-good manifest or index baseline,
4. rollback or quarantine the change,
5. document the cause and fix in the module runbook or release notes.

## Conclusion

No independent runtime configuration or production operator procedure is provided here. See [module limits](README.md), [API behavior](API.md), and [security boundaries](SECURITY.md).
