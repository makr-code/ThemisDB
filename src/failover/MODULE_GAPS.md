---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Failover Gap Scan — Historical Snapshot

> **Historical, non-normative artifact.** The scan data below was generated on
> 2026-06-04 and was not regenerated for this documentation update. Its raw
> finding counts and remediation classifications are not verified current
> status. Some conclusions conflict with current source (notably timed thread
> joins and manager-state recovery from the quorum log); do not use this file to
> claim that findings are fixed or open.

The original scanner snapshot reported 22 findings (5 critical, 15 high, and 2
low) across source and documentation. These counts describe the old scan only.
The original scanner output is retained in repository history; this summary does
not reproduce its outdated line numbers or unsupported closure claims.

## Current follow-up items confirmed by source inspection

- `AutoFailoverManager::stop()` uses direct thread `join()` calls; the computed
  deadlines do not enforce a bounded join.
- `QuorumLog::recover()` reads the file, but construction only logs the recovered
  record and does not restore manager epoch or quorum state.
- Built-in DR restore and traffic-shift handlers do not call external adapters;
  spare activation and metadata update likewise do not complete those actions.
- Failover-specific runtime does not expose named metrics or trace spans.

See [`AUDIT.md`](AUDIT.md) and [`IMPLEMENTATION.md`](IMPLEMENTATION.md) for the
current code-backed audit. New scan findings require a fresh scan and source
review before they are classified.
