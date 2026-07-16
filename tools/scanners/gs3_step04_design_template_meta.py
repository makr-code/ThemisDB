#!/usr/bin/env python3
"""
Phase 6 - Template Meta-Programming

Conservative scanner for legacy SFINAE-heavy patterns and template complexity
that should migrate toward C++20 concepts/requires-based APIs.
"""

from __future__ import annotations

import re
from collections import Counter
from pathlib import Path
from typing import Dict, List


class TemplateMetaProgrammingScan:
    """Detect SFINAE anti-patterns and template meta-programming complexity."""

    ENABLE_IF_RETURN_RE = re.compile(r"\b(?:typename\s+)?std::enable_if(?:_t)?\s*<")
    ENABLE_IF_TEMPLATE_RE = re.compile(r"template\s*<[^>]*enable_if[^>]*>")
    ENABLE_IF_NON_TYPE_RE = re.compile(r"enable_if_t\s*<[^>]+,\s*(?:int|bool|size_t)\s*>\s*[A-Za-z_]\w*\s*=\s*")
    OLD_ENABLE_IF_TYPE_RE = re.compile(r"typename\s+std::enable_if\s*<.*>\s*::\s*type")
    VOID_T_RE = re.compile(r"\bstd::void_t\s*<")
    BOOL_DISPATCH_RE = re.compile(r"\bstd::integral_constant\s*<\s*bool\b")
    CONDITIONAL_CHAIN_RE = re.compile(r"std::conditional_t\s*<")
    TRAIT_GUARD_RE = re.compile(r"\bstd::is_[A-Za-z_]\w*\s*<")
    REQUIRES_RE = re.compile(r"\brequires\b")
    EXPLICIT_SPECIALIZATION_RE = re.compile(r"template\s*<\s*>\s*(?:class|struct)\s+([A-Za-z_]\w*)\s*<")
    TEMPLATE_DECL_RE = re.compile(r"^\s*template\s*<")

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
            "category": "template_meta_programming",
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

            self._check_sfinae_patterns(rel_file, lines)
            self._check_concept_migration_patterns(rel_file, lines)
            self._check_complexity_patterns(rel_file, lines)
            self._check_specialization_clusters(rel_file, lines)

        return self.gaps

    def _check_sfinae_patterns(self, rel_file: str, lines: List[str]) -> None:
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            if self.ENABLE_IF_RETURN_RE.search(line) and "(" in line:
                self._emit(
                    rel_file,
                    idx,
                    "HIGH",
                    "enable_if_return_type",
                    "enable_if used in function return type instead of requires/concepts",
                    line,
                )
            if self.ENABLE_IF_TEMPLATE_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "HIGH",
                    "enable_if_template_parameter",
                    "enable_if used in template parameter list",
                    line,
                )
            if self.ENABLE_IF_NON_TYPE_RE.search(line) or ("enable_if_t<" in line and "= 0" in line):
                self._emit(
                    rel_file,
                    idx,
                    "HIGH",
                    "enable_if_non_type_parameter",
                    "enable_if sentinel non-type template parameter increases API complexity",
                    line,
                )
            if self.OLD_ENABLE_IF_TYPE_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "HIGH",
                    "old_enable_if_type_alias",
                    "Legacy typename std::enable_if<...>::type pattern detected",
                    line,
                )
            if self.VOID_T_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "void_t_detection_idiom",
                    "void_t detection idiom could be simplified with concepts/requires",
                    line,
                )

    def _check_concept_migration_patterns(self, rel_file: str, lines: List[str]) -> None:
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            if self.BOOL_DISPATCH_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "bool_constant_dispatch",
                    "bool integral_constant dispatch pattern suggests pre-concepts meta-programming",
                    line,
                )
            if self.TRAIT_GUARD_RE.search(line) and self.TEMPLATE_DECL_RE.search(line) and not self.REQUIRES_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "trait_guard_without_requires",
                    "Type-trait constrained template lacks requires/concept syntax",
                    line,
                )
            if self.REQUIRES_RE.search(line) and "enable_if" in line:
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "requires_enable_if_mixed",
                    "Mixed requires and enable_if in one declaration increases template complexity",
                    line,
                )

    def _check_complexity_patterns(self, rel_file: str, lines: List[str]) -> None:
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            conditional_count = len(self.CONDITIONAL_CHAIN_RE.findall(line))
            if conditional_count >= 2:
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "nested_conditional_t_chain",
                    "Multiple std::conditional_t branches in one declaration indicate brittle template logic",
                    line,
                )

            if "template" in line and line.count("<") >= 4 and line.count(">") >= 4:
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "deep_template_nesting",
                    "Template declaration has deep angle-bracket nesting",
                    line,
                )

            if "::type::type" in line:
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "nested_type_extraction_chain",
                    "Nested ::type extraction suggests legacy meta-programming layering",
                    line,
                )

    def _check_specialization_clusters(self, rel_file: str, lines: List[str]) -> None:
        counts: Counter[str] = Counter()
        first_lines: Dict[str, int] = {}
        pending_template_line = False

        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            stripped = line.strip()
            if stripped.startswith("template <>"):
                pending_template_line = True
            combined = line
            if pending_template_line and not stripped.startswith("template <>"):
                combined = f"template <> {stripped}"
                pending_template_line = False

            match = self.EXPLICIT_SPECIALIZATION_RE.search(combined)
            if not match:
                if stripped and stripped not in {"template <>", "template<>"}:
                    pending_template_line = False
                continue
            name = match.group(1)
            counts[name] += 1
            first_lines.setdefault(name, idx)
            pending_template_line = False

        for name, count in counts.items():
            if count >= 2:
                self._emit(
                    rel_file,
                    first_lines[name],
                    "MEDIUM",
                    "explicit_specialization_cluster",
                    f"Multiple explicit specializations for template '{name}' in one file raise maintenance risk",
                    f"{name}: {count} explicit specializations",
                )


if __name__ == "__main__":
    import sys

    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    scanner = TemplateMetaProgrammingScan(str(root))
    cpp_files = []
    for ext in ("*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp", "*.hh", "*.hxx"):
        cpp_files.extend(root.rglob(ext))
    gaps = scanner.scan_files(cpp_files)
    print(f"Found {len(gaps)} template meta-programming findings")
    for gap in gaps[:20]:
        print(f"  {gap['file']}:{gap['line']} [{gap['severity']}] {gap['pattern']}")
