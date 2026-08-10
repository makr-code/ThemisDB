# Maturity Claim Verification Checklist

**Document:** Release Notes Claim Validation  
**Purpose:** Prevent unsubstantiated maturity claims in release notes  
**Owner:** Platform Release Team  
**Last Updated:** 2026-08-10

---

## Overview

This checklist ensures that every maturity claim made in release notes (e.g., "Module X is production-ready") is automatically verified against evidence in the MATURITY_EVIDENCE_REGISTRY.md and gate validation results.

**Policy:** No claim of module production readiness can be included in release notes without passing verification.

---

## Verification Rules by Claim Type

### Claim Template 1: "Module X is production-ready"

**Verification Requirements:**

- [ ] Module Phase = 6 (verified in `src/<module>/ROADMAP.md`)
- [ ] All Phase 1-6 acceptance criteria PASS (check module_phase_verifier.py report)
- [ ] Latest sanitizer suite run (ASan/UBSan/TSan) shows PASS with 0 new CRITICAL findings
- [ ] Latest benchmark gates all PASS within SLA baseline (≤10% regression)
- [ ] Doxygen API documentation coverage ≥ 95%
- [ ] Zero blockers listed in PHASE_CLOSURE_POLICY.md sign-off section
- [ ] Domain owner sign-off timestamp present in ROADMAP.md Phase 6 header

**Evidence Location:**
- Phase status: `src/<module>/ROADMAP.md`
- Test acceptance: `ai_working/module_phase_report_*.json`
- Sanitizer: `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`
- Benchmarks: `benchmarks/<module>/GATE_BASELINE_*.json` + latest results
- Doxygen: `audit/doxygen_coverage_<module>.json`
- Sign-off: `src/<module>/ROADMAP.md` (Phase 6 section)

**Automated Check:**
```bash
python3 .github/scripts/verify_maturity_claim.py \
  --claim "production-ready" \
  --module <module_name> \
  --verify-against-registry
```

---

### Claim Template 2: "Performance improved by X% in module Y"

**Verification Requirements:**

- [ ] Benchmark file exists: `benchmarks/<module>/bench_*_gates.cpp`
- [ ] Baseline established in: `benchmarks/<module>/GATE_BASELINE_*.json`
- [ ] Latest benchmark run shows improvement (results file exists and is ≤ 7 days old)
- [ ] Improvement statistically significant (> 5% delta, multiple runs confirm)
- [ ] No regression in other benchmarks introduced by same change

**Evidence Location:**
- Benchmark gates: `benchmarks/<module>/GATE_RESULTS_*.json`
- Improvements log: `ai_working/GATE_IMPROVEMENTS.md`

**Automated Check:**
```bash
python3 .github/scripts/benchmark_gate_validator.py \
  --module <module> \
  --verify-improvement
```

---

### Claim Template 3: "Module X complies with [EU AI Act | BSI C5 | GDPR]"

**Verification Requirements:**

- [ ] Compliance assessment exists in `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`
- [ ] Compliance statement explicitly says "COMPLIANT" or "COMPLIANT_WITH_NOTES"
- [ ] If "COMPLIANT_WITH_NOTES", gaps must be listed with remediation plan and target date
- [ ] Model Card (if AI module) completed and signed: `.github/templates/MODEL_CARD.md`
- [ ] Latest compliance audit ≤ 90 days old

**Evidence Location:**
- Pentest compliance: `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`
- Model Card: Signed instance in module documentation
- Compliance audit: `docs/governance/GA_PROMOTION_SIGN_OFF.md`

**Automated Check:**
```bash
python3 .github/scripts/verify_maturity_claim.py \
  --claim "compliance" \
  --standard <EU_AI_ACT|BSI_C5|GDPR> \
  --module <module_name>
```

---

### Claim Template 4: "All tests pass in module X"

**Verification Requirements:**

- [ ] Latest CI run for module X shows all focused tests PASS
- [ ] Test coverage ratio ≥ 80% (tests/acceptance_criteria_count)
- [ ] No skipped or xfail tests in critical path
- [ ] Latest run timestamp ≤ 7 days old

**Evidence Location:**
- Test results: `ai_working/module_phase_report_*.json`
- CI logs: `.github/workflows/03-pr-gates_*` artifacts

**Automated Check:**
```bash
python3 .github/scripts/module_phase_verifier.py \
  --module <module> \
  --verify-test-ratio
```

---

## Automated Verification Workflow

**Trigger:** Release notes are committed to PR

**Process:**

1. Extract all maturity claims from release notes (pattern: "production-ready", "complies with", "passes", "100%")
2. Map each claim to verification rule above
3. For each claim:
   - Query MATURITY_EVIDENCE_REGISTRY.md for relevant gates
   - Check gate validation result (PASS/FAIL/STALE)
   - Compare against claim statement
   - If any claim is unsubstantiated, block PR merge
4. Post PR comment with verification results

**Script Location:** `.github/scripts/verify_release_notes_claims.py`

---

## Release Notes Verification Checklist

Use this checklist when writing release notes:

- [ ] All production-ready claims reference Phase 6 closure
- [ ] All compliance claims reference latest audit (≤ 90 days)
- [ ] All performance claims reference latest benchmark (≤ 7 days)
- [ ] No vague claims like "improved", "better", "enhanced" without metrics
- [ ] All module names match exactly those in `src/*/ROADMAP.md`
- [ ] All version numbers match tag being released
- [ ] All gate references (e.g., "W7 benchmark gates PASS") verified to be current
- [ ] Security claims reference latest sanitizer/pentest (<30 days for W8)

---

## Examples

### ✅ VALID Claims

```markdown
## Production-Ready Modules (Phase 6)

- **auth**: Phase 6 complete, all W7-W9 gates PASS, sanitizer suites clean
- **failover**: Phase 6 complete, canTransition gate ≤100µs baseline, zero new CVE findings

## Compliance

- **rag**: EU AI Act Article 13/22 compliant; Model Card signed 2026-08-09
- **llm**: GDPR compliant; data retention < 30d; privacy audit PASS

## Performance

- **sharding**: Query latency improved 12% vs v8.0.0 baseline (benchmark gate FP23-01)
```

### ❌ INVALID Claims (will be rejected)

```markdown
## Production-Ready Modules

- **ethics_ai**: Production-ready (❌ No phase closure evidence)
- **gpu**: Significantly improved performance (❌ Vague; no metrics or benchmark reference)

## Compliance

- **prompt_engineering**: GDPR-compliant (❌ No recent audit; cannot verify)
```

---

## Enforcement

**Merge Gate Policy (Tier 0):**

If release notes contain unverifiable maturity claims:
1. Merge gate workflow blocks PR
2. Comment posted with specific claim failures
3. Release lead must update notes to remove/substantiate claims
4. Merge allowed only after verification passes

**Escalation:**

- Release lead can override for documented business need (requires justification)
- Waiver logged in `ai_working/ENFORCEMENT_WAIVERS.md` with expiration (14 days)

---

## Supporting Scripts

### `verify_release_notes_claims.py`

Scans release notes for maturity claims and verifies each against registry.

```bash
python3 .github/scripts/verify_release_notes_claims.py \
  --notes <file.md> \
  --registry docs/governance/MATURITY_EVIDENCE_REGISTRY.md \
  --output-report claims_verification.json
```

**Output:** JSON report with claim-by-claim verification results.

---

## References

- **Evidence Registry:** `docs/governance/MATURITY_EVIDENCE_REGISTRY.md`
- **Phase Closure Policy:** `docs/governance/PHASE_CLOSURE_POLICY.md`
- **Release Promotion Gate Policy:** `docs/governance/RELEASE_PROMOTION_GATE_POLICY.md`
- **Gate Validation Results:** `ai_working/maturity-verification-report-*.json`

