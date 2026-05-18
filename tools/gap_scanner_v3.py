#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Unified Orchestrator

Runs all Phase 1 scanners (Security, Memory, Reliability) and produces:
- Aggregate report (gap_scan_v3_aggregate.json)
- Module-level reports (gap_scan_v3_<module>.json)
- Summary statistics (gap_scan_v3_summary.json)
"""

import json
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, Any

# Import Phase 1 scanners
from gap_scanner_v3_security import SecurityGapScanner
from gap_scanner_v3_memory import MemoryGapScanner
from gap_scanner_v3_reliability import ReliabilityGapScanner


class UnifiedGapScannerV3:
    """Orchestrate Phase 1 security, memory, reliability scanning"""
    
    def __init__(self, repo_root: str = '.', output_dir: str = 'ai_working'):
        self.repo_root = Path(repo_root)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
    
    def run_complete_scan(self) -> Dict[str, Any]:
        """Execute Phase 1 security + memory + reliability scan"""
        
        print("\n" + "=" * 80)
        print("ThemisDB Gap Scanner v3 — Phase 1 (Security + Memory + Reliability)")
        print("=" * 80)
        
        results = {}
        
        # Run Security scanner
        print("\n[1/3] Security Gap Scanner")
        print("-" * 80)
        security_scanner = SecurityGapScanner(str(self.repo_root))
        security_results = security_scanner.run_full_scan(str(self.output_dir))
        results['security'] = security_results
        
        # Run Memory scanner
        print("\n[2/3] Memory Safety Gap Scanner")
        print("-" * 80)
        memory_scanner = MemoryGapScanner(str(self.repo_root))
        memory_results = memory_scanner.run_full_scan(str(self.output_dir))
        results['memory'] = memory_results
        
        # Run Reliability scanner
        print("\n[3/3] Reliability Gap Scanner")
        print("-" * 80)
        reliability_scanner = ReliabilityGapScanner(str(self.repo_root))
        reliability_results = reliability_scanner.run_full_scan(str(self.output_dir))
        results['reliability'] = reliability_results
        
        # Aggregate results
        print("\n[...] Aggregating results...")
        aggregate = self._aggregate_results(results)
        
        # Save aggregates
        self._save_aggregate(aggregate)
        self._save_module_reports(aggregate)
        self._save_summary(aggregate)
        
        print("\n[OK] Phase 1 scan complete!")
        print(f"\n[INFO] Results saved to: {self.output_dir}/")
        
        return aggregate
    
    def _aggregate_results(self, results: Dict[str, Dict]) -> Dict[str, Any]:
        """Combine results from all 3 scanners"""
        
        modules = {}
        
        for scanner_type, scanner_results in results.items():
            for module, module_data in scanner_results.items():
                if module not in modules:
                    modules[module] = {
                        'total': 0,
                        'severity_critical': 0,
                        'severity_high': 0,
                        'severity_medium': 0,
                        'by_category': {},
                        'by_file': {}
                    }
                
                # Aggregate counts
                modules[module]['total'] += module_data.get('total', 0)
                modules[module]['severity_critical'] += module_data.get('severity_critical', 0)
                modules[module]['severity_high'] += module_data.get('severity_high', 0)
                modules[module]['severity_medium'] += module_data.get('severity_medium', 0)
                
                # Track by category (scanner type)
                if scanner_type not in modules[module]['by_category']:
                    modules[module]['by_category'][scanner_type] = 0
                modules[module]['by_category'][scanner_type] += module_data.get('total', 0)
                
                # Merge file-level data
                for file_path, gaps in module_data.get('gaps_by_file', {}).items():
                    if file_path not in modules[module]['by_file']:
                        modules[module]['by_file'][file_path] = []
                    modules[module]['by_file'][file_path].extend(gaps)
        
        return modules
    
    def _save_aggregate(self, aggregate: Dict[str, Any]):
        """Save main aggregate file"""
        output_file = self.output_dir / 'gap_scan_v3_aggregate.json'
        with open(output_file, 'w') as f:
            json.dump(aggregate, f, indent=2)
        print(f"[OK] Saved: {output_file.name}")
    
    def _save_module_reports(self, aggregate: Dict[str, Any]):
        """Save per-module reports"""
        for module, data in aggregate.items():
            output_file = self.output_dir / f'gap_scan_v3_{module}.json'
            with open(output_file, 'w') as f:
                json.dump({module: data}, f, indent=2)
        print(f"[OK] Saved: {len(aggregate)} module reports")
    
    def _save_summary(self, aggregate: Dict[str, Any]):
        """Generate and save summary statistics"""
        
        total_gaps = sum(m.get('total', 0) for m in aggregate.values())
        critical = sum(m.get('severity_critical', 0) for m in aggregate.values())
        high = sum(m.get('severity_high', 0) for m in aggregate.values())
        medium = sum(m.get('severity_medium', 0) for m in aggregate.values())
        
        # Category breakdown
        category_totals = {'security': 0, 'memory': 0, 'reliability': 0}
        for module_data in aggregate.values():
            for cat, count in module_data.get('by_category', {}).items():
                if cat in category_totals:
                    category_totals[cat] += count
        
        # Module ranking
        module_ranking = sorted(
            [(m, d.get('total', 0)) for m, d in aggregate.items()],
            key=lambda x: x[1],
            reverse=True
        )
        
        summary = {
            'scan_date': datetime.now().isoformat(),
            'phase': 'Phase 1 (Security + Memory + Reliability)',
            'total_gaps': total_gaps,
            'by_severity': {
                'critical': critical,
                'high': high,
                'medium': medium,
                'actionable': critical + high  # Critical + High = need fixing
            },
            'by_category': category_totals,
            'modules_scanned': len(aggregate),
            'top_modules': [
                {'module': m, 'gaps': c}
                for m, c in module_ranking[:10]
            ],
            'implementation_effort': self._estimate_effort(critical, high)
        }
        
        output_file = self.output_dir / 'gap_scan_v3_summary.json'
        with open(output_file, 'w') as f:
            json.dump(summary, f, indent=2)
        
        # Print to console
        print(f"\n[SUMMARY] Phase 1 Gap Analysis")
        print("=" * 80)
        print(f"  Total Gaps Found:        {total_gaps}")
        print(f"  CRITICAL Severity:       {critical}")
        print(f"  HIGH Severity:           {high}")
        print(f"  MEDIUM Severity:         {medium}")
        print(f"  ACTIONABLE (C+H):        {critical + high}")
        print()
        print(f"  Security Gaps:           {category_totals.get('security', 0)}")
        print(f"  Memory Safety Gaps:      {category_totals.get('memory', 0)}")
        print(f"  Reliability Gaps:        {category_totals.get('reliability', 0)}")
        print()
        print(f"  Modules Scanned:         {len(aggregate)}")
        print()
        print(f"  Top 5 Modules by Gap Count:")
        for i, (m, c) in enumerate(module_ranking[:5], 1):
            print(f"    {i}. {m:30} {c:4} gaps")
        print()
        print(f"  Estimated Effort:        {summary['implementation_effort']}")
        print("=" * 80)
        
        print(f"[OK] Saved: {output_file.name}")
    
    def _estimate_effort(self, critical: int, high: int) -> str:
        """Estimate dev effort needed to fix gaps"""
        
        actionable = critical + high
        
        # Rough estimates: 2 hours per critical, 1 hour per high
        hours = (critical * 2) + (high * 1)
        days = hours / 8
        
        if days < 1:
            return f"{int(hours)} hours"
        elif days < 5:
            return f"{days:.1f} days ({int(hours)} hours)"
        else:
            weeks = days / 5
            return f"{weeks:.1f} weeks ({int(days)} days)"


def main():
    """Main entry point"""
    
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    
    scanner = UnifiedGapScannerV3(repo_root, output_dir)
    aggregate = scanner.run_complete_scan()
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
