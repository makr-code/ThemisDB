#!/usr/bin/env python3
"""
Phase 10-5: Themis Module Governance Rules Scanner

Rule sources:
- src/*/PRODUCTION_REQUIREMENTS.md (module-level mandatory requirements)
- module documentation conventions used across ThemisDB
- research/architecture_decisions (ADR references)

Detects:
- Missing module governance docs in src/<module>/
- Missing ADR references in architecture docs where adapters/bridges are used
"""

from pathlib import Path
from typing import Dict, List, Set


class ThemisModuleGovernanceRulesScan:
    """Scan module-level governance and documentation contract rules."""

    REQUIRED_MODULE_DOCS = [
        "PRODUCTION_REQUIREMENTS.md",
        "README.md",
        "ARCHITECTURE.md",
    ]

    ADR_REFERENCE_REQUIRED_MODULES = {
        "chimera": ["adr_006"],
        "server": ["adr_003", "adr_007", "adr_008"],
        "storage": ["adr_002"],
        "index": ["adr_001"],
    }

    def __init__(self, repo_root: str = "."):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    def scan_files(self, _file_list: List[Path]) -> List[Dict]:
        self.gaps = []

        src_root = self.repo_root / "src"
        if not src_root.exists() or not src_root.is_dir():
            return self.gaps

        module_dirs = [d for d in src_root.iterdir() if d.is_dir()]
        module_names: Set[str] = {d.name.lower() for d in module_dirs}

        for module_dir in module_dirs:
            self._check_module_docs(module_dir)
            self._check_adr_references(module_dir)

        self._check_bottom_layer_presence(module_names)

        return self.gaps

    def _append(self, file_rel: str, line: int, severity: str, pattern: str, description: str, context: str) -> None:
        self.gaps.append(
            {
                "file": file_rel,
                "line": line,
                "category": "module_governance_rules",
                "severity": severity,
                "pattern": pattern,
                "description": description,
                "context": context[:180],
            }
        )

    def _check_module_docs(self, module_dir: Path) -> None:
        module_name = module_dir.name
        for required in self.REQUIRED_MODULE_DOCS:
            p = module_dir / required
            if not p.exists():
                self._append(
                    file_rel=str(module_dir.relative_to(self.repo_root)).replace("\\", "/"),
                    line=1,
                    severity="MEDIUM",
                    pattern="missing_module_doc",
                    description=f"Module '{module_name}' missing required governance doc '{required}'",
                    context=f"Expected file: src/{module_name}/{required}",
                )

    def _check_adr_references(self, module_dir: Path) -> None:
        module_name = module_dir.name.lower()
        required_adrs = self.ADR_REFERENCE_REQUIRED_MODULES.get(module_name)
        if not required_adrs:
            return

        arch_doc = module_dir / "ARCHITECTURE.md"
        if not arch_doc.exists():
            return

        try:
            content = arch_doc.read_text(encoding="utf-8", errors="ignore").lower()
        except Exception:
            return

        missing = [adr for adr in required_adrs if adr not in content]
        if missing:
            self._append(
                file_rel=str(arch_doc.relative_to(self.repo_root)).replace("\\", "/"),
                line=1,
                severity="LOW",
                pattern="missing_adr_reference",
                description=f"Architecture doc missing ADR references: {', '.join(missing)}",
                context="Add explicit ADR links/references for module-critical design decisions",
            )

    def _check_bottom_layer_presence(self, module_names: Set[str]) -> None:
        bottom_layers = {"config", "core", "base", "utils", "themis"}
        missing = sorted(bottom_layers - module_names)
        if missing:
            self._append(
                file_rel="src",
                line=1,
                severity="HIGH",
                pattern="missing_foundation_module",
                description=f"Missing expected foundation modules: {', '.join(missing)}",
                context="Bottom-layer modules are required by architecture governance",
            )
