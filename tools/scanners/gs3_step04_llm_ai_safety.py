#!/usr/bin/env python3
"""
Phase 9-3: LLM/AI Safety & Model Integrity Scanner

CWE-89 (Prompt Injection), CWE-95 (Code Injection), CWE-434 (Unrestricted Upload)

Detects:
- Prompt injection in user input
- Model weights not validated (hash/signature)
- Untrusted model URLs
- Unsafe deserialization of model
- No input normalization before LLM
- Output not validated before use
- Memory exhaustion on large prompts
- Tokenizer bias not documented
- Model poisoning risk (no checksum)
- Hallucination not bounded
- Token length not enforced
"""

import re
from pathlib import Path
from typing import List, Dict


class LLMAISafetyScan:
    """Scan for LLM/AI safety and model integrity issues"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
        self.reported_by_location = {}

    def _mark_reported(self, rel_file: str, line_no: int, pattern: str) -> None:
        key = (rel_file, line_no)
        if key not in self.reported_by_location:
            self.reported_by_location[key] = set()
        self.reported_by_location[key].add(pattern)

    def _is_reported(self, rel_file: str, line_no: int, pattern: str) -> bool:
        return pattern in self.reported_by_location.get((rel_file, line_no), set())

    def _append_gap(self, rel_file: str, line_no: int, severity: str, pattern: str, description: str, context: str) -> None:
        if self._is_reported(rel_file, line_no, pattern):
            return
        self.gaps.append({
            'file': rel_file,
            'line': line_no,
            'category': 'llm_ai_safety',
            'severity': severity,
            'pattern': pattern,
            'description': description,
            'context': context,
        })
        self._mark_reported(rel_file, line_no, pattern)

    @staticmethod
    def _is_comment_line(line: str) -> bool:
        stripped = line.strip()
        return stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*')

    @staticmethod
    def _strip_string_literals(line: str) -> str:
        """Remove string literals to avoid matching load-model tokens inside log messages."""
        return re.sub(r'"(?:\\.|[^"\\])*"', '""', line)
    
    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files for LLM/AI safety issues"""
        
        for file_path in file_list:
            if not file_path.suffix in ['.cpp', '.cc', '.py', '.h', '.hpp']:
                continue
            
            # Check if file is LLM/AI-related
            if not self._is_llm_file(str(file_path)):
                continue
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = content.split('\n')
            except Exception:
                continue
            
            # Scan patterns
            self._check_prompt_injection(file_path, lines)
            self._check_model_validation(file_path, lines)
            self._check_input_sanitization(file_path, lines)
            self._check_output_validation(file_path, lines)
            self._check_resource_limits(file_path, lines)
        
        return self.gaps
    
    def _is_llm_file(self, file_path: str) -> bool:
        """Check if file is LLM/AI-related"""
        keywords = ['llm', 'model', 'prompt', 'embeddings', 'rag', 'agent', 'ai', 'inference',
                   'ollama', 'llamacpp', 'transformers', 'pytorch']
        return any(kw in file_path.lower() for kw in keywords)
    
    def _check_prompt_injection(self, file_path: Path, lines: List[str]):
        """Find prompt injection vulnerabilities"""
        rel_file = str(file_path.relative_to(self.repo_root))
        
        for idx, line in enumerate(lines, 1):
            # Look for prompt construction from user input.
            if re.search(r'(prompt|query|instruction)\s*[=\+].*(user|input|request)', line, re.IGNORECASE):
                # Check if sanitized
                prev_lines = '\n'.join(lines[max(0, idx-12):idx])
                
                if not re.search(r'(sanitize|escape|validate|verify)', prev_lines, re.IGNORECASE):
                    self._append_gap(
                        rel_file,
                        idx,
                        'CRITICAL',
                        'prompt_injection',
                        'User input in prompt without sanitization (injection risk)',
                        line.strip(),
                    )
    
    def _check_model_validation(self, file_path: Path, lines: List[str]):
        """Find missing model integrity validation"""
        rel_file = str(file_path.relative_to(self.repo_root))
        
        for idx, line in enumerate(lines, 1):
            if self._is_comment_line(line):
                continue

            code_line = self._strip_string_literals(line)

            # Look for model loading
            if re.search(r'\b(?:load_model|loadModel|LoadModel|load\w*weights|deserialize)\b\s*\(', code_line, re.IGNORECASE):
                # Skip declarations/comments and lines that are integrity APIs themselves.
                stripped = line.strip()
                if stripped.endswith(';'):
                    continue
                if re.search(r'(checksum|hash|signature|verify)', code_line, re.IGNORECASE):
                    continue

                # Check for checksum/signature validation
                next_lines = '\n'.join(lines[idx:min(idx+120, len(lines))])
                
                if not re.search(r'(checksum|hash|signature|verify|validate)', next_lines, re.IGNORECASE):
                    self._append_gap(
                        rel_file,
                        idx,
                        'CRITICAL',
                        'model_integrity_gap',
                        'Model loading without integrity verification (poisoning risk)',
                        line.strip(),
                    )
            
            # Look for remote model URLs
            if re.search(r'(http|https|url|URI).*model', line, re.IGNORECASE):
                if 'https' not in line and 'http' in line:
                    self._append_gap(
                        rel_file,
                        idx,
                        'HIGH',
                        'insecure_model_url',
                        'Model downloaded over insecure HTTP',
                        line.strip(),
                    )
    
    def _check_input_sanitization(self, file_path: Path, lines: List[str]):
        """Find unsanitized input to LLM"""
        rel_file = str(file_path.relative_to(self.repo_root))
        
        for idx, line in enumerate(lines, 1):
            # Look for inference/generation calls
            if re.search(r'(generate|infer|predict|complete)\s*\([^)]*(user|input|request)', line, re.IGNORECASE):
                # If prompt injection already reported on same line, avoid duplicate signal.
                if self._is_reported(rel_file, idx, 'prompt_injection'):
                    continue

                # Check for normalization
                prev_lines = '\n'.join(lines[max(0, idx-12):idx])
                
                if not re.search(r'(normalize|sanitize|clean|strip)', prev_lines, re.IGNORECASE):
                    self._append_gap(
                        rel_file,
                        idx,
                        'HIGH',
                        'unsanitized_llm_input',
                        'User input passed to LLM without normalization/sanitization',
                        line.strip(),
                    )
    
    def _check_output_validation(self, file_path: Path, lines: List[str]):
        """Find LLM output used without validation"""
        rel_file = str(file_path.relative_to(self.repo_root))
        
        for idx, line in enumerate(lines, 1):
            # Look for LLM output usage
            if re.search(r'(output|result|generated|response)\s*=\s*.*(generate|infer|predict|complete)', line, re.IGNORECASE):
                # Check if validated before use
                next_lines = '\n'.join(lines[idx:min(idx+18, len(lines))])
                
                if not re.search(r'(validate|verify|check|assert)', next_lines, re.IGNORECASE):
                    self._append_gap(
                        rel_file,
                        idx,
                        'HIGH',
                        'unvalidated_llm_output',
                        'LLM output used without validation (hallucination/bias risk)',
                        line.strip(),
                    )
    
    def _check_resource_limits(self, file_path: Path, lines: List[str]):
        """Find missing resource limits on LLM calls"""
        rel_file = str(file_path.relative_to(self.repo_root))
        
        for idx, line in enumerate(lines, 1):
            # Look for inference calls
            if re.search(r'(generate|infer|complete|embed)\s*\(', line, re.IGNORECASE):
                # Check for token limit or timeout
                next_lines = '\n'.join(lines[idx:min(idx+10, len(lines))])
                
                if not re.search(r'(max_tokens|token_limit|timeout|max_length|length)', 
                                next_lines, re.IGNORECASE):
                    self._append_gap(
                        rel_file,
                        idx,
                        'MEDIUM',
                        'missing_resource_limits',
                        'LLM inference without token limit or timeout (DOS risk)',
                        line.strip(),
                    )
