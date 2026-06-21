#!/usr/bin/env python3
"""
Phase 10-6: Themis Documentation Freshness Rules Scanner

Rule sources:
- src/<module>/*.md cross-link conventions
- explicit code comments referencing module docs and section anchors

Detects:
- stale code->doc section references where the referenced section no longer exists
- broken module doc linksets in core module docs
"""

import re
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple


class ThemisDocFreshnessRulesScan:
    """Scan for code and module-documentation drift."""

    CORE_DOCS = {
        "README.md",
        "ARCHITECTURE.md",
        "ROADMAP.md",
        "FUTURE_ENHANCEMENTS.md",
        "PRODUCTION_REQUIREMENTS.md",
    }
    LINK_COMMENT_RE = re.compile(r"<!--\s*Links:\s*(.*?)\s*-->", re.IGNORECASE)
    SECTION_REF_RE = re.compile(
        r"(?P<doc>(?:src/[A-Za-z0-9_\-]+/)?(?:README|ARCHITECTURE|ROADMAP|FUTURE_ENHANCEMENTS|PRODUCTION_REQUIREMENTS)\.md)\s*§\s*(?P<section>[^\n]+)",
        re.IGNORECASE,
    )

    def __init__(self, repo_root: str = "."):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []
        src_root = self.repo_root / "src"
        if not src_root.exists() or not src_root.is_dir():
            return self.gaps

        module_dirs = [d for d in src_root.iterdir() if d.is_dir()]
        module_names = {d.name for d in module_dirs}
        scoped_modules = self._modules_in_scope(file_list, module_names)
        doc_cache: Dict[Path, str] = {}

        for module_dir in module_dirs:
            if scoped_modules and module_dir.name not in scoped_modules:
                continue
            self._check_module_doc_linksets(module_dir, doc_cache)

        for file_path in file_list:
            if file_path.suffix not in [".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx"]:
                continue
            self._check_code_doc_references(file_path, module_names, doc_cache)

        return self.gaps

    def _modules_in_scope(self, file_list: List[Path], module_names: Set[str]) -> Set[str]:
        scoped: Set[str] = set()
        for file_path in file_list:
            parts = list(file_path.parts)
            parts_lower = [part.lower() for part in parts]
            for anchor in ("src", "include"):
                if anchor in parts_lower:
                    idx = parts_lower.index(anchor)
                    if idx + 1 < len(parts):
                        module_name = parts[idx + 1]
                        if module_name in module_names:
                            scoped.add(module_name)
        return scoped

    def _append(self, file_rel: str, line: int, severity: str, pattern: str, description: str, context: str) -> None:
        self.gaps.append(
            {
                "file": file_rel,
                "line": line,
                "category": "doc_freshness_rules",
                "severity": severity,
                "pattern": pattern,
                "description": description,
                "context": context[:180],
            }
        )

    def _read_text(self, path: Path, cache: Dict[Path, str]) -> str:
        if path in cache:
            return cache[path]
        try:
            cache[path] = path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            cache[path] = ""
        return cache[path]

    def _normalize_rel(self, path: Path) -> str:
        return str(path.relative_to(self.repo_root)).replace("\\", "/")

    def _extract_doc_links(self, text: str) -> Set[str]:
        match = self.LINK_COMMENT_RE.search(text)
        if not match:
            return set()
        raw = match.group(1)
        return {token.strip() for token in raw.split("·") if token.strip()}

    def _expected_links_for_doc(self, doc_name: str, module_dir: Path) -> Set[str]:
        expected = {
            "README.md": {"ARCHITECTURE.md", "ROADMAP.md", "FUTURE_ENHANCEMENTS.md"},
            "ARCHITECTURE.md": {"README.md", "ROADMAP.md", "FUTURE_ENHANCEMENTS.md"},
            "ROADMAP.md": {"README.md", "ARCHITECTURE.md", "FUTURE_ENHANCEMENTS.md"},
            "FUTURE_ENHANCEMENTS.md": {"README.md", "ROADMAP.md", "ARCHITECTURE.md"},
            "PRODUCTION_REQUIREMENTS.md": {"README.md", "ROADMAP.md", "FUTURE_ENHANCEMENTS.md"},
        }.get(doc_name, set())

        return {name for name in expected if (module_dir / name).exists()}

    def _check_module_doc_linksets(self, module_dir: Path, cache: Dict[Path, str]) -> None:
        for doc_name in self.CORE_DOCS:
            doc_path = module_dir / doc_name
            if not doc_path.exists():
                continue

            text = self._read_text(doc_path, cache)
            if not text:
                continue

            linked = self._extract_doc_links(text)
            expected = self._expected_links_for_doc(doc_name, module_dir)
            missing = sorted(expected - linked)
            if missing:
                self._append(
                    self._normalize_rel(doc_path),
                    1,
                    "LOW",
                    "module_doc_linkset_drift",
                    f"Module doc '{doc_name}' is missing expected cross-links: {', '.join(missing)}",
                    "Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set",
                )

            for target in linked:
                if target.endswith(".md") and "/" not in target and not (module_dir / target).exists():
                    self._append(
                        self._normalize_rel(doc_path),
                        1,
                        "MEDIUM",
                        "module_doc_broken_link",
                        f"Module doc '{doc_name}' links to missing peer doc '{target}'",
                        target,
                    )

    def _resolve_doc_reference(self, current_file: Path, raw_doc: str, module_names: Set[str]) -> Optional[Path]:
        norm = raw_doc.replace("\\", "/")
        if norm.startswith("src/"):
            return self.repo_root / norm

        parts = [part.lower() for part in current_file.parts]
        for anchor in ("src", "include"):
            if anchor in parts:
                idx = parts.index(anchor)
                if idx + 1 < len(parts):
                    module_name = current_file.parts[idx + 1]
                    if module_name in module_names:
                        return self.repo_root / "src" / module_name / norm
        return None

    def _clean_section_label(self, section: str) -> str:
        cleaned = section.strip().strip('"').strip("'")
        cleaned = cleaned.split(".")[0].strip() if cleaned.count(" ") > 2 else cleaned
        return cleaned

    def _find_line_number(self, text: str, offset: int) -> int:
        return text[:offset].count("\n") + 1

    def _check_code_doc_references(self, file_path: Path, module_names: Set[str], cache: Dict[Path, str]) -> None:
        try:
            text = file_path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            return

        for match in self.SECTION_REF_RE.finditer(text):
            raw_doc = match.group("doc")
            raw_section = match.group("section")
            doc_path = self._resolve_doc_reference(file_path, raw_doc, module_names)
            line = self._find_line_number(text, match.start())

            if doc_path is None or not doc_path.exists():
                self._append(
                    self._normalize_rel(file_path),
                    line,
                    "MEDIUM",
                    "missing_referenced_module_doc",
                    f"Code comment references missing module doc '{raw_doc}'",
                    match.group(0).strip(),
                )
                continue

            section = self._clean_section_label(raw_section)
            doc_text = self._read_text(doc_path, cache)
            if not section or section.lower() not in doc_text.lower():
                self._append(
                    self._normalize_rel(file_path),
                    line,
                    "MEDIUM",
                    "stale_doc_section_reference",
                    f"Code comment references section '{section}' that was not found in '{self._normalize_rel(doc_path)}'",
                    match.group(0).strip(),
                )