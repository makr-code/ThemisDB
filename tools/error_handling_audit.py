"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            error_handling_audit.py                            ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:54:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     671                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • dc186e7716  2026-03-11  fix: move from __future__ import annotations to top of to... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 67305c3063  2026-02-23  feat: add repository-wide error-handling audit tool and C... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

#!/usr/bin/env python3
"""
ThemisDB Error-Handling Audit Tool
====================================

Scans source files for violations of the error-handling rules defined in
``docs/error_handling/checklist.md`` and exits with a non-zero status code
when violations are found.

Usage
-----
    python3 tools/error_handling_audit.py [OPTIONS] [PATH ...]

    PATH   One or more root directories to scan (default: src/ include/ apps/ tools/)

Options
-------
    --ignore-file FILE   Path to ignore-list file (default: tools/error_handling_audit.ignore)
    --format {text,json} Output format (default: text)
    --no-color           Disable ANSI colour output
    -q, --quiet          Suppress per-violation detail; only print summary

Exit codes
----------
    0  No violations found
    1  One or more violations found
    2  Internal error / bad arguments
"""

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Iterator, List, Optional, Sequence, Tuple

# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------

@dataclass
class Violation:
    rule: str
    file: str
    line: int
    column: int
    text: str
    message: str


@dataclass
class RuleResult:
    rule_id: str
    violations: List[Violation] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Ignore-list helpers
# ---------------------------------------------------------------------------

def load_ignore_patterns(ignore_file: str) -> List[str]:
    """Return a list of path-prefix patterns to skip (lines starting with #
    and empty lines are ignored)."""
    patterns: List[str] = []
    path = Path(ignore_file)
    if not path.exists():
        return patterns
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if stripped and not stripped.startswith("#"):
            patterns.append(stripped)
    return patterns


def is_ignored(file_path: str, patterns: List[str]) -> bool:
    """Return True if *file_path* matches any ignore pattern."""
    normalized = file_path.replace("\\", "/")
    for pat in patterns:
        pat_norm = pat.replace("\\", "/")
        if normalized == pat_norm or normalized.startswith(pat_norm.rstrip("/") + "/"):
            return True
        # simple glob: pattern ending in * (prefix match already handled above)
        if pat_norm.endswith("*") and normalized.startswith(pat_norm[:-1]):
            return True
    return False


# ---------------------------------------------------------------------------
# File discovery
# ---------------------------------------------------------------------------

CPP_EXTENSIONS = {".cpp", ".cc", ".cxx", ".h", ".hpp"}
PY_EXTENSIONS = {".py"}
CS_EXTENSIONS = {".cs"}
PHP_EXTENSIONS = {".php"}
PS1_EXTENSIONS = {".ps1"}

ALL_EXTENSIONS = CPP_EXTENSIONS | PY_EXTENSIONS | CS_EXTENSIONS | PHP_EXTENSIONS | PS1_EXTENSIONS


def iter_source_files(roots: List[str], ignore_patterns: List[str]) -> Iterator[Path]:
    """Yield all source files under *roots* that are not ignored."""
    for root in roots:
        root_path = Path(root)
        if not root_path.exists():
            continue
        if root_path.is_file():
            p = str(root_path)
            if not is_ignored(p, ignore_patterns) and root_path.suffix in ALL_EXTENSIONS:
                yield root_path
            continue
        for path in sorted(root_path.rglob("*")):
            if path.is_file() and path.suffix in ALL_EXTENSIONS:
                rel = str(path)
                if not is_ignored(rel, ignore_patterns):
                    yield path


# ---------------------------------------------------------------------------
# Rule helpers
# ---------------------------------------------------------------------------

def lines_of(path: Path) -> List[str]:
    try:
        return path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return []


def scan_with_regex(
    path: Path,
    lines: List[str],
    rule_id: str,
    pattern: re.Pattern,
    message_fn: Callable[[re.Match, str], str],
    *,
    skip_line_fn: Optional[Callable[[str], bool]] = None,
) -> List[Violation]:
    violations: List[Violation] = []
    for lineno, text in enumerate(lines, start=1):
        if skip_line_fn and skip_line_fn(text):
            continue
        for m in pattern.finditer(text):
            violations.append(
                Violation(
                    rule=rule_id,
                    file=str(path),
                    line=lineno,
                    column=m.start() + 1,
                    text=text.rstrip(),
                    message=message_fn(m, text),
                )
            )
    return violations


# ---------------------------------------------------------------------------
# C++ rules
# ---------------------------------------------------------------------------

# RULE-CPP-001: return nullptr without Result<T>
# We look for "return nullptr;" in .cpp/.h files.
# Exempt patterns: constructors (hard to detect syntactically – we use a
# heuristic: function name ends in OrNull/OrNullptr/MaybeNull), test files.
_CPP_RETURN_NULLPTR = re.compile(r"\breturn\s+nullptr\s*;")

# Lines that are inside a comment
_CPP_LINE_COMMENT = re.compile(r"^\s*//")

# Functions that are explicitly allowed to return nullptr
_CPP_NULLABLE_NAME = re.compile(
    r"\b\w+(?:OrNull|OrNullptr|MaybeNull|_or_null|_or_nullptr)\s*\("
)


def _is_cpp_comment_line(text: str) -> bool:
    return bool(_CPP_LINE_COMMENT.match(text))


def check_cpp_001(path: Path, lines: List[str]) -> List[Violation]:
    """RULE-CPP-001 – no bare return nullptr in C++ files."""
    violations: List[Violation] = []
    for lineno, text in enumerate(lines, start=1):
        stripped = text.strip()
        if stripped.startswith("//") or stripped.startswith("*"):
            continue
        if not _CPP_RETURN_NULLPTR.search(text):
            continue
        # Check surrounding context for nullable function names (look back up to
        # 10 lines for function signature)
        context_start = max(0, lineno - 10)
        context = "\n".join(lines[context_start:lineno])
        if _CPP_NULLABLE_NAME.search(context):
            continue
        violations.append(
            Violation(
                rule="RULE-CPP-001",
                file=str(path),
                line=lineno,
                column=text.index("return") + 1,
                text=text.rstrip(),
                message="return nullptr detected – use Result<T*> with Err(ErrorCode, context) instead",
            )
        )
    return violations


# RULE-CPP-002: catch (...) without logging
_CPP_CATCH_ALL = re.compile(r"\bcatch\s*\(\s*\.\.\.\s*\)")
_LOGGING_CALL = re.compile(
    r"\b(THEMIS_ERROR|THEMIS_WARN|THEMIS_INFO|THEMIS_DEBUG"
    r"|spdlog::|LOG_|logger\.|log\.|std::cerr)\b"
)


def _extract_block_body(lines: List[str], start_line: int) -> str:
    """Return the text of the brace-delimited block beginning at or after
    *start_line* (0-based index).  Counts only braces that appear *after* the
    first opening brace we find, so that a leading ``}`` on the same line
    (e.g. ``} catch (...) {``) is not counted.

    The scan is capped at 80 lines from *start_line*, which is sufficient for
    the vast majority of catch/except blocks while keeping scan time bounded.
    Very large catch blocks (>80 lines) are uncommon and would themselves be a
    code smell; developers can explicitly ignore such files if needed."""
    body: List[str] = []
    depth = 0
    found_open = False
    for j in range(start_line, min(start_line + 80, len(lines))):
        line = lines[j]
        for ch in line:
            if ch == "{":
                if not found_open:
                    found_open = True
                depth += 1
            elif ch == "}" and found_open:
                depth -= 1
        body.append(line)
        if found_open and depth == 0:
            break
    return "\n".join(body)


def check_cpp_002(path: Path, lines: List[str]) -> List[Violation]:
    """RULE-CPP-002 – catch(...) must contain a logging call."""
    violations: List[Violation] = []
    i = 0
    while i < len(lines):
        text = lines[i]
        if _CPP_CATCH_ALL.search(text) and not text.strip().startswith("//"):
            block_text = _extract_block_body(lines, i)
            if not _LOGGING_CALL.search(block_text):
                violations.append(
                    Violation(
                        rule="RULE-CPP-002",
                        file=str(path),
                        line=i + 1,
                        column=text.index("catch") + 1,
                        text=text.rstrip(),
                        message="catch(...) without logging – add THEMIS_ERROR/WARN or spdlog call",
                    )
                )
        i += 1
    return violations


# RULE-CPP-003: local struct Status definitions
_CPP_STRUCT_STATUS = re.compile(r"\bstruct\s+Status\b")
_CPP_THIRD_PARTY_NS = re.compile(r"\bnamespace\s+(rocksdb|leveldb|grpc|google)\b")


def check_cpp_003(path: Path, lines: List[str]) -> List[Violation]:
    """RULE-CPP-003 – no local struct Status; use Result<T> instead."""
    violations: List[Violation] = []
    # Collect namespaces active in file (rough heuristic)
    full_text = "\n".join(lines)
    if _CPP_THIRD_PARTY_NS.search(full_text):
        return violations  # whole file is or contains third-party namespace
    for lineno, text in enumerate(lines, start=1):
        stripped = text.strip()
        if stripped.startswith("//") or stripped.startswith("*"):
            continue
        if _CPP_STRUCT_STATUS.search(text):
            violations.append(
                Violation(
                    rule="RULE-CPP-003",
                    file=str(path),
                    line=lineno,
                    column=text.index("struct") + 1,
                    text=text.rstrip(),
                    message="local 'struct Status' detected – use Result<T> from include/utils/expected.h",
                )
            )
    return violations


# ---------------------------------------------------------------------------
# Python rules
# ---------------------------------------------------------------------------

# RULE-PY-001: bare except: or except Exception: without re-raise or logging
_PY_BARE_EXCEPT = re.compile(r"^\s*except\s*(Exception\s*)?(as\s+\w+\s*)?:\s*$")
_PY_LOGGING = re.compile(
    r"\b(logger\.|logging\.|log\.|print\s*\(|sys\.stderr|raise\b)"
)


def check_py_001(path: Path, lines: List[str]) -> List[Violation]:
    """RULE-PY-001 – bare except/except Exception without re-raise or logging."""
    violations: List[Violation] = []
    i = 0
    while i < len(lines):
        text = lines[i]
        if _PY_BARE_EXCEPT.match(text):
            # Inspect the body of the except block (indented lines that follow)
            except_indent = len(text) - len(text.lstrip())
            body_lines: List[str] = []
            j = i + 1
            while j < len(lines):
                body = lines[j]
                if body.strip() == "" or body.strip().startswith("#"):
                    j += 1
                    continue
                body_indent = len(body) - len(body.lstrip())
                if body_indent <= except_indent:
                    break
                body_lines.append(body)
                j += 1
            body_text = "\n".join(body_lines)
            if not _PY_LOGGING.search(body_text):
                violations.append(
                    Violation(
                        rule="RULE-PY-001",
                        file=str(path),
                        line=i + 1,
                        column=1,
                        text=text.rstrip(),
                        message="bare except without logging or re-raise – add logging and/or raise",
                    )
                )
        i += 1
    return violations


# ---------------------------------------------------------------------------
# C# rules
# ---------------------------------------------------------------------------

# RULE-CS-001/002: empty or silent catch blocks
_CS_CATCH = re.compile(r"\bcatch\s*(\([^)]*\))?\s*\{")
_CS_LOGGING = re.compile(
    r"\b(logger\.|Logger\.|log\.|Log\.|Console\.|Debug\.|Trace\.|throw\b)"
)


def check_cs_001(path: Path, lines: List[str]) -> List[Violation]:
    """RULE-CS-001/002 – empty or silent C# catch blocks."""
    violations: List[Violation] = []
    i = 0
    while i < len(lines):
        text = lines[i]
        stripped = text.strip()
        if stripped.startswith("//"):
            i += 1
            continue
        if _CS_CATCH.search(text):
            body_text = _extract_block_body(lines, i)
            if not _CS_LOGGING.search(body_text):
                violations.append(
                    Violation(
                        rule="RULE-CS-001",
                        file=str(path),
                        line=i + 1,
                        column=text.index("catch") + 1,
                        text=text.rstrip(),
                        message="empty or silent catch block – add logging and/or rethrow",
                    )
                )
        i += 1
    return violations


# ---------------------------------------------------------------------------
# PHP rules
# ---------------------------------------------------------------------------

_PHP_CATCH = re.compile(r"\bcatch\s*\(")
_PHP_LOGGING = re.compile(
    r"\b(error_log|throw\b|\$_|logger\.|Logger\.|syslog|trigger_error)\b"
)


def check_php_001(path: Path, lines: List[str]) -> List[Violation]:
    """RULE-PHP-001 – empty or silent PHP catch blocks."""
    violations: List[Violation] = []
    i = 0
    while i < len(lines):
        text = lines[i]
        if text.strip().startswith("//") or text.strip().startswith("#"):
            i += 1
            continue
        if _PHP_CATCH.search(text):
            body_text = _extract_block_body(lines, i)
            if not _PHP_LOGGING.search(body_text):
                violations.append(
                    Violation(
                        rule="RULE-PHP-001",
                        file=str(path),
                        line=i + 1,
                        column=text.index("catch") + 1,
                        text=text.rstrip(),
                        message="empty or silent catch block – add error_log/throw",
                    )
                )
        i += 1
    return violations


# ---------------------------------------------------------------------------
# PowerShell rules
# ---------------------------------------------------------------------------

_PS1_CATCH = re.compile(r"\bcatch\s*(\{|$)")
_PS1_LOGGING = re.compile(
    r"\b(Write-Error|Write-Warning|throw\b|\$_|Write-Host|Out-File)\b",
    re.IGNORECASE,
)


def check_ps1_001(path: Path, lines: List[str]) -> List[Violation]:
    """RULE-PS1-001 – empty or silent PowerShell catch blocks."""
    violations: List[Violation] = []
    i = 0
    while i < len(lines):
        text = lines[i]
        if text.strip().startswith("#"):
            i += 1
            continue
        if _PS1_CATCH.search(text):
            body_text = _extract_block_body(lines, i)
            if not _PS1_LOGGING.search(body_text):
                violations.append(
                    Violation(
                        rule="RULE-PS1-001",
                        file=str(path),
                        line=i + 1,
                        column=1,
                        text=text.rstrip(),
                        message="empty or silent catch block – add Write-Error/throw",
                    )
                )
        i += 1
    return violations


# ---------------------------------------------------------------------------
# Dispatcher
# ---------------------------------------------------------------------------

def audit_file(path: Path) -> List[Violation]:
    """Run applicable rules on *path* and return all violations."""
    ext = path.suffix.lower()
    lines = lines_of(path)
    violations: List[Violation] = []

    if ext in CPP_EXTENSIONS:
        violations += check_cpp_001(path, lines)
        violations += check_cpp_002(path, lines)
        violations += check_cpp_003(path, lines)
    elif ext in PY_EXTENSIONS:
        violations += check_py_001(path, lines)
    elif ext in CS_EXTENSIONS:
        violations += check_cs_001(path, lines)
    elif ext in PHP_EXTENSIONS:
        violations += check_php_001(path, lines)
    elif ext in PS1_EXTENSIONS:
        violations += check_ps1_001(path, lines)

    return violations


# ---------------------------------------------------------------------------
# Output formatting
# ---------------------------------------------------------------------------

ANSI_RED = "\033[31m"
ANSI_YELLOW = "\033[33m"
ANSI_CYAN = "\033[36m"
ANSI_RESET = "\033[0m"
ANSI_BOLD = "\033[1m"


def format_text(
    all_violations: List[Violation],
    *,
    color: bool = True,
    quiet: bool = False,
) -> str:
    lines: List[str] = []
    grouped: dict[str, List[Violation]] = {}
    for v in all_violations:
        grouped.setdefault(v.file, []).append(v)

    for filepath, file_violations in sorted(grouped.items()):
        if not quiet:
            header = f"{ANSI_BOLD}{filepath}{ANSI_RESET}" if color else filepath
            lines.append(header)
            for v in file_violations:
                rule_col = f"{ANSI_CYAN}{v.rule}{ANSI_RESET}" if color else v.rule
                loc = f"{ANSI_YELLOW}{v.line}:{v.column}{ANSI_RESET}" if color else f"{v.line}:{v.column}"
                msg = f"{ANSI_RED}{v.message}{ANSI_RESET}" if color else v.message
                lines.append(f"  [{rule_col}] {loc}  {msg}")
                lines.append(f"    {v.text}")
            lines.append("")

    total = len(all_violations)
    files = len(grouped)
    summary_text = f"Found {total} violation(s) in {files} file(s)."
    if color:
        summary_text = f"{ANSI_BOLD}{summary_text}{ANSI_RESET}"
    lines.append(summary_text)
    return "\n".join(lines)


def format_json(all_violations: List[Violation]) -> str:
    return json.dumps(
        [
            {
                "rule": v.rule,
                "file": v.file,
                "line": v.line,
                "column": v.column,
                "message": v.message,
                "text": v.text,
            }
            for v in all_violations
        ],
        indent=2,
    )


# ---------------------------------------------------------------------------
# CLI entry-point
# ---------------------------------------------------------------------------

DEFAULT_SCAN_ROOTS = ["src", "include", "apps", "tools", "scripts", "clients", "plugins"]
DEFAULT_IGNORE_FILE = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "error_handling_audit.ignore"
)


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="ThemisDB error-handling audit tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument(
        "paths",
        nargs="*",
        default=DEFAULT_SCAN_ROOTS,
        metavar="PATH",
        help="Directories or files to scan (default: src include apps tools scripts clients plugins)",
    )
    p.add_argument(
        "--max-violations",
        type=int,
        default=None,
        metavar="N",
        help=(
            "Exit 1 only if violation count exceeds N. "
            "Use to set a baseline budget while migrating an existing codebase. "
            "Exit 0 when violations <= N (violations are still reported). "
            "Omit (default) to exit 1 on any violation."
        ),
    )
    p.add_argument(
        "--ignore-file",
        default=DEFAULT_IGNORE_FILE,
        metavar="FILE",
        help="Path to ignore-list file",
    )
    p.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    p.add_argument(
        "--no-color",
        action="store_true",
        help="Disable ANSI colour output",
    )
    p.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="Suppress per-violation detail; only print summary",
    )
    return p


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    ignore_patterns = load_ignore_patterns(args.ignore_file)

    all_violations: List[Violation] = []
    scanned = 0
    for path in iter_source_files(args.paths, ignore_patterns):
        violations = audit_file(path)
        all_violations.extend(violations)
        scanned += 1

    use_color = not args.no_color and sys.stdout.isatty()

    if args.format == "json":
        print(format_json(all_violations))
    else:
        output = format_text(all_violations, color=use_color, quiet=args.quiet)
        print(output)

    if not args.quiet:
        sys.stderr.write(f"Scanned {scanned} file(s).\n")

    total = len(all_violations)
    if args.max_violations is not None:
        if total > args.max_violations:
            sys.stderr.write(
                f"FAIL: {total} violation(s) found, budget is {args.max_violations}.\n"
            )
            return 1
        if total > 0:
            sys.stderr.write(
                f"WARN: {total} violation(s) found (within budget of {args.max_violations}).\n"
            )
        return 0
    return 1 if all_violations else 0


if __name__ == "__main__":
    sys.exit(main())
