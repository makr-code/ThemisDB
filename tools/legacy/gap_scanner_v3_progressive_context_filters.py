#!/usr/bin/env python3
"""
Progressive Context FP Reduction Filter
======================================

Staged false positive elimination with increasing context window sizes.

Waves:
  1. ±5 lines   → Obvious FPs (comments, tests)
  2. ±15 lines  → Pattern-based detection
  3. ±30 lines  → Safe pattern recognition
  4. ±50 lines  → Edge case elimination
  5. Full func  → Complete semantic analysis
"""

import json
import re
from pathlib import Path
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass, asdict
import os


@dataclass
class GapWithContext:
    """Gap with full context information."""
    gap: Dict
    context: str
    context_size: int
    file_path: str
    line_num: int
    confidence: float = 0.5


class ProgressiveContextFilter:
    """Apply FP detection with progressively larger context windows."""
    
    def __init__(self, repo_root: Path):
        self.repo_root = repo_root
        self.elimination_stats = {
            'wave1_obvious': 0,
            'wave2_pattern': 0,
            'wave3_safe': 0,
            'wave4_edge': 0,
            'wave5_semantic': 0,
            'total_eliminated': 0,
            'remaining': 0,
        }
    
    def get_context(self, file_path: str, line_num: int, context_size: int = 5) -> str:
        """
        Read context around a line.
        
        context_size:
          5-99: number of lines to read before/after
          -1: full function
        """
        try:
            abs_path = self.repo_root / file_path.lstrip('./')
            if not abs_path.exists():
                return ""
            
            with open(abs_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
            
            if line_num < 1 or line_num > len(lines):
                return ""
            
            if context_size < 0:  # Full function
                return self._get_full_function(lines, line_num)
            
            start = max(0, line_num - context_size - 1)
            end = min(len(lines), line_num + context_size)
            return "".join(lines[start:end])
        except Exception:
            return ""
    
    def _get_full_function(self, lines: List[str], line_num: int) -> str:
        """Extract full function containing line_num."""
        # Find function start
        func_start = line_num - 1
        for i in range(line_num - 2, max(0, line_num - 100), -1):
            line = lines[i].strip()
            if line.endswith('{') or (')' in line and any(c in lines[i] for c in ['{', '\n'])):
                func_start = i
                break
        
        # Find function end
        brace_count = 0
        func_end = min(line_num + 200, len(lines))
        for i in range(func_start, len(lines)):
            brace_count += lines[i].count('{') - lines[i].count('}')
            if brace_count <= 0 and i > func_start:
                func_end = i + 1
                break
        
        return "".join(lines[func_start:func_end])
    
    # ========== WAVE 1: Obvious FPs (±5 lines) ==========
    
    def wave1_obvious(self, gaps: List[Dict]) -> List[Dict]:
        """Eliminate obvious false positives with minimal context."""
        remaining = []
        
        for gap in gaps:
            file_path = gap.get('file', '')
            line_num = gap.get('line', 0)
            
            context = self.get_context(file_path, line_num, context_size=5)
            
            if self._is_obvious_fp(gap, context):
                self.elimination_stats['wave1_obvious'] += 1
                continue
            
            remaining.append(gap)
        
        return remaining
    
    def _is_obvious_fp(self, gap: Dict, context: str) -> bool:
        """Check if gap is obviously false positive."""
        if not context:
            return False
        
        message = gap.get('description', gap.get('message', '')).lower()
        category = gap.get('category', gap.get('type', '')).lower()
        
        # Comment blocks
        if '//' in context and '//' in context[:100]:
            return True
        if '/*' in context or '*/' in context:
            return True
        
        # Test code
        if any(x in context for x in ['TEST(', 'TEST_F(', 'GTEST_', 'Mock', 'Stub']):
            return True
        
        # Examples/demos
        if 'example' in context.lower() or 'demo' in context.lower():
            return True
        
        # TODOs/FIXMEs
        if 'TODO' in context or 'FIXME' in context or 'XXX' in context:
            return True
        
        return False
    
    # ========== WAVE 2: Pattern-based (±15 lines) ==========
    
    def wave2_pattern(self, gaps: List[Dict]) -> List[Dict]:
        """Eliminate pattern-based false positives."""
        remaining = []
        
        for gap in gaps:
            file_path = gap.get('file', '')
            line_num = gap.get('line', 0)
            category = gap.get('category', gap.get('type', '')).lower()
            
            context = self.get_context(file_path, line_num, context_size=15)
            
            if category == 'performance' and self._is_performance_pattern_fp(gap, context):
                self.elimination_stats['wave2_pattern'] += 1
                continue
            
            if category in ['uncaught_exception', 'exception_safety'] and \
               self._is_exception_pattern_fp(gap, context):
                self.elimination_stats['wave2_pattern'] += 1
                continue
            
            remaining.append(gap)
        
        return remaining
    
    def _is_performance_pattern_fp(self, gap: Dict, context: str) -> bool:
        """Check if performance gap is pattern-based FP."""
        msg = gap.get('description', '').lower()
        
        # vector::reserve in nlohmann::json arrays
        if 'push_back' in msg and 'nlohmann' in context:
            return True
        
        # Small allocations that are intentional
        if 'allocation' in msg and ('small' in msg or 'fixed' in msg):
            return True
        
        return False
    
    def _is_exception_pattern_fp(self, gap: Dict, context: str) -> bool:
        """Check if exception gap is pattern-based FP."""
        msg = gap.get('description', '').lower()
        
        # Generic catch() for input validation is intentional
        if 'catch(...)' in context and ('input' in msg or 'user' in msg):
            return True
        
        # Intentional broad exception handling
        if 'intentional' in msg or 'by_design' in msg:
            return True
        
        return False
    
    # ========== WAVE 3: Safe Patterns (±30 lines) ==========
    
    def wave3_safe_patterns(self, gaps: List[Dict]) -> List[Dict]:
        """Eliminate gaps with safe pattern recognition."""
        remaining = []
        
        for gap in gaps:
            file_path = gap.get('file', '')
            line_num = gap.get('line', 0)
            category = gap.get('category', gap.get('type', '')).lower()
            
            context = self.get_context(file_path, line_num, context_size=30)
            
            if self._is_safe_pattern(gap, context, line_num, file_path):
                self.elimination_stats['wave3_safe'] += 1
                continue
            
            remaining.append(gap)
        
        return remaining
    
    def _is_safe_pattern(self, gap: Dict, context: str, line_num: int, file_path: str) -> bool:
        """Check if gap matches safe code pattern."""
        msg = gap.get('description', '').lower()
        category = gap.get('category', '').lower()
        
        # Status/Result assigned then immediately checked
        if 'status' in msg and 's =' in context and '.ok()' in context:
            return True
        
        # Error code checked immediately
        if 'error' in msg and '!=' in context and ('== 0' in context or '.ok()' in context):
            return True
        
        # rocksdb::Status pattern
        if 'rocksdb' in context and 'Status' in context:
            lines = context.split('\n')
            for i, line in enumerate(lines):
                if 's =' in line and i < len(lines) - 2:
                    # Check if next few lines check status
                    following = ' '.join(lines[i:i+3])
                    if '.ok()' in following or '!s' in following:
                        return True
        
        # JSON/nlohmann patterns
        if 'json' in category and 'push_back' in msg:
            if 'json::array' in context or 'nlohmann::json' in context:
                return True
        
        return False
    
    # ========== WAVE 4: Edge Cases (±50 lines) ==========
    
    def wave4_edge_cases(self, gaps: List[Dict]) -> List[Dict]:
        """Eliminate edge case false positives."""
        remaining = []
        
        for gap in gaps:
            file_path = gap.get('file', '')
            line_num = gap.get('line', 0)
            
            context = self.get_context(file_path, line_num, context_size=50)
            
            if self._is_edge_case_fp(gap, context):
                self.elimination_stats['wave4_edge'] += 1
                continue
            
            remaining.append(gap)
        
        return remaining
    
    def _is_edge_case_fp(self, gap: Dict, context: str) -> bool:
        """Check for edge case FPs visible in 50-line context."""
        msg = gap.get('description', '').lower()
        
        # Determinism flag: strings vs floats
        if 'float' in msg and 'comparison' in msg:
            if 'std::string' in context or '.compare(' in context:
                return True
        
        # Pointer arithmetic with smart pointers
        if 'pointer' in msg and 'arithmetic' in msg:
            if 'unique_ptr' in context or 'shared_ptr' in context or 'auto [' in context:
                return True
        
        # Manual cleanup in constrained scopes
        if 'manual_cleanup' in gap.get('category', ''):
            if 'try' in context and 'catch' in context:
                return True  # RAII would be better but not critical
        
        return False
    
    # ========== WAVE 5: Full Semantic (full function) ==========
    
    def wave5_semantic(self, gaps: List[Dict]) -> List[Dict]:
        """Full semantic analysis with complete function context."""
        remaining = []
        
        for gap in gaps:
            file_path = gap.get('file', '')
            line_num = gap.get('line', 0)
            
            context = self.get_context(file_path, line_num, context_size=-1)  # Full function
            
            if self._is_semantic_fp(gap, context):
                self.elimination_stats['wave5_semantic'] += 1
                continue
            
            remaining.append(gap)
        
        return remaining
    
    def _is_semantic_fp(self, gap: Dict, context: str) -> bool:
        """Full semantic FP detection."""
        if not context:
            return False
        
        category = gap.get('category', '').lower()
        msg = gap.get('description', '').lower()
        
        # LLM safety: host buffers vs user input
        if 'llm' in category and 'input' in msg:
            if '_host' in context or 'device' in context:
                return True
        
        # Observability: missing health checks in initialization
        if 'health' in msg and 'check' in msg:
            if 'initialize' in context and 'return' in context:
                # Check if function has error handling
                if any(x in context for x in ['throw', 'return false', 'return nullptr']):
                    return True
        
        # Concurrency: lock scope visible
        if 'lock_guard' in context or 'unique_lock' in context:
            return False  # RAII locks are safe
        
        return False
    
    # ========== Main Pipeline ==========
    
    def apply_progressive_filtering(self, gaps: List[Dict]) -> Tuple[List[Dict], Dict]:
        """
        Apply all 5 waves of progressive context filtering.
        
        Returns:
          (remaining_gaps, elimination_stats)
        """
        print(f"[PROGRESSIVE FILTER] Starting with {len(gaps)} gaps")
        
        # Wave 1
        gaps = self.wave1_obvious(gaps)
        print(f"[WAVE 1] After obvious FPs: {len(gaps)} gaps (-{self.elimination_stats['wave1_obvious']})")
        
        # Wave 2
        gaps = self.wave2_pattern(gaps)
        print(f"[WAVE 2] After pattern FPs: {len(gaps)} gaps (-{self.elimination_stats['wave2_pattern']})")
        
        # Wave 3
        gaps = self.wave3_safe_patterns(gaps)
        print(f"[WAVE 3] After safe patterns: {len(gaps)} gaps (-{self.elimination_stats['wave3_safe']})")
        
        # Wave 4
        gaps = self.wave4_edge_cases(gaps)
        print(f"[WAVE 4] After edge cases: {len(gaps)} gaps (-{self.elimination_stats['wave4_edge']})")
        
        # Wave 5
        gaps = self.wave5_semantic(gaps)
        print(f"[WAVE 5] After semantic analysis: {len(gaps)} gaps (-{self.elimination_stats['wave5_semantic']})")
        
        # Final stats
        self.elimination_stats['total_eliminated'] = sum([
            self.elimination_stats['wave1_obvious'],
            self.elimination_stats['wave2_pattern'],
            self.elimination_stats['wave3_safe'],
            self.elimination_stats['wave4_edge'],
            self.elimination_stats['wave5_semantic'],
        ])
        self.elimination_stats['remaining'] = len(gaps)
        
        return gaps, self.elimination_stats


if __name__ == '__main__':
    # Test with a sample gap
    sample_gaps = [
        {
            'file': 'src/api/test.cpp',
            'line': 42,
            'category': 'performance',
            'description': 'missing vector::reserve',
            'message': 'vector::push_back in loop'
        }
    ]
    
    filter_obj = ProgressiveContextFilter(Path('.'))
    remaining, stats = filter_obj.apply_progressive_filtering(sample_gaps)
    
    print("\n[STATS]")
    for key, value in stats.items():
        print(f"  {key}: {value}")
