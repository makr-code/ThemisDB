#!/usr/bin/env python3
"""
Phase 6 - Const Correctness & API Design

Conservative scanner focused on high-signal const-correctness and API design
issues in production C++ code.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Dict, List, Set


class ConstCorrectnessApiDesignScan:
    """Detect const-correctness violations and expensive const-by-value APIs."""

    MUTABLE_DECL_RE = re.compile(r"\bmutable\b[^;=]*?\b([A-Za-z_]\w*)\s*(?:\[[^\]]+\])?\s*;")
    CONST_METHOD_RE = re.compile(r"\)\s*const\b")
    NON_CONST_REF_RETURN_RE = re.compile(
        r"^\s*(?!.*\bconst\s+[\w:<>]+\s*&)(?:virtual\s+)?(?:inline\s+)?(?:static\s+)?"
        r"[\w:<>]+\s*&\s+[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:noexcept\s*)?const\b"
    )
    NON_CONST_PTR_RETURN_RE = re.compile(
        r"^\s*(?!.*\bconst\s+[\w:<>]+\s*\*)(?:virtual\s+)?(?:inline\s+)?(?:static\s+)?"
        r"[\w:<>]+\s*\*\s*[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:noexcept\s*)?const\b"
    )
    MUTABLE_CONTAINER_RETURN_RES: Dict[str, re.Pattern[str]] = {
        "mutable_vector_return_const_method": re.compile(
            r"^\s*(?:virtual\s+)?std::vector\s*<[^>]+>\s*&\s+[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:noexcept\s*)?const\b"
        ),
        "mutable_map_return_const_method": re.compile(
            r"^\s*(?:virtual\s+)?std::map\s*<[^>]+>\s*&\s+[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:noexcept\s*)?const\b"
        ),
        "mutable_unordered_map_return_const_method": re.compile(
            r"^\s*(?:virtual\s+)?std::unordered_map\s*<[^>]+>\s*&\s+[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:noexcept\s*)?const\b"
        ),
        "mutable_set_return_const_method": re.compile(
            r"^\s*(?:virtual\s+)?std::(?:unordered_)?set\s*<[^>]+>\s*&\s+[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:noexcept\s*)?const\b"
        ),
        "mutable_string_return_const_method": re.compile(
            r"^\s*(?:virtual\s+)?std::basic_string\s*<[^>]+>\s*&\s+[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:noexcept\s*)?const\b|"
            r"^\s*(?:virtual\s+)?std::string\s*&\s+[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:noexcept\s*)?const\b"
        ),
        "mutable_span_return_const_method": re.compile(
            r"^\s*(?:virtual\s+)?std::span\s*<\s*(?!const\b)[^>]+>\s+[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:noexcept\s*)?const\b"
        ),
    }
    CONST_BY_VALUE_PARAM_PATTERNS: Dict[str, re.Pattern[str]] = {
        "const_value_param_string": re.compile(r"\bconst\s+std::string\s+[A-Za-z_]\w*(?=\s*[,)=])"),
        "const_value_param_vector": re.compile(r"\bconst\s+std::vector\s*<[^>]+>\s+[A-Za-z_]\w*(?=\s*[,)=])"),
        "const_value_param_map": re.compile(r"\bconst\s+std::map\s*<[^>]+>\s+[A-Za-z_]\w*(?=\s*[,)=])"),
        "const_value_param_unordered_map": re.compile(
            r"\bconst\s+std::unordered_map\s*<[^>]+>\s+[A-Za-z_]\w*(?=\s*[,)=])"
        ),
        "const_value_param_set": re.compile(r"\bconst\s+std::(?:unordered_)?set\s*<[^>]+>\s+[A-Za-z_]\w*(?=\s*[,)=])"),
        "const_value_param_function": re.compile(r"\bconst\s+std::function\s*<[^>]+>\s+[A-Za-z_]\w*(?=\s*[,)=])"),
        "const_value_param_optional": re.compile(r"\bconst\s+std::optional\s*<[^>]+>\s+[A-Za-z_]\w*(?=\s*[,)=])"),
        "const_value_param_filesystem_path": re.compile(
            r"\bconst\s+std::filesystem::path\s+[A-Za-z_]\w*(?=\s*[,)=])"
        ),
    }

    def __init__(self, repo_root: str = "."):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    @staticmethod
    def _is_non_prod_path(rel_file: str) -> bool:
        p = rel_file.lower()
        markers = [
            "tests/", "test_", "_test.", "benchmarks/", "bench_", "_bench.", "examples/",
            "demo_", "_demo.", "_mock.", "tools/", "scripts/", "fuzz/",
        ]
        return any(m in p for m in markers)

    @staticmethod
    def _is_comment_or_pp(line: str) -> bool:
        s = line.strip()
        return not s or s.startswith("//") or s.startswith("/*") or s.startswith("*") or s.startswith("#")

    def _emit(self, rel_file: str, line: int, severity: str, pattern: str, description: str, context: str) -> None:
        self.gaps.append({
            "file": rel_file,
            "line": line,
            "category": "const_correctness_api_design",
            "severity": severity,
            "pattern": pattern,
            "description": description,
            "context": context.strip()[:220],
        })

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []

        for file_path in file_list:
            if file_path.suffix not in [".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx"]:
                continue

            try:
                rel_file = str(file_path.relative_to(self.repo_root)).replace("\\", "/")
            except Exception:
                rel_file = str(file_path).replace("\\", "/")

            if self._is_non_prod_path(rel_file):
                continue

            try:
                lines = file_path.read_text(encoding="utf-8", errors="ignore").splitlines()
            except Exception:
                continue

            mutable_members = self._collect_mutable_members(lines)
            self._check_const_cast(rel_file, lines)
            self._check_non_const_returns(rel_file, lines)
            self._check_mutable_container_returns(rel_file, lines)
            self._check_mutable_member_writes(rel_file, lines, mutable_members)
            self._check_const_by_value_params(rel_file, lines)

        return self.gaps

    def _collect_mutable_members(self, lines: List[str]) -> Set[str]:
        members: Set[str] = set()
        for line in lines:
            if self._is_comment_or_pp(line):
                continue
            match = self.MUTABLE_DECL_RE.search(line)
            if match:
                members.add(match.group(1))
        return members

    def _const_method_context(self, lines: List[str], idx0: int, lookback: int = 8) -> str:
        start = max(0, idx0 - lookback)
        return "\n".join(lines[start:idx0 + 1])

    def _check_const_cast(self, rel_file: str, lines: List[str]) -> None:
        for idx, line in enumerate(lines, 1):
            if "const_cast<" not in line:
                continue
            if not self.CONST_METHOD_RE.search(self._const_method_context(lines, idx - 1)):
                continue
            self._emit(
                rel_file,
                idx,
                "HIGH",
                "const_cast_in_const_method",
                "const_cast used inside const-qualified method",
                line,
            )

    def _check_non_const_returns(self, rel_file: str, lines: List[str]) -> None:
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            if self.NON_CONST_REF_RETURN_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "HIGH",
                    "non_const_ref_return_const_method",
                    "Const-qualified method returns mutable reference",
                    line,
                )
            if self.NON_CONST_PTR_RETURN_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "HIGH",
                    "non_const_ptr_return_const_method",
                    "Const-qualified method returns mutable pointer",
                    line,
                )

    def _check_mutable_container_returns(self, rel_file: str, lines: List[str]) -> None:
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            for pattern_name, pattern_re in self.MUTABLE_CONTAINER_RETURN_RES.items():
                if pattern_re.search(line):
                    self._emit(
                        rel_file,
                        idx,
                        "HIGH",
                        pattern_name,
                        "Const-qualified method exposes mutable container/view",
                        line,
                    )

    def _check_mutable_member_writes(self, rel_file: str, lines: List[str], mutable_members: Set[str]) -> None:
        if not mutable_members:
            return

        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            context = self._const_method_context(lines, idx - 1, lookback=12)
            if not self.CONST_METHOD_RE.search(context):
                continue

            for member in mutable_members:
                patterns = [
                    rf"\b{re.escape(member)}\s*=",
                    rf"\b{re.escape(member)}\s*(?:\+\+|--)",
                    rf"\b{re.escape(member)}\s*\.(?:clear|insert|emplace|push_back|emplace_back|erase|reset|store)\s*\(",
                    rf"this->\s*{re.escape(member)}\s*=",
                ]
                if any(re.search(p, line) for p in patterns):
                    self._emit(
                        rel_file,
                        idx,
                        "MEDIUM",
                        "mutable_member_write_in_const_method",
                        "Mutable member updated inside const-qualified method",
                        line,
                    )
                    break

    def _check_const_by_value_params(self, rel_file: str, lines: List[str]) -> None:
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            if "(" not in line or ")" not in line:
                continue

            for pattern_name, pattern_re in self.CONST_BY_VALUE_PARAM_PATTERNS.items():
                if pattern_re.search(line):
                    self._emit(
                        rel_file,
                        idx,
                        "MEDIUM",
                        pattern_name,
                        "Heavy read-only parameter passed by const value",
                        line,
                    )


if __name__ == "__main__":
    import sys

    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    scanner = ConstCorrectnessApiDesignScan(str(root))
    cpp_files = []
    for ext in ("*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp", "*.hh", "*.hxx"):
        cpp_files.extend(root.rglob(ext))
    gaps = scanner.scan_files(cpp_files)
    print(f"Found {len(gaps)} const-correctness/API design findings")
    for gap in gaps[:20]:
        print(f"  {gap['file']}:{gap['line']} [{gap['severity']}] {gap['pattern']}")
