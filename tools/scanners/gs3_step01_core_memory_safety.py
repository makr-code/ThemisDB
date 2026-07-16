#!/usr/bin/env python3
"""
Gap Scanner Step 01.1 (IMPROVED) - Memory Safety

Phase-4 improvements:
- Test/bench/example exclusion for lower false positives
- Safe API whitelist: std::string, std::vector, nlohmann::json, std::span, .at()
- RAII-aware handling for new/delete heuristics
"""

import re
import sys
from pathlib import Path
from typing import List, Set

sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class MemorySafetyScannerImproved(BaseGapScanner):
    """Improved memory safety scanner with safer heuristics."""

    PRIORITY = ScannerPriority.MEDIUM
    ENABLED = True
    MAX_RUNTIME_SECONDS = 30

    def __init__(self):
        super().__init__("MemorySafetyScannerImproved", "4.0")

        self.new_pattern = re.compile(r"\bnew\s+[\w:]+\s*\(")
        self.delete_pattern = re.compile(r"\bdelete\s+([A-Za-z_]\w*)")
        self.malloc_pattern = re.compile(r"\b(malloc|calloc|realloc)\s*\(")
        self.ptr_index_pattern = re.compile(r"\b([A-Za-z_]\w*(?:ptr|buffer|data|array))\s*\[\s*([^\]]+)\s*\]")
        self.ptr_add_pattern = re.compile(r"\*\s*\(\s*([A-Za-z_]\w*)\s*\+\s*([^)]+)\)")

    @staticmethod
    def _is_non_prod_file(file_path: Path) -> bool:
        p = str(file_path).replace("\\", "/").lower()
        patterns = [
            "/tests/", "test_", "_test.", "/benchmarks/", "bench_", "_bench.",
            "/examples/", "/demo/", "/fuzz/", "/tools/",
        ]
        return any(x in p for x in patterns)

    @staticmethod
    def _is_comment(line: str) -> bool:
        s = line.strip()
        return not s or s.startswith("//") or s.startswith("/*") or s.startswith("*") or s.startswith("#")

    @staticmethod
    def _uses_safe_api(line: str) -> bool:
        safe_tokens = [
            "std::string", "std::vector", "std::array", "nlohmann::json",
            "std::span", "gsl::span", ".at(",
        ]
        return any(t in line for t in safe_tokens)

    @staticmethod
    def _has_raii_context(lines: List[str], line_no: int) -> bool:
        start = max(0, line_no - 3)
        end = min(len(lines), line_no + 2)
        ctx = "\n".join(lines[start:end])
        return any(x in ctx for x in [
            "unique_ptr", "shared_ptr", "weak_ptr", "make_unique", "make_shared",
        ])

    def scan(self, source_dir: str) -> List[Gap]:
        gaps: List[Gap] = []
        self.source_path = Path(source_dir).resolve()

        for file_path in self._scan_files(source_dir):
            file_path = file_path.resolve()
            self.files_scanned += 1

            if self._is_non_prod_file(file_path):
                continue

            lines = self._read_file_lines(file_path)
            if not lines:
                continue

            gaps.extend(self._check_new_without_raii(file_path, lines))
            gaps.extend(self._check_pointer_arithmetic(file_path, lines))
            gaps.extend(self._check_unchecked_malloc(file_path, lines))
            gaps.extend(self._check_delete_without_nullptr(file_path, lines))

        return self.deduplicate(gaps)

    def _check_new_without_raii(self, file_path: Path, lines: List[str]) -> List[Gap]:
        gaps: List[Gap] = []
        for line_no, line in enumerate(lines, 1):
            if self._is_comment(line):
                continue
            if not self.new_pattern.search(line):
                continue

            if self._has_raii_context(lines, line_no - 1):
                continue

            gaps.append(
                Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type="new_without_raii",
                    severity="CRITICAL",
                    confidence=0.78,
                    description="Raw new() without RAII wrapper",
                    remediation="Use std::unique_ptr/std::make_unique",
                    context="\n".join(self._get_context(lines, line_no, window=3)),
                )
            )
        return gaps

    def _check_pointer_arithmetic(self, file_path: Path, lines: List[str]) -> List[Gap]:
        gaps: List[Gap] = []

        for line_no, line in enumerate(lines, 1):
            if self._is_comment(line):
                continue
            if self._uses_safe_api(line):
                continue

            idx_match = self.ptr_index_pattern.search(line)
            add_match = self.ptr_add_pattern.search(line)
            if not idx_match and not add_match:
                continue

            expr = ""
            if idx_match:
                expr = idx_match.group(2).strip()
            elif add_match:
                expr = add_match.group(2).strip()

            if expr.isdigit():
                continue

            if self._has_bounds_guard(lines, line_no):
                continue

            gaps.append(
                Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type="pointer_arithmetic_unbounded",
                    severity="HIGH",
                    confidence=0.72,
                    description="Pointer/array access without visible bounds check",
                    remediation="Add bounds check (index < size) before access",
                    context="\n".join(self._get_context(lines, line_no, window=3)),
                )
            )

        return gaps

    def _has_bounds_guard(self, lines: List[str], line_no: int) -> bool:
        start = max(0, line_no - 12)
        end = min(len(lines), line_no + 1)
        ctx = "\n".join(lines[start:end])
        checks = [
            r"if\s*\([^)]*(?:\.size\s*\(|size\s*\(|\.length\s*\()",
            r"assert\s*\([^)]*(?:index|idx|offset|size)",
            r"CHECK\s*\([^)]*(?:index|idx|offset|size)",
            r"\bstd::min\s*\(",
            r"\bclamp\s*\(",
        ]
        return any(re.search(p, ctx) for p in checks)

    def _check_unchecked_malloc(self, file_path: Path, lines: List[str]) -> List[Gap]:
        gaps: List[Gap] = []
        for line_no, line in enumerate(lines, 1):
            if self._is_comment(line):
                continue
            m = self.malloc_pattern.search(line)
            if not m:
                continue

            if self._context_window_search(
                lines,
                line_no,
                [
                    r"if\s*\([^)]*!=\s*nullptr",
                    r"if\s*\([^)]*==\s*nullptr",
                    r"if\s*\(!\s*\w+\)",
                    r"CHECK\s*\([^)]*nullptr",
                ],
                window=4,
            ):
                continue

            gaps.append(
                Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type="unchecked_malloc",
                    severity="HIGH",
                    confidence=0.76,
                    description=f"Unchecked {m.group(1)}() result",
                    remediation="Check returned pointer before dereference",
                    context="\n".join(self._get_context(lines, line_no, window=3)),
                )
            )
        return gaps

    def _check_delete_without_nullptr(self, file_path: Path, lines: List[str]) -> List[Gap]:
        gaps: List[Gap] = []
        for line_no, line in enumerate(lines, 1):
            if self._is_comment(line):
                continue
            m = self.delete_pattern.search(line)
            if not m:
                continue

            if self._is_destructor_scope(lines, line_no):
                continue

            name = m.group(1)
            if self._context_window_search(lines, line_no, [rf"{re.escape(name)}\s*=\s*nullptr"], window=2):
                continue

            gaps.append(
                Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type="delete_without_nullptr",
                    severity="MEDIUM",
                    confidence=0.66,
                    description="delete without explicit nullification",
                    remediation=f"Set pointer to nullptr after delete: {name} = nullptr;",
                    context="\n".join(self._get_context(lines, line_no, window=3)),
                )
            )
        return gaps

    def _is_destructor_scope(self, lines: List[str], line_no: int) -> bool:
        start = max(0, line_no - 18)
        ctx = "\n".join(lines[start:line_no])
        return re.search(r"~[A-Za-z_]\w*\s*\(", ctx) is not None


if __name__ == "__main__":
    import time

    source_dir = sys.argv[1] if len(sys.argv) > 1 else "./src"
    print("[Memory Safety Scanner Improved] Starting scan...")
    t0 = time.time()

    scanner = MemorySafetyScannerImproved()
    gaps = scanner.scan(source_dir)

    dt = time.time() - t0
    print(f"Found {len(gaps)} gaps in {dt:.2f}s")
    print(f"Scanned {scanner.files_scanned} files")
