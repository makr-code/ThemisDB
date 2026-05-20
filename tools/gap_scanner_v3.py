#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Unified Orchestrator (Phase 1-10)

Runs all Phase 1-10 scanners:
- Phase 1-4: Security, Memory, Reliability, Concurrency, RAII, Container, Platform, Performance
- Phase 5: Type Conversion, Input Validation, Exception Safety, Uninitialized, OOP Design
- Phase 7: Audit Trail & Logging, Deprecated APIs
- Phase 8: Performance Patterns, GPU Memory Safety
- Phase 9: Query Correctness, Distributed Consistency, LLM/AI Safety
- Phase 10: Observability, Determinism

Produces:
- Aggregate report (gap_scan_v3_aggregate.json)
- Module-level reports (gap_scan_v3_<module>.json)
- Summary statistics (gap_scan_v3_summary.json)
"""

import json
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, Any

# Import Phase 1-4 scanners
from gap_scanner_v3_security import SecurityGapScanner
from gap_scanner_v3_memory import MemoryGapScanner
from gap_scanner_v3_reliability import ReliabilityGapScanner
from gap_scanner_v3_concurrency import ConcurrencyGapScanner
from gap_scanner_v3_raii import RAIIGapScanner
from gap_scanner_v3_container_misuse import ContainerGapScanner
from gap_scanner_v3_platform import PlatformGapScanner
from gap_scanner_v3_performance import PerformanceGapScanner

# Import Phase 5 scanners
from gap_scanner_v3_type_conversion import TypeConversionGapScanner
from gap_scanner_v3_input_validation import InputValidationGapScanner
from gap_scanner_v3_exception_safety import ExceptionSafetyGapScanner
from gap_scanner_v3_uninitialized import UninitializedGapScanner
from gap_scanner_v3_virtual_oop import OOPGapScanner

# Import Phase 7 scanners
from gap_scanner_v3_phase7_audit_logging import AuditLoggingScan
from gap_scanner_v3_phase7_deprecated_apis import DeprecatedAPIsScan

# Import Phase 8 scanners
from gap_scanner_v3_phase8_performance_patterns import PerformanceAntiPatternsScan
from gap_scanner_v3_phase8_gpu_memory import GPUMemorySafetyScan

# Import Phase 9 scanners
from gap_scanner_v3_phase9_query_correctness import QueryCorrectnessScan
from gap_scanner_v3_phase9_distributed_consistency import DistributedConsistencyScan
from gap_scanner_v3_phase9_llm_ai_safety import LLMAISafetyScan

# Import Phase 10 scanners
from gap_scanner_v3_phase10_observability import ObservabilityScan
from gap_scanner_v3_phase10_determinism import DeterminismScan


class UnifiedGapScannerV3:
    """Orchestrate Phase 1-4 security, memory, reliability, concurrency, RAII, container, platform & performance scanning"""
    
    def __init__(self, repo_root: str = '.', output_dir: str = 'ai_working'):
        self.repo_root = Path(repo_root)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
    
    def run_complete_scan(self) -> Dict[str, Any]:
        """Execute Phase 1-10 security + memory + reliability + concurrency + RAII + container + platform + performance + type_conversion + input_validation + audit + deprecated + perf_patterns + gpu_memory + query + distributed + llm + observability + determinism scan"""
        
        print("\n" + "=" * 80)
        print("ThemisDB Gap Scanner v3 — Phase 1-10 Complete Suite (27 Scanners)")
        print("=" * 80)
        
        results = {}
        
        # Run Security scanner
        print("\n[1/13] Security Gap Scanner")
        print("-" * 80)
        security_scanner = SecurityGapScanner(str(self.repo_root))
        security_results = security_scanner.run_full_scan(str(self.output_dir))
        results['security'] = security_results
        
        # Run Memory scanner
        print("\n[2/13] Memory Safety Gap Scanner")
        print("-" * 80)
        memory_scanner = MemoryGapScanner(str(self.repo_root))
        memory_results = memory_scanner.run_full_scan(str(self.output_dir))
        results['memory'] = memory_results
        
        # Run Reliability scanner
        print("\n[3/13] Reliability Gap Scanner")
        print("-" * 80)
        reliability_scanner = ReliabilityGapScanner(str(self.repo_root))
        reliability_results = reliability_scanner.run_full_scan(str(self.output_dir))
        results['reliability'] = reliability_results
        
        # Run Concurrency scanner
        print("\n[4/13] Concurrency & Threading Gap Scanner")
        print("-" * 80)
        concurrency_scanner = ConcurrencyGapScanner(str(self.repo_root))
        concurrency_results = concurrency_scanner.run_full_scan(str(self.output_dir))
        results['concurrency'] = concurrency_results
        
        # Run RAII scanner
        print("\n[5/13] RAII & Resource Management Gap Scanner")
        print("-" * 80)
        raii_scanner = RAIIGapScanner(str(self.repo_root))
        raii_results = raii_scanner.run_full_scan(str(self.output_dir))
        results['raii'] = raii_results
        
        # Run Container scanner
        print("\n[6/13] STL Container Misuse Gap Scanner")
        print("-" * 80)
        container_scanner = ContainerGapScanner(str(self.repo_root))
        container_results = container_scanner.run_full_scan(str(self.output_dir))
        results['container'] = container_results
        
        # Run Platform scanner
        print("\n[7/13] Platform Portability Gap Scanner")
        print("-" * 80)
        platform_scanner = PlatformGapScanner(str(self.repo_root))
        platform_results = platform_scanner.run_full_scan(str(self.output_dir))
        results['platform'] = platform_results
        
        # Run Performance scanner
        print("\n[8/13] Performance Anti-Patterns Gap Scanner")
        print("-" * 80)
        perf_scanner = PerformanceGapScanner(str(self.repo_root))
        perf_results = perf_scanner.run_full_scan(str(self.output_dir))
        results['performance'] = perf_results
        
        # Run Type Conversion scanner (Phase 5)
        print("\n[9/13] Type Conversion & Narrowing Gap Scanner (Phase 5)")
        print("-" * 80)
        type_conv_scanner = TypeConversionGapScanner()
        src_path = self.repo_root / 'src'
        type_conv_gaps = type_conv_scanner.run_full_scan(src_path)
        # Convert to module-based format for aggregation
        type_conv_results = self._convert_gaps_to_module_format(type_conv_gaps)
        results['type_conversion'] = type_conv_results
        
        # Run Input Validation scanner (Phase 5)
        print("\n[10/13] Input Validation & Bounds Gap Scanner (Phase 5)")
        print("-" * 80)
        input_val_scanner = InputValidationGapScanner()
        input_val_gaps = input_val_scanner.run_full_scan(src_path)
        # Convert to module-based format for aggregation
        input_val_results = self._convert_gaps_to_module_format(input_val_gaps)
        results['input_validation'] = input_val_results

        print("\n[11/13] Exception Safety & Move Semantics Gap Scanner (Phase 5)")
        print("-" * 80)
        exc_safety_scanner = ExceptionSafetyGapScanner()
        exc_safety_gaps = exc_safety_scanner.run_full_scan(src_path)
        exc_safety_results = self._convert_gaps_to_module_format(exc_safety_gaps)
        results['exception_safety'] = exc_safety_results

        print("\n[12/13] Uninitialized Variables & Undefined Behavior Gap Scanner (Phase 5)")
        print("-" * 80)
        uninit_scanner = UninitializedGapScanner()
        uninit_gaps = uninit_scanner.run_full_scan(src_path)
        uninit_results = self._convert_gaps_to_module_format(uninit_gaps)
        results['uninitialized'] = uninit_results

        print("\n[13/13] OOP Design & Virtual Functions Gap Scanner (Phase 5)")
        print("-" * 80)
        oop_scanner = OOPGapScanner()
        oop_gaps = oop_scanner.run_full_scan(src_path)
        oop_results = self._convert_gaps_to_module_format(oop_gaps)
        results['oop_design'] = oop_results
        
        # Phase 7 Scanners
        print("\n[14/27] Audit Trail & Logging Consistency Gap Scanner (Phase 7)")
        print("-" * 80)
        audit_scanner = AuditLoggingScan(str(self.repo_root))
        src_files = list(src_path.rglob('*.cpp')) + list(src_path.rglob('*.h'))
        audit_gaps = audit_scanner.scan_files(src_files)
        audit_results = self._convert_phase7_gaps_to_module_format(audit_gaps)
        results['audit_logging'] = audit_results
        
        print("\n[15/27] Deprecated API Usage Gap Scanner (Phase 7)")
        print("-" * 80)
        deprecated_scanner = DeprecatedAPIsScan(str(self.repo_root))
        deprecated_gaps = deprecated_scanner.scan_files(src_files)
        deprecated_results = self._convert_phase7_gaps_to_module_format(deprecated_gaps)
        results['deprecated_apis'] = deprecated_results
        
        # Phase 8 Scanners
        print("\n[16/27] Performance Patterns Gap Scanner (Phase 8)")
        print("-" * 80)
        perf_patterns_scanner = PerformanceAntiPatternsScan(str(self.repo_root))
        perf_patterns_gaps = perf_patterns_scanner.scan_files(src_files)
        perf_patterns_results = self._convert_phase7_gaps_to_module_format(perf_patterns_gaps)
        results['performance_patterns'] = perf_patterns_results
        
        print("\n[17/27] GPU Memory Safety Gap Scanner (Phase 8)")
        print("-" * 80)
        gpu_mem_scanner = GPUMemorySafetyScan(str(self.repo_root))
        gpu_mem_gaps = gpu_mem_scanner.scan_files(src_files)
        gpu_mem_results = self._convert_phase7_gaps_to_module_format(gpu_mem_gaps)
        results['gpu_memory_safety'] = gpu_mem_results
        
        # Phase 9 Scanners
        print("\n[18/27] Query Correctness & Semantic Validation Gap Scanner (Phase 9)")
        print("-" * 80)
        query_scanner = QueryCorrectnessScan(str(self.repo_root))
        query_gaps = query_scanner.scan_files(src_files)
        query_results = self._convert_phase7_gaps_to_module_format(query_gaps)
        results['query_correctness'] = query_results
        
        print("\n[19/27] Distributed Consistency & Consensus Gap Scanner (Phase 9)")
        print("-" * 80)
        dist_scanner = DistributedConsistencyScan(str(self.repo_root))
        dist_gaps = dist_scanner.scan_files(src_files)
        dist_results = self._convert_phase7_gaps_to_module_format(dist_gaps)
        results['distributed_consistency'] = dist_results
        
        print("\n[20/27] LLM/AI Safety & Model Integrity Gap Scanner (Phase 9)")
        print("-" * 80)
        llm_scanner = LLMAISafetyScan(str(self.repo_root))
        llm_gaps = llm_scanner.scan_files(src_files)
        llm_results = self._convert_phase7_gaps_to_module_format(llm_gaps)
        results['llm_ai_safety'] = llm_results
        
        # Phase 10 Scanners
        print("\n[21/27] Observability & Instrumentation Gap Scanner (Phase 10)")
        print("-" * 80)
        obs_scanner = ObservabilityScan(str(self.repo_root))
        obs_gaps = obs_scanner.scan_files(src_files)
        obs_results = self._convert_phase7_gaps_to_module_format(obs_gaps)
        results['observability'] = obs_results
        
        print("\n[22/27] Determinism & Reproducibility Gap Scanner (Phase 10)")
        print("-" * 80)
        det_scanner = DeterminismScan(str(self.repo_root))
        det_gaps = det_scanner.scan_files(src_files)
        det_results = self._convert_phase7_gaps_to_module_format(det_gaps)
        results['determinism'] = det_results
        
        # Aggregate results
        print("\n[...] Aggregating results...")
        aggregate = self._aggregate_results(results)
        
        # Save aggregates
        self._save_aggregate(aggregate)
        self._save_module_reports(aggregate)
        self._save_summary(aggregate)
        
        print("\n[OK] Phase 1-10 scan complete!")
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
        category_totals = {
            'security': 0, 
            'memory': 0, 
            'reliability': 0,
            'concurrency': 0,
            'raii': 0,
            'container': 0,
            'platform': 0,
            'performance': 0,
            'type_conversion': 0,
            'input_validation': 0,
            'exception_safety': 0,
            'uninitialized': 0,
            'oop_design': 0,
            'audit_logging': 0,
            'deprecated_apis': 0,
            'performance_patterns': 0,
            'gpu_memory_safety': 0,
            'query_correctness': 0,
            'distributed_consistency': 0,
            'llm_ai_safety': 0,
            'observability': 0,
            'determinism': 0
        }
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
            'phase': 'Phase 1-10 Extended (27 scanners: 8 Phase 1-4 + 5 Phase 5 + 2 Phase 7 + 2 Phase 8 + 3 Phase 9 + 2 Phase 10)',
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
        print(f"\n[SUMMARY] Phase 1-10 Complete Gap Analysis (27 Scanners)")
        print("=" * 80)
        print(f"  Total Gaps Found:        {total_gaps}")
        print(f"  CRITICAL Severity:       {critical}")
        print(f"  HIGH Severity:           {high}")
        print(f"  MEDIUM Severity:         {medium}")
        print(f"  ACTIONABLE (C+H):        {critical + high}")
        print()
        print(f"  Phase 1-4 Gaps:")
        print(f"    Security Gaps:           {category_totals.get('security', 0)}")
        print(f"    Memory Safety Gaps:      {category_totals.get('memory', 0)}")
        print(f"    Reliability Gaps:        {category_totals.get('reliability', 0)}")
        print(f"    Concurrency Gaps:        {category_totals.get('concurrency', 0)}")
        print(f"    RAII/Resource Gaps:      {category_totals.get('raii', 0)}")
        print(f"    Container Misuse Gaps:   {category_totals.get('container', 0)}")
        print(f"    Platform Portability:    {category_totals.get('platform', 0)}")
        print(f"    Performance Anti-Pat.:   {category_totals.get('performance', 0)}")
        print()
        print(f"  Phase 5 Gaps:")
        print(f"    Type Conversion Gaps:    {category_totals.get('type_conversion', 0)}")
        print(f"    Input Validation Gaps:   {category_totals.get('input_validation', 0)}")
        print(f"    Exception Safety Gaps:   {category_totals.get('exception_safety', 0)}")
        print(f"    Uninitialized Gaps:      {category_totals.get('uninitialized', 0)}")
        print(f"    OOP Design Gaps:         {category_totals.get('oop_design', 0)}")
        print()
        print(f"  Phase 7-10 Gaps:")
        print(f"    Audit Logging Gaps:      {category_totals.get('audit_logging', 0)}")
        print(f"    Deprecated APIs Gaps:    {category_totals.get('deprecated_apis', 0)}")
        print(f"    Performance Patterns:    {category_totals.get('performance_patterns', 0)}")
        print(f"    GPU Memory Safety:       {category_totals.get('gpu_memory_safety', 0)}")
        print(f"    Query Correctness:       {category_totals.get('query_correctness', 0)}")
        print(f"    Distributed Consistency: {category_totals.get('distributed_consistency', 0)}")
        print(f"    LLM/AI Safety:           {category_totals.get('llm_ai_safety', 0)}")
        print(f"    Observability:           {category_totals.get('observability', 0)}")
        print(f"    Determinism:             {category_totals.get('determinism', 0)}")
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
    
    def _convert_gaps_to_module_format(self, gaps: list) -> Dict[str, Dict]:
        """Convert TypeConversionGap list to module-based aggregation format"""
        modules = {}
        
        for gap in gaps:
            # Extract module name from file path (e.g., 'src/server/...' → 'server')
            file_parts = Path(gap.file_path).parts
            module_name = file_parts[1] if len(file_parts) > 1 else 'unknown'
            
            if module_name not in modules:
                modules[module_name] = {
                    'total': 0,
                    'severity_critical': 0,
                    'severity_high': 0,
                    'severity_medium': 0,
                    'gaps_by_file': {}
                }
            
            # Aggregate counts
            modules[module_name]['total'] += 1
            if gap.severity == 'CRITICAL':
                modules[module_name]['severity_critical'] += 1
            elif gap.severity == 'HIGH':
                modules[module_name]['severity_high'] += 1
            else:
                modules[module_name]['severity_medium'] += 1
            
            # Store gap by file
            if gap.file_path not in modules[module_name]['gaps_by_file']:
                modules[module_name]['gaps_by_file'][gap.file_path] = []
            modules[module_name]['gaps_by_file'][gap.file_path].append(gap.to_dict())
        
        return modules
    
    def _convert_phase7_gaps_to_module_format(self, gaps: list) -> Dict[str, Dict]:
        """Convert Phase 7-10 gap dicts to module-based aggregation format"""
        modules = {}
        
        for gap in gaps:
            # Extract module name from file path (e.g., 'src/server/...' → 'server')
            file_parts = Path(gap['file']).parts
            module_name = file_parts[1] if len(file_parts) > 1 else 'unknown'
            
            if module_name not in modules:
                modules[module_name] = {
                    'total': 0,
                    'severity_critical': 0,
                    'severity_high': 0,
                    'severity_medium': 0,
                    'gaps_by_file': {}
                }
            
            # Aggregate counts
            modules[module_name]['total'] += 1
            severity = gap.get('severity', 'MEDIUM')
            if severity == 'CRITICAL':
                modules[module_name]['severity_critical'] += 1
            elif severity == 'HIGH':
                modules[module_name]['severity_high'] += 1
            else:
                modules[module_name]['severity_medium'] += 1
            
            # Store gap by file
            if gap['file'] not in modules[module_name]['gaps_by_file']:
                modules[module_name]['gaps_by_file'][gap['file']] = []
            modules[module_name]['gaps_by_file'][gap['file']].append(gap)
        
        return modules


def main():
    """Main entry point"""
    
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    
    scanner = UnifiedGapScannerV3(repo_root, output_dir)
    aggregate = scanner.run_complete_scan()
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
