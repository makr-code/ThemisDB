---
name: aql-plan-analysis
description: Interpret ThemisDB AQL query plans and distinguish optimizer estimates from observed performance.
metadata:
  AI_Used: true
  AI_Models: "unknown"
  AI_Contributor: "GitHub Copilot"
  Human_Review: false
  Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

# Analyze an AQL query plan

Use this skill for a supplied plan or an explicitly authorized plan-only
interface. For logical query correctness, use the `aql-query-review` guidance.

1. Establish the target version, query shape, plan source, and available index
   metadata. Ask for a sanitized plan when the exact plan interface is unknown.
   The repository's C++ plan entry points are in `include/query/aql_runner.h`;
   their existence does not imply an HTTP endpoint or identical coverage.
2. Identify access paths, estimated cardinality, filters, sorts, joins, and
   limits when the plan displays them. Note when the visualizer shows a
   simplified plan. Do not equate missing nodes with work that did not occur.
3. Distinguish estimated rows or cost from measured rows, timing, and memory.
   A plan-only operation, an `analyze` option, or an `EXPLAIN` label is not
   proof of measured execution. Never execute or profile the query, or apply
   index or configuration changes, as part of this review.
4. Return the suspected bottleneck, the plan evidence, and one bounded
   verification step. Present rewrites and index changes as proposals and
   flag ways they could change result semantics or write overhead.
