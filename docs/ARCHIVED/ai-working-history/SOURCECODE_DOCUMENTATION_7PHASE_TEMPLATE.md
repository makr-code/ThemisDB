# Source Code Documentation Template (7 Phases)

**Purpose:** Generate or refresh documentation for source code modules, public APIs, implementation details, and operational behavior.

**Audience:** AI agents, maintainers, reviewers, and developers who need accurate, source-backed documentation.

**Rule:** Document only what is supported by the codebase, tests, and build configuration. Do not invent behavior.

---

## Input Contract

- Module or subsystem:
- Target source files:
- Target symbols or APIs:
- Documentation formats required:
- Intended audience:
- Source of truth files:
- Out-of-scope areas:
- Required reviewer:
- Compatibility or version constraints:

---

## Phase 0: Pre-Start Validation & Scope Lock

### Entry Criteria
- [ ] Target files and symbols identified
- [ ] Existing documentation located
- [ ] Build and doc tooling available
- [ ] Scope is limited to documentation, comments, and linked artifacts unless a code comment change is required for correctness

### Tasks
- [ ] Read the target headers, source files, and tests
- [ ] Confirm the public API surface and ownership boundaries
- [ ] Identify any generated files, build flags, or platform gates that affect the docs
- [ ] Confirm the exact doc outputs required: Doxygen, markdown guide, module README, migration note, troubleshooting note, or architecture note
- [ ] Record open questions before drafting

### Acceptance Criteria
- [ ] Scope is unambiguous
- [ ] Source of truth is documented
- [ ] No undocumented assumptions remain

### Exit Criteria
- [ ] PASS: proceed to Phase 1
- [ ] FAIL: stop and resolve missing inputs

---

## Phase 1: Source Audit & Documentation Inventory

### Entry Criteria
- [ ] Phase 0 completed

### Tasks
- [ ] Inventory all public classes, functions, methods, constants, and configuration objects
- [ ] Map each symbol to its declaration, definition, and main call sites
- [ ] Identify existing comments, README sections, and Doxygen blocks
- [ ] Capture error paths, null/empty handling, threading constraints, and ownership rules
- [ ] Note versioned or platform-specific behavior
- [ ] Collect examples from tests or existing usage sites

### Deliverables
- [ ] Documentation inventory table with columns: file, symbol, current doc state, required doc type, risk, reviewer notes
- [ ] List of gaps grouped by category: API reference, behavior, examples, limitations, migration, troubleshooting

### Acceptance Criteria
- [ ] Every target symbol is accounted for
- [ ] Gaps are grouped and prioritized
- [ ] No unsupported behavior has been inferred

### Exit Criteria
- [ ] PASS: proceed to Phase 2
- [ ] NEEDS REVIEW: unresolved gaps or ambiguous behavior

---

## Phase 2: Documentation Plan & Outline

### Entry Criteria
- [ ] Inventory complete

### Tasks
- [ ] Define the final artifact set and the order in which it will be produced
- [ ] Assign one doc owner per artifact and one source file set per section
- [ ] Draft the section outline for each artifact
- [ ] Decide the terminology that will be used consistently across docs
- [ ] Decide where examples should come from and which ones need validation
- [ ] Mark any unresolved items that require human confirmation

### Required Planning Output

| Artifact | Purpose | Primary Source Files | Validation Method |
|---|---|---|---|
| API reference | Document public behavior and contracts |  |  |
| Module guide | Explain how the subsystem works |  |  |
| Usage example | Show supported calling patterns |  |  |
| Troubleshooting note | Capture known failures and fixes |  |  |
| Migration note | Explain breaking changes or replacements |  |  |

### Acceptance Criteria
- [ ] Every artifact has an outline
- [ ] Every outline has a source of truth
- [ ] Examples and edge cases are planned

### Exit Criteria
- [ ] PASS: proceed to Phase 3
- [ ] NEEDS REVIEW: outline is incomplete or conflicts with code

---

## Phase 3: Draft Documentation Authoring

### Entry Criteria
- [ ] Phase 2 approved

### Tasks
- [ ] Write the initial documentation draft for each artifact
- [ ] For public APIs, document purpose, parameters, return values, failure modes, ownership, and lifetime rules
- [ ] For modules, document responsibilities, dependencies, data flow, and extension points
- [ ] For behavior notes, document edge cases, platform differences, and known limitations
- [ ] Add or refresh Doxygen comments for changed public C++ APIs
- [ ] Add examples that match real code paths and real naming
- [ ] Mark any uncertain behavior clearly as unresolved instead of guessing

### Required Content Blocks

#### API Reference Block
- Purpose:
- Parameters / inputs:
- Return behavior:
- Error handling:
- Ownership / lifetime:
- Thread-safety:
- Performance characteristics:
- Security or validation notes:

#### Module Guide Block
- Module responsibility:
- Key files:
- Main data flow:
- External dependencies:
- Extension points:
- Configuration / feature flags:
- Known limitations:

#### Troubleshooting Block
- Symptom:
- Likely cause:
- Verification steps:
- Recommended fix:
- When to escalate:

### Acceptance Criteria
- [ ] Drafts cover every in-scope artifact
- [ ] No placeholder claims remain
- [ ] Examples are consistent with the codebase
- [ ] Doxygen is present for every changed public C++ API

### Exit Criteria
- [ ] PASS: proceed to Phase 4
- [ ] PARTIAL: draft exists but needs technical verification

---

## Phase 4: Technical Verification

### Entry Criteria
- [ ] Drafts written

### Tasks
- [ ] Build the affected targets if documentation comments touched compiled code
- [ ] Run the doc generator or markdown validator used by the repository
- [ ] Check links, references, anchors, and cross references
- [ ] Verify code snippets compile or match the documented APIs where applicable
- [ ] Confirm no stale symbol names, file paths, or version claims remain
- [ ] Validate that limitation statements match the tests and runtime behavior

### Acceptance Criteria
- [ ] Documentation builds cleanly
- [ ] Links and references resolve
- [ ] Snippets are correct
- [ ] No stale behavior claims remain

### Exit Criteria
- [ ] PASS: proceed to Phase 5
- [ ] FAIL: return to Phase 3 with concrete corrections

---

## Phase 5: Human Review & Polishing

### Entry Criteria
- [ ] Technical verification passed

### Review Checklist
- [ ] Documentation is accurate and source-backed
- [ ] Scope matches the requested module or API surface
- [ ] Error cases and edge cases are explicit
- [ ] Ownership, lifetime, and threading behavior are documented where relevant
- [ ] Examples are minimal, correct, and representative
- [ ] Terminology is consistent across all docs
- [ ] No undocumented compatibility or migration risk remains

### Acceptance Criteria
- [ ] Reviewer approves the content
- [ ] Any corrections are applied
- [ ] Final wording is clear and concise

### Exit Criteria
- [ ] APPROVED: proceed to Phase 6
- [ ] CHANGES REQUESTED: revise and return to Phase 3 or Phase 4 as needed

---

## Phase 6: Publish, Sync & Maintenance

### Entry Criteria
- [ ] Review approved

### Tasks
- [ ] Publish the final documentation to the target location
- [ ] Update the relevant README, index, or module navigation entry
- [ ] Add changelog or migration notes if the docs reflect a behavioral or API change
- [ ] Link follow-up tasks for any unresolved documentation gaps
- [ ] Record ownership for future updates

### Deliverables
- [ ] Final documentation files
- [ ] Updated index or README entries
- [ ] Follow-up issue list for open questions
- [ ] Maintenance note for future re-scans

### Acceptance Criteria
- [ ] Docs are discoverable from the project navigation
- [ ] Docs stay aligned with the source of truth
- [ ] Follow-up work is tracked

### Exit Criteria
- [ ] COMPLETE: documentation is published and ready for maintenance

---

## Output Template

Use this structure for a module-level documentation deliverable.

### 1. Overview
- What the module does
- Why it exists
- Who should read this

### 2. Public Surface
- Key files
- Key symbols
- Supported entry points

### 3. Behavior
- Normal flow
- Error flow
- Edge cases
- Platform or build-flag differences

### 4. Contracts
- Input expectations
- Output guarantees
- Ownership and lifetime
- Threading and concurrency
- Security and validation

### 5. Examples
- Minimal example
- Realistic example
- Failure example, if useful

### 6. Limitations
- Known constraints
- Unsupported paths
- Future work

### 7. Verification
- Tests used
- Build or doc commands used
- Reviewer notes

---

## Fill-In Prompt for AI Agents

Generate documentation for the following source-code scope:

- Module:
- Target files:
- Target symbols:
- Documentation artifact types:
- Intended audience:
- Required language and style:
- Known constraints:
- Relevant tests:
- Open questions:

The final output must be consistent with the codebase, should not invent behavior, and must include explicit notes for edge cases, ownership, and failure modes.