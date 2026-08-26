# Release Promotion Gate Policy

**Document:** ThemisDB Release Quality Gates  
**Status:** Phase 3 Enforcement (2026-Q4)  
**Version:** 1.0  
**Owner:** Platform Release / @makr-code  
**Last Updated:** 2026-08-10

---

## Purpose

This document establishes binding, non-waiverable quality gates that control promotion of code from development (`develop`) to release lanes (`community`, `military`, `hyperscaler`). The policy defines three tiers of gates with escalating enforcement rigor.

**Principle:** *No release-lane merge without verifiable evidence of quality conformance.*

---

## Gate Hierarchy

### Tier 0: Hard Block (Non-Waiverable)

**Definition:** Merge to `community`, `military`, or `hyperscaler` is **completely blocked** until these gates PASS.

**Gates:**

1. **Performance Baseline Gates (W7-01 through W7-06)**
   - **Requirement:** All Wave 7 benchmark gates PASS
   - **Evidence:** `benchmarks/wave7/release_gate_manifest_w7.json` with latest execution results
   - **Verification:** Automated workflow validates manifest exists and results ≤ 7 days old
   - **Remediation:** Re-run Wave 7 benchmarks on RC candidate; if regression detected, block until root-caused

2. **Security Gates (W8-01 through W8-04)**
   - **Requirement:** Latest sanitizer suites (ASan/UBSan/TSan) and pentest both PASS with zero new CRITICAL findings
   - **Evidence:** 
     - `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md` (≤ 30 days old)
     - `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md` (≤ 90 days old)
   - **Verification:** Automated check confirms all sections show "PASS" status
   - **Remediation:** If new findings appear, remediate and re-run sanitizer cycle before merge

3. **GA Promotion Sign-Off**
   - **Requirement:** All boxes in `docs/governance/GA_PROMOTION_SIGN_OFF.md` §9 must be signed off by authorized signers
   - **Evidence:** Human signatures with timestamps in sign-off document
   - **Verification:** Manual PR review; cannot be automated
   - **Remediation:** Collect required sign-offs from release lead, security lead, operations lead

4. **Module Phase Dependency Graph**
   - **Requirement:** Phase dependency DAG must be acyclic; all phase prerequisites for release modules must be MET
   - **Evidence:** `docs/governance/PHASE_DEPENDENCY_GRAPH.md` with current dependency state
   - **Verification:** Automated script validates graph, checks prerequisites
   - **Remediation:** If blocked by prerequisite module, advance prerequisite module phases first, or skip dependent modules from release

5. **Zero Module Phase Regressions**
   - **Requirement:** No module already in release lane can have its phase number decreased
   - **Evidence:** Compare current phase state in release lane vs. development
   - **Verification:** Automated check on merge to release lane
   - **Remediation:** Reverse phase regression commit; if intentional, requires joint domain owner + release lead approval

### Tier 1: Escalation Required (Waivable)

**Definition:** Merge is **permitted only after escalation and explicit waiver** by Release Lead (with 72-hour review window). Waivers expire after 14 days.

**Gates:**

1. **Doxygen Coverage for Phase 6 Modules**
   - **Requirement:** All modules at Phase 6 must have ≥ 95% Doxygen coverage for public APIs
   - **Evidence:** Scoped `Doxyfile.audit` build output with XML coverage report and warning log
   - **Verification:** Automated check runs in `.github/workflows/gate-pr-doxygen-governance.yml` and Phase 6 acceptance validation
   - **Escalation Path:** If < 95%, file GitHub issue `[escalation] Doxygen coverage gap`, require release lead waiver
   - **Operational Marker:** Approved temporary overrides must be visible via PR label `governance/doxygen-waiver`
   - **Waiver Expiration:** 14 days; waiver must be renewed if gap not closed

2. **BSI C5 2026 Compliance Gaps**
   - **Requirement:** All identified BSI C5 compliance gaps must have remediation plan and be assigned to an owner
   - **Evidence:** GitHub issues with label `[compliance] BSI-C5-<control-id>` and remediation status
   - **Verification:** Automated scan of GitHub issues; count open gaps
   - **Escalation Path:** If any unassigned gap exists, require release lead escalation + domain owner commitment
   - **Waiver Expiration:** No waiver; gaps must be assigned before merge to `community` lane

3. **EU AI Act Model Card Coverage**
   - **Requirement:** For releases that include AI modules (llm, rag, ethics_ai, prompt_engineering), ≥ 75% must have signed Model Cards
   - **Evidence:** Markdown Model Cards in `docs/governance/ai-model-cards/` directory
   - **Verification:** Automated count of signed model cards vs. AI module count
   - **Escalation Path:** If < 75%, release lead can waive for `military` only; `community` requires ≥ 75%
   - **Waiver Expiration:** 14 days; must reach 75% before next release attempt

---

### Tier 2: Warning (Tracked but Non-Blocking)

**Definition:** Conditions logged and tracked, but do **not block** merge. Remediation tracked in backlog.

**Gates:**

1. **Modules with Zero Automated Tests**
   - **Condition:** Any module with 0 focused test files (`tests/<module>/test_*_focused.cpp`)
   - **Action:** Auto-create GitHub issue `[governance] <Module> — zero automated tests detected`
   - **Tracking:** Issue tracked in "Maturity → Compliance Auto-Fix Backlog" GitHub project
   - **Review:** Quarterly review; if still 0 tests after 3 months, escalate to Tier 1

2. **Research Backbone Gaps (Algorithm-Critical Modules)**
   - **Condition:** Module with algorithmic/ML significance (llm, rag, gpu, sharding, tensor) lacks entry in `research/implementation_influence/by_module.md`
   - **Action:** Auto-create GitHub issue `[governance] <Module> — research backbone missing`
   - **Tracking:** Logged but non-blocking
   - **Review:** Before Phase 6 closure, research backbone becomes mandatory (converts to Tier 1)

3. **Performance Regression Trends**
   - **Condition:** Benchmark results show consistent degradation > 3% over 4-week period
   - **Action:** Log to `ai_working/GATE_IMPROVEMENTS.md` with trend analysis
   - **Tracking:** Flag for operations review; if trend continues, escalate to performance lead

---

## Gate Validation Matrix

| Gate Category | Gate ID | Tier | Freshness | Validation | Auto-Escalate |
|---|---|---|---|---|---|
| Performance | W7-01 through W7-06 | 0 | ≤ 7 days | manifest + results | Yes, if > 7 days |
| Security | W8-01 through W8-04 | 0 | ≤ 30 days (sanitizer), ≤ 90 days (pentest) | section contains "PASS" | Yes, if stale |
| GA Sign-Off | GA_PROMOTION_SIGN_OFF.md §9 | 0 | N/A (manual) | human signature | No (manual only) |
| Phase Deps | PHASE_DEPENDENCY_GRAPH.md | 0 | ≤ 7 days | DAG acyclic + prereqs met | Yes, if broken |
| Doxygen | Phase 6 modules | 1 | ≤ 7 days | coverage ≥ 95% | Yes, if < 95% |
| BSI Gaps | COMPLIANCE_GAPS.md | 1 | ≤ 30 days | count open gaps | Yes, if > 0 |
| EU AI Act | MODEL_CARD_COVERAGE | 1 | ≤ 7 days | count ≥ 75% | Yes, if < 75% |
| Test Coverage | ZERO_TEST_MODULES | 2 | ≤ 7 days | count = 0 | No (logged only) |
| Research | RESEARCH_BACKBONE_GAPS | 2 | ≤ 7 days | entries present | No (pre-Phase-6) |

---

## Release Lane Promotion Flow

```
develop
  ↓
[Tier 0 gates PASS?] ──NO──→ [Block merge, post gate failures]
  ↓ YES
[Tier 1 gates PASS?]
  ├──YES──→ [Auto-merge to community/military/hyperscaler]
  └──NO───→ [Post escalation request, release lead reviews]
            ├──APPROVE (with waiver)──→ [Merge + log waiver to audit trail, expires 14 days]
            └──DENY────────────────→ [Block merge]
  ↓
[community / military / hyperscaler branch]
  ↓
[Phase 6 modules frozen: only hotfix patches allowed]
```

---

## Waiver Management

### Issuing a Waiver (Tier 1 only)

1. **Trigger:** PR to release lane fails Tier 1 gate
2. **Release Lead Action:** 
   - Posts `/approve-with-waiver <gate_id> <justification>` comment on PR
   - Justification must reference: business risk, remediation timeline, and stakeholder acknowledgment
3. **Logging:** Waiver automatically logged to `ai_working/ENFORCEMENT_WAIVERS.md`:
   ```
   | PR | Gate | Approver | Justification | Issue Date | Expires | Status |
   | #12345 | GATE-DOXYGEN-P6 | @release-lead | "Doxygen gap in new module; 10 tests added this week" | 2026-08-15 | 2026-08-29 | ACTIVE |
   ```
4. **Expiration:** Waiver automatically expires after 14 days; if not renewed, gate becomes blocking again

### Monitoring Waivers

- Weekly automated email to release team: "Active waivers summary"
- Quarterly audit: all waivers reviewed; expired waivers cleaned up
- Prevent silent bypasses: never allow Tier 0 waivers under any circumstance

---

## Exceptions & Emergency Procedures

### Critical Security Patch (Hotfix)

**When:** Security vulnerability discovered in released version that affects customers

**Process:**
1. Cherry-pick fix to release lane from develop
2. Re-run security gates (W8-01 through W8-04) only
3. If security gates PASS: hotfix can be released without full Tier 0 re-validation
4. Log hotfix exception to audit trail with CVSS score and customer impact

### Emergency Rollback

**When:** Release introduces regression that cannot be fixed forward

**Process:**
1. Revert merge commit to release lane
2. Create GitHub issue: `[emergency] Rollback reason`
3. Post-mortem: identify which Tier 0 gates failed to catch the regression
4. Strengthen gate detection before re-attempting merge

---

## Governance & Audit Trail

Every gate decision is logged to `ai_working/MERGE_GATE_AUDIT_LOG.json`:

```json
{
  "timestamp": "2026-08-15T10:30:00Z",
  "event_type": "merge_gate_validation",
  "pr_number": 12345,
  "target_branch": "community",
  "gate_id": "W7-01",
  "result": "PASS",
  "actor": "workflow/ci",
  "evidence_link": "benchmarks/wave7/release_gate_manifest_w7.json",
  "automation_version": "Phase2"
}
```

This log is immutable (append-only) and used for compliance audits and root cause analysis.

---

## Review Schedule

- **Weekly:** Release lead reviews active waivers and escalations
- **Monthly:** Security lead reviews Tier 0 security gate status
- **Quarterly:** Full gate policy review; adjust thresholds if needed
- **Annually:** Policy compliance audit against release tag history
