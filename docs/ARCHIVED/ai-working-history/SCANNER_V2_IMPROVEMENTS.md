# 🔬 ThemisDB Enhanced Gap Scanner v2 — Improvements & Usage

**Version:** 2.0  
**Date:** 2026-05-18  
**Status:** Ready for testing

---

## 🎯 What's New in v2

### 1. **Contextual Analysis** (Reduce False Positives)

| Issue | v1 Behavior | v2 Improvement |
|-------|-------------|-----------------|
| STUB in test code | Counted as gap | Marked as LOW priority, tracked separately |
| Platform fallbacks | Counted as critical gap | Detected as INTENTIONAL (conditional compilation) |
| Mock/test frameworks | Counted as gap | Filtered out with LOW severity |
| Documented STUBs | Counted as gap | Marked as INTENTIONAL (4-line template) |

### 2. **Enhanced Pattern Detection**

**New Patterns Added:**
- ✅ Empty function bodies: `void foo() { }`
- ✅ Functions with only logging: `void foo() { LOG(...); }`
- ✅ Platform-specific patterns: `#ifdef THEMIS_ENABLE_*`
- ✅ Disabled code blocks: `#if 0 ... #endif`
- ✅ Mock/test framework code: `EXPECT_CALL`, `MOCK_METHOD`
- ✅ Fallback implementations: "not available", "unsupported on"
- ✅ Technical debt markers: HACK, DEBT, OPTIMIZE

### 3. **Severity Scoring**

Now calculates severity based on **context**, not just pattern:

```python
# Example: STUB marker
if has_documentation and is_test_code:
    severity = LOW
elif has_documentation:
    severity = INTENTIONAL
else:
    severity = HIGH  # Undocumented STUB
```

### 4. **File Header Statistics** 📊

Every source file gets a header with live gap statistics:

**Simple Format (one line):**
```cpp
// THEMIS_GAP_STATS: gaps=5 unimpl=3 stub=2 mock=1 sim=0 todo=0 debt=0 scanned=2026-05-18
```

**Detailed Format (multi-line):**
```cpp
// THEMIS_GAP_ANALYSIS
//   Total Gaps: 5
//   Unimplemented: 3
//   STUB (documented): 1
//   STUB (undocumented): 1
//   Mock/Test: 1
//   Simulations: 0
//   TODO/FIXME: 0
//   Technical Debt: 0
//   Platform-Specific: 0
//   Disabled Code: 0
//   Last Scanned: 2026-05-18 14:32:15
```

This header:
- 🔄 **Auto-updates** on each scan
- 📍 **Shows current state** of each file
- 🎯 **Tracks progress** (gaps decrease over time)
- 🔗 **Enables tools** to find files needing work

### 5. **Gap Categories Expanded**

| Category | v1 | v2 | Severity |
|----------|----|----|----------|
| `unimplemented` | ✓ | ✓ | CRITICAL |
| `stub_documented` | ~ | ✓ | INTENTIONAL |
| `stub_undocumented` | ✓ | ✓ | HIGH |
| `mock_framework` | ✗ | ✓ | LOW |
| `test_only` | ✗ | ✓ | LOW |
| `platform_fallback` | ✗ | ✓ | INTENTIONAL |
| `conditional` | ✗ | ✓ | INTENTIONAL |
| `disabled_code` | ✗ | ✓ | LOW |
| `technical_debt` | ~ | ✓ | LOW/MEDIUM |

---

## 📖 Usage

### Quick Start

```bash
# Run full pipeline (scan + report + update headers)
python tools/gap_audit_pipeline_v2.py

# Scan only
python tools/gap_scanner_v2.py

# Update headers only (detailed)
python tools/file_header_updater.py ai_working/gap_scan_v2_aggregate.json . --detailed
```

### Advanced Options

```bash
# Custom output directory
python tools/gap_audit_pipeline_v2.py --output my_reports

# Skip header updates
python tools/gap_audit_pipeline_v2.py --no-headers

# Use detailed multi-line headers
python tools/gap_audit_pipeline_v2.py --detailed-headers

# Custom repo root
python tools/gap_audit_pipeline_v2.py --repo /path/to/themis
```

---

## 📊 Output Files

### New Report Structure

```
ai_working/
├── gap_scan_v2_aggregate.json          # Summary by module
├── gap_scan_v2_summary.json            # Overall metrics (NEW)
├── gap_scan_v2_<module>.json           # Per-module details (57 files)
└── reports/
    ├── TOP_MODULES.md                  # Top 20 modules report
    ├── CATEGORIES_ANALYSIS.md          # Gap breakdown by category
    ├── FALSE_POSITIVE_AUDIT.md         # Flagged false positives
    └── INTENTIONAL_GAPS.md             # By-design gaps (platform-specific, etc.)
```

### Example Report: `gap_scan_v2_summary.json`

```json
{
  "scan_date": "2026-05-18T14:32:15.123456",
  "total_gaps": 1862,
  "total_modules": 57,
  "by_severity": {
    "critical": 1620,
    "high": 384,
    "medium": 29,
    "low": 156,
    "intentional": 247
  },
  "by_category": {
    "unimplemented": 1620,
    "stub_documented": 100,
    "stub_undocumented": 284,
    "todo_items": 52,
    "technical_debt": 98,
    "platform_fallback": 150,
    "disabled_code": 78,
    "mock_framework": 95
  },
  "modules": {
    "acceleration": { "total": 235, ... },
    "security": { "total": 139, ... },
    ...
  }
}
```

---

## 🔍 Gap Detection Examples

### Example 1: Undocumented STUB → HIGH Priority

**Code:**
```cpp
void processGPUKernel() {
    // STUB: GPU implementation missing
    throw std::runtime_error("not implemented");
}
```

**Gap Detection:**
```json
{
  "category": "stub_undocumented",
  "severity": "high",
  "context_info": {
    "has_documentation": false,
    "is_test_code": false,
    "notes": "Missing 4-line STUB/SIMULATION NOTE template"
  }
}
```

**Action:** Add proper 4-line template (from COPILOT_INSTRUCTIONS.md § 8)

### Example 2: Documented Fallback → INTENTIONAL

**Code:**
```cpp
#ifdef THEMIS_ENABLE_VULKAN
    return vulkan_impl();
#else
    // Fallback: CPU implementation on platforms without Vulkan
    return cpu_fallback();
#endif
```

**Gap Detection:**
```json
{
  "category": "platform_fallback",
  "severity": "intentional",
  "context_info": {
    "is_platform_specific": true,
    "conditional_define": "THEMIS_ENABLE_VULKAN",
    "is_fallback": true
  }
}
```

**Action:** None (by design)

### Example 3: Mock Test → LOW Priority

**Code (in test file):**
```cpp
class MockVectorIndex : public VectorIndex {
    MOCK_METHOD(Status, insert, (const Vector&), (override));
    MOCK_METHOD(void, clear, (), (override));
};
```

**Gap Detection:**
```json
{
  "category": "mock_framework",
  "severity": "low",
  "is_test": true,
  "is_mock_code": true
}
```

**Action:** None (test infrastructure)

### Example 4: TODO with Issue Link → MEDIUM Priority

**Code:**
```cpp
void optimizeQuery() {
    // TODO(#5234): Add query planner optimization for join order
    // See: https://github.com/makr-code/ThemisDB/issues/5234
    return;
}
```

**Gap Detection:**
```json
{
  "category": "todo_item",
  "severity": "medium",
  "context_info": {
    "has_documentation": true,
    "documentation": "Issue #5234 referenced"
  }
}
```

**Action:** Link to issue, track in project

---

## 📈 Interpreting Results

### Priority Matrix

| Category | Severity | Action |
|----------|----------|--------|
| Unimplemented | CRITICAL | ⚠️ Must fix before release |
| STUB (undoc) | HIGH | 📝 Add documentation template |
| TODO (linked) | MEDIUM | 🔗 Ensure issue tracking |
| STUB (doc) | INTENTIONAL | ✅ No action (by design) |
| Platform fallback | INTENTIONAL | ✅ No action (by design) |
| Mock/test | LOW | ✅ No action (test code) |
| Disabled code | LOW | 🗑️ Consider removing |

### Metrics to Track

**Monthly Audit:**
```bash
# Run scanner
python tools/gap_audit_pipeline_v2.py

# Compare with last month's results
diff gap_scan_v2_summary.json gap_scan_v2_summary.LAST_MONTH.json
```

**Key Metrics:**
- ✅ **Unimplemented count trending down** (goal: 0)
- ✅ **STUB compliance increasing** (goal: 100%)
- ✅ **TODO items decreasing** (goal: 0)
- ✅ **Intentional gaps stable or decreasing**

---

## 🤖 Automation

### CI/CD Integration

```yaml
# .github/workflows/gap-audit.yml
name: Monthly Gap Audit
on:
  schedule:
    - cron: '0 0 1 * *'  # 1st of each month
jobs:
  audit:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run gap audit
        run: |
          python tools/gap_audit_pipeline_v2.py \
            --output reports/gap-audit-$(date +%Y-%m)
      - name: Commit results
        run: |
          git add reports/gap-audit-*.json
          git commit -m "chore: monthly gap audit snapshot"
          git push
      - name: Create issue if critical gaps
        run: |
          CRITICAL=$(jq '.by_severity.critical' reports/gap-audit-*/gap_scan_v2_summary.json)
          if [ $CRITICAL -gt 100 ]; then
            gh issue create --title "⚠️ Critical Gaps Detected ($CRITICAL)" ...
          fi
```

### Git Hook: Pre-commit Gap Check

```bash
#!/bin/bash
# .git/hooks/pre-commit
python tools/gap_scanner_v2.py
if [ $? -ne 0 ]; then
    echo "❌ Gap scan failed. Run: python tools/gap_audit_pipeline_v2.py"
    exit 1
fi
```

---

## 🧪 Testing the Scanner

### Quick Test

```bash
# Scan just one module
cd src/acceleration
python ../../tools/gap_scanner_v2.py

# Or test on a single file
python -c "
from tools.gap_scanner_v2 import EnhancedGapScanner
scanner = EnhancedGapScanner('.')
gaps = scanner.scan_file('src/acceleration/gpu_kernel.cpp')
for gap in gaps[:5]:
    print(f'{gap.file_path}:{gap.line_num} {gap.category.value} - {gap.snippet}')
"
```

### Validate Against v1

```bash
# Compare results
python tools/gap_scanner.py --output reports/v1
python tools/gap_scanner_v2.py --output reports/v2

python -c "
import json
with open('reports/v1/gap_scan_aggregate.json') as f:
    v1 = json.load(f)
with open('reports/v2/gap_scan_v2_summary.json') as f:
    v2 = json.load(f)

print(f'v1 Total: {sum(m.get(\"total\", 0) for m in v1.values())}')
print(f'v2 Total: {v2[\"total_gaps\"]}')
print(f'v2 Critical: {v2[\"by_severity\"][\"critical\"]}')
print(f'v2 Intentional (false positives reduced): {v2[\"by_severity\"][\"intentional\"]}')
"
```

---

## 🎓 Design Philosophy

### Key Principles

1. **Context > Pattern** — A STUB in test code is not a production gap
2. **Intentional Design** — Platform fallbacks and conditionals are by-design
3. **False-Positive Reduction** — Better to miss a gap than waste time on noise
4. **Traceability** — Headers show current state of every file
5. **Progress Tracking** — Compare scans over time to see improvement

### When to Ignore Gaps

✅ **Safely Ignore:**
- STUB markers with 4-line documentation template
- Platform-specific fallbacks (#ifdef THEMIS_ENABLE_*)
- Mock/test framework code (GTest, GMock)
- Intentional empty bodies with documentation
- Disabled code blocks (#if 0)

❌ **Never Ignore:**
- Unimplemented paths that throw in production code
- TODO items without issue tracking
- STUB markers without documentation
- Undocumented empty functions in production

---

## 🚀 Next Steps

1. **Run v2 scanner on full repo:**
   ```bash
   python tools/gap_audit_pipeline_v2.py
   ```

2. **Review summary and top modules:**
   ```bash
   cat ai_working/gap_scan_v2_summary.json
   ```

3. **Compare with v1 results** to see false-positive reduction

4. **Set up monthly audits** for tracking progress

5. **Integrate into CI/CD** for continuous monitoring

---

## 📞 Questions?

See detailed docs:
- FINAL_SUMMARY.md — Overview and next steps
- CLUSTERED_ISSUES_REPORT.md — Full audit report
- COPILOT_INSTRUCTIONS.md § 8 — STUB/MOCK template requirement
