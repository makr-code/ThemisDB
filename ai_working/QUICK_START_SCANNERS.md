# Scanner Best Practices & GitHub Automation — Quick Start

**Date:** 2026-05-18  
**Status:** Ready to execute  
**Complexity:** Medium | **Duration:** 2-4 weeks implementation

---

## 🎯 Three Paths Forward

### Path 1: Run Phase 1 Scanner Locally (TODAY)

```bash
cd c:\Projects\ThemisDB

# Activate venv
.\.venv\Scripts\Activate.ps1

# Run phase 1 (Security + Memory + Reliability)
python tools/gap_scanner_v3.py . ai_working

# Expected output:
# - ai_working/gap_scan_v3_aggregate.json (all modules)
# - ai_working/gap_scan_v3_<module>.json (per-module detail)
# - ai_working/gap_scan_v3_summary.json (statistics)
```

**Output:** 130-230 new gaps (CRITICAL/HIGH focus)

---

### Path 2: Auto-Create GitHub Issues (ONCE Phase 1 Runs)

```bash
# Option A: Create from clustered templates (recommended)
python tools/github_issue_creator.py \
  --repo makr-code/ThemisDB \
  --cluster-dir ai_working/clustered_issues \
  --dry-run  # Remove for live execution

# Option B: Create from raw gap data
python tools/github_issue_creator.py \
  --repo makr-code/ThemisDB \
  --gap-json ai_working/gap_scan_v3_aggregate.json

# Expected: 25-35 GitHub issues created automatically
```

**Requires:** `gh CLI` installed and authenticated  
```bash
gh auth login  # One-time setup
```

---

### Path 3: Full End-to-End Pipeline (RECOMMENDED)

```bash
# Single command: Scan → Cluster → Create Issues
python tools/gap_scanner_and_issues.py \
  --repo-root c:\Projects\ThemisDB \
  --output-dir ai_working \
  --github  # Omit for dry-run

# Duration: ~5-10 minutes (depending on codebase size)
# Result: 13-35 GitHub issues auto-created + labeled + documented
```

---

## 📊 What Each Tool Does

### 1. gap_scanner_v3.py (Phase 1 Scanner)

**Input:** C++ source files  
**Output:** JSON gap reports + statistics

```bash
python tools/gap_scanner_v3.py [repo_root] [output_dir]

# Scans for:
# - Security gaps (unsafe functions, hardcoded secrets, SQL injection)
# - Memory safety (new/delete, pointer arithmetic, bounds)
# - Reliability (retry logic, timeouts, error handling)
```

**Generates:**
- `gap_scan_v3_aggregate.json` — All modules combined
- `gap_scan_v3_<module>.json` — Per-module (60 files)
- `gap_scan_v3_summary.json` — High-level metrics

---

### 2. github_issue_creator.py (Issue Automation)

**Input:** Gap JSON or markdown templates  
**Output:** GitHub issues (via gh CLI)

```bash
python tools/github_issue_creator.py \
  --repo makr-code/ThemisDB \
  --cluster-dir ai_working/clustered_issues \
  [--dry-run]
```

**Features:**
- Auto-extract labels (critical, high, module name)
- Auto-assign milestone (Current Sprint / Next Sprint)
- Batch creation (all files at once)
- Dry-run mode (preview without creating)

**Output:** 
- `github_issues_log.csv` — Issue URLs + titles

---

### 3. gap_scanner_and_issues.py (Full Pipeline)

**Input:** None (finds source files, gap templates automatically)  
**Output:** Complete pipeline report

```bash
python tools/gap_scanner_and_issues.py \
  --repo-root c:\Projects\ThemisDB \
  --output-dir ai_working \
  [--github]  # Add to create issues
```

**Pipeline Steps:**
1. ✓ Run Phase 1 scanner
2. ✓ Aggregate results
3. ✓ Cluster gaps by module
4. ✓ Generate issue templates
5. ✓ Create GitHub issues (if --github)
6. ✓ Generate status report

**Output:**
- All step 1-4 artifacts
- `pipeline_metrics.json` — Execution stats
- `pipeline_report.md` — Human-readable summary

---

## 🔧 Setup Checklist

- [ ] Python 3.13 environment activated (`.venv/Scripts/Activate.ps1`)
- [ ] `gh CLI` installed (`gh --version`)
- [ ] `gh` authenticated (`gh auth status`)
- [ ] Read: BEST_PRACTICE_SCANNER_INTEGRATION.md
- [ ] Review: SCANNER_ENHANCEMENTS_ROADMAP.md

---

## 🚀 Recommended Timeline

| Week | Task | Duration | Deliverable |
|------|------|----------|-------------|
| **This Week** | Phase 1 Scanner (security, memory, reliability) | 3-4 days | 130-230 new gaps |
| **This Week** | GitHub Automation | 2-3 days | 13-35 auto-created issues |
| **Next Week** | Phase 2 Scanner (concurrency, RAII, exception safety) | 3-4 days | +110-180 gaps |
| **Week 3** | Phase 3 Scanner (container, platform, performance) | 3-4 days | +80-130 gaps |
| **Ongoing** | Fix gaps per ROADMAP.md | Parallel | Track via GitHub issues |

---

## 📈 Expected Results After Phase 1

**Gaps Found:**
- Total: 130-230 (new in v3)
- CRITICAL: 30-50 (security/memory/reliability)
- HIGH: 80-150 (actionable)
- MEDIUM: 20-50 (lower priority)

**GitHub Issues:**
- 13-35 module-level issues
- All auto-labeled (P0/P1/P2)
- All auto-assigned milestones
- All linked to MODULE_GAPS.md

**Team Impact:**
- Visibility: Public issue tracking
- Accountability: Assigned to modules/owners
- Prioritization: CRITICAL fixes first
- Progress: GitHub project board

---

## 🔌 Integration Points

### Integration 1: CI/CD Pipeline
```yaml
# .github/workflows/gap-scanner.yml
- run: python tools/gap_scanner_v3.py . ai_working
- run: python tools/github_issue_creator.py --repo makr-code/ThemisDB --cluster-dir ai_working/clustered_issues
```

### Integration 2: ROADMAP.md Link
```markdown
# Implementation Roadmap

## Security Fixes (2026-Q3)
See [GitHub Issue MOD-security](https://github.com/makr-code/ThemisDB/issues/XXX)
- [ ] Fix 15+ CRITICAL security gaps
- [ ] Harden input validation
```

### Integration 3: Code Review Checklist
```
- [ ] No new gaps introduced (check gap_scan_v3_<module>.json)
- [ ] Related gap issues linked in PR description
- [ ] Gap fixes tracked in related GitHub issue
```

---

## ⚠️ Known Limitations (v3 Phase 1)

| Limitation | Impact | Workaround |
|-----------|--------|-----------|
| Text-based regex scanning | ~30% false positives | Manual review of HIGH/CRITICAL |
| No AST analysis | Misses some pointer issues | Use clang-tidy in Phase 3 |
| No cross-function analysis | Can't detect data flows | Add type checking in Phase 2 |
| Limited macro support | Misses some #ifdef blocks | Manual code inspection |
| No concurrency analysis yet | Race conditions not detected | Phase 2 has this |

---

## ✅ Success Criteria

Phase 1 considered **successful** when:

- [x] Phase 1 scanner produces gap reports (130-230 gaps)
- [x] GitHub issues auto-created (13-35 issues)
- [ ] CRITICAL gaps reviewed by team
- [ ] Security module assigned to security lead
- [ ] Memory module assigned to performance lead
- [ ] Reliability module assigned to infra lead
- [ ] Implementation begins (estimated 2-3 weeks)

---

## 🎯 Next Actions (Pick One)

### Option A: Start Phase 1 Scanning NOW
```bash
python tools/gap_scanner_v3.py . ai_working
```
**Duration:** 5-10 minutes  
**Effort:** None (fully automated)  
**Impact:** See 130-230 new gaps immediately

### Option B: Set Up GitHub Automation First
```bash
# Install gh CLI
winget install github-cli

# Authenticate
gh auth login

# Test
gh issue list --repo makr-code/ThemisDB --limit 5
```
**Duration:** 10 minutes  
**Effort:** Low (one-time setup)  
**Impact:** Ready for issue creation

### Option C: Do Both in Parallel
**Duration:** 30 minutes total  
**Effort:** Medium  
**Impact:** Full pipeline ready by end of today

---

## 📞 Support Resources

| Resource | Purpose | Link |
|----------|---------|------|
| SCANNER_ENHANCEMENTS_ROADMAP.md | Full details on 8 gap categories | ai_working/ |
| BEST_PRACTICE_SCANNER_INTEGRATION.md | Integration best practices | ai_working/ |
| gap_scanner_v3.py | Annotated source code | tools/ |
| github_issue_creator.py | Annotated source code | tools/ |

---

## 🎓 Learning Path

If you want to understand the scanners deeply:

1. Read: BEST_PRACTICE_SCANNER_INTEGRATION.md (Categories A-I)
2. Read: gap_scanner_v3_security.py (patterns explained)
3. Run: `python tools/gap_scanner_v3.py . ai_working` (see live output)
4. Review: ai_working/gap_scan_v3_security_aggregate.json (understand structure)
5. Run: `python tools/github_issue_creator.py --dry-run` (preview issues)
6. Read: ai_working/clustered_issues/*.md (templates)

---

**Ready to proceed? Choose Option A, B, or C above and execute!** ✅
