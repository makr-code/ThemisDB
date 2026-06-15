#!/usr/bin/env python3
"""
Phase 10-4: Themis Design & Error-Handling Rules Scanner

Rule sources:
- research/architecture_decisions/adr_006_plugin_chimera_adapter_architecture.md
- .github/copilot/CODE_STANDARDS.md

Detects:
- duplicated retry logic in Chimera adapters (should be centralized)
- swallowed catch(...) blocks without handling
"""

import re
from pathlib import Path
from typing import Dict, List


class ThemisDesignErrorRulesScan:
    """Scan for design and error-handling rule violations."""

    CATCH_ALL_BLOCK_RE = re.compile(r"catch\s*\(\s*\.\.\.\s*\)\s*\{(.*?)\}", re.DOTALL)

    def __init__(self, repo_root: str = "."):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []

        for file_path in file_list:
            if file_path.suffix not in [".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx"]:
                continue

            text = self._read_text(file_path)
            if not text:
                continue

            self._check_chimera_retry_duplication(file_path, text)
            self._check_swallowed_catch_all(file_path, text)

        return self.gaps

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
                "category": "design_error_rules",
                "severity": severity,
                "pattern": pattern,
                "description": description,
                "context": context.strip()[:180],
            }
        )

    def _check_chimera_retry_duplication(self, file_path: Path, text: str) -> None:
        path_norm = str(file_path).lower().replace("\\", "/")
        if "/chimera/" not in path_norm:
            return

        # Centralized retry is expected in shared/base infrastructure only.
        allowlist_tokens = [
            "chimera_client",
            "database_adapter",
            "adapter_factory",
            "adapter_export",
            "mocks",
            "tests",
        ]
        if any(tok in path_norm for tok in allowlist_tokens):
            return

        lines = text.splitlines()
        for idx, line in enumerate(lines, 1):
            lower = line.lower()
            if any(tok in lower for tok in ["retry", "backoff", "jitter", "sleep_for", "attempt", "max_retries"]):
                self._append(
                    file_path,
                    idx,
                    "MEDIUM",
                    "chimera_retry_duplication",
                    "Chimera retry logic should be centralized (avoid per-adapter retry implementations)",
                    line,
                )

    def _check_swallowed_catch_all(self, file_path: Path, text: str) -> None:
        path_norm = str(file_path).lower().replace("\\", "/")
        critical_tokens = ["/server/", "/api/", "/network/", "/security/", "/auth/", "/query/", "/chimera/"]
        if not any(tok in path_norm for tok in critical_tokens):
            return

        for m in self.CATCH_ALL_BLOCK_RE.finditer(text):
            block = m.group(1) or ""
            block_lower = block.lower()
            start_line = text[: m.start()].count("\n") + 1

            meaningful_lines = [
                ln.strip()
                for ln in block.splitlines()
                if ln.strip() and not ln.strip().startswith("//") and not ln.strip().startswith("/*")
            ]

            has_handling = any(tok in block_lower for tok in [
                "throw",
                "return",
                "log",
                "spdlog",
                "error",
                "status",
                "abort",
                "terminate",
                "fallback",
                "recover",
                "degrade",
            ])

            # Focus on truly suspicious swallow blocks, not large fallback handlers.
            is_short_block = len(meaningful_lines) <= 4
            has_call_like_stmt = any("(" in ln and ")" in ln for ln in meaningful_lines)
            has_assignment = any("=" in ln for ln in meaningful_lines)
            only_trivial = all(
                ln in {";", "continue;", "break;"} or ln.startswith("//")
                for ln in meaningful_lines
            ) if meaningful_lines else True

            if (not has_handling) and is_short_block and (not has_call_like_stmt) and (not has_assignment) and only_trivial:
                self._append(
                    file_path,
                    start_line,
                    "HIGH",
                    "catch_all_swallow",
                    "catch(...) block swallows errors without rethrowing or explicit handling",
                    "catch(...) { ... }",
                )
