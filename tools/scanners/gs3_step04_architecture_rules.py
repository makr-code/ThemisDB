#!/usr/bin/env python3
"""
Phase 10-2: Themis Architecture Rules Scanner

Rule sources:
- ARCHITECTURE.md (layered architecture)
- audit/docs/Audit/ARCHITECTURE_AUDIT.md (Appendix A prohibited dependencies)

Detects include dependency violations for bottom/foundation layers.
"""

import re
from pathlib import Path
from typing import Dict, List, Optional


class ThemisArchitectureRulesScan:
    """Scan for architecture layer dependency violations."""

    def __init__(self, repo_root: str = "."):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

        # Rules derived from ARCHITECTURE_AUDIT Appendix A.2
        self.prohibited_dependencies = {
            "config": {"server", "query", "storage", "llm"},
            "themis": {"server", "query", "sharding"},
            "core": {"server", "query"},
            "storage": {"server", "query"},
            "llm": {"server"},  # known coupling risk (Appendix A.3)
        }

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []
        files_to_scan = self._expand_include_scope(file_list)

        for file_path in files_to_scan:
            if file_path.suffix not in [".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx"]:
                continue

            module = self._module_from_path(file_path)
            if not module:
                continue

            lines = self._read_lines(file_path)
            if not lines:
                continue

            self._check_prohibited_includes(file_path, module, lines)
            self._check_sharding_storage_coupling(file_path, module, lines)

        return self.gaps

    def _expand_include_scope(self, file_list: List[Path]) -> List[Path]:
        seen = set()
        merged: List[Path] = []

        for f in file_list:
            try:
                rp = f.resolve()
            except Exception:
                rp = f
            key = str(rp)
            if key in seen:
                continue
            seen.add(key)
            merged.append(f)

        include_candidates = [self.repo_root / "include"]
        if self.repo_root.name.lower() == "src":
            include_candidates.append(self.repo_root.parent / "include")

        for include_dir in include_candidates:
            if not include_dir.exists() or not include_dir.is_dir():
                continue
            for pattern in ["*.h", "*.hpp", "*.hh", "*.hxx"]:
                for header in include_dir.rglob(pattern):
                    key = str(header.resolve())
                    if key in seen:
                        continue
                    seen.add(key)
                    merged.append(header)

        return merged

    def _module_from_path(self, file_path: Path) -> Optional[str]:
        parts = [p.lower() for p in file_path.parts]
        if "src" in parts:
            idx = parts.index("src")
            if idx + 1 < len(parts):
                return parts[idx + 1]
        if "include" in parts:
            idx = parts.index("include")
            if idx + 1 < len(parts):
                return parts[idx + 1]
        return None

    def _read_lines(self, file_path: Path) -> List[str]:
        try:
            with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                return f.readlines()
        except Exception:
            return []

    def _extract_include_module(self, line: str) -> Optional[str]:
        m = re.search(r"#\s*include\s*[<\"]([^>\"]+)[>\"]", line)
        if not m:
            return None

        include_path = m.group(1).strip().replace("\\", "/")

        # Project-style include with module prefix: module/file.h
        if "/" in include_path:
            return include_path.split("/", 1)[0].lower()

        return None

    def _check_prohibited_includes(self, file_path: Path, module: str, lines: List[str]) -> None:
        if module not in self.prohibited_dependencies:
            return

        forbidden = self.prohibited_dependencies[module]

        for idx, line in enumerate(lines, 1):
            if "#include" not in line:
                continue

            dep_module = self._extract_include_module(line)
            if not dep_module:
                continue

            if dep_module in forbidden:
                self.gaps.append(
                    {
                        "file": str(file_path.relative_to(self.repo_root)),
                        "line": idx,
                        "category": "architecture_rules",
                        "severity": "HIGH",
                        "pattern": "layer_dependency_violation",
                        "description": f"Module '{module}' must not depend on '{dep_module}' (layer violation)",
                        "context": line.strip(),
                    }
                )

        # utils special rule: avoid project-module dependencies where possible.
        if module == "utils":
            for idx, line in enumerate(lines, 1):
                if "#include" not in line:
                    continue
                dep_module = self._extract_include_module(line)
                if dep_module and dep_module not in {"utils", "themis"}:
                    self.gaps.append(
                        {
                            "file": str(file_path.relative_to(self.repo_root)),
                            "line": idx,
                            "category": "architecture_rules",
                            "severity": "MEDIUM",
                            "pattern": "foundation_dependency_risk",
                            "description": "utils/ should remain dependency-light and avoid upper-layer module includes",
                            "context": line.strip(),
                        }
                    )

    def _check_sharding_storage_coupling(self, file_path: Path, module: str, lines: List[str]) -> None:
        if module not in {"sharding", "storage"}:
            return

        target = "storage" if module == "sharding" else "sharding"

        for idx, line in enumerate(lines, 1):
            if "#include" not in line:
                continue
            dep_module = self._extract_include_module(line)
            if dep_module == target:
                self.gaps.append(
                    {
                        "file": str(file_path.relative_to(self.repo_root)),
                        "line": idx,
                        "category": "architecture_rules",
                        "severity": "MEDIUM",
                        "pattern": "coupling_risk_sharding_storage",
                        "description": "Potential coupling risk between sharding/ and storage/ (validate no circular dependency)",
                        "context": line.strip(),
                    }
                )
