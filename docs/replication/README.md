# Replication Documentation

## Purpose

This directory is the secondary documentation layer for replication operations and architecture.
Primary workload and behavior state is defined in `src/replication/` planning docs.

## Alignment Contract

Leading workload sources:

- `src/replication/FUTURE_ENHANCEMENTS.md`
- `src/replication/MODULE_GAPS.md`
- `src/replication/ROADMAP.md`

Rule:

- Newer planning docs override older historical status pages.
- This directory must not present completion claims that conflict with newer gap/planning evidence.

## Current Focus

- high-availability operation and deployment guidance
- topology and failover runbooks
- operator troubleshooting and benchmarking references

## Key References

- `../replication-ha-guide.md`
- `../replication_raid_plan.md`
- `../reports/REPLICATION_IMPLEMENTATION_STATUS.md` (historical/contextual)
- `../de/replication/PRIMARY_SOURCES.md`
- `../en/replication/PRIMARY_SOURCES.md`

## Maintenance Notes

- Keep behavior statements synchronized with `src/replication/` planning files.
- Treat old implementation-status percentages as historical context, not authoritative current state.
- Prioritize updates when alignment report flags stale docs against module planning.

---

Updated during docs-vs-planning alignment sweep on 2026-05-31.
