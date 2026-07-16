# Konsolidierung & Reparatur — Schritt-für-Schritt Plan

**Status:** 39 Gap Scanner Dateien, 6 Phase 11 produzieren 4,458 Gaps, Phase 1-4 broken

**Ziel:** Unified Pipeline: Phase 1-4 Baseline → Spezialisierung → Wave Filter

---

## PHASE 1: KONSOLIDIERUNG (2-3 Stunden)

### Schritt 1.1: Verzeichnis-Struktur erstellen
```powershell
# Neues Verzeichnis für organisierte Scanner
mkdir tools/scanners
mkdir tools/scanners/base
mkdir tools/scanners/tier0_baseline
mkdir tools/scanners/tier1_basic
mkdir tools/scanners/tier2_specialized
mkdir tools/scanners/tier3_hardening
mkdir tools/scanners/tier4_semantic
mkdir tools/scanners/utils
mkdir tools/.deprecated
mkdir tools/.experimental
```

### Schritt 1.2: Base Scanner Class implementieren
```python
# tools/scanners/base/base_scanner.py
from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
from typing import List, Optional

class ScannerPriority(Enum):
    BASELINE = 0
    MEDIUM = 1
    SPECIALIZED = 2
    FP_FILTER = 3
    SEMANTIC = 4

@dataclass
class Gap:
    file: str
    line: int
    type: str
    severity: str
    confidence: float
    description: str
    remediation: str
    context: Optional[str] = None

class BaseGapScanner(ABC):
    PRIORITY: ScannerPriority
    ENABLED: bool = True
    
    def __init__(self, name: str):
        self.name = name
        self.gaps: List[Gap] = []
    
    @abstractmethod
    def scan(self, source_dir: str) -> List[Gap]:
        pass
```

### Schritt 1.3: Phase 11 in produktive Location verschieben
```powershell
# Phase 11 bleibt im tools/ root (Production), aber mit __init__.py
# tools/gap_scanner_v3_phase11_integration.py → tools/scanners/tier3_hardening/phase11_integration.py
# oder: Phase 11 bleibt wo es ist, aber wird als "tier3_hardening" referenziert
```

### Schritt 1.4: Alte Phasen archivieren
```powershell
# Verschiebe Phase 1-4 Scanner zu .deprecated (zum reparieren)
move tools/gap_scanner_v3_security.py tools/.deprecated/
move tools/gap_scanner_v3_memory.py tools/.deprecated/
move tools/gap_scanner_v3_reliability.py tools/.deprecated/
move tools/gap_scanner_v3_concurrency.py tools/.deprecated/

# Verschiebe Phase 5, 7-10 zu .deprecated
# Verschiebe Wave Filter zu .experimental
```

### Schritt 1.5: gap_scanner_v3.py als deprecated markieren
```python
#!/usr/bin/env python3
"""⚠️  DEPRECATED Orchestrator — broken Phase 1-4 integration

USE: gap_scanner_v3_phase11_integration.py
See: tools/README_SCANNER_PIPELINE.md
"""
import sys
print("ERROR: gap_scanner_v3.py orchestrator is deprecated.")
print("USE: python tools/gap_scanner_v3_phase11_integration.py")
print("See tools/README_SCANNER_PIPELINE.md for architecture")
sys.exit(1)
```

---

## PHASE 2: REPARATUR PHASE 1-4 (8-12 Stunden)

### Schritt 2.1: Analysiere Phase 1-4 Scanner
Für jede Phase:
- Welche Methoden fehlen? (z.B. `_is_raii_wrapper_cleanup`)
- Welche Pattern-Coverage ist unvollständig?
- Welche Tests existieren?

### Schritt 2.2: Refaktoriere Phase 1-4 zur Base Scanner Class

**Beispiel: gap_scanner_v3_memory.py → tier1_basic/memory_safety.py**

```python
from scanners.base.base_scanner import BaseGapScanner, Gap, ScannerPriority

class MemorySafetyScanner(BaseGapScanner):
    """Phase 1.1: Memory Safety Gaps (±5 line context)"""
    PRIORITY = ScannerPriority.MEDIUM
    
    def __init__(self):
        super().__init__("MemorySafetyScanner")
        self.patterns = {...}  # Bestehende Patterns beibehalten
    
    def scan(self, source_dir: str) -> List[Gap]:
        gaps = []
        # Bestehende scan-Logik beibehalten
        # Aber: Rückgabetyp = List[Gap] (unified)
        for file_path in self._scan_files(source_dir):
            gaps.extend(self._check_new_without_raii(file_path))
            gaps.extend(self._check_pointer_arithmetic(file_path))
            gaps.extend(self._check_unchecked_malloc(file_path))
            gaps.extend(self._is_raii_wrapper_cleanup(file_path))  # ← ADD THIS
        return gaps
    
    def _is_raii_wrapper_cleanup(self, file_path: str) -> List[Gap]:
        """Check if RAII wrapper cleanup is missing"""
        gaps = []
        with open(file_path) as f:
            lines = f.readlines()
        
        for line_no, line in enumerate(lines, 1):
            # Detect pattern: manual cleanup instead of RAII destructor
            if 'cleanup()' in line or 'close()' in line:
                context = self._get_context(lines, line_no, window=5)
                if not self._has_raii_destructor_in_context(context):
                    gaps.append(Gap(
                        file=str(file_path),
                        line=line_no,
                        type="missing_raii_cleanup",
                        severity="HIGH",
                        confidence=0.70,
                        description="Manual cleanup() call instead of RAII destructor",
                        remediation="Use ~Class() destructor for automatic cleanup"
                    ))
        return gaps
    
    def _has_raii_destructor_in_context(self, context: List[str]) -> bool:
        """Check if RAII destructor exists"""
        context_str = '\n'.join(context)
        return bool(re.search(r'~\w+\s*\(', context_str))
```

### Schritt 2.3: Implementiere alle Phase 1-4 Scanner neu

```
tier1_basic/
├─ __init__.py
├─ memory_safety.py          ← Refactored, with _is_raii_wrapper_cleanup
├─ error_handling.py         ← Phase 1.2 Reliability
├─ thread_safety.py          ← Phase 1.3 Concurrency
├─ raii.py                   ← Phase 1.4 RAII (enhanced)
└─ tests/
   ├─ test_memory.py
   ├─ test_error_handling.py
   ├─ test_thread_safety.py
   └─ test_raii.py
```

### Schritt 2.4: Schreibe Unit Tests für Phase 1

```python
# tools/scanners/tier1_basic/tests/test_memory.py
import pytest
from scanners.tier1_basic.memory_safety import MemorySafetyScanner

def test_detects_new_without_delete():
    scanner = MemorySafetyScanner()
    # Create test file with: int* ptr = new int(5);
    # Expected: 1 gap of type "new_without_delete"
    gaps = scanner.scan("test_data/memory/new_without_delete.cpp")
    assert len(gaps) == 1
    assert gaps[0].type == "new_without_delete"

def test_detects_raii_wrapper_cleanup():
    scanner = MemorySafetyScanner()
    # Create test file with manual cleanup() instead of destructor
    gaps = scanner.scan("test_data/memory/missing_raii_cleanup.cpp")
    assert len(gaps) == 1
    assert gaps[0].type == "missing_raii_cleanup"

def test_ignores_smart_ptr():
    scanner = MemorySafetyScanner()
    # Create test file with std::unique_ptr (safe)
    gaps = scanner.scan("test_data/memory/with_smart_ptr.cpp")
    assert len(gaps) == 0  # No gaps expected
```

---

## PHASE 3: PIPELINE ORCHESTRATOR (4-6 Stunden)

### Schritt 3.1: Erstelle Pipeline Orchestrator

```python
# tools/scanners/pipeline.py
from scanners.base.base_scanner import BaseGapScanner, ScannerPriority
from scanners.tier1_basic import MemorySafetyScanner, ErrorHandlingScanner, ...
from scanners.tier3_hardening import Phase11SecurityScanners

class ScannerRegistry:
    def __init__(self):
        self.scanners = []
    
    def register(self, scanner: BaseGapScanner):
        if scanner.ENABLED:
            self.scanners.append(scanner)
    
    def get_by_priority(self):
        return sorted(self.scanners, key=lambda s: s.PRIORITY.value)

class GapScannerPipeline:
    def __init__(self):
        self.registry = ScannerRegistry()
        self._register_all_scanners()
    
    def _register_all_scanners(self):
        # Tier 0: Baseline
        # self.registry.register(BaselineSecurityScanner())  # TODO: implement
        
        # Tier 1: Basic (Phase 1-4)
        self.registry.register(MemorySafetyScanner())
        self.registry.register(ErrorHandlingScanner())
        self.registry.register(ThreadSafetyScanner())
        self.registry.register(RAIIScanner())
        
        # Tier 2: Specialized
        # self.registry.register(TypeConversionScanner())  # TODO
        # self.registry.register(InputValidationScanner())  # TODO
        
        # Tier 3: Hardening (Phase 11)
        self.registry.register(SecurityHardeningScanner())
        self.registry.register(DataLeakScanner())
        # ... rest of Phase 11
        
        # Tier 4: FP Filters
        # self.registry.register(Wave5FPFilter())  # TODO
    
    def execute(self, source_dir: str):
        all_gaps = []
        scanners = self.registry.get_by_priority()
        
        for scanner in scanners:
            print(f"Running {scanner.name}...")
            gaps = scanner.scan(source_dir)
            all_gaps.extend(gaps)
            print(f"  → Found {len(gaps)} gaps")
        
        return all_gaps
```

### Schritt 3.2: Erstelle neue Einstiegspunkte

```bash
# tools/scan_all_tiers.py — Läuft alle Phasen (Phase 1-4 + Phase 11)
# tools/scan_tier1_only.py — Läuft nur Tier 1 (Phase 1-4)
# tools/scan_tier3_only.py — Läuft nur Tier 3 (Phase 11, current)
```

---

## PHASE 4: INTEGRATION & TESTING (4-6 Stunden)

### Schritt 4.1: Integriere alle Komponenten
- Base Scanner Class
- Alle Tier 1-3 Scanner
- Pipeline Orchestrator
- Utility functions

### Schritt 4.2: Full-Suite Test
```powershell
# Test Tier 1 (Phase 1-4)
python tools/scan_tier1_only.py --source ./src --output tier1_results.json

# Test Tier 3 (Phase 11)
python tools/scan_tier3_only.py --source ./src --output tier3_results.json

# Test Full Pipeline
python tools/scan_all_tiers.py --source ./src --output full_results.json

# Verify: full_results >= tier1_results + tier3_results
```

### Schritt 4.3: Git Consolidation Commit
```powershell
git add tools/scanners/ tools/.deprecated/ tools/.experimental/
git commit -m "consolidate: implement unified gap scanner pipeline architecture

- Implement BaseGapScanner OOP class with ScannerPriority enum
- Reorganize scanners into Tier 0-4 directory structure
- Phase 1-4 repaired: add missing methods (_is_raii_wrapper_cleanup, etc.)
- Phase 11 integrated as Tier 3 hardening
- Pipeline orchestrator with registry pattern
- SoC: each scanner = 1 responsibility

Metrics:
  - Tier 1 (Phase 1-4): ~3,000 gaps (memory, error handling, threading)
  - Tier 3 (Phase 11): 4,458 gaps (security hardening)
  - Total: ~7,458 gaps (combined before FP filtering)

Next: Wave 5-6 semantic filtering and Tier 4 orchestration
"

git push origin develop
```

---

## ZEITPLAN

| Phase | Aufwand | Status | Ergebnis |
|-------|---------|--------|----------|
| 1. Konsolidierung | 2-3 Std | TODO | Verzeichnisstruktur + Base Class |
| 2. Phase 1-4 Reparatur | 8-12 Std | TODO | 4 funktionierende Scanner |
| 3. Pipeline Orchestrator | 4-6 Std | TODO | Unified execution |
| 4. Integration & Tests | 4-6 Std | TODO | Full suite tested |
| **TOTAL** | **18-27 Std** | **TODO** | **Production-ready** |

---

## SOFORTIGE NÄCHSTE SCHRITTE

### Schritt 1 (JETZT): Verzeichnisstruktur erstellen
```powershell
cd c:\Projects\ThemisDB

# Create structure
mkdir tools/scanners/base
mkdir tools/scanners/tier0_baseline
mkdir tools/scanners/tier1_basic
mkdir tools/scanners/tier2_specialized
mkdir tools/scanners/tier3_hardening
mkdir tools/scanners/tier4_semantic
mkdir tools/scanners/utils
mkdir tools/.deprecated
mkdir tools/.experimental

echo "✓ Directory structure created"
```

### Schritt 2 (JETZT): Base Scanner Class
→ Implementiere in `tools/scanners/base/base_scanner.py`

### Schritt 3 (DANN): Phase 1-4 Scanner archivieren
→ `move` zu `.deprecated/`

### Schritt 4 (DANN): Phase 1-4 Scanner reparieren
→ Eines nach dem anderen in `tier1_basic/`

### Schritt 5 (SPÄTER): Pipeline Orchestrator

---

**Sollen wir Schritt 1 & 2 jetzt starten?** (Base Structure + Base Class)
