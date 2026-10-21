---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Vector Search Runbook

## Purpose

This document covers the expected recovery and diagnosis flow for the ThemisDB vector-search subsystem. It is grounded in the shared index runtime and the operational constraints expressed in the module docs, especially the index lifecycle and the failure handling described in `vector_index.h` and `vector_index.cpp`.

## Primary symptoms

Common operator symptoms include:

- search results are missing or unexpectedly inconsistent,
- queries fail immediately after index creation,
- insertion/update operations appear to succeed but are not reflected in later searches,
- the index rebuild or reindex path triggers repeatedly,
- a backend fails to initialize because advanced support is unavailable.

## Triage flow

### 1. Confirm the configuration

Check:

- configured dimension,
- selected metric,
- object namespace and persistence path,
- advanced backend settings,
- `efSearch` and other ANN tuning values.

A mismatch between query and index dimension or metric is the first condition to confirm. The code treats this as a hard failure path.

### 2. Check the status object and logs

`VectorIndexManager::Status` exists to surface operational failure clearly. Examine:

- `ok` state,
- the returned `message`,
- any warnings logged during advanced-index setup or reindex operations.

A missing or misleading status is a sign that the runtime was not executed with the expected guard rails.

### 3. Inspect reindex and rebuild state

If the index is stale, check whether:

- storage state drift is large enough to require rebuild,
- the incremental reindex threshold has been exceeded,
- index persistence or load has not completed correctly.

The API explicitly exposes `incrementalReindex()` and `rebuildFromStorage()` to make this diagnosable.

### 4. Validate persistence and load

If the index is unexpectedly empty after restart or fails to load, confirm:

- the save directory is valid,
- the index was shut down cleanly,
- the persisted data matches the configured namespace,
- the advanced backend configuration is still compatible with the runtime.

### 5. Check advanced backend support

The implementation warns and fails when advanced index support is requested without required runtime support. If a backend was configured but the machine lacks the required dependency or compile-time support, validate the build configuration before investigating deeper index corruption.

## Recovery actions

### Safe recovery path

1. halt write traffic to the affected namespace if possible,
2. confirm dimension and metric configuration,
3. run a rebuild or incremental reindex,
4. verify search queries against a small known-good vector set,
5. re-enable normal access after consistency checks pass.

### Persistence recovery

If the index was saved and later fails to load:

- verify the save path,
- confirm the runtime configuration matches the persisted data,
- rebuild from storage if the persisted index metadata is stale,
- keep the operational note that persistent index state is not a substitute for correct validation.

## Escalation triggers

Escalate when:

- repeated rebuilds are required on a healthy workload,
- queries fail with dimension or metric mismatches under normal runtime conditions,
- the same vector ID fails repeatedly across multiple operations,
- benchmark or stress-test regressions exceed the documented operational thresholds,
- an unsupported advanced backend or storage drift pattern recurs across restarts.

## Operational notes

The module docs already distinguish between planned features and active requirements. High-availability sharding and durable distributed persistence remain planned work, so operators should treat those as future hardening rather than current production guarantees.

## Conclusion

The vector-search runbook should emphasize explicit diagnosis and recoverable rebuild steps, not hidden fallback behavior. The primary design principle is clear: when the runtime is in an invalid state, call out the failure and repair the underlying contract rather than silently masking it.
