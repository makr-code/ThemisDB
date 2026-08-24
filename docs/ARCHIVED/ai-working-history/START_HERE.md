# 🚀 GA v2.4.0 Promotion — START HERE

**Current Status:** 🔴 **BLOCKED** — Awaiting RocksDB Dependency Resolution  
**Time:** 2026-08-07 15:30 UTC  
**Estimated Time to Release:** 1–2 hours (once unblocked)  

---

## ⚡ IN ONE SENTENCE

**GA v2.4.0 is ready to release; benchmark validation is blocked by missing RocksDB dependency. Install librocksdb-dev to unblock.**

---

## 🎯 YOUR NEXT ACTION (Pick ONE)

### ✅ Option A: RECOMMENDED (5–10 minutes)

**Install RocksDB via system package:**
```bash
sudo apt-get update && sudo apt-get install -y librocksdb-dev librocksdb8.9
```

Then reply: **"Option A: Installing RocksDB via apt"**

---

### ⏳ Option B: Alternative (45–60 minutes)

**Compile RocksDB via vcpkg:**
```bash
cd /home/runner/work/ThemisDB/ThemisDB && ./vcpkg/vcpkg install
```

Then reply: **"Option B: Compiling via vcpkg"**

---

### 🔄 Option C: Fallback (10 minutes, lower assurance)

**Use baseline comparison validation:**
```bash
python3 benchmarks/ga_v2_4_0_gate_validation.py --use-baseline --output /tmp/report.json
```

Then reply: **"Option C: Using baseline validation"**

---

### ⬆️ Option D: Escalate

**Require infrastructure team to install RocksDB:**

Then reply: **"Option D: Escalating to infrastructure team"**

---

## 📊 What's the Situation?

| What | Status | Notes |
|------|--------|-------|
| Release Code | ✅ READY | All modules Phase 1–6 complete |
| Security & Compliance | ✅ PASS | Sanitizer ✅, Pentest ✅, Gaps ✅ |
| Documentation | ✅ COMPLETE | Checklists, runbooks, guides created |
| Release Automation | ✅ READY | Merge/tag scripts ready |
| CI Infrastructure | ✅ VERIFIED | Release-critical gates passing |
| **Benchmark Build** | ❌ FAILED | RocksDB missing |
| **Gate Validation** | ❌ BLOCKED | No benchmarks to validate |
| **Release** | ❌ BLOCKED | Awaiting validation |

---

## 📚 Full Documentation (Read These After Unblocking)

### For Release Team
- **`ai_working/GA_V2_4_0_EXECUTIVE_SUMMARY.md`** ← Main overview (read this!)
- `ai_working/GA_V2_4_0_PROMOTION_RUNBOOK.md` — Phase-by-phase execution guide
- `ai_working/GA_V2_4_0_PROMOTION_CHECKLIST.md` — Pre-release verification matrix
- `ai_working/GA_V2_4_0_STATUS_SUMMARY.md` — Component status details

### For Release Approver (Human)
- `ai_working/RELEASE_APPROVER_QUICK_REFERENCE.md` — 5-minute quick reference
- `docs/governance/GA_PROMOTION_SIGN_OFF.md` — Official governance (Section 9 needs your signature)

### For Troubleshooting
- `ai_working/BENCHMARK_EXECUTION_FAILURE_ANALYSIS.md` — Root cause & solutions
- `ai_working/IMMEDIATE_ACTION_REQUIRED.md` — Quick action summary

---

## ✨ What Happens After You Unblock RocksDB

**Automatic sequence (no manual intervention needed):**

1. **Build benchmarks** (~15–20 min) — `cmake --build --preset linux-release --target bench_w7a_* bench_w7d_* bench_w9a_* bench_w9b_* bench_w9c_* bench_w9d_*`

2. **Execute & validate gates** (~20–30 min) — `python3 benchmarks/ga_v2_4_0_gate_validation.py ...`

3. **Wait for human approval** (~15–30 min) — Release approver reviews evidence and signs Section 9

4. **Merge & release** (~15 min) — `bash scripts/ga_v2_4_0_release_merge_and_tag.sh ...`

**Total:** ~1–2 hours from RocksDB resolution to v2.4.0 released

---

## 📋 Six Critical Gates (All Must PASS)

```
GATE-W9-01: Audit throughput ≥ 100k ops/s      → UNVALIDATED (needs benchmarks)
GATE-W9-02: Auth p99 ≤ 150 µs                  → UNVALIDATED (needs benchmarks)
GATE-W9-03: Node rejoin p99 ≤ 2000 µs          → UNVALIDATED (needs benchmarks)
GATE-W9-04: RTO recovery p99 ≤ 5000 µs         → UNVALIDATED (needs benchmarks)
GATE-W9-05: Triage completeness = 1.0          → UNVALIDATED (needs benchmarks)
GATE-W9-06: Cross-tenant throughput ≥ 60k ops/s → UNVALIDATED (needs benchmarks)
```

**Release proceeds only if all 6 gates validate PASS**

---

## 🔄 Timeline Scenarios

### If You Choose Option A (RECOMMENDED) ✅
- **5–10 min:** Install RocksDB via apt
- **20–30 min:** Build & execute benchmarks
- **5 min:** Validate all gates
- **15–30 min:** Wait for human approval
- **15 min:** Execute merge/tag/release
- **Total:** ~1–1.5 hours to GA v2.4.0

### If You Choose Option B 🟡
- **45–60 min:** Compile RocksDB via vcpkg
- **20–30 min:** Build & execute benchmarks
- **5 min:** Validate all gates
- **15–30 min:** Wait for human approval
- **15 min:** Execute merge/tag/release
- **Total:** ~2 hours to GA v2.4.0

### If You Choose Option C 🔴
- **10 min:** Baseline comparison validation
- **15–30 min:** Wait for human approval (lower assurance)
- **15 min:** Execute merge/tag/release
- **Total:** ~30 min to GA v2.4.0 (but less rigorous)

---

## ❓ FAQ

**Q: Why is RocksDB missing?**  
A: The release-profile build requires RocksDB (dependency in cmake/Dependencies.cmake:214). CI environment doesn't have it pre-installed.

**Q: Can we skip benchmarks?**  
A: No. All 6 Wave 9 hard gates must validate PASS per GA_PROMOTION_SIGN_OFF.md Section 3.

**Q: What if a gate FAILS?**  
A: P0 incident — debug root cause on develop branch, fix, re-run benchmarks.

**Q: Who signs off on release?**  
A: Human release approver at Section 9 of GA_PROMOTION_SIGN_OFF.md (AI cannot approve).

**Q: Can I skip the human approval?**  
A: No. Section 9 is a mandatory human-only gate per governance model.

---

## 🎯 WHAT YOU NEED TO DO RIGHT NOW

**Choose your action from the three options above ⬆️** and reply with the option number.

---

## 📞 Need Help?

- **Release process questions?** → Read `ai_working/GA_V2_4_0_EXECUTIVE_SUMMARY.md`
- **Troubleshooting?** → Read `ai_working/BENCHMARK_EXECUTION_FAILURE_ANALYSIS.md`
- **Specific phase?** → Read `ai_working/GA_V2_4_0_PROMOTION_RUNBOOK.md`

---

**Status:** 🔴 BLOCKED — Awaiting your action  
**Your move:** Pick Option A/B/C/D above and reply

---

*Prepared by: Claude AI Agent*  
*Date: 2026-08-07 15:30 UTC*  
*Next Update: When RocksDB is resolved*
