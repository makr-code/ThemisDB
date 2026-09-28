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

### Auto-Generated and Meta-Documentation (Archived 2026-09-28)

- **DOXYGEN.md** — Snapshot of Doxygen XML output from module scanning. This is a meta-artifact generated during development and is superseded by the actual Doxygen output in the CI/CD pipeline.

- **MODULE_GAPS.md** — Auto-generated gap scanner output from June 2026. This file was automatically generated and is now superseded by the compliance governance system. Original findings have been reviewed and addressed in the implementation.

**Rationale:** These files are automatically generated and are not part of the canonical governance documentation set. Developers should consult the CI/CD compliance reports and the Doxygen governance workflow for current status.

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

## Recovery

If any archived document is needed for reference:

1. Check the git history: `git log --follow src/onnx_clip/archive/PHASE3B_*.md`
2. Restore from git if needed: `git checkout <commit> -- src/onnx_clip/archive/PHASE3B_*.md`

**Note on Branch References:** Some archived documents may reference branch names that are no longer canonical (e.g., "main" instead of "community"). These are historical references preserved from when the documents were created. Refer to `BRANCHING_STRATEGY.md` for current canonical branch names.

---

**Last Updated:** 2026-09-28  
**Archivist:** Governance Documentation Alignment (Issue #6591)
