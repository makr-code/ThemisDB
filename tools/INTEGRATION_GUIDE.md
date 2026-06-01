# 🔗 INTEGRATION GUIDE — Auto-Workflow Tools

**Status:** Ready to Deploy  
**Effort:** 8 hours total (3 Quick Wins)  
**ROI:** 30-40% time savings

---

## 📦 TOOLKIT OVERVIEW

### Auto Phase 0 Validator
**File:** `tools/auto_phase0_validator.py`  
**Time:** ~5 minutes per module  
**Replaces:** Manual environment checks

```bash
python tools/auto_phase0_validator.py llm
python tools/auto_phase0_validator.py server
```

**Output:**
- Console report (PASS/FAIL)
- Exit code 0 (success) / 1 (failure)

### Auto Gap Categorizer
**File:** `tools/auto_gap_categorizer.py`  
**Time:** ~2-3 minutes per module (vs 2-4 hours manual)  
**Replaces:** Manual gap categorization

```bash
python tools/auto_gap_categorizer.py ai_working/gap_scan_v3_aggregate.json --module llm
```

**Output:**
- `phase1_report.md` (human readable)
- `phase1_report.json` (structured data)

### Auto Checkpoint Runner
**File:** `tools/auto_checkpoint_runner.py`  
**Time:** ~10-15 minutes per checkpoint  
**Replaces:** Manual build/test/report cycle

```bash
python tools/auto_checkpoint_runner.py llm --commit abc1234
```

**Output:**
- `checkpoint_YYYYMMDD_HHMMSS.md` (GitHub comment)
- `checkpoint_YYYYMMDD_HHMMSS.json` (metrics)

---

## 🔄 WORKFLOW INTEGRATION

### Phase 0 → Phase 1
```
1. Run Phase 0 Validator
   ├─ If FAIL: Stop, fix environment
   └─ If PASS: Proceed to Phase 1

2. Run Gap Categorizer (Phase 1)
   └─ Output: phase1_report.md
     - Categories breakdown
     - Risk analysis
     - Cross-dependencies
```

**Command Chain:**
```bash
#!/bin/bash
# Quick start for new module

MODULE=$1  # e.g., "llm"

echo "Phase 0: Validation"
python tools/auto_phase0_validator.py $MODULE || exit 1

echo "Phase 1: Categorization"  
python tools/auto_gap_categorizer.py ai_working/gap_scan_v3_aggregate.json --module $MODULE

echo "[OK] Ready for Phase 2 Planning"
```

### Phase 3 Integration
```
Development Loop:
  1. Start implementation (Phase 3)
  2. After 5 commits: Run checkpoint
  3. If PASS: Continue to next 5 commits
  4. If FAIL: Fix issues, re-run checkpoint
```

**Git Hook Integration:**
```bash
#!/bin/bash
# .git/hooks/post-commit (or CI trigger)

# Count commits since last checkpoint
COMMITS_SINCE=$(git rev-list HEAD --count)
LAST_CHECKPOINT=$(git log --oneline | grep -i "checkpoint" | head -1 | cut -d' ' -f1)

COMMITS_SINCE_CHECKPOINT=$((COMMITS_SINCE - $(git rev-list $LAST_CHECKPOINT --count 2>/dev/null || echo 0)))

if [ $((COMMITS_SINCE_CHECKPOINT % 5)) -eq 0 ]; then
    echo "[AUTO] Running checkpoint after 5 commits"
    python tools/auto_checkpoint_runner.py $MODULE_NAME --commit HEAD
fi
```

---

## 📊 EXAMPLE: Complete LLM Module Workflow

### Week 1: Phase 0-2

**Day 1-2: Phase 0 Validation**
```bash
$ python tools/auto_phase0_validator.py llm
================================================================================
PHASE 0: Pre-Flight Validation (LLM)
================================================================================

[OK] CMake Preset
[OK] Build Tools
[OK] Aggregate JSON
[OK] CTest
[OK] Module Buildable

[OK] Phase 0 validation PASSED - Ready for Phase 1
```

**Day 3-4: Phase 1 Audit & Categorization**
```bash
$ python tools/auto_gap_categorizer.py ai_working/gap_scan_v3_aggregate.json --module llm

[OK] Phase 1 report generated: phase1_report.md
[OK] JSON output: phase1_report.json

# Report shows:
# - Security (234 gaps)
# - Memory (567 gaps)
# - Concurrency (123 gaps)
# - etc.
```

**Day 5: Phase 2 Planning (Manual)**
- Use phase1_report.json to create task breakdown
- Prioritize by severity + dependency
- Assign story points

### Week 2-3: Phase 3 Implementation

**Iteration 1 (5 commits)**
```bash
$ git commit -m "fix: add input validation to tokenizer"
$ git commit -m "fix: handle null pointers in cache layer"
$ git commit -m "fix: thread-safe model loading"
$ git commit -m "fix: add exception safety guarantees"
$ git commit -m "fix: improve error messages"

# Auto-trigger after 5 commits:
$ python tools/auto_checkpoint_runner.py llm

================================================================================
CHECKPOINT VALIDATION: llm
================================================================================

[*] Building project...
[OK] Build passed

[*] Running tests...
[OK] All tests passed

[*] Checking code quality...
[OK] Code quality check passed

================================================================================
[OK] CHECKPOINT PASSED - Ready to continue implementation
================================================================================

[OK] Checkpoint report: checkpoint_20260520_143022.md
[OK] JSON results: checkpoint_20260520_143022.json
```

**Checkpoint Report (auto-posted to GitHub):**
```markdown
## Checkpoint Report: llm

**Commit:** abc1234f  
**Timestamp:** 2026-05-20T14:30:22  
**Preset:** windows-release

### Checkpoint Summary
- Build: **PASS**
- Tests: **PASS**
- Code Quality: **PASS**

**Result:** ✅ CHECKPOINT PASSED

### Details
- Project built successfully
- 1247 tests passed (0 failed)
- All quality checks passed

### Next Steps
1. Continue implementation
2. Next checkpoint at 5 commits
```

**Iteration 2 (5 commits)**
```
[Same pattern]
- 5 commits
- Auto-checkpoint
- Report posted to GitHub
```

---

## 🔧 CI/CD INTEGRATION

### GitHub Actions Workflow
```yaml
name: Auto Checkpoints

on:
  push:
    branches: [feature/llm-*]

jobs:
  checkpoint:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup Python
        uses: actions/setup-python@v4
        with:
          python-version: 3.11
      
      - name: Check if checkpoint needed
        run: |
          $commits = (git rev-list --count HEAD)
          if ($commits % 5 -eq 0) {
            echo "CHECKPOINT_NEEDED=true" >> $env:GITHUB_ENV
          }
      
      - name: Run Auto Checkpoint
        if: env.CHECKPOINT_NEEDED == 'true'
        run: |
          python tools/auto_checkpoint_runner.py llm --commit HEAD
      
      - name: Comment on PR
        if: always()
        uses: actions/github-script@v6
        with:
          script: |
            const fs = require('fs');
            const reports = fs.readdirSync('.').filter(f => f.startsWith('checkpoint_'));
            if (reports.length > 0) {
              const report = fs.readFileSync(reports[0], 'utf8');
              github.rest.issues.createComment({
                issue_number: context.issue.number,
                owner: context.repo.owner,
                repo: context.repo.repo,
                body: report
              });
            }
```

---

## 📈 METRICS & TRACKING

### Per-Module Dashboard
```
Module: LLM
Status: Phase 3 (Implementation)

Progress:
  Phase 0 ✅ Validation (1h spent)
  Phase 1 ✅ Audit (4h spent)
  Phase 2 ✅ Planning (8h spent)
  Phase 3 🔄 Implementation (32h/520h)
    - Iteration 1: ✅ 5 commits, checkpoint passed
    - Iteration 2: ✅ 5 commits, checkpoint passed
    - Iteration 3: 🔄 3 commits (in progress)

Velocity: ~50 gaps/week
Next Checkpoint: After 2 more commits
```

### Aggregate Metrics (All Modules)
```
Total Progress: 8100 / 193858 gaps (4.2%)

Phase Distribution:
  Phase 0 (Validation): ✅ 100% (all 10 modules)
  Phase 1 (Audit): 🔄 30% (3 modules complete)
  Phase 2 (Planning): 🔄 20% (2 modules complete)
  Phase 3 (Implementation): 🔄 5% (1 module)

Bottlenecks:
  - Phase 5 (Code Review): 40% of time
  - Recommended: Parallel reviews across modules
```

---

## ⚡ QUICK START (Next Monday)

### 30-Minute Setup

**Step 1: Deploy Tools** (5 min)
```bash
# Already in repo:
# - auto_phase0_validator.py
# - auto_gap_categorizer.py
# - auto_checkpoint_runner.py
```

**Step 2: Test on LLM Module** (20 min)
```bash
# Phase 0
python tools/auto_phase0_validator.py llm

# Phase 1
python tools/auto_gap_categorizer.py ai_working/gap_scan_v3_aggregate.json --module llm

# Review outputs
cat phase1_report.md
```

**Step 3: Create First Checkpoint** (5 min)
```bash
python tools/auto_checkpoint_runner.py llm --commit HEAD
cat checkpoint_*.md
```

### First Week Wins
- ✅ All 10 top modules pass Phase 0
- ✅ All 10 modules categorized (Phase 1 reports ready)
- ✅ Top 3 modules start Phase 3 with checkpoints

---

## 🎯 SUCCESS CRITERIA

| Metric | Target | Current | Week 1 | Week 2 |
|--------|--------|---------|--------|--------|
| Phase 0 time | <30 min | 2h manual | ✅ | ✅ |
| Phase 1 time | <2h | 8h manual | 2h | ✅ |
| Phase 3 velocity | 100 gaps/week | 50 gaps/week | 60 | 80+ |
| Checkpoint time | <15 min | N/A (manual) | 15m | ✅ |
| Code quality | 92% | 85% | 87% | 90% |

---

## 📚 DOCUMENTATION

See also:
- `DEVELOPMENT_OPPORTUNITIES.md` — Full feature roadmap
- `ISSUE_WORKFLOW_TEMPLATE.md` — 7-phase specification
- `WORKFLOW_ACTIVATION_REPORT.md` — Current status

---

**Ready to deploy?**  
→ Start with Quick Win #1: `python tools/auto_phase0_validator.py llm`
