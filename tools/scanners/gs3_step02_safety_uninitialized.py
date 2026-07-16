#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 - Phase 4 (IMPROVED): Uninitialized & Initialization Semantics

Focus:
- Reduce false positives in tests/benchmarks/examples
- Detect real uninitialized-use patterns in production code
- Ignore explicit value-initialization forms ({} / = {} / std::optional)
"""

from dataclasses import dataclass
from enum import Enum
import re
from pathlib import Path
from typing import Any, Dict, List, Set


class UninitializedGapType(Enum):
    UNINITIALIZED_VAR = "uninitialized_variable"
    UNINITIALIZED_POINTER = "uninitialized_pointer"
    CONDITIONAL_INIT_USE = "conditional_initialization_use"
    UNINITIALIZED_ARRAY = "uninitialized_array"
    USE_AFTER_FREE = "use_after_free"
    POINTER_WITHOUT_CHECK = "pointer_without_null_check"


@dataclass
class UninitializedGap:
    gap_type: UninitializedGapType
    severity: str
    file_path: str
    line_number: int
    var_name: str = ""
    context: str = ""
    reason: str = ""

    def to_dict(self) -> Dict[str, Any]:
        return {
            "gap_type": self.gap_type.value,
            "severity": self.severity,
            "file_path": self.file_path,
            "line_number": self.line_number,
            "var_name": self.var_name,
            "context": self.context,
            "reason": self.reason,
        }


class UninitializedGapScannerImproved:
    """Improved uninitialized scanner with production/test and safe-init filters."""

    _DECL_TYPES = (
        "int", "float", "double", "bool", "size_t", "uint8_t", "uint16_t", "uint32_t",
        "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t", "char", "long", "short",
    )

    def __init__(self):
        self.gaps: List[UninitializedGap] = []
        self.gap_types: Dict[str, int] = {}

    @staticmethod
    def _normalize(path: Path) -> str:
        return str(path).replace("\\", "/").lower()

    def _is_non_prod_file(self, file_path: Path) -> bool:
        p = self._normalize(file_path)
        markers = [
            "/tests/", "test_", "_test.", "/benchmarks/", "bench_", "_bench.",
            "/examples/", "example_", "_demo.", "/fuzz/", "/tools/",
        ]
        return any(m in p for m in markers)

    @staticmethod
    def _is_comment_or_preprocessor(line: str) -> bool:
        s = line.strip()
        return not s or s.startswith("//") or s.startswith("/*") or s.startswith("*") or s.startswith("#")

    @staticmethod
    def _is_explicitly_initialized(line: str) -> bool:
        # Value-init and modern safe forms.
        safe_tokens = ["{}", "= {}", "= {};", "std::optional", "nullptr", "std::nullopt"]
        return any(t in line for t in safe_tokens) or "=" in line

    @staticmethod
    def _get_context(lines: List[str], idx0: int, window: int) -> str:
        start = max(0, idx0 - window)
        end = min(len(lines), idx0 + window + 1)
        return "".join(lines[start:end])

    @staticmethod
    def _extract_declared_var(line: str) -> str:
        m = re.search(r"\b(?:const\s+)?[\w:<>]+\s+([A-Za-z_]\w*)\s*;", line)
        return m.group(1) if m else ""

    def _check_uninitialized_var_use(self, file_path: str, lines: List[str]) -> List[UninitializedGap]:
        result: List[UninitializedGap] = []

        declared: Dict[str, int] = {}
        initialized: Set[str] = set()

        type_union = "|".join(re.escape(t) for t in self._DECL_TYPES)
        decl_re = re.compile(rf"\b(?:const\s+)?(?:{type_union})\b\s+[A-Za-z_]\w*\s*;")

        for i, line in enumerate(lines, 1):
            if self._is_comment_or_preprocessor(line):
                continue

            if decl_re.search(line) and not self._is_explicitly_initialized(line):
                var = self._extract_declared_var(line)
                if var:
                    declared[var] = i
                continue

            for var in list(declared.keys()):
                if re.search(rf"\b{re.escape(var)}\s*=", line):
                    initialized.add(var)

                if var in initialized:
                    continue

                # Usage heuristic: appears in expression, return, function arg, arithmetic.
                used = (
                    re.search(rf"\breturn\s+{re.escape(var)}\b", line) or
                    re.search(rf"\b{re.escape(var)}\b\s*[+\-*/%]", line) or
                    re.search(rf"[=,(]\s*{re.escape(var)}\s*[),;]", line)
                )
                if used:
                    result.append(
                        UninitializedGap(
                            gap_type=UninitializedGapType.UNINITIALIZED_VAR,
                            severity="HIGH",
                            file_path=file_path,
                            line_number=i,
                            var_name=var,
                            context=line.strip()[:120],
                            reason="Variable used before visible initialization",
                        )
                    )
                    initialized.add(var)

        return result

    def _check_uninitialized_arrays(self, file_path: str, lines: List[str]) -> List[UninitializedGap]:
        result: List[UninitializedGap] = []
        for i, line in enumerate(lines, 1):
            if self._is_comment_or_preprocessor(line):
                continue
            arr_decl = re.search(r"\b\w+\s+([A-Za-z_]\w*)\s*\[\d+\]\s*;", line)
            if not arr_decl:
                continue
            if self._is_explicitly_initialized(line):
                continue
            name = arr_decl.group(1)
            look = "".join(lines[i:min(len(lines), i + 6)])
            if re.search(rf"\b{re.escape(name)}\s*\[", look):
                result.append(
                    UninitializedGap(
                        gap_type=UninitializedGapType.UNINITIALIZED_ARRAY,
                        severity="MEDIUM",
                        file_path=file_path,
                        line_number=i,
                        var_name=name,
                        context=line.strip()[:120],
                        reason="Array appears used without explicit initialization",
                    )
                )
        return result

    def _check_use_after_free(self, file_path: str, lines: List[str]) -> List[UninitializedGap]:
        result: List[UninitializedGap] = []
        for i, line in enumerate(lines, 1):
            m = re.search(r"\bdelete\s+([A-Za-z_]\w*)\s*;", line)
            if not m:
                continue
            name = m.group(1)
            look = "".join(lines[i:min(len(lines), i + 4)])
            if re.search(rf"\b{re.escape(name)}\s*(->|\[)", look):
                result.append(
                    UninitializedGap(
                        gap_type=UninitializedGapType.USE_AFTER_FREE,
                        severity="CRITICAL",
                        file_path=file_path,
                        line_number=i,
                        var_name=name,
                        context=line.strip()[:120],
                        reason="Potential use-after-free after delete",
                    )
                )
        return result

    def scan_file(self, file_path: str) -> List[UninitializedGap]:
        p = Path(file_path)
        if self._is_non_prod_file(p):
            return []

        try:
            lines = p.read_text(encoding="utf-8", errors="ignore").splitlines(keepends=True)
        except Exception:
            return []

        rel_path = str(p).replace("\\", "/")
        file_gaps: List[UninitializedGap] = []
        file_gaps.extend(self._check_uninitialized_var_use(rel_path, lines))
        file_gaps.extend(self._check_uninitialized_arrays(rel_path, lines))
        file_gaps.extend(self._check_use_after_free(rel_path, lines))

        self.gaps.extend(file_gaps)
        for g in file_gaps:
            self.gap_types[g.gap_type.value] = self.gap_types.get(g.gap_type.value, 0) + 1

        return file_gaps

    def run_full_scan(self, src_path: str) -> List[UninitializedGap]:
        root = Path(src_path)
        if not root.exists():
            return []

        all_gaps: List[UninitializedGap] = []
        for ext in ("*.cpp", "*.cc", "*.hpp", "*.h"):
            for file_path in root.rglob(ext):
                all_gaps.extend(self.scan_file(str(file_path)))
        return all_gaps

    def get_stats(self) -> Dict[str, Any]:
        return {"total_gaps": len(self.gaps), "gap_types": self.gap_types}


if __name__ == "__main__":
    import sys

    root = sys.argv[1] if len(sys.argv) > 1 else "src"
    scanner = UninitializedGapScannerImproved()
    gaps = scanner.run_full_scan(root)

    print(f"Found {len(gaps)} uninitialized gaps (improved)")
    for g in gaps[:10]:
        print(f"  {g.file_path}:{g.line_number} [{g.severity}] {g.gap_type.value}")
