#!/usr/bin/env python3
"""
Automated validation sample analysis (TP/FP assessment)
Loads 50 gaps, analyzes with semantic heuristics, generates assessment report
"""

import json
import argparse
from pathlib import Path
from collections import defaultdict
import re
from datetime import datetime, timezone
from typing import Dict, List, Tuple, Optional

BASELINE_POLICY = {
    'tp_percent': 24.0,
    'critical_tp_percent': 50.0,
    'priority_categories': ['legacy_duplication', 'smart_ptr_misuse', 'memory_order', 'uncaught_exception'],
    'deferred_high_fp_categories': ['observability', 'copy_overhead', 'db_connection_leak', 'no_health_check', 'hardcoded_path'],
    'priority_modules': ['llm', 'server', 'sharding'],
}

class GapAnalyzer:
    """Analyzes gaps for TP vs FP classification"""
    
    def __init__(self, repo_root: Path):
        self.repo_root = repo_root
        self.cache = {}
        
    def load_sample_gaps(self, metadata_path: Path) -> List[Dict]:
        """Load gaps from sample metadata"""
        if not metadata_path.exists():
            return []
        
        with open(metadata_path) as f:
            data = json.load(f)
        return data.get('gaps', [])
    
    def get_source_context(self, file_path: str, line_num: int, context_lines: int = 20) -> str:
        """Load actual source code with extended context"""
        try:
            full_path = self.repo_root / file_path
            if not full_path.exists():
                return "[FILE NOT FOUND]"
            
            with open(full_path, encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
            
            if line_num <= 0 or line_num > len(lines):
                return f"[LINE {line_num} OUT OF RANGE (file has {len(lines)} lines)]"
            
            start = max(0, line_num - context_lines - 1)
            end = min(len(lines), line_num + context_lines)
            
            context = []
            for i in range(start, end):
                marker = ">>>" if (i + 1) == line_num else "   "
                context.append(f"{i+1:5d} {marker} {lines[i].rstrip()}")
            
            return "\n".join(context)
        except Exception as e:
            return f"[ERROR: {str(e)[:100]}]"
    
    def analyze_gap(self, gap: Dict) -> Tuple[str, float, str]:
        """
        Analyze single gap and return (classification, confidence, reasoning)
        Returns: ('TP', confidence, reasoning) or ('FP', confidence, reasoning)
        """
        category = gap.get('category', 'unknown')
        severity = gap.get('severity', 'MEDIUM')
        file_path = gap.get('_file', gap.get('file', 'unknown'))
        line_num = gap.get('line', 0)
        description = gap.get('description', '')
        
        # Load actual code context
        context = self.get_source_context(file_path, line_num, context_lines=30)
        
        # Apply heuristic rules based on category
        if category == 'no_health_check':
            return self._analyze_health_check(context, description, severity)
        elif category == 'no_timeout':
            return self._analyze_timeout(context, description, severity)
        elif category == 'db_connection_leak':
            return self._analyze_resource_leak(context, description, severity)
        elif category == 'data_race':
            return self._analyze_data_race(context, description, severity)
        elif category == 'null_dereference':
            return self._analyze_null_dereference(context, description, severity)
        elif category == 'pointer_arithmetic':
            return self._analyze_pointer_arithmetic(context, description, severity)
        elif category == 'no_retry_logic':
            return self._analyze_retry_logic(context, description, severity)
        elif category == 'llm_ai_safety':
            return self._analyze_llm_safety(context, description, severity)
        elif category == 'performance':
            return self._analyze_performance(context, description, severity)
        elif category == 'observability':
            return self._analyze_observability(context, description, severity)
        elif category == 'uncaught_exception':
            return self._analyze_exception_handling(context, description, severity)
        elif category == 'legacy_duplication':
            return self._analyze_legacy(context, description, severity)
        elif category in ['string_concat_loop', 'o_n_squared', 'repeated_lookup', 'repeated_search']:
            return self._analyze_algorithm(context, description, severity)
        elif category == 'smart_ptr_misuse':
            return self._analyze_smart_ptr(context, description, severity)
        elif category in ['memory_order', 'determinism', 'copy_overhead']:
            return self._analyze_concurrency(context, description, severity)
        else:
            # Generic heuristic for unknown categories
            return self._generic_analysis(context, description, severity)
    
    def _analyze_health_check(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """no_health_check category"""
        # Real issue if Status is a field that should be initialized/monitored
        if 'status' in context.lower() and ('UNKNOWN' in context or 'enum' in context):
            return ('TP', 0.65, "Status field with enum/UNKNOWN state pattern suggests real health check need")
        if 'createErrorResponse' in context:
            return ('FP', 0.85, "Status used in response creation, not internal state")
        return ('?', 0.5, "Inconclusive without full class definition")
    
    def _analyze_timeout(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """no_timeout category - mutex/semaphore locks"""
        if 'tg2.wait()' in context or 'wait()' in context:
            if 'TaskGroup' in context or 'thread_group' in context:
                return ('TP', 0.85, "TaskGroup::wait() without timeout can block indefinitely")
            if 'for' not in context[:context.find('wait()')]:
                return ('TP', 0.70, "Unguarded wait() without timeout")
        if 'std::lock_guard' in context or 'std::unique_lock' in context:
            if 'try_lock' in context or '_for(' in context:
                return ('TP', 0.75, "Potential deadlock in lock acquisition")
        return ('FP', 0.60, "Wait/lock has implicit timeout or is in try-catch block")
    
    def _analyze_resource_leak(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """db_connection_leak / resource leak"""
        # Check for proper RAII patterns
        if 'std::lock_guard' in context or 'std::unique_lock' in context:
            return ('FP', 0.80, "RAII lock guard present, no leak")
        if 'std::make_shared' in context or 'std::unique_ptr' in context:
            return ('FP', 0.85, "Smart pointer used, automatic cleanup")
        if '.release()' in context or '.reset()' in context:
            return ('FP', 0.75, "Explicit release/reset pattern")
        if 'delete' in context and '->' not in context:
            return ('TP', 0.70, "Raw delete without corresponding new visible")
        return ('?', 0.5, "Need to see allocation/deallocation pairing")
    
    def _analyze_data_race(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """shared data access without lock"""
        if 'std::make_shared' in context:
            return ('FP', 0.85, "make_shared is thread-safe construction, not racy")
        if 'metrics_vec' in context and 'for' in context:
            # Check if there's a lock
            if 'lock' in context.lower():
                return ('FP', 0.80, "Locked iteration over shared vector")
            return ('TP', 0.70, "Unprotected iteration over shared metrics")
        return ('?', 0.5, "Need visibility of synchronization context")
    
    def _analyze_null_dereference(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """potential null pointer dereference"""
        if '?' in context and 'str' in context:
            # e.g., reply->str ? reply->str : "unknown"
            return ('FP', 0.85, "Ternary guard before use")
        if 'if (' in context and '->' in context:
            lines = context.split('\n')
            for i, line in enumerate(lines):
                if '->' in line and i > 0 and 'if' in lines[i-1]:
                    return ('FP', 0.80, "Null check on previous line")
        return ('TP', 0.60, "Potential null dereference without visible guard")
    
    def _analyze_pointer_arithmetic(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """pointer/array access without bounds"""
        if 'metadata[' in context and '=' in context:
            return ('FP', 0.85, "Map access is bounds-safe (not raw array)")
        if 'std::vector' in context and '.at(' in context:
            return ('FP', 0.88, ".at() throws on out-of-bounds")
        if 'match.match_method' in context:
            return ('FP', 0.80, "Struct field access, not pointer arithmetic")
        return ('?', 0.5, "Need to verify if access is on raw pointer or container")
    
    def _analyze_retry_logic(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """no retry logic for transient failures"""
        if 'execute(' in context and 'plan' in context:
            return ('FP', 0.70, "Query execution without visible retry is common pattern")
        if 'return' in context and 'Result<' in context:
            return ('FP', 0.65, "Returning Result type, caller can retry")
        return ('?', 0.5, "Depends on caller's retry strategy")
    
    def _analyze_llm_safety(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """LLM injection, hallucination, token limit risks"""
        if 'generate(' in context and 'wrapper_' in context:
            if 'token_limit' in context or 'timeout' in context:
                return ('FP', 0.80, "Token limit/timeout configured")
            return ('TP', 0.75, "LLM generation without visible safeguards")
        if 'upload' in context and 'weights' in context:
            return ('FP', 0.85, "Weight upload is not LLM input injection")
        if 'hipLaunchKernelGGL' in context:
            return ('FP', 0.90, "GPU kernel launch, not LLM input")
        return ('?', 0.5, "Unclear if this is actual LLM inference path")
    
    def _analyze_performance(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """O(n²), missing reserve, string concat, etc."""
        if 'find(' in context and 'for' in context:
            if 'std::unordered_set' in context or '.count(' in context:
                return ('FP', 0.75, "Set lookup is O(1), not O(n²)")
            if 'text_lower.find(word)' in context:
                return ('TP', 0.70, "String search in loop is O(n*m)")
        if 'std::vector' in context and '<<' in context:
            return ('TP', 0.65, "String concatenation in loop")
        if 'reserve(' not in context and 'push_back' in context:
            return ('FP', 0.60, "Vector push_back without reserve is acceptable pattern")
        return ('?', 0.5, "Need algorithm analysis")
    
    def _analyze_observability(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """missing trace/log points"""
        if 'THEMIS_INFO' in context or 'THEMIS_WARN' in context or 'spdlog' in context:
            return ('FP', 0.80, "Logging already present")
        if 'function' in context.lower() and 'allocate' in context:
            return ('TP', 0.60, "Critical function without observable tracing")
        return ('?', 0.5, "Depends on tracing infrastructure usage")
    
    def _analyze_exception_handling(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """generic catch or uncaught exceptions"""
        if 'catch (...)' in context:
            if 'return' in context and 'fallback' in context:
                return ('FP', 0.80, "Catch-all with graceful fallback is acceptable")
            return ('TP', 0.75, "Generic catch(...) masks specific error types")
        if 'throw' in context:
            if 'try' in context or 'noexcept' not in context:
                return ('FP', 0.70, "Exception thrown in try block or non-noexcept context")
            return ('TP', 0.65, "Exception thrown in noexcept function")
        return ('?', 0.5, "Need exception context")
    
    def _analyze_legacy(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """legacy paths, deprecated APIs"""
        if '/v2/changes' in context or 'legacy' in context.lower():
            return ('TP', 0.80, "Explicitly marked legacy code")
        return ('FP', 0.70, "Legacy comment without actual deprecated code")
    
    def _analyze_algorithm(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """O(n²) patterns, repeated searches"""
        if 'std::find(' in context and 'for' in context:
            return ('TP', 0.75, "std::find in loop is O(n²)")
        if 'out +=' in context and 'for' in context:
            return ('TP', 0.80, "String concatenation in loop is O(n²)")
        return ('FP', 0.60, "Pattern not actually problematic")
    
    def _analyze_smart_ptr(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """smart pointer misuse"""
        if 'new' in context and 'make_unique' not in context and 'std::unique_ptr<' not in context:
            return ('TP', 0.70, "Raw new without immediate smart pointer wrapping")
        if 'std::make_shared' in context or 'std::make_unique' in context:
            return ('FP', 0.85, "Correct smart pointer usage")
        return ('?', 0.5, "Need to see pointer initialization")
    
    def _analyze_concurrency(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """memory_order, determinism, copy overhead"""
        if 'memory_order_relaxed' in context and 'lock_guard' in context:
            return ('TP', 0.70, "Relaxed atomics with mutex may mask synchronization")
        if 'unordered_map' in context and 'iteration' in context.lower():
            return ('TP', 0.65, "unordered_map iteration is non-deterministic")
        if 'memcpy' in context and 'delta_bytes' in context:
            return ('FP', 0.80, "memcpy of fixed-size value is fine")
        return ('?', 0.5, "Concurrency pattern unclear")
    
    def _generic_analysis(self, context: str, desc: str, severity: str) -> Tuple[str, float, str]:
        """Generic fallback analysis"""
        if severity == 'CRITICAL':
            return ('TP', 0.60, "CRITICAL severity suggests real issue")
        elif severity == 'HIGH':
            return ('?', 0.55, "HIGH severity warrants investigation")
        else:
            return ('FP', 0.50, "MEDIUM severity likely false positive or low-priority")

def _build_remediation_backlog(assessments: List[Dict]) -> Dict[str, List[Dict]]:
    backlog = defaultdict(list)
    for item in assessments:
        if item['classification'] != 'TP':
            continue
        if str(item['severity']).upper() not in {'CRITICAL', 'HIGH'}:
            continue
        backlog[item['module']].append(item)

    # sort each module: CRITICAL first
    for module in list(backlog.keys()):
        backlog[module] = sorted(
            backlog[module],
            key=lambda a: (str(a['severity']).upper() != 'CRITICAL', a['category'], a['file'], a['line'])
        )
    return dict(backlog)


def _write_remediation_backlog(backlog: Dict[str, List[Dict]], output_path: Path):
    module_priority = BASELINE_POLICY['priority_modules']
    ordered_modules = [m for m in module_priority if m in backlog]
    ordered_modules.extend(sorted(m for m in backlog.keys() if m not in module_priority))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("# Validated Remediation Backlog\n\n")
        f.write("## Prioritized Module Order\n\n")
        for module in module_priority:
            f.write(f"- {module}\n")
        f.write("\n## Batch Exit Criteria\n\n")
        f.write("- No new CRITICAL findings\n")
        f.write("- Tests green (`release_critical` plus Wave 5/6 regressions)\n")
        f.write("- Failure/recovery semantics documented\n")
        f.write("- Wave 7 baseline stable; Wave 8/Fault-Injection as follow-up sign-off\n\n")

        for module in ordered_modules:
            f.write(f"## {module}\n\n")
            items = backlog[module]
            if not items:
                f.write("- Keine validierten CRITICAL/HIGH Findings\n\n")
                continue
            for item in items[:200]:
                f.write(
                    f"- [{item['severity']}] {item['category']} "
                    f"({item['file']}:{item['line']}) — {item['reasoning']}\n"
                )
            f.write("\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Analyze validation sample and derive remediation backlog")
    parser.add_argument("--repo-root", default=str(Path(__file__).parent.parent), help="Repository root")
    parser.add_argument("--metadata-file", default="ai_working/sample_validation_metadata.json", help="Validation sample metadata path")
    parser.add_argument("--report-out", default="ai_working/VALIDATION_ANALYSIS_REPORT.json", help="Detailed JSON output")
    parser.add_argument("--summary-out", default="ai_working/VALIDATION_ANALYSIS_SUMMARY.md", help="Markdown summary output")
    parser.add_argument("--backlog-out", default="ai_working/VALIDATED_REMEDIATION_BACKLOG.md", help="Validated remediation backlog output")
    return parser.parse_args()


def main():
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    metadata_file = (repo_root / args.metadata_file).resolve()
    report_file = (repo_root / args.report_out).resolve()
    md_file = (repo_root / args.summary_out).resolve()
    backlog_file = (repo_root / args.backlog_out).resolve()
    analyzer = GapAnalyzer(repo_root)
    
    print("\n" + "=" * 100)
    print("AUTOMATED GAP VALIDATION SAMPLE ANALYSIS")
    print("=" * 100)
    
    gaps = analyzer.load_sample_gaps(metadata_file)
    if not gaps:
        print("[ERROR] Could not load sample gaps")
        return
    
    print(f"\n[ANALYZING {len(gaps)} GAPS...]")
    print("=" * 100)
    
    assessments = []
    tp_count = 0
    fp_count = 0
    uncertain_count = 0
    
    for idx, gap in enumerate(gaps, 1):
        classification, confidence, reasoning = analyzer.analyze_gap(gap)
        
        module = gap.get('_module', 'unknown')
        severity = gap.get('severity', 'MEDIUM')
        category = gap.get('category', 'unknown')
        file_path = gap.get('_file', gap.get('file', 'unknown'))
        line_num = gap.get('line', 0)
        
        assessment = {
            'index': idx,
            'module': module,
            'file': file_path,
            'line': line_num,
            'severity': severity,
            'category': category,
            'classification': classification,
            'confidence': confidence,
            'reasoning': reasoning
        }
        assessments.append(assessment)
        
        # Update counters
        if classification == 'TP':
            tp_count += 1
        elif classification == 'FP':
            fp_count += 1
        else:
            uncertain_count += 1
        
        # Progress output
        status_symbol = '✓' if classification == 'TP' else ('✗' if classification == 'FP' else '?')
        conf_str = f"{confidence:.0%}"
        print(f"[{idx:2d}/50] {status_symbol} {module:15s} | {severity:8s} | {category:20s} | {conf_str}")
    
    print("\n" + "=" * 100)
    print("ANALYSIS RESULTS")
    print("=" * 100)
    
    total = len(assessments)
    tp_pct = (tp_count / total * 100) if total > 0 else 0
    fp_pct = (fp_count / total * 100) if total > 0 else 0
    unc_pct = (uncertain_count / total * 100) if total > 0 else 0
    
    print(f"\n[CLASSIFICATION SUMMARY]")
    print(f"  True Positives (TP):   {tp_count:3d} ({tp_pct:5.1f}%)")
    print(f"  False Positives (FP):  {fp_count:3d} ({fp_pct:5.1f}%)")
    print(f"  Uncertain (?):         {uncertain_count:3d} ({unc_pct:5.1f}%)")
    
    # Extrapolate to full dataset
    print(f"\n[EXTRAPOLATION TO 18,795 FULL GAPS]")
    total_gaps = 18795
    estimated_tp = int(tp_pct / 100 * total_gaps)
    estimated_fp = int(fp_pct / 100 * total_gaps)
    estimated_uncertain = int(unc_pct / 100 * total_gaps)
    
    print(f"  Estimated TP in full set:  {estimated_tp:,} ({tp_pct:.1f}%)")
    print(f"  Estimated FP in full set:  {estimated_fp:,} ({fp_pct:.1f}%)")
    print(f"  Estimated Uncertain:       {estimated_uncertain:,} ({unc_pct:.1f}%)")
    
    # Breakdown by category
    print(f"\n[BREAKDOWN BY CATEGORY]")
    by_category = defaultdict(lambda: {'TP': 0, 'FP': 0, '?': 0})
    for a in assessments:
        by_category[a['category']][a['classification']] += 1
    
    for cat in sorted(by_category.keys()):
        counts = by_category[cat]
        total_cat = sum(counts.values())
        tp_cat = counts['TP']
        tp_cat_pct = (tp_cat / total_cat * 100) if total_cat > 0 else 0
        print(f"  {cat:20s}: {tp_cat}/{total_cat} TP ({tp_cat_pct:5.1f}%)")
    
    # Breakdown by severity
    print(f"\n[BREAKDOWN BY SEVERITY]")
    by_severity = defaultdict(lambda: {'TP': 0, 'FP': 0, '?': 0})
    for a in assessments:
        by_severity[a['severity']][a['classification']] += 1
    
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM']:
        if sev in by_severity:
            counts = by_severity[sev]
            total_sev = sum(counts.values())
            tp_sev = counts['TP']
            tp_sev_pct = (tp_sev / total_sev * 100) if total_sev > 0 else 0
            print(f"  {sev:8s}: {tp_sev}/{total_sev} TP ({tp_sev_pct:5.1f}%)")
    
    # Save detailed report
    report_file.parent.mkdir(parents=True, exist_ok=True)
    with open(report_file, 'w') as f:
        json.dump({
            'date': datetime.now(timezone.utc).isoformat(),
            'baseline_policy': BASELINE_POLICY,
            'sample_size': len(assessments),
            'tp_count': tp_count,
            'tp_percent': tp_pct,
            'fp_count': fp_count,
            'fp_percent': fp_pct,
            'uncertain_count': uncertain_count,
            'uncertain_percent': unc_pct,
            'estimated_tp_in_18795': estimated_tp,
            'estimated_fp_in_18795': estimated_fp,
            'by_category': dict(by_category),
            'by_severity': dict(by_severity),
            'assessments': assessments
        }, f, indent=2)
    
    print(f"\n[✓] Detailed report saved to: {report_file}")
    
    # Generate markdown summary
    md_file.parent.mkdir(parents=True, exist_ok=True)
    with open(md_file, 'w', encoding='utf-8') as f:
        f.write("# Automated Gap Validation Analysis Summary\n\n")
        f.write(f"**Date:** {datetime.now(timezone.utc).isoformat()}\n")
        f.write(f"**Sample Size:** {len(assessments)} gaps\n")
        f.write(f"**Full Dataset:** 18,795 gaps\n\n")
        f.write("## Frozen Baseline\n\n")
        f.write(f"- Baseline TP: {BASELINE_POLICY['tp_percent']}%\n")
        f.write(f"- CRITICAL TP baseline: {BASELINE_POLICY['critical_tp_percent']}%\n")
        f.write(f"- Priority categories: {', '.join(BASELINE_POLICY['priority_categories'])}\n")
        f.write(f"- Deferred high-FP categories: {', '.join(BASELINE_POLICY['deferred_high_fp_categories'])}\n\n")
        
        f.write("## Classification Results\n\n")
        f.write(f"| Classification | Count | % | Est. in Full Set |\n")
        f.write(f"|---|---|---|---|\n")
        f.write(f"| **True Positives** | {tp_count} | {tp_pct:.1f}% | {estimated_tp:,} |\n")
        f.write(f"| **False Positives** | {fp_count} | {fp_pct:.1f}% | {estimated_fp:,} |\n")
        f.write(f"| **Uncertain** | {uncertain_count} | {unc_pct:.1f}% | {estimated_uncertain:,} |\n\n")
        
        f.write("## Interpretation\n\n")
        if tp_pct >= 70:
            f.write("✅ **HIGH QUALITY** - Most gaps are valid issues. Gap scanners v3 are producing reliable results.\n\n")
        elif tp_pct >= 50:
            f.write("⚠️ **MODERATE QUALITY** - About half are valid issues. Scanner tuning recommended for categories with <50% TP rate.\n\n")
        else:
            f.write("❌ **LOW QUALITY** - Most gaps are false positives. Significant scanner tuning needed.\n\n")
        
        f.write("## Recommendations\n\n")
        if fp_pct > 40:
            f.write("1. **Scanner Tuning Required:** FP rate exceeds 40%. Focus on high-FP categories:\n")
            worst_cats = sorted(by_category.items(), 
                               key=lambda x: x[1]['FP'] / (sum(x[1].values()) or 1), 
                               reverse=True)[:3]
            for cat, counts in worst_cats:
                total_cat = sum(counts.values())
                fp_rate = counts['FP'] / total_cat * 100
                f.write(f"   - `{cat}`: {counts['FP']}/{total_cat} FP ({fp_rate:.0f}%)\n")
            f.write("\n2. Investigate why these categories generate false positives\n")
            f.write("3. Modify detection logic or add filters\n")
            f.write("4. Re-run pipeline and validate again\n\n")
        else:
            f.write("1. **Acceptable FP Rate** (<40%) - Most gaps are actionable\n")
            f.write("2. Monitor high-severity gaps first (CRITICAL > HIGH > MEDIUM)\n")
            f.write("3. Consider sampling additional gaps from low-confidence categories\n\n")
        
        f.write("## Next Steps\n\n")
        f.write(f"1. Use TP rate ({tp_pct:.0f}%) as baseline quality metric\n")
        f.write(f"2. Prioritize ~{estimated_tp:,} estimated true positive gaps\n")
        f.write(f"3. Organize by module and severity for remediation planning\n")
        f.write(f"4. Track remediation progress\n\n")
        
        f.write("---\n\n")
        f.write("**Detailed assessment data:** See `VALIDATION_ANALYSIS_REPORT.json`\n")
    
    print(f"[✓] Summary saved to: {md_file}")
    backlog = _build_remediation_backlog(assessments)
    _write_remediation_backlog(backlog, backlog_file)
    print(f"[✓] Validated remediation backlog saved to: {backlog_file}")
    print("\n" + "=" * 100)
    print("✅ ANALYSIS COMPLETE")
    print("=" * 100 + "\n")

if __name__ == '__main__':
    main()
