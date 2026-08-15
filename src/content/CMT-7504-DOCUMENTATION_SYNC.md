# CMT-7504: Module Documentation Linkset Synchronization
## Content Module Documentation Cross-Reference Validation

**Status:** Phase 2 Preparation (In Progress)  
**Scope:** Synchronize documentation linksets across ROADMAP, FUTURE_ENHANCEMENTS, README, and processor design docs  
**Date:** 2026-08-15  
**Target:** 100% consistency across all module documentation files

---

## Executive Summary

CMT-7504 ensures that the Content module documentation remains internally consistent and that all cross-references between key documents (ROADMAP.md, FUTURE_ENHANCEMENTS.md, README.md, and processor-specific design documents) are accurate and up-to-date.

**Key Metrics:**
- Documentation files in scope: 12 (ROADMAP, FUTURE_ENHANCEMENTS, README, ARCHITECTURE, SECURITY, PERFORMANCE_EXPECTATIONS, PRODUCTION_REQUIREMENTS, AUDIT, CHANGELOG, CONTENT_DEFERRED_FEATURES, MODULE_GAPS, MODULE_GAPS_BATCH5)
- Processor design documents: 44 (.cpp/.h files with Doxygen headers)
- Target: 100% cross-reference consistency
- Validation: Markdown link-check against anchors

---

## Phase 2: Core Implementation (CMT-7504-01 to 7504-04)

### CMT-7504-01: Update ROADMAP.md with Processor Inventory

**Status:** [ ] In Progress

**Current ROADMAP.md version:** 2026-07-29  
**Target update:** Add Batch 5 Phase 6B structure (COMPLETED)

**Tasks:**
- [x] Add Batch 5 header to "In Progress" section
- [x] Add Phase 6B subsection with CMT-7504/7505/7506 tasks
- [ ] Verify all 44 processor files listed in README.md are referenced
- [ ] Create linkset to CONTENT_DEFERRED_FEATURES.md

**Inventory Verification Checklist:**

| Component | File(s) | Referenced in ROADMAP | Status |
|-----------|---------|----------------------|--------|
| Core orchestration | content_manager.cpp, content_manager_llm.cpp, content_manager_embedding.cpp | [ ] Verify | [ ] |
| Validation/security | content_validator.cpp, content_policy.cpp, content_security.cpp | [ ] Verify | [ ] |
| Format processors | mime_detector.cpp, text_processor.cpp, pdf_processor.cpp, office_processor.cpp, html_processor.cpp, markdown_processor.cpp, image_processor.cpp, ocr_processor.cpp, audio_processor.cpp, stt_processor.cpp, tts_processor.cpp, video_processor.cpp, archive_processor.cpp | [ ] Verify | [ ] |
| Enrichment/dedup | deduplication_checker.cpp, embedding_pipeline.cpp | [ ] Verify | [ ] |
| Async/plugin | async_ingestion_worker.cpp, ingestion_plugin.cpp | [ ] Verify | [ ] |
| Adapters | (8 adapter files in adapters/ subdirectory) | [ ] Verify | [ ] |

**Acceptance:** All 44 processor files must have explicit or implicit mention in ROADMAP or linked inventory.

---

### CMT-7504-02: Update FUTURE_ENHANCEMENTS.md with Deferred Features

**Status:** [ ] In Progress

**Current FUTURE_ENHANCEMENTS.md version:** 2026-05-31 → 2026-08-15  
**Target update:** Add Batch 5 deferred features from CMT-7502 TODO scan (COMPLETED)

**Tasks:**
- [x] Add "Deferred Features from Batch 5" section
- [x] List 5 key deferred features (Optimization, Feature Flag, Vendor Integration, Archive, OCR)
- [ ] Cross-check against CONTENT_DEFERRED_FEATURES.md (ensure consistency)
- [ ] Verify GitHub issue references for each deferred item (if available)

**Deferred Features Verification:**

| Feature | Category | Target Release | Issue Ref | CONTENT_DEFERRED_FEATURES.md | Status |
|---------|----------|-----------------|-----------|------------------------------|--------|
| Linear→hash-table for patterns (abuse_detector) | Optimization | Q4 2026 | [ ] Link | [ ] Verify | [ ] |
| Geospatial distance filtering with CUDA | Feature Flag | Q4 2026 | [ ] Link | [ ] Verify | [ ] |
| Cloudflare Abuse Database API integration | Vendor Integration | Q4 2026 | [ ] Link | [ ] Verify | [ ] |
| Compression format expansion (ZIP/TAR→...) | Enhancement | Q1 2027 | [ ] Link | [ ] Verify | [ ] |
| Handwriting recognition for OCR | Enhancement | Q1 2027 | [ ] Link | [ ] Verify | [ ] |

**Acceptance:** All deferred features must be listed in both FUTURE_ENHANCEMENTS.md and CONTENT_DEFERRED_FEATURES.md with consistent target dates.

---

### CMT-7504-03: Cross-Check Phase Status Consistency

**Status:** [ ] In Progress

**Scope:** Verify that implementation phase status is consistent across all 4 documentation sources.

**Four Documentation Sources:**

1. **ROADMAP.md** — Primary roadmap with phase definitions
   - Phase 1: [x] Design / API Contract
   - Phase 2: [~] Core Implementation
   - Phase 3: [~] Error Handling and Edge Cases
   - Phase 4: [x] Tests
   - Phase 5: [x] Performance and Hardening
   - Phase 6: [x] Documentation and Acceptance
   - Phase 6B: [ ] Batch 5 — GA Documentation & Quality Gates

2. **FUTURE_ENHANCEMENTS.md** — Future scope and design constraints
   - Must reflect same phase model as ROADMAP
   - Must align on what's deferred vs. in-scope for v2.4.0 GA

3. **README.md** — Module overview and verified interfaces
   - Must list current processor inventory (44 files)
   - Must describe scope and limitations

4. **Processor Design Docs** — Distributed Doxygen headers in .cpp/.h files
   - Each file should have `@note Maturity:` indicator
   - Maturity status must align with ROADMAP phase markers

**Cross-Check Matrix:**

| Aspect | ROADMAP | FUTURE_ENHANCEMENTS | README | Processor Headers | Consistent? |
|--------|---------|---------------------|--------|-------------------|-------------|
| Phase status markers | 6B [ ] | aligned to Phase 5 | (not applicable) | (not applicable) | [ ] Verify |
| Processor inventory (44 files) | (implicit in Phase 2) | (implicit in constraints) | [x] Listed | [x] Headers present | [ ] Verify |
| Deferred features | (implicit in Phase 6B) | [x] Deferred Features section | (not applicable) | (implicit in TODOs) | [ ] Verify |
| Current maturity status | Phase 5-6 PASS | Reflected in constraints | Phase 1-6 complete | 🟢 PRODUCTION-READY (most files) | [ ] Verify |
| Error taxonomy | (Phase 1) | (constraints) | (not listed) | (partial in headers) | [ ] Verify |

**Acceptance:** All 4 sources must have consistent phase markers, processor inventory, and maturity status.

---

### CMT-7504-04: Add Automated Linkset Validation in CI

**Status:** [ ] In Progress

**Scope:** Prepare placeholder for automated markdown-link-check in CI workflow.

**Proposed Implementation:**

CI Step (to be added in Phase 4):
```yaml
# .github/workflows/doc-validation.yml (new or enhanced)
- name: Validate content module documentation links
  run: |
    cd src/content
    markdown-link-check *.md --config <config>
    # Verify anchors are consistent across ROADMAP/FUTURE_ENHANCEMENTS/README
```

**Files to Validate:**

| File | Anchors to Check | Cross-links | Status |
|------|------------------|------------|--------|
| ROADMAP.md | Phase markers, CMT task IDs | → FUTURE_ENHANCEMENTS, README, MODULE_GAPS_BATCH5 | [ ] Placeholder |
| FUTURE_ENHANCEMENTS.md | Section headings | → ROADMAP, README, CONTENT_DEFERRED_FEATURES | [ ] Placeholder |
| README.md | Processor list, verified surfaces | → ROADMAP, ARCHITECTURE, CMT docs | [ ] Placeholder |
| MODULE_GAPS_BATCH5.md | CMT-7504, CMT-7505, CMT-7506 task IDs | → GA_PROMOTION_SIGN_OFF.md | [ ] Placeholder |
| CONTENT_DEFERRED_FEATURES.md | Deferred feature list | → FUTURE_ENHANCEMENTS, ROADMAP | [ ] Placeholder |

**Acceptance:** CI placeholder created; full implementation deferred to Phase 4.

---

## Phase 3: Error Handling & Edge Cases (CMT-7504-03 Refined)

### Validation Hardens

**Status:** [ ] Pending Phase 3

**Tasks:**
- [ ] Run markdown-link-check locally against all `src/content/*.md` files
- [ ] Identify broken or missing anchors
- [ ] Cross-reference ROADMAP ↔ FUTURE_ENHANCEMENTS ↔ README ↔ processor headers
- [ ] Repair any inconsistencies found
- [ ] Document validation log

---

## Phase 4: Tests & Verification (CMT-7504-04 Extended)

### Automated Validation in CI

**Status:** [ ] Pending Phase 4

**Tasks:**
- [ ] Implement markdown-link-check in `.github/workflows/doc-validation.yml`
- [ ] Add anchor-consistency check to Doxygen build validation
- [ ] Verify all cross-references pass validation
- [ ] Document automated validation strategy

**Tests:** CMT-FIN-41..50 (doc link validation, anchor consistency, cross-reference integrity)

---

## Acceptance Criteria (Per Phase)

### Phase 2 Complete When:
- [x] ROADMAP.md Phase 6B section with CMT tasks added
- [x] FUTURE_ENHANCEMENTS.md Deferred Features section added
- [ ] Processor inventory cross-checked (44 files consistent)
- [ ] CI placeholder documented

### Phase 3 Complete When:
- [ ] Markdown-link-check executed; broken links identified and repaired
- [ ] Anchor consistency validated across all 4 docs
- [ ] No inconsistencies remain
- [ ] Validation log collected

### Phase 4 Complete When:
- [ ] CI check deployed and passing
- [ ] Doxygen anchor validation integrated
- [ ] All tests (CMT-FIN-41..50) passing
- [ ] Documentation linksets fully validated

---

## References

- **MODULE_GAPS_BATCH5.md** — CMT-7504 detailed task definitions
- **ROADMAP.md** — Content module roadmap with phase structure
- **FUTURE_ENHANCEMENTS.md** — Scope and deferred features
- **README.md** — Module overview and verified interfaces
- **CONTENT_DEFERRED_FEATURES.md** — Complete deferred features inventory
- **GA_PROMOTION_SIGN_OFF.md** — Gate closure tracking

---

**Phase 2 Status:** [ ] In Progress  
**Phase 3 Status:** [ ] Awaiting  
**Phase 4 Status:** [ ] Awaiting  

**Last Updated:** 2026-08-15  
**Owner:** CMT-7504 Implementation  
**Target Completion:** Sept 22, 2026 (v2.4.0 GA)
