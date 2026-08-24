# Gap Scanner V3 — Pipeline-Architektur mit OOP & SoC

**Prinzip:** Schnelle, grobe Scanner zuerst (Baseline) → Progressive Spezialisierung & FP-Filterung

---

## 1. BEST PRACTICE: PIPELINE-ARCHITEKTUR

### Klassisches Vorbild: Unix-Pipeline
```bash
# Schnelle Filter zuerst → Spezialisierte Filter später
cat data.log | grep "ERROR" | sed 's/old/new/' | awk '{print $1}' | sort | uniq
               ↑ Fast         ↑ Medium           ↑ Slow                ↑ Specialized
```

### Gap Scanner V3 Pipeline (gleiches Konzept)
```
Repository
    ↓
[Phase 0: Baseline Scanner]      (Fast, alles flaggen)
    ├─ Keyword matching
    ├─ Regex patterns
    └─ ~1-2 seconds/file
    ↓ (Results: hochvoluming, viele FP)
[Phase 1-4: Basis-Analyse]       (Medium, grobe Klassifikation)
    ├─ Context window ±5 lines
    ├─ Type checking
    └─ ~5-10 seconds/file
    ↓ (Results: -30% FP durch Kontext)
[Phase 5+: Spezialisierung]      (Medium-Slow, spezielle Patterns)
    ├─ Security-spezifische Patterns
    ├─ Memory safety checks
    └─ ~10-20 seconds/file
    ↓ (Results: -50% FP durch Domain-Knowledge)
[Wave 1-3: Progressive FP Filter] (Slow, Kontext-Fenster größer)
    ├─ Wave 1: ±0 lines (lokal)
    ├─ Wave 2: ±2 lines
    ├─ Wave 3: ±5 lines
    └─ ~30-60 seconds/file
    ↓ (Results: -70% FP durch weiter Kontext)
[Wave 5-6: Semantic Analysis]    (Very Slow, AST/Control Flow)
    ├─ AST parsing
    ├─ Data flow tracing
    ├─ Control flow analysis
    └─ ~2-5 minutes/file (nur auf Resultat-Kandidaten)
    ↓
Clean Results (with confidence scores)
```

---

## 2. OOP STRUKTUR: Base Scanner + Spezialisierung

### Base Scanner Class
```python
from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
from typing import List, Optional

class ScannerPriority(Enum):
    """Pipeline execution order"""
    BASELINE = 0      # Fast keyword matching
    MEDIUM = 1        # Context-aware analysis
    SPECIALIZED = 2   # Domain-specific patterns
    FP_FILTER = 3     # False positive reduction
    SEMANTIC = 4      # AST/data flow (most expensive)

@dataclass
class Gap:
    """Standard gap representation"""
    file: str
    line: int
    type: str
    severity: str  # CRITICAL, HIGH, MEDIUM, LOW
    confidence: float  # 0.0-1.0
    description: str
    remediation: str
    context: Optional[str] = None
    
    def __hash__(self):
        return hash((self.file, self.line, self.type))

class BaseGapScanner(ABC):
    """Base class for all scanners"""
    
    # Implementation requirements
    PRIORITY: ScannerPriority = ScannerPriority.MEDIUM
    ENABLED: bool = True
    MAX_RUNTIME_SECONDS: int = 60
    
    def __init__(self, name: str, version: str = "1.0"):
        self.name = name
        self.version = version
        self.gaps: List[Gap] = []
        self.runtime_ms = 0
        
    @abstractmethod
    def scan(self, source_dir: str, patterns: dict) -> List[Gap]:
        """Scan source directory and return gaps"""
        pass
    
    def deduplicate(self, gaps: List[Gap]) -> List[Gap]:
        """Remove duplicate gaps by (file, line, type)"""
        return list(dict.fromkeys(gaps))
    
    def filter_by_confidence(self, gaps: List[Gap], min_confidence: float) -> List[Gap]:
        """Filter gaps by minimum confidence"""
        return [g for g in gaps if g.confidence >= min_confidence]
    
    @staticmethod
    def merge_gap_lists(*gap_lists: List[Gap]) -> List[Gap]:
        """Merge multiple gap lists, deduplicating"""
        all_gaps = []
        for gaps in gap_lists:
            all_gaps.extend(gaps)
        return list(dict.fromkeys(all_gaps))

# Example: Baseline Security Scanner
class BaselineSecurityScanner(BaseGapScanner):
    """Phase 0: Fast keyword-based security patterns"""
    PRIORITY = ScannerPriority.BASELINE
    
    def __init__(self):
        super().__init__("BaselineSecurityScanner", "1.0")
        self.patterns = {
            "hardcoded_password": [r"password\s*=\s*['\"].*['\"]"],
            "hardcoded_api_key": [r"api[_-]?key\s*=\s*['\"].*['\"]"],
            "hardcoded_secret": [r"secret\s*=\s*['\"].*['\"]"],
            "sql_placeholder": [r"SELECT.*WHERE.*%s", r"INSERT.*VALUES.*%s"],
        }
    
    def scan(self, source_dir: str, patterns: dict) -> List[Gap]:
        gaps = []
        # Scan files...
        for file_path in scan_files(source_dir):
            with open(file_path) as f:
                for line_no, line in enumerate(f, 1):
                    for pattern_name, regexes in patterns.items():
                        for regex in regexes:
                            if re.search(regex, line):
                                gaps.append(Gap(
                                    file=file_path,
                                    line=line_no,
                                    type=pattern_name,
                                    severity="HIGH",
                                    confidence=0.5,  # LOW confidence (keyword only)
                                    description=f"Potential {pattern_name}",
                                    remediation="Review and remove hardcoded values"
                                ))
        return self.deduplicate(gaps)

# Example: Context-Aware Security Scanner (Phase 1)
class ContextSecurityScanner(BaseGapScanner):
    """Phase 1: Security patterns with ±5 line context"""
    PRIORITY = ScannerPriority.MEDIUM
    
    def __init__(self):
        super().__init__("ContextSecurityScanner", "1.0")
    
    def scan(self, source_dir: str, patterns: dict) -> List[Gap]:
        gaps = []
        # Scan with context window...
        for file_path in scan_files(source_dir):
            lines = read_file_lines(file_path)
            for line_no, line in enumerate(lines, 1):
                if self._is_sensitive_operation(line):
                    context = self._get_context(lines, line_no, window=5)
                    if self._has_protection_in_context(context):
                        continue  # False positive filtered
                    gaps.append(Gap(
                        file=file_path,
                        line=line_no,
                        type="unprotected_operation",
                        severity="HIGH",
                        confidence=0.75,  # HIGHER confidence (context verified)
                        description="Sensitive operation without protection",
                        remediation="Add appropriate security checks",
                        context="\n".join(context)
                    ))
        return gaps
    
    def _is_sensitive_operation(self, line: str) -> bool:
        return bool(re.search(r"(execute|query|system|exec|eval)", line))
    
    def _get_context(self, lines: List[str], line_no: int, window: int) -> List[str]:
        start = max(0, line_no - window - 1)
        end = min(len(lines), line_no + window)
        return lines[start:end]
    
    def _has_protection_in_context(self, context: List[str]) -> bool:
        protection_patterns = [r"validate", r"sanitize", r"check", r"assert"]
        context_str = "\n".join(context)
        return any(re.search(p, context_str) for p in protection_patterns)

# Example: Specialized Security Scanner (Phase 11)
class SpecializedSecurityScanner(BaseGapScanner):
    """Phase 11: Domain-specific security patterns (highest confidence)"""
    PRIORITY = ScannerPriority.SPECIALIZED
    
    def __init__(self):
        super().__init__("SpecializedSecurityScanner", "1.0")
    
    def scan(self, source_dir: str, patterns: dict) -> List[Gap]:
        gaps = []
        # OWASP Top 10 specific detection...
        for file_path in scan_files(source_dir):
            # Phase 11: Attack vectors, military hardening, etc.
            gaps.extend(self._check_csrf_vulnerability(file_path))
            gaps.extend(self._check_sql_injection(file_path))
            gaps.extend(self._check_classified_data_protection(file_path))
        return gaps
    
    def _check_csrf_vulnerability(self, file_path: str) -> List[Gap]:
        """Detect state-changing ops without CSRF tokens"""
        gaps = []
        lines = read_file_lines(file_path)
        for line_no, line in enumerate(lines, 1):
            if self._is_state_changing_operation(line):
                context = self._get_context(lines, line_no, window=5)
                if not self._has_csrf_token_in_context(context):
                    gaps.append(Gap(
                        file=file_path,
                        line=line_no,
                        type="csrf_vulnerability",
                        severity="HIGH",
                        confidence=0.80,  # HIGH confidence (domain-specific)
                        description="State-changing operation without CSRF protection",
                        remediation="Add CSRF token validation"
                    ))
        return gaps
    
    def _is_state_changing_operation(self, line: str) -> bool:
        return bool(re.search(r"(POST|PUT|DELETE|PATCH)", line))
    
    def _has_csrf_token_in_context(self, context: List[str]) -> bool:
        return any(re.search(r"csrf.*token|token.*csrf", line, re.I) 
                   for line in context)

# Example: Wave 5 FP Filter (uses results from earlier phases)
class Wave5FPFilter(BaseGapScanner):
    """Wave 5: Aggressive false positive filtering via extended context"""
    PRIORITY = ScannerPriority.FP_FILTER
    
    def __init__(self):
        super().__init__("Wave5FPFilter", "1.0")
    
    def filter_results(self, gaps: List[Gap]) -> List[Gap]:
        """Filter out false positives from earlier scanners"""
        filtered = []
        for gap in gaps:
            context = self._get_extended_context(gap.file, gap.line, window=10)
            if self._is_genuine_gap(gap, context):
                gap.confidence += 0.1  # Boost confidence after filtering
                filtered.append(gap)
            else:
                # Gap marked as filtered out (could log separately)
                pass
        return filtered
    
    def _get_extended_context(self, file_path: str, line_no: int, window: int) -> List[str]:
        lines = read_file_lines(file_path)
        start = max(0, line_no - window - 1)
        end = min(len(lines), line_no + window)
        return lines[start:end]
    
    def _is_genuine_gap(self, gap: Gap, context: List[str]) -> bool:
        # Check if gap is genuine in extended context
        context_str = "\n".join(context)
        # If helper function or utility code, likely FP
        if re.search(r"def.*helper|class.*util|def.*test", context_str):
            return False
        return True
```

---

## 3. SCANNER REGISTRY & PIPELINE ORCHESTRATOR

```python
from typing import Dict, List, Callable
from enum import Enum
import time

class ScannerRegistry:
    """Registry for all scanners (plugin architecture)"""
    
    def __init__(self):
        self.scanners: Dict[str, BaseGapScanner] = {}
        self.fp_filters: List[Callable] = []
    
    def register(self, scanner: BaseGapScanner) -> None:
        """Register a scanner"""
        if scanner.ENABLED:
            key = f"{scanner.PRIORITY.name}_{scanner.name}"
            self.scanners[key] = scanner
    
    def register_fp_filter(self, filter_func: Callable) -> None:
        """Register a false positive filter"""
        self.fp_filters.append(filter_func)
    
    def get_scanners_by_priority(self) -> List[BaseGapScanner]:
        """Get scanners sorted by priority (fast first)"""
        return sorted(
            self.scanners.values(),
            key=lambda s: s.PRIORITY.value
        )

class GapScannerPipeline:
    """Execute scanners in pipeline order"""
    
    def __init__(self, registry: ScannerRegistry):
        self.registry = registry
        self.all_gaps: List[Gap] = []
    
    def execute(self, source_dir: str) -> List[Gap]:
        """Run all scanners in priority order"""
        scanners = self.registry.get_scanners_by_priority()
        
        print("=" * 80)
        print("GAP SCANNER V3 PIPELINE EXECUTION")
        print("=" * 80)
        
        for scanner in scanners:
            print(f"\n[{scanner.PRIORITY.name}] Running {scanner.name}...")
            start_time = time.time()
            
            try:
                gaps = scanner.scan(source_dir, patterns={})
                runtime_ms = (time.time() - start_time) * 1000
                scanner.runtime_ms = runtime_ms
                
                print(f"  ✓ Found {len(gaps)} gaps in {runtime_ms:.1f}ms")
                self.all_gaps.extend(gaps)
                
            except Exception as e:
                print(f"  ✗ Error: {e}")
        
        print(f"\nTotal gaps found: {len(self.all_gaps)}")
        
        # Apply FP filters
        print("\nApplying false positive filters...")
        filtered_gaps = self._apply_fp_filters(self.all_gaps)
        print(f"After FP filtering: {len(filtered_gaps)} gaps")
        
        return filtered_gaps
    
    def _apply_fp_filters(self, gaps: List[Gap]) -> List[Gap]:
        """Apply progressive false positive filtering"""
        for fp_filter in self.registry.fp_filters:
            gaps = fp_filter(gaps)
        return gaps

# Usage example:
registry = ScannerRegistry()
registry.register(BaselineSecurityScanner())
registry.register(ContextSecurityScanner())
registry.register(SpecializedSecurityScanner())
registry.register(Wave5FPFilter())

pipeline = GapScannerPipeline(registry)
results = pipeline.execute("/path/to/source")
```

---

## 4. SCANNER NUMMERIERUNG NACH RELEVANZ & AUFWAND

```
PRIORITY LEVEL 0: BASELINE (FAST, BROAD)
├─ Phase 0.1: Hardcoded Credentials Scanner
│  ├─ Pattern: Simple keyword matching (password, api_key, secret)
│  ├─ Context: None
│  ├─ Runtime: 1-2 sec/file
│  ├─ Confidence: 0.5 (high FP)
│  └─ Volume: ~10,000 gaps (raw)
│
├─ Phase 0.2: Dangerous Function Calls Scanner
│  ├─ Pattern: System(), eval(), exec(), dangerous_fn()
│  ├─ Context: None
│  ├─ Runtime: 1-2 sec/file
│  ├─ Confidence: 0.4 (very high FP)
│  └─ Volume: ~5,000 gaps (raw)

PRIORITY LEVEL 1: BASIC (MEDIUM, CONTEXT-AWARE)
├─ Phase 1.1: Memory Safety (Memory Scanner)
│  ├─ Pattern: new/delete, malloc/free, pointer usage
│  ├─ Context: ±5 lines
│  ├─ Runtime: 5-10 sec/file
│  ├─ Confidence: 0.65
│  └─ Volume: ~2,000 gaps (filtered)
│
├─ Phase 1.2: Error Handling (Reliability Scanner)
│  ├─ Pattern: throw, try-catch, error returns
│  ├─ Context: ±5 lines
│  ├─ Runtime: 5-10 sec/file
│  ├─ Confidence: 0.70
│  └─ Volume: ~1,500 gaps (filtered)
│
├─ Phase 1.3: Thread Safety (Concurrency Scanner)
│  ├─ Pattern: mutex, lock_guard, atomic
│  ├─ Context: ±5 lines
│  ├─ Runtime: 5-10 sec/file
│  ├─ Confidence: 0.65
│  └─ Volume: ~800 gaps (filtered)
│
└─ Phase 1.4: Resource Management (RAII Scanner)
   ├─ Pattern: unique_ptr, shared_ptr, new/delete
   ├─ Context: ±5 lines
   ├─ Runtime: 10-15 sec/file
   ├─ Confidence: 0.70
   └─ Volume: ~600 gaps (filtered)

PRIORITY LEVEL 2: SPECIALIZED (MEDIUM-SLOW, DOMAIN-SPECIFIC)
├─ Phase 2.1: Type Safety (Type Conversion Scanner)
│  ├─ Pattern: Narrowing conversions, unsafe casts
│  ├─ Context: Type analysis
│  ├─ Runtime: 10-15 sec/file
│  ├─ Confidence: 0.75
│  └─ Volume: ~400 gaps
│
├─ Phase 2.2: Input Validation (Input Validation Scanner)
│  ├─ Pattern: Unchecked parameters, bounds violations
│  ├─ Context: Function signatures + ±5 lines
│  ├─ Runtime: 10-15 sec/file
│  ├─ Confidence: 0.70
│  └─ Volume: ~300 gaps
│
└─ Phase 2.3: Security (Attack Vector Scanner)
   ├─ Pattern: SQL injection, command injection, XSS, CSRF
   ├─ Context: OWASP-specific patterns ±10 lines
   ├─ Runtime: 15-20 sec/file
   ├─ Confidence: 0.75-0.85
   └─ Volume: ~600 gaps

PRIORITY LEVEL 3: HARDENING (SLOW, MILITARY-GRADE)
├─ Phase 3.1: Encryption & Keys (Military Hardening Scanner)
│  ├─ Pattern: Plaintext storage, weak KDF, key rotation
│  ├─ Context: Encryption operations ±5 lines
│  ├─ Runtime: 20-30 sec/file
│  ├─ Confidence: 0.80
│  └─ Volume: ~1,050 gaps
│
├─ Phase 3.2: Data Protection (Data Leak Scanner)
│  ├─ Pattern: PII exposure, logging secrets, unzeroed memory
│  ├─ Context: Sensitive operations ±5 lines
│  ├─ Runtime: 20-30 sec/file
│  ├─ Confidence: 0.80
│  └─ Volume: ~150 gaps
│
└─ Phase 3.3: Audit & Compliance (Audit Logging Scanner)
   ├─ Pattern: Missing audit logs, untraced operations
   ├─ Context: System calls ±10 lines
   ├─ Runtime: 30-40 sec/file
   ├─ Confidence: 0.75
   └─ Volume: ~400 gaps

PRIORITY LEVEL 4: FP FILTERING (VERY SLOW, CONTEXT-HEAVY)
├─ Wave 1 (±0 lines): Already in baseline scanner
├─ Wave 2 (±2 lines): Fine-tune context
├─ Wave 3 (±5 lines): Standard context (most scanners use this)
├─ Wave 4 (±10 lines): Aggressive context filtering
└─ Wave 5-6: Semantic analysis
   ├─ Pattern: AST + control flow + data flow
   ├─ Context: Full function scope + call graph
   ├─ Runtime: 2-5 min/file (only on candidates)
   ├─ Confidence: 0.90+
   └─ Volume: ~4,458 genuine gaps (Phase 11 baseline)
```

---

## 5. KONSOLIDIERTES SCANNER-SCHEMA

```python
# New scanner numbering scheme (relevance-based)

SCANNER_PHASES = [
    # Tier 0: Ultra-fast baseline (keyword matching)
    ("Phase 0.1", BaselineSecurityScanner, ScannerPriority.BASELINE, "1-2 sec"),
    ("Phase 0.2", DangerousFunctionScanner, ScannerPriority.BASELINE, "1-2 sec"),
    
    # Tier 1: Basic context-aware (±5 lines)
    ("Phase 1.1", MemorySafetyScanner, ScannerPriority.MEDIUM, "5-10 sec"),
    ("Phase 1.2", ReliabilityScanner, ScannerPriority.MEDIUM, "5-10 sec"),
    ("Phase 1.3", ConcurrencyScanner, ScannerPriority.MEDIUM, "5-10 sec"),
    ("Phase 1.4", RAIIScanner, ScannerPriority.MEDIUM, "10-15 sec"),
    
    # Tier 2: Specialized domain patterns
    ("Phase 2.1", TypeConversionScanner, ScannerPriority.SPECIALIZED, "10-15 sec"),
    ("Phase 2.2", InputValidationScanner, ScannerPriority.SPECIALIZED, "10-15 sec"),
    ("Phase 2.3", AttackVectorScanner, ScannerPriority.SPECIALIZED, "15-20 sec"),
    
    # Tier 3: Hardening & compliance
    ("Phase 3.1", MilitaryHardeningScanner, ScannerPriority.SPECIALIZED, "20-30 sec"),
    ("Phase 3.2", DataLeakScanner, ScannerPriority.SPECIALIZED, "20-30 sec"),
    ("Phase 3.3", AuditLoggingScanner, ScannerPriority.SPECIALIZED, "30-40 sec"),
    
    # Tier 4: False positive reduction (semantic)
    ("Wave 5-6", SemanticFPFilter, ScannerPriority.FP_FILTER, "2-5 min (candidates only)"),
]

# Result: Progressive refinement from ~15,000 raw gaps → 4,458 genuine gaps
```

---

## 6. DATEI-STRUKTUR (OOP & SoC)

```
tools/
├─ gap_scanner_v3_pipeline.py          (Main orchestrator)
│  └─ GapScannerPipeline
│  └─ ScannerRegistry
│
├─ scanners/
│  ├─ base_scanner.py                   (OOP base class)
│  │  └─ BaseGapScanner
│  │  └─ Gap
│  │  └─ ScannerPriority
│  │
│  ├─ tier0_baseline/
│  │  ├─ credentials.py                 (Phase 0.1)
│  │  └─ dangerous_functions.py         (Phase 0.2)
│  │
│  ├─ tier1_basic/
│  │  ├─ memory_safety.py               (Phase 1.1)
│  │  ├─ reliability.py                 (Phase 1.2)
│  │  ├─ concurrency.py                 (Phase 1.3)
│  │  └─ raii.py                        (Phase 1.4)
│  │
│  ├─ tier2_specialized/
│  │  ├─ type_conversion.py             (Phase 2.1)
│  │  ├─ input_validation.py            (Phase 2.2)
│  │  └─ attack_vectors.py              (Phase 2.3)
│  │
│  ├─ tier3_hardening/
│  │  ├─ military_hardening.py          (Phase 3.1)
│  │  ├─ data_leak.py                   (Phase 3.2)
│  │  └─ audit_logging.py               (Phase 3.3)
│  │
│  └─ tier4_semantic/
│     ├─ wave5_fp_filter.py             (Wave 5)
│     └─ wave6_semantic.py              (Wave 6)
│
├─ utils/
│  ├─ file_scanner.py                   (Shared file iteration)
│  ├─ context_window.py                 (Shared context extraction)
│  ├─ confidence_scoring.py             (Shared confidence logic)
│  └─ output.py                         (Shared JSON export)
│
└─ README_SCANNER_PIPELINE.md           (This documentation)
```

---

## 7. PRINCIPLE: SEPARATION OF CONCERNS (SoC)

```
✅ GOOD (SoC-compliant):

class MemorySafetyScanner(BaseGapScanner):
    """ONLY detects memory safety gaps"""
    def scan(self):
        - new/delete usage
        - malloc/free usage
        - Pointer operations
        # Does NOT do:
        # - FP filtering (that's Wave 5-6 job)
        # - Type checking (that's Phase 2.1 job)
        # - Security analysis (that's Phase 2.3 job)

class Wave5FPFilter(BaseGapScanner):
    """ONLY filters false positives"""
    def filter(self):
        - Take Phase 1-3 results
        - Apply extended context (±10 lines)
        - Return filtered list
        # Does NOT do:
        # - New gap detection (that's Phase 1-3 job)
        # - Output formatting (that's utils/output.py job)

❌ BAD (mixing concerns):

class MemorySafetyAndFPFilterScanner:
    """Mixes detection and filtering — WRONG!"""
    def scan(self):
        - Detect memory gaps
        - Apply FP filtering locally
        - Do type checking too
        - Format JSON output
        # This violates SoC: 4 responsibilities in 1 class
```

---

## 8. VORTEILE DIESER ARCHITEKTUR

| Aspekt | Vorteil |
|--------|---------|
| **Performance** | Schnelle Scanner zuerst, teure Scanner nur auf Kandidaten |
| **Clarity** | Jeder Scanner hat 1 Verantwortung (SoC) |
| **Testability** | Jede Tier kann isoliert getestet werden |
| **Extensibility** | Neue Scanner einfach registrieren |
| **Maintainability** | Keine Vermischung von Concerns |
| **Configurability** | Phasen an/ausschaltbar (ENABLED flag) |
| **Confidence** | Early phases: low confidence, later phases: high confidence |
| **Reproducibility** | Klare Pipeline → reproduzierbare Ergebnisse |

---

## 9. NÄCHSTE SCHRITTE

1. **Analyiere Phase 1-4, 2, 3 Scanners** nach dieser Struktur
2. **Refaktoriere auf Tier-Basis** (nicht Phase-Basis)
3. **Definiere klare SoC-Grenzen** pro Scanner
4. **Implementiere Base Scanner Class** als gemeinsame Basis
5. **Erstelle Pipeline Orchestrator** mit Registry
6. **Teste** Tier für Tier (0 → 1 → 2 → 3 → 4)

---

**Best Practice Summary:**

✅ **OOP:** Base Class mit Subklassen pro Tier  
✅ **SoC:** 1 Scanner = 1 Verantwortung  
✅ **Pipeline:** Schnell → Spezialisiert → FP-Filter  
✅ **Registry:** Plugin-Architektur für Erweiterbarkeit  
✅ **Confidence:** Frühe Phasen low, späte Phasen high  
