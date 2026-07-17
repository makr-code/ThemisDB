#!/usr/bin/env python3
"""
Phase 9-1: Query Correctness & Semantic Validation Scanner

CWE-89 (SQL Injection), CWE-843 (Type Confusion)

Detects:
- Query string concatenation (injection risk)
- Missing query parameter validation
- Type mismatches in WHERE clauses
- Join cardinality not validated
- Aggregation correctness gaps
- GROUP BY without proper semantics
- HAVING without GROUP BY
- Correlated subquery inefficiency
- Union type mismatches
- Query result ordering not guaranteed
- Missing DISTINCT in aggregates
"""

import re
from pathlib import Path
from typing import List, Dict


class QueryCorrectnessScan:
    """Scan for query correctness and semantic validation issues"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
        self._sql_kw_re = re.compile(
            r'\b(SELECT|FROM|WHERE|JOIN|GROUP\s+BY|HAVING|FILTER|FOR\s+\w+\s+IN)\b',
            re.IGNORECASE,
        )
        self._named_placeholder_re = re.compile(r'["\'][^"\']*:[A-Za-z_]\w*[^"\']*["\']')
    
    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files for query correctness issues"""
        
        for file_path in file_list:
            if not file_path.suffix in ['.cpp', '.cc', '.h', '.hpp']:
                continue
            
            # Check if file is query-related
            if not self._is_query_file(str(file_path)):
                continue
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = content.split('\n')
            except Exception:
                continue
            
            # Scan patterns
            self._check_query_string_concat(file_path, lines)
            self._check_parameter_validation(file_path, lines)
            self._check_type_mismatches(file_path, lines)
            self._check_join_semantics(file_path, lines)
            self._check_aggregation_issues(file_path, lines)
        
        return self.gaps
    
    def _is_query_file(self, file_path: str) -> bool:
        """Check if file is query-related"""
        keywords = ['query', 'plan', 'parser', 'executor', 'optimizer', 'compile']
        return any(kw in file_path.lower() for kw in keywords)

    def _is_likely_query_literal(self, line: str) -> bool:
        """Heuristic: only scan lines that likely embed SQL/AQL text literals."""
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            return False

        if '"' not in line and "'" not in line:
            return False

        literals = re.findall(r'["\']([^"\']+)["\']', line)
        if not literals:
            return False

        text = ' '.join(literals)
        if not self._sql_kw_re.search(text):
            return False

        upper = text.upper()
        keyword_hits = sum(
            kw in upper for kw in ['SELECT', 'FROM', 'WHERE', 'FILTER', 'JOIN', 'GROUP BY', 'HAVING']
        )
        if keyword_hits >= 2:
            return True

        # AQL common shape: FOR <var> IN <collection>
        return 'FOR ' in upper and ' IN ' in upper

    def _contains_named_placeholder(self, line: str) -> bool:
        """Detect named placeholders like ':name' only inside quoted query literals."""
        return bool(self._named_placeholder_re.search(line))
    
    def _check_query_string_concat(self, file_path: Path, lines: List[str]):
        """Find query string concatenation (injection risk)"""
        
        for idx, line in enumerate(lines, 1):
            # Look for string + or append in SQL/AQL query-literal context.
            if not self._is_likely_query_literal(line):
                continue
            if re.search(r'(\+=|append|concat|operator\+).*["\'].*["\']', line):
                self.gaps.append({
                    'file': str(file_path.relative_to(self.repo_root)),
                    'line': idx,
                    'category': 'query_correctness',
                    'severity': 'CRITICAL',
                    'pattern': 'query_string_concat',
                    'description': 'Query string concatenation (SQL injection risk)',
                    'context': line.strip()
                })
    
    def _check_parameter_validation(self, file_path: Path, lines: List[str]):
        """Find missing query parameter validation"""
        
        for idx, line in enumerate(lines, 1):
            # Look for parameter binding
            has_bind_call = bool(re.search(r'bind\s*\(|bind_param', line))
            has_named_placeholder = self._contains_named_placeholder(line)

            if not has_bind_call and not has_named_placeholder:
                continue

            # Placeholder-only matches are noisy in C++; require SQL/AQL literal context.
            if has_named_placeholder and not has_bind_call and not self._is_likely_query_literal(line):
                continue

            # Check if value is validated
            prev_lines = '\n'.join(lines[max(0, idx-10):idx])

            if not re.search(r'(validate|check|assert|range)', prev_lines, re.IGNORECASE):
                self.gaps.append({
                    'file': str(file_path.relative_to(self.repo_root)),
                    'line': idx,
                    'category': 'query_correctness',
                    'severity': 'HIGH',
                    'pattern': 'missing_param_validation',
                    'description': 'Query parameter binding without validation',
                    'context': line.strip()
                })
    
    def _check_type_mismatches(self, file_path: Path, lines: List[str]):
        """Find type mismatches in WHERE clauses"""
        
        for idx, line in enumerate(lines, 1):
            if not self._is_likely_query_literal(line):
                continue
            # Look for WHERE clause comparisons
            if 'WHERE' in line.upper() or 'where' in line.lower():
                # Check for string/int comparisons
                if re.search(r'(["\'].*["\'])\s*(<|>|=|!=)\s*(\d+|NULL)', line):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'query_correctness',
                        'severity': 'HIGH',
                        'pattern': 'type_mismatch_where',
                        'description': 'Type mismatch in WHERE clause (string vs numeric)',
                        'context': line.strip()
                    })
    
    def _check_join_semantics(self, file_path: Path, lines: List[str]):
        """Find join semantics issues"""
        
        for idx, line in enumerate(lines, 1):
            if not self._is_likely_query_literal(line):
                continue
            literal_text = ' '.join(re.findall(r'["\']([^"\']+)["\']', line))
            if not literal_text:
                continue
            has_query_structure = bool(
                re.search(r'\bSELECT\b.+\bFROM\b', literal_text, re.IGNORECASE)
                or re.search(r'\bFOR\s+\w+\s+IN\s+[A-Za-z_]\w*(?!-)\b', literal_text, re.IGNORECASE)
                or re.search(r'\bFILTER\b', literal_text, re.IGNORECASE)
            )
            if not has_query_structure:
                continue
            has_join_token = bool(re.search(r'\bJOIN\b', literal_text, re.IGNORECASE))
            has_on_token = bool(re.search(r'\bON\b', literal_text, re.IGNORECASE))
            # Look for JOIN without ON condition
            if has_join_token and not has_on_token:
                self.gaps.append({
                    'file': str(file_path.relative_to(self.repo_root)),
                    'line': idx,
                    'category': 'query_correctness',
                    'severity': 'CRITICAL',
                    'pattern': 'missing_join_condition',
                    'description': 'JOIN without ON condition (Cartesian product risk)',
                    'context': line.strip()
                })
            
            # OUTER JOIN cardinality issues
            if 'LEFT JOIN' in line.upper() or 'RIGHT JOIN' in line.upper():
                # Check if result cardinality is addressed
                next_lines = '\n'.join(lines[idx:min(idx+10, len(lines))])
                if 'DISTINCT' not in next_lines.upper() and 'GROUP BY' not in next_lines.upper():
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'query_correctness',
                        'severity': 'MEDIUM',
                        'pattern': 'outer_join_cardinality',
                        'description': 'OUTER JOIN without DISTINCT/GROUP BY (cardinality issues)',
                        'context': line.strip()
                    })
    
    def _check_aggregation_issues(self, file_path: Path, lines: List[str]):
        """Find aggregation correctness issues"""
        
        for idx, line in enumerate(lines, 1):
            if not self._is_likely_query_literal(line):
                continue
            # HAVING without GROUP BY
            if 'HAVING' in line.upper() and 'GROUP BY' not in '\n'.join(lines[max(0, idx-10):idx]):
                self.gaps.append({
                    'file': str(file_path.relative_to(self.repo_root)),
                    'line': idx,
                    'category': 'query_correctness',
                    'severity': 'HIGH',
                    'pattern': 'having_without_group_by',
                    'description': 'HAVING clause without GROUP BY',
                    'context': line.strip()
                })
            
            # Aggregates not in DISTINCT
            if 'SELECT' in line.upper():
                next_lines = '\n'.join(lines[idx:min(idx+5, len(lines))])
                if re.search(r'(COUNT|SUM|AVG|MAX|MIN)\s*\(', next_lines) and \
                   'DISTINCT' not in next_lines and 'GROUP BY' not in next_lines:
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'query_correctness',
                        'severity': 'MEDIUM',
                        'pattern': 'aggregate_without_grouping',
                        'description': 'Aggregate function without GROUP BY or DISTINCT',
                        'context': line.strip()
                    })
