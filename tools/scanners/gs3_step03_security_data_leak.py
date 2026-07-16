#!/usr/bin/env python3
"""
Phase 6 - Data Leak Detection (IMPROVED)

Improvements:
1. Exclude tests/mocks/fixtures/examples/benchmarks to suppress synthetic data noise.
2. Keep high-confidence detections (hardcoded secret formats, PII + semantic labels, sensitive logging).
3. Reduce numeric false positives by requiring semantic context around PII patterns.
"""

import json
import re
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Dict, List


class DataLeakType(Enum):
    HARDCODED_PII = 'hardcoded_pii'
    HARDCODED_SECRET = 'hardcoded_secret'
    SENSITIVE_LOGGING = 'sensitive_logging'


@dataclass
class DataLeakGap:
    file_path: str
    line_num: int
    gap_type: DataLeakType
    snippet: str
    severity: str
    description: str
    remediation: str
    confidence: float

    def to_dict(self):
        return {
            'file': self.file_path,
            'line': self.line_num,
            'type': self.gap_type.value,
            'severity': self.severity,
            'snippet': self.snippet,
            'description': self.description,
            'remediation': self.remediation,
            'confidence': self.confidence,
        }


class DataLeakScannerImproved:
    """Improved data leak scanner with production-context filters."""

    PII_PATTERNS = {
        'ssn': re.compile(r'\b\d{3}[-\.]?\d{2}[-\.]?\d{4}\b'),
        'credit_card': re.compile(r'\b\d{4}[-\s]?\d{4}[-\s]?\d{4}[-\s]?\d{4}\b'),
        'email': re.compile(r'\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b'),
    }

    SECRET_PATTERNS = {
        'api_key': re.compile(r'(?:api[_-]?key|apikey)\s*=\s*["\']([a-zA-Z0-9_-]{20,})["\']', re.IGNORECASE),
        'token': re.compile(r'(?:token|access_token|bearer)\s*=\s*["\']([a-zA-Z0-9_.-]{20,})["\']', re.IGNORECASE),
        'password': re.compile(r'(?:password|passwd|pwd)\s*=\s*["\']([^"\']{4,})["\']', re.IGNORECASE),
        'aws_key': re.compile(r'AKIA[0-9A-Z]{16}'),
        'github_token': re.compile(r'ghp_[A-Za-z0-9_]{36}'),
    }

    SENSITIVE_LOG_KEYWORDS = [
        'password', 'passwd', 'pwd', 'secret', 'token', 'key', 'credential',
        'apikey', 'api_key', 'auth', 'bearer', 'session', 'cookie', 'ssn', 'credit_card',
    ]

    TEST_DATA_PATTERNS = [
        r'test.*\d{3}-\d{2}-\d{4}',
        r'1234[-\s]?4567[-\s]?8901[-\s]?2345',
        r'test@test\.com|example@example\.com|demo@demo\.com',
        r'TEST_API_KEY|TEST_TOKEN|TEST_SECRET',
    ]

    NON_PROD_MARKERS = ['tests/', 'test_', '_test.', 'benchmarks/', 'bench_', 'fixtures/', 'examples/', 'demo_', '_mock.']
    PII_CONTEXT_WORDS = ['ssn', 'social', 'credit', 'card', 'email', 'customer', 'user', 'phone', 'contact']

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[DataLeakGap]] = {}
        self.test_patterns = [re.compile(p, re.IGNORECASE) for p in self.TEST_DATA_PATTERNS]

    @staticmethod
    def _norm(path: Path) -> str:
        return str(path).replace('\\', '/').lower()

    def _is_non_prod_file(self, file_path: Path) -> bool:
        p = self._norm(file_path)
        return any(m in p for m in self.NON_PROD_MARKERS)

    def _is_test_data(self, text: str) -> bool:
        return any(p.search(text) for p in self.test_patterns)

    @staticmethod
    def _is_comment(line: str) -> bool:
        s = line.strip()
        return s.startswith('//') or s.startswith('*') or s.startswith('/*')

    def _has_pii_context(self, line: str, context: str) -> bool:
        joined = (line + '\n' + context).lower()
        return any(w in joined for w in self.PII_CONTEXT_WORDS)

    def _check_pii(self, file_path: Path, lines: List[str]) -> List[DataLeakGap]:
        gaps: List[DataLeakGap] = []
        for idx, line in enumerate(lines, 1):
            if self._is_comment(line):
                continue

            context = ''.join(lines[max(0, idx - 2):min(len(lines), idx + 3)])
            for p_type, pattern in self.PII_PATTERNS.items():
                m = pattern.search(line)
                if not m:
                    continue
                if self._is_test_data(m.group(0)):
                    continue
                if not self._has_pii_context(line, context):
                    continue

                severity = 'CRITICAL' if p_type in {'ssn', 'credit_card'} else 'HIGH'
                gaps.append(DataLeakGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=idx,
                    gap_type=DataLeakType.HARDCODED_PII,
                    snippet=line.strip()[:100],
                    severity=severity,
                    description=f'Potential hardcoded {p_type} in source',
                    remediation='Remove literal PII and fetch from secured storage path',
                    confidence=0.85 if p_type == 'email' else 0.92,
                ))

        return gaps

    def _check_secrets(self, file_path: Path, lines: List[str]) -> List[DataLeakGap]:
        gaps: List[DataLeakGap] = []
        for idx, line in enumerate(lines, 1):
            if self._is_comment(line):
                continue

            for secret_type, pattern in self.SECRET_PATTERNS.items():
                m = pattern.search(line)
                if not m:
                    continue
                val = m.group(1) if m.lastindex else m.group(0)
                if self._is_test_data(val):
                    continue

                gaps.append(DataLeakGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=idx,
                    gap_type=DataLeakType.HARDCODED_SECRET,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description=f'Potential hardcoded {secret_type} in source',
                    remediation='Use secure secret manager / environment injection',
                    confidence=0.97,
                ))

        return gaps

    def _check_sensitive_logging(self, file_path: Path, lines: List[str]) -> List[DataLeakGap]:
        gaps: List[DataLeakGap] = []
        for idx, line in enumerate(lines, 1):
            if self._is_comment(line):
                continue

            lower = line.lower()
            if not any(k in lower for k in ['log', 'logger', 'printf', 'cout', 'cerr', 'spdlog']):
                continue
            if not any(k in lower for k in self.SENSITIVE_LOG_KEYWORDS):
                continue

            gaps.append(DataLeakGap(
                file_path=str(file_path.relative_to(self.repo_root)),
                line_num=idx,
                gap_type=DataLeakType.SENSITIVE_LOGGING,
                snippet=line.strip()[:100],
                severity='HIGH',
                description='Sensitive token/value appears in logging statement',
                remediation='Redact/hash/mask sensitive value before logging',
                confidence=0.83,
            ))

        return gaps

    def scan_file(self, file_path: Path) -> List[DataLeakGap]:
        if self._is_non_prod_file(file_path):
            return []

        try:
            lines = file_path.read_text(encoding='utf-8', errors='ignore').splitlines()
        except Exception:
            return []

        gaps: List[DataLeakGap] = []
        gaps.extend(self._check_pii(file_path, lines))
        gaps.extend(self._check_secrets(file_path, lines))
        gaps.extend(self._check_sensitive_logging(file_path, lines))
        return gaps

    def scan_repository(self) -> Dict[str, List[DataLeakGap]]:
        gaps_by_file: Dict[str, List[DataLeakGap]] = {}

        for ext in ('*.cpp', '*.cc', '*.cxx'):
            for src_file in (self.repo_root / 'src').rglob(ext):
                gaps = self.scan_file(src_file)
                if gaps:
                    gaps_by_file[str(src_file)] = gaps

        for ext in ('*.h', '*.hpp', '*.hh', '*.hxx'):
            for hdr_file in (self.repo_root / 'include').rglob(ext):
                gaps = self.scan_file(hdr_file)
                if gaps:
                    gaps_by_file[str(hdr_file)] = gaps

        self.gaps = gaps_by_file
        return gaps_by_file

    def to_json(self) -> str:
        data = {file: [g.to_dict() for g in entries] for file, entries in self.gaps.items()}
        return json.dumps(data, indent=2)
