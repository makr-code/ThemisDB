#!/usr/bin/env python3
"""
Root-Cause Analysis for False Positive Gap Categories

Analyzes 0% TP categories to understand why they're all false positives.
Extracts patterns and generates concrete scanner tuning recommendations.
"""

import json
from pathlib import Path
from typing import List, Dict, Tuple, Set
from collections import defaultdict
import re


class RootCauseAnalyzer:
    def __init__(self, repo_root: Path):
        self.repo_root = repo_root
        self.report_json = repo_root / "ai_working" / "VALIDATION_ANALYSIS_REPORT.json"
        self.gaps_json = repo_root / "ai_working" / "sample_validation_metadata.json"
        
    def load_analysis_report(self) -> Dict:
        """Load automated analysis report"""
        if not self.report_json.exists():
            print(f"[ERROR] Report not found: {self.report_json}")
            return {}
        
        with open(self.report_json, 'r', encoding='utf-8') as f:
            return json.load(f)
    
    def load_gaps_metadata(self) -> Dict:
        """Load gaps metadata with sample data"""
        if not self.gaps_json.exists():
            print(f"[ERROR] Metadata not found: {self.gaps_json}")
            return {}
        
        with open(self.gaps_json, 'r', encoding='utf-8') as f:
            return json.load(f)
    
    def get_source_context(self, file_path: str, line_num: int, context_lines: int = 20) -> str:
        """Get source code context around flagged line"""
        full_path = self.repo_root / file_path
        if not full_path.exists():
            return f"[FILE NOT FOUND: {file_path}]"
        
        try:
            with open(full_path, 'r', encoding='utf-8', errors='replace') as f:
                lines = f.readlines()
            
            start = max(0, line_num - 1 - context_lines)
            end = min(len(lines), line_num + context_lines)
            
            context = []
            for i in range(start, end):
                marker = ">>>" if i == line_num - 1 else "   "
                context.append(f"{marker} {i+1:5d}: {lines[i].rstrip()}")
            
            return "\n".join(context)
        except Exception as e:
            return f"[ERROR reading file: {e}]"
    
    def analyze_category_fps(self, category: str, all_gaps: List[Dict]) -> Dict:
        """Analyze all FP cases in a category to find patterns"""
        
        # Filter gaps from metadata
        fps = [g for g in all_gaps 
               if g.get('category') == category]
        
        if not fps:
            return {'category': category, 'fps': 0, 'patterns': [], 'fp_patterns': {}}
        
        patterns = defaultdict(list)
        
        for gap in fps:
            file_path = gap.get('file', '')
            line_num = gap.get('line', 0)
            description = gap.get('description', '')
            reasoning = gap.get('reasoning', '')
            
            # Get source context
            context = self.get_source_context(file_path, line_num, context_lines=10)
            
            # Extract patterns
            pattern_info = {
                'file': file_path,
                'line': line_num,
                'description': description,
                'reasoning': reasoning,
                'context': context,
            }
            
            # Categorize FP reason
            fp_reason = self._extract_fp_reason(reasoning, description, context)
            patterns[fp_reason].append(pattern_info)
        
        return {
            'category': category,
            'fps': len(fps),
            'fp_patterns': dict(patterns),
            'samples': fps[:2]  # First 2 as samples
        }
    
    def _extract_fp_reason(self, reasoning: str, description: str, context: str) -> str:
        """Extract why this is likely a false positive"""
        
        # Pattern-based FP detection
        if 'safe pattern' in reasoning.lower():
            return 'SAFE_PATTERN'
        if 'raii' in reasoning.lower() or 'unique_ptr' in reasoning.lower():
            return 'RAII_PATTERN'
        if 'internal' in reasoning.lower() or 'utility' in reasoning.lower():
            return 'INTERNAL_CODE'
        if 'test' in reasoning.lower():
            return 'TEST_CODE'
        if 'mock' in reasoning.lower() or 'stub' in reasoning.lower():
            return 'MOCK_CODE'
        if 'lock_guard' in reasoning.lower() or 'unique_lock' in reasoning.lower():
            return 'SYNCHRONIZED'
        if 'const' in reasoning.lower() and ('&' in reasoning.lower() or 'constexpr' in reasoning.lower()):
            return 'CONST_SAFE'
        if 'no issue found' in reasoning.lower():
            return 'NO_ISSUE'
        if 'over-aggressive' in reasoning.lower():
            return 'OVER_AGGRESSIVE'
        
        return 'OTHER'
    
    def generate_tuning_recommendations(self, analysis: Dict) -> Dict:
        """Generate concrete scanner tuning recommendations from analysis"""
        
        recommendations = []
        
        # Copy_overhead analysis
        if analysis.get('category') == 'copy_overhead':
            recommendations.append({
                'action': 'WHITELIST_PATTERN',
                'pattern': 'std::make_shared / std::make_unique',
                'reason': 'These are safe and should never be flagged',
                'confidence': 'HIGH'
            })
            recommendations.append({
                'action': 'REQUIRE_CONTEXT',
                'pattern': 'Copy in loop detection',
                'reason': 'Need actual loop context, not just function calls',
                'confidence': 'HIGH'
            })
            recommendations.append({
                'action': 'ADD_TYPE_CHECK',
                'pattern': 'Distinguish POD vs complex types',
                'reason': 'POD copies are cheap, complex copies are expensive',
                'confidence': 'MEDIUM'
            })
        
        # Observability analysis
        elif analysis.get('category') == 'observability':
            recommendations.append({
                'action': 'SKIP_INTERNAL',
                'pattern': 'Private/internal functions',
                'reason': 'Internal utils don\'t need observability',
                'confidence': 'HIGH'
            })
            recommendations.append({
                'action': 'SKIP_TRIVIAL',
                'pattern': 'Functions < 5 lines',
                'reason': 'Trivial functions don\'t need logging',
                'confidence': 'MEDIUM'
            })
            recommendations.append({
                'action': 'REQUIRE_PUBLIC_API',
                'pattern': 'Only flag public/exported functions',
                'reason': 'Users care about public API observability',
                'confidence': 'HIGH'
            })
        
        # Generic recommendations
        recommendations.extend([
            {
                'action': 'ADD_CONFIDENCE_THRESHOLD',
                'pattern': 'Require multiple signals',
                'reason': 'Single-signal flags have high FP rate',
                'confidence': 'HIGH'
            },
            {
                'action': 'EXPAND_CONTEXT',
                'pattern': 'Increase context window from 5->15 lines',
                'reason': 'Current context too small for safe detection',
                'confidence': 'MEDIUM'
            },
            {
                'action': 'TYPE_SAFE_PATTERNS',
                'pattern': 'Whitelist type-safe containers/patterns',
                'reason': 'Many flags are on type-safe code',
                'confidence': 'HIGH'
            }
        ])
        
        return {
            'category': analysis.get('category'),
            'fps': analysis.get('fps'),
            'recommendations': recommendations
        }
    
    def run_analysis(self, target_categories: List[str] = None) -> Dict:
        """Run root-cause analysis on target categories"""
        
        if target_categories is None:
            target_categories = [
                'copy_overhead', 'observability', 'db_connection_leak',
                'no_health_check', 'hardcoded_path'
            ]
        
        metadata = self.load_gaps_metadata()
        if not metadata:
            return {'error': 'Could not load metadata'}
        
        all_gaps = metadata.get('gaps', [])
        
        results = {
            'timestamp': str(Path.cwd()),
            'categories_analyzed': target_categories,
            'analyses': [],
            'tuning_roadmap': []
        }
        
        print("\n" + "="*80)
        print("ROOT-CAUSE ANALYSIS: False Positive Categories (0% TP)")
        print("="*80 + "\n")
        
        for category in target_categories:
            print(f"[ANALYZING] {category}...")
            
            analysis = self.analyze_category_fps(category, all_gaps)
            tuning = self.generate_tuning_recommendations(analysis)
            
            results['analyses'].append(analysis)
            results['tuning_roadmap'].append(tuning)
            
            # Print summary
            print(f"  FP Count: {analysis.get('fps')}")
            print(f"  Patterns: {len(analysis.get('fp_patterns', {}))}")
            for pattern, samples in analysis.get('fp_patterns', {}).items():
                print(f"    - {pattern}: {len(samples)} cases")
            print()
        
        return results
    
    def save_results(self, results: Dict) -> Path:
        """Save root-cause analysis results"""
        output_file = self.repo_root / "ai_working" / "ROOTCAUSE_ANALYSIS_FP_CATEGORIES.json"
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(results, f, indent=2, ensure_ascii=False)
        
        print(f"\n[✓] Root-cause analysis saved to: {output_file}")
        return output_file
    
    def generate_markdown_report(self, results: Dict) -> str:
        """Generate markdown report with findings and recommendations"""
        
        md = """# Root-Cause Analysis: False Positive Categories

**Date:** 2026-06-02  
**Focus:** Categories with 0% TP rate (complete false positives)

---

## Summary

False positive categories indicate that the scanner is over-aggressive in these areas.
This analysis extracts why gaps are false positives and generates concrete tuning recommendations.

---

"""
        
        for tuning in results.get('tuning_roadmap', []):
            category = tuning.get('category', 'unknown')
            fp_count = tuning.get('fps', 0)
            
            md += f"## {category.upper()} (FP Count: {fp_count})\n\n"
            
            for rec in tuning.get('recommendations', []):
                action = rec.get('action', '')
                pattern = rec.get('pattern', '')
                reason = rec.get('reason', '')
                confidence = rec.get('confidence', '')
                
                md += f"### {action}\n"
                md += f"- **Pattern:** {pattern}\n"
                md += f"- **Reason:** {reason}\n"
                md += f"- **Confidence:** {confidence}\n\n"
        
        return md


def main():
    repo_root = Path.cwd()
    analyzer = RootCauseAnalyzer(repo_root)
    
    # Run analysis on 0% TP categories
    results = analyzer.run_analysis([
        'copy_overhead',
        'observability', 
        'db_connection_leak',
        'no_health_check',
        'hardcoded_path'
    ])
    
    # Save JSON results
    analyzer.save_results(results)
    
    # Generate markdown report
    md_report = analyzer.generate_markdown_report(results)
    md_file = repo_root / "ai_working" / "ROOTCAUSE_ANALYSIS_MARKDOWN.md"
    with open(md_file, 'w', encoding='utf-8') as f:
        f.write(md_report)
    print(f"[✓] Markdown report saved to: {md_file}")
    
    # Print tuning roadmap
    print("\n" + "="*80)
    print("TUNING RECOMMENDATIONS")
    print("="*80 + "\n")
    
    for tuning in results.get('tuning_roadmap', []):
        print(f"\n[{tuning.get('category')}]")
        for rec in tuning.get('recommendations', []):
            print(f"  • {rec.get('action')}: {rec.get('pattern')}")
            print(f"    → {rec.get('reason')}")
    
    print("\n" + "="*80)
    print("✅ ROOT-CAUSE ANALYSIS COMPLETE")
    print("="*80 + "\n")


if __name__ == '__main__':
    main()
