# ThemisDB Gap Scanner V3 - Complete Integration Guide

**Status**: ✅ Fully Integrated  
**Latest Update**: 2026-06-21  
**Scanner Count**: 50+ specialized scanners  
**Architecture**: Modular, step-based pipeline  

---

## 📋 System Overview

Gap Scanner V3 is a comprehensive gap detection system that orchestrates **50+ specialized scanners** across 5 execution tiers (0-4) to identify:

- ✅ Memory safety issues (RAII, leaks, use-after-free)
- ✅ Concurrency bugs (race conditions, deadlocks)
- ✅ Security vulnerabilities (injection, crypto, auth)
- ✅ Production logic gaps (TODOs, stubs, error handling)
- ✅ AI/LLM safety issues (prompt injection, model integrity)
- ✅ Documentation gaps (missing Doxygen, header drift)

### Classification Dimensions

**1. Severity** (How bad is it?)
- `CRITICAL` - Blocks release, affects core engine
- `HIGH` - Significant issues, should fix soon
- `MEDIUM` - Code quality, style, documentation
- `LOW` - Cosmetic, suggestions

**2. Impact** (Where does it matter most?) — **Impact-Based Classification**
- `CRITICAL` - Core engine (src/core/, consensus, auth, security)
- `HIGH` - LLM, networking, graph, model modules
- `MEDIUM` - Monitoring, protocols, multi-GPU
- `LOW` - Utilities, helpers, tests
- `THIRD_PARTY` - External dependencies

**Prioritization**: P0-P3 based on Severity × Impact matrix

---

## 🎯 Quick Start

### 1. Scan a Directory (Fast Mode)
```bash
python -m tools.gs3 scan src include --scan-mode fast
```

**Output**:
- JSON: `ai_working/scan_results.json` (370k+ findings)
- Markdown: `ai_working/scan_results.md` (human-readable report)

### 2. List All Scanners
```bash
python -m tools.gs3 list-scanners
python -m tools.gs3 list-scanners --step 2  # Step 2 only
```

### 3. Generate Report
```bash
python -m tools.gs3 report ai_working/scan_results.json --format md --output report.md
```

### 4. Show Configuration
```bash
python -m tools.gs3 config --show
```

---

## 📂 Architecture Overview

```
tools/
├── gs3.py                              ← MAIN CLI (NEW - START HERE)
├── gs3_orchestrator.py                 ← Orchestrator (runs all scanners)
├── gs3_base_scanner.py                 ← Base class + Gap dataclass
├── scanners/
│   ├── gs3_impact_classifier.py        ← Impact classification engine
│   ├── gs3_step00_uniform_full.py      ← Step 0: Meta-orchestrator
│   ├── gs3_step01_*.py                 ← Step 1: Baseline (20+ scanners)
│   ├── gs3_step02_*.py                 ← Step 2: Context-aware (5+ scanners)
│   ├── gs3_step03_*.py                 ← Step 3: Security (8+ scanners)
│   └── gs3_step04_*.py                 ← Step 4: Design rules (15+ scanners)
└── gap_scanner_v3*.py                  ← LEGACY CODE (deprecated)
```

---

## 🔄 Execution Flow

```
1. CLI Entry Point (tools/gs3.py)
      ↓
2. Argument Parsing + Validation
      ↓
3. Orchestrator (gs3_orchestrator.py)
      ├→ Load all scanner classes by priority
      ├→ Initialize each scanner
      ├→ Execute by step (0 → 1 → 2 → 3 → 4)
      └→ Aggregate results
      ↓
4. Impact Classifier (gs3_impact_classifier.py)
      ├→ For each finding, classify impact level
      ├→ Assign to subsystem (core, llm, graph, utils, etc.)
      └→ Tag priority tier (P0-P3)
      ↓
5. Export Results
      ├→ JSON: Full structured data
      ├→ Markdown: Human-readable report
      └→ Metrics: Summary statistics
```

---

## 📊 Step-by-Step Breakdown

### **Step 0** - Meta-Orchestrator
- **File**: `gs3_step00_uniform_full.py`
- **Purpose**: Coordinates all other steps
- **Mode**: Ultra-fast discovery phase
- **Scanners**: 1

### **Step 1** - Baseline Detection
- **File**: `gs3_step01_*.py` (20+ files)
- **Purpose**: Fast keyword/pattern matching (~1-2 sec/file)
- **Examples**:
  - Braces imbalance (parsing)
  - Scope mismatches (namespaces)
  - TODOs in production code
  - Missing Doxygen tags
- **Scanners**: 20+

### **Step 2** - Context-Aware Analysis
- **File**: `gs3_step02_*.py` (5+ files)
- **Purpose**: Semantic analysis with code context (~5-15 sec/file)
- **Examples**:
  - Exception safety issues
  - Input validation gaps
  - Type conversion bugs
  - Uninitialized variables
- **Scanners**: 5+

### **Step 3** - Security & Cryptography
- **File**: `gs3_step03_*.py` (8+ files)
- **Purpose**: Security-focused patterns (~15-40 sec/file)
- **Examples**:
  - Attack vectors
  - Data leaks
  - Encryption misuse
  - Legacy duplication
- **Scanners**: 8+

### **Step 4** - Design & Architecture Rules
- **File**: `gs3_step04_*.py` (15+ files)
- **Purpose**: High-level design patterns (~2-5 min/file candidates)
- **Examples**:
  - Architecture violations
  - Module governance
  - API deprecation
  - Performance patterns
  - Distributed consistency
- **Scanners**: 15+

---

## 🎯 Scanner Registry & Auto-Discovery

All scanners are automatically discovered and registered via the `ScannerRegistry` singleton:

```python
# In gs3_base_scanner.py
class ScannerRegistry:
    _instance = None
    _scanners = set()
    
    @classmethod
    def get_instance(cls):
        if cls._instance is None:
            cls._instance = ScannerRegistry()
        return cls._instance
    
    def register(self, scanner_class):
        self._scanners.add(scanner_class)
    
    def get_scanners(self):
        return sorted(self._scanners, key=lambda s: getattr(s, 'step', 0))
```

**How It Works**:
1. Each scanner module (`gs3_step*.py`) imports `BaseGapScanner`
2. Scanner class inherits from `BaseGapScanner`
3. On import, the scanner auto-registers via `@dataclass` decorator
4. Orchestrator queries `ScannerRegistry.get_instance().get_scanners()`
5. Scanners execute in priority order (Step 0 → Step 4)

---

## 📝 Creating a New Scanner

### Template
```python
#!/usr/bin/env python3
"""Scanner for detecting X issues"""

from pathlib import Path
from typing import List
from ..gs3_base_scanner import BaseGapScanner, Gap

class MyCustomScanner(BaseGapScanner):
    """Detects X issues in code"""
    
    step = 1  # Step number (0-4)
    priority = "BASELINE"  # BASELINE, MEDIUM, SPECIALIZED, FP_FILTER, SEMANTIC
    
    def scan_file(self, filepath: Path, content: str) -> List[Gap]:
        """Scan single file and return gaps"""
        gaps = []
        
        for line_num, line in enumerate(content.split('\n'), 1):
            if self._is_issue(line):
                gap = Gap(
                    file=str(filepath),
                    line=line_num,
                    type='my_issue_type',
                    severity='HIGH',
                    confidence=0.95,
                    description='What is wrong',
                    remediation='How to fix it',
                    context=line.strip()
                )
                gaps.append(gap)
        
        return gaps
    
    def _is_issue(self, line: str) -> bool:
        """Detect the issue pattern"""
        return 'TODO' in line and 'FIXME' in line
```

### Register the Scanner
1. Create `tools/scanners/gs3_step01_my_custom.py`
2. Import `BaseGapScanner`
3. Inherit and implement `scan_file()` method
4. Set `step` and `priority` class attributes
5. Run: `python -m tools.gs3 list-scanners` to verify registration

---

## 🔧 Impact Classification

The `ImpactClassifier` automatically assigns impact levels based on file paths:

```python
from tools.scanners.gs3_impact_classifier import ImpactClassifier

# Classify a file
impact_level, subsystem = ImpactClassifier.classify('src/core/engine.cpp')
# Returns: ('CRITICAL', 'core')

impact_level, subsystem = ImpactClassifier.classify('tests/unit/test_utils.cpp')
# Returns: ('LOW', 'utils')
```

### Module Hierarchy
```
CRITICAL Impact:
  src/core/          → 'core'
  src/auth/          → 'auth'
  src/security/      → 'security'
  src/distributed/*  → 'core'

HIGH Impact:
  src/llm/           → 'llm'
  src/ai_*/          → 'ai'
  src/network/       → 'network'
  src/graph/         → 'graph'
  src/model/         → 'model'

MEDIUM Impact:
  src/monitoring/    → 'monitoring'
  src/multi/         → 'multi'
  src/mqtt/          → 'mqtt'
  src/kafka/         → 'kafka'

LOW Impact:
  src/utils/         → 'utils'
  src/helper/        → 'utils'
  tests/             → 'tests'
  benchmarks/        → 'benchmarks'

THIRD_PARTY:
  external/          → 'external'
  third_party/       → 'external'
```

---

## 📊 Output Format

### JSON Output (`scan_results.json`)
```json
{
  "summary": {
    "total_findings": 370707,
    "severity": {
      "CRITICAL": 5530,
      "HIGH": 43086,
      "MEDIUM": 298581,
      "LOW": 23510
    },
    "impact": {
      "CRITICAL": 315,
      "HIGH": 2838,
      "LOW": 367021,
      "THIRD_PARTY": 533
    }
  },
  "gaps": [
    {
      "file": "src/core/engine.cpp",
      "line": 123,
      "type": "todo_as_productionlogic",
      "severity": "HIGH",
      "confidence": 0.95,
      "description": "TODO in production logic path",
      "remediation": "Replace with implementation or justified skip",
      "context": "  // TODO: Implement proper error handling",
      "scanner": "TodoProductionLogicScanner",
      "step": 1,
      "impact_level": "CRITICAL",
      "subsystem": "core"
    }
  ]
}
```

### Markdown Report (`scan_results.md`)
```markdown
# Gap Scanner V3 Report

**Total Findings**: 370,707

## Summary by Severity
| Severity | Count |
|----------|-------|
| CRITICAL | 5,530 |
| HIGH | 43,086 |
| MEDIUM | 298,581 |
| LOW | 23,510 |

## Summary by Impact
| Impact | Count |
|--------|-------|
| CRITICAL | 315 |
| HIGH | 2,838 |
| MEDIUM | 0 |
| LOW | 367,021 |

## Priority Analysis
- P0 (CRITICAL×CRITICAL): 0 findings
- P0.5 (CRITICAL×HIGH): 0 findings
- P1 (HIGH×CRITICAL): 3 findings
- P1.5 (HIGH×HIGH): 3 findings
```

---

## 🚀 Usage Examples

### Full Codebase Scan
```bash
python -m tools.gs3 scan src include tests benchmarks --scan-mode thorough --output results_full.json
```

### Fast Scan (Production)
```bash
python -m tools.gs3 scan src --scan-mode fast --output results_fast.json
```

### Scan with Markdown Report
```bash
python -m tools.gs3 scan src \
  --output ai_working/scan.json \
  --md-report ai_working/scan.md
```

### Generate Report from Existing Scan
```bash
python -m tools.gs3 report ai_working/scan.json --format md --output report.md
```

### List Scanners by Step
```bash
python -m tools.gs3 list-scanners --step 1
python -m tools.gs3 list-scanners --step 4
```

---

## 📦 Dependencies

- **Python**: 3.9+
- **Internal**:
  - `tools.gs3_base_scanner` - Base class
  - `tools.gs3_orchestrator` - Orchestrator
  - `tools.scanners.*` - Individual scanners
- **External**: None (stdlib only)

---

## 🔍 Troubleshooting

### "No scanners found"
- Verify `tools/scanners/` directory exists
- Check scanner files have `BaseGapScanner` inheritance
- Run: `python -m tools.gs3 list-scanners`

### "Import error: gs3_base_scanner"
- Ensure `tools/` is in Python path
- Run from project root: `python -m tools.gs3 ...`

### "Orchestrator fails"
- Check `tools/gs3_orchestrator.py` is executable
- Verify scanner step numbers are 0-4
- Check input directories exist

### "Large output file"
- Full codebase scan generates ~100-200MB JSON
- Use `--scan-mode fast` for baseline only
- Filter results before reporting

---

## 📚 Related Documentation

- [IMPACT_CLASSIFICATION_SESSION_SUMMARY.md](../IMPACT_CLASSIFICATION_SESSION_SUMMARY.md) - Impact system details
- [IMPACT_REMEDIATION_ROADMAP.md](../IMPACT_REMEDIATION_ROADMAP.md) - Remediation strategy
- [FULL_SCAN_COMPREHENSIVE_REPORT_2026_06_21.md](../FULL_SCAN_COMPREHENSIVE_REPORT_2026_06_21.md) - Sample report
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Code standards

---

## ✨ Key Features

✅ **Modular Architecture**: 50+ independent scanners  
✅ **Auto-Discovery**: Scanners self-register via registry  
✅ **Impact Classification**: Severity × Impact dual-axis prioritization  
✅ **Fast Baseline**: Ultra-fast keyword scanning (Step 1)  
✅ **Deep Analysis**: Multi-step context-aware analysis (Steps 2-4)  
✅ **Export Formats**: JSON, Markdown, metrics  
✅ **Extensible**: Easy to add new scanners  

---

## 🎓 Learning Path

1. **Start Here**: Read this file (you are here)
2. **Understand Flow**: Check `tools/gs3_orchestrator.py`
3. **Base Class**: Review `tools/gs3_base_scanner.py`
4. **Impact System**: Study `tools/scanners/gs3_impact_classifier.py`
5. **Example Scanner**: Look at `tools/scanners/gs3_step01_ai_todo_productionlogic.py`
6. **Run CLI**: Execute `python -m tools.gs3 list-scanners`
7. **Create Scanner**: Follow [Creating a New Scanner](#-creating-a-new-scanner)

---

## 📞 Questions?

Refer to inline code comments for detailed implementation notes. Each scanner module includes:
- Clear docstrings
- Example patterns
- Confidence thresholds
- Remediation guidance

**Report bugs in**: [CONTRIBUTING.md](../CONTRIBUTING.md)

---

**Last Updated**: 2026-06-21  
**Maintained By**: ThemisDB Development Team  
**License**: See LICENSE file
