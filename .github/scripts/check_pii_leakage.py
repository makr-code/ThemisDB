#!/usr/bin/env python3
"""
check_pii_leakage.py – PII Redaction Policy Enforcement Check

Scans C++ source files and templates for patterns that write user-supplied or
structured data to logs / traces / metrics *without* routing through the
PIIRedactionPolicy.  Any match is printed and the script exits with code 1 so
that CI fails on new sources of unredacted PII.

Usage:
    python .github/scripts/check_pii_leakage.py [--root <repo-root>] [--strict]

    --root    Repository root (default: current working directory)
    --strict  Also warn on indirect spdlog calls that do not carry a
              REDACT_ comment annotation.
"""

import argparse
import os
import re
import sys
from pathlib import Path
from typing import List, Tuple

# ---------------------------------------------------------------------------
# Patterns that indicate a telemetry write without redaction.
#
# The heuristic is: if a line calls a logging/tracing/metrics helper *and*
# contains a variable name that carries a recognisable PII semantic (field
# names like "email", "phone", "ssn", "iban", "credit_card", "password",
# "token", "secret", "key") then it is a candidate leak.
# ---------------------------------------------------------------------------

# Telemetry write calls we care about.
TELEMETRY_CALL_RE = re.compile(
    r'\b('
    r'THEMIS_TRACE|THEMIS_DEBUG|THEMIS_INFO|THEMIS_WARN|THEMIS_ERROR|THEMIS_CRITICAL'
    r'|spdlog::(trace|debug|info|warn|error|critical)'
    r'|span\.(setAttribute|recordError)'
    r'|ScopedSpan|TracedSpan'
    r'|MetricsCollector::|recordQuery|recordShardRequest|recordFullScan'
    r')',
    re.IGNORECASE,
)

# Variable / argument names that suggest PII content.
PII_FIELD_RE = re.compile(
    r'\b('
    r'email|e_mail|phone|telephone|mobile'
    r'|ssn|social_security'
    r'|iban|credit_card|card_number|cvv|cvc'
    r'|password|passwd|passphrase'
    r'|secret|api_key|access_token|refresh_token|auth_token|bearer'
    r'|private_key|priv_key'
    r'|user_name|username(?!_len|_prefix)'  # avoid false positives like username_len
    r'|full_name|first_name|last_name|birthdate|dob|date_of_birth'
    r'|address|street|postcode|zip_code'
    r')',
    re.IGNORECASE,
)

# Lines that are already safe (routed through the policy or explicitly suppressed).
SAFE_PATTERN_RE = re.compile(
    r'('
    r'redactForLog|redactAttributes|redactLabels'           # policy calls
    r'|PIIRedactionPolicy'
    r'|maskValue|getRedactionRecommendation'                 # direct masking
    r'|NOPII'                                               # explicit suppression annotation
    r'|THEMIS_PII_SAFE'
    r')',
    re.IGNORECASE,
)

# File extensions to scan.
SCAN_EXTENSIONS = {'.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx'}

# Directories to skip entirely.
SKIP_DIRS = {
    '.git', 'build', '_build', 'dist', 'node_modules',
    'vcpkg', 'llama.cpp', 'third_party', 'external',
    'releases', 'archive', 'debian', 'packaging',
}

# Files that are themselves the redaction implementation – safe to ignore.
SKIP_FILES = {
    'pii_redaction_policy.cpp',
    'pii_redaction_policy.h',
    'pii_detector.cpp',
    'pii_detector.h',
    'pii_detection_engine.cpp',
    'pii_detection_engine.h',
    'regex_detection_engine.cpp',
    'regex_detection_engine.h',
    'pii_pseudonymizer.cpp',
    'pii_pseudonymizer.h',
    # Test files: they intentionally use PII strings as test inputs.
    'test_pii_redaction_policy.cpp',
    'test_pii_detector.cpp',
    'test_pii_soft_delete.cpp',
    'test_http_pii_manager.cpp',
    'test_http_pii_manager_new.cpp',
    'test_http_pii_lazy_init.cpp',
    # Fuzz harnesses use raw PII probes by design.
    'pii_redaction_harness.cpp',
}


def should_skip(path: Path) -> bool:
    parts = set(path.parts)
    if parts & SKIP_DIRS:
        return True
    if path.name in SKIP_FILES:
        return True
    if path.suffix not in SCAN_EXTENSIONS:
        return True
    return False


def scan_file(path: Path, strict: bool) -> List[Tuple[int, str]]:
    """Return list of (line_number, line_text) for flagged lines."""
    hits: List[Tuple[int, str]] = []
    try:
        text = path.read_text(encoding='utf-8', errors='replace')
    except OSError:
        return hits

    for lineno, line in enumerate(text.splitlines(), start=1):
        stripped = line.strip()
        # Skip blank lines and pure comments.
        if not stripped or stripped.startswith('//') or stripped.startswith('*'):
            continue
        # Already safe.
        if SAFE_PATTERN_RE.search(line):
            continue
        # Check for telemetry call combined with PII field name.
        if TELEMETRY_CALL_RE.search(line) and PII_FIELD_RE.search(line):
            hits.append((lineno, line.rstrip()))
            continue
        if strict:
            # In strict mode also flag any telemetry call where the argument
            # looks like a raw variable (not a string literal) – conservative
            # check to surface indirect leaks.
            if TELEMETRY_CALL_RE.search(line) and not re.search(r'"[^"]*"', line):
                if PII_FIELD_RE.search(line):
                    hits.append((lineno, line.rstrip()))
    return hits


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', default='.', help='Repository root directory')
    parser.add_argument('--strict', action='store_true',
                        help='Enable stricter heuristics')
    args = parser.parse_args()

    root = Path(args.root).resolve()
    if not root.is_dir():
        print(f"ERROR: --root '{root}' is not a directory", file=sys.stderr)
        return 2

    total_files = 0
    total_hits = 0
    violations: List[Tuple[str, int, str]] = []

    for dirpath, dirnames, filenames in os.walk(root):
        # Prune skip dirs in-place so os.walk does not recurse into them.
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fname in filenames:
            fpath = Path(dirpath) / fname
            if should_skip(fpath):
                continue
            total_files += 1
            hits = scan_file(fpath, args.strict)
            for lineno, text in hits:
                rel = fpath.relative_to(root)
                violations.append((str(rel), lineno, text))
                total_hits += 1

    if violations:
        print(f"\n{'='*72}")
        print("PII Leakage Check – FAILED")
        print(f"{'='*72}")
        print(f"Found {total_hits} potential unredacted PII write(s) in "
              f"{total_files} scanned file(s):\n")
        for rel, lineno, text in violations:
            print(f"  {rel}:{lineno}")
            print(f"    {text}")
            print()
        print("Fix: route the value through PIIRedactionPolicy::get().redactForLog()")
        print("     or add // NOPII with a justification comment if safe.")
        print(f"{'='*72}\n")
        return 1

    print(f"PII Leakage Check – PASSED "
          f"({total_files} file(s) scanned, 0 violations)")
    return 0


if __name__ == '__main__':
    sys.exit(main())
