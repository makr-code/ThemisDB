# Gap Scanner V3 — Neues Benennungsschema gs3_stepXX_(<focus>)

**Prinzip:** `gs3_step<NN>_<focus>.py`
- `gs3` = gap_scanner_v3 (Abkürzung)
- `step<NN>` = Tier/Phase (00=Baseline, 01-03=Production, 04=FP Filter)
- `<focus>` = Scanner-Fokus (z.B. memory_safety, csrf, etc.)

---

## Neues Benennungsschema

### Tier 0: Baseline (Ultra-Fast)
```
gs3_step00_credentials.py       ← Phase 0.1: Hardcoded credentials
gs3_step00_dangerous_functions.py ← Phase 0.2: Dangerous function calls
```

### Tier 1: Basic Code Quality (Phase 1-4)
```
gs3_step01_memory_safety.py     ← Phase 1.1: Memory leaks, UAF, bounds
gs3_step01_error_handling.py    ← Phase 1.2: Reliability, exceptions
gs3_step01_thread_safety.py     ← Phase 1.3: Concurrency, race conditions
gs3_step01_raii.py              ← Phase 1.4: Resource management
```

### Tier 2: Specialized Patterns
```
gs3_step02_type_conversion.py   ← Phase 2.1: Narrowing conversions
gs3_step02_input_validation.py  ← Phase 2.2: Input safety, bounds
gs3_step02_attack_vectors.py    ← Phase 2.3: OWASP Top 10, CWE
```

### Tier 3: Hardening & Security (Phase 11 + specialized)
```
gs3_step03_military_hardening.py  ← Phase 3.1: FIPS 140-2, crypto
gs3_step03_data_leak.py           ← Phase 3.2: PII, secrets, memory
gs3_step03_audit_logging.py       ← Phase 3.3: Audit trail, compliance
gs3_step03_e2e_encryption.py      ← Phase 3.4: End-to-end encryption
gs3_step03_key_management.py      ← Phase 3.5: Key rotation, KDF
```

### Tier 4: False Positive Reduction
```
gs3_step04_wave5_context_filter.py   ← Wave 5: ±10 lines context
gs3_step04_wave6_semantic_filter.py  ← Wave 6: AST + control flow
```

---

## Altes Schema → Neues Schema (Mapping)

| Alter Name | Neuer Name | Tier | Fokus |
|-----------|-----------|------|-------|
| gap_scanner_v3_security.py | gs3_step01_attack_vectors.py | 2/3 | OWASP |
| gap_scanner_v3_memory.py | gs3_step01_memory_safety.py | 1 | Memory |
| gap_scanner_v3_reliability.py | gs3_step01_error_handling.py | 1 | Reliability |
| gap_scanner_v3_concurrency.py | gs3_step01_thread_safety.py | 1 | Threading |
| gap_scanner_v3_raii.py | gs3_step01_raii.py | 1 | RAII |
| gap_scanner_v3_type_conversion.py | gs3_step02_type_conversion.py | 2 | Types |
| gap_scanner_v3_input_validation.py | gs3_step02_input_validation.py | 2 | Input |
| gap_scanner_v3_phase11_data_leak.py | gs3_step03_data_leak.py | 3 | DataLeak |
| gap_scanner_v3_phase11_encryption_leak.py | gs3_step03_encryption_leak.py | 3 | Crypto |
| gap_scanner_v3_phase11_e2e_encryption.py | gs3_step03_e2e_encryption.py | 3 | E2E |
| gap_scanner_v3_phase11_key_failure.py | gs3_step03_key_management.py | 3 | Keys |
| gap_scanner_v3_phase11_attack_vectors.py | gs3_step03_attack_vectors.py | 3 | Security |
| gap_scanner_v3_phase11_military_hardening.py | gs3_step03_military_hardening.py | 3 | Hardening |
| gap_scanner_v3_wave5_*.py | gs3_step04_wave5_context_filter.py | 4 | FPFilter |
| gap_scanner_v3_wave6_*.py | gs3_step04_wave6_semantic_filter.py | 4 | Semantic |
| gap_scanner_v3_phase11_integration.py | gs3_orchestrator.py | - | Main |

---

## Neue Verzeichnisstruktur (FLACH)

```
tools/
├─ gs3_orchestrator.py                   ← Main entry point
├─ gs3_base_scanner.py                   ← OOP base class (420 LOC)
│
├─ scanners/
│  ├─ __init__.py
│  │
│  ├─ gs3_step00_credentials.py          ← Tier 0: Baseline (hardcoded secrets)
│  ├─ gs3_step00_dangerous_functions.py  ← Tier 0: Baseline (dangerous calls)
│  │
│  ├─ gs3_step01_memory_safety.py        ← Tier 1: Memory (Phase 1.1)
│  ├─ gs3_step01_error_handling.py       ← Tier 1: Reliability (Phase 1.2)
│  ├─ gs3_step01_thread_safety.py        ← Tier 1: Concurrency (Phase 1.3)
│  ├─ gs3_step01_raii.py                 ← Tier 1: RAII (Phase 1.4)
│  │
│  ├─ gs3_step02_type_conversion.py      ← Tier 2: Type safety
│  ├─ gs3_step02_input_validation.py     ← Tier 2: Input validation
│  ├─ gs3_step02_attack_vectors.py       ← Tier 2: General OWASP
│  │
│  ├─ gs3_step03_data_leak.py            ← Tier 3: Data leaks (Phase 11.1)
│  ├─ gs3_step03_encryption_leak.py      ← Tier 3: Unencrypted data (Phase 11.2)
│  ├─ gs3_step03_e2e_encryption.py       ← Tier 3: E2E coverage (Phase 11.3)
│  ├─ gs3_step03_key_management.py       ← Tier 3: Key handling (Phase 11.4)
│  ├─ gs3_step03_attack_vectors.py       ← Tier 3: Security patterns (Phase 11.5)
│  ├─ gs3_step03_military_hardening.py   ← Tier 3: FIPS 140-2 (Phase 11.6)
│  ├─ gs3_step03_legacy_duplication.py   ← Tier 3: Legacy cleanup (Phase 11.7)
│  │
│  ├─ gs3_step04_wave5_context_filter.py ← Tier 4: Context FP reduction
│  ├─ gs3_step04_wave6_semantic_filter.py ← Tier 4: AST-based FP reduction
│  │
│  └─ .deprecated/                        ← Archive old gap_scanner_v3_*.py
```
---

## Import-Beispiele (Flache Struktur)

**Vorher (chaotisch, 39 Dateien):**
```python
from gap_scanner_v3_memory import MemoryGapScanner
from gap_scanner_v3_phase11_data_leak import DataLeakScanner
from gap_scanner_v3_wave5_parallel_filters import Wave5Filter
```

**Nachher (strukturiert, 15-18 Scanner):**
```python
import sys
sys.path.insert(0, 'tools')

from gs3_base_scanner import BaseGapScanner, Gap, ScannerRegistry, GapScannerPipeline
from scanners.gs3_step01_memory_safety import MemorySafetyScanner
from scanners.gs3_step03_data_leak import DataLeakScanner
from scanners.gs3_step04_wave5_context_filter import Wave5ContextFilter

# Usage:
registry = ScannerRegistry()
registry.register(MemorySafetyScanner())
registry.register(DataLeakScanner())
registry.register(Wave5ContextFilter())

pipeline = GapScannerPipeline(registry)
gaps = pipeline.execute('./src')
```

---

## Benennungs-Regeln

1. **Präfix:** `gs3_` (immer)
2. **Step:** `step00` bis `step04` (progressive Reihenfolge)
3. **Focus:** Kurz und prägnant (kebab-case)
   - `memory_safety` (nicht `memory_gap_detection`)
   - `thread_safety` (nicht `concurrency`)
   - `data_leak` (nicht `data_leak_detection`)
4. **Suffix:** `.py`

### Gültige Focus-Namen:
```
credentials, dangerous_functions                    (Tier 0)
memory_safety, error_handling, thread_safety, raii (Tier 1)
type_conversion, input_validation, attack_vectors   (Tier 2)
military_hardening, data_leak, audit_logging,       (Tier 3)
e2e_encryption, key_management
wave5_context_filter, wave6_semantic_filter         (Tier 4)
```

---

## Implementierungs-Reihenfolge

### Phase 1: Verzeichnisstruktur + Base Class (0.5 Std)
```powershell
mkdir tools/scanners/tier{0,1,2,3,4}_*
touch tools/gs3_base_scanner.py
touch tools/gs3_orchestrator.py
```

### Phase 2: Umbenennen & Verlagern (1-2 Std)
```powershell
# Archiviere alte Dateien
move tools/gap_scanner_v3_*.py tools/.deprecated/

# Erstelle neue Dateien in tier*/gs3_step*.py
# (mit refactored Code)
```

### Phase 3: Implementierung (8-12 Std)
```
gs3_step01_*.py ← Refactored Phase 1-4
gs3_step03_*.py ← Phase 11 integriert
gs3_orchestrator.py ← Pipeline koordiniert
```

---

## Vorteile des neuen Schemas

| Aspekt | Vorteil |
|--------|---------|
| **Klarheit** | step00 < step01 < step02 < step03 < step04 (logische Reihenfolge) |
| **Finderbarkeit** | `gs3_step01_*` findet alle Tier-1 Scanner |
| **Konsistenz** | Alle Namen folgen gleichem Muster |
| **Skalierbarkeit** | Neue Scanner einfach als `gs3_step01_new_focus.py` |
| **Imports** | Strukturiert nach Verzeichnis (keine Chaos-Importe) |
| **Version** | gs3 = v3, später könnte gs4 = v4 sein |

---

## Start-Strategie

**Schritt 1:** Dokumentiere das Schema (DONE ✓)

**Schritt 2:** Erstelle Verzeichnisstruktur + Base Class
```
tools/scanners/tier0_baseline/__init__.py
tools/scanners/tier1_basic/__init__.py
tools/scanners/tier2_specialized/__init__.py
tools/scanners/tier3_hardening/__init__.py
tools/scanners/tier4_semantic/__init__.py
tools/gs3_base_scanner.py
```

**Schritt 3:** Implementiere erste Scanner (mit neuem Namen)
```
gs3_step01_memory_safety.py
gs3_step01_error_handling.py
gs3_step01_thread_safety.py
gs3_step01_raii.py
```

**Schritt 4:** Integriere Phase 11 Scanner
```
gs3_step03_data_leak.py
gs3_step03_military_hardening.py
gs3_step03_*.py
```

**Schritt 5:** Orchestrator + Tests

---

**Ready?** Sollen wir jetzt mit Schritt 2 starten?
