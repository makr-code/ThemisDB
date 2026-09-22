# ONNX CLIP Plugin – Archive

## Purpose

This directory contains delivery reports and implementation summaries from completed feature phases that were produced during development.

These documents are historical and are no longer part of the canonical module governance set.

When new feature phases (e.g., Phase 4B) are completed, their delivery reports will be archived here as well.

## Archived Documents

### Phase 3B – Hot-Swap Model Reloading

- **PHASE3B_DELIVERY_REPORT.md** — Executive summary of Phase 3B completion, deliverables, and status
- **PHASE3B_IMPLEMENTATION_SUMMARY.md** — Detailed implementation walkthrough with code annotations
- **PHASE3B_QUICK_REFERENCE.md** — Quick reference guide for developers

**Rationale:** Phase 3B features (`reloadModel()`, request draining, atomic swap) have been merged into the main production codebase. The delivery report serves as historical evidence and is not needed for ongoing operations.

### Phase 4B – Memory-Mapped Model Loading

*Status:* Phase 4B is currently in progress. When Phase 4B delivery reports are completed, they will be archived in this directory alongside Phase 3B documentation.

See `ROADMAP.md` for current Phase 4B status and completion timeline.

## Canonical Governance Documents

The authoritative module documentation is located in `src/onnx_clip/`:

- **README.md** — Overview, purpose, components, public API, configuration
- **ARCHITECTURE.md** — Design principles, component diagram, interface implementation
- **ROADMAP.md** — Current status, planned features, implementation phases
- **SECURITY.md** — Threat model, security controls, known limitations
- **AUDIT.md** — Audit findings and compliance status
- **PRODUCTION_REQUIREMENTS.md** — Mandatory operational and security requirements
- **FUTURE_ENHANCEMENTS.md** — Medium/long-term enhancements and research areas
- **PERFORMANCE_EXPECTATIONS.md** — Performance targets and benchmarks
- **CHANGELOG.md** — Version history, added/changed/fixed/removed items
- **MODULE_GAPS.md** — Tracked implementation gaps and follow-up work

## Recovery

If any archived document is needed for reference:

1. Check the git history: `git log --follow src/onnx_clip/archive/PHASE3B_*.md`
2. Restore from git if needed: `git checkout <commit> -- src/onnx_clip/archive/PHASE3B_*.md`

**Note on Branch References:** Some archived documents may reference branch names that are no longer canonical (e.g., "main" instead of "community"). These are historical references preserved from when the documents were created. Refer to `BRANCHING_STRATEGY.md` for current canonical branch names.

---

**Last Updated:** 2026-09-22  
**Archivist:** Documentation Governance Update (Issue #6475)
