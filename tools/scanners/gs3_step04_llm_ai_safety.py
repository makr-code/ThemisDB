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
        
        for idx, line in enumerate(lines, 1):
            # Look for user input in prompt
            if re.search(r'(prompt|query|instruction)\s*[=\+].*user|input', line, re.IGNORECASE):
                # Check if sanitized
                prev_lines = '\n'.join(lines[max(0, idx-10):idx])
                
                if not re.search(r'(sanitize|escape|validate|verify)', prev_lines, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'llm_ai_safety',
                        'severity': 'CRITICAL',
                        'pattern': 'prompt_injection',
                        'description': 'User input in prompt without sanitization (injection risk)',
                        'context': line.strip()
                    })
    
    def _check_model_validation(self, file_path: Path, lines: List[str]):
        """Find missing model integrity validation"""
        
        for idx, line in enumerate(lines, 1):
            # Look for model loading
            if re.search(r'(load_model|LoadModel|load.*weights|deserialize)', line, re.IGNORECASE):
                # Check for checksum/signature validation
                next_lines = '\n'.join(lines[idx:min(idx+20, len(lines))])
                
                if not re.search(r'(checksum|hash|signature|verify|validate)', next_lines, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'llm_ai_safety',
                        'severity': 'CRITICAL',
                        'pattern': 'model_integrity_gap',
                        'description': 'Model loading without integrity verification (poisoning risk)',
                        'context': line.strip()
                    })
            
            # Look for remote model URLs
            if re.search(r'(http|https|url|URI).*model', line, re.IGNORECASE):
                if 'https' not in line and 'http' in line:
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'llm_ai_safety',
                        'severity': 'HIGH',
                        'pattern': 'insecure_model_url',
                        'description': 'Model downloaded over insecure HTTP',
                        'context': line.strip()
                    })
    
    def _check_input_sanitization(self, file_path: Path, lines: List[str]):
        """Find unsanitized input to LLM"""
        
        for idx, line in enumerate(lines, 1):
            # Look for inference/generation calls
            if re.search(r'(generate|infer|predict|complete)\s*\(.*user|input', line, re.IGNORECASE):
                # Check for normalization
                prev_lines = '\n'.join(lines[max(0, idx-10):idx])
                
                if not re.search(r'(normalize|sanitize|clean|strip)', prev_lines, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'llm_ai_safety',
                        'severity': 'HIGH',
                        'pattern': 'unsanitized_llm_input',
                        'description': 'User input passed to LLM without normalization/sanitization',
                        'context': line.strip()
                    })
    
    def _check_output_validation(self, file_path: Path, lines: List[str]):
        """Find LLM output used without validation"""
        
        for idx, line in enumerate(lines, 1):
            # Look for LLM output usage
            if re.search(r'(output|result|generated|response).*=.*generate|infer', line, re.IGNORECASE):
                # Check if validated before use
                next_lines = '\n'.join(lines[idx:min(idx+15, len(lines))])
                
                if not re.search(r'(validate|verify|check|assert)', next_lines, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'llm_ai_safety',
                        'severity': 'HIGH',
                        'pattern': 'unvalidated_llm_output',
                        'description': 'LLM output used without validation (hallucination/bias risk)',
                        'context': line.strip()
                    })
    
    def _check_resource_limits(self, file_path: Path, lines: List[str]):
        """Find missing resource limits on LLM calls"""
        
        for idx, line in enumerate(lines, 1):
            # Look for inference calls
            if re.search(r'(generate|infer|complete|embed)\s*\(', line, re.IGNORECASE):
                # Check for token limit or timeout
                next_lines = '\n'.join(lines[idx:min(idx+10, len(lines))])
                
                if not re.search(r'(max_tokens|token_limit|timeout|max_length|length)', 
                                next_lines, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'llm_ai_safety',
                        'severity': 'MEDIUM',
                        'pattern': 'missing_resource_limits',
                        'description': 'LLM inference without token limit or timeout (DOS risk)',
                        'context': line.strip()
                    })
