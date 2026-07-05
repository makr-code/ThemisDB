# Q3 2026 GitHub Epic Creation - Quick Reference

**Last Updated:** 2026-07-05  
**Status:** 🟢 Ready for Execution  

---

## 📋 What You're Getting

| Deliverable | Count | Description |
|-------------|-------|-------------|
| GitHub Epics | 5 | EPIC-001 through EPIC-005 |
| Sub-Issues | 23 | Detailed work items per epic |
| Total Issues | 28 | 5 epics + 23 sub-issues |
| Timeline | 20 weeks | W31 (Aug 4) through W49 (Dec 7, 2026) |
| Estimated Effort | 500+ hours | Across all work streams |

---

## 🚀 Quick Start

### Step 1: Verify Prerequisites

```bash
cd /home/runner/work/ThemisDB/ThemisDB

# Check that epic files exist
ls -la ai_working/EPIC_00*.md
# Should show:
# - EPIC_001_PHASE_1_4_SPRINT_8_9.md
# - EPIC_002_MODULE_HARDENING.md
# - EPIC_003_PHASE_6_SCANNER.md
# - EPIC_004_AQL_MUTATIONS_PHASE_1.md
# - EPIC_005_DISTRIBUTED_CONSISTENCY.md

# Check that script exists
ls -la .github/scripts/create-q3-2026-epics.py
```

### Step 2: Test with DRY-RUN

```bash
# Preview all issues without creating
DRY_RUN=1 python3 .github/scripts/create-q3-2026-epics.py

# Expected output:
# Repository : makr-code/ThemisDB
# Dry-run    : True
# Epic Filter: None (all)
# Epics Found: 5
# Sub-Issues : 24
#
# === EPIC-001: Phase 1-4 Gap Remediation Sprint 8-9 (Move & Concurrency) ===
# → [001-A] Sprint 8 - Move Semantics Hardening (97 Gaps)
#   [DRY-RUN] Would create with labels: ['phase-1-4-remediation', 'move-semantics', ...]
# ...
# ============================================================
# Created : 28
# Skipped : 0
# Failed  : 0
```

### Step 3: Set GitHub Token

```bash
# Export your personal access token with 'repo' scope
export GITHUB_TOKEN=ghp_YOUR_PERSONAL_ACCESS_TOKEN_HERE
export GITHUB_REPOSITORY=makr-code/ThemisDB

# Verify token is set (DO NOT ECHO - keep token private!)
test -n "$GITHUB_TOKEN" && echo "✅ Token set" || echo "❌ Token missing"
```

### Step 4: Create All Issues

```bash
# Create all 28 issues (takes ~45 seconds due to rate limiting)
python3 .github/scripts/create-q3-2026-epics.py

# Expected output:
# Repository : makr-code/ThemisDB
# Dry-run    : False
# Epic Filter: None (all)
# Epics Found: 5
# Sub-Issues : 24
#
# === EPIC-001: Phase 1-4 Gap Remediation Sprint 8-9 (Move & Concurrency) ===
# → [001-A] Sprint 8 - Move Semantics Hardening (97 Gaps)
#   ✅  Created: #5420 → https://github.com/makr-code/ThemisDB/issues/5420
# ...
# ============================================================
# Created : 28
# Skipped : 0
# Failed  : 0
```

### Step 5: Verify Issues on GitHub

```bash
# Open GitHub to verify
# https://github.com/makr-code/ThemisDB/issues
# Filter by label 'q3-2026' or 'epic-001' etc.
```

---

## 🎯 Epic Summary

### EPIC-001: Phase 1-4 Closure (117 Gaps)
- **Timeline:** W31-W34 (Aug 4 - Aug 25, 2026)
- **Effort:** 80 hours (3 sub-issues)
- **Deliverable:** v1.5.0 Stable Release
- **Sub-Issues:**
  - [001-A] Sprint 8: Move Semantics (97 gaps, 40h)
  - [001-B] Sprint 9: Concurrency (20 gaps, 25h)
  - [001-C] v1.5.0 Release Gate (15h)

### EPIC-002: Module Hardening (22,068 → 15,000 HIGH gaps)
- **Timeline:** W35-W42 (Sep 1 - Oct 20, 2026)
- **Effort:** 165 hours (5 sub-issues)
- **Target:** 35% reduction in HIGH gaps across 5 modules
- **Sub-Issues:**
  - [002-A] LLM: Input Validation (30h)
  - [002-B] Server: HTTP Validation (35h)
  - [002-C] Query: Exception Safety (35h)
  - [002-D] Sharding: 2PC/FK (35h)
  - [002-E] Index: Range Queries (30h)

### EPIC-003: Phase 6 Scanners (5 new categories)
- **Timeline:** W40-W47 (Oct 1 - Nov 23, 2026)
- **Effort:** 150 hours (6 sub-issues)
- **Discovery:** 1,500-2,500 new gaps
- **Sub-Issues:**
  - [003-A] Type Conversion Scanner (30h)
  - [003-B] Input Validation Scanner (35h)
  - [003-C] Exception Safety Scanner (30h)
  - [003-D] Uninitialized Variables (30h)
  - [003-E] OOP Design Violations (35h)
  - [003-F] Scanner CI/CD Integration (20h)

### EPIC-004: AQL 2.0.0 Mutations Phase 1 (Parser Foundation)
- **Timeline:** W36-W45 (Sep 8 - Nov 9, 2026)
- **Effort:** 55 hours (4 sub-issues)
- **Deliverable:** DML Parser + AST foundation
- **Sub-Issues:**
  - [004-A] Tokenizer DML Extension (12h)
  - [004-B] Parser DML Statements (20h)
  - [004-C] Mutation AST (15h)
  - [004-D] Documentation (8h)

### EPIC-005: Distributed Consistency (Unification)
- **Timeline:** W40-W49 (Oct 1 - Dec 7, 2026)
- **Effort:** 130 hours (4 sub-issues)
- **Scope:** 2PC/WAL/Replication/Raft unification
- **Sub-Issues:**
  - [005-A] 2PC/3PC Unification (40h)
  - [005-B] WAL Recovery Consolidation (35h)
  - [005-C] Cross-Shard FK at Scale (30h)
  - [005-D] Raft Membership Safety (25h)

---

## 📊 Effort Breakdown

| Epic | Sub-Issues | Total Effort | Weekly Velocity | Duration |
|------|-----------|--------------|-----------------|----------|
| EPIC-001 | 3 | 80h | 20h/week | 4 weeks |
| EPIC-002 | 5 | 165h | 21h/week | 8 weeks |
| EPIC-003 | 6 | 150h | 21h/week | 7 weeks |
| EPIC-004 | 4 | 55h | 10h/week | 5 weeks |
| EPIC-005 | 4 | 130h | 13h/week | 10 weeks |
| **TOTAL** | **23** | **580h** | **19h/week** | **30 weeks** |

---

## 🔍 Validation Checklist

After running the script, verify:

- [ ] All 5 epics created with correct titles
- [ ] All 23 sub-issues linked to parent epics
- [ ] Labels applied correctly:
  - `q3-2026` on all issues
  - `epic-001` through `epic-005` on respective sub-issues
  - Module-specific labels (llm, server, query, sharding, index)
- [ ] Issue descriptions include acceptance criteria
- [ ] No duplicate issues (script skips existing)
- [ ] GitHub Projects can filter by label

### Verification Command

```bash
# List all created issues with Q3 2026 label
# (via GitHub CLI or web interface)
gh issue list --label q3-2026 --repo makr-code/ThemisDB

# Expected: 28 issues (5 epics + 23 sub-issues)
```

---

## ⚙️ Advanced Options

### Create Single Epic

```bash
# Create only EPIC-002 and its sub-issues
EPIC=002 python3 .github/scripts/create-q3-2026-epics.py
```

### Verify Epic Files Parsing

```bash
# Check if epic parsing works correctly
python3 << 'PYTHON'
from pathlib import Path
import re

AI_WORKING = Path("ai_working")
for i in range(1, 6):
    epic_file = list(AI_WORKING.glob(f"EPIC_{i:03d}_*.md"))[0]
    content = epic_file.read_text()
    match = re.search(r'^# EPIC-(\d+):\s*(.+)$', content, re.MULTILINE)
    if match:
        print(f"✅ EPIC-{match.group(1)}: {match.group(2)[:60]}...")
    else:
        print(f"❌ No header in {epic_file.name}")
PYTHON
```

---

## 🐛 Troubleshooting

### Token Not Set

```
❌  GITHUB_TOKEN not set.
    Export it or set DRY_RUN=1 to preview.
```

**Solution:**
```bash
export GITHUB_TOKEN=ghp_YOUR_TOKEN
python3 .github/scripts/create-q3-2026-epics.py
```

### Epic Files Not Found

```
❌  No epic files found in ai_working/
```

**Solution:**
```bash
# Verify epic files exist
ls ai_working/EPIC_*.md

# If missing, check git status
git status ai_working/EPIC_*.md
git pull origin develop
```

### Rate Limiting (429 Error)

The script respects GitHub's secondary rate limit (1 request/second).  
If you see HTTP 429 errors:

1. Wait 60 seconds
2. Run again (already-created issues will be skipped)

---

## 📞 Support

### Questions?

1. Read `ai_working/Q3_2026_EPIC_IMPLEMENTATION_INDEX.md` (full documentation)
2. Check epic definitions: `ai_working/EPIC_*.md`
3. Review ROADMAP.md for baseline metrics

### Issues with Script?

1. Enable verbose output: `DRY_RUN=1` first
2. Check GitHub token permissions (requires `repo` scope)
3. Verify repository access: `gh repo view makr-code/ThemisDB`

---

## ✅ Success Criteria

Once all 28 issues are created, you should be able to:

- [ ] Filter by label `q3-2026` → 28 issues
- [ ] Filter by label `epic-001` → 3 sub-issues (001-A, 001-B, 001-C)
- [ ] Filter by label `epic-002` → 5 sub-issues (002-A through 002-E)
- [ ] View acceptance criteria in each issue description
- [ ] Sort by priority (CRITICAL, HIGH, MEDIUM, LOW)
- [ ] Group by epic for sprint planning

---

**Next:** Run the script, then update ROADMAP.md with issue numbers  
**Timeline:** 30 weeks execution (Q3-Q4 2026)  
**Owner:** Copilot AI Code Generation Agent
