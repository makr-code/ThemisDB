# GA v2.4.0 Release: IMMEDIATE ACTION REQUIRED

**Issue:** Benchmark build failed — RocksDB dependency missing  
**Severity:** P0 (Release Blocker)  
**Status:** BLOCKED — Awaiting dependency resolution  
**Time Since Failure:** ~20 minutes (since 15:15 UTC)  

---

## What Happened

The GA v2.4.0 promotion benchmark suite failed to build during CMake configuration. **Missing dependency:** RocksDB

```
Error: RocksDB not found. Install via vcpkg (rocksdb) or system package librocksdb-dev
```

**This blocks all benchmark validation and prevents GA promotion from proceeding.**

---

## What's at Risk

- ❌ Wave 7 benchmark validation (gates W7-A, W7-D)
- ❌ Wave 8 regression gates
- ❌ Wave 9 hard gates (6 critical gates: GATE-W9-01 through GATE-W9-06)
- ❌ Human sign-off (Section 9 of GA_PROMOTION_SIGN_OFF.md)
- ❌ GA v2.4.0 release merge/tag/publication

**Release Status:** 🔴 CANNOT PROCEED UNTIL RESOLVED

---

## How to Unblock (Pick ONE Option)

### ✅ OPTION A: RECOMMENDED (5–10 minutes)

**Install RocksDB via system package** (requires root/sudo):

```bash
sudo apt-get update
sudo apt-get install -y librocksdb-dev librocksdb8.9
```

Then restart the benchmark suite:
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset linux-release
cmake --build --preset linux-release --parallel 16 \
  --target bench_w7a_release_critical_signoff \
           bench_w7d_guardrails_variance_operability \
           bench_w9a_security_overhead_audit \
           bench_w9b_sla_measurement_compliance \
           bench_w9c_chaos_fault_recovery \
           bench_w9d_multi_tenant_isolation
```

**Success Probability:** Very High  
**Time to Benchmarks Running:** ~5–10 minutes  
**Time to GA Release:** ~1–1.5 hours total (after build completes)

---

### ⏳ OPTION B: ALTERNATIVE (45–60 minutes, then build)

**Allow vcpkg to compile RocksDB from source:**

```bash
cd /home/runner/work/ThemisDB/ThemisDB
./vcpkg/vcpkg install
# Wait 45–60+ minutes for compilation...

cmake --preset linux-release
cmake --build --preset linux-release --parallel 16 \
  --target bench_w7a_release_critical_signoff \
           bench_w7d_guardrails_variance_operability \
           bench_w9a_security_overhead_audit \
           bench_w9b_sla_measurement_compliance \
           bench_w9c_chaos_fault_recovery \
           bench_w9d_multi_tenant_isolation
```

**Success Probability:** Very High  
**Time to Benchmarks Running:** ~45–60 minutes  
**Time to GA Release:** ~2 hours total (compilation + build + benchmarks)

---

### 🔄 OPTION C: FALLBACK ONLY (10 minutes, but less rigorous)

**Use baseline comparison validation** (not ideal for GA):

```bash
python3 benchmarks/ga_v2_4_0_gate_validation.py \
  --use-baseline \
  --output /tmp/ga_v2_4_0_validation_report.json
```

**Success Probability:** Acceptable but lower assurance  
**Time to Report:** ~10 minutes  
**Risk:** Does not validate live performance; compares against pre-recorded baselines only

---

## What Happens Next (Once Dependency is Resolved)

1. **Build completes** (~10–20 minutes after unblock)
2. **Run benchmarks** (~10–20 minutes)
3. **Validate gates** (~5 minutes)
4. **Request human sign-off** (ongoing until approver available)
5. **Merge develop → community** (~5 minutes)
6. **Create v2.4.0 tag** (~2 minutes)
7. **Build release artefact** (~15 minutes)
8. **Publish release** (~5 minutes)

**Total Time to GA Release:** ~1–2 hours from RocksDB resolution

---

## Decision Point

**Choose your action:**

- [ ] **Option A:** I have root/sudo access — proceed with `apt-get install`
- [ ] **Option B:** I'll allow vcpkg to compile — proceed with `./vcpkg/vcpkg install`
- [ ] **Option C:** I accept baseline-only validation — proceed with fallback validation script
- [ ] **ESCALATE:** I don't have access — escalate to IT/infrastructure team

---

## Why This Happened

RocksDB is a required dependency for the release-profile build. The current CI environment doesn't have it pre-installed or cached. This is documented in:
- `cmake/Dependencies.cmake:214`
- Repository memory: "community-release configure fails unless RocksDB is installed"

For future GA releases, we recommend:
1. Pre-install system dependencies in CI workflow
2. Add pre-flight verification before benchmarking
3. Document in benchmark runbooks

---

## Files Ready to Use (Don't Touch These)

✅ Benchmarks are ready to build (no code changes needed)  
✅ Gate validation script ready (`benchmarks/ga_v2_4_0_gate_validation.py`)  
✅ Merge/tag automation ready (`scripts/ga_v2_4_0_release_merge_and_tag.sh`)  
✅ Complete documentation suite ready (5 guides in `ai_working/`)  
✅ Security & compliance evidence complete (Batch A-C PASS)  

**Your only action:** Resolve RocksDB dependency, then everything else proceeds automatically.

---

## Support Documents

- Full analysis: `ai_working/BENCHMARK_EXECUTION_FAILURE_ANALYSIS.md`
- Release runbook: `ai_working/GA_V2_4_0_PROMOTION_RUNBOOK.md`
- Approver quick reference: `ai_working/RELEASE_APPROVER_QUICK_REFERENCE.md`
- Status summary: `ai_working/GA_V2_4_0_STATUS_SUMMARY.md`

---

**Time-Critical:** Please take action within the next 30 minutes to maintain GA release timeline.

**Questions?** Review the analysis document or escalate to release engineering lead.
