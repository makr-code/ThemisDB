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

1. keep packages and products in a mutex-protected cache backed by an explicit RocksDB path,
2. verify manifest hashes and require pinned-signer signature verification,
3. deploy products only after quality `kAllow` and independent service authorization,
4. compute metrics from explicitly supplied result and ground-truth vectors.

These APIs do not enforce artifact governance or perform live retrieval validation.

## Runtime inputs and outputs

### Package registry lifecycle

The application must load `config/ai_ml/retrieval.yaml` through
`RuntimeConfig::loadFromThemisConfig()` or construct the production store with
`openFromThemisConfig()`. The YAML file must contain a dedicated persistent
RocksDB path and administrator-provisioned Ed25519 public-key pins; an empty or
invalid pin set prevents startup. Keep the database path dedicated to this
store and protected by service filesystem/storage controls. When deployment is
required, inject the quality controller and the distinct authorization callback
from the owning deployment service; without authorization, `deployProduct()`
denies.

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

The store does not emit lifecycle-transition or audit events. Missing or
invalid signatures, unpinned signers, bad hashes, missing quality controller,
and missing/denied/throwing authorization all fail closed. The quality
controller evaluates regression only and cannot authorize an operation.

## Safety posture

The module does not implement a general retrieval fail-closed operational policy:

- selected missing/malformed JSON fields can throw, while many fields remain optional,
- production writes/imports reject artifacts with missing records, hashes,
  signatures, pins, or failed Ed25519 verification; invalid persisted records
  prevent startup,
- deployment requires explicit owning-service authorization and quality allow,
- metrics are only informative when supplied with real results and judgments,
- no automatic promotion/rollback decision is implemented.

## Caller-side considerations (not module controls)

If an application builds on these APIs, it must independently decide:

- how the RocksDB path and signer keys in `config/ai_ml/retrieval.yaml` are
  provisioned/rotated/protected,
- how the owning service implements and audits deployment authorization,
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
