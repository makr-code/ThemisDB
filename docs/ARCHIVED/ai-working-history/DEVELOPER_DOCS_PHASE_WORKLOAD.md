# Developer Documentation Phase Workload

**Scope:** Developer-facing documentation in `src/<module>/`, `include/<module>/`, `tests/`, and `benchmarks/`.

**Goal:** Update and consolidate source-adjacent documentation so that every module has a consistent, source-backed doc package that is easy to maintain and easy to regenerate.

**Primary Templates and Guides**

- [SOURCECODE_DOCUMENTATION_7PHASE_TEMPLATE.md](SOURCECODE_DOCUMENTATION_7PHASE_TEMPLATE.md)
- [DOCUMENTATION_GUIDELINES.md](DOCUMENTATION_GUIDELINES.md)
- [MODULE_DOCUMENTATION_GUIDE.md](MODULE_DOCUMENTATION_GUIDE.md)
- [docs/_standards/DOC_TEMPLATE.md](../docs/_standards/DOC_TEMPLATE.md)

---

## Baseline From Local Python Inventory

The following inventory was generated locally from the workspace:

- `src/` contains 68 module READMEs.
- `src/` contains 62 `MODULE_GAPS.md`, 62 `ROADMAP.md`, and 62 `FUTURE_ENHANCEMENTS.md` files.
- `src/` also contains a much larger set of additional markdown files spread across the module folders, so the rewrite scope must include all `.md` files in each folder, not just the canonical quartet.
- `include/` contains 70 `README.md` files.
- `tests/` contains 25 `README.md` files.
- `benchmarks/` contains 18 `README.md` files.
- A source scan found that only a small subset of `src/` READMEs use the same section vocabulary, so cross-module structure is not yet uniform.

Interpretation:

- `src/` is already the strongest source of module-level truth, but the section structure is not standardized enough.
- `include/` is the public contract layer and should be harmonized with `src/` behavior and ownership notes.
- `tests/` and `benchmarks/` need explicit documentation of purpose, coverage, determinism, and execution flow.

---

## Phase 0: Inventory, Baseline, and Scope Lock

### Objective
Capture the current documentation surface and freeze the source of truth for each area before rewriting anything.

### Local Python Inputs

- `ai_working/show_top_modules.py`
- `ai_working/inspect_gaps.py`
- custom Python inventory over `src/`, `include/`, `tests/`, and `benchmarks/`

### Tasks

- [ ] Inventory all developer docs by area and file type.
- [ ] Map each module to its canonical doc set: `README.md`, `MODULE_GAPS.md`, `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`.
- [ ] Detect missing or stale references between `src/` and `include/`.
- [ ] Identify `tests/` and `benchmarks/` docs that lack prerequisites, execution commands, or determinism notes.
- [ ] Record the exact section vocabulary currently used across `src/` readmes.

### Deliverables

- Baseline inventory JSON or markdown summary in `ai_working/`.
- List of modules that require normalization.
- List of docs that are missing cross-links or execution notes.

### Exit Criteria

- Every target area has a documented baseline.
- No rewrite starts before the source-of-truth map is complete.

---

## Phase 1: Structure Normalization for `src/<module>/`

### Objective
Bring every module README into a consistent source-code documentation shape.

### Standard Structure Target

- Overview
- Responsibility / scope
- Source-of-truth links
- Entry points and usage
- Dependencies and build hooks
- Open documentation tasks

### Tasks

- [ ] Normalize headings and section order across all module READMEs.
- [ ] Ensure every module README links to its `MODULE_GAPS.md`, `ROADMAP.md`, and `FUTURE_ENHANCEMENTS.md` where they exist.
- [ ] Add missing source-of-truth links to `src/<module>/README.md`.
- [ ] Remove duplicated or conflicting prose across neighboring module docs.
- [ ] Mark stale behavioral statements as historical if they are not verified anymore.
- [ ] Inventory and classify all additional `.md` files in each `src/<module>/` folder so they can be updated or linked consistently.
- [ ] Decide whether folder-local markdown files are canonical, supporting, or historical, and apply the same rule across every module.

### Local Python Support

- Use Python to identify README files missing canonical links.
- Use Python to compare section headings across modules.

### Deliverables

- Updated `src/<module>/README.md` files.
- An inventory of all additional markdown files in each source folder.
- A consistency report with before/after statistics.

### Exit Criteria

- Every module README uses the same high-level structure.
- Canonical module artifacts are linked consistently.

---

## Phase 2: Technical Contract Alignment for `include/<module>/`

### Objective
Make public header documentation reflect the actual API surface and usage constraints.

### Tasks

- [ ] Align `include/<module>/README.md` content with the corresponding `src/<module>/` module overview.
- [ ] Document public API purpose, ownership, and lifetime expectations.
- [ ] Add or refresh Doxygen-compatible comments for changed public headers where needed.
- [ ] Ensure include docs describe how the headers are consumed from CMake and external targets.
- [ ] Cross-check header docs against `src/` behavior and tests.

### Deliverables

- Updated include-level README files.
- API documentation backlog for any public headers that still need Doxygen comments.

### Exit Criteria

- Public headers are documented as contracts, not implementation notes.
- API docs match source and build behavior.

---

## Phase 3: Test Documentation Consolidation

### Objective
Turn `tests/` documentation into a reliable execution and regression guide.

### Tasks

- [ ] Document what each test suite validates.
- [ ] Document required fixtures, environment assumptions, and gates.
- [ ] Document stable execution commands and useful filters.
- [ ] Mark any focused or flaky tests with rationale and stabilization notes.
- [ ] Add missing pointers from `tests/README.md` to module docs or affected source areas.

### Deliverables

- Updated `tests/README.md` files.
- A test-doc matrix mapping suites to modules and execution commands.

### Exit Criteria

- Each important test area explains purpose, run mode, and failure interpretation.
- The docs make it clear how tests support source-code changes.

---

## Phase 4: Benchmark Documentation Consolidation

### Objective
Make benchmark docs reproducible and traceable to source changes.

### Tasks

- [ ] Document benchmark purpose, input data, and expected outputs.
- [ ] Capture preset, script, and command-line invocation for each benchmark area.
- [ ] Record baseline assumptions and how to compare results across runs.
- [ ] Separate historical measurements from current reproducible guidance.
- [ ] Link benchmark docs back to the affected modules in `src/`.

### Deliverables

- Updated `benchmarks/README.md` files.
- Benchmark runbook entries for reproducibility-critical cases.

### Exit Criteria

- Every benchmark doc explains how to reproduce the measurement.
- Historical numbers are clearly separated from current instructions.

---

## Phase 5: Cross-Linking, Indexing, and De-duplication

### Objective
Remove fractured navigation and make the doc set traversable.

### Tasks

- [ ] Add or repair links between `src/`, `include/`, `tests/`, and `benchmarks/`.
- [ ] Ensure all module docs point to the same canonical source-of-truth files.
- [ ] Update indexes and landing pages where module navigation is missing.
- [ ] Deduplicate repeated explanations and move shared material to the right parent doc.
- [ ] Tag historical or migration notes explicitly instead of mixing them into current docs.

### Deliverables

- A link-integrity report.
- A deduplication list with files that were merged, shortened, or retired.

### Exit Criteria

- Navigation paths are consistent.
- Duplicate documentation blocks have been collapsed or clearly scoped.

---

## Phase 6: Verification, Freeze, and Maintenance Loop

### Objective
Verify the revised docs and define the maintenance rhythm.

### Tasks

- [ ] Run local Python validation for missing cross-links and inconsistent section vocabulary.
- [ ] Spot-check a representative subset of modules, headers, tests, and benchmarks.
- [ ] Record unresolved gaps as follow-up work instead of leaving them implicit.
- [ ] Define the next maintenance sweep and ownership per area.
- [ ] Freeze the developer-doc structure once the first consolidation wave is accepted.

### Deliverables

- Verification summary.
- Maintenance backlog.
- Frozen developer-doc checklist for future updates.

### Exit Criteria

- The documentation set is internally consistent.
- Remaining work is explicitly tracked.

---

## Suggested Execution Order

1. Run the local Python inventory and export the baseline report.
2. Normalize `src/<module>/README.md` first.
3. Align `include/<module>/README.md` to the source docs.
4. Consolidate `tests/README.md`.
5. Consolidate `benchmarks/README.md`.
6. Repair cross-links and indexes.
7. Re-run validation and freeze the pattern.

---

## Recommended Python Checks

Use the local interpreter at `c:/Projects/ThemisDB/.venv/Scripts/python.exe`.

Suggested checks:

- Compare section headings across all `src/README.md` files.
- Report module docs missing canonical links.
- Detect `include/`, `tests/`, and `benchmarks/` docs missing purpose or usage sections.
- Produce a list of docs with stale or duplicate references.
