#!/usr/bin/env python3
"""
Gap Scanner Step 01 — LLM Prompt Injection & Hardcoding Detection

Detects:
- Hardcoded LLM prompts (should be loaded from YAML/config)
- String concatenation into prompts (injection vulnerability)
- User input directly embedded in prompts without delimiter isolation
- Prompt templating without proper escaping
- SQL-like prompt construction patterns

ThemisDB LLM Standard:
- ALL system prompts MUST be loaded from YAML config (src/llm/config/prompts/)
- Prompts MUST use structured templating ({{variable}}) with explicit delimiters
- User input MUST be wrapped in delimiters (e.g., <USER_INPUT>...</USER_INPUT>)
- No string concatenation ("prompt = " + user_input) allowed
- Prompt composition MUST be validated through prompt schema

Purpose:
LLM prompt injection is a critical vulnerability. AI systems often hardcode prompts
or directly concatenate user input. This scanner catches both patterns.
"""

import re
import sys
from pathlib import Path
from typing import List

sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority, SeverityLevel


class LlmPromptInjectionScanner(BaseGapScanner):
    """Phase 1: LLM Prompt Injection & Hardcoding Detection (LLM module only)"""

    PRIORITY = ScannerPriority.BASELINE
    ENABLED = True
    MAX_RUNTIME_SECONDS = 60

    def __init__(self):
        """Initialize LLM Prompt Injection Scanner."""
        super().__init__("LLM Prompt Injection Scanner", "1.0")

        # Patterns indicating hardcoded prompts
        self.hardcoded_prompt_patterns = [
            r'"\s*(You are|You will|Your role|System:|Assistant:)',
            r'"\s*(Analyze|Process|Generate|Create|Transform|Extract)',
            r'prompt\s*=\s*"[A-Z].*[:.?!]"',  # Full sentences in code
            r'system_message\s*=\s*R?"[^"]{50,}"',  # Long string literal
        ]

        # Patterns for user input concatenation
        self.injection_patterns = [
            (r'prompt\s*[\+\=]\s*user', 'User input directly concatenated to prompt'),
            (r'message\s*[\+\=]\s*\w*input', 'Input concatenated to message'),
            (r'".*"\s*\+\s*\w+\s*\+\s*".*"', 'String interpolation without safe delimiters'),
            (r'fmt::format.*\{\}\s*.*user', 'User input in format template'),
            (r'sprintf.*%s.*\w*input', 'User input in sprintf'),
            (r'snprintf.*%s.*\w*input', 'User input in snprintf'),
        ]

        # Patterns for SQL-like prompt building (dangerous)
        self.sql_like_patterns = [
            r'WHERE.*query.*=.*["\'].*["\'].*\+',
            r'SELECT.*FROM.*WHERE.*\+',
            r'prompt.*LIKE.*\+',
        ]

        # Patterns indicating proper loading from config
        self.safe_patterns = [
            r'load_prompt',
            r'from_config',
            r'yaml::load',
            r'config\[.*prompt',
            r'prompt_db\.get',
            r'PromptStore::',
        ]

    def _is_llm_module_file(self, file_path: Path) -> bool:
        """Check if file is in LLM module."""
        path_str = str(file_path).lower()
        return any(token in path_str for token in [
            'src/llm', 'src/ai_', 'include/llm',
            'modules/llm', 'modules/ai_',
        ])

    def _has_safe_loading(self, context: str) -> bool:
        """Check if context shows prompt is loaded safely from config."""
        return any(re.search(pattern, context, re.IGNORECASE) for pattern in self.safe_patterns)

    def _scan_file(self, file_path: Path) -> List[Gap]:
        """Scan single file for LLM prompt injection issues."""
        gaps = []

        # Only scan LLM module files
        if not self._is_llm_module_file(file_path):
            return gaps

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
                content = ''.join(lines)
        except Exception:
            return gaps

        for line_no, line in enumerate(lines, 1):
            stripped = line.strip()

            # Skip comments and pure declarations
            if stripped.startswith('//') or stripped.startswith('/*'):
                continue

            # PATTERN 1: Hardcoded long prompts in code
            for pattern in self.hardcoded_prompt_patterns:
                if re.search(pattern, line, re.IGNORECASE):
                    # Check if this is actually loading from config (false positive check)
                    context_before = ''.join(lines[max(0, line_no - 5):line_no])
                    if self._has_safe_loading(context_before):
                        continue  # Safe loading, skip

                    # Check if it's a string literal continuation
                    if '\\' in line:
                        # Multi-line string
                        full_match = ''.join(lines[line_no - 1:min(line_no + 10, len(lines))])
                        if len(full_match) > 200 and any(end in full_match for end in ['.load', '.get', 'read_file']):
                            continue  # Likely config loading

                    self._add_gap(
                        gaps,
                        str(file_path.relative_to(file_path.parents[2])),
                        line_no,
                        "hardcoded_llm_prompt",
                        SeverityLevel.CRITICAL.value,
                        0.82,
                        "Hardcoded LLM prompt detected; should be loaded from YAML config",
                        "Move prompt to src/llm/config/prompts/*.yaml; load with PromptStore",
                        line.rstrip()[:120]
                    )

            # PATTERN 2: User input concatenation into prompts
            for pattern, description in self.injection_patterns:
                if re.search(pattern, line, re.IGNORECASE):
                    # Check if it's in a safe delimiter context
                    context_window = ''.join(lines[max(0, line_no - 2):min(line_no + 2, len(lines))])
                    if any(delim in context_window for delim in ['<USER_INPUT>', '[USER]', '{{', '<%']):
                        continue  # Safe delimiter usage

                    self._add_gap(
                        gaps,
                        str(file_path.relative_to(file_path.parents[2])),
                        line_no,
                        "llm_prompt_injection_risk",
                        SeverityLevel.CRITICAL.value,
                        0.88,
                        f"LLM prompt injection risk: {description}",
                        "Use safe delimiter wrapping: <USER_INPUT>{{}}...</USER_INPUT>; validate inputs",
                        line.rstrip()[:120]
                    )

            # PATTERN 3: SQL-like prompt building (high FP but worth catching)
            for pattern in self.sql_like_patterns:
                if re.search(pattern, line, re.IGNORECASE):
                    self._add_gap(
                        gaps,
                        str(file_path.relative_to(file_path.parents[2])),
                        line_no,
                        "sql_like_prompt_building",
                        SeverityLevel.HIGH.value,
                        0.68,
                        "SQL-like prompt construction detected (high injection risk)",
                        "Use structured prompt template system; avoid string concatenation",
                        line.rstrip()[:120]
                    )

            # PATTERN 4: Missing user input delimiter/escaping
            # Look for "user_" or "input" variables in prompt contexts
            if any(term in line.lower() for term in ['prompt', 'message', 'query']) and any(term in line.lower() for term in ['user', 'input', 'request']):
                if not any(delim in line for delim in ['<USER_', '[USER', '{{', '<%', 'escape', 'sanitize']):
                    # Could be injection risk
                    self._add_gap(
                        gaps,
                        str(file_path.relative_to(file_path.parents[2])),
                        line_no,
                        "missing_input_delimiter",
                        SeverityLevel.HIGH.value,
                        0.65,
                        "User input in prompt context without explicit delimiter wrapping",
                        "Wrap user input with delimiters: <USER_INPUT>...use_input...</USER_INPUT>",
                        line.rstrip()[:120]
                    )

        return gaps

    def scan(self, source_dir: str) -> List[Gap]:
        """Main scanner entry point."""
        gaps = []
        source_path = Path(source_dir).resolve()

        excluded = {'.git', 'build', 'vcpkg', 'vcpkg_installed', 'external', 'third_party', '.venv', 'examples'}

        # Scan only LLM-related directories
        llm_patterns = [
            source_path / 'src' / 'llm',
            source_path / 'src' / 'ai_orchestrator',
            source_path / 'include' / 'llm',
            source_path / 'include' / 'ai_orchestrator',
        ]

        for cpp_file in source_path.rglob('*.cpp'):
            if any(excluded_dir in cpp_file.parts for excluded_dir in excluded):
                continue
            gaps.extend(self._scan_file(cpp_file))

        for hpp_file in source_path.rglob('*.hpp'):
            if any(excluded_dir in hpp_file.parts for excluded_dir in excluded):
                continue
            gaps.extend(self._scan_file(hpp_file))

        for h_file in source_path.rglob('*.h'):
            if any(excluded_dir in h_file.parts for excluded_dir in excluded):
                continue
            gaps.extend(self._scan_file(h_file))

        return gaps
