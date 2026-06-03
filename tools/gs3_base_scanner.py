#!/usr/bin/env python3
"""
ThemisDB Gap Scanner V3 — Base Scanner Class (OOP Architecture)

Unified base class for all scanners (Tier 0-4).
Implements common interface and data structures.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass, field, asdict
from enum import Enum
from typing import List, Optional, Dict, Tuple
from pathlib import Path
import time
import json


class ScannerPriority(Enum):
    """Pipeline execution order (fast first, expensive last)"""
    BASELINE = 0          # Ultra-fast keyword matching (~1-2 sec/file)
    MEDIUM = 1            # Basic context-aware analysis (~5-15 sec/file)
    SPECIALIZED = 2       # Domain-specific patterns (~15-40 sec/file)
    FP_FILTER = 3         # False positive reduction (~2-5 min/file, candidates only)
    SEMANTIC = 4          # AST + control flow (~5-10 min/file, candidates only)


class SeverityLevel(Enum):
    """Gap severity classification"""
    CRITICAL = "CRITICAL"
    HIGH = "HIGH"
    MEDIUM = "MEDIUM"
    LOW = "LOW"
    INFO = "INFO"


@dataclass
class Gap:
    """Unified gap representation across all scanners"""
    file: str                           # File path (relative to repo root)
    line: int                           # Line number (1-indexed)
    type: str                           # Gap type (e.g., "memory_leak", "csrf")
    severity: str                       # CRITICAL, HIGH, MEDIUM, LOW, INFO
    confidence: float                   # Detection confidence (0.0-1.0)
    description: str                    # Human-readable description
    remediation: str                    # Suggested fix
    context: Optional[str] = None       # Code context (optional)
    scanner: Optional[str] = None       # Scanner that detected (populated by orchestrator)
    step: Optional[int] = None          # Step number that detected (0-4)
    
    def __hash__(self):
        """Enable deduplication by (file, line, type)"""
        return hash((self.file, self.line, self.type))
    
    def __eq__(self, other):
        """Compare by (file, line, type) for deduplication"""
        if not isinstance(other, Gap):
            return False
        return (self.file, self.line, self.type) == (other.file, other.line, other.type)
    
    def to_dict(self) -> Dict:
        """Convert to JSON-serializable dict"""
        d = asdict(self)
        d['severity'] = self.severity
        return d
    
    @staticmethod
    def from_dict(d: Dict) -> 'Gap':
        """Reconstruct from dict"""
        return Gap(**d)


class BaseGapScanner(ABC):
    """
    Abstract base class for all gap scanners.
    
    Subclasses must implement:
    - PRIORITY: Which tier this scanner runs in
    - ENABLED: Whether scanner is enabled
    - scan(): Main scanning logic
    
    Example:
        class MemorySafetyScanner(BaseGapScanner):
            PRIORITY = ScannerPriority.MEDIUM
            
            def scan(self, source_dir: str) -> List[Gap]:
                gaps = []
                # Detection logic...
                return gaps
    """
    
    # Subclasses must define these
    PRIORITY: ScannerPriority = ScannerPriority.MEDIUM
    ENABLED: bool = True
    MAX_RUNTIME_SECONDS: int = 60
    
    def __init__(self, name: str, version: str = "1.0"):
        """
        Initialize scanner.
        
        Args:
            name: Human-readable scanner name
            version: Scanner version
        """
        self.name = name
        self.version = version
        self.gaps: List[Gap] = []
        self.runtime_ms: float = 0.0
        self.files_scanned: int = 0
    
    @abstractmethod
    def scan(self, source_dir: str) -> List[Gap]:
        """
        Scan source directory and detect gaps.
        
        Args:
            source_dir: Root directory to scan
            
        Returns:
            List of Gap objects found
        """
        pass
    
    def _scan_files(self, source_dir: str, extensions: Tuple[str, ...] = ('.cpp', '.hpp', '.h', '.c')) -> List[Path]:
        """
        Recursively find files to scan.
        
        Args:
            source_dir: Root directory
            extensions: File extensions to scan
            
        Returns:
            List of file paths
        """
        source_path = Path(source_dir)
        if not source_path.exists():
            return []
        
        files = []
        for ext in extensions:
            files.extend(source_path.rglob(f'*{ext}'))
        
        # Skip test files and build directories
        return [f for f in files if 'test' not in f.parts and 'build' not in f.parts]
    
    def _read_file_lines(self, file_path: Path) -> List[str]:
        """Safely read file lines (handle encoding errors)"""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                return f.readlines()
        except:
            return []
    
    def _get_context(self, lines: List[str], line_no: int, window: int = 5) -> List[str]:
        """
        Extract context around a line.
        
        Args:
            lines: All file lines
            line_no: Target line number (1-indexed)
            window: Number of lines before/after to include
            
        Returns:
            Context lines (including target)
        """
        idx = line_no - 1  # Convert to 0-indexed
        start = max(0, idx - window)
        end = min(len(lines), idx + window + 1)
        return lines[start:end]
    
    def _context_window_search(self, lines: List[str], line_no: int, patterns: List[str], window: int = 5) -> bool:
        """
        Check if any pattern exists in context window.
        
        Args:
            lines: All file lines
            line_no: Target line number (1-indexed)
            patterns: Patterns to search for (can be regex or string)
            window: Context window size
            
        Returns:
            True if any pattern found in context
        """
        import re
        context = self._get_context(lines, line_no, window)
        context_str = '\n'.join(context)
        
        for pattern in patterns:
            try:
                if re.search(pattern, context_str):
                    return True
            except:
                # If regex fails, try literal string match
                if pattern in context_str:
                    return True
        
        return False
    
    def deduplicate(self, gaps: List[Gap]) -> List[Gap]:
        """Remove duplicate gaps by (file, line, type)"""
        return list(dict.fromkeys(gaps))
    
    def filter_by_confidence(self, gaps: List[Gap], min_confidence: float) -> List[Gap]:
        """Filter gaps by minimum confidence threshold"""
        return [g for g in gaps if g.confidence >= min_confidence]
    
    def filter_by_severity(self, gaps: List[Gap], severities: List[str]) -> List[Gap]:
        """Filter gaps by severity level"""
        return [g for g in gaps if g.severity in severities]
    
    @staticmethod
    def merge_gap_lists(*gap_lists: List[Gap]) -> List[Gap]:
        """Merge multiple gap lists, deduplicating"""
        all_gaps = []
        for gaps in gap_lists:
            all_gaps.extend(gaps)
        return list(dict.fromkeys(all_gaps))
    
    @staticmethod
    def merge_gaps_by_confidence(gaps: List[Gap]) -> List[Gap]:
        """
        Merge duplicate gaps, keeping highest confidence.
        
        If same gap detected by multiple scanners with different confidences,
        keep highest confidence version.
        """
        gap_dict = {}
        for gap in gaps:
            key = (gap.file, gap.line, gap.type)
            if key not in gap_dict or gap.confidence > gap_dict[key].confidence:
                gap_dict[key] = gap
        
        return list(gap_dict.values())


class ScannerRegistry:
    """
    Registry pattern: manage scanner lifecycle and execution.
    
    Allows:
    - Dynamic scanner registration
    - Execution in priority order
    - Filtering by tier/priority
    """
    
    def __init__(self):
        self.scanners: Dict[str, BaseGapScanner] = {}
        self.fp_filters: List['FPFilter'] = []
    
    def register(self, scanner: BaseGapScanner, name: Optional[str] = None) -> None:
        """
        Register a scanner.
        
        Args:
            scanner: Scanner instance to register
            name: Optional custom name (defaults to scanner.name)
        """
        if scanner.ENABLED:
            key = name or scanner.name
            self.scanners[key] = scanner
    
    def register_fp_filter(self, fp_filter: 'FPFilter') -> None:
        """Register a false positive filter"""
        if fp_filter.ENABLED:
            self.fp_filters.append(fp_filter)
    
    def get_scanners_by_priority(self) -> List[BaseGapScanner]:
        """Get scanners sorted by priority (low to high cost)"""
        return sorted(
            self.scanners.values(),
            key=lambda s: s.PRIORITY.value
        )
    
    def get_scanners_by_tier(self, priority: ScannerPriority) -> List[BaseGapScanner]:
        """Get scanners for specific tier"""
        return [s for s in self.scanners.values() if s.PRIORITY == priority]
    
    def unregister(self, name: str) -> None:
        """Unregister a scanner"""
        if name in self.scanners:
            del self.scanners[name]


class GapScannerPipeline:
    """
    Execute scanners in priority order (pipeline pattern).
    
    Flow:
    1. Load all registered scanners
    2. Execute in order: Baseline → Medium → Specialized → FP Filter → Semantic
    3. Aggregate results
    4. Apply false positive filters
    5. Export results
    """
    
    def __init__(self, registry: ScannerRegistry):
        self.registry = registry
        self.all_gaps: List[Gap] = []
        self.execution_log: List[Dict] = []
    
    def execute(self, source_dir: str, verbose: bool = True) -> List[Gap]:
        """
        Run all scanners in pipeline.
        
        Args:
            source_dir: Source root directory
            verbose: Print progress information
            
        Returns:
            Final aggregated gap list
        """
        scanners = self.registry.get_scanners_by_priority()
        
        if verbose:
            print("=" * 80)
            print("GAP SCANNER V3 PIPELINE EXECUTION")
            print("=" * 80)
        
        for scanner in scanners:
            if verbose:
                print(f"\n[{scanner.PRIORITY.name}] Running {scanner.name}...")
            
            start_time = time.time()
            
            try:
                gaps = scanner.scan(source_dir)
                runtime_ms = (time.time() - start_time) * 1000
                scanner.runtime_ms = runtime_ms
                
                # Log execution
                self.execution_log.append({
                    'scanner': scanner.name,
                    'priority': scanner.PRIORITY.name,
                    'gaps_found': len(gaps),
                    'runtime_ms': runtime_ms,
                    'status': 'success'
                })
                
                if verbose:
                    print(f"  [OK] Found {len(gaps)} gaps in {runtime_ms:.1f}ms")
                
                self.all_gaps.extend(gaps)
                
            except Exception as e:
                self.execution_log.append({
                    'scanner': scanner.name,
                    'priority': scanner.PRIORITY.name,
                    'status': 'error',
                    'error': str(e)
                })
                
                if verbose:
                    print(f"  [ERROR] {e}")
        
        if verbose:
            print(f"\nTotal gaps found (pre-filter): {len(self.all_gaps)}")
        
        # Deduplicate
        self.all_gaps = BaseGapScanner.merge_gaps_by_confidence(self.all_gaps)
        
        if verbose:
            print(f"After deduplication: {len(self.all_gaps)}")
        
        # Apply FP filters
        if self.registry.fp_filters:
            if verbose:
                print("\nApplying false positive filters...")
            
            filtered_gaps = self.all_gaps
            for fp_filter in self.registry.fp_filters:
                before = len(filtered_gaps)
                filtered_gaps = fp_filter.filter(filtered_gaps)
                after = len(filtered_gaps)
                
                if verbose:
                    print(f"  {fp_filter.name}: {before} → {after} (-{before-after})")
            
            self.all_gaps = filtered_gaps
        
        if verbose:
            print(f"\nFinal result: {len(self.all_gaps)} gaps")
            print("=" * 80)
        
        return self.all_gaps
    
    def export_json(self, output_path: Path) -> None:
        """Export results to JSON"""
        data = {
            'metadata': {
                'scanner': 'ThemisDB Gap Scanner V3',
                'total_gaps': len(self.all_gaps),
                'execution_log': self.execution_log
            },
            'gaps': [gap.to_dict() for gap in self.all_gaps]
        }
        
        with open(output_path, 'w') as f:
            json.dump(data, f, indent=2)


class FPFilter(ABC):
    """Base class for false positive filters (Wave 5-6)"""
    
    ENABLED: bool = True
    
    def __init__(self, name: str):
        self.name = name
    
    @abstractmethod
    def filter(self, gaps: List[Gap]) -> List[Gap]:
        """Filter out false positives from gap list"""
        pass
