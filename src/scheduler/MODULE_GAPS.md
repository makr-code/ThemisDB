---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: module-behavior
Canonical references: src/scheduler/*.cpp; include/scheduler/*.h; tests/scheduler/*.cpp; benchmarks/scheduler/*.cpp
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler module gaps

## Status of the prior scanner output

The previous version reported 1,493 scanner findings, including severity/type
counts, but identified its scan location only as `C:\Projects\ThemisDB` and did
not preserve a reproducible command, revision, or timestamp. Those numbers are
historical scanner output, not verified current findings; they are not carried
forward as active defects.

## Gaps confirmed during documentation review

- Distributed dispatch uses a lifetime-safe shared gate checked on immediate,
  scheduled, and DAG execution paths. Existing coordination-failure placeholder
  tests remain skipped; a production coordinator integration test covers the
  no-leadership denial path.
- The coordination failure fixture skips, and some burst/retention test cases
  are simulated or placeholders.
- Existing benchmark families do not establish an enforced CI threshold for
  production scheduler latency or a hardware baseline.
- Event-trigger configuration and runtime evaluation reject malformed or
  unsupported conditions.
- Security-sensitive scheduler operations deny when `RequestContext` is absent;
  HTTP and trusted in-process callers now set operation-limited scoped contexts.

These are source/documentation observations, not a rescan of all static-analysis
gap categories.

## Follow-up

**TODO:** rerun the canonical gap scanner against a pinned revision, record its
command/output provenance, and triage results before publishing current counts
or severity totals.
