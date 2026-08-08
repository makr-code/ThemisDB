# GA v2.4.0 — Post-Resolution Workflow (Once RocksDB is Available)

**This document describes the AUTOMATED steps that will execute once RocksDB dependency is resolved.**

**Prerequisites:** RocksDB must be installed (either via apt, vcpkg, or baseline option)

---

## PHASE 2: BENCHMARK BUILD & EXECUTION

**Timeline:** ~40–50 minutes (automated)  
**Manual Intervention:** None (if build succeeds)  
**Owner:** Build system (automated)

### Step 2.1: CMake Configuration

```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset linux-release
```

**Expected Output:**
```
[100%] Built target [all dependencies]
-- Configuring done
-- Generating done
```

**If fails:** Check CMAKE output for missing dependencies; escalate to build engineer

---

### Step 2.2: Build Benchmark Targets

```bash
cmake --build --preset linux-release --parallel 16 \
  --target bench_w7a_release_critical_signoff \
           bench_w7d_guardrails_variance_operability \
           bench_w9a_security_overhead_audit \
           bench_w9b_sla_measurement_compliance \
           bench_w9c_chaos_fault_recovery \
           bench_w9d_multi_tenant_isolation
```

**Expected Output:**
```
[100%] Built target bench_w7a_release_critical_signoff
[100%] Built target bench_w7d_guardrails_variance_operability
[100%] Built target bench_w9a_security_overhead_audit
[100%] Built target bench_w9b_sla_measurement_compliance
[100%] Built target bench_w9c_chaos_fault_recovery
[100%] Built target bench_w9d_multi_tenant_isolation
```

**If fails:** Likely linker error or dependency issue; check build logs

---

### Step 2.3: Execute Wave 7 Benchmarks

```bash
mkdir -p benchmarks/results/wave{7,8,9}

# W7-A: Release Critical Sign-Off
./build-gcc-linux-release/benchmarks/wave7/bench_w7a_release_critical_signoff \
  --benchmark_out=benchmarks/results/wave7/w7a.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=5

# W7-D: Guardrails Variance & Operability  
./build-gcc-linux-release/benchmarks/wave7/bench_w7d_guardrails_variance_operability \
  --benchmark_out=benchmarks/results/wave7/w7d.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=7
```

**Expected Output:** JSON files created in `benchmarks/results/wave7/`

**If timeout:** Increase `--benchmark_repetitions` timeout or check system load

---

### Step 2.4: Execute Wave 9 Benchmarks

```bash
# W9-A: Security Overhead (Audit Operations)
./build-gcc-linux-release/benchmarks/wave9/bench_w9a_security_overhead_audit \
  --benchmark_out=benchmarks/results/wave9/w9a.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=5

# W9-B: SLA Measurement Compliance
./build-gcc-linux-release/benchmarks/wave9/bench_w9b_sla_measurement_compliance \
  --benchmark_out=benchmarks/results/wave9/w9b.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=5

# W9-C: Chaos Fault Recovery
./build-gcc-linux-release/benchmarks/wave9/bench_w9c_chaos_fault_recovery \
  --benchmark_out=benchmarks/results/wave9/w9c.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=5

# W9-D: Multi-Tenant Isolation
./build-gcc-linux-release/benchmarks/wave9/bench_w9d_multi_tenant_isolation \
  --benchmark_out=benchmarks/results/wave9/w9d.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=5
```

**Expected Output:** JSON files in `benchmarks/results/wave9/`

**Typical Duration:** 20–30 minutes total for all Wave 9 benchmarks

---

## PHASE 3: GATE VALIDATION

**Timeline:** ~5 minutes (automated)  
**Manual Intervention:** Review results  
**Owner:** Validation script

### Step 3.1: Run Gate Validation Orchestrator

```bash
python3 benchmarks/ga_v2_4_0_gate_validation.py \
  --wave7-a benchmarks/results/wave7/w7a.json \
  --wave7-d benchmarks/results/wave7/w7d.json \
  --wave9-a benchmarks/results/wave9/w9a.json \
  --wave9-b benchmarks/results/wave9/w9b.json \
  --wave9-c benchmarks/results/wave9/w9c.json \
  --wave9-d benchmarks/results/wave9/w9d.json \
  --output /tmp/ga_v2_4_0_validation_report.json
```

**Expected Output (SUCCESS):**
```
Gate Validation Report: /tmp/ga_v2_4_0_validation_report.json

GATE-W9-01: PASS ✓ (101,234 ops/s >= 100,000)
GATE-W9-02: PASS ✓ (125 µs <= 150 µs)
GATE-W9-03: PASS ✓ (1,850 µs <= 2,000 µs)
GATE-W9-04: PASS ✓ (4,200 µs <= 5,000 µs)
GATE-W9-05: PASS ✓ (1.0 == 1.0)
GATE-W9-06: PASS ✓ (65,432 ops/s >= 60,000)

Overall Status: ALL GATES PASS ✅
Exit Code: 0
```

**If ANY gate FAILS:**
```
GATE-W9-02: FAIL ✗ (175 µs > 150 µs)  [P0 INCIDENT]
Overall Status: GATES FAILED ❌
Exit Code: 1
```

**Action if FAIL:** P0 incident — investigate root cause, fix on develop, re-run benchmarks

---

### Step 3.2: Archive Validation Report

```bash
cp /tmp/ga_v2_4_0_validation_report.json \
   /home/runner/work/ThemisDB/ThemisDB/ai_working/GA_V2_4_0_GATE_VALIDATION_REPORT.json

git add ai_working/GA_V2_4_0_GATE_VALIDATION_REPORT.json
git commit -m "GA v2.4.0: Gate validation report - ALL GATES PASS"
```

---

## PHASE 4: REQUEST HUMAN SIGN-OFF

**Timeline:** Ongoing (awaiting approver availability)  
**Manual Intervention:** Release approver must review & sign  
**Owner:** Human (Release Approver)

### Step 4.1: Notify Release Approver

**Message to approver:**
```
GA v2.4.0 benchmark validation COMPLETE — ALL GATES PASS ✅

Gate Validation Report: 
  → /home/runner/work/ThemisDB/ThemisDB/ai_working/GA_V2_4_0_GATE_VALIDATION_REPORT.json

Security Evidence (Already Approved):
  → docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md ✅
  → security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md ✅

Module Compliance (Already Verified):
  → No new CRITICAL findings in top-risk modules ✅

Action Required:
  1. Review gate validation results
  2. Review security & module evidence
  3. Complete Section 9 signature block in GA_PROMOTION_SIGN_OFF.md
  4. Commit & push sign-off to develop

Quick Reference: ai_working/RELEASE_APPROVER_QUICK_REFERENCE.md
```

---

### Step 4.2: Release Approver Completes Sign-Off

**Approver action:**

1. **Review validation report** (5 min)
   ```
   Review: /home/runner/work/ThemisDB/ThemisDB/ai_working/GA_V2_4_0_GATE_VALIDATION_REPORT.json
   Expected: All 6 gates PASS
   ```

2. **Review security evidence** (10 min)
   ```
   Review: docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md
   Status: ✅ PASS (verified 2026-08-01)
   
   Review: security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md
   Status: ✅ PASS (verified 2026-08-01)
   ```

3. **Review module compliance** (5 min)
   ```
   Verify: No new CRITICAL findings per ROADMAP.md §Module Status
   Status: ✅ VERIFIED (updated 2026-08-04)
   ```

4. **Review promotion checklist** (10 min)
   ```
   File: ai_working/GA_V2_4_0_PROMOTION_CHECKLIST.md
   Action: Verify all 14 items in Section 3 marked [x]
   ```

5. **Complete Section 9 signature block** (5 min)
   ```
   File: docs/governance/GA_PROMOTION_SIGN_OFF.md
   
   Edit lines 183–206:
   - [x] YES — proceed with develop → community merge and v2.4.0 tag
   - Release Approver: [Your Name], [Your Role]
   - Signature: [GitHub handle or email]
   - Date: [YYYY-MM-DD]
   - Deferred Items Acceptance: [Accept / Reject]
   ```

6. **Commit & push approval**
   ```bash
   cd /home/runner/work/ThemisDB/ThemisDB
   git add docs/governance/GA_PROMOTION_SIGN_OFF.md
   git commit -m "GA v2.4.0: Section 9 human sign-off complete"
   git push origin copilot/makr-code-themisdb-5668-update-scheduler-status
   ```

**Estimated Duration:** ~30 minutes for approver

---

## PHASE 5: AUTOMATED RELEASE (Merge, Tag, Publish)

**Timeline:** ~15 minutes (automated)  
**Manual Intervention:** None (script runs autonomously)  
**Owner:** Release automation script

### Step 5.1: Execute Release Automation Script

**Approver has signed off (Section 9 committed) → Proceed with release**

```bash
cd /home/runner/work/ThemisDB/ThemisDB

bash scripts/ga_v2_4_0_release_merge_and_tag.sh \
  "Release Approver Name" \
  "approver.email@company.com"
```

**What the script does:**
1. Verify Section 9 signature block exists (hard fail if missing)
2. Create develop → community merge commit
3. Wait for CI gate (release-critical tests) to PASS
4. Create v2.4.0 tag with annotation
5. Build release artefact from tag
6. Create GitHub Release entry
7. Update RELEASE_TYPE to "stable"

**Expected Output:**
```
✅ Verifying human sign-off in GA_PROMOTION_SIGN_OFF.md...
   Section 9 signature found ✓

✅ Creating develop → community merge...
   Merge commit: abc1234 "GA v2.4.0: Release promotion from develop"

✅ Waiting for CI gate (release-critical-tests)...
   CI Status: PASS ✓

✅ Creating v2.4.0 tag...
   Tag: v2.4.0
   Commit: abc1234

✅ Building release artefact...
   Binary: releases/themisdb-v2.4.0-linux-x86_64.tar.gz
   Checksum: abc123...

✅ Creating GitHub Release entry...
   Release: https://github.com/makr-code/ThemisDB/releases/tag/v2.4.0

✅ Updating RELEASE_TYPE to 'stable'...
   File: RELEASE_TYPE (content: "stable")

✅ GA v2.4.0 RELEASED SUCCESSFULLY 🎉
```

**If FAIL at any step:** Script stops with error message; manual intervention required

---

## PHASE 6: POST-RELEASE VERIFICATION

**Timeline:** ~10 minutes (manual verification)  
**Owner:** Release engineer

### Step 6.1: Verify Tag & Release

```bash
# Verify tag exists and points to merge commit
git tag -l | grep v2.4.0
git show v2.4.0 --stat

# Verify GitHub Release was published
# → Check: https://github.com/makr-code/ThemisDB/releases/tag/v2.4.0

# Verify release artefact is available
ls -lh releases/themisdb-v2.4.0-*.tar.gz

# Verify RELEASE_TYPE file updated
cat RELEASE_TYPE
# Expected output: "stable"
```

### Step 6.2: Verify CI Pipeline

```bash
# Check that release-critical CI gate PASSED on the merge commit
# → Visit: .github/workflows/09-pr-gates_release-critical-tests.yml (check latest run)
# Expected: All tests PASS on tag commit
```

### Step 6.3: Create Release Notes

**Action:** Update GitHub Release entry with notes:

```markdown
# ThemisDB v2.4.0 — General Availability Release

## Release Summary
ThemisDB v2.4.0 marks the first General Availability (GA) release of the community edition.

## Highlights
- Wave 7 release-critical gates PASS ✅
- Wave 8 regression gates PASS ✅
- Wave 9 hard gates PASS ✅
- Security & compliance verified ✅
- Sanitizer evidence: PASS
- Pentest evidence: PASS

## Changes Since v2.3.x
[Link to CHANGELOG.md v2.4.0 section]

## Installation
```bash
tar -xzf themisdb-v2.4.0-linux-x86_64.tar.gz
cd themisdb-v2.4.0
./install.sh
```

## Support
- Documentation: https://themisdb.io/docs/2.4.0
- Issues: https://github.com/makr-code/ThemisDB/issues
- Discussions: https://github.com/makr-code/ThemisDB/discussions
```

---

## PHASE 7: COMMUNICATION & HANDOFF

**Timeline:** ~15 minutes (notification)  
**Owner:** Release manager

### Step 7.1: Notify Stakeholders

**Message to team:**
```
🎉 GA v2.4.0 Released Successfully

Release Details:
- Tag: v2.4.0
- Commit: [merge commit hash]
- Released: [date/time]
- Gate Status: ALL PASS ✅

Download: https://github.com/makr-code/ThemisDB/releases/tag/v2.4.0

Documentation: https://themisdb.io/docs/2.4.0

Thank you to everyone who contributed to this release!
```

### Step 7.2: Archive Promotion Documentation

```bash
# Create archive of all GA promotion artifacts
mkdir -p ai_working/GA_V2_4_0_ARCHIVE
cp ai_working/GA_V2_4_0_*.md \
   ai_working/GA_V2_4_0_ARCHIVE/

cp benchmarks/ga_v2_4_0_gate_validation.py \
   ai_working/GA_V2_4_0_ARCHIVE/

cp scripts/ga_v2_4_0_release_merge_and_tag.sh \
   ai_working/GA_V2_4_0_ARCHIVE/

# Commit archive
git add ai_working/GA_V2_4_0_ARCHIVE/
git commit -m "GA v2.4.0: Archive promotion artifacts post-release"
```

---

## SUCCESS CRITERIA CHECKLIST

- [ ] Benchmarks built successfully
- [ ] All Wave 7 & 9 benchmarks executed
- [ ] All 6 Wave 9 hard gates PASS
- [ ] Gate validation report generated
- [ ] Human sign-off completed (Section 9)
- [ ] develop → community merge created
- [ ] CI gate (release-critical) PASSED on merge
- [ ] v2.4.0 tag created
- [ ] Release artefact built & available
- [ ] GitHub Release entry published
- [ ] RELEASE_TYPE updated to "stable"
- [ ] Stakeholders notified
- [ ] Promotion artifacts archived

---

## TROUBLESHOOTING

### Build Fails After RocksDB Install

**Error:** CMake still reports RocksDB not found

**Solution:**
```bash
# Clear CMake cache and retry
cd /home/runner/work/ThemisDB/ThemisDB
rm -rf build-gcc-linux-release CMakeCache.txt CMakeFiles/
cmake --preset linux-release
cmake --build --preset linux-release --target [benchmarks]
```

---

### Benchmarks Timeout or Crash

**Error:** Benchmark hangs or segmentation fault

**Diagnosis:**
1. Check system load: `top`, `free -h`
2. Check disk space: `df -h`
3. Check logs: `tail -100 build-gcc-linux-release/CMakeFiles/CMakeOutput.log`

**Solution:**
1. Reduce repetitions: `--benchmark_repetitions=3`
2. Run one benchmark at a time
3. Increase timeout: `--benchmark_time_unit=ms`

---

### Gate Validation FAILS

**Error:** One or more gates report FAIL

**Response:** P0 Incident

1. Investigate root cause (module performance regression?)
2. Fix on develop branch
3. Re-run full benchmark suite
4. Do NOT proceed with release until ALL gates PASS

---

### Human Sign-Off Delayed

**Issue:** Approver unavailable within expected timeframe

**Action:**
1. Check if approver has other commitments
2. Designate alternate approver if needed
3. Update timeline; communicate delay to stakeholders
4. Do NOT proceed with release without proper human approval

---

## QUICK REFERENCE: SUCCESS PATH

1. ✅ RocksDB installed
2. ✅ Benchmarks built & executed
3. ✅ All gates PASS (W7, W8, W9)
4. ✅ Human sign-off completed
5. ✅ Release script executed
6. ✅ v2.4.0 tag created
7. ✅ GitHub Release published
8. 🎉 **GA v2.4.0 RELEASED**

---

**Total Time:** ~1.5–2 hours (from RocksDB resolution to GA release)

**Next Steps:** Follow Phase 2–7 in order

*Document prepared: 2026-08-07 15:30 UTC*
