# Public Documentation Template (7 Phases)

**Purpose:** Create or refresh user-facing, admin-facing, and operational documentation in `docs/` with a repeatable workflow and a consistent final structure.

**Audience:** Users, admins, maintainers, and reviewers who need clear, source-backed documentation.

**Rule:** Keep claims aligned with the product behavior, the source-of-truth docs, and the current release state. Do not copy internal implementation details unless they are necessary to use or operate the system.

---

## Input Contract

- Document type:
- Target path under `docs/`:
- Audience:
- Product area:
- Source of truth:
- Required screenshots, tables, or examples:
- Localization or terminology constraints:
- Review owner:
- Publication target:

---

## Phase 0: Scope Lock & Audience Check

### Entry Criteria
- [ ] Audience is known
- [ ] Document type is known
- [ ] Source of truth is identified
- [ ] Target location under `docs/` is chosen

### Tasks
- [ ] Determine whether the document is user, admin, analysis, tutorial, or operational content
- [ ] Identify the canonical source material and any related source-code or research references
- [ ] Confirm whether the document is public, internal, or mixed audience
- [ ] Define what the document must not cover

### Acceptance Criteria
- [ ] Audience and scope are explicit
- [ ] Source of truth is listed
- [ ] No ambiguous target location remains

### Exit Criteria
- [ ] PASS: proceed to Phase 1
- [ ] FAIL: resolve the missing input before drafting

---

## Phase 1: Content Inventory & Source Harvesting

### Entry Criteria
- [ ] Scope locked

### Tasks
- [ ] Collect the existing pages, guides, or notes that cover the same topic
- [ ] Capture the product behavior that must be explained
- [ ] Capture any operational constraints, setup steps, or prerequisites
- [ ] Collect examples, commands, and troubleshooting cases from real usage
- [ ] Note links to source-code files, research notes, or issue trackers when they are part of the narrative

### Deliverables
- [ ] Source inventory table with columns: item, location, relevance, confidence, and reuse decision
- [ ] Gap list of missing explanations, missing examples, and stale statements

### Acceptance Criteria
- [ ] All relevant source material is inventoried
- [ ] Gaps are grouped and prioritized
- [ ] No unsupported claims have been added

### Exit Criteria
- [ ] PASS: proceed to Phase 2
- [ ] NEEDS REVIEW: unresolved source conflicts or missing behavior details

---

## Phase 2: Information Architecture & Outline

### Entry Criteria
- [ ] Inventory complete

### Tasks
- [ ] Decide the final page structure and headings
- [ ] Choose the preferred terminology and voice
- [ ] Decide which examples are mandatory and which are optional
- [ ] Define navigation links, cross references, and parent index entries
- [ ] Mark content that requires screenshots, tables, or code snippets
- [ ] Record open questions that must be answered before drafting

### Required Planning Output

| Section | Purpose | Source of Truth | Validation Method |
|---|---|---|---|
| TL;DR / overview | Fast orientation |  |  |
| Prerequisites | Setup and access requirements |  |  |
| Procedure / usage | Step-by-step guidance |  |  |
| Troubleshooting | Known issues and fixes |  |  |
| Admin notes | Operational constraints |  |  |
| Links / references | Navigation and traceability |  |  |

### Acceptance Criteria
- [ ] The page outline is complete
- [ ] The navigation strategy is defined
- [ ] The doc type matches the audience

### Exit Criteria
- [ ] PASS: proceed to Phase 3
- [ ] NEEDS REVIEW: outline does not match the audience or source material

---

## Phase 3: Draft Authoring

### Entry Criteria
- [ ] Phase 2 approved

### Tasks
- [ ] Write the first full draft using the approved outline
- [ ] Keep language concrete and user-oriented
- [ ] Explain prerequisites, setup, and the expected result clearly
- [ ] Add examples that can be reproduced by the reader
- [ ] Include troubleshooting notes for common failure paths
- [ ] Add explicit links to source documentation and related modules where appropriate
- [ ] Mark any unresolved issue as unresolved instead of guessing

### Required Content Blocks

#### Overview Block
- What this page is for
- Who should use it
- What problem it solves

#### Usage Block
- Preconditions
- Steps
- Expected outcome

#### Admin / Operations Block
- Operational constraints
- Monitoring or maintenance notes
- Failure handling

#### Troubleshooting Block
- Symptom
- Likely cause
- How to verify
- How to fix

### Acceptance Criteria
- [ ] Draft covers all planned sections
- [ ] Examples are reproducible
- [ ] No unsupported behavior is described
- [ ] Links point to real locations

### Exit Criteria
- [ ] PASS: proceed to Phase 4
- [ ] PARTIAL: draft exists but needs verification

---

## Phase 4: Source, Link, and Rendering Verification

### Entry Criteria
- [ ] Draft complete

### Tasks
- [ ] Verify every internal link and anchor
- [ ] Verify any referenced file path exists
- [ ] Check that examples match current behavior
- [ ] Check markdown rendering and table formatting
- [ ] Verify there are no stale version numbers or retired feature names
- [ ] Validate screenshots or images if included

### Acceptance Criteria
- [ ] Links resolve
- [ ] Rendering is clean
- [ ] Examples are current
- [ ] No stale product claims remain

### Exit Criteria
- [ ] PASS: proceed to Phase 5
- [ ] FAIL: return to Phase 3 with concrete fixes

---

## Phase 5: Editorial Review & Polish

### Entry Criteria
- [ ] Verification passed

### Review Checklist
- [ ] The document is clear for the intended audience
- [ ] The order of sections supports the intended workflow
- [ ] The tone is consistent and concise
- [ ] The document avoids internal-only jargon unless defined
- [ ] Safety, permissions, and operational limits are explicit where relevant
- [ ] The document does not overstate guarantees

### Acceptance Criteria
- [ ] Reviewer approves the content
- [ ] Required revisions are applied
- [ ] The final version is readable without codebase context

### Exit Criteria
- [ ] APPROVED: proceed to Phase 6
- [ ] CHANGES REQUESTED: revise and re-verify as needed

---

## Phase 6: Publish, Index, and Maintain

### Entry Criteria
- [ ] Review approved

### Tasks
- [ ] Publish the document in the target location
- [ ] Update any index, sidebar, landing page, or hub page
- [ ] Update related README or tutorial links
- [ ] Record follow-up items for gaps that could not be resolved
- [ ] Note ownership and review cadence

### Deliverables
- [ ] Final document in `docs/`
- [ ] Updated navigation entry or index link
- [ ] Follow-up item list for unresolved content gaps

### Acceptance Criteria
- [ ] The page is discoverable
- [ ] The page is linked from the correct index
- [ ] Maintenance ownership is clear

### Exit Criteria
- [ ] COMPLETE: published and ready for ongoing maintenance

---

## Output Template

Use this structure for a public-facing document.

### 1. Title and Status
- Title
- Status
- Audience
- Date
- Owner

### 2. TL;DR
- Three sentences maximum

### 3. Context
- Problem or need
- Goal
- Non-goals

### 4. Prerequisites
- Required access
- Required version or environment
- Known constraints

### 5. Procedure or Content
- Main steps, explanation, or guidance
- Tables and examples if needed

### 6. Troubleshooting
- Symptoms
- Causes
- Fixes

### 7. References
- Source-of-truth links
- Related docs
- Related issues or research notes
