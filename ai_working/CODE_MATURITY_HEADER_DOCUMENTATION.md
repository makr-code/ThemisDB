# 📊 Code Maturity Header System — Documentation & Template

**Version:** 2.0  
**Status:** ✅ Active in production  
**Last Updated:** 2026-05-18  
**Responsible Workflow:** `.github/workflows/08-maintenance_code-maturity.yml`  
**Implementation Script:** `.github/scripts/analyze_code_maturity.py`

---

## 🎯 Overview

Every source file in ThemisDB has an **auto-generated header** that documents:

1. **Quality Metrics** — Code maturity level, quality score, line count
2. **Open Issues** — TODOs, STUBs, Simulations, Dead code
3. **Revision History** — Last 5 commits with dates and titles
4. **Version Tracking** — Semantic version incremented on each modification

---

## 📄 Header Template (Standard Format)

This is the **canonical template** used in every source file:

```
╔═══════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                 ║
╠═══════════════════════════════════════════════════════════════════╣
  File:            <filename>                                       ║
  Version:         <major>.<minor>.<patch>                          ║
  Last Modified:   <YYYY-MM-DD HH:MM:SS>                            ║
  Author:          <git-user-name or "unknown">                     ║
╠═══════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                  ║
    • Maturity Level:  <LEVEL>                                      ║
    • Quality Score:   <score>/100                                  ║
    • Total Lines:     <lines>                                      ║
    • Open Issues:     <ISSUE_SUMMARY>                              ║
╠═══════════════════════════════════════════════════════════════════╣
  Revision History:                                                 ║
    • <sha>  <date>  <title>                                        ║
    • <sha>  <date>  <title>                                        ║
    • ... (up to 5 entries)                                         ║
╠═══════════════════════════════════════════════════════════════════╣
  Status: <STATUS_LINE>                                             ║
╚═══════════════════════════════════════════════════════════════════╝
```

### Example (from grpc_server.cpp)

```
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_server.cpp                                    ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:48:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     335                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ed2d46e8d1  2026-04-13  fix(api): complete gRPC stub wiring...  ║
    • f38c013cdc  2026-03-29  Enhance various components...           ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
```

---

## 📋 "Open Issues" Field Format

The **Open Issues** line summarizes code quality issues in one compact line:

```
Open Issues:     TODOs: 0, Stubs: 0, Simulations: 2, DeadCode: 1, DocumentedStubs: 3
```

### Components

| Label | Meaning | Source |
|-------|---------|--------|
| **TODOs** | `TODO:`, `FIXME:`, `HACK:`, `XXX:`, `BUG:` comments | Pattern matching |
| **Stubs** | `stub`, `placeholder`, `not implemented` patterns | Pattern matching |
| **Simulations** | `simulate`, `mock`, `fake`, `dummy` code | Pattern matching |
| **DeadCode** | `if (false &&` or `// DISABLED` blocks | Pattern matching |
| **DocumentedStubs** | `STUB/SIMULATION NOTE:` comment blocks | Pattern matching |

### Examples

```
Open Issues:     TODOs: 0, Stubs: 0                          // Clean file
Open Issues:     TODOs: 5, Stubs: 3, Simulations: 1         // Some work needed
Open Issues:     TODOs: 12, Stubs: 8, DocumentedStubs: 2    // Multiple issues
```

---

## 🎯 Maturity Levels & Quality Scores

### Maturity Levels

The **Maturity Level** is determined by the **Quality Score** (0-100):

| Score | Level | Symbol | Meaning |
|-------|-------|--------|---------|
| ≥ 80  | PRODUCTION-READY | 🟢 | Ready for production; minimal issues |
| 60-79 | RELEASE-CANDIDATE | 🟡 | Mostly ready; minor fixes needed |
| 40-59 | BETA | 🟠 | Functional but needs work |
| 20-39 | ALPHA | 🔴 | Early development stage |
| 0-19  | DRAFT | ⚫ | Stub or incomplete |

### Quality Score Calculation

```
Score = 100.0
Score -= (undocumented_stubs) * 7
Score -= (simulations) * 5
Score -= (documented_stubs) * 2
Score -= (todos) * 3
Score -= (hardcoded_values) * 4
Score -= (dead_code) * 6
Score -= (debug_statements) * 1
Score += min((tests) * 2, 20)
Score += min((docs) * 1, 15)
Score += min((production_marker) * 3, 9)
Score = max(0, min(100, Score))
```

### Example Scoring

```
File: kernel.cpp
- Stubs: 2               → -14 points
- TODOs: 3              → -9 points
- Test patterns: 8      → +16 points (max 20)
- Documentation: 5      → +5 points (max 15)
────────────────────────────
Final Score: 100 - 14 - 9 + 16 + 5 = 98/100 → 🟢 PRODUCTION-READY
```

---

## 🔄 CI/CD Pipeline

### Workflow: `08-maintenance_code-maturity.yml`

**Triggers:**

- ✅ Pull requests touching `.github/scripts/analyze_code_maturity.py`
- ✅ Push to `develop/main` touching `.github/version_tracking.json`
- ✅ Weekly on Monday at 03:00 UTC
- ✅ Manual via `workflow_dispatch` with parameters

**Two Modes:**

### Mode 1: Check-Only (Default)

```bash
python .github/scripts/analyze_code_maturity.py \
  --root . \
  --no-headers \
  --report-path /tmp/code_maturity_report.md
```

**Actions:**
- Scans all supported source files
- Generates `docs/code_maturity_report.md`
- Detects drift (compares with committed version)
- **No file modifications**

**Use Case:** PR validation, drift detection, nightly checks

### Mode 2: Rewrite (Manual Only)

```bash
python .github/scripts/analyze_code_maturity.py \
  --root .
```

**Actions:**
- Scans all supported source files
- Updates headers in every file
- Increments patch versions
- Generates/updates:
  - `docs/code_maturity_report.md`
  - `.github/version_tracking.json`
  - `.github/badges/*.json`
- **Files are modified**

**Use Case:** Monthly maintenance, version bumps, header refresh

**Manual Trigger:**
```
Push to GitHub → "Actions" tab → "Code Maturity Maintenance"
→ "Run workflow" → Set "update_headers=true"
```

---

## 🛠️ Implementation Script

### File: `.github/scripts/analyze_code_maturity.py`

#### Supported File Types

| Extension | Comment Style | Example |
|-----------|---------------|---------|
| `.cpp`, `.cc`, `.c` | C-style `/* */` | `int main() { ... }` |
| `.h`, `.hpp`, `.hxx` | C-style `/* */` | `class MyClass { ... };` |
| `.py` | Python `"""docstring"""` | `def foo(): ...` |
| `.php` | C-style `/* */` | `<?php function foo() { ... }` |
| `.cs` | C-style `/* */` | `public class MyClass { ... }` |

#### Pattern Detection

The script uses regex patterns to detect quality issues:

##### Negative Patterns (Deduct Points)

```python
PATTERN_STUBS = re.compile(
    r'\b(?:stub|placeholder|noop|NotImplemented)\b'
    r'|throw\s+std::runtime_error\s*\(\s*["\']not\s+implemented["\']\s*\)'
)

PATTERN_TODOS = re.compile(r'\b(TODO|FIXME|HACK|XXX|BUG)\b')

PATTERN_SIMULATIONS = re.compile(
    r'\b(?:simulat\w*|mock\w*|fake[ds]?|dummy)\b'
)

PATTERN_DEAD_CODE = re.compile(
    r'if\s*\(\s*false\s*&&|//\s*DISABLED\b'
)

PATTERN_HARDCODED = re.compile(
    r'\b(?:hardcoded|temporary|temp\s+fix)\b'
)
```

##### Positive Patterns (Award Points)

```python
PATTERN_PRODUCTION = re.compile(r'@production\b|production_ready')

PATTERN_TESTS = re.compile(r'\b(TEST|EXPECT_|ASSERT_)\w*[\(\{]')

PATTERN_DOCS = re.compile(r'(/\*\*|"""|\'\'\')|(///\s+\w)|\* @param')
```

---

## 📊 Version Tracking

### File: `.github/version_tracking.json`

Stores version history for every file:

```json
{
  "src/api/grpc_server.cpp": {
    "version": "0.0.15",
    "last_update": "2026-04-15 18:48:33 UTC",
    "author": "unknown",
    "maturity_level": "🟢 PRODUCTION-READY",
    "score": 100.0,
    "recent_commits": 5
  },
  "src/security/auth_handler.cpp": {
    "version": "0.1.42",
    "last_update": "2026-05-10 14:22:15 UTC",
    "author": "alice",
    "maturity_level": "🟡 RELEASE-CANDIDATE",
    "score": 72.5,
    "recent_commits": 8
  }
}
```

### Semantic Versioning

- **Major**: Reserved (always 0 for in-development)
- **Minor**: Assigned by category (0-9 based on module)
- **Patch**: Auto-incremented on every modification

---

## 📈 Code Maturity Report

### File: `docs/code_maturity_report.md`

**Auto-generated summary** showing:

```markdown
# Code Maturity Report

## Summary

| Module | Total Files | Avg Score | Maturity |
|--------|-------------|-----------|----------|
| acceleration | 12 | 78.3 | 🟡 RC |
| security | 8 | 85.1 | 🟢 PROD |
| storage | 15 | 64.2 | 🟡 RC |
| ... | ... | ... | ... |

## Top 10 Highest Quality Files

1. src/api/grpc_server.cpp (100.0) — 🟢 PRODUCTION-READY
2. src/security/encryption.cpp (96.3) — 🟢 PRODUCTION-READY
...

## Top 10 Files Needing Work

1. src/llm/inference_stub.cpp (18.2) — ⚫ DRAFT
2. src/acceleration/gpu_fallback.cpp (31.5) — 🔴 ALPHA
...

## Issues Summary

- **Total Files Analyzed**: 847
- **Production-Ready (≥80)**: 612 (72.2%)
- **Needs Work (<60)**: 89 (10.5%)
- **Draft/Stub (<20)**: 12 (1.4%)
```

---

## 🔗 Integration with Gap Audit System

### Connection Points

The **Code Maturity Header** and **Gap Audit System** work together:

| Aspect | Code Maturity | Gap Audit | Integration |
|--------|---------------|-----------|-------------|
| **Detects** | Code patterns (TODO, STUB, MOCK) | Same patterns + context | Complementary |
| **Scope** | Individual file headers | Module-level documentation | Headers feed audit data |
| **Frequency** | Weekly (scheduled) | Monthly (manual trigger) | Audit runs after maturity update |
| **Output** | File headers + report | JSON scan + module docs | Both artifacts preserved |

### Data Flow

```
┌──────────────────────────────────────┐
│  analyze_code_maturity.py            │
│  (runs weekly via CI/CD)             │
├──────────────────────────────────────┤
│ Updates:                             │
│  • File headers (THEMIS_GAP_STATS)   │
│  • .github/version_tracking.json     │
│  • docs/code_maturity_report.md      │
└────────────┬─────────────────────────┘
             │
             │ (Output feeds into)
             │
┌────────────▼──────────────────────┐
│  gap_audit_pipeline_v2.py          │
│  (runs monthly via manual trigger) │
├────────────────────────────────────┤
│ Reads:                             │
│  • File headers (gap stats)        │
│  • Code patterns (enhanced v2)     │
│ Generates:                         │
│  • Module documentation            │
│  • Clustered GitHub issues         │
│  • Progress metrics                │
└────────────────────────────────────┘
```

---

## 🚀 How to Use

### For Developers

**View your file's status:**

```bash
head -30 src/acceleration/kernel.cpp
# Shows: maturity level, quality score, open issues, recent commits
```

**Understand what needs work:**

```
Open Issues:     TODOs: 5, Stubs: 2
# Means: 5 TODO comments, 2 unimplemented functions to fix
```

### For CI/CD Teams

**Run check-only (no modifications):**

```bash
python .github/scripts/analyze_code_maturity.py \
  --root . \
  --no-headers
```

**Update headers (manual, with artifacts):**

```
GitHub Actions → Code Maturity Maintenance workflow
→ workflow_dispatch → update_headers=true → Run
```

**Artifact artifacts:**
- `.github/version_tracking.json` (download and review)
- `.github/badges/*.json` (download and review)
- `docs/code_maturity_report.md` (download and review)

### For Project Managers

**Check monthly progress:**

```bash
cat docs/code_maturity_report.md
# See: overall quality trends, production-ready %, files needing work
```

**Track specific modules:**

```bash
grep -A 5 "acceleration" docs/code_maturity_report.md
# See: module quality score, file count, maturity level
```

---

## ⚙️ Configuration

### Environment Variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `MATURITY_MAX_HISTORY` | 5 | Max commits in revision history |
| `EXCLUDE_BOT_COMMITS` | true | Filter workflow automation commits |

**Example:**
```bash
export MATURITY_MAX_HISTORY=10
export EXCLUDE_BOT_COMMITS=false
python .github/scripts/analyze_code_maturity.py
```

### Excluded Directories

Files in these directories are skipped:

```
.git, node_modules, build, dist, vendor, .github, third_party, external
```

---

## 📚 File Format Details

### C/C++ Header Format

```cpp
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example.cpp                                        ║
  Version:         0.0.5                                              ║
  ...
*/

#include <iostream>

// Your actual source code here
int main() { ... }
```

### Python Header Format

```python
"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example.py                                         ║
  Version:         0.0.5                                              ║
  ...
"""

import sys

# Your actual source code here
def main(): ...
```

---

## 🔧 Troubleshooting

### Issue: Header not being updated

**Cause:** File not in supported extensions or in excluded directory

**Fix:**
```bash
# Check if file extension is supported
python -c "
ext = 'YOUR_FILE.ext'
SUPPORTED = {'.cpp', '.h', '.hpp', '.py', '.php', '.cs'}
print(f'{ext} supported: {ext.lower() in SUPPORTED}')
"
```

### Issue: Version not incrementing

**Cause:** File not in version_tracking.json or analysis skipped

**Fix:**
```bash
python .github/scripts/analyze_code_maturity.py \
  --root . \
  --report-path /tmp/report.md
# Check /tmp/report.md for file list
```

### Issue: Score seems wrong

**Cause:** Patterns matching false positives (e.g., word "simulation" in comment)

**Fix:**
- Document intentional patterns with `STUB/SIMULATION NOTE:` marker
- Use exact pattern matching in code
- Check PATTERN definitions in script

---

## 📖 Related Documentation

- **SYSTEM_OVERVIEW.md** — Complete gap audit system overview
- **MODULE_DOCUMENTATION_GUIDE.md** — Per-module gap docs
- **SMART_HEADER_FORMAT_GUIDE.md** — Intelligent header format detection
- **SCANNER_V2_IMPROVEMENTS.md** — Context-aware gap detection

---

## 📝 Maintenance

### Monthly Checklist

- [ ] Run `complete_gap_audit.py`
- [ ] Review `docs/code_maturity_report.md` for drift
- [ ] Update workflow if patterns need adjustment
- [ ] Archive previous month's metrics

### Annual Review

- [ ] Reassess penalty/bonus weights
- [ ] Update maturity level thresholds
- [ ] Document any workflow changes

---

**Status:** ✅ Production Ready  
**Last Updated:** 2026-05-18  
**Maintained By:** ThemisDB Development Team  
**GitHub Workflow:** `.github/workflows/08-maintenance_code-maturity.yml`  
**Script:** `.github/scripts/analyze_code_maturity.py`
