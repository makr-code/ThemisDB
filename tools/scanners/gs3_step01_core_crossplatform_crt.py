#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Cross-Platform CRT Portability

Detects Windows-only C Runtime functions that are called WITHOUT a
``#ifdef _WIN32`` / ``#ifdef _MSC_VER`` guard.  These calls compile
cleanly on MSVC but fail on GCC/Clang (Linux, macOS), producing chronic
build errors like the ones tracked in issue #6275.

Categories detected
-------------------
WINDOWS_SECURE_CRT
    Microsoft Secure CRT variants: ``sscanf_s``, ``sprintf_s``,
    ``fopen_s``, ``strcpy_s``, ``memcpy_s``, etc.  Standard
    alternatives exist on all platforms.

WINDOWS_TIME_CRT
    ``_mkgmtime`` / ``_mkgmtime64``: UTC tm→time_t conversion.
    POSIX equivalent: ``timegm``.  ``gmtime_s`` / ``localtime_s``
    have reversed argument order from the POSIX ``*_r`` variants.

WINDOWS_STRING_CRT
    ``_strdup``, ``_stricmp``, ``_strnicmp``, ``_strlwr``, ``_strupr``
    and wide-char equivalents.  All have portable alternatives.

WINDOWS_IO_CRT
    ``_getcwd``, ``_chdir``, ``_mkdir``, ``_putenv_s``.
    POSIX: ``getcwd``, ``chdir``, ``mkdir``, ``setenv``.

Remediation
-----------
Wrap platform-specific calls in ``#ifdef _WIN32 / #else / #endif``
blocks, or extract them into a portable helper function.

False-positive reduction
------------------------
* Lines that are pure comments are skipped.
* The preprocessor-guard context is tracked with a stack so calls
  already guarded by ``#ifdef _WIN32`` are suppressed.
* Test files are excluded by default.
* Macro *definitions* that merely declare the symbol are skipped.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import List

sys.path.insert(0, str(Path(__file__).parent.parent))

from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority

# ---------------------------------------------------------------------------
# Windows-only CRT function catalogue
# ---------------------------------------------------------------------------

# Each entry: (function_name_pattern, gap_subtype, remediation_hint)
_WIN_CRT_CATALOGUE: list[tuple[re.Pattern[str], str, str]] = []

def _reg(pattern: str, subtype: str, remediation: str) -> None:
    _WIN_CRT_CATALOGUE.append((re.compile(pattern), subtype, remediation))


# ── Secure CRT scanf/printf family ─────────────────────────────────────────
_reg(r"\b(?:std::)?(?:::)?sscanf_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace sscanf_s with std::sscanf (no buffer-size arg needed for %%d/%%f/%%i)")
_reg(r"\b(?:std::)?(?:::)?scanf_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace scanf_s with std::scanf")
_reg(r"\b(?:std::)?(?:::)?fscanf_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace fscanf_s with std::fscanf")
_reg(r"\b(?:std::)?(?:::)?vscanf_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace vscanf_s with std::vscanf")
_reg(r"\b(?:::)?sprintf_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace sprintf_s with std::snprintf")
_reg(r"\b(?:::)?vsprintf_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace vsprintf_s with std::vsnprintf")
_reg(r"\b(?:::)?printf_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace printf_s with std::printf")
_reg(r"\b(?:::)?fprintf_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace fprintf_s with std::fprintf")
_reg(r"\b(?:::)?vprintf_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace vprintf_s with std::vprintf")

# ── Secure CRT string family ────────────────────────────────────────────────
_reg(r"\b(?:::)?strcpy_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace strcpy_s with strlcpy or use std::string")
_reg(r"\b(?:::)?strncpy_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace strncpy_s with std::strncpy or std::string")
_reg(r"\b(?:::)?strcat_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace strcat_s with strlcat or use std::string")
_reg(r"\b(?:::)?strncat_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace strncat_s with std::strncat or std::string")
_reg(r"\b(?:::)?strtok_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace strtok_s with POSIX strtok_r or std::regex")
_reg(r"\b(?:::)?gets_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace gets_s with std::fgets")

# ── Secure CRT memory family ────────────────────────────────────────────────
_reg(r"\b(?:::)?memcpy_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace memcpy_s with std::memcpy (validate sizes explicitly)")
_reg(r"\b(?:::)?memmove_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace memmove_s with std::memmove")

# ── Secure CRT file family ──────────────────────────────────────────────────
_reg(r"\b(?:::)?fopen_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace fopen_s with std::fopen")
_reg(r"\b(?:::)?freopen_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace freopen_s with std::freopen")
_reg(r"\b(?:::)?tmpnam_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace tmpnam_s with std::tmpnam or mkstemp (POSIX)")
_reg(r"\b(?:::)?tmpfile_s\s*\(", "WINDOWS_SECURE_CRT",
     "Replace tmpfile_s with std::tmpfile")

# ── Windows time functions ──────────────────────────────────────────────────
_reg(r"\b_mkgmtime(?:64)?\s*\(", "WINDOWS_TIME_CRT",
     "Replace _mkgmtime with timegm (POSIX) in an #ifdef _WIN32 / #else block")
_reg(r"\b(?:::)?gmtime_s\s*\(", "WINDOWS_TIME_CRT",
     "Replace gmtime_s with gmtime_r (POSIX); note reversed argument order")
_reg(r"\b(?:::)?localtime_s\s*\(", "WINDOWS_TIME_CRT",
     "Replace localtime_s with localtime_r (POSIX); note reversed argument order")

# ── Windows string utilities ────────────────────────────────────────────────
_reg(r"\b_strdup\s*\(", "WINDOWS_STRING_CRT",
     "Replace _strdup with POSIX strdup or std::string")
_reg(r"\b_wcsdup\s*\(", "WINDOWS_STRING_CRT",
     "Replace _wcsdup with POSIX wcsdup")
_reg(r"\b_stricmp\s*\(", "WINDOWS_STRING_CRT",
     "Replace _stricmp with strcasecmp (POSIX) in an #ifdef _WIN32 / #else block")
_reg(r"\b_wcsicmp\s*\(", "WINDOWS_STRING_CRT",
     "Replace _wcsicmp with wcscasecmp or a portable wrapper")
_reg(r"\b_strnicmp\s*\(", "WINDOWS_STRING_CRT",
     "Replace _strnicmp with strncasecmp (POSIX) in an #ifdef _WIN32 / #else block")
_reg(r"\b_wcsnicmp\s*\(", "WINDOWS_STRING_CRT",
     "Replace _wcsnicmp with wcsncasecmp or a portable wrapper")
_reg(r"\b_strlwr_s?\s*\(", "WINDOWS_STRING_CRT",
     "Replace _strlwr with a portable to_lower() implementation")
_reg(r"\b_strupr_s?\s*\(", "WINDOWS_STRING_CRT",
     "Replace _strupr with a portable to_upper() implementation")

# ── Windows I/O / filesystem CRT ────────────────────────────────────────────
_reg(r"\b_getcwd\s*\(", "WINDOWS_IO_CRT",
     "Replace _getcwd with getcwd (POSIX) in an #ifdef _WIN32 / #else block")
_reg(r"\b_wgetcwd\s*\(", "WINDOWS_IO_CRT",
     "Replace _wgetcwd with a portable std::filesystem::current_path()")
_reg(r"\b_chdir\s*\(", "WINDOWS_IO_CRT",
     "Replace _chdir with chdir (POSIX) in an #ifdef _WIN32 / #else block")
_reg(r"\b_mkdir\s*\(", "WINDOWS_IO_CRT",
     "Replace _mkdir with mkdir (POSIX) or std::filesystem::create_directory")
_reg(r"\b_putenv_s\s*\(", "WINDOWS_IO_CRT",
     "Replace _putenv_s with setenv (POSIX) in an #ifdef _WIN32 / #else block")
_reg(r"\b_dupenv_s\s*\(", "WINDOWS_IO_CRT",
     "Replace _dupenv_s with getenv (POSIX) in an #ifdef _WIN32 / #else block")


# ---------------------------------------------------------------------------
# Preprocessor-guard context tracker
# ---------------------------------------------------------------------------

_WIN_GUARD_TOKENS = frozenset({"_WIN32", "_WIN64", "_MSC_VER", "WINAPI", "_WINDOWS_"})

_RE_IF = re.compile(r"^\s*#\s*if(?:def|ndef)?\b(.*)$")
_RE_ELIF = re.compile(r"^\s*#\s*elif\b(.*)$")
_RE_ELSE = re.compile(r"^\s*#\s*else\b")
_RE_ENDIF = re.compile(r"^\s*#\s*endif\b")


def _is_win_guard(condition: str) -> bool:
    """Return True if a preprocessor condition activates Windows-specific code."""
    return any(tok in condition for tok in _WIN_GUARD_TOKENS)


class _IfdefFrame:
    """One level of #if / #ifdef nesting."""

    __slots__ = ("win_branch_active",)

    def __init__(self, win_branch_active: bool) -> None:
        # True  → current branch is Windows-specific (safe to use Win32 APIs)
        # False → current branch is NOT Windows-specific (Win32 APIs would break)
        self.win_branch_active = win_branch_active


def _build_guard_context(lines: list[str]) -> list[bool]:
    """
    Return a per-line boolean list where True means the line is inside a
    Windows-specific preprocessor block (``#ifdef _WIN32`` or equivalent).
    """
    stack: list[_IfdefFrame] = []
    result: list[bool] = []

    for line in lines:
        stripped = line.strip()

        m_if = _RE_IF.match(stripped)
        m_elif = _RE_ELIF.match(stripped)
        m_else = _RE_ELSE.match(stripped)
        m_endif = _RE_ENDIF.match(stripped)

        if m_if:
            condition = m_if.group(1)
            # #ifndef _WIN32 means this branch is *not* Windows-specific
            is_ifndef = bool(re.match(r"^\s*#\s*ifndef\b", stripped))
            win_active = _is_win_guard(condition) and not is_ifndef
            stack.append(_IfdefFrame(win_active))

        elif m_elif and stack:
            condition = m_elif.group(1)
            stack[-1].win_branch_active = _is_win_guard(condition)

        elif m_else and stack:
            # Flip: if we were in a Windows branch, now we're in the POSIX branch
            stack[-1].win_branch_active = not stack[-1].win_branch_active

        elif m_endif and stack:
            stack.pop()

        # A line is "Windows-guarded" when ANY active frame is Windows-specific
        win_guarded = any(f.win_branch_active for f in stack)
        result.append(win_guarded)

    return result


# ---------------------------------------------------------------------------
# Scanner implementation
# ---------------------------------------------------------------------------

class CrossPlatformCRTScanner(BaseGapScanner):
    """
    Detect Windows-only CRT function calls that are not guarded by
    ``#ifdef _WIN32``.  Such calls compile only with MSVC and cause
    ``error: '...' was not declared in this scope`` on GCC/Clang.
    """

    PRIORITY = ScannerPriority.BASELINE
    ENABLED = True
    MAX_RUNTIME_SECONDS = 30

    # Extensions to scan
    _CPP_EXTENSIONS = frozenset({".cpp", ".cc", ".cxx", ".c", ".hpp", ".h", ".hh", ".hxx"})

    # Directories to exclude from production-code scanning
    _SKIP_DIRS = frozenset({
        "tests", "test", "benchmarks", "bench", "third_party", "thirdparty",
        "external", "vendor", "generated", "build", ".git",
    })

    def __init__(self) -> None:
        super().__init__("CrossPlatformCRTScanner", "1.0")

    # ------------------------------------------------------------------
    # BaseGapScanner interface
    # ------------------------------------------------------------------

    def scan(self, source_dir: str) -> List[Gap]:
        gaps: List[Gap] = []
        self.source_path = Path(source_dir).resolve()

        for file_path in self._collect_files(self.source_path):
            self.files_scanned += 1
            gaps.extend(self._scan_file(file_path))

        return self.deduplicate(gaps)

    # ------------------------------------------------------------------
    # File collection
    # ------------------------------------------------------------------

    def _collect_files(self, root: Path):
        for path in root.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix.lower() not in self._CPP_EXTENSIONS:
                continue
            # Skip excluded directories
            parts = {p.lower() for p in path.parts}
            if parts & self._SKIP_DIRS:
                continue
            yield path

    # ------------------------------------------------------------------
    # Per-file analysis
    # ------------------------------------------------------------------

    def _scan_file(self, file_path: Path) -> List[Gap]:
        gaps: List[Gap] = []

        try:
            lines = file_path.read_text(encoding="utf-8", errors="ignore").splitlines()
        except OSError:
            return gaps

        if not lines:
            return gaps

        guard_context = _build_guard_context(lines)
        rel_path = self._relative(file_path)

        for idx, line in enumerate(lines):
            line_no = idx + 1

            # Skip if already inside a Windows-specific guard
            if guard_context[idx]:
                continue

            stripped = line.strip()

            # Skip blank lines and pure comments
            if not stripped or stripped.startswith("//") or stripped.startswith("*") or stripped.startswith("/*"):
                continue

            # Skip preprocessor directives themselves (not executable code)
            if stripped.startswith("#"):
                continue

            # Inline comment stripping — only keep code portion
            code_part = re.sub(r"//.*$", "", line)

            # Check each catalogue entry
            for pattern, subtype, remediation in _WIN_CRT_CATALOGUE:
                if not pattern.search(code_part):
                    continue

                # Extra FP filter: skip if this is a *declaration* or *define*
                # (e.g. a forward declaration in a compatibility header)
                if re.search(r"\b(?:typedef|#define|extern\s+\"C\"|WINAPI)\b", code_part):
                    continue

                gaps.append(Gap(
                    file=rel_path,
                    line=line_no,
                    type=f"crossplatform_crt/{subtype.lower()}",
                    severity="HIGH",
                    confidence=0.90,
                    description=(
                        f"Windows-only CRT call without #ifdef _WIN32 guard "
                        f"({pattern.pattern.split('(')[0].strip()!r}) — "
                        f"fails to compile on Linux/macOS with GCC/Clang"
                    ),
                    remediation=remediation,
                    context=stripped[:120],
                    scanner=self.name,
                    step=1,
                    subsystem=self._module_from_path(file_path),
                ))
                # One gap per line per pattern (avoid duplicate reports for
                # the same line if both sscanf_s and another variant match)
                break

        return gaps

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _relative(self, file_path: Path) -> str:
        try:
            return str(file_path.relative_to(self.source_path.parents[0]))
        except ValueError:
            return str(file_path)

    @staticmethod
    def _module_from_path(file_path: Path) -> str:
        parts = file_path.parts
        for i, part in enumerate(parts):
            if part in {"src", "include"} and i + 1 < len(parts):
                return parts[i + 1]
        return "unknown"
