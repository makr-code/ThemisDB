#!/usr/bin/env python3
"""
False Positive Detection Engine — Advanced FP Elimination
========================================================

Unified framework for detecting and eliminating false positives from gap scanner output.

Features:
1. Pattern-specific FP detectors (Memory, Audit, Encryption, etc.)
2. Context-aware analysis (scope, control flow, type information)
3. Multi-factor confidence scoring
4. Approved pattern whitelisting
5. Automatic classification (Real Gap | Guarded Stub | Test Mock | FP | Placeholder)

Usage:
    engine = FPDetectionEngine(repo_root='.')
    filtered_gaps = engine.filter_gaps(raw_gaps)
"""

import re
import json
from pathlib import Path
from typing import List, Dict, Any, Tuple, Set
from dataclasses import dataclass, asdict
from abc import ABC, abstractmethod
from enum import Enum
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class GapClassification(Enum):
    """Classification of gap after FP analysis"""
    REAL_GAP = "REAL_GAP"              # Unimplemented production code, no guards
    GUARDED_STUB = "GUARDED_STUB"      # Has if/guard, defensive pattern
    TEST_MOCK = "TEST_MOCK"            # In test code with marker
    FALSE_POSITIVE = "FALSE_POSITIVE"  # Scanner error or benign code
    PLACEHOLDER = "PLACEHOLDER"        # Marked TODO/FIXME/STUB (Phase N+1)


class ConfidenceLevel(Enum):
    """Confidence score categorization"""
    CRITICAL = (0.85, 1.00)   # High confidence true positives
    HIGH = (0.70, 0.85)        # Likely true positives
    MEDIUM = (0.50, 0.70)      # Mixed; requires filtering
    LOW = (0.00, 0.50)         # Likely false positives


@dataclass
class GapAnalysis:
    """Analysis result for a single gap"""
    gap: Dict[str, Any]
    classification: GapClassification
    original_severity: str
    verified_severity: str
    confidence_score: float
    is_false_positive: bool
    reason: str
    evidence: List[str]  # Code snippets or patterns supporting classification


class ContextAnalyzer:
    """Analyzes code context around gaps"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
    
    def read_context(self, file_path: str, line_num: int, context_lines: int = 15) -> Tuple[List[str], int]:
        """
        Read code context around a gap.
        
        Returns:
            (lines: List[str], start_line: int)
        """
        try:
            full_path = self.repo_root / file_path
            with open(full_path, 'r', encoding='utf-8', errors='ignore') as f:
                all_lines = f.readlines()
            
            start = max(0, line_num - context_lines - 1)
            end = min(len(all_lines), line_num + context_lines)
            
            return all_lines[start:end], start + 1
        except Exception as e:
            logger.warning(f"Failed to read context for {file_path}:{line_num}: {e}")
            return [], line_num
    
    def is_test_code(self, file_path: str) -> bool:
        """Check if file is test code"""
        return bool(re.search(r'test_|_test\.cpp|_test\.hpp|/tests?/', file_path, re.IGNORECASE))
    
    def is_example_code(self, file_path: str) -> bool:
        """Check if file is example or documentation code"""
        return bool(re.search(r'example|demo|doc/', file_path, re.IGNORECASE))
    
    def find_guards(self, lines: List[str]) -> List[str]:
        """Find guard patterns (if, guard, assert) in context"""
        guards = []
        for line in lines:
            if re.search(r'\bif\s*\(|guard|DCHECK|assert', line):
                guards.append(line.strip())
        return guards
    
    def find_placeholders(self, lines: List[str]) -> List[str]:
        """Find placeholder markers (TODO, FIXME, STUB)"""
        markers = []
        for line in lines:
            if re.search(r'\b(TODO|FIXME|STUB|TEMPORARY)\b', line):
                markers.append(line.strip())
        return markers
    
    def is_in_crypto_block(self, lines: List[str]) -> bool:
        """Check if code is in crypto/security handling context"""
        crypto_keywords = r'encrypt|decrypt|sign|verify|hash|cipher|secret|key|password|auth'
        context = ' '.join(lines)
        return bool(re.search(crypto_keywords, context, re.IGNORECASE))
    
    def is_in_test_fixture(self, lines: List[str]) -> bool:
        """Check if code is in test fixture or mock section"""
        test_keywords = r'// MOCK|// TEST|// FIXTURE|TEST_F|MOCK_|@Test|def test_'
        context = ' '.join(lines)
        return bool(re.search(test_keywords, context, re.IGNORECASE))


class SpecializedFPDetector(ABC):
    """Base class for pattern-specific FP detectors"""
    
    def __init__(self, name: str, context_analyzer: ContextAnalyzer):
        self.name = name
        self.context = context_analyzer
    
    @abstractmethod
    def detect(self, gap: Dict[str, Any], lines: List[str], start_line: int) -> Tuple[bool, str, float]:
        """
        Detect if this gap is a false positive.
        
        Returns:
            (is_fp: bool, reason: str, confidence_adjustment: float)
        """
        pass


class MemorySafetyFPDetector(SpecializedFPDetector):
    """Detects false positives in memory safety patterns"""
    
    def __init__(self, context_analyzer: ContextAnalyzer):
        super().__init__("MemorySafety", context_analyzer)
    
    def detect(self, gap: Dict[str, Any], lines: List[str], start_line: int) -> Tuple[bool, str, float]:
        """
        Detect unzeroed_memory false positives.
        Only flag allocations in crypto/security contexts.
        """
        pattern = gap.get('pattern', '').lower()
        
        # Only process memory-related gaps
        if 'unzeroed' not in pattern and 'memory' not in pattern:
            return False, "", 0.0
        
        context_text = ' '.join(lines)
        
        # Check if in security context
        if self.context.is_in_crypto_block(lines):
            return False, "In crypto/security block", 0.15  # Increase confidence
        
        # Check for generic buffer allocations (HTTP responses, etc.)
        if re.search(r'vector.*response|buffer.*http|malloc.*body|new.*response', context_text, re.IGNORECASE):
            return True, "Generic buffer allocation, not sensitive data", -0.30
        
        # Check for common safe patterns
        safe_patterns = [
            r'std::make_unique',
            r'std::make_shared',
            r'DCHECK.*nullptr',
            r'guard\s*\(',
            r'std::unique_ptr.*reset',
            r'std::scoped_guard',
        ]
        
        for safe_pattern in safe_patterns:
            if re.search(safe_pattern, context_text):
                return True, "Safe allocation pattern detected", -0.25
        
        # Check for initialization patterns
        if re.search(r'memset.*0|zero_memory|secure_zero|OPENSSL_cleanse', context_text):
            return True, "Memory is explicitly zeroed", -0.30
        
        return False, "Requires manual review", 0.0


class AuditLoggingFPDetector(SpecializedFPDetector):
    """Detects false positives in audit logging requirements"""
    
    def __init__(self, context_analyzer: ContextAnalyzer):
        super().__init__("AuditLogging", context_analyzer)
        self.security_ops = {
            'authenticate', 'authorize', 'encrypt', 'decrypt',
            'sign', 'verify', 'key_derive', 'key_generate'
        }
        self.safe_functions = {
            'init', 'cleanup', 'destructor', 'helper', 'util',
            'format', 'parse', 'validate_input', 'get_config'
        }
    
    def detect(self, gap: Dict[str, Any], lines: List[str], start_line: int) -> Tuple[bool, str, float]:
        """
        Detect missing_audit_log false positives.
        Only flag actual security operations, not utility functions.
        """
        pattern = gap.get('pattern', '').lower()
        
        if 'audit' not in pattern and 'logging' not in pattern:
            return False, "", 0.0
        
        context_text = ' '.join(lines).lower()
        
        # Check if it's a security operation
        is_security_op = any(op in context_text for op in self.security_ops)
        
        # Check if it's a safe utility function
        is_safe_func = any(safe in context_text for safe in self.safe_functions)
        
        if not is_security_op or is_safe_func:
            return True, "Utility function, not security operation", -0.35
        
        # Check if logging already exists
        if re.search(r'LOG\(|logger\.|slog\.|audit_log|security_log', context_text, re.IGNORECASE):
            return True, "Audit logging already present", -0.40
        
        # Check for internal/private functions
        if re.search(r'private:|_impl::|internal::|detail::', context_text):
            return True, "Internal implementation function", -0.25
        
        return False, "Requires manual review", 0.0


class EncryptionContextFPDetector(SpecializedFPDetector):
    """Detects false positives in encryption requirements"""
    
    def __init__(self, context_analyzer: ContextAnalyzer):
        super().__init__("EncryptionContext", context_analyzer)
    
    def detect(self, gap: Dict[str, Any], lines: List[str], start_line: int) -> Tuple[bool, str, float]:
        """
        Detect classified_data_unprotected and encryption false positives.
        Require evidence of plaintext storage, not just naming.
        """
        pattern = gap.get('pattern', '').lower()
        
        if 'encrypt' not in pattern and 'classified' not in pattern and 'secret' not in pattern:
            return False, "", 0.0
        
        context_text = ' '.join(lines)
        
        # Check if data is actually encrypted
        encryption_patterns = [
            r'\.encrypt\(',
            r'Cipher\(.*mode.*',
            r'EVP_',
            r'AES_',
            r'sodium_crypto',
            r'libsodium',
            r'openssl',
            r'boringssl',
        ]
        
        for enc_pattern in encryption_patterns:
            if re.search(enc_pattern, context_text, re.IGNORECASE):
                return True, "Data is encrypted in context", -0.35
        
        # Check for database/file protection
        if re.search(r'encrypted_db|secure_storage|CryptFS|dm-crypt', context_text):
            return True, "Protected storage layer", -0.30
        
        # Check if just a naming convention without actual plaintext
        if not re.search(r'=\s*"[^"]*"|file.*write|db.*insert|plaintext', context_text):
            return True, "No evidence of plaintext storage, likely naming convention", -0.40
        
        return False, "Requires manual review", 0.0


class TestCodeFPDetector(SpecializedFPDetector):
    """Filters out gaps in test, example, and documentation code"""
    
    def __init__(self, context_analyzer: ContextAnalyzer):
        super().__init__("TestCode", context_analyzer)
    
    def detect(self, gap: Dict[str, Any], lines: List[str], start_line: int) -> Tuple[bool, str, float]:
        """
        Eliminate all gaps in test and example code.
        """
        file_path = gap.get('file', '')
        
        # Check file path
        if self.context.is_test_code(file_path):
            return True, "Gap in test code", -1.0  # Max confidence reduction
        
        if self.context.is_example_code(file_path):
            return True, "Gap in example/documentation code", -0.95
        
        # Check for test markers in content
        if self.context.is_in_test_fixture(lines):
            return True, "Gap in test fixture/mock", -0.90
        
        return False, "", 0.0


class PlaceholderDetector(SpecializedFPDetector):
    """Detects intentional placeholders (TODO, FIXME, STUB)"""
    
    def __init__(self, context_analyzer: ContextAnalyzer):
        super().__init__("Placeholder", context_analyzer)
    
    def detect(self, gap: Dict[str, Any], lines: List[str], start_line: int) -> Tuple[bool, str, float]:
        """
        Downgrade severity for intentional placeholders.
        These are expected for Phase N+1 work.
        """
        markers = self.context.find_placeholders(lines)
        
        if markers:
            return True, f"Intentional placeholder: {markers[0]}", -0.50
        
        return False, "", 0.0


class GuardedStubDetector(SpecializedFPDetector):
    """Detects guarded stubs (defensive patterns)"""
    
    def __init__(self, context_analyzer: ContextAnalyzer):
        super().__init__("GuardedStub", context_analyzer)
    
    def detect(self, gap: Dict[str, Any], lines: List[str], start_line: int) -> Tuple[bool, str, float]:
        """
        Downgrade severity for guarded stubs.
        These are defensive patterns and not critical.
        """
        guards = self.context.find_guards(lines)
        
        if guards:
            return True, f"Guarded stub pattern: {guards[0]}", -0.35
        
        return False, "", 0.0


class FPDetectionEngine:
    """
    Main false positive detection engine.
    Orchestrates all specialized detectors.
    """
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.context = ContextAnalyzer(repo_root)
        
        # Initialize all detectors in priority order
        self.detectors: List[SpecializedFPDetector] = [
            TestCodeFPDetector(self.context),                    # Fast, high-confidence
            PlaceholderDetector(self.context),                   # Fast, high-confidence
            GuardedStubDetector(self.context),                   # Fast, medium-confidence
            MemorySafetyFPDetector(self.context),                # Medium complexity
            AuditLoggingFPDetector(self.context),                # Medium complexity
            EncryptionContextFPDetector(self.context),           # Higher complexity
        ]
        
        self.stats = {
            'total_gaps': 0,
            'gaps_filtered': 0,
            'confidence_adjustments': [],
            'detector_hits': {d.name: 0 for d in self.detectors},
        }
    
    def filter_gaps(self, gaps: List[Dict[str, Any]]) -> Tuple[List[GapAnalysis], Dict[str, Any]]:
        """
        Filter gaps through all detectors and return analyzed results.
        
        Args:
            gaps: Raw gap findings from scanner
        
        Returns:
            (analyzed_gaps: List[GapAnalysis], stats: Dict)
        """
        analyzed_gaps = []
        self.stats['total_gaps'] = len(gaps)
        
        for gap in gaps:
            analysis = self._analyze_single_gap(gap)
            analyzed_gaps.append(analysis)
            
            if analysis.is_false_positive:
                self.stats['gaps_filtered'] += 1
        
        return analyzed_gaps, self.stats
    
    def _analyze_single_gap(self, gap: Dict[str, Any]) -> GapAnalysis:
        """Analyze a single gap through all detectors"""
        file_path = gap.get('file', '')
        line_num = gap.get('line', 0)
        severity = gap.get('severity', 'UNKNOWN')
        confidence = gap.get('confidence', 0.7)
        
        # Read context
        lines, start_line = self.context.read_context(file_path, line_num)
        
        # Run all detectors
        is_false_positive = False
        classification = GapClassification.REAL_GAP
        confidence_adjustment = 0.0
        reasons = []
        evidence = []
        
        for detector in self.detectors:
            is_fp, reason, adj = detector.detect(gap, lines, start_line)
            
            if is_fp:
                self.stats['detector_hits'][detector.name] += 1
                is_false_positive = True
                confidence_adjustment += adj
                reasons.append(reason)
                
                # Determine classification
                if detector.name == "TestCode":
                    classification = GapClassification.TEST_MOCK
                elif detector.name == "Placeholder":
                    classification = GapClassification.PLACEHOLDER
                elif detector.name == "GuardedStub":
                    classification = GapClassification.GUARDED_STUB
                else:
                    classification = GapClassification.FALSE_POSITIVE
        
        # Adjust confidence
        adjusted_confidence = max(0.0, min(1.0, confidence + confidence_adjustment))
        self.stats['confidence_adjustments'].append({
            'file': file_path,
            'line': line_num,
            'original': confidence,
            'adjusted': adjusted_confidence,
            'delta': confidence_adjustment
        })
        
        # Determine verified severity
        verified_severity = self._downgrade_severity(severity, classification, adjusted_confidence)
        
        return GapAnalysis(
            gap=gap,
            classification=classification,
            original_severity=severity,
            verified_severity=verified_severity,
            confidence_score=adjusted_confidence,
            is_false_positive=is_false_positive,
            reason=' | '.join(reasons) if reasons else 'No false positives detected',
            evidence=evidence
        )
    
    def _downgrade_severity(self, original: str, classification: GapClassification, confidence: float) -> str:
        """
        Determine verified severity based on classification and confidence.
        """
        if classification == GapClassification.FALSE_POSITIVE:
            return "IGNORE"
        
        if classification == GapClassification.TEST_MOCK:
            return "INFO"
        
        if classification == GapClassification.PLACEHOLDER:
            return "MEDIUM"
        
        if classification == GapClassification.GUARDED_STUB:
            return "HIGH"
        
        # For real gaps, adjust by confidence
        if confidence < 0.5:
            return "MEDIUM"
        elif confidence < 0.7:
            return original if original != "CRITICAL" else "HIGH"
        
        return original
    
    def export_analysis(self, analyses: List[GapAnalysis], output_file: str) -> None:
        """Export analyzed gaps to JSON"""
        output_data = {
            'timestamp': str(Path(__file__).stat().st_mtime),
            'summary': {
                'total': len(analyses),
                'false_positives': sum(1 for a in analyses if a.is_false_positive),
                'real_gaps': sum(1 for a in analyses if not a.is_false_positive),
                'confidence_improvement': sum(
                    a.confidence_score - a.gap.get('confidence', 0.7)
                    for a in analyses
                ) / len(analyses) if analyses else 0.0
            },
            'classifications': {
                'REAL_GAP': sum(1 for a in analyses if a.classification == GapClassification.REAL_GAP),
                'GUARDED_STUB': sum(1 for a in analyses if a.classification == GapClassification.GUARDED_STUB),
                'TEST_MOCK': sum(1 for a in analyses if a.classification == GapClassification.TEST_MOCK),
                'FALSE_POSITIVE': sum(1 for a in analyses if a.classification == GapClassification.FALSE_POSITIVE),
                'PLACEHOLDER': sum(1 for a in analyses if a.classification == GapClassification.PLACEHOLDER),
            },
            'detector_stats': self.stats['detector_hits'],
            'analyses': [
                {
                    **a.gap,
                    'classification': a.classification.value,
                    'verified_severity': a.verified_severity,
                    'confidence_adjusted': a.confidence_score,
                    'is_false_positive': a.is_false_positive,
                    'reason': a.reason,
                }
                for a in analyses
            ]
        }
        
        with open(output_file, 'w') as f:
            json.dump(output_data, f, indent=2)
        
        logger.info(f"Exported {len(analyses)} analyzed gaps to {output_file}")


def main():
    """Example usage"""
    import sys
    
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    input_file = sys.argv[2] if len(sys.argv) > 2 else 'gap_scan_results.json'
    output_file = sys.argv[3] if len(sys.argv) > 3 else 'gap_scan_fp_analyzed.json'
    
    # Load raw gaps
    try:
        with open(input_file) as f:
            raw_gaps = json.load(f)
        if isinstance(raw_gaps, dict) and 'findings' in raw_gaps:
            raw_gaps = raw_gaps['findings']
    except FileNotFoundError:
        logger.error(f"Input file not found: {input_file}")
        return 1
    
    # Analyze
    engine = FPDetectionEngine(repo_root)
    analyses, stats = engine.filter_gaps(raw_gaps)
    
    # Export
    engine.export_analysis(analyses, output_file)
    
    # Print summary
    print(f"\n{'='*70}")
    print(f"False Positive Detection Complete")
    print(f"{'='*70}")
    print(f"Total gaps analyzed: {stats['total_gaps']}")
    print(f"False positives identified: {stats['gaps_filtered']}")
    print(f"FP reduction: {stats['gaps_filtered']/max(1, stats['total_gaps'])*100:.1f}%")
    print(f"\nDetector hits:")
    for detector, hits in stats['detector_hits'].items():
        print(f"  {detector:30s}: {hits:6d}")
    print(f"\nAnalysis exported to: {output_file}")
    
    return 0


if __name__ == '__main__':
    exit(main())
