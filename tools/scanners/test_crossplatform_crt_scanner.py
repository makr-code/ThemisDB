#!/usr/bin/env python3
"""Unit tests for the cross-platform CRT portability scanner."""

import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from scanners.gs3_step01_core_crossplatform_crt import CrossPlatformCRTScanner


def _run_scan(code: str, filename: str = "sample.cpp"):
    with tempfile.TemporaryDirectory() as tmpdir:
        root = Path(tmpdir)
        src_dir = root / "src" / "auth"
        src_dir.mkdir(parents=True)
        cpp_file = src_dir / filename
        cpp_file.write_text(code)
        scanner = CrossPlatformCRTScanner()
        return scanner.scan(str(root))


# ---------------------------------------------------------------------------
# True-positive tests — scanner MUST find these
# ---------------------------------------------------------------------------

def test_unguarded_sscanf_s_detected():
    """Unguarded sscanf_s causes 'not declared in this scope' on Linux."""
    gaps = _run_scan("""
#include <cstdio>
void parse(const char* s) {
    int year = 0, month = 0;
    ::sscanf_s(s, "%d-%d", &year, &month);
}
""")
    types = [g.type for g in gaps]
    assert any("crossplatform_crt" in t for t in types), f"Expected crossplatform_crt gap, got: {types}"


def test_unguarded_mkgmtime_detected():
    gaps = _run_scan("""
#include <time.h>
time_t to_utc(struct tm* t) {
    return _mkgmtime(t);
}
""")
    types = [g.type for g in gaps]
    assert any("crossplatform_crt" in t for t in types), f"Expected crossplatform_crt gap, got: {types}"


def test_unguarded_strdup_detected():
    gaps = _run_scan("""
#include <string.h>
char* dup(const char* s) {
    return _strdup(s);
}
""")
    types = [g.type for g in gaps]
    assert any("crossplatform_crt" in t for t in types), f"Expected crossplatform_crt gap, got: {types}"


def test_unguarded_stricmp_detected():
    gaps = _run_scan("""
#include <string.h>
bool equal(const char* a, const char* b) {
    return _stricmp(a, b) == 0;
}
""")
    types = [g.type for g in gaps]
    assert any("crossplatform_crt" in t for t in types), f"Expected crossplatform_crt gap, got: {types}"


def test_unguarded_getcwd_detected():
    gaps = _run_scan("""
#include <direct.h>
void show_dir() {
    char buf[256];
    _getcwd(buf, sizeof(buf));
}
""")
    types = [g.type for g in gaps]
    assert any("crossplatform_crt" in t for t in types), f"Expected crossplatform_crt gap, got: {types}"


def test_unguarded_fopen_s_detected():
    gaps = _run_scan("""
#include <stdio.h>
void open_file() {
    FILE* f = nullptr;
    fopen_s(&f, "x.txt", "r");
}
""")
    types = [g.type for g in gaps]
    assert any("crossplatform_crt" in t for t in types), f"Expected crossplatform_crt gap, got: {types}"


def test_unguarded_strcpy_s_detected():
    gaps = _run_scan("""
#include <string.h>
void copy(char* dst, const char* src) {
    strcpy_s(dst, 64, src);
}
""")
    types = [g.type for g in gaps]
    assert any("crossplatform_crt" in t for t in types), f"Expected crossplatform_crt gap, got: {types}"


# ---------------------------------------------------------------------------
# True-negative tests — scanner MUST NOT flag these
# ---------------------------------------------------------------------------

def test_guarded_sscanf_s_not_flagged():
    """sscanf_s inside #ifdef _WIN32 is intentional and must not be reported."""
    gaps = _run_scan("""
#include <cstdio>
void parse(const char* s) {
    int year = 0;
#ifdef _WIN32
    ::sscanf_s(s, "%d", &year);
#else
    std::sscanf(s, "%d", &year);
#endif
}
""")
    win_gaps = [g for g in gaps if "crossplatform_crt" in g.type]
    assert not win_gaps, f"Guarded sscanf_s should not be flagged: {win_gaps}"


def test_guarded_mkgmtime_not_flagged():
    """_mkgmtime inside #if defined(_WIN32) must not be reported."""
    gaps = _run_scan("""
#include <time.h>
time_t to_utc(struct tm* t) {
#if defined(_WIN32)
    return _mkgmtime(t);
#else
    return timegm(t);
#endif
}
""")
    win_gaps = [g for g in gaps if "crossplatform_crt" in g.type]
    assert not win_gaps, f"Guarded _mkgmtime should not be flagged: {win_gaps}"


def test_msvc_guard_not_flagged():
    """Calls guarded by #ifdef _MSC_VER must not be reported."""
    gaps = _run_scan("""
#include <string.h>
#ifdef _MSC_VER
bool eq(const char* a, const char* b) {
    return _stricmp(a, b) == 0;
}
#endif
""")
    win_gaps = [g for g in gaps if "crossplatform_crt" in g.type]
    assert not win_gaps, f"MSC_VER-guarded call should not be flagged: {win_gaps}"


def test_comment_only_line_not_flagged():
    """A comment mentioning sscanf_s must not be flagged."""
    gaps = _run_scan("""
// Use std::sscanf instead of sscanf_s here
void parse(const char* s) {
    int x = 0;
    std::sscanf(s, "%d", &x);
}
""")
    win_gaps = [g for g in gaps if "crossplatform_crt" in g.type]
    assert not win_gaps, f"Comment-only line must not be flagged: {win_gaps}"


def test_portable_sscanf_not_flagged():
    """std::sscanf (portable) must not trigger any gap."""
    gaps = _run_scan("""
#include <cstdio>
void parse(const char* s) {
    int year = 0;
    std::sscanf(s, "%d", &year);
}
""")
    win_gaps = [g for g in gaps if "crossplatform_crt" in g.type]
    assert not win_gaps, f"Portable std::sscanf must not be flagged: {win_gaps}"


def test_nested_win32_guard_not_flagged():
    """Nested #if _WIN32 within another block still protects the inner call."""
    gaps = _run_scan("""
#ifdef SOME_FEATURE
void process() {
#ifdef _WIN32
    char buf[64];
    _getcwd(buf, sizeof(buf));
#else
    getcwd(buf, sizeof(buf));
#endif
}
#endif
""")
    win_gaps = [g for g in gaps if "crossplatform_crt" in g.type]
    assert not win_gaps, f"Nested Win32 guard should not be flagged: {win_gaps}"


# ---------------------------------------------------------------------------
# Remediation content tests
# ---------------------------------------------------------------------------

def test_remediation_mentions_alternative():
    """Gap remediation must suggest a portable replacement."""
    gaps = _run_scan("""
void f(const char* s) {
    int x = 0;
    ::sscanf_s(s, "%d", &x);
}
""")
    win_gaps = [g for g in gaps if "crossplatform_crt" in g.type]
    assert win_gaps, "Expected a gap"
    assert win_gaps[0].remediation, "Remediation must not be empty"
    # Must mention a portable alternative
    assert "sscanf" in win_gaps[0].remediation.lower() or "snprintf" in win_gaps[0].remediation.lower()


def test_gap_severity_is_high():
    """Unguarded Windows CRT calls are build-breaking → severity HIGH."""
    gaps = _run_scan("""
void f() {
    char* p = _strdup("hello");
}
""")
    win_gaps = [g for g in gaps if "crossplatform_crt" in g.type]
    assert win_gaps, "Expected a gap"
    assert win_gaps[0].severity == "HIGH"


def test_gap_line_number_is_accurate():
    """Scanner must report the exact line number of the offending call."""
    code = "\n".join([
        "#include <cstdio>",            # line 1
        "void parse(const char* s) {",  # line 2
        "    int y = 0;",               # line 3
        "    ::sscanf_s(s, \"%d\", &y);",  # line 4
        "}",                            # line 5
    ])
    gaps = _run_scan(code)
    win_gaps = [g for g in gaps if "crossplatform_crt" in g.type]
    assert win_gaps, "Expected a gap"
    assert win_gaps[0].line == 4, f"Expected line 4, got {win_gaps[0].line}"


if __name__ == "__main__":
    import pytest
    pytest.main([__file__, "-v"])
