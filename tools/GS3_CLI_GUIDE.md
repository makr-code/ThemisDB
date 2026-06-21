# GS3 CLI User Guide

**Gap Scanner V3 — Unified Command-Line Interface**

---

## Overview

The GS3 CLI provides a unified interface for all gap scanning operations in ThemisDB. All scanners are organized into 4 execution phases with consistent naming conventions.

```
gs3_step<N>_<category>_<name>.py

Categories by phase:
  Phase 1: ai (AI-Vibe)     | core (C++ baseline)    | check (syntax)
  Phase 2: safety (exception & input)
  Phase 3: security (cryptography & hardening)
  Phase 4: design (architecture) | quality (documentation)
```

---

## Quick Start

### 1. View All Available Scanners
```bash
python tools/gs3.py list-scanners
```

**Output**: Shows all 46 scanners organized by phase and category:
- Phase 1: 18 baseline scanners (5 AI + 2 Check + 11 Core C++)
- Phase 2: 5 safety scanners
- Phase 3: 7 security scanners
- Phase 4: 16 design + quality scanners

### 2. Run a Full Codebase Scan
```bash
python tools/gs3.py scan src include tests benchmarks \
  --scan-mode fast \
  --output results.json \
  --md-report results.md
```

**Output**:
- JSON file: `results.json` (machine-readable)
- Markdown: `results.md` (human-readable summary)
- Console: Real-time progress and summary statistics

### 3. Generate a Report from Existing Scan
```bash
python tools/gs3.py report results.json \
  --format md \
  --output report.md
```

### 4. View Configuration
```bash
python tools/gs3.py config --show
```

---

## Command Reference

### Main Subcommands

#### `gs3 scan` — Run gap scanning pipeline

```bash
python tools/gs3.py scan <directories...> [options]
```

**Positional Arguments**:
```
directories       One or more directories to scan
                  Examples: src, include, tests, benchmarks
```

**Options**:
```
--scan-mode {fast,thorough}
  fast           Quick scan (recommended for CI)
  thorough       Complete analysis (more findings, slower)
  Default: thorough

--output OUTPUT
  Write JSON results to file
  Default: results.json

--md-report MD_REPORT
  Generate Markdown report
  Default: scan_results.md

-v, --verbose
  Print detailed scan progress
  Default: quiet mode
```

**Examples**:

```bash
# Scan all directories
python tools/gs3.py scan src include tests benchmarks

# Fast mode for CI pipelines
python tools/gs3.py scan src --scan-mode fast

# With verbose output
python tools/gs3.py scan src -v

# Custom output paths
python tools/gs3.py scan src \
  --output my_scan.json \
  --md-report my_report.md

# Scan single directory
python tools/gs3.py scan include
```

**Output**:
```
[OK] Results exported to results.json
[OK] Markdown report exported to scan_results.md
[OK] Completed in 125.34s

[SUMMARY]
Total gaps: 28884

By Severity:
  CRITICAL: 1598
  HIGH: 2916
  MEDIUM: 18500
  LOW: 5870

By Scope:
  themis_core: 126 (0.44%)
  themis_tests: 0 (0.0%)
  third_party: 28758 (99.56%)

Top Gap Types:
  scope_mismatch: 15005
  missing_doxygen_comment: 3157
  todo_as_productionlogic: 3010
  ...
```

---

#### `gs3 report` — Generate reports from scan results

```bash
python tools/gs3.py report <scan_file> [options]
```

**Positional Arguments**:
```
scan_file         Path to JSON scan results file
```

**Options**:
```
--format {json,md}
  json           Machine-readable format
  md             Markdown (human-readable)
  Default: md

--output OUTPUT
  Write report to file
  Default: stdout (console)
```

**Examples**:

```bash
# Generate Markdown report
python tools/gs3.py report results.json --format md

# Generate JSON report
python tools/gs3.py report results.json --format json

# Write to file
python tools/gs3.py report results.json \
  --format md \
  --output my_report.md

# Print to console (default)
python tools/gs3.py report results.json
```

**Output** (Markdown format):
```markdown
# Gap Scanner Report

## Summary
- Total Findings: 28,884
- P0 Blockers: 0
- P1 High Risk: 3
- Production Ready: ✓ YES

## By Severity
| Severity | Count | Percentage |
|----------|-------|-----------|
| CRITICAL | 1,598 | 5.5% |
| HIGH     | 2,916 | 10.1% |
| MEDIUM   | 18,500| 64.1% |
| LOW      | 5,870 | 20.3% |

## Gap Types
1. scope_mismatch: 15,005 (52%)
2. missing_doxygen_comment: 3,157 (11%)
3. todo_as_productionlogic: 3,010 (10%)
...
```

---

#### `gs3 list-scanners` — Show registered scanners

```bash
python tools/gs3.py list-scanners [options]
```

**Options**:
```
--step {0,1,2,3,4}
  Filter by execution phase
  0: Meta-orchestrator
  1: Baseline detection
  2: Context-aware analysis
  3: Security & crypto
  4: Design & quality rules
```

**Examples**:

```bash
# List all 46 scanners
python tools/gs3.py list-scanners

# List only Phase 1 baseline scanners
python tools/gs3.py list-scanners --step 1

# List only security scanners
python tools/gs3.py list-scanners --step 3
```

**Output**:
```
====================================================================================================
GAP SCANNER V3 - REGISTERED SCANNERS
====================================================================================================

===============================================Step 1===============================================

  Step01 Ai Error Handling Consistency
  Step01 Ai Header Drift
  Step01 Ai Llm Prompt Injection
  Step01 Ai Simulation Stub Leak
  Step01 Ai Todo Productionlogic
  Step01 Check Braces
  Step01 Check Namespace Unity
  Step01 Core Concurrency
  Step01 Core Container
  Step01 Core Error Handling
  Step01 Core Memory Safety
  Step01 Core Memory
  Step01 Core Performance
  Step01 Core Platform
  Step01 Core Raii
  Step01 Core Reliability
  Step01 Core Security
  Step01 Core Thread Safety

===============================================Step 2===============================================

  Step02 Safety Exception
  Step02 Safety Input Validation
  Step02 Safety Type Conversion
  Step02 Safety Uninitialized
  Step02 Safety Virtual Oop

===============================================Step 3===============================================

  Step03 Security Attack Vectors
  Step03 Security Data Leak
  Step03 Security E2E Encryption
  Step03 Security Encryption Leak
  Step03 Security Key Failure
  Step03 Security Legacy Duplication
  Step03 Security Military Hardening

===============================================Step 4===============================================

  Step04 Design Architecture
  Step04 Design Bridge Interface
  Step04 Design Deprecated Apis
  Step04 Design Determinism
  Step04 Design Distributed Consistency
  Step04 Design Error Rules
  Step04 Design Gpu Memory
  Step04 Design Llm Ai Safety
  Step04 Design Module Governance
  Step04 Design Observability
  Step04 Design Performance Patterns
  Step04 Design Query Correctness
  Step04 Quality Audit Logging
  Step04 Quality Cpp Doxygen
  Step04 Quality Doc Freshness
  Step04 Quality Docs Markdown

46 scanners found
====================================================================================================
```

---

#### `gs3 config` — Manage GS3 configuration

```bash
python tools/gs3.py config [options]
```

**Options**:
```
--show          Display current configuration
--edit          Open configuration in VS Code
```

**Examples**:

```bash
# Show current config
python tools/gs3.py config --show

# Edit config in VS Code
python tools/gs3.py config --edit
```

---

## Use Cases

### Use Case 1: Full Codebase Audit

```bash
# Run complete scan with all phases
python tools/gs3.py scan src include tests benchmarks \
  --scan-mode thorough \
  --output audit_results.json \
  --md-report audit_report.md \
  -v
```

**When to use**: Pre-release reviews, major refactoring, architectural changes

---

### Use Case 2: CI/CD Pipeline Integration

```bash
# Fast scan suitable for CI
python tools/gs3.py scan src \
  --scan-mode fast \
  --output ci_results.json

# Check for blockers
if grep -q '"severity":"CRITICAL"' ci_results.json; then
  echo "ERROR: Critical issues found"
  python tools/gs3.py report ci_results.json --format md
  exit 1
fi
```

**When to use**: Continuous integration, pull request validation

---

### Use Case 3: Module-Specific Analysis

```bash
# Scan specific module
python tools/gs3.py scan src/llm \
  --output llm_scan.json \
  --md-report llm_report.md

# Generate human-readable report
python tools/gs3.py report llm_scan.json --format md
```

**When to use**: Module development, isolated testing

---

### Use Case 4: Trend Analysis

```bash
# Run multiple scans over time
for date in 2026-06-20 2026-06-21 2026-06-22; do
  python tools/gs3.py scan src \
    --scan-mode fast \
    --output "results_${date}.json"
done

# Compare findings
python tools/gs3.py report results_2026-06-20.json --format json > old.json
python tools/gs3.py report results_2026-06-22.json --format json > new.json
# (Use external diff tools to compare)
```

**When to use**: Tracking progress, measuring improvement

---

## Scanner Architecture

### Phase 1: Baseline Detection (18 scanners)

**Purpose**: Detect fundamental C++ issues

**Categories**:
- **AI (5 scanners)**: AI-Vibe specific issues
  - `gs3_step01_ai_error_handling_consistency.py`
  - `gs3_step01_ai_header_drift.py`
  - `gs3_step01_ai_llm_prompt_injection.py`
  - `gs3_step01_ai_simulation_stub_leak.py`
  - `gs3_step01_ai_todo_productionlogic.py`

- **Check (2 scanners)**: Syntactic validation
  - `gs3_step01_check_braces.py`
  - `gs3_step01_check_namespace_unity.py`

- **Core (11 scanners)**: C++ baseline issues
  - Concurrency, containers, memory, performance, platform
  - RAII, reliability, security, error handling, thread safety

### Phase 2: Context-Aware Analysis (5 scanners)

**Purpose**: Deeper analysis requiring context

**Category**: Safety
- Exception safety, input validation, type conversion
- Uninitialized variables, OOP patterns

### Phase 3: Security & Cryptography (7 scanners)

**Purpose**: Security-critical issues

**Category**: Security
- Attack vectors, data leaks, encryption
- Key failures, legacy paths, military hardening

### Phase 4: Design & Architecture Rules (16 scanners)

**Purpose**: High-level architecture compliance

**Categories**:
- **Design (12 scanners)**: Architecture governance
- **Quality (4 scanners)**: Documentation standards

---

## Output Files

### JSON Scan Results (`results.json`)

Machine-readable format containing all findings:

```json
{
  "metadata": {
    "scanner": "UniformFullScanner v4.0",
    "scan_date": "2026-06-21T21:30:00Z",
    "directories": ["src", "include", "tests", "benchmarks"],
    "total_files": 1234,
    "total_findings": 28884
  },
  "gaps": [
    {
      "file": "src/core/engine.cpp",
      "line": 42,
      "type": "todo_as_productionlogic",
      "severity": "CRITICAL",
      "confidence": 0.95,
      "impact_level": "CRITICAL",
      "subsystem": "core",
      "description": "TODO comment in production code",
      "remediation": "Remove TODO or implement immediately",
      "scanner": "TodoProductionlogicScanner"
    },
    ...
  ],
  "summary": {
    "by_severity": {
      "CRITICAL": 1598,
      "HIGH": 2916,
      "MEDIUM": 18500,
      "LOW": 5870
    },
    "by_type": { ... },
    "by_subsystem": { ... }
  }
}
```

### Markdown Report (`report.md`)

Human-readable summary:

```markdown
# Gap Scanner Report

## Executive Summary

**Total Findings**: 28,884  
**Production Status**: ✓ READY

### Priority Breakdown
- **P0 Blockers** (CRITICAL × CRITICAL): 0
- **P1 High Risk** (HIGH × HIGH): 3
- **P1.5 Significant** (HIGH × CRITICAL): 3
- **P2 Medium**: 18,500
- **P3 Low**: 5,870

### Top Issues by Count
1. scope_mismatch: 15,005
2. missing_doxygen_comment: 3,157
3. todo_as_productionlogic: 3,010
...
```

---

## Troubleshooting

### Issue: "No module named 'gs3_orchestrator'"

**Solution**: Ensure you're running from the ThemisDB root directory:
```bash
cd /path/to/ThemisDB
python tools/gs3.py scan src
```

### Issue: Scan takes too long

**Solution**: Use `--scan-mode fast` for quicker results:
```bash
python tools/gs3.py scan src --scan-mode fast
```

### Issue: Memory usage too high

**Solution**: Scan directories individually:
```bash
python tools/gs3.py scan src
python tools/gs3.py scan include
python tools/gs3.py scan tests
```

### Issue: No scanners found

**Solution**: Verify scanner files exist:
```bash
python tools/gs3.py list-scanners
```

If this fails, check that `tools/scanners/` directory contains `gs3_step*.py` files.

---

## Integration with CI/CD

### GitHub Actions Example

```yaml
name: GS3 Scan

on: [pull_request]

jobs:
  scan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.11'
      
      - name: Run GS3 scan
        run: |
          python tools/gs3.py scan src include \
            --scan-mode fast \
            --output scan_results.json \
            --md-report scan_report.md
      
      - name: Check for blockers
        run: |
          if grep -q 'P0 Blockers: [1-9]' scan_report.md; then
            echo "ERROR: Critical blockers found"
            cat scan_report.md
            exit 1
          fi
      
      - name: Comment on PR
        if: always()
        uses: actions/github-script@v6
        with:
          script: |
            const fs = require('fs');
            const report = fs.readFileSync('scan_report.md', 'utf8');
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.repo,
              body: report
            });
```

---

## Advanced Configuration

### Creating a `.gs3config.yaml` file

```yaml
# GS3 Configuration

scan:
  default_mode: fast
  timeout_seconds: 600
  
output:
  json_indent: 2
  include_raw_context: true
  
reporting:
  markdown_details: true
  json_compact: false
  
performance:
  parallel_workers: 4
  batch_size: 1000
```

Load with:
```bash
python tools/gs3.py config --edit
```

---

## Performance Benchmarks

On typical ThemisDB codebase (370K+ findings):

| Mode | Directories | Time | Memory |
|------|-------------|------|--------|
| fast | src | 45s | 500MB |
| fast | src+include+tests | 120s | 800MB |
| thorough | src | 180s | 1.2GB |
| thorough | src+include+tests+benchmarks | 450s | 2.0GB |

---

## Support & Reporting Issues

For bugs or feature requests:
1. Run: `python tools/gs3.py list-scanners` to verify setup
2. Create issue with scan output (JSON or markdown)
3. Include CLI version: `python tools/gs3.py --version`

---

**GS3 CLI v4.0 — Production Ready** ✓
