#!/usr/bin/env python3
"""
Phase 10-7: Themis Docs Markdown Rules Scanner

Rule sources:
- docs/**/*.md cross-link integrity
- docs module mirrors referencing src/include markdown files

Detects:
- broken markdown file links inside docs/
- stale markdown anchors inside docs/
"""

import re
import subprocess
import os
from pathlib import Path
from typing import Dict, List, Optional, Set


class ThemisDocsMarkdownRulesScan:
    """Scan docs/ markdown files for broken links and stale anchors."""

    MD_LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
    HEADING_RE = re.compile(r"^(#{1,6})\s+(.+?)\s*$", re.MULTILINE)
    DOXYGEN_WARNING_RE = re.compile(r"(?P<file>[^:\n]+):(?P<line>\d+):\s*warning:\s*(?P<msg>.+)", re.IGNORECASE)
    DEFAULT_DOXYGEN_TIMEOUT_SECONDS = 180

    def __init__(self, repo_root: str = ".", run_doxygen: bool = False):
        self.repo_root = Path(repo_root)
        self.run_doxygen = run_doxygen
        self.gaps: List[Dict] = []
        self.doxygen_timeout_seconds = self._resolve_doxygen_timeout_seconds()

    def _resolve_doxygen_timeout_seconds(self) -> int:
        raw = os.environ.get("THEMIS_DOXYGEN_TIMEOUT_SECONDS", "").strip()
        if not raw:
            return self.DEFAULT_DOXYGEN_TIMEOUT_SECONDS
        try:
            parsed = int(raw)
            return parsed if parsed > 0 else self.DEFAULT_DOXYGEN_TIMEOUT_SECONDS
        except Exception:
            return self.DEFAULT_DOXYGEN_TIMEOUT_SECONDS

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []
        docs_root = self.repo_root / "docs"
        if not docs_root.exists() or not docs_root.is_dir():
            return self.gaps

        scoped_modules = self._modules_in_scope(file_list)
        docs_files = self._collect_docs_files(docs_root, scoped_modules)
        anchor_cache: Dict[Path, Set[str]] = {}

        for doc_path in docs_files:
            self._check_doc_links(doc_path, anchor_cache)

        if self.run_doxygen:
            self._run_doxygen_checks(docs_root)

        return self.gaps

    def _modules_in_scope(self, file_list: List[Path]) -> Set[str]:
        scoped: Set[str] = set()
        for file_path in file_list:
            parts = [part.lower() for part in file_path.parts]
            for anchor in ("src", "include"):
                if anchor in parts:
                    idx = parts.index(anchor)
                    if idx + 1 < len(parts):
                        scoped.add(file_path.parts[idx + 1])
        return scoped

    def _collect_docs_files(self, docs_root: Path, scoped_modules: Set[str]) -> List[Path]:
        all_docs = sorted(docs_root.rglob("*.md"))
        if not scoped_modules:
            return all_docs

        selected: List[Path] = []
        markers = {f"src/{module}/" for module in scoped_modules} | {f"include/{module}/" for module in scoped_modules}
        for doc_path in all_docs:
            rel = str(doc_path.relative_to(self.repo_root)).replace("\\", "/").lower()
            if not rel.startswith("docs/"):
                continue
            if any(f"/{module.lower()}/" in rel for module in scoped_modules):
                selected.append(doc_path)
                continue
            try:
                text = doc_path.read_text(encoding="utf-8", errors="ignore").lower()
            except Exception:
                continue
            if any(marker in text for marker in markers):
                selected.append(doc_path)
        return selected

    def _append(self, file_path: Path | str, line: int, severity: str, pattern: str, description: str, context: str) -> None:
        if isinstance(file_path, Path):
            rel_file = str(file_path.relative_to(self.repo_root)).replace("\\", "/")
        else:
            rel_file = file_path.replace("\\", "/")
        self.gaps.append(
            {
                "file": rel_file,
                "line": line,
                "category": "docs_markdown_rules",
                "severity": severity,
                "pattern": pattern,
                "description": description,
                "context": context[:180],
            }
        )

    def _resolve_doxygen_config(self) -> Optional[Path]:
        preferred = self.repo_root / "Doxyfile.audit"
        fallback = self.repo_root / "Doxyfile"
        if preferred.exists() and preferred.is_file():
            return preferred
        if fallback.exists() and fallback.is_file():
            return fallback
        return None

    def _read_doxygen_setting(self, config: Path, key: str) -> Optional[str]:
        pattern = re.compile(rf"^\s*{re.escape(key)}\s*=\s*(.*?)\s*$", re.IGNORECASE)
        try:
            text = config.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            return None
        for raw_line in text.splitlines():
            line = raw_line.split("#", 1)[0].strip()
            if not line:
                continue
            match = pattern.match(line)
            if match:
                return match.group(1).strip().strip('"')
        return None

    def _resolve_xml_index_path(self, config: Path) -> Path:
        output_dir = self._read_doxygen_setting(config, "OUTPUT_DIRECTORY") or "build/doxygen"
        xml_output = self._read_doxygen_setting(config, "XML_OUTPUT") or "xml"
        return (self.repo_root / output_dir / xml_output / "index.xml").resolve()

    def _normalize_doxygen_file(self, raw_file: str) -> str:
        path = Path(raw_file)
        if not path.is_absolute():
            path = (self.repo_root / path).resolve()
        try:
            return str(path.relative_to(self.repo_root)).replace("\\", "/")
        except Exception:
            return "docs"

    def _run_doxygen_checks(self, docs_root: Path) -> None:
        config = self._resolve_doxygen_config()
        if config is None:
            self._append(
                "docs",
                1,
                "LOW",
                "docs_doxygen_config_missing",
                "Doxygen check requested but no Doxyfile.audit/Doxyfile found",
                "Create Doxyfile.audit or Doxyfile at repository root",
            )
            return

        if config.name != "Doxyfile.audit":
            self._append(
                "docs",
                1,
                "LOW",
                "docs_doxygen_config_fallback",
                "Using fallback Doxyfile; XML-first scanner path prefers Doxyfile.audit",
                str(config.name),
            )

        generate_xml = (self._read_doxygen_setting(config, "GENERATE_XML") or "").strip().upper()
        if generate_xml != "YES":
            self._append(
                "docs",
                1,
                "HIGH",
                "docs_doxygen_xml_disabled",
                "Doxygen XML output is disabled but XML-first processing is required",
                f"{config.name}: GENERATE_XML={generate_xml or 'unset'}",
            )
            return

        try:
            proc = subprocess.run(
                ["doxygen", str(config)],
                cwd=str(self.repo_root),
                capture_output=True,
                text=True,
                timeout=self.doxygen_timeout_seconds,
            )
        except FileNotFoundError:
            self._append(
                str(docs_root.relative_to(self.repo_root)).replace("\\", "/"),
                1,
                "LOW",
                "docs_doxygen_unavailable",
                "Doxygen executable not found while docs_doxygen is enabled",
                "Install doxygen or run without --docs-doxygen",
            )
            return
        except subprocess.TimeoutExpired:
            self._append(
                str(docs_root.relative_to(self.repo_root)).replace("\\", "/"),
                1,
                "MEDIUM",
                "docs_doxygen_timeout",
                "Doxygen run timed out",
                f"{config.name} (timeout={self.doxygen_timeout_seconds}s)",
            )
            return

        output = (proc.stdout or "") + "\n" + (proc.stderr or "")

        xml_index = self._resolve_xml_index_path(config)
        if not xml_index.exists() or not xml_index.is_file():
            rel = "build/doxygen/xml/index.xml"
            try:
                rel = str(xml_index.relative_to(self.repo_root)).replace("\\", "/")
            except Exception:
                pass
            self._append(
                rel,
                1,
                "HIGH",
                "docs_doxygen_xml_missing",
                "Doxygen completed but XML index is missing; downstream XML processing cannot continue",
                f"config={config.name}",
            )

        for match in self.DOXYGEN_WARNING_RE.finditer(output):
            rel_file = self._normalize_doxygen_file(match.group("file").strip())
            line = int(match.group("line")) if match.group("line").isdigit() else 1
            message = match.group("msg").strip()
            self._append(
                rel_file,
                line,
                "LOW",
                "doxygen_doc_warning",
                f"Doxygen warning: {message}",
                match.group(0).strip(),
            )

    def _line_for_offset(self, text: str, offset: int) -> int:
        return text[:offset].count("\n") + 1

    def _slugify_heading(self, heading: str) -> str:
        slug = heading.strip().lower()
        slug = re.sub(r"[`*_~]", "", slug)
        slug = re.sub(r"[^a-z0-9\-\s]", "", slug)
        slug = re.sub(r"\s+", "-", slug)
        slug = re.sub(r"-+", "-", slug)
        return slug.strip("-")

    def _anchors_for_doc(self, target: Path, cache: Dict[Path, Set[str]]) -> Set[str]:
        if target in cache:
            return cache[target]
        try:
            text = target.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            cache[target] = set()
            return cache[target]

        anchors = {self._slugify_heading(match.group(2)) for match in self.HEADING_RE.finditer(text)}
        cache[target] = {anchor for anchor in anchors if anchor}
        return cache[target]

    def _resolve_target(self, doc_path: Path, raw_target: str) -> Optional[Path]:
        if not raw_target or raw_target.startswith(("http://", "https://", "mailto:", "#")):
            return None

        target = raw_target.split("#", 1)[0].strip()
        if not target:
            return None

        candidate = (doc_path.parent / target).resolve()
        try:
            candidate.relative_to(self.repo_root)
        except Exception:
            return None
        return candidate

    def _check_doc_links(self, doc_path: Path, anchor_cache: Dict[Path, Set[str]]) -> None:
        try:
            text = doc_path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            return

        for match in self.MD_LINK_RE.finditer(text):
            raw_target = match.group(1).strip()
            line = self._line_for_offset(text, match.start())
            target_path = self._resolve_target(doc_path, raw_target)

            if raw_target.startswith(("http://", "https://", "mailto:", "#")):
                continue

            if target_path is None or not target_path.exists():
                self._append(
                    doc_path,
                    line,
                    "MEDIUM",
                    "docs_broken_markdown_link",
                    f"Markdown link target '{raw_target}' could not be resolved from docs file",
                    match.group(0),
                )
                continue

            if "#" not in raw_target or target_path.suffix.lower() != ".md":
                continue

            anchor = raw_target.split("#", 1)[1].strip().lower()
            if not anchor:
                continue
            anchors = self._anchors_for_doc(target_path, anchor_cache)
            if anchor not in anchors:
                self._append(
                    doc_path,
                    line,
                    "LOW",
                    "docs_stale_markdown_anchor",
                    f"Markdown anchor '#{anchor}' was not found in '{str(target_path.relative_to(self.repo_root)).replace('\\', '/')}'",
                    match.group(0),
                )