"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cross-compile-reviewer.py                          ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:45:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     482                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Cross-Compile Reviewer for ThemisDB

Automatische Code-Review für Cross-Compile-Kompatibilität
Kann lokal (pre-commit), in CI/CD (GitHub Actions), oder via Copilot-Inline verwendet werden.

Verwendung:
    python3 scripts/cross-compile-reviewer.py --pr-files file1.cpp file2.h
    python3 scripts/cross-compile-reviewer.py --staged-files
    python3 scripts/cross-compile-reviewer.py --file myfile.cpp
"""

import sys
import json
import re
from pathlib import Path
from typing import List, Dict, Tuple, Set
from dataclasses import dataclass, field

# ============================================================================
# CONFIGURATION
# ============================================================================

FORBIDDEN_HEADERS = {
    # Windows
    "windows.h", "winbase.h", "wininet.h", "winsock.h", "winsock2.h",
    "windowsx.h", "winerror.h", "winuser.h", "winreg.h",
    # POSIX/Linux
    "unistd.h", "sys/socket.h", "sys/select.h", "netinet/in.h", "arpa/inet.h",
    "pthread.h", "semaphore.h", "mqueue.h", "fcntl.h", "termios.h", "ioctl.h",
    "sys/mman.h", "dlfcn.h", "link.h",
    # macOS/Darwin
    "CoreFoundation/CoreFoundation.h", "Cocoa/Cocoa.h", "Security/Security.h",
    "AppKit/AppKit.h", "Foundation/Foundation.h",
    # X11
    "X11/Xlib.h", "X11/Xutil.h", "X11/keysym.h", "GL/glx.h",
}

ALLOWED_HEADERS_PREFIXES = {
    "boost/", "fmt/", "spdlog/", "nlohmann/", "protobuf", "grpc",
    "openssl/", "sqlite3.h", "rocksdb/", "zstd.h", "yaml-cpp/",
    # C++ Standard Library
    "algorithm", "array", "atomic", "bitset", "chrono", "cmath", "complex",
    "deque", "exception", "forward_list", "fstream", "functional", "future",
    "iomanip", "ios", "iosfwd", "iostream", "istream", "iterator",
    "limits", "list", "locale", "map", "memory", "mutex", "new", "numeric",
    "optional", "ostream", "queue", "random", "ratio", "regex", "set",
    "shared_mutex", "sstream", "stack", "stdexcept", "string", "string_view",
    "thread", "tuple", "typeinfo", "type_traits", "unordered_map",
    "unordered_set", "utility", "valarray", "variant", "vector", "version",
}

FORBIDDEN_FUNCTIONS = {
    # Windows
    "GetFileSize", "CreateFileA", "CreateFileW", "CreateThread", "CreateProcess",
    "SetEnvironmentVariable", "GetEnvironmentVariable", "RegOpenKeyEx",
    "RegQueryValueEx", "ShellExecute", "CoInitialize", "CoUninitialize",
    "LoadLibrary", "GetProcAddress", "SetConsoleCP", "GetConsoleMode",
    # POSIX
    "fork", "exec", "execve", "mmap", "munmap", "dlopen", "dlsym",
    "pthread_create", "pthread_join", "pthread_mutex_init", "sem_init",
    "msgget", "shmat", "ioctl", "fcntl",
    # macOS specific
    "FSEventStreamCreate", "FSEventStreamStart", "Gestalt",
}

FUNCTION_REPLACEMENTS = {
    "CreateThread": "std::thread",
    "GetEnvironmentVariable": "std::getenv",
    "SetEnvironmentVariable": "std::putenv / std::setenv",
    "fork": "std::thread + subprocess library",
    "pthread_create": "std::thread",
    "pthread_mutex": "std::mutex",
    "mmap": "boost::interprocess",
    "dlopen": "boost::dll",
    "open": "std::fstream or std::filesystem",
}

# ============================================================================
# DATA STRUCTURES
# ============================================================================

@dataclass
class Violation:
    """Represents a code review violation"""
    rule: str
    severity: str  # CRITICAL, HIGH, WARNING
    line_number: int
    file_path: str
    code_snippet: str
    message: str
    suggestion: str = ""

    def to_dict(self) -> Dict:
        return {
            "rule": self.rule,
            "severity": self.severity,
            "line": self.line_number,
            "file": self.file_path,
            "snippet": self.code_snippet,
            "message": self.message,
            "suggestion": self.suggestion,
        }

@dataclass
class ReviewResult:
    """Summary of code review"""
    approved: bool
    violations: List[Violation] = field(default_factory=list)
    warnings: List[Violation] = field(default_factory=list)
    total_files: int = 0
    critical_count: int = 0
    high_count: int = 0

    def to_dict(self) -> Dict:
        return {
            "approved": self.approved,
            "critical_violations": self.critical_count,
            "high_violations": self.high_count,
            "total_violations": len(self.violations) + len(self.warnings),
            "violations": [v.to_dict() for v in self.violations],
            "warnings": [v.to_dict() for v in self.warnings],
        }

# ============================================================================
# REVIEWER LOGIC
# ============================================================================

class CrossCompileReviewer:
    """Main reviewer class"""

    def __init__(self):
        self.violations: List[Violation] = []
        self.warnings: List[Violation] = []

    def review_file(self, file_path: str) -> ReviewResult:
        """Review a single file"""
        path = Path(file_path)

        # Only review C++, CMake, and Python files
        if path.suffix not in {".cpp", ".cc", ".cxx", ".h", ".hpp", ".cmake", ".py"}:
            return ReviewResult(approved=True)

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            print(f"⚠️  Cannot read {file_path}: {e}")
            return ReviewResult(approved=True, total_files=1)

        self.violations = []
        self.warnings = []

        # Run all checks
        self._check_forbidden_headers(file_path, lines)
        self._check_conditional_compilation(file_path, lines)
        self._check_hardcoded_paths(file_path, lines)
        self._check_forbidden_functions(file_path, lines)
        self._check_compiler_pragmas(file_path, lines)
        self._check_memory_layout_assumptions(file_path, lines)

        # Determine approval
        critical = sum(1 for v in self.violations if v.severity == "CRITICAL")
        high = sum(1 for v in self.violations if v.severity == "HIGH")

        result = ReviewResult(
            approved=(critical == 0),
            violations=self.violations,
            warnings=self.warnings,
            total_files=1,
            critical_count=critical,
            high_count=high,
        )

        return result

    def _check_forbidden_headers(self, file_path: str, lines: List[str]):
        """RULE 1: Check for forbidden platform-specific headers"""
        for i, line in enumerate(lines, 1):
            # Look for #include statements
            match = re.search(r'#include\s+[<"]([^>"]+)[>"]', line)
            if not match:
                continue

            header = match.group(1)
            header_name = Path(header).name

            if header_name in FORBIDDEN_HEADERS:
                self.violations.append(Violation(
                    rule="RULE_1_forbidden_headers",
                    severity="CRITICAL",
                    line_number=i,
                    file_path=file_path,
                    code_snippet=line.strip(),
                    message=f"Forbidden platform-specific header: {header_name}",
                    suggestion=f"Use cross-platform alternative from vcpkg or C++20 standard",
                ))

    def _check_conditional_compilation(self, file_path: str, lines: List[str]):
        """RULE 2: Check for incomplete conditional compilation"""
        i = 0
        while i < len(lines):
            line = lines[i]

            # Look for #ifdef directives
            if re.match(r'^\s*#ifdef\s+(_WIN32|__linux__|__APPLE__|_MSC_VER)', line):
                # Find corresponding #endif
                level = 0
                has_else = False
                j = i

                while j < len(lines):
                    current = lines[j]
                    if re.match(r'^\s*#ifdef', current):
                        level += 1
                    elif re.match(r'^\s*#endif', current):
                        level -= 1
                        if level == 0:
                            break
                    elif re.match(r'^\s*#else', current) and level == 1:
                        has_else = True
                    j += 1

                if not has_else and level == 0:
                    self.violations.append(Violation(
                        rule="RULE_3_conditional_compilation",
                        severity="CRITICAL",
                        line_number=i + 1,
                        file_path=file_path,
                        code_snippet=line.strip(),
                        message="Missing #else block for platform conditional",
                        suggestion="Add #else or #elif for other platforms",
                    ))

            i += 1

    def _check_hardcoded_paths(self, file_path: str, lines: List[str]):
        """RULE 3: Check for hardcoded absolute paths"""
        hardcoded_patterns = [
            (r'["\']C:\\', "Windows absolute path"),
            (r'["\']\/var\/', "Linux /var path"),
            (r'["\']\/opt\/', "Linux /opt path"),
            (r'["\']\/usr\/', "Linux /usr path"),
            (r'["\']\/home\/', "Linux /home path"),
            (r'["\']~', "Home directory shortcut"),
        ]

        for i, line in enumerate(lines, 1):
            for pattern, desc in hardcoded_patterns:
                if re.search(pattern, line):
                    self.violations.append(Violation(
                        rule="RULE_4_hardcoded_paths",
                        severity="CRITICAL",
                        line_number=i,
                        file_path=file_path,
                        code_snippet=line.strip(),
                        message=f"Hardcoded path not cross-compile safe: {desc}",
                        suggestion="Use std::filesystem::path or boost::filesystem with getenv()",
                    ))

    def _check_forbidden_functions(self, file_path: str, lines: List[str]):
        """RULE 4: Check for forbidden OS-specific function calls"""
        for i, line in enumerate(lines, 1):
            for func in FORBIDDEN_FUNCTIONS:
                if re.search(rf'\b{func}\s*\(', line):
                    suggestion = FUNCTION_REPLACEMENTS.get(func, "Use cross-platform alternative")
                    self.violations.append(Violation(
                        rule="RULE_5_forbidden_os_apis",
                        severity="CRITICAL",
                        line_number=i,
                        file_path=file_path,
                        code_snippet=line.strip(),
                        message=f"OS-specific function call: {func}",
                        suggestion=f"Use {suggestion}",
                    ))

    def _check_compiler_pragmas(self, file_path: str, lines: List[str]):
        """RULE 5: Check for compiler-specific pragmas without fallback"""
        i = 0
        while i < len(lines):
            line = lines[i]

            if re.search(r'#pragma\s+warning\(', line):
                # Check if there's an #else with GCC equivalent nearby
                found_gcc = False
                for j in range(max(0, i-2), min(len(lines), i+5)):
                    if re.search(r'#pragma\s+GCC\s+diagnostic', lines[j]):
                        found_gcc = True
                        break

                if not found_gcc and not re.search(r'#ifdef\s+_MSC_VER', lines[i-1] if i > 0 else ""):
                    self.warnings.append(Violation(
                        rule="RULE_6_compiler_pragmas",
                        severity="HIGH",
                        line_number=i + 1,
                        file_path=file_path,
                        code_snippet=line.strip(),
                        message="MSVC-only pragma without GCC/Clang fallback",
                        suggestion="Wrap in #ifdef _MSC_VER ... #else ... #endif",
                    ))

            i += 1

    def _check_memory_layout_assumptions(self, file_path: str, lines: List[str]):
        """RULE 6: Check for architecture-specific memory assumptions"""
        for i, line in enumerate(lines, 1):
            # Check for sizeof assumptions
            if re.search(r'sizeof\s*\(\s*void\s*\*\s*\)\s*==\s*8', line):
                self.warnings.append(Violation(
                    rule="RULE_7_memory_layout",
                    severity="HIGH",
                    line_number=i,
                    file_path=file_path,
                    code_snippet=line.strip(),
                    message="Assumption that sizeof(void*) == 8 (not ARM compatible)",
                    suggestion="Use static_assert or validate at runtime",
                ))

            # Check for pointer casts without validation
            if re.search(r'reinterpret_cast\s*<\s*uintptr_t\s*>\s*\(', line):
                self.warnings.append(Violation(
                    rule="RULE_7_memory_layout",
                    severity="HIGH",
                    line_number=i,
                    file_path=file_path,
                    code_snippet=line.strip(),
                    message="Pointer cast - verify alignment is correct on ARM",
                    suggestion="Add static_assert for pointer size assumptions",
                ))

# ============================================================================
# OUTPUT & REPORTING
# ============================================================================

def format_output(result: ReviewResult, file_path: str = None) -> str:
    """Format review result for human consumption"""
    output = []

    if result.approved:
        output.append("✅ APPROVED - All cross-compile rules passed")
    else:
        output.append("❌ REJECTED - Cross-compile violations found")

    output.append(f"\n📊 Summary:")
    output.append(f"  Critical Violations: {result.critical_count}")
    output.append(f"  High Violations: {result.high_count}")
    output.append(f"  Total Violations: {len(result.violations) + len(result.warnings)}")

    if result.violations:
        output.append(f"\n🔴 Critical Issues:")
        for v in result.violations:
            if v.severity == "CRITICAL":
                output.append(f"  [{v.rule}] Line {v.line_number}: {v.message}")
                output.append(f"      Code: {v.code_snippet}")
                if v.suggestion:
                    output.append(f"      💡 {v.suggestion}")

        output.append(f"\n⚠️  High Priority Issues:")
        for v in result.violations:
            if v.severity == "HIGH":
                output.append(f"  [{v.rule}] Line {v.line_number}: {v.message}")
                output.append(f"      Code: {v.code_snippet}")

    if result.warnings:
        output.append(f"\n⚠️  Warnings:")
        for w in result.warnings:
            output.append(f"  [{w.rule}] Line {w.line_number}: {w.message}")

    output.append(f"\n📖 Reference: CROSS_COMPILE_REQUIREMENTS.md")

    return "\n".join(output)

# ============================================================================
# CLI INTERFACE
# ============================================================================

def main():
    import argparse

    parser = argparse.ArgumentParser(
        description="Cross-Compile Code Review Tool for ThemisDB",
    )
    parser.add_argument('--file', help='Review single file')
    parser.add_argument('--files', nargs='+', help='Review multiple files')
    parser.add_argument('--pr-files', nargs='+', help='Review PR changed files')
    parser.add_argument('--staged-files', action='store_true', help='Review staged git files')
    parser.add_argument('--output', choices=['text', 'json'], default='text', help='Output format')
    parser.add_argument('--json-file', help='Output JSON to file')

    args = parser.parse_args()

    files_to_review = []

    if args.file:
        files_to_review = [args.file]
    elif args.files:
        files_to_review = args.files
    elif args.pr_files:
        files_to_review = args.pr_files
    elif args.staged_files:
        # Get staged files from git
        import subprocess
        try:
            result = subprocess.run(['git', 'diff', '--staged', '--name-only'],
                                   capture_output=True, text=True, check=True)
            files_to_review = result.stdout.strip().split('\n')
        except Exception as e:
            print(f"Error getting staged files: {e}")
            sys.exit(1)

    if not files_to_review:
        parser.print_help()
        sys.exit(1)

    # Review all files
    reviewer = CrossCompileReviewer()
    all_violations = []
    all_warnings = []
    total_files = 0
    approved = True

    for file_path in files_to_review:
        if not file_path or not Path(file_path).exists():
            continue

        result = reviewer.review_file(file_path)
        all_violations.extend(result.violations)
        all_warnings.extend(result.warnings)
        total_files += result.total_files
        approved = approved and result.approved

    # Generate output
    final_result = ReviewResult(
        approved=approved,
        violations=all_violations,
        warnings=all_warnings,
        total_files=total_files,
        critical_count=sum(1 for v in all_violations if v.severity == "CRITICAL"),
        high_count=sum(1 for v in all_violations if v.severity == "HIGH") +
                   sum(1 for w in all_warnings if w.severity == "HIGH"),
    )

    if args.output == 'json':
        output = json.dumps(final_result.to_dict(), indent=2)
        if args.json_file:
            Path(args.json_file).write_text(output)
            print(f"JSON output written to {args.json_file}")
        else:
            print(output)
    else:
        output = format_output(final_result)
        print(output)

    # Exit with error if rejected
    sys.exit(0 if approved else 1)

if __name__ == '__main__':
    main()
