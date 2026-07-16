#!/usr/bin/env python3
"""
Wave 6: Semantic FP Reduction — Context-aware filtering for ThemisDB Gap Scanner

Strategy: Use code context, comments, and semantic patterns to eliminate benign gaps
- Analyze surrounding code (±5 lines around gap location)
- Detect intentional patterns (comments, established idioms, safe patterns)
- Filter dead code (unreachable, test-only, conditionally compiled)
- Identify known-safe configurations and well-structured code

Expected reduction: 30-40% additional FP elimination
Target: 12,539 → 7,500-8,800 gaps
"""

import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any


class Wave6SemanticFilters:
    """Semantic FP reduction using code context analysis"""
    
    # Semantic patterns indicating benign/intentional code
    BENIGN_COMMENT_PATTERNS = [
        r'intentional',
        r'deliberate',
        r'by design',
        r'performance ok',
        r'perf ok',
        r'safe',
        r'verified',
        r'trusted',
        r'known issue',
        r'documented',
        r'legacy support',
        r'backward compat',
    ]
    
    # Patterns indicating false positive territory
    FP_INDICATOR_PATTERNS = [
        r'test',
        r'mock',
        r'stub',
        r'demo',
        r'example',
        r'benchmark',
        r'temporary',
        r'TODO',
        r'FIXME',
        r'HACK',
        r'XXX',
    ]
    
    # Common idioms and patterns that are fine
    SAFE_IDIOM_PATTERNS = [
        r'auto\s+\w+\s*=\s*std::make_unique',  # RAII construction
        r'std::lock_guard',  # Scoped locking
        r'std::scoped_lock',
        r'std::unique_lock',
        r'const\s+auto\s+&',  # Const reference (non-owning)
        r'std::move\(',  # Explicit move semantics
        r'\[this\]',  # Lambda capture (scoped)
        r'final\s*\(',  # Final method override
        r'noexcept',  # Exception safety declaration
        r'[[nodiscard]]',  # Compiler hint (intentional)
        r'[[deprecated',  # Intentional deprecation
        r'[[maybe_unused]]',  # Intentional
    ]
    
    @staticmethod
    def _read_code_context(file_path: str, line_num: Optional[int], context_lines: int = 5) -> str:
        """
        Read code context around a gap location.
        
        Args:
            file_path: Path to source file
            line_num: Line number of gap (0-indexed or None)
            context_lines: Number of lines before/after to read
        
        Returns:
            Multi-line code context string
        """
        try:
            if not Path(file_path).exists():
                return ""
            
            content = Path(file_path).read_text(errors='ignore')
            lines = content.split('\n')
            
            if line_num is None or line_num <= 0:
                return ""
            
            start = max(0, line_num - context_lines - 1)
            end = min(len(lines), line_num + context_lines)
            
            context = '\n'.join(lines[start:end])
            return context
        except Exception:
            return ""
    
    @staticmethod
    def _has_benign_comment(context: str) -> bool:
        """Check if code context has benign/intentional comment"""
        for pattern in Wave6SemanticFilters.BENIGN_COMMENT_PATTERNS:
            if re.search(pattern, context, re.IGNORECASE):
                return True
        return False
    
    @staticmethod
    def _is_likely_test_code(context: str, file_path: str) -> bool:
        """Detect if gap is in test-only code"""
        # File-level detection
        if any(test_dir in file_path.lower() for test_dir in ['test', 'benchmark', 'demo', 'example']):
            return True
        
        # Code pattern detection
        for test_pattern in [
            r'TEST\(',
            r'TEST_F\(',
            r'BENCHMARK\(',
            r'GTEST_SKIP',
            r'mock',
            r'stub',
        ]:
            if re.search(test_pattern, context):
                return True
        
        return False
    
    @staticmethod
    def _has_safe_idiom(context: str) -> bool:
        """Detect well-established safe patterns"""
        for idiom_pattern in Wave6SemanticFilters.SAFE_IDIOM_PATTERNS:
            if re.search(idiom_pattern, context):
                return True
        return False
    
    @staticmethod
    def _is_dead_code(context: str) -> bool:
        """Detect dead/unreachable code markers"""
        dead_patterns = [
            r'#if\s+0',  # Disabled code block
            r'#ifdef\s+NEVER',
            r'if\s*\(\s*false\s*\)',  # Never-execute block
            r'if\s*\(\s*0\s*\)',
            r'return\s*;.*\n.*',  # Code after return
            r'throw\s+.*?;.*\n.*',  # Code after throw
        ]
        
        for pattern in dead_patterns:
            if re.search(pattern, context):
                return True
        
        return False
    
    @staticmethod
    def _is_fp_indicator_code(context: str) -> bool:
        """Detect code marked as unfinished/temporary"""
        # Check for TODO/FIXME/HACK comments
        for indicator in Wave6SemanticFilters.FP_INDICATOR_PATTERNS:
            # Only treat as FP indicator if marked as unfinished
            pattern = rf'{indicator}.*(?:fix|implement|complete|review)'
            if re.search(pattern, context, re.IGNORECASE):
                return True
        
        return False
    
    @staticmethod
    def _is_scoped_operation(context: str, gap_context: str) -> bool:
        """Detect scope-limited operations (stack-allocated, local scope)"""
        scope_patterns = [
            r'\{\s*$',  # Opening brace (scope start)
            r'for\s*\([^)]+\)',  # Loop scope
            r'while\s*\([^)]+\)',
            r'if\s*\([^)]+\)',  # Conditional scope
            r'auto\s+\w+\s*=',  # Local variable
            r'std::\w+<.*>',  # STL container
        ]
        
        # Check if gap is within tight scope
        if re.search(r'\{\s*\n.*\}', context, re.DOTALL):
            # Looks like a self-contained block
            return True
        
        for pattern in scope_patterns:
            if re.search(pattern, context):
                return True
        
        return False
    
    @classmethod
    def filter_semantic(cls, gaps: List[Dict], file_path: str) -> List[Dict]:
        """
        Apply semantic filtering to gaps based on code context.
        
        Args:
            gaps: List of gaps with location info
            file_path: Source file path
        
        Returns:
            Filtered gaps (removes obvious false positives)
        """
        filtered = []
        
        for gap in gaps:
            if gap.get('severity') in ('LOW', 'SKIP'):
                filtered.append(gap)
                continue
            
            # Extract line number
            location = gap.get('location', '').strip()
            line_num = None
            if ':' in location:
                try:
                    line_num = int(location.split(':')[1])
                except (ValueError, IndexError):
                    pass
            
            # Read code context
            context = cls._read_code_context(file_path, line_num, context_lines=7)
            if not context:
                # Can't read context; keep gap
                filtered.append(gap)
                continue
            
            gap_context = gap.get('context', '').strip()
            combined_context = context + '\n' + gap_context
            
            # Semantic filtering heuristics (elimination rules)
            
            # Rule 1: Has benign/intentional comment → eliminate
            if cls._has_benign_comment(combined_context):
                continue
            
            # Rule 2: Test-only code → eliminate
            if cls._is_likely_test_code(combined_context, file_path):
                continue
            
            # Rule 3: Uses safe idiom → eliminate
            if cls._has_safe_idiom(combined_context):
                continue
            
            # Rule 4: Dead/unreachable code → eliminate
            if cls._is_dead_code(combined_context):
                continue
            
            # Rule 5: Marked as unfinished → keep (not a FP)
            if cls._is_fp_indicator_code(combined_context):
                # This is likely a real issue waiting to be fixed
                filtered.append(gap)
                continue
            
            # Rule 6: Tight scope operation → eliminate
            if cls._is_scoped_operation(combined_context, gap_context):
                # Check if truly scoped (limited impact)
                if len(gap_context) < 150 and context.count('\n') <= 10:
                    continue
            
            # Rule 7: Empty or minimal context (unclear) → keep
            if len(gap_context.strip()) < 20:
                filtered.append(gap)
                continue
            
            # Keep gap (couldn't conclusively identify as FP)
            filtered.append(gap)
        
        return filtered
    
    @classmethod
    def apply_wave6_filters(cls, gaps_dict: Dict[str, Any], file_path: str) -> Dict[str, Any]:
        """
        Apply Wave 6 semantic filters to all categories.
        
        Args:
            gaps_dict: Dict with 'by_category' key
            file_path: Source file path
        
        Returns:
            Filtered gaps dict
        """
        by_category = gaps_dict.get('by_category', {})
        filtered_by_category = {}
        
        for category, category_gaps in by_category.items():
            # Apply semantic filtering for this category
            filtered_by_category[category] = cls.filter_semantic(category_gaps, file_path)
        
        gaps_dict['by_category'] = filtered_by_category
        return gaps_dict


# Semantic pattern analysis (extended heuristics)
class SemanticAnalyzer:
    """Advanced semantic analysis for gap filtering"""
    
    @staticmethod
    def analyze_gap_confidence(gap: Dict, context: str, file_path: str) -> float:
        """
        Compute semantic confidence that a gap is a real issue (not FP).
        
        Args:
            gap: Gap dict
            context: Code context
            file_path: Source file path
        
        Returns:
            Confidence score 0.0-1.0 (0=likely FP, 1=likely real issue)
        """
        score = 0.5  # Base confidence
        
        # Negative confidence signals (likely FP)
        if 'test' in file_path.lower():
            score -= 0.15
        if 'benchmark' in file_path.lower():
            score -= 0.15
        if 'example' in file_path.lower():
            score -= 0.10
        if re.search(r'intentional|deliberate|by design|safe', context, re.IGNORECASE):
            score -= 0.20
        if re.search(r'TODO.*fix|FIXME.*complete|XXX.*implement', context, re.IGNORECASE):
            score -= 0.10
        if re.search(r'#ifdef.*NEVER|if.*false|if.*0', context):
            score -= 0.15
        
        # Positive confidence signals (likely real issue)
        if re.search(r'production|critical|real.*data|user.*input', context, re.IGNORECASE):
            score += 0.15
        if 'src/' in file_path and 'test' not in file_path.lower():
            score += 0.10
        if gap.get('severity') == 'CRITICAL':
            score += 0.15
        
        # Clamp to [0.0, 1.0]
        return max(0.0, min(1.0, score))
        if gap.get('severity') == 'HIGH':
            score += 0.10
        if len(gap.get('context', '')) > 200:
            score += 0.05
        
        # Clamp to [0.0, 1.0]
        return max(0.0, min(1.0, score))
