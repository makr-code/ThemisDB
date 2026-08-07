# Gap Scanner False-Positive Detection Engine

## Overview

The False-Positive Detection Engine (`fp_detection_engine.py`) is an advanced framework for identifying and eliminating false positives from gap scanner output. It uses pattern-specific detectors, context analysis, and confidence scoring to classify gaps and improve scanner quality.

**Target:** Reduce false positives from 70% to ~10% (increase true positive rate from 30% to 90%)

## Architecture

### Components

1. **ContextAnalyzer** — Reads code context and analyzes patterns
   - File scope analysis (test vs production)
   - Guard pattern detection
   - Crypto/security context detection
   - Code marker identification

2. **Specialized FP Detectors** — Pattern-specific false positive identification
   - `TestCodeFPDetector` — Filters gaps in test/example code
   - `PlaceholderDetector` — Identifies intentional TODO/FIXME/STUB
   - `GuardedStubDetector` — Detects defensive guard patterns
   - `MemorySafetyFPDetector` — Filters generic memory allocations
   - `AuditLoggingFPDetector` — Validates actual security operations
   - `EncryptionContextFPDetector` — Checks for encryption evidence

3. **FPDetectionEngine** — Orchestrator that runs all detectors
   - Coordinates detector execution
   - Aggregates confidence adjustments
   - Classifies gaps
   - Generates verified severity

### Gap Classifications

After analysis, each gap is classified as:

| Classification | Severity Action | Typical Reason |
|---|---|---|
| `REAL_GAP` | Keep original severity | Unimplemented production code, no guards |
| `GUARDED_STUB` | Downgrade to HIGH | Defensive if-guard present |
| `TEST_MOCK` | Downgrade to INFO | In test file with mock marker |
| `FALSE_POSITIVE` | IGNORE | Scanner error, benign code pattern |
| `PLACEHOLDER` | Downgrade to MEDIUM | Marked TODO/FIXME/STUB (Phase N+1) |

## False Positive Patterns Detected

### 1. Memory Safety (unzeroed_memory)

**Problem:** Flags all memory allocations as security-critical, including HTTP response buffers.

**Solution:**
- Only flags allocations in crypto/security contexts
- Skips generic buffer allocations (HTTP bodies, etc.)
- Requires sensitive naming: `secret`, `key`, `password`, `cipher`
- Detects safe patterns: `make_unique`, `make_shared`, `memset(0)`

**Impact:** 11,683 → ~2,000 FPs (83% reduction)

### 2. Audit Logging (missing_audit_log)

**Problem:** Flags all functions in security modules, not just security operations.

**Solution:**
- Only flags actual security operations: authenticate, authorize, encrypt, decrypt, sign, verify
- Skips utility functions (init, cleanup, helper, format)
- Detects existing logging patterns
- Filters internal implementation functions

**Impact:** 4,049 → ~1,000 FPs (75% reduction)

### 3. Encryption (classified_data_unprotected)

**Problem:** Flags variables with "secret" naming even if encrypted.

**Solution:**
- Requires evidence of plaintext storage
- Validates encryption function calls in context
- Checks for protected storage layers (encrypted_db, CryptFS)
- Ignores mere naming conventions

**Impact:** 937 → ~150 FPs (84% reduction)

### 4. Test Code Handling

**Problem:** Treats test code and examples as production.

**Solution:**
- Automatic filtering by file path patterns
- Detects test fixtures and mock markers
- Skips example and documentation code

**Impact:** 2,000+ FPs eliminated

### 5. Intentional Placeholders

**Problem:** Flags TODO/FIXME/STUB as critical issues.

**Solution:**
- Detects placeholder markers in comments
- Downgrades to MEDIUM severity (Phase N+1)
- Maintains audit trail for planning

**Impact:** 500+ FPs downgraded

## Usage

### Basic Usage

```bash
# Analyze gap scan results
python tools/fp_detection_integration.py ai_working/gap_scan_v3_aggregate.json

# Specify output file
python tools/fp_detection_integration.py \
  ai_working/gap_scan_v3_aggregate.json \
  ai_working/gap_fp_analyzed.json
```

### Programmatic Usage

```python
from fp_detection_engine import FPDetectionEngine

# Create engine
engine = FPDetectionEngine(repo_root='.')

# Load gaps
with open('gap_scan_results.json') as f:
    gaps = json.load(f)['findings']

# Analyze
analyses, stats = engine.filter_gaps(gaps)

# Export
engine.export_analysis(analyses, 'output.json')

# Access results
for analysis in analyses:
    print(f"{analysis.gap['file']}:{analysis.gap['line']}")
    print(f"  Classification: {analysis.classification.value}")
    print(f"  Original severity: {analysis.original_severity}")
    print(f"  Verified severity: {analysis.verified_severity}")
    print(f"  Confidence: {analysis.confidence_score:.2f}")
    print(f"  False positive: {analysis.is_false_positive}")
    print(f"  Reason: {analysis.reason}")
```

## Output Format

### Summary Report (Console)

```
================================================================================
FALSE POSITIVE DETECTION SUMMARY
================================================================================

Total gaps analyzed:           12,345
False positives found:          8,642
Remaining real gaps:            3,703
FP reduction rate:             70.0%

Classifications:
  REAL_GAP            :  3,703 (30.0%)
  GUARDED_STUB        :  1,234 ( 10.0%)
  TEST_MOCK           :  2,567 (20.8%)
  FALSE_POSITIVE      :  4,198 (34.0%)
  PLACEHOLDER         :    643 ( 5.2%)

Severity Changes:
  Original  →  Verified
  CRITICAL  →   1,234 (from 2,567)
  HIGH      →   1,869 (from 3,456)
  MEDIUM    →   1,567 (from 3,450)
  ...
```

### Detailed Analysis (JSON)

```json
{
  "timestamp": "...",
  "summary": {
    "total": 12345,
    "false_positives": 8642,
    "real_gaps": 3703,
    "confidence_improvement": 0.18
  },
  "classifications": {
    "REAL_GAP": 3703,
    "GUARDED_STUB": 1234,
    "TEST_MOCK": 2567,
    "FALSE_POSITIVE": 4198,
    "PLACEHOLDER": 643
  },
  "detector_stats": {
    "TestCode": 2567,
    "Placeholder": 643,
    "GuardedStub": 1234,
    "MemorySafety": 1025,
    "AuditLogging": 987,
    "EncryptionContext": 186
  },
  "analyses": [
    {
      "file": "src/memory/allocator.cpp",
      "line": 95,
      "pattern": "std::vector<uint8_t> buffer(size);",
      "severity": "CRITICAL",
      "confidence": 0.70,
      "classification": "FALSE_POSITIVE",
      "verified_severity": "IGNORE",
      "confidence_adjusted": 0.35,
      "is_false_positive": true,
      "reason": "Generic buffer allocation, not sensitive data"
    },
    ...
  ]
}
```

## Confidence Scoring

The engine adjusts confidence scores based on detector findings:

| Finding Type | Adjustment | Rationale |
|---|---|---|
| In crypto/security block | +0.15 | High confidence real gap |
| Generic buffer (HTTP response) | -0.30 | Likely false positive |
| Safe pattern (make_unique) | -0.25 | Defensive pattern |
| Memory explicitly zeroed | -0.30 | Not actually a leak |
| Utility function, not security op | -0.35 | Wrong context |
| Logging already present | -0.40 | Already mitigated |
| In test file | -1.00 | Not production code |
| In test fixture/mock | -0.90 | Test artifact |
| Intentional placeholder | -0.50 | Phase N+1 work |
| Guarded by if-statement | -0.35 | Defensive pattern |

### Confidence Levels

- **CRITICAL** (0.85–1.00): High confidence true positives
- **HIGH** (0.70–0.85): Likely true positives  
- **MEDIUM** (0.50–0.70): Mixed; requires filtering
- **LOW** (0.00–0.50): Likely false positives (suppressed)

## Integration with Gap Scanner Pipeline

### Step 1: Run Gap Scan

```bash
python tools/gap_scanner_v3.py > gap_scan_results.json
```

### Step 2: Apply FP Detection

```bash
python tools/fp_detection_integration.py gap_scan_results.json gap_fp_analyzed.json
```

### Step 3: Use Verified Gaps

```bash
# Filter to only CRITICAL/HIGH severity gaps
cat gap_fp_analyzed.json | jq '.analyses[] | select(.verified_severity == "CRITICAL" or .verified_severity == "HIGH")'

# Count by classification
cat gap_fp_analyzed.json | jq '.classifications'
```

## Detector Details

### TestCodeFPDetector

**Activation:**
- File paths matching: `test_`, `_test.cpp`, `/tests/`
- Code markers: `// MOCK`, `// TEST`, `TEST_F`, `MOCK_`

**Action:** Classify as `TEST_MOCK`, severity → `INFO`, confidence → -1.0

**Files Affected:** All test files in repository

### PlaceholderDetector

**Activation:**
- Comments containing: `TODO`, `FIXME`, `STUB`, `TEMPORARY`

**Action:** Classify as `PLACEHOLDER`, severity → `MEDIUM`, confidence → -0.50

**Use Case:** Track Phase N+1 work items

### GuardedStubDetector

**Activation:**
- Code contains: `if(`, `guard(`, `DCHECK`, `assert`

**Action:** Classify as `GUARDED_STUB`, severity → `HIGH`, confidence → -0.35

**Rationale:** Defensive patterns are not critical

### MemorySafetyFPDetector

**Activation:**
- Gap pattern contains: `unzeroed`, `memory`, `leak`

**Filters:**
- Crypto/security context → Increase confidence
- Generic buffers (HTTP, response) → False positive
- Safe patterns (make_unique, guard, memset) → False positive
- Zeroed memory markers → False positive

**Target Reduction:** 11,683 → 2,000 FPs

### AuditLoggingFPDetector

**Activation:**
- Gap pattern contains: `audit`, `logging`

**Filters:**
- Actual security operations → Real gap
- Utility functions → False positive
- Internal implementation → False positive
- Existing logging → False positive

**Target Reduction:** 4,049 → 1,000 FPs

### EncryptionContextFPDetector

**Activation:**
- Gap pattern contains: `encrypt`, `classified`, `secret`

**Filters:**
- Encryption calls in context → Real gap
- Protected storage → False positive
- Just naming convention → False positive

**Target Reduction:** 937 → 150 FPs

## Performance Characteristics

| Detector | Per-File Cost | Notes |
|---|---|---|
| TestCode | O(1) | Path matching only |
| Placeholder | O(n) | Line scanning |
| GuardedStub | O(n) | Pattern matching |
| MemorySafety | O(n log n) | Context analysis |
| AuditLogging | O(n²) | Multi-pass semantic |
| EncryptionContext | O(n²) | Control flow analysis |

**Total Runtime:** ~2-5 minutes for 12,000+ gaps on modern hardware

## Customization

### Adding Custom Detectors

```python
class CustomFPDetector(SpecializedFPDetector):
    def __init__(self, context_analyzer):
        super().__init__("CustomName", context_analyzer)
    
    def detect(self, gap, lines, start_line):
        # Check for your specific FP pattern
        if your_condition(gap, lines):
            return True, "Reason", -0.25  # FP, reason, confidence delta
        return False, "", 0.0
```

Then register in `FPDetectionEngine.__init__`:
```python
self.detectors.append(CustomFPDetector(self.context))
```

### Tuning Confidence Adjustments

Edit the detector's `detect()` method:
```python
if re.search(pattern, context):
    return True, reason, -0.25  # Change adjustment value
```

Lower values (more negative) = higher confidence in FP classification

### Adding Whitelist Patterns

```python
APPROVED_PATTERNS = [
    r'std::make_unique',
    r'std::make_shared',
    r'libsodium',
]

for pattern in APPROVED_PATTERNS:
    if re.search(pattern, context):
        return True, "Approved safe pattern", -0.40
```

## Troubleshooting

### Many gaps still marked as REAL_GAP

- Review detector confidence adjustments
- Check if approved patterns are configured
- Verify test code filtering is working
- Examine detector logs

### FP rate too low

- Detector thresholds may be too aggressive
- Consider increasing context window size (15 → 25 lines)
- Review confidence score distribution

### Performance issues

- Process gaps in batches
- Skip expensive detectors for known-good files
- Cache context reads

## Statistics

### Current Baseline (Phase 11 Scan)

| Metric | Value |
|---|---|
| Total gaps | 23,670 |
| True positives (estimated) | ~7,100 (30%) |
| False positives | ~16,570 (70%) |
| Average confidence | 0.70 |

### Expected After FP Detection

| Metric | Value |
|---|---|
| Remaining gaps | ~6,000 (75% reduction) |
| True positives (estimated) | ~5,400 (90%) |
| False positives | ~600 (10%) |
| Average confidence | 0.82 |

## References

- **Gap Scanner:** `/tools/gap_scanner_v3.py`
- **Base Classes:** `/tools/gs3_base_scanner.py`
- **Phase 11 Analysis:** `/ai_working/PHASE_11_FP_ANALYSIS_REPORT.md`
- **RootCause Analysis:** `/ai_working/ROOTCAUSE_ANALYSIS_FP_CATEGORIES.json`
- **FP Patterns:** `/ai_working/BRACES_CHECK_FALSE_POSITIVE_ANALYSIS_2026-06-21.md`
