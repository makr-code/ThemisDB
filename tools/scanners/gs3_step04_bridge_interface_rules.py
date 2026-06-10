#!/usr/bin/env python3
"""
Phase 10-3: Themis Bridge & Interface Rules Scanner

Rule sources:
- audit/docs/Audit/ARCHITECTURE_AUDIT.md (Interface catalog + I-prefix convention)
- research/architecture_decisions/adr_006_plugin_chimera_adapter_architecture.md

Detects interface naming and bridge/adapter contract issues.
"""

import re
from pathlib import Path
from typing import Dict, List


class ThemisBridgeInterfaceRulesScan:
    """Scan for bridge/interface rule violations."""

    CLASS_RE = re.compile(
        r"class\s+([A-Za-z_]\w*)\s*(?::\s*([^\{]+))?\s*\{(.*?)\};",
        re.DOTALL,
    )
    ALLOWED_ABSTRACT_SUFFIXES = (
        "Provider",
        "Service",
        "Engine",
        "Handler",
        "Client",
        "Server",
        "Iterator",
        "Resolver",
        "PolicyEngine",
        "Policy",
        "Module",
        "Backend",
        "Runtime",
        "Session",
        "Cache",
        "Estimator",
        "Tracker",
        "Plugin",
        "Transport",
        "Factory",
    )

    def __init__(self, repo_root: str = "."):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []
        files_to_scan = self._expand_header_scope(file_list)

        for file_path in files_to_scan:
            if file_path.suffix not in [".h", ".hpp", ".hh", ".hxx"]:
                continue

            text = self._read_text(file_path)
            if not text:
                continue

            self._check_class_rules(file_path, text)

        return self.gaps

    def _expand_header_scope(self, file_list: List[Path]) -> List[Path]:
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

    def _read_text(self, file_path: Path) -> str:
        try:
            return file_path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            return ""

    def _append(self, file_path: Path, line: int, severity: str, pattern: str, description: str, context: str) -> None:
        self.gaps.append(
            {
                "file": str(file_path.relative_to(self.repo_root)),
                "line": line,
                "category": "bridge_interface_rules",
                "severity": severity,
                "pattern": pattern,
                "description": description,
                "context": context.strip()[:180],
            }
        )

    def _check_class_rules(self, file_path: Path, text: str) -> None:
        lower_path = str(file_path).lower().replace("\\", "/")
        is_chimera = "/chimera/" in lower_path

        for m in self.CLASS_RE.finditer(text):
            class_name = m.group(1)
            base_spec = (m.group(2) or "").strip()
            body = m.group(3) or ""
            start_line = text[: m.start()].count("\n") + 1

            # Ignore lower-case or macro-expanded pseudo names.
            if not class_name or not class_name[0].isupper():
                continue

            pure_virtual_count = len(re.findall(r"=\s*0\s*;", body))
            has_virtual = "virtual" in body
            is_abstract = pure_virtual_count > 0
            context_line = m.group(0).splitlines()[0] if m.group(0).splitlines() else f"class {class_name}"

            # Rule 1: I-prefix classes should be abstract interfaces.
            if class_name.startswith("I") and len(class_name) > 1 and class_name[1].isupper():
                if not is_abstract:
                    self._append(
                        file_path,
                        start_line,
                        "MEDIUM",
                        "i_prefix_non_abstract",
                        f"Interface-style class '{class_name}' should be abstract (pure virtual contract)",
                        context_line,
                    )

            # Rule 2: abstract interface-like classes should follow I-prefix convention.
            if is_abstract and has_virtual and not (class_name.startswith("I") and len(class_name) > 1 and class_name[1].isupper()):
                # Ignore common non-interface base naming patterns.
                if not class_name.endswith(("Base", "Impl", "Mock", "Stub", "Test", *self.ALLOWED_ABSTRACT_SUFFIXES)):
                    self._append(
                        file_path,
                        start_line,
                        "LOW",
                        "abstract_without_i_prefix",
                        f"Abstract contract '{class_name}' does not follow I-prefix interface convention",
                        context_line,
                    )

            # Rule 3: Bridge classes should bind to explicit interface contracts.
            if class_name.endswith("Bridge"):
                if "I" not in base_spec:
                    self._append(
                        file_path,
                        start_line,
                        "MEDIUM",
                        "bridge_without_interface_base",
                        f"Bridge '{class_name}' should inherit from at least one explicit interface (I*)",
                        context_line,
                    )

            # Rule 4: Chimera adapters should implement documented adapter interfaces.
            if is_chimera and class_name.endswith("Adapter"):
                required = ["IRelationalAdapter", "IDocumentAdapter", "IVectorAdapter", "IGraphAdapter", "IDatabaseAdapter"]
                if not any(r in base_spec for r in required):
                    self._append(
                        file_path,
                        start_line,
                        "HIGH",
                        "chimera_adapter_missing_interface",
                        f"Chimera adapter '{class_name}' should implement one of the required adapter interfaces",
                        context_line,
                    )
