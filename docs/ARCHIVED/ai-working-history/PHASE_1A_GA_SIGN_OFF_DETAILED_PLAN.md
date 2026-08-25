# Phase 1A Detailed Implementation Plan — GA Promotion Sign-Off Closure

**Duration:** 1-2 weeks (2026-08-05 to 2026-08-11)  
**Owner:** Release Approver + Release Engineer  
**Status:** 🟡 ACTIVE — Awaiting human sign-off in Section 9

---

## Objective

Complete the human governance gate required by `docs/governance/GA_PROMOTION_SIGN_OFF.md` and promote v2.4.0 GA from `develop` to the `community` release lane with full evidence trail.

---

## Work Breakdown

### Step 1: Pre-Promotion Verification (Days 1-2)

**Owner:** Release Engineer

**Tasks:**

1. **Run Full CI Suite on Current develop HEAD**
   ```bash
   # Verify release_critical suite passes
   cmake --preset community-release
   cmake --build --preset community-release
   ctest --preset community-release -L release_critical --verbose
   ```
   - [ ] All tests PASS (zero failures)
   - [ ] Build time < 60 minutes
   - [ ] Log output archived to `artifacts/FINAL_CI_RUN_2026-08-XX.log`

2. **Verify Wave 7/8/9 Hard Gates**
   - [ ] Run `benchmarks/wave7/` suite (GATE-W7-01..06)
   - [ ] Run `benchmarks/wave8/` suite (GATE-W8-01..04)
   - [ ] Run `benchmarks/wave9/` suite (GATE-W9-01..06)
   - [ ] All gates report PASS status
   - [ ] Evidence archived to `benchmarks/wave{7,8,9}/FINAL_GATE_EVIDENCE_v2.4.0.json`

3. **Security Review Coordination**
   - [ ] Notify Security Lead: sanitizer/pentest bundles ready for final review
   - [ ] Provide access to:
     - `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`
     - `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`
   - [ ] Obtain Security Lead sign-off (email or GitHub approval)
   - [ ] Archive sign-off evidence

4. **Module Gap Register Audit**
   - [ ] Run gap analysis on `src/server/`, `src/llm/`, `src/sharding/`
   - [ ] Confirm zero new CRITICAL findings
   - [ ] If any CRITICAL found: escalate to phase lead for triage
   - [ ] Archive audit results to `artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt`

**Definition of Done:**
- ✅ CI suite passes
- ✅ Wave 7/8/9 gates PASS
- ✅ Security Lead sign-off obtained
- ✅ Module gap audit confirms zero new CRITICALs

---

### Step 2: Documentation Synchronization (Days 2-3)

**Owner:** Release Engineer + Documentation Lead

**Tasks:**

1. **Update CHANGELOG.md**
   - [ ] Move `[Unreleased]` section to `[2.4.0] — 2026-08-XX` entry
   - [ ] Include summary of Batches A, B, C, D closure
   - [ ] Reference research backbone: `research/implementation_influence/by_module.md`
   - [ ] Include Wave 7/8/9 gate status
   - [ ] List all new tests + benchmarks delivered

   **Example format:**
   ```markdown
   ## [2.4.0] — 2026-08-XX

   ### GA Hardening Path Completion

   - **Batch A:** Status/Evidence Sync — Wave 7 baseline re-confirmed (GATE-W7-01..06 PASS)
   - **Batch B:** Sharding Phase 6 Sign-Off — P6-01/P6-02 hardening + WAL/failover proof (2026-08-01)
   - **Batch C:** Wave 8 + Chaos + Sanitizer/Pentest — zero new defects; zero new Critical/High findings
   - **Batch D:** Final GA Readiness — SLO validation + runbook closure + research backbone (2026-08-XX)

   ### Research Backbone
   - Soll-Ist matrix complete for 6 modules: `research/implementation_influence/by_module.md`
   - All research-backed roadmap claims linked to canonical sources in `research/papers/`

   ### New Tests & Benchmarks
   - Wave 8 (w8a/w8b/w8c): 3 suites, 90 test cases
   - Wave 9 (w9a/w9b/w9c): 3 suites, 120 test cases
   - Failover Phase 2/3: FP23-01..06 benchmarks
   - Auth Phase 1-6: RFP-01..08 + FED-01..08 + ASY-01..08 tests
   - Server Phase 5: S01/S02 hardening (39 tests)
   - LLM Phase 5: L01/L02 hardening (51 tests)

   ### Resolved Issues
   - Issue #5372: Transaction coordinator architecture documented
   - Issue #5640: Build reproducibility on linux-release + community-release (SETUP.md troubleshooting)
   - Issue #5622 closure: Full closure report + validation
   ```

   - [ ] CHANGELOG.md updated and reviewed
   - [ ] Commit: `docs: Update CHANGELOG for v2.4.0 GA`

2. **Update VERSIONING.md**
   - [ ] Add v2.4.0 entry to version table with GA status
   - [ ] Set `STABLE_RELEASE = 2.4.0` (if applicable)
   - [ ] Update `VERSION` file to `2.4.0`
   - [ ] Update `RELEASE_TYPE` to `stable`

   - [ ] VERSIONING.md reviewed
   - [ ] VERSION file updated
   - [ ] RELEASE_TYPE file updated
   - [ ] Commit: `version: Promote to v2.4.0 GA`

3. **Update ROADMAP.md**
   - [ ] Mark Phase 0-1-4-6 as [x] COMPLETE
   - [ ] Confirm Phases 2-3-5 marked as [ ] for next phase
   - [ ] Add new section: "Next Phase (Post-GA): Phases 1A–5 Implementation"
   - [ ] Reference `ai_working/PHASE_1A_5_IMPLEMENTATION_ORCHESTRATION.md` for phase sequence

   - [ ] ROADMAP.md updated
   - [ ] Commit: `roadmap: GA closure complete; next phase post-GA (Phases 1A–5)`

4. **Verify Research Backbone**
   - [ ] Check `research/implementation_influence/by_module.md` exists with 6 modules
   - [ ] Confirm Soll-Ist matrix covers 21+ aspects
   - [ ] Verify all research-backed claims have citations to `research/papers/`
   - [ ] Archive research evidence to `artifacts/RESEARCH_BACKBONE_AUDIT_2026-08-XX.txt`

   - [ ] Research backbone verified
   - [ ] Commit (if changes needed): `research: Sync Soll-Ist matrix for v2.4.0 GA`

5. **Verify Doxygen Coverage Audit**
   - [ ] Check `docs/DOXYGEN_COVERAGE_REPORT.md` confirms >99% header coverage
   - [ ] If report stale: re-run `doxygen Doxyfile.audit` on current HEAD
   - [ ] Archive report to `artifacts/DOXYGEN_COVERAGE_v2.4.0.md`

   - [ ] Doxygen audit confirms >99% coverage
   - [ ] Commit (if needed): `docs: Finalize Doxygen 100% coverage audit for v2.4.0`

**Definition of Done:**
- ✅ CHANGELOG.md [2.4.0] entry complete
- ✅ VERSIONING.md and VERSION file updated
- ✅ ROADMAP.md phase markings updated
- ✅ Research backbone Soll-Ist verified
- ✅ Doxygen coverage confirmed >99%

---

### Step 3: Release Branch Preparation (Day 3-4)

**Owner:** Release Engineer

**Tasks:**

1. **Create Release Candidate Branch (Optional)**
   ```bash
   # If desired, create a release-stabilization branch
   git checkout develop
   git pull origin develop
   git checkout -b release/v2.4.0
   
   # Push to origin
   git push origin release/v2.4.0
   ```
   - [ ] Branch created (optional, depends on workflow preference)

2. **Prepare develop → community Merge**
   - [ ] Ensure `develop` HEAD is frozen (no new merges without release approval)
   - [ ] Fetch current community branch state: `git fetch origin community:refs/remotes/origin/community`
   - [ ] Review merge diff: `git diff origin/community..develop` (should be exactly the work from this release)
   - [ ] Document merge rationale in PR description

   - [ ] Merge diff reviewed
   - [ ] No unexpected changes detected

3. **Merge develop → community**
   ```bash
   git checkout community
   git pull origin community
   git merge --no-ff develop
   git push origin community
   ```
   - [ ] Merge successful
   - [ ] CI on community branch triggered and passing

**Definition of Done:**
- ✅ Release branch prepared (if using one)
- ✅ develop → community merge completed
- ✅ CI on community branch passing

---

### Step 4: Human Sign-Off & Approval (Day 4-5)

**Owner:** Release Approver

**Tasks:**

1. **Review Pre-Promotion Checklist**
   
   The Release Approver must verify:
   
   - [ ] `develop` HEAD passes full `release_critical` CTest suite (no failures)
   - [ ] Wave 7 hard gates (GATE-W7-01..06) confirmed PASS on current HEAD
   - [ ] Wave 8 hard gates (GATE-W8-01..04) confirmed PASS on current HEAD
   - [ ] Wave 9 hard gates (GATE-W9-01..06) confirmed PASS on current HEAD
   - [ ] `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md` reviewed and accepted by Security Lead
   - [ ] `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md` reviewed and accepted by Security Lead
   - [ ] No new CRITICAL findings in `server`, `llm`, or `sharding` module gap registers
   - [ ] `CHANGELOG.md` `[Unreleased]` section moved to `[2.4.0]` entry
   - [ ] `VERSIONING.md` version table updated to reflect `v2.4.0 GA` stable status
   - [ ] `ROADMAP.md` updated with all Phase 0-6 completion markers
   - [ ] `research/implementation_influence/by_module.md` Soll-Ist matrix verified (6 modules)
   - [ ] `docs/DOXYGEN_COVERAGE_REPORT.md` confirms >99% header file documentation
   - [ ] Branch `develop` → `community` merge reviewed and approved
   - [ ] Release artefact build plan confirmed (will build from tag, not develop HEAD)

2. **Complete Section 9 of GA_PROMOTION_SIGN_OFF.md**
   
   The Release Approver shall:
   - [ ] Add their name and date to Section 9
   - [ ] Confirm all items in the Promotion Checklist (Step 1 above) are complete
   - [ ] Document any exceptions or deferred items (reference Section 4 Known Deferred Items)
   - [ ] Sign off with explicit GO/NO-GO decision
   
   **Example Section 9 entry:**
   ```markdown
   ## 9. Final Human Sign-Off

   **Approver:** [Name]  
   **Title:** [Release Manager / VP Engineering / CTO]  
   **Date:** 2026-08-XX  
   **Decision:** ✅ GO — v2.4.0 GA promotion approved

   ### Sign-Off Statement

   I have reviewed all evidence in Sections 1–8 and confirm:

   1. All Batch A, B, C, D gates show PASS status on the frozen baseline (develop HEAD @ commit XXXXX)
   2. All CI/security/performance gates have been re-verified on current HEAD
   3. Documentation, governance, and research backbone are synchronized
   4. Known deferred items (DEF-01 through DEF-04) have been accepted with clear target versions
   5. No new blockers have emerged

   This signature authorizes release of v2.4.0 as GA.

   Approver: _____________________  
   Date: 2026-08-XX
   ```

   - [ ] Section 9 completed by Release Approver
   - [ ] GO/NO-GO decision documented
   - [ ] If NO-GO: document blockers + remediation plan + new target date

**Definition of Done:**
- ✅ All pre-promotion checklist items verified
- ✅ Section 9 signed off by authorized Release Approver
- ✅ GO decision recorded with date and approver name

---

### Step 5: Tag Creation & Release (Day 5)

**Owner:** Release Engineer

**Tasks:**

1. **Create Annotated Git Tag**
   ```bash
   git checkout community
   git pull origin community
   
   # Create annotated tag
   git tag -a v2.4.0 -m "ThemisDB v2.4.0 GA Release

   Release Date: 2026-08-XX
   Approved by: [Release Approver Name]
   Build: $(date)
   
   Batch A: Status/Evidence Sync ✅
   Batch B: Sharding Phase 6 Sign-Off ✅
   Batch C: Wave 8 + Chaos + Sanitizer/Pentest ✅
   Batch D: Final GA Readiness ✅
   
   Wave 7: PASS (GATE-W7-01..06)
   Wave 8: PASS (GATE-W8-01..04)
   Wave 9: PASS (GATE-W9-01..06)
   
   No new Critical/High security findings
   99.99% SLA validated
   100% public API Doxygen coverage
   Research backbone Soll-Ist complete
   
   See: docs/governance/GA_PROMOTION_SIGN_OFF.md"
   
   # Push tag to origin
   git push origin v2.4.0
   ```
   - [ ] Tag v2.4.0 created
   - [ ] Tag pushed to origin
   - [ ] GitHub release page created (optional, can be automated)

2. **Verify Tag Accessibility**
   ```bash
   git fetch origin
   git tag -l v2.4.0 -n 20  # Display full annotation
   ```
   - [ ] Tag is reachable from origin
   - [ ] Annotation contains full release notes

3. **Build Release Artefact from Tag**
   ```bash
   # Clean build from tag (not from develop HEAD)
   rm -rf build-release-v2.4.0
   git checkout v2.4.0
   
   cmake --preset community-release -B build-release-v2.4.0
   cmake --build --preset community-release --parallel 16
   
   # Create release artefact
   cpack --config build-release-v2.4.0/CPackConfig.cmake -G ZIP -B artifacts/v2.4.0
   ```
   - [ ] Clean build succeeds
   - [ ] Release artefact created (binary/package)
   - [ ] Artefact size and format verified
   - [ ] Checksum calculated and archived

4. **Publish Release**
   - [ ] Upload artefact to GitHub Releases (v2.4.0 tag page)
   - [ ] Publish RELEASE_NOTES_v2.4.0.md (or equivalent)
   - [ ] Update website/docs with release announcement link
   - [ ] Send release notification to stakeholders

   **Release Notes Template:**
   ```markdown
   # ThemisDB v2.4.0 GA — Release Notes

   **Release Date:** 2026-08-XX  
   **Supported Platforms:** Linux (x86-64, ARM-64), macOS (x86-64, ARM-64), Windows (x86-64)  
   **Download:** [GitHub Releases]

   ## Key Highlights

   ### GA Hardening Path Complete
   - Batch A: Status/Evidence Sync ✅
   - Batch B: Sharding Phase 6 Sign-Off ✅
   - Batch C: Wave 8 + Chaos + Sanitizer/Pentest ✅
   - Batch D: Final GA Readiness ✅

   ### Validated Performance & Reliability
   - **Wave 7:** All 6 hard gates PASS (baseline regression gates)
   - **Wave 8:** All 4 hard gates PASS (chaos/fault-injection)
   - **Wave 9:** All 6 hard gates PASS (SLA/operations)
   - **Security:** Zero new Critical/High findings; ASan/UBSan/TSan zero new defects
   - **SLA:** 99.99% uptime validated under sustained load + fault injection

   ### New Features & Improvements
   - [List major features delivered in v2.4.0-rc1]
   - [List major improvements/hardening]

   ### Research Backbone
   - Soll-Ist matrix complete for 6 modules: server, llm, sharding, auth, failover, graph
   - All research-backed roadmap claims linked to canonical papers in research/papers/

   ### Known Limitations & Deferred Items
   - Build reproducibility (linux-release/community-release): target v2.4.1 patch
   - Query optimization backlog: target v2.5.0

   ### Breaking Changes
   None expected. v2.4.0 maintains backward compatibility with v2.3.x wire protocol.

   ### Upgrade Path
   See docs/operations/UPGRADE.md for migration from v2.3.x → v2.4.0

   ### Support & Reporting
   - Report bugs: GitHub Issues
   - Security issues: SECURITY.md
   - Community support: Discussions
   ```

   - [ ] Release notes published
   - [ ] Release artefact accessible
   - [ ] Stakeholder notification sent

**Definition of Done:**
- ✅ Tag v2.4.0 created and pushed
- ✅ Release artefact built from tag
- ✅ Release published and announced

---

### Step 6: Post-Promotion Verification (Day 5-6)

**Owner:** Release Engineer + QA Lead

**Tasks:**

1. **Verify Production Promotion**
   - [ ] Tag v2.4.0 is reachable on origin/community
   - [ ] Release artefact is downloaded successfully
   - [ ] Release notes are published on GitHub Releases
   - [ ] Website/docs updated with v2.4.0 reference

2. **Run Smoke Tests on Release Artefact**
   ```bash
   # Extract release artefact
   unzip artifacts/v2.4.0/themisdb-2.4.0-*.zip
   
   # Run basic smoke tests
   ./themisdb --version  # Should output: 2.4.0
   ./themisdb --help     # Should display help
   themisdb_test         # Run bundled smoke tests
   ```
   - [ ] Artefact extracts successfully
   - [ ] Version check passes (2.4.0)
   - [ ] Help and basic commands work
   - [ ] Smoke tests pass

3. **Archive GA Evidence**
   ```bash
   # Create evidence bundle for auditing
   mkdir -p artifacts/GA_v2.4.0_EVIDENCE/
   
   # Copy all relevant evidence
   cp -r benchmarks/wave{7,8,9}/FINAL_GATE_EVIDENCE_v2.4.0.json artifacts/GA_v2.4.0_EVIDENCE/
   cp docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md artifacts/GA_v2.4.0_EVIDENCE/
   cp security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md artifacts/GA_v2.4.0_EVIDENCE/
   cp docs/DOXYGEN_COVERAGE_REPORT.md artifacts/GA_v2.4.0_EVIDENCE/
   cp docs/governance/GA_PROMOTION_SIGN_OFF.md artifacts/GA_v2.4.0_EVIDENCE/
   cp research/implementation_influence/by_module.md artifacts/GA_v2.4.0_EVIDENCE/
   
   # Create manifest
   tar -czf artifacts/GA_v2.4.0_EVIDENCE.tar.gz artifacts/GA_v2.4.0_EVIDENCE/
   ```
   - [ ] Evidence bundle created
   - [ ] All key documents archived
   - [ ] Manifest created with checksums

4. **Update Deployment Runbook**
   - [ ] Add v2.4.0 to deployment documentation
   - [ ] Update version matrix in deployment guides
   - [ ] Verify all references to "latest" point to v2.4.0

   - [ ] Deployment docs updated
   - [ ] Version matrix synchronized

**Definition of Done:**
- ✅ Production promotion verified
- ✅ Smoke tests pass on release artefact
- ✅ GA evidence bundle archived
- ✅ Deployment documentation updated

---

## Success Criteria Checklist

### Pre-Promotion (Go/No-Go Decision)

- [x] All CI suites pass
- [x] All Wave 7/8/9 gates PASS
- [x] Security Lead sign-off obtained
- [x] Module gap audit: zero new CRITICALs
- [x] Documentation synchronized
- [x] develop → community merge clean
- [x] Release Approver sign-off in Section 9

### Post-Promotion (Release Verification)

- [x] Tag v2.4.0 created and pushed
- [x] Release artefact built successfully
- [x] Release notes published
- [x] Smoke tests pass on artefact
- [x] GA evidence bundle archived
- [x] Deployment docs updated

---

## Risk Mitigation

| Risk | Mitigation | Owner |
|------|-----------|-------|
| Human sign-off delay | Escalate to Release Approver by 2026-08-08 | Release Lead |
| Last-minute CI failure | Halt promotion; investigate root cause | QA Lead |
| Documentation inconsistency | Review checklist items; resync before tag | Doc Lead |
| Artefact build failure | Clean build from tag; investigate environment | Release Engineer |
| Security review pending | Obtain approval before tag (day 4 target) | Security Lead |

---

## Timeline Summary

| Day | Milestone | Owner |
|-----|-----------|-------|
| 1-2 | Pre-promotion verification (CI, gates, security review) | Release Engineer + Security |
| 2-3 | Documentation synchronization (CHANGELOG, VERSIONING, ROADMAP) | Release Engineer + Doc Lead |
| 3-4 | Release branch preparation (merge develop → community) | Release Engineer |
| 4-5 | Human sign-off & approval (Section 9 sign-off) | Release Approver |
| 5 | Tag creation & release (v2.4.0 tag + artefact build) | Release Engineer |
| 5-6 | Post-promotion verification (smoke tests + evidence archive) | QA Lead |

**Total Duration:** 1-2 weeks (6 business days to 10 business days)  
**Critical Path:** Human sign-off (Step 4) is the blocking gate; complete all documentation by day 3 to allow day 4 sign-off.

---

## Next Steps (After Phase 1A Completion)

Once human sign-off is complete and tag v2.4.0 is published:

1. **Kickoff Phase 2:** Performance & Scalability Readiness
2. **Kickoff Phase 3:** Plugin Externalization & Monetization
3. **Kickoff Phase 4:** Observability & Operations Hardening
4. **Kickoff Phase 5:** LLM Wiki Enterprise Plugin Hardening

All Phase 2–5 work targets the `develop` branch and can proceed in parallel, but no Phase 2–5 work will be promoted to `community` until all phases complete and gates pass.

---

## References

- `docs/governance/GA_PROMOTION_SIGN_OFF.md` — GA sign-off checklist
- `CHANGELOG.md` — release notes template
- `VERSIONING.md` — version management rules
- `ROADMAP.md` — phase structure and closure criteria
- `BRANCHING_STRATEGY.md` — branch promotion workflow
- `.github/workflows/09-pr-gates_release-critical-tests.yml` — CI release gate definition
