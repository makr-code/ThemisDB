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

# Temporal data model

This page explains the temporal model used by the `themisdb::temporal` runtime
for developers designing schema, query, or lifecycle integrations.

## Two independent time dimensions

`VersionedDocument` carries a payload and two ranges:

- `sys_time` records when ThemisDB held that version. Insert/update/delete
  operations on `SystemVersionedTable` close and open system-time versions.
- `valid_time` records when the caller says the fact applies. The bitemporal
  table uses this dimension to select a fact at a requested valid-time point.

The runtime `TimeRange` is half-open: it contains `t` when
`start <= t && t < end`; `kMaxTimestamp` denotes an open upper bound.
`Timestamp` is milliseconds since epoch in `include/temporal/temporal_types.h`.
Use the same unit and endpoint convention for values, predicates, serialized
documents, and tests.

`BiTemporalTable` preserves both dimensions across versions. Its public API
rejects overlapping current valid-time rows for the same key on insertion;
updates close the system-time version and create a replacement. Queries select
the version current at a system-time point whose valid-time period contains the
requested point. See [`bi_temporal.h`](../../include/temporal/bi_temporal.h)
and [`system_versioned_table.h`](../../include/temporal/system_versioned_table.h).

## Separate contract surface

[`temporal_api_contract.h`](../../include/temporal/temporal_api_contract.h)
defines constants and rules in namespace `themis::temporal`, including
nanosecond-named timestamp bounds and an inclusive snapshot formula. Those
rules are not interchangeable with `themisdb::temporal::TimeRange`'s
millisecond, half-open semantics. The associated hardening tests use test-local
mock helpers. Integrators must identify which API surface they call and must
not convert or compare timestamps across the two without an explicit unit and
boundary conversion.

## Modeling trade-offs

System time provides history of database changes; valid time represents
domain-time assertions and may be supplied or corrected by an application.
Keeping both enables as-of queries, interval predicates, and bitemporal joins,
but it requires callers to choose the correct clock for each question. An
empty query result can mean no version matched either time constraint; it is
not by itself evidence that a key never existed.

Retention deliberately changes the available history horizon. Snapshot
managers retain deep copies only in process memory, while CDC has both a
bounded in-memory log and a separate WAL-backed `CDCPersistentLog` interface.
These are different persistence and recovery models.

Exact model backend not verifiable from final document text alone.
