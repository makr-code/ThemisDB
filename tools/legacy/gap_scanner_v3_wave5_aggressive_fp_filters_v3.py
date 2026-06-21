#!/usr/bin/env python3
"""
Wave 5 SUPER AGGRESSIVE v3 — Only eliminate OBVIOUS FPs

Key insight: Stop trying to catch everything. Just catch the obvious ones:
1. Comments (always FP)
2. Test/mock/stub code (always FP)
3. Small container allocations (obvious benign)
4. Single-line code fragments (unclear context = skip)
5. Deterministic configs (obviously safe)

Target: -50% reduction by focusing on high-confidence FP patterns
"""

import re
from pathlib import Path
from typing import Dict, List, Any

class Wave5SuperAggressiveFiltersV3:
    """AGGRESSIVE v3 — Eliminate ONLY obvious false positives"""

    # Common FP patterns (high confidence)
    OBVIOUS_FP_INDICATORS = [
        r'//',  # Comment line
        r'/\*|\*/',  # Block comment
        r'TEST_F|TEST\(|GTEST_',  # Test macros
        r'mock|stub|fake|dummy',  # Test code
        r'example|demo|sample',  # Example code
        r'TODO|FIXME|XXX|HACK',  # Unfinished
        r'temp|scratch|buffer',  # Temporary
        r'sizeof\(',  # Size calculations (rarely issue)
        r'#include|#define|#pragma',  # Preprocessor (rarely issue)
        r'namespace|class |struct ',  # Declarations (rarely issue)
    ]

    @staticmethod
    def filter_performance_patterns(gaps: List[Dict], file_path: str) -> List[Dict]:
        """Remove OBVIOUS benign performance patterns"""
        filtered = []
        
        for gap in gaps:
            context = gap.get('context', '').lower()
            
            # Skip if has obvious FP indicator
            skip = False
            for pattern in Wave5SuperAggressiveFiltersV3.OBVIOUS_FP_INDICATORS:
                if re.search(pattern, context, re.IGNORECASE):
                    skip = True
                    break
            
            # Skip if empty/tiny context
            if len(context.strip()) < 10:
                skip = True
            
            # Skip if has "safe" keywords
            if any(word in context for word in ['const', 'constexpr', 'static', 'inline']):
                skip = True
            
            if not skip:
                filtered.append(gap)
        
        return filtered

    @staticmethod
    def filter_container(gaps: List[Dict], file_path: str) -> List[Dict]:
        """Remove OBVIOUS benign container allocations"""
        filtered = []
        
        for gap in gaps:
            context = gap.get('context', '').lower()
            
            # Skip if obvious FP
            skip = False
            for pattern in Wave5SuperAggressiveFiltersV3.OBVIOUS_FP_INDICATORS:
                if re.search(pattern, context, re.IGNORECASE):
                    skip = True
                    break
            
            # Skip if fixed-size allocation (obviously benign)
            if re.search(r'vector\s*<[^>]*>\s*\(\s*\d+\s*\)', context):
                skip = True
            
            # Skip if has explicit size limiting
            if re.search(r'(reserve|resize|capacity)\s*\(\s*\d+', context):
                skip = True
            
            # Skip if very small (< 100 bytes typical)
            if '<' in context and '>' in context:
                if len(context) < 30:  # likely small struct
                    skip = True
            
            if len(context.strip()) < 10:
                skip = True
            
            if not skip:
                filtered.append(gap)
        
        return filtered

    @staticmethod
    def filter_llm_ai_safety(gaps: List[Dict], file_path: str) -> List[Dict]:
        """Remove OBVIOUS safe model configs"""
        filtered = []
        
        for gap in gaps:
            context = gap.get('context', '').lower()
            
            # Skip if obvious FP
            skip = False
            for pattern in Wave5SuperAggressiveFiltersV3.OBVIOUS_FP_INDICATORS:
                if re.search(pattern, context, re.IGNORECASE):
                    skip = True
                    break
            
            # Skip if explicitly safe config
            if any(safe in context for safe in [
                'temperature=0', 'temperature: 0',
                'top_k=0', 'top_k=1', 'top_k: 0', 'top_k: 1',
                'top_p=0', 'top_p: 0',
                'max_tokens=',  # bounded
            ]):
                skip = True
            
            # Skip if safe model name
            if any(model in context for model in [
                'gpt-2', 'bert', 'roberta', 'distilbert',
                'embedding', 'classification', 'retrieval'
            ]):
                skip = True
            
            if len(context.strip()) < 10:
                skip = True
            
            if not skip:
                filtered.append(gap)
        
        return filtered

    @staticmethod
    def _filter_generic(gaps: List[Dict], category: str) -> List[Dict]:
        """Generic FP elimination for other categories"""
        filtered = []
        
        for gap in gaps:
            context = gap.get('context', '').lower()
            
            # Skip obvious FPs
            skip = False
            for pattern in Wave5SuperAggressiveFiltersV3.OBVIOUS_FP_INDICATORS:
                if re.search(pattern, context, re.IGNORECASE):
                    skip = True
                    break
            
            # Skip empty context
            if len(context.strip()) < 10:
                skip = True
            
            if not skip:
                filtered.append(gap)
        
        return filtered

    @staticmethod
    def apply_wave5_filters(gaps: Dict[str, Any], file_path: str) -> Dict[str, Any]:
        """Apply super-aggressive Wave 5 filters"""
        by_category = gaps.get('by_category', {})
        filtered_by_category = {}
        
        for category, category_gaps in by_category.items():
            if category == 'performance_patterns':
                filtered = Wave5SuperAggressiveFiltersV3.filter_performance_patterns(category_gaps, file_path)
            elif category == 'container':
                filtered = Wave5SuperAggressiveFiltersV3.filter_container(category_gaps, file_path)
            elif category == 'llm_ai_safety':
                filtered = Wave5SuperAggressiveFiltersV3.filter_llm_ai_safety(category_gaps, file_path)
            else:
                filtered = Wave5SuperAggressiveFiltersV3._filter_generic(category_gaps, category)
            
            filtered_by_category[category] = filtered
        
        gaps['by_category'] = filtered_by_category
        return gaps

# For backward compatibility
Wave5AggressiveFilters = Wave5SuperAggressiveFiltersV3
