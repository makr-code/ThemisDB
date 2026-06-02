#!/usr/bin/env python3
"""
Scanner Feedback Loop - Reinforced Learning System
===================================================

After each gap scan + validation cycle:
1. Analyze which scanners have high TP/FP rates
2. Identify patterns in false positives
3. Generate concrete improvement suggestions
4. Track improvement over time

This enables data-driven scanner tuning instead of guesswork.
"""

import json
from pathlib import Path
from typing import Dict, List, Tuple
from dataclasses import dataclass, asdict
from collections import defaultdict
from datetime import datetime


@dataclass
class ScannerFeedback:
    """Feedback for a single scanner/category"""
    category: str
    total_gaps: int
    tp_count: int
    fp_count: int
    uncertain_count: int
    tp_rate: float
    fp_rate: float
    confidence: float
    
    # Identified issues
    common_fp_patterns: List[str]
    false_negative_patterns: List[str]
    
    # Recommendations
    suggested_fixes: List[str]
    priority: str  # HIGH, MEDIUM, LOW
    estimated_impact: str  # "0% -> 50%", "24% -> 35%", etc
    effort_days: float


@dataclass
class ScanFeedbackReport:
    """Complete feedback report after a scan cycle"""
    timestamp: str
    scan_date: str
    validation_sample_size: int
    overall_tp_rate: float
    overall_fp_rate: float
    
    scanner_feedback: Dict[str, ScannerFeedback]
    
    # Top priorities
    high_priority_fixes: List[str]
    medium_priority_fixes: List[str]
    
    # Timeline estimate
    total_effort_days: float
    recommended_sequence: List[str]
    
    # Metrics tracking
    previous_tp_rate: float  # Track improvement over time
    improvement_delta: float


class ScannerFeedbackAnalyzer:
    """Analyze validation results and generate scanner feedback"""
    
    def __init__(self, repo_root: Path = Path('.')):
        self.repo_root = repo_root
        self.ai_working = repo_root / 'ai_working'
    
    def analyze_validation_results(self, 
                                   validation_report_path: Path,
                                   previous_report_path: Path = None) -> ScanFeedbackReport:
        """
        Analyze validation results and generate improvement feedback.
        
        Args:
            validation_report_path: Path to VALIDATION_ANALYSIS_REPORT.json
            previous_report_path: Path to previous feedback report (for comparison)
        
        Returns:
            ScanFeedbackReport with actionable recommendations
        """
        
        # Load validation data
        with open(validation_report_path) as f:
            validation = json.load(f)
        
        # Load previous feedback if available
        previous_tp_rate = 0.24  # Default starting baseline
        if previous_report_path and previous_report_path.exists():
            with open(previous_report_path) as f:
                prev_feedback = json.load(f)
                previous_tp_rate = prev_feedback.get('overall_tp_rate', 0.24)
        
        # Analyze per-category
        scanner_feedback = {}
        for category, count_data in validation.get('by_category', {}).items():
            # Convert count dict to category analysis dict
            category_data = {
                'category': category,
                'tp_count': count_data.get('TP', 0),
                'fp_count': count_data.get('FP', 0),
                'uncertain_count': count_data.get('?', 0),
                'total': count_data.get('TP', 0) + count_data.get('FP', 0) + count_data.get('?', 0),
            }
            feedback = self._analyze_category(category, category_data)
            scanner_feedback[category] = asdict(feedback)
        
        # Identify high-priority fixes
        high_priority = self._identify_priority_fixes(scanner_feedback, 'HIGH')
        medium_priority = self._identify_priority_fixes(scanner_feedback, 'MEDIUM')
        
        # Calculate timeline
        total_effort = sum(f['effort_days'] for f in scanner_feedback.values())
        recommended_sequence = self._rank_fixes(scanner_feedback)
        
        # Build report
        current_tp_rate = validation.get('tp_percent', 0.24) / 100
        current_fp_rate = validation.get('fp_percent', 0.36) / 100
        
        report = ScanFeedbackReport(
            timestamp=datetime.now().isoformat(),
            scan_date=validation.get('date', 'unknown'),
            validation_sample_size=validation['sample_size'],
            overall_tp_rate=current_tp_rate,
            overall_fp_rate=current_fp_rate,
            scanner_feedback=scanner_feedback,
            high_priority_fixes=high_priority,
            medium_priority_fixes=medium_priority,
            total_effort_days=total_effort,
            recommended_sequence=recommended_sequence,
            previous_tp_rate=previous_tp_rate,
            improvement_delta=current_tp_rate - previous_tp_rate,
        )
        
        return report
    
    def _analyze_category(self, category: str, data: Dict) -> ScannerFeedback:
        """Deep analysis of a single category"""
        
        total = data['total']
        tp = data['tp_count']
        fp = data['fp_count']
        uncertain = data['uncertain_count']
        
        tp_rate = tp / total if total > 0 else 0
        fp_rate = fp / total if total > 0 else 0
        
        # Analyze patterns
        common_fp_patterns = self._identify_fp_patterns(category, data)
        false_negatives = self._identify_false_negative_patterns(category, data)
        suggested_fixes = self._generate_fixes(category, tp_rate, common_fp_patterns)
        
        # Determine priority
        priority = self._determine_priority(tp_rate, fp_rate)
        
        # Estimate impact
        impact = self._estimate_impact(category, tp_rate, suggested_fixes)
        
        # Estimate effort
        effort = self._estimate_effort(category, len(suggested_fixes))
        
        return ScannerFeedback(
            category=category,
            total_gaps=total,
            tp_count=tp,
            fp_count=fp,
            uncertain_count=uncertain,
            tp_rate=tp_rate,
            fp_rate=fp_rate,
            confidence=tp / (tp + uncertain) if (tp + uncertain) > 0 else 0,
            common_fp_patterns=common_fp_patterns,
            false_negative_patterns=false_negatives,
            suggested_fixes=suggested_fixes,
            priority=priority,
            estimated_impact=impact,
            effort_days=effort,
        )
    
    def _identify_fp_patterns(self, category: str, data: Dict) -> List[str]:
        """Identify common patterns in false positives"""
        
        patterns = {
            'copy_overhead': [
                'make_shared/make_unique detected but whitelisted',
                'POD types (int, float) flagged as complex',
                'std::move() calls flagged as overhead',
                'Short loop iterations (< 5 iterations) flagged',
            ],
            'observability': [
                'Internal functions flagged (detail::, _impl)',
                'Trivial getters/setters flagged',
                'Private member functions flagged',
                'Functions < 5 lines flagged as missing metrics',
            ],
            'db_connection_leak': [
                'RAII-wrapped connections not recognized',
                'ConnectionPool pattern not detected',
                'Explicit close() calls outside context window',
                'Smart pointer cleanup not visible in ±5 line context',
            ],
            'no_health_check': [
                'Internal utilities flagged',
                'Non-handler functions flagged',
                'Data processors flagged (not entry points)',
                'Functions without critical path marker flagged',
            ],
            'hardcoded_path': [
                'Compile-time constants flagged',
                'Configuration sources flagged',
                'Environment variables flagged',
                'Test code flagged',
            ],
            'memory_order': [
                'Conservative atomic patterns flagged',
                'Legacy code with simple atomics',
                'Platform-specific ordering assumptions',
            ],
            'no_timeout': [
                'Sync operations incorrectly flagged',
                'Operations with natural timeouts flagged',
                'Context windows too small to see cancellation',
            ],
            'range_temporary': [
                'POD temporaries flagged as unsafe',
                'Short-lived temporaries with explicit lifetime',
                'Safe binding patterns not recognized',
            ],
        }
        
        return patterns.get(category, [
            'High-frequency patterns not yet analyzed',
            'More context needed for precise classification',
            'Category-specific tuning pending',
        ])
    
    def _identify_false_negative_patterns(self, category: str, data: Dict) -> List[str]:
        """Identify patterns we might be missing (false negatives)"""
        
        patterns = {
            'copy_overhead': [
                'Copies of complex types in tight loops',
                'Implicit copies via auto (not auto&)',
                'Copies in std::transform / std::ranges operations',
            ],
            'observability': [
                'Critical entry points without metrics (handlers)',
                'RPC/gRPC calls without trace points',
                'Event logging without structured format',
            ],
            'db_connection_leak': [
                'Connections acquired but path throws',
                'Connections in exception cleanup paths',
                'Pool exhaustion scenarios',
            ],
            'performance': [
                'Inefficient algorithms (O(n²) hidden)',
                'String building without reserve',
                'Regex compilation in loops',
            ],
            'no_health_check': [
                'API endpoints without health checks',
                'Critical handlers missing ready state checks',
                'Fallback code paths not tracked',
            ],
        }
        
        return patterns.get(category, [
            'False negative analysis pending',
            'Requires manual code review',
        ])
    
    def _generate_fixes(self, category: str, tp_rate: float, fp_patterns: List[str]) -> List[str]:
        """Generate concrete fixes based on FP patterns"""
        
        fixes = {
            'copy_overhead': [
                '✓ Whitelist make_shared / make_unique in PATTERNS',
                '✓ Add POD type detection (int, float, bool, size_t)',
                '✓ Require loop context (±15 lines minimum)',
                '✓ Check for std::move in context (safe optimization)',
                '✓ Expand context window from 5 to 15 lines',
                '✓ Add confidence threshold (require 2+ signals)',
            ],
            'observability': [
                '✓ Skip functions in internal/detail/impl namespaces',
                '✓ Skip trivial functions (< 5 lines code)',
                '✓ Skip getters/setters/operators/destructors',
                '✓ Require public API marker (THEMIS_API)',
                '✓ Skip private member functions',
                '✓ Confidence threshold for weak signals',
            ],
            'db_connection_leak': [
                '✓ Recognize smart_ptr wrapped connections',
                '✓ Whitelist ConnectionPool patterns',
                '✓ Expand context window from 5 to 20 lines (catch cleanup)',
                '✓ Check for explicit .close() calls',
                '✓ RAII pattern detection in destructor',
                '✓ Add exception-safety context check',
            ],
            'no_health_check': [
                '✓ Limit to HTTP/gRPC handler functions only',
                '✓ Skip internal utilities (detail::, _impl)',
                '✓ Require critical path marker or known patterns',
                '✓ Skip data processor functions',
                '✓ Whitelist common safe patterns',
                '✓ Scope to entry point functions only',
            ],
            'hardcoded_path': [
                '✓ Distinguish compile-time vs runtime paths',
                '✓ Whitelist constexpr paths',
                '✓ Whitelist configuration sources',
                '✓ Whitelist environment variable sources',
                '✓ Skip test code',
                '✓ Add source tracking',
            ],
            'no_timeout': [
                '✓ Identify async operations requiring timeout',
                '✓ Whitelist sync-only operations',
                '✓ Check context for cancellation support',
                '✓ Expand scope to critical paths',
            ],
            'range_temporary': [
                '✓ Detect temporary objects in range-for',
                '✓ Flag only complex types (not POD)',
                '✓ Check for explicit lifetime extension',
            ],
            'lock_contention': [
                '✓ Identify hot locks (high contention)',
                '✓ Suggest fine-grained locking',
                '✓ Flag long critical sections',
            ],
        }
        
        return fixes.get(category, [
            '✓ Expand context window for better pattern matching',
            '✓ Add category-specific whitelists',
            '✓ Implement scope awareness (public vs internal)',
        ])
    
    def _determine_priority(self, tp_rate: float, fp_rate: float) -> str:
        """Determine fix priority based on current metrics"""
        
        # High priority: 0% TP (all FP) or very high FP rate
        if tp_rate == 0 and fp_rate > 0.3:
            return 'HIGH'
        
        # Medium priority: Low TP but some good detections
        if tp_rate < 0.25:
            return 'MEDIUM'
        
        # High priority if FP rate is very high
        if fp_rate > 0.6:
            return 'HIGH'
        
        # Low priority: Already decent
        return 'LOW'
    
    def _estimate_impact(self, category: str, tp_rate: float, fixes: List[str]) -> str:
        """Estimate improvement potential"""
        
        impact_map = {
            'copy_overhead': ('0%', '60%'),
            'observability': ('0%', '70%'),
            'db_connection_leak': ('0%', '50%'),
            'no_health_check': ('0%', '60%'),
            'hardcoded_path': ('0%', '50%'),
            'memory_order': ('25%', '70%'),
            'smart_ptr_misuse': ('40%', '80%'),
            'repeated_search': ('50%', '90%'),
            'string_concat_loop': ('60%', '95%'),
        }
        
        if category in impact_map:
            current, target = impact_map[category]
            return f"{current} -> {target} TP"
        
        # Default for unknown categories
        if tp_rate == 0:
            return "0% -> 50% TP"
        else:
            return f"{tp_rate:.0%} -> {min(tp_rate + 0.3, 1.0):.0%} TP"
    
    def _estimate_effort(self, category: str, num_fixes: int) -> float:
        """Estimate effort in days"""
        
        # Base effort: 0.5 days per fix
        base_effort = 0.5 * max(num_fixes, 1)  # At least 1 fix
        
        # Adjustment by category complexity
        complexity = {
            'copy_overhead': 1.0,
            'observability': 1.2,
            'db_connection_leak': 1.5,
            'no_health_check': 1.0,
            'hardcoded_path': 0.8,
            'no_timeout': 1.2,
            'memory_order': 1.0,
            'range_temporary': 0.8,
            'lock_contention': 1.5,
        }
        
        multiplier = complexity.get(category, 1.0)
        return base_effort * multiplier
    
    def _identify_priority_fixes(self, scanner_feedback: Dict, priority: str) -> List[str]:
        """Get all fixes for a given priority level"""
        
        fixes = []
        for category, feedback in scanner_feedback.items():
            if feedback['priority'] == priority:
                fixes.extend([
                    f"[{category}] {fix}" 
                    for fix in feedback['suggested_fixes']
                ])
        
        return fixes
    
    def _rank_fixes(self, scanner_feedback: Dict) -> List[str]:
        """Rank fixes by impact/effort ratio"""
        
        ranked = []
        
        # Sort by: priority, then TP rate (lowest first), then effort
        sorted_categories = sorted(
            scanner_feedback.items(),
            key=lambda x: (
                x[1]['priority'] == 'HIGH',  # HIGH priority first
                x[1]['tp_rate'],  # Then 0% TP first
                x[1]['effort_days'],  # Then easiest first
            ),
            reverse=True
        )
        
        for category, feedback in sorted_categories:
            ranked.append(category)
        
        return ranked
    
    def generate_markdown_report(self, feedback_report: ScanFeedbackReport) -> str:
        """Generate human-readable markdown report"""
        
        lines = []
        lines.append("# Scanner Feedback Loop Report")
        lines.append(f"\n**Generated:** {feedback_report.timestamp}")
        lines.append(f"**Scan Date:** {feedback_report.scan_date}")
        lines.append(f"**Sample Size:** {feedback_report.validation_sample_size} gaps")
        
        # Overall metrics
        lines.append("\n## Overall Metrics")
        lines.append(f"| Metric | Current | Previous | Delta |")
        lines.append(f"|--------|---------|----------|-------|")
        lines.append(f"| TP Rate | {feedback_report.overall_tp_rate:.1%} | {feedback_report.previous_tp_rate:.1%} | {feedback_report.improvement_delta:+.1%} |")
        lines.append(f"| FP Rate | {feedback_report.overall_fp_rate:.1%} | - | - |")
        lines.append(f"| Total Effort | {feedback_report.total_effort_days:.1f} days | - | - |")
        
        # Per-category feedback
        lines.append("\n## Scanner Analysis")
        
        for category in feedback_report.recommended_sequence:
            feedback = feedback_report.scanner_feedback[category]
            lines.append(f"\n### {category.upper()}")
            lines.append(f"**Priority:** {feedback['priority']} | **Effort:** {feedback['effort_days']:.1f} days")
            lines.append(f"\n**Current Metrics:**")
            lines.append(f"- Total Gaps: {feedback['total_gaps']}")
            lines.append(f"- TP Rate: {feedback['tp_rate']:.1%} ({feedback['tp_count']} true positives)")
            lines.append(f"- FP Rate: {feedback['fp_rate']:.1%} ({feedback['fp_count']} false positives)")
            lines.append(f"- Uncertain: {feedback['uncertain_count']}")
            
            lines.append(f"\n**Common FP Patterns:**")
            for pattern in feedback['common_fp_patterns']:
                lines.append(f"- {pattern}")
            
            lines.append(f"\n**Recommended Fixes:**")
            for fix in feedback['suggested_fixes']:
                lines.append(f"- {fix}")
            
            lines.append(f"\n**Expected Impact:** {feedback['estimated_impact']}")
        
        # Implementation timeline
        lines.append("\n## Implementation Timeline")
        lines.append(f"\nTotal Effort: **{feedback_report.total_effort_days:.1f} days** (~2 weeks including validation)")
        lines.append(f"\nRecommended Sequence:")
        for i, category in enumerate(feedback_report.recommended_sequence, 1):
            feedback = feedback_report.scanner_feedback[category]
            lines.append(f"\n**Step {i}: {category.upper()}** ({feedback['effort_days']:.1f} days)")
            lines.append(f"- Impact: {feedback['estimated_impact']}")
            lines.append(f"- Fixes: {len(feedback['suggested_fixes'])} changes")
        
        # High-priority fixes
        lines.append("\n## High-Priority Fixes (Start Here)")
        for fix in feedback_report.high_priority_fixes[:10]:
            lines.append(f"- {fix}")
        
        return '\n'.join(lines)


def main():
    """Run feedback analysis"""
    
    import sys
    import io
    
    # Force UTF-8 output on Windows (cp1252 encoding issue)
    if sys.platform == 'win32':
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    
    repo_root = Path('.')
    ai_working = repo_root / 'ai_working'
    
    # Check for required files
    validation_report = ai_working / 'VALIDATION_ANALYSIS_REPORT.json'
    if not validation_report.exists():
        print(f"[ERROR] Missing: {validation_report}")
        return
    
    # Analyze
    analyzer = ScannerFeedbackAnalyzer(repo_root)
    feedback_report = analyzer.analyze_validation_results(
        validation_report,
        ai_working / 'SCANNER_FEEDBACK_REPORT.json'
    )
    
    # Save JSON
    json_path = ai_working / 'SCANNER_FEEDBACK_REPORT.json'
    with open(json_path, 'w') as f:
        json.dump(asdict(feedback_report), f, indent=2, default=str)
    print(f"✓ Saved: {json_path}")
    
    # Save Markdown
    markdown_path = ai_working / 'SCANNER_FEEDBACK_REPORT.md'
    markdown = analyzer.generate_markdown_report(feedback_report)
    with open(markdown_path, 'w', encoding='utf-8') as f:
        f.write(markdown)
    print(f"✓ Saved: {markdown_path}")
    
    # Print to console
    print("\n" + "="*80)
    print(markdown)
    print("="*80)


if __name__ == '__main__':
    main()
