# Konsolidierung Status — 2. Juni 2026

**Ziel:** Unified Gap Scanner V3 Pipeline mit OOP-Architektur (FLACHE STRUKTUR)

---

## ✅ PHASE 1: KONSOLIDIERUNG ABGESCHLOSSEN

### Schritt 1.1: Flache Verzeichnisstruktur ✓
```
tools/scanners/
├─ __init__.py                    (Updated: New imports + legacy compat)
├─ gs3_base_scanner.py            (Base class, utilities, pipeline)
├─ gs3_step00_*.py                (Tier 0: Baseline scanners)
├─ gs3_step01_*.py                (Tier 1: Phase 1-4 scanners)
├─ gs3_step02_*.py                (Tier 2: Specialized scanners)
├─ gs3_step03_*.py                (Tier 3: Hardening & Security)
├─ gs3_step04_*.py                (Tier 4: Semantic & FP Filters)
└─ gs3_orchestrator.py            (Pipeline main orchestrator)

Legacy:
├─ gap_scanner_v3_*.py            (39 alte Dateien, zu konsolidieren)
└─ .deprecated/                   (Archive)
```

### Schritt 1.2: Base Scanner Class ✓
```
gs3_base_scanner.py (420 LOC)
├─ ScannerPriority Enum (BASELINE → SEMANTIC)
├─ Gap dataclass (unified representation)
├─ BaseGapScanner abstract base class
├─ ScannerRegistry (plugin architecture)
├─ GapScannerPipeline (orchestrator)
└─ FPFilter abstract class
```

### Schritt 1.3: Naming Schema dokumentiert ✓
```
TIER 0 (Baseline, ~1-2 sec/file):
  gs3_step00_keyword_matching.py

TIER 1 (Basic Code Quality, ~5-15 sec/file):
  gs3_step01_memory_safety.py
  gs3_step01_error_handling.py
  gs3_step01_thread_safety.py
  gs3_step01_raii.py

TIER 2 (Specialized, ~15-40 sec/file):
  gs3_step02_type_conversion.py
  gs3_step02_input_validation.py
  gs3_step02_performance.py

TIER 3 (Hardening & Security, Phase 11):
  gs3_step03_data_leak.py
  gs3_step03_encryption_leak.py
  gs3_step03_e2e_encryption.py
  gs3_step03_key_failure.py
  gs3_step03_attack_vectors.py
  gs3_step03_military_hardening.py
  gs3_step03_legacy_duplication.py

TIER 4 (Semantic & FP Filters):
  gs3_step04_wave5_context_filter.py
  gs3_step04_wave6_ast_filter.py

Orchestrator:
  gs3_orchestrator.py
```

---

## 🚧 PHASE 2: PHASE 1-4 REPARATUR (IN PROGRESS)

### Archivierung bestehender Scanner (TODO)
```powershell
# Move Phase 1-4 zu .deprecated für Reparatur
mkdir tools/.deprecated -ErrorAction SilentlyContinue
move tools/gap_scanner_v3_memory.py tools/.deprecated/
move tools/gap_scanner_v3_reliability.py tools/.deprecated/
move tools/gap_scanner_v3_concurrency.py tools/.deprecated/
move tools/gap_scanner_v3_security.py tools/.deprecated/
move tools/gap_scanner_v3_raii.py tools/.deprecated/
```

### Umzubenennende Dateien (TODO)
```
gap_scanner_v3_memory.py → tools/scanners/gs3_step01_memory_safety.py
gap_scanner_v3_reliability.py → tools/scanners/gs3_step01_error_handling.py
gap_scanner_v3_concurrency.py → tools/scanners/gs3_step01_thread_safety.py
gap_scanner_v3_raii.py → tools/scanners/gs3_step01_raii.py
gap_scanner_v3_type_conversion.py → tools/scanners/gs3_step02_type_conversion.py
gap_scanner_v3_input_validation.py → tools/scanners/gs3_step02_input_validation.py

(Phase 11 folgt später mit gs3_step03_* Namen)
```

### Zu reparierender Code (pro Scanner)
```
1. Erbe von BaseGapScanner (nicht custom classes)
2. Definiere PRIORITY (z.B. ScannerPriority.MEDIUM)
3. Implementiere scan() → List[Gap]
4. Nutze Gap dataclass für Rückgabewerte
5. Ergänze fehlende Methoden (_is_raii_wrapper_cleanup, etc.)
6. Schreibe Unit Tests neben Scanner
```

---

## 📝 NÄCHSTE SCHRITTE (TODO)

### Sofort (1-2 Stunden)
1. [ ] Verschiebe alte Scanner zu `.deprecated/`
2. [ ] Erstelle erste refactored Scanner in `tools/scanners/`:
   - `gs3_step01_memory_safety.py` (aus gap_scanner_v3_memory.py)
   - `gs3_step01_error_handling.py` (aus gap_scanner_v3_reliability.py)
3. [ ] Schreibe Unit Tests neben den Scannern

### Dann (4-6 Stunden)
4. [ ] Refactor remaining Phase 1-4 Scanner in `tools/scanners/`
5. [ ] Benenne Phase 11 Scanner um (mit neuem Namen):
   - `gs3_step03_data_leak.py`
   - `gs3_step03_military_hardening.py`
   - etc.
6. [ ] Implementiere Pipeline Orchestrator: `gs3_orchestrator.py`

### Später (2-3 Stunden)
7. [ ] Schreibe Orchestrator-Tests
8. [ ] Validiere end-to-end
9. [ ] Git commit

---

## STRUKTUR: Scanner neu schreiben (Template)

**Beispiel: tools/scanners/gs3_step01_memory_safety.py**

```python
#!/usr/bin/env python3
"""
Gap Scanner Step 01.1 — Memory Safety

Detects:
- Raw new/delete without RAII
- Use-after-free risks
- Memory leaks
- Unchecked malloc/calloc
"""

import re
from pathlib import Path
from typing import List
import sys

# Import base scanner from tools/
sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority

class MemorySafetyScanner(BaseGapScanner):
    """Phase 1.1: Memory Safety (±5 line context)"""
    
    PRIORITY = ScannerPriority.MEDIUM
    ENABLED = True
    
    def __init__(self):
        super().__init__("MemorySafetyScanner", "2.0")
        self.patterns = {...}  # Bestehende patterns beibehalten
    
    def scan(self, source_dir: str) -> List[Gap]:
        gaps = []
        
        for file_path in self._scan_files(source_dir):
            lines = self._read_file_lines(file_path)
            
            # Existing detection logic, aber mit Gap statt MemoryGap
            gaps.extend(self._check_new_without_raii(file_path, lines))
            gaps.extend(self._check_pointer_arithmetic(file_path, lines))
            gaps.extend(self._check_unchecked_malloc(file_path, lines))
            gaps.extend(self._is_raii_wrapper_cleanup(file_path, lines))  # ← ADD
        
        return self.deduplicate(gaps)
    
    def _check_new_without_raii(self, file_path: Path, lines: List[str]) -> List[Gap]:
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if re.search(r'\bnew\s+\w+', line):
                # Check if smart ptr in context
                if not self._context_window_search(lines, line_no, 
                    ['unique_ptr', 'shared_ptr', 'make_unique', 'make_shared'], 
                    window=2):
                    
                    gaps.append(Gap(
                        file=str(file_path.relative_to(Path.cwd())),
                        line=line_no,
                        type="new_without_raii",
                        severity="CRITICAL",
                        confidence=0.75,
                        description="Raw new without RAII wrapper",
                        remediation="Use std::unique_ptr<T> or std::make_unique<T>()",
                        context='\n'.join(self._get_context(lines, line_no, window=3))
                    ))
        
        return gaps
    
    def _is_raii_wrapper_cleanup(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Check if RAII wrapper cleanup is missing"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if re.search(r'\.cleanup\(\)|\.close\(\)', line):
                # Check if RAII destructor exists
                if not self._context_window_search(lines, line_no, [r'~\w+\s*\('], window=10):
                    gaps.append(Gap(
                        file=str(file_path.relative_to(Path.cwd())),
                        line=line_no,
                        type="missing_raii_cleanup",
                        severity="HIGH",
                        confidence=0.70,
                        description="Manual cleanup() instead of RAII destructor",
                        remediation="Use ~Class() destructor for automatic cleanup",
                        context='\n'.join(self._get_context(lines, line_no, window=3))
                    ))
        
        return gaps

# Usage:
if __name__ == "__main__":
    scanner = MemorySafetyScanner()
    gaps = scanner.scan("./src")
    print(f"Found {len(gaps)} memory gaps")
    for gap in gaps[:5]:
        print(f"  {gap.file}:{gap.line} - {gap.type}")
```

---

## Metriken (nach Abschluss erwartet)

```
Tier 0 (Baseline):        ~100-200 gaps (keywords only)
Tier 1 (Phase 1-4):       ~3,000 gaps (memory, concurrency, error handling)
Tier 2 (Specialized):     ~400-600 gaps (types, input validation)
Tier 3 (Hardening):       4,458 gaps (Phase 11, security baseline)
After Wave 5-6 filtering: ~7,458 gaps → ~5,500 gaps (FP reduction)

Directory Size:
  tools/scanners/*.py:    ~15 Scanner files (~500-1000 LOC each)
  tools/gs3_base_scanner.py: 420 LOC (shared utilities)
  Total code: ~8,000 LOC
```

---

## Status Zusammenfassung

**ABGESCHLOSSEN:**
- ✅ OOP Base Scanner Class
- ✅ Verzeichnisstruktur
- ✅ Naming Schema (gs3_stepXX_<focus>.py)
- ✅ Package Imports (__init__.py)

**IN PROGRESS:**
- 🚧 Phase 1-4 Scanner refactor + reparieren
- 🚧 Archivierung alte Dateien

**TODO:**
- [ ] Pipeline Orchestrator
- [ ] End-to-end Integration Tests
- [ ] Phase 11 Scanner mit neuem Namen
- [ ] Wave 5-6 Semantic Filters
- [ ] Final Git Consolidation Commit

---

**Nächste Aktion:** Starte Phase 1-4 Refactoring mit `gs3_step01_memory_safety.py`
