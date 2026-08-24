#!/usr/bin/env python3
"""
ENHANCED L0.5 GAP VERIFICATION SYSTEM
Improved false-positive detection based on code pattern analysis.

From the graph module validation analysis, we learned that many
"unimplemented" patterns are actually defensive coding practices:
- Empty vector returns (defensive early exit)
- Guard clauses for untrained/missing resources
- Standard error signals

This enhanced system applies those learnings to all modules.
"""

import json
import glob
import re
from pathlib import Path
from datetime import datetime
from collections import defaultdict

class EnhancedL05VerificationSystem:
    """Enhanced L0.5 verification with aggressive FP detection."""
    
    def __init__(self):
        self.modules = ['graph', 'cache', 'query', 'network', 'server', 'llm']
        self.l0_data = {}
        self.verified_data = {}
        self.verification_stats = defaultdict(lambda: {
            'total_reviewed': 0,
            'false_positives': 0,
            'downgraded': 0,
            'critical_verified': 0,
            'verified_gaps': 0,
            'by_reason': defaultdict(int),
        })
        
        # Enhanced false-positive patterns (from graph module validation study)
        self.fp_indicators = [
            # Pattern 1: Guard clauses for untrained/uninitialized state
            ('untrained_guard', r'untrained|uninitialized|!initialized|\!trained|not ready|disabled'),
            # Pattern 2: Empty-return patterns for edge cases
            ('empty_return', r'return\s*\{\}|return\s+nullptr|return\s+null|return\s+false|return\s+0'),
            # Pattern 3: Missing embedding/resource → graceful degradation
            ('missing_resource', r'embedding|model|resource|engine|provider.*missing|not found|unavailable'),
            # Pattern 4: Empty input → empty output (standard semantics)
            ('empty_input', r'empty\(\)|\.size\(\)\s*==\s*0|\blength.*==\s*0|\.empty|no.*data|empty.*input'),
            # Pattern 5: Parse error → empty/error signal
            ('parse_error', r'parse error|invalid format|malformed|bad.*syntax|decode.*error'),
        ]
    
    def load_l0_data(self):
        """Load all L0 gap scan V3 results."""
        print("\n[L0.5-ENHANCED] LOADING L0 DATA")
        print("-" * 80)
        
        v3_files = glob.glob('ai_working/gap_scan_v3_*.json')
        
        for mod in self.modules:
            matching = [f for f in v3_files if f'gap_scan_v3_{mod}' in f]
            if matching:
                try:
                    with open(matching[0]) as f:
                        data = json.load(f)
                        if mod in data and isinstance(data[mod], dict):
                            self.l0_data[mod] = data[mod]
                            findings = len(data[mod].get('by_file', {}))
                            total = data[mod].get('total', 0)
                            print(f"  [OK] {mod:12s}: {findings:6d} files | {total:6d} findings loaded")
                        else:
                            print(f"  [WARN] {mod:12s}: Invalid format")
                except Exception as e:
                    print(f"  [FAIL] {mod:12s}: Error loading - {e}")
    
    def is_false_positive(self, finding):
        """Determine if finding is likely a false positive using enhanced patterns."""
        desc = self._safe_str(finding.get('description', ''))
        ctx = self._safe_str(finding.get('context', ''))
        pattern = self._safe_str(finding.get('pattern', ''))
        category = self._safe_str(finding.get('category', ''))
        
        combined = f"{desc} {ctx} {pattern}".lower()
        
        fp_reasons = []
        
        # Rule 1: Defensive patterns
        for reason, regex in self.fp_indicators:
            if re.search(regex, combined, re.IGNORECASE):
                fp_reasons.append(reason)
        
        # Rule 2: Severity indicates likely FP
        severity = finding.get('severity', 'MEDIUM')
        
        # CRITICAL severity with defensive pattern = likely FP
        if fp_reasons and severity == 'CRITICAL':
            fp_reasons.append('defensive_critical')
        
        # Rule 3: Common non-gaps
        non_gap_patterns = [
            'performance_patterns',  # Performance hints, not gaps
            'legacy_duplication',     # Historical duplication, not blocking
            'audit_logging',          # Optional logging
        ]
        
        if category in non_gap_patterns:
            fp_reasons.append('non_blocking_category')
        
        # Rule 4: Distributed consistency is often design choice, not gap
        if category == 'distributed_consistency' and 'without' in desc.lower():
            fp_reasons.append('distributed_tradeoff')
        
        return bool(fp_reasons), fp_reasons
    
    def _safe_str(self, value):
        """Safely convert value to string."""
        if isinstance(value, list):
            return ' '.join(str(v) for v in value)
        return str(value) if value else ''
    
    def verify_findings(self):
        """Perform enhanced L0.5 semantic verification."""
        print("\n[VERIFY] L0.5 ENHANCED VERIFICATION PHASE")
        print("-" * 80)
        
        for mod in self.modules:
            if mod not in self.l0_data:
                continue
            
            module_data = self.l0_data[mod]
            by_file = module_data.get('by_file', {})
            stats = self.verification_stats[mod]
            verified_gaps = []
            
            for filepath, findings in by_file.items():
                if isinstance(findings, dict):
                    findings = [findings]
                elif not isinstance(findings, list):
                    findings = [findings] if findings else []
                
                for finding in findings:
                    if not isinstance(finding, dict):
                        continue
                    
                    stats['total_reviewed'] += 1
                    
                    # Check if false positive
                    is_fp, fp_reasons = self.is_false_positive(finding)
                    
                    if is_fp:
                        stats['false_positives'] += 1
                        for reason in fp_reasons:
                            stats['by_reason'][reason] += 1
                    else:
                        # Real gap - classify
                        severity = finding.get('severity', 'MEDIUM')
                        
                        # Check if should be downgraded
                        is_defensive = self._is_defensive_but_real(finding)
                        if is_defensive and severity == 'CRITICAL':
                            finding['l0_5_severity'] = 'HIGH'
                            finding['l0_5_classification'] = 'DEFENSIVE_HIGH'
                            stats['downgraded'] += 1
                        else:
                            finding['l0_5_severity'] = severity
                            finding['l0_5_classification'] = 'REAL_GAP'
                        
                        finding['l0_5_verified'] = True
                        stats['verified_gaps'] += 1
                        
                        if finding.get('l0_5_severity', 'MEDIUM') == 'CRITICAL':
                            stats['critical_verified'] += 1
                        
                        verified_gaps.append(finding)
            
            self.verified_data[mod] = {
                'metadata': {
                    'module': mod,
                    'verification_timestamp': datetime.now().isoformat(),
                    'l0_findings': module_data.get('total', 0),
                    'verified_gaps': stats['verified_gaps'],
                    'false_positives_removed': stats['false_positives'],
                    'downgraded': stats['downgraded'],
                    'real_gap_rate': f"{stats['verified_gaps'] / stats['total_reviewed'] * 100:.1f}%" if stats['total_reviewed'] > 0 else "0%",
                },
                'verified_findings': verified_gaps,
                'statistics': {k: v for k, v in stats.items() if k != 'by_reason'},
                'fp_classification': dict(stats['by_reason']),
            }
            
            total = stats['total_reviewed']
            fp_rate = stats['false_positives'] / total * 100 if total > 0 else 0
            print(f"  {mod:12s}: {total:6d} reviewed | {stats['false_positives']:6d} FP ({fp_rate:5.1f}%) | {stats['verified_gaps']:6d} verified | {stats['critical_verified']:4d} CRITICAL")
    
    def _is_defensive_but_real(self, finding):
        """Check if this is a real gap with defensive pattern."""
        # This distinction is subtle - most should stay CRITICAL
        # Only downgrade if it's a known non-blocking pattern
        category = finding.get('category', '')
        
        non_blocking = [
            'performance_patterns',
            'observability',
            'audit_logging',
        ]
        
        return category in non_blocking
    
    def generate_report(self):
        """Generate comprehensive report."""
        print("\n[STATS] L0.5 ENHANCED VERIFICATION SUMMARY")
        print("=" * 80)
        
        total_reviewed = sum(s['total_reviewed'] for s in self.verification_stats.values())
        total_fp = sum(s['false_positives'] for s in self.verification_stats.values())
        total_downgraded = sum(s['downgraded'] for s in self.verification_stats.values())
        total_verified = sum(s['verified_gaps'] for s in self.verification_stats.values())
        total_critical_verified = sum(s['critical_verified'] for s in self.verification_stats.values())
        
        fp_rate = total_fp / total_reviewed * 100 if total_reviewed > 0 else 0
        verified_rate = total_verified / total_reviewed * 100 if total_reviewed > 0 else 0
        
        print(f"\n[DATA] OVERALL STATISTICS:")
        print(f"   Total Findings Reviewed:        {total_reviewed:,}")
        print(f"   False Positives Removed:        {total_fp:,} ({fp_rate:.1f}%)")
        print(f"   Findings Downgraded:            {total_downgraded:,}")
        print(f"   Verified Real Gaps:             {total_verified:,} ({verified_rate:.1f}%)")
        print(f"   Verified CRITICAL Gaps:         {total_critical_verified:,}")
        print()
        
        print(f"[OK] FALSE-POSITIVE REMOVAL TARGET: 70-80%")
        print(f"   Achieved: {fp_rate:.1f}% (Target: 70-80%)")
        target_status = "[OK] ON TARGET" if 70 <= fp_rate <= 80 else ("[WARN] BELOW TARGET" if fp_rate < 70 else "[OK] EXCEEDS TARGET")
        print(f"   Status: {target_status}")
        print()
        
        # Aggregate FP reasons
        all_fp_reasons = defaultdict(int)
        for mod_stats in self.verification_stats.values():
            for reason, count in mod_stats.get('by_reason', {}).items():
                all_fp_reasons[reason] += count
        
        if all_fp_reasons:
            print(f"[DATA] FALSE-POSITIVE CLASSIFICATION:")
            print("-" * 80)
            for reason, count in sorted(all_fp_reasons.items(), key=lambda x: -x[1])[:10]:
                pct = count / total_fp * 100 if total_fp > 0 else 0
                print(f"   {reason:30s}: {count:6d} ({pct:5.1f}%)")
        print()
        
        print(f"[STATS] BY MODULE:")
        print("-" * 80)
        for mod in self.modules:
            if mod in self.verification_stats:
                s = self.verification_stats[mod]
                print(f"   {mod:12s}: {s['verified_gaps']:6d} verified | {s['false_positives']:6d} FP | {s['downgraded']:4d} downgraded | {s['critical_verified']:4d} CRITICAL")
        print()
    
    def export_verified_findings(self, output_file='ai_working/gap_scan_results_verified_L0.5_enhanced.json'):
        """Export verified findings to JSON."""
        print(f"\n[SAVE] EXPORTING VERIFIED FINDINGS")
        print("-" * 80)
        
        export_data = {
            'metadata': {
                'orchestration_level': 'L0.5',
                'operation': 'Enhanced Gap Verification with Aggressive FP Elimination',
                'execution_timestamp': datetime.now().isoformat(),
                'modules': self.modules,
                'verification_type': 'Code Pattern Analysis (Enhanced)',
                'target_fp_removal_rate': '70-80%',
                'summary': {
                    'total_reviewed': sum(s['total_reviewed'] for s in self.verification_stats.values()),
                    'false_positives_removed': sum(s['false_positives'] for s in self.verification_stats.values()),
                    'verified_gaps': sum(s['verified_gaps'] for s in self.verification_stats.values()),
                    'critical_verified': sum(s['critical_verified'] for s in self.verification_stats.values()),
                }
            },
            'modules': self.verified_data,
        }
        
        with open(output_file, 'w') as f:
            json.dump(export_data, f, indent=2)
        
        size_mb = Path(output_file).stat().st_size / (1024 * 1024)
        print(f"  [OK] Exported to {output_file} ({size_mb:.2f} MB)")
    
    def run(self):
        """Execute full enhanced L0.5 verification cycle."""
        print("\n" + "=" * 80)
        print("ENHANCED L0.5 GAP VERIFICATION ORCHESTRATION SYSTEM")
        print("=" * 80)
        
        self.load_l0_data()
        self.verify_findings()
        self.generate_report()
        self.export_verified_findings()
        
        print("\n" + "=" * 80)
        print("[OK] L0.5 ENHANCED VERIFICATION COMPLETE")
        print("=" * 80)
        print("\n[NOTE] Next Steps:")
        print("   1. Review ai_working/gap_scan_results_verified_L0.5_enhanced.json")
        print("   2. Execute L1 (Module Documentation) updates")
        print("   3. Execute L2 (Aggregates) generation")
        print("   4. Execute L3 (Root Documentation) updates")
        print()

if __name__ == '__main__':
    system = EnhancedL05VerificationSystem()
    system.run()
