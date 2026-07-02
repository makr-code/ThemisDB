# Phase 1-4 & 6 Quick Reference Guide

**Status**: 🚀 Ready for immediate use  
**Last Updated**: 2026-07-02  

---

## 🎯 One-Liner Commands

### Generate Batch Issues (Replace `A` with B/C/D/E)
```bash
python3 tools/generate_github_issues_phase_1_4.py \
  --security ai_working/fp_tuning_after/gap_scan_v3_security_aggregate.json \
  --memory ai_working/fp_tuning_after/gap_scan_v3_memory_aggregate.json \
  --concurrency ai_working/fp_tuning_after/gap_scan_v3_concurrency_aggregate.json \
  --batch A --priority CRITICAL --output-json batch_A_issues.json --output-md batch_A_issues.md
```

### Run All Scanners + Generate Metrics
```bash
python3 tools/ci_phase_1_4_scanner_runner.py --repo-root . --all --output-metrics metrics.html
```

### Run Single Scanner
```bash
python3 tools/ci_phase_1_4_scanner_runner.py --repo-root . --security
python3 tools/ci_phase_1_4_scanner_runner.py --repo-root . --memory
python3 tools/ci_phase_1_4_scanner_runner.py --repo-root . --concurrency
```

### Compare with Baseline
```bash
python3 tools/ci_phase_1_4_scanner_runner.py \
  --repo-root . --all \
  --compare-baseline ai_working/baseline_scan.json \
  --output-metrics metrics_comparison.html
```

---

## 📁 Key Files & Locations

### Planning & Design Documents
```
ai_working/
├── PHASE_1_4_REMEDIATION_BATCHES.md          # Batch specifications
├── PHASE_1_4_AND_6_IMPLEMENTATION_GUIDE.md    # Detailed execution guide
├── PHASE_1_4_AND_6_EXECUTIVE_SUMMARY.md       # High-level overview
├── PHASE_6_EXTENDED_SCANNER_DESIGN.md         # Phase 6 specification
├── PHASE_1_4_COMPLETION_SUMMARY.md            # Delivery summary
└── PHASE_1_4_ENHANCEMENTS_DELIVERY_REPORT.md  # Detailed delivery report
```

### Tools
```
tools/
├── generate_github_issues_phase_1_4.py        # Issue generator
├── ci_phase_1_4_scanner_runner.py             # CI/CD integration
├── gap_scanner_v3.py                          # Main orchestrator
├── gap_scanner_v3_security.py                 # Security scanner
├── gap_scanner_v3_memory.py                   # Memory scanner
└── gap_scanner_v3_concurrency.py              # Concurrency scanner
```

### Gap Reports
```
ai_working/
├── fp_tuning_after/
│   ├── gap_scan_v3_security_aggregate.json       # Security gaps (35 modules)
│   ├── gap_scan_v3_memory_aggregate.json         # Memory gaps
│   └── gap_scan_v3_concurrency_aggregate.json    # Concurrency gaps
└── fp_tuning_before/
    └── [Previous versions for comparison]
```

---

## 🚀 Batch Execution Quickstart

### Week 28: Batch A (XXE)

**Monday (Day 1)**:
```bash
# Generate issues
python3 tools/generate_github_issues_phase_1_4.py \
  --batch A --priority CRITICAL \
  --output-json batch_A.json --output-md batch_A.md

# Create branch
git checkout -b feature/remediate-batch-A-xxe
```

**Tuesday-Thursday (Days 2-4)**:
```bash
# For each top 10 gap:
# 1. Open file at line number
# 2. Apply remediation pattern (from PHASE_1_4_REMEDIATION_BATCHES.md)
# 3. Add regression test
# 4. Commit: git commit -m "Fix XXE in security module (Batch A)"

# Run tests locally
ctest -R "batch_A" --output-on-failure

# Re-scan to verify gap closed
python3 tools/gap_scanner_v3_security.py
```

**Friday (Day 5)**:
```bash
# Push and create PR
git push origin feature/remediate-batch-A-xxe

# Generate metrics
python3 tools/ci_phase_1_4_scanner_runner.py --all \
  --compare-baseline ai_working/baseline_scan.json \
  --output-metrics batch_A_metrics.html

# After approval, merge
git checkout develop && git merge feature/remediate-batch-A-xxe
```

---

## 📊 Metrics Dashboard Commands

### Generate Weekly Report
```bash
python3 tools/ci_phase_1_4_scanner_runner.py \
  --repo-root . --all \
  --compare-baseline ai_working/baseline_scan.json \
  --output-metrics weekly_report_W28.html
```

### Track Gap Reduction
```bash
# After each batch merge:
# 1. Run scanners
python3 tools/ci_phase_1_4_scanner_runner.py --repo-root . --all

# 2. Compare with baseline
# Update PHASE_1_4_REMEDIATION_BATCHES.md with new gap counts

# 3. Archive old report
cp ai_working/fp_tuning_after/gap_scan_v3_security_aggregate.json \
   ai_working/archive/gap_scan_v3_security_W28_after_batch_A.json
```

---

## 🛠️ Troubleshooting

### Issue: "Report not found"
```bash
# Find all gap reports
find ai_working -name "gap_scan*.json" -o -name "gap_phase*.json"

# Re-run scanner
python3 tools/gap_scanner_v3_security.py
python3 tools/gap_scanner_v3_memory.py
python3 tools/gap_scanner_v3_concurrency.py
```

### Issue: "False positives too high"
```bash
# Check false positive rate
python3 tools/generate_github_issues_phase_1_4.py --batch A --dry-run

# Manually review top 20 gaps
# Update pattern filtering if needed
# Re-run scanner
```

### Issue: "New gaps introduced after fix"
```bash
# Compare current vs baseline
python3 tools/ci_phase_1_4_scanner_runner.py --repo-root . --all \
  --compare-baseline ai_working/baseline_scan.json

# If regression, revert and investigate
git revert HEAD
```

### Issue: "Batch A stuck on one gap"
1. Escalate to batch owner
2. Post in `#gap-remediation` Slack
3. Check if gap is false positive
4. Consider deferring to later batch

---

## 📋 Batch Cheat Sheet

| Batch | Pattern | Modules | Gaps | Week | Lead |
|-------|---------|---------|------|------|------|
| **A** | XXE (CWE-611) | security | 783 | W28 | Security lead |
| **B** | Format/ReDoS (CWE-134/1333) | query, security | 202 | W29 | Security + Query leads |
| **C** | Iterator Invalidation (CWE-416) | containers | 134 | W30 | Core infra lead |
| **D** | Use-After-Move (CWE-416) | STL/TX | 97 | W31 | Core infra lead |
| **E** | Concurrency (CWE-362) + misc | distributed | 20 | W32 | Distributed lead |

---

## 🔄 Remediation Pattern Templates

### Batch A: XXE Fix Template
```cpp
// Before (vulnerable)
xmlDocPtr doc = xmlParseFile(filename);  // No XXE protection

// After (fixed)
xmlDocPtr doc = xmlParseFile(filename);
xmlParserSetFeature(parser, XML_PARSE_NONET);  // Disable network
xmlParserSetFeature(parser, XML_PARSE_NOENT, 0);  // Disable entity expansion
```

### Batch B: Format String Fix Template
```cpp
// Before (vulnerable)
printf(user_input);  // User input as format string

// After (fixed)
printf("%s", user_input);  // Controlled format string
```

### Batch C: Iterator Fix Template
```cpp
// Before (vulnerable)
for (auto it = vec.begin(); it != vec.end(); ++it) {
    vec.push_back(*it);  // Iterator invalidated
}

// After (fixed)
for (size_t i = 0; i < vec.size(); ++i) {
    vec.push_back(vec[i]);  // Index-based, safe
}
```

### Batch D: Move Fix Template
```cpp
// Before (vulnerable)
T t = std::move(u);
use(u);  // Use-after-move

// After (fixed)
T t = std::move(u);
// Don't use u after move
```

### Batch E: Concurrency Fix Template
```cpp
// Before (vulnerable)
cv.wait();  // No lock held

// After (fixed)
{
    std::lock_guard<std::mutex> lock(mtx);
    cv.wait(lock);  // Lock held during wait
}
```

---

## 🎯 Phase 6 Scanner Command

### Run All Phase 6 Scanners (Ready Week 32)
```bash
python3 tools/gap_scanner_v3_phase6_runner.py --repo-root . --all
```

### Run Individual Phase 6 Scanners
```bash
python3 tools/gap_scanner_v3_type_conversion.py
python3 tools/gap_scanner_v3_input_validation.py
python3 tools/gap_scanner_v3_exception_safety.py
python3 tools/gap_scanner_v3_uninitialized.py
python3 tools/gap_scanner_v3_oop_design.py
```

---

## 📞 Support & Resources

### Slack Channel
`#gap-remediation` — All questions, updates, blockers

### Document References
- **Quick Start**: This file (PHASE_1_4_AND_6_QUICK_REFERENCE.md)
- **Detailed Guide**: PHASE_1_4_AND_6_IMPLEMENTATION_GUIDE.md
- **Executive Summary**: PHASE_1_4_AND_6_EXECUTIVE_SUMMARY.md
- **Batch Specs**: PHASE_1_4_REMEDIATION_BATCHES.md
- **Phase 6 Design**: PHASE_6_EXTENDED_SCANNER_DESIGN.md

### Weekly Sync
- **Time**: Tuesday 10:00 UTC
- **Duration**: 30 minutes
- **Attendees**: Batch owners + leads

---

## ✅ Before You Start

- [ ] Read PHASE_1_4_AND_6_IMPLEMENTATION_GUIDE.md (15 min)
- [ ] Understand your batch or scanner assignment (10 min)
- [ ] Test tools locally (10 min)
- [ ] Confirm team availability (5-week commitment)
- [ ] Join `#gap-remediation` Slack channel
- [ ] Set up daily standup cadence

---

**Status**: 🚀 READY FOR EXECUTION  
**Questions?**: Post in `#gap-remediation` Slack  

---

**Created**: 2026-07-02  
**Version**: 1.0
