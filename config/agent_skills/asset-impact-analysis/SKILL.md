---
name: asset-impact-analysis
description: Analyze downstream asset impact by combining a host-authorized graph relation with bounded time-series window comparisons.
Author: ThemisDB Team
Created: 2026-11-19
Last Updated: 2026-11-19
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# Asset Impact Analysis

Use this skill only when the host has separately registered and allowlisted
`analyze_asset_impact`. Loading this package does not enable the tool, authorize
graph or time-series reads, or allow arbitrary AQL.

## What the tool does

The host binds one authorized property-graph ID, one outgoing relationship
type, one TSStore metric, and the caller's data scope. The tool starts at the
requested asset, follows matching outgoing edges, and compares the same
half-open baseline and current time windows for every reached asset. Results
include the start asset at depth zero. Use it for domain-neutral asset or
entity graphs such as engineering dependencies, financial relationships, or
other time-series-backed systems; do not assume station or sensor semantics.

## Inputs

Provide:

- `start_asset_id`: graph node ID, without `:`.
- `max_depth`: integer from 0 to 3.
- `baseline_start_ms` and `current_start_ms`: non-negative epoch milliseconds.
- `duration_ms`: 1 ms through 31 days; the baseline must end no later than the
  current window starts.
- Optional `geo`: GeoJSON point center `{lon, lat}` and `radius_m` in
  `(0, 1000000]`. This filters nodes by stored `location` Point.

The graph, relationship type, metric, collection scope, and permissions are
host configuration, not tool arguments.

## Reusable guidance

The host may explicitly load `graph-exploration` for graph-scope and
traversal interpretation and `timeseries-window-analysis` for window and
measurement interpretation. These are optional guidance dependencies, not
automatic calls. Carry forward the same authorized scope, limits, and error
status. If a package is unavailable, report it and use this skill's own
instructions; loading it does not register or invoke its associated tool.
In particular, `analyze_asset_impact` uses its own graph and time-series
contract; do not substitute `compare_timeseries_windows` or assume identical
collection bindings.

## Interpretation

Each result reports depth, sample counts, unweighted means, and `delta`
(`current_mean - baseline_mean`). Empty windows produce a null mean; delta is
null unless both windows have samples. A geo request skips nodes without a
location and reports their count; malformed locations fail the whole request.
No geo request means location is not used as a filter.

The tool fails rather than returning a partial traversal when the outgoing
search exceeds 64 vertices, 256 examined adjacency entries, depth 3, or its
cooperative 500 ms graph-traversal deadline. Each node document is limited to
64 KiB; each window is limited to 4096 samples per asset. These are bounded
analysis limits, not database authorization or snapshot guarantees.

Graph and time-series reads are synchronous and not atomic together. A graph
write or measurement update concurrent with the analysis may be reflected
partially. Matching malformed time-series records or compressed chunks cause
an error instead of being silently skipped, but valid missing source
measurements cannot be detected. Do not present the response as a causal or
transactionally consistent result; include the selected windows, graph
relation, and observed limits when explaining conclusions.

## Recommended workflow

1. Confirm the host has allowlisted `analyze_asset_impact` for this mode and
   that the graph, edge type, metric, and series are authorized for the caller.
2. Select a start asset and non-overlapping windows appropriate to the
   domain's sampling cadence.
3. Use a shallow depth first; increase only when a wider downstream scope is
   required.
4. If location is relevant and valid for the domain, add a bounded geo radius.
5. Treat any `error` response as an incomplete analysis; narrow the depth or
   scope and retry rather than interpreting partial results.
6. Report empty windows, missing locations, limits, and the lack of a shared
   snapshot alongside the observed deltas.
