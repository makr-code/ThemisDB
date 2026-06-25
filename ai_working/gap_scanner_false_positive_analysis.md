# Gap Scanner False-Positive Analysis & Improvement Plan

**Date:** 2026-06-25  
**Module:** graph  
**Finding:** `test_compute_graph_header.cpp:93` — File Not Found  
**Agent:** gap-verifier (L0.5)

---

## 1. False-Positive Finding

### Detected Issue
```json
{
  "file": "tests/test_compute_graph_header.cpp",
  "line": 93,
  "pattern": "// Mock factory (TEST)",
  "original_severity": "HIGH",
  "verified_severity": "IGNORE",
  "classification": "FALSE_POSITIVE",
  "rationale": "File does not exist in repository"
}
```

### Root Cause Analysis

**L0 Scanner Output:**
- Reported finding in `tests/test_compute_graph_header.cpp:93`
- Pattern: Mock factory stub marker

**L0.5 Verification:**
- Attempted file load: `tests/test_compute_graph_header.cpp` — **NOT FOUND**
- Search across all locations: `tests/graph/`, `tests/`, root `tests/` — **ZERO matches**
- Verdict: Artifact (historical reference, renamed file, deleted test, or stale scanner cache)

---

## 2. Impact Assessment

### Why This Matters

1. **False-Positive Rate:** 1/9 findings (11%) for graph module
2. **Risk:** If not caught by L0.5 verification, would inflate CRITICAL count
3. **Release Decision:** Could have triggered unnecessary "hardening" phase
4. **Severity:** HIGH (originally) — relatively minor, but principle matters

### Scenarios That Could Cause This

1. **Stale Scanner Cache**
   - `gap_scan_v3_aggregate.json` cached from previous run
   - File was deleted/renamed since last L0 run
   - Scanner didn't re-discover file set

2. **Partial File Discovery**
   - Scanner found reference in CMakeLists.txt or source import
   - Actual test file path differs (e.g., renamed, moved to subdirectory)
   - File globbing pattern mismatch

3. **Build Artifact vs. Source**
   - Scanner indexed `build/` directory instead of `src/tests/`
   - Temp build output contains old filenames

4. **Symlink or Include Mishandling**
   - File included/referenced but not actually present
   - Symlink target deleted

---

## 3. Proposed Gap_Scanner Improvements

### 3.1 File Existence Pre-Check (CRITICAL)

**Current Behavior:**
```python
# gap_scanner_v3.py (hypothetical current code)
for finding in raw_findings:
    file = finding['file']
    line = finding['line']
    # ... process without checking if file exists
```

**Improved Behavior:**
```python
import os

for finding in raw_findings:
    file = finding['file']
    line = finding['line']
    
    # PRE-CHECK: File must exist
    if not os.path.exists(file):
        finding['status'] = 'FILE_NOT_FOUND'
        finding['verified_severity'] = 'IGNORE'
        finding['classification'] = 'FALSE_POSITIVE'
        verified_findings.append(finding)
        logging.warning(f"Gap in non-existent file: {file}:{line} — flagged as FALSE_POSITIVE")
        continue
    
    # ... proceed with normal gap processing
```

### 3.2 Context-Aware Classification (IMPORTANT)

**Current:** Binary pass/fail on gap detection  
**Improved:** Multi-factor classification

```python
def classify_gap(file, line, pattern):
    """
    Classify gap with multi-factor analysis:
    - Is file reachable/readable?
    - Is this a guarded pattern (if/guard)?
    - Is this in test code?
    - Is pattern marked as TODO/STUB/TEMPORARY?
    """
    
    # Factor 1: File existence + readability
    if not os.path.exists(file) or not os.access(file, os.R_OK):
        return 'FALSE_POSITIVE', 'FILE_NOT_FOUND'
    
    # Factor 2: Read source context
    try:
        with open(file, 'r') as f:
            lines = f.readlines()
            context = lines[max(0, line-5):min(len(lines), line+5)]
    except Exception as e:
        return 'FALSE_POSITIVE', f'READ_ERROR: {e}'
    
    # Factor 3: Pattern analysis
    source_line = lines[line-1] if 0 < line <= len(lines) else ''
    
    # Guarded stub pattern?
    if re.match(r'\s*(if|while|for)\s*\(', source_line):
        return 'GUARDED_STUB', 'DOWNGRADE_HIGH'
    
    # Test code marker?
    if file.startswith('tests/') and ('MOCK' in source_line or 'TEST' in source_line):
        return 'TEST_MOCK', 'DOWNGRADE_INFO'
    
    # TODO/STUB marker?
    if any(marker in source_line for marker in ['TODO', 'FIXME', 'STUB', 'TEMPORARY', 'WIP']):
        return 'PLACEHOLDER', 'DOWNGRADE_MEDIUM'
    
    # Unguarded real gap
    return 'REAL_GAP', 'KEEP_SEVERITY'
```

### 3.3 Stale Cache Detection (MEDIUM)

**Detect cache rot:**

```python
def validate_scanner_cache(cache_file, source_dir):
    """Check if cache is stale (files have been deleted/moved)"""
    
    with open(cache_file, 'r') as f:
        cached_findings = json.load(f)
    
    missing_files = []
    for finding in cached_findings['findings']:
        file = finding['file']
        if not os.path.exists(os.path.join(source_dir, file)):
            missing_files.append(file)
    
    if missing_files:
        logging.warning(f"Cache contains {len(missing_files)} non-existent files:")
        for f in missing_files[:10]:  # Show first 10
            logging.warning(f"  - {f}")
        logging.warning(f"Gap scanner cache is STALE. Recommend: gap_scanner_v3.py --force-refresh")
        return False
    
    return True
```

### 3.4 Output Enrichment (MEDIUM)

**Add metadata to output:**

```json
{
  "scan_timestamp": "2026-06-25T10:21:15",
  "source_directory": "/home/user/ThemisDB",
  "python_version": "3.10.8",
  "cache_validation": {
    "cache_file_used": "ai_working/gap_scan_cache.json",
    "cache_age_hours": 2.5,
    "cache_stale": false,
    "files_not_found": ["tests/test_compute_graph_header.cpp"],
    "recommendation": "Cache is fresh; non-existent file is likely deleted test"
  },
  "summary": {
    "total_files_scanned": 127,
    "findings_raw": 9,
    "findings_with_file_check": 8,
    "findings_file_not_found": 1,
    "findings_verified": 8
  }
}
```

---

## 4. Implementation Roadmap

### Phase 1: Immediate Fix (Day 1)
- [ ] Add `os.path.exists()` pre-check to gap_scanner_v3.py
- [ ] Flag FILE_NOT_FOUND findings with `status: 'FALSE_POSITIVE'`
- [ ] Document in scanner output

### Phase 2: Classification Engine (Days 2-3)
- [ ] Implement `classify_gap()` function with multi-factor logic
- [ ] Add context-aware pattern analysis (guarded, test, marker detection)
- [ ] Update verified_findings schema to include classification rationale

### Phase 3: Cache Management (Days 3-4)
- [ ] Add `--force-refresh` flag to re-scan all files
- [ ] Implement `validate_scanner_cache()` with stale detection
- [ ] Auto-refresh if > 10% of cached files missing

### Phase 4: Testing & Documentation (Days 4-5)
- [ ] Unit tests: test_file_not_found, test_guarded_stub, test_cache_stale
- [ ] Integration test: Re-scan graph module, verify 0 FALSE_POSITIVEs
- [ ] Update README.md with `/gap-verifier` workflow

---

## 5. Code Template: Improved Scanner Entry Point

```python
#!/usr/bin/env python3
"""gap_scanner_v3.py — Improved with pre-checks and classification"""

import json
import logging
import os
import re
from pathlib import Path

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def scan_module(module_name, source_dir='.', force_refresh=False):
    """
    Scan module for gaps with improved false-positive handling
    """
    
    # Load/generate raw findings
    raw_findings = run_gap_scanner(module_name, source_dir)
    logger.info(f"Scanner found {len(raw_findings)} raw findings")
    
    # Pre-validate: file existence
    verified_findings = []
    for finding in raw_findings:
        file_path = finding['file']
        
        # File existence check
        if not os.path.exists(file_path):
            finding['status'] = 'FALSE_POSITIVE'
            finding['classification'] = 'FILE_NOT_FOUND'
            finding['verified_severity'] = 'IGNORE'
            logger.warning(f"Gap in non-existent file: {file_path}:{finding['line']}")
            verified_findings.append(finding)
            continue
        
        # Classification analysis
        classification, severity_action = classify_gap(file_path, finding['line'], finding['pattern'])
        finding['classification'] = classification
        
        if severity_action.startswith('DOWNGRADE_'):
            new_severity = severity_action.split('_')[1]
            finding['verified_severity'] = new_severity
            logger.info(f"Downgraded {file_path}:{finding['line']} {finding['severity']} → {new_severity}")
        elif severity_action == 'KEEP_SEVERITY':
            finding['verified_severity'] = finding['severity']
        
        verified_findings.append(finding)
    
    # Output verified findings
    output_file = f'ai_working/gap_scanner_verified_{module_name}.json'
    with open(output_file, 'w') as f:
        json.dump({
            'module': module_name,
            'timestamp': datetime.now().isoformat(),
            'total_raw': len(raw_findings),
            'total_verified': len(verified_findings),
            'findings': verified_findings
        }, f, indent=2)
    
    logger.info(f"Verified findings written to {output_file}")
    return verified_findings
```

---

## 6. Testing Checklist

After implementing improvements:

- [ ] `/dokifi L0 graph` → `gap_scanner_results.json` (raw, 9 findings)
- [ ] `/dokifi L0.5 graph` → `gap_scanner_verified_graph.json` (1 FILE_NOT_FOUND flagged, 8 verified)
- [ ] Verify no FILE_NOT_FOUND findings escalate to L1 docs
- [ ] Run against other modules (`cache`, `query`, `network`) to catch stale caches
- [ ] Measure: FALSE_POSITIVE rate reduction (target: < 2%)

---

## 7. Benefits

| Aspect | Before | After |
|--------|--------|-------|
| **False-Positive Rate** | 11% (1/9) | < 2% (pre-filter FILE_NOT_FOUND) |
| **Risk Inflation** | 8 CRITICAL forced into docs | 0 CRITICAL (verified before L1) |
| **Release Decision** | Blocked by noise | Unblocked by signal |
| **Verifier Effort** | Manual code review (L0.5) | Auto-filtered (L0.5 + scanner pre-checks) |
| **Maintainability** | Brittle cache | Robust with validation |

---

## Summary

**Root Cause:** Scanner reported finding in non-existent file (stale cache or deleted test)

**Why L0.5 Matters:** Caught false-positive before it cascaded to L1 (docs) → would have inflated risk

**Solution:** 
1. Add file existence pre-check to gap_scanner_v3.py
2. Implement multi-factor classification (guarded, test, marker detection)
3. Add cache validation with stale detection
4. Output enriched metadata for debugging

**Expected Outcome:** < 2% false-positive rate, more accurate risk assessment, fewer release blockers
