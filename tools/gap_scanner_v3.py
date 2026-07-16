#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Unified Orchestrator (Phase 1-11)

Runs all Phase 1-11 scanners:
- Phase 1-4: Security, Memory, Reliability, Concurrency, RAII, Container, Platform, Performance
- Phase 5: Type Conversion, Input Validation, Exception Safety, Uninitialized, OOP Design
- Phase 7: Audit Trail & Logging, Deprecated APIs
- Phase 8: Performance Patterns, GPU Memory Safety
- Phase 9: Query Correctness, Distributed Consistency, LLM/AI Safety
- Phase 10: Observability, Determinism
- Phase 11: Legacy paths, duplicate implementations

Produces:
- Aggregate report (gap_scan_v3_aggregate.json)
- Module-level reports (gap_scan_v3_<module>.json)
- Summary statistics (gap_scan_v3_summary.json)
"""

import json
import sys
import os
import re
import logging
from pathlib import Path
from datetime import datetime
from typing import Dict, Any, List, Tuple

# Ensure tools/ is in sys.path for local module imports (critical for subprocess execution)
sys.path.insert(0, str(Path(__file__).parent))

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
from gap_scanner_v3_phase11_legacy_duplication import LegacyDuplicationScan

# Import Wave 5 Aggressive FP reduction filters (PARALLEL version preferred)
try:
    from gap_scanner_v3_wave5_parallel_filters import apply_wave5_parallel_filters
    WAVE5_FILTERING_ENABLED = True
    WAVE5_PARALLEL = True
except ImportError:
    try:
        from gap_scanner_v3_wave5_aggressive_fp_filters import Wave5AggressiveFilters
        WAVE5_FILTERING_ENABLED = True
        WAVE5_PARALLEL = False
    except ImportError:
        WAVE5_FILTERING_ENABLED = False
        WAVE5_PARALLEL = False

# Import Wave 6 Semantic FP reduction filters (PARALLEL version preferred)
try:
    from gap_scanner_v3_wave6_parallel_semantic_filters import apply_wave6_parallel_semantic_filters
    WAVE6_FILTERING_ENABLED = True
    WAVE6_PARALLEL = True
except ImportError as e:
    try:
        from gap_scanner_v3_wave6_semantic_filters import Wave6SemanticFilters
        WAVE6_FILTERING_ENABLED = True
        WAVE6_PARALLEL = False
    except ImportError as e2:
        WAVE6_FILTERING_ENABLED = False
        WAVE6_PARALLEL = False

# Debug: Print filter status on startup
import sys
print(f"[DEBUG] WAVE5_FILTERING_ENABLED: {WAVE5_FILTERING_ENABLED} (WAVE5_PARALLEL: {WAVE5_PARALLEL})", file=sys.stderr, flush=True)
print(f"[DEBUG] WAVE6_FILTERING_ENABLED: {WAVE6_FILTERING_ENABLED} (WAVE6_PARALLEL: {WAVE6_PARALLEL})", file=sys.stderr, flush=True)


class UnifiedGapScannerV3:
    """Orchestrate Phase 1-4 security, memory, reliability, concurrency, RAII, container, platform & performance scanning"""
    
    def __init__(self, repo_root: str = '.', output_dir: str = 'ai_working'):
        self.repo_root = Path(repo_root)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
    
    def run_complete_scan(self) -> Dict[str, Any]:
        """Execute Phase 1-11 security + memory + reliability + concurrency + RAII + container + platform + performance + type_conversion + input_validation + audit + deprecated + perf_patterns + gpu_memory + query + distributed + llm + observability + determinism + legacy_duplication scan"""
        
        print("\n" + "=" * 80)
        print("ThemisDB Gap Scanner v3 — Phase 1-11 Complete Suite (28 Scanners)")
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
        print("\n[14/28] Audit Trail & Logging Consistency Gap Scanner (Phase 7)")
        print("-" * 80)
        audit_scanner = AuditLoggingScan(str(self.repo_root))
        src_files = list(src_path.rglob('*.cpp')) + list(src_path.rglob('*.h'))
        audit_gaps = audit_scanner.scan_files(src_files)
        audit_results = self._convert_phase7_gaps_to_module_format(audit_gaps)
        results['audit_logging'] = audit_results
        
        print("\n[15/28] Deprecated API Usage Gap Scanner (Phase 7)")
        print("-" * 80)
        deprecated_scanner = DeprecatedAPIsScan(str(self.repo_root))
        deprecated_gaps = deprecated_scanner.scan_files(src_files)
        deprecated_results = self._convert_phase7_gaps_to_module_format(deprecated_gaps)
        results['deprecated_apis'] = deprecated_results
        
        # Phase 8 Scanners
        print("\n[16/28] Performance Patterns Gap Scanner (Phase 8)")
        print("-" * 80)
        perf_patterns_scanner = PerformanceAntiPatternsScan(str(self.repo_root))
        perf_patterns_gaps = perf_patterns_scanner.scan_files(src_files)
        perf_patterns_results = self._convert_phase7_gaps_to_module_format(perf_patterns_gaps)
        results['performance_patterns'] = perf_patterns_results
        
        print("\n[17/28] GPU Memory Safety Gap Scanner (Phase 8)")
        print("-" * 80)
        gpu_mem_scanner = GPUMemorySafetyScan(str(self.repo_root))
        gpu_mem_gaps = gpu_mem_scanner.scan_files(src_files)
        gpu_mem_results = self._convert_phase7_gaps_to_module_format(gpu_mem_gaps)
        results['gpu_memory_safety'] = gpu_mem_results
        
        # Phase 9 Scanners
        print("\n[18/28] Query Correctness & Semantic Validation Gap Scanner (Phase 9)")
        print("-" * 80)
        query_scanner = QueryCorrectnessScan(str(self.repo_root))
        query_gaps = query_scanner.scan_files(src_files)
        query_results = self._convert_phase7_gaps_to_module_format(query_gaps)
        results['query_correctness'] = query_results
        
        print("\n[19/28] Distributed Consistency & Consensus Gap Scanner (Phase 9)")
        print("-" * 80)
        dist_scanner = DistributedConsistencyScan(str(self.repo_root))
        dist_gaps = dist_scanner.scan_files(src_files)
        dist_results = self._convert_phase7_gaps_to_module_format(dist_gaps)
        results['distributed_consistency'] = dist_results
        
        print("\n[20/28] LLM/AI Safety & Model Integrity Gap Scanner (Phase 9)")
        print("-" * 80)
        llm_scanner = LLMAISafetyScan(str(self.repo_root))
        llm_gaps = llm_scanner.scan_files(src_files)
        llm_results = self._convert_phase7_gaps_to_module_format(llm_gaps)
        results['llm_ai_safety'] = llm_results
        
        # Phase 10 Scanners
        print("\n[21/28] Observability & Instrumentation Gap Scanner (Phase 10)")
        print("-" * 80)
        obs_scanner = ObservabilityScan(str(self.repo_root))
        obs_gaps = obs_scanner.scan_files(src_files)
        obs_results = self._convert_phase7_gaps_to_module_format(obs_gaps)
        results['observability'] = obs_results
        
        print("\n[22/28] Determinism & Reproducibility Gap Scanner (Phase 10)")
        print("-" * 80)
        det_scanner = DeterminismScan(str(self.repo_root))
        det_gaps = det_scanner.scan_files(src_files)
        det_results = self._convert_phase7_gaps_to_module_format(det_gaps)
        results['determinism'] = det_results

        print("\n[23/28] Legacy Paths & Duplicate Implementation Scanner (Phase 11)")
        print("-" * 80)
        legacy_scanner = LegacyDuplicationScan(str(self.repo_root))
        legacy_gaps = legacy_scanner.scan_files(src_files)
        legacy_results = self._convert_phase7_gaps_to_module_format(legacy_gaps)
        results['legacy_duplication'] = legacy_results
        
        # Aggregate results
        print("\n[...] Aggregating results...")
        aggregate = self._aggregate_results(results)
        
        # Apply Wave 5 Aggressive FP reduction filters (if available)
        if WAVE5_FILTERING_ENABLED:
            print("\n[...] Applying Wave 5 Aggressive FP reduction filters (all 21 categories)...")
            aggregate = self._apply_wave5_filters(aggregate)
            print("[OK] Wave 5 filters applied! (Expected: -55-60% reduction per category)")
        
        # Apply Wave 6 Semantic FP reduction filters (if available)
        if WAVE6_FILTERING_ENABLED:
            print("\n[...] Applying Wave 6 Semantic FP reduction filters (context-aware)...")
            aggregate = self._apply_wave6_filters(aggregate)
            print("[OK] Wave 6 semantic filters applied! (Expected: -30-40% additional reduction)")
        
        # Save aggregates
        self._save_aggregate(aggregate)
        self._save_module_reports(aggregate)
        self._save_summary(aggregate)
        
        print("\n[OK] Phase 1-11 scan complete!")
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
    
    def _apply_wave5_filters(self, aggregate: Dict[str, Any]) -> Dict[str, Any]:
        """Apply Wave 5 FP reduction filters using parallel threading (if available)"""
        if not WAVE5_FILTERING_ENABLED:
            return aggregate
        
        # Use parallel version if available
        if WAVE5_PARALLEL:
            print("\n[...] Using Wave 5 PARALLEL Filtering (multi-threaded)")
            return apply_wave5_parallel_filters(aggregate, num_workers=None, verbose=True)
        
        # Fallback to sequential filtering
        print("\n[...] Using Wave 5 SEQUENTIAL Filtering (parallel unavailable)")
        filtered_aggregate = {}
        total_before = 0
        total_after = 0
        
        for module, module_data in aggregate.items():
            filtered_aggregate[module] = dict(module_data)
            filtered_by_file = {}
            
            for file_path, gaps in module_data.get('by_file', {}).items():
                try:
                    file_full_path = self.repo_root / file_path
                    
                    # Group gaps by category for Wave 5 filters
                    by_category = {}
                    for gap in gaps:
                        cat = gap.get('category', 'unknown')
                        if cat not in by_category:
                            by_category[cat] = []
                        by_category[cat].append(gap)
                    
                    # Apply Wave 5 aggressive filters
                    gaps_dict = {'by_category': by_category}
                    filtered_gaps_dict = Wave5AggressiveFilters.apply_wave5_filters(
                        gaps_dict, str(file_full_path)
                    )
                    
                    # Flatten back to gap list
                    filtered_gaps = []
                    for cat_gaps in filtered_gaps_dict.get('by_category', {}).values():
                        filtered_gaps.extend(cat_gaps)
                    
                    filtered_by_file[file_path] = filtered_gaps
                    total_after += len(filtered_gaps)
                    total_before += len(gaps)
                    
                except Exception as e:
                    # Fallback: keep original gaps if filter fails
                    filtered_by_file[file_path] = gaps
                    total_after += len(gaps)
                    total_before += len(gaps)
            
            # Recalculate totals
            filtered_aggregate[module]['by_file'] = filtered_by_file
            filtered_aggregate[module]['total'] = sum(
                len(gaps) for gaps in filtered_by_file.values()
            )
            
            # Recalculate severity breakdown
            critical = high = medium = 0
            for gaps in filtered_by_file.values():
                for gap in gaps:
                    severity = str(gap.get('severity', 'MEDIUM')).upper()
                    if severity == 'CRITICAL':
                        critical += 1
                    elif severity == 'HIGH':
                        high += 1
                    elif severity == 'MEDIUM':
                        medium += 1
            
            filtered_aggregate[module]['severity_critical'] = critical
            filtered_aggregate[module]['severity_high'] = high
            filtered_aggregate[module]['severity_medium'] = medium
        
        # Print Wave 5 reduction summary
        total_reduction = total_before - total_after
        total_reduction_pct = (total_reduction / total_before * 100) if total_before > 0 else 0
        
        print(f"\n[INFO] Wave 5 FP Reduction Summary:")
        print(f"  Total Before: {total_before:,}")
        print(f"  Total After:  {total_after:,}")
        print(f"  Reduced:      {total_reduction:,} ({total_reduction_pct:.1f}%)")
        
        return filtered_aggregate
    
    def _apply_wave6_filters(self, aggregate: Dict[str, Any]) -> Dict[str, Any]:
        """Apply Wave 6 semantic FP reduction filters using code context analysis"""
        if not WAVE6_FILTERING_ENABLED:
            return aggregate
        
        # Use parallel version if available
        if WAVE6_PARALLEL:
            print("\n[...] Using Wave 6 PARALLEL Semantic Filtering (multi-threaded)")
            return apply_wave6_parallel_semantic_filters(aggregate, num_workers=None, verbose=True)
        
        # Fallback to sequential filtering
        print("\n[...] Using Wave 6 SEQUENTIAL Semantic Filtering (parallel unavailable)")
        filtered_aggregate = {}
        total_before = 0
        total_after = 0
        
        for module, module_data in aggregate.items():
            filtered_aggregate[module] = dict(module_data)
            filtered_by_file = {}
            
            for file_path, gaps in module_data.get('by_file', {}).items():
                try:
                    file_full_path = self.repo_root / file_path
                    
                    # Group gaps by category for Wave 6 semantic filters
                    by_category = {}
                    for gap in gaps:
                        cat = gap.get('category', 'unknown')
                        if cat not in by_category:
                            by_category[cat] = []
                        by_category[cat].append(gap)
                    
                    # Apply Wave 6 semantic filters
                    gaps_dict = {'by_category': by_category}
                    filtered_gaps_dict = Wave6SemanticFilters.apply_wave6_filters(
                        gaps_dict, str(file_full_path)
                    )
                    
                    # Flatten back to gap list
                    filtered_gaps = []
                    for cat_gaps in filtered_gaps_dict.get('by_category', {}).values():
                        filtered_gaps.extend(cat_gaps)
                    
                    filtered_by_file[file_path] = filtered_gaps
                    total_after += len(filtered_gaps)
                    total_before += len(gaps)
                    
                except Exception as e:
                    # Fallback: keep original gaps if filter fails
                    filtered_by_file[file_path] = gaps
                    total_after += len(gaps)
                    total_before += len(gaps)
            
            # Recalculate totals
            filtered_aggregate[module]['by_file'] = filtered_by_file
            filtered_aggregate[module]['total'] = sum(
                len(gaps) for gaps in filtered_by_file.values()
            )
            
            # Recalculate severity breakdown
            critical = high = medium = 0
            for gaps in filtered_by_file.values():
                for gap in gaps:
                    severity = str(gap.get('severity', 'MEDIUM')).upper()
                    if severity == 'CRITICAL':
                        critical += 1
                    elif severity == 'HIGH':
                        high += 1
                    elif severity == 'MEDIUM':
                        medium += 1
            
            filtered_aggregate[module]['severity_critical'] = critical
            filtered_aggregate[module]['severity_high'] = high
            filtered_aggregate[module]['severity_medium'] = medium
        
        # Print Wave 6 reduction summary
        total_reduction = total_before - total_after
        total_reduction_pct = (total_reduction / total_before * 100) if total_before > 0 else 0
        
        print(f"\n[INFO] Wave 6 Semantic Filtering Summary:")
        print(f"  Total Before: {total_before:,}")
        print(f"  Total After:  {total_after:,}")
        print(f"  Reduced:      {total_reduction:,} ({total_reduction_pct:.1f}%)")
        
        return filtered_aggregate
    
    def _save_aggregate(self, aggregate: Dict[str, Any]):
        """Save main aggregate file"""
        output_file = self.output_dir / 'gap_scan_v3_aggregate.json'
        with open(output_file, 'w') as f:
            json.dump(aggregate, f, indent=2)
        print(f"[OK] Saved: {output_file.name}")
    
    def _save_module_reports(self, aggregate: Dict[str, Any]):
        """Save per-module reports"""
        for module, data in aggregate.items():
            sorted_data = self._sort_report_data_by_confidence(data)
            output_file = self.output_dir / f'gap_scan_v3_{module}.json'
            with open(output_file, 'w') as f:
                json.dump({module: sorted_data}, f, indent=2)
        print(f"[OK] Saved: {len(aggregate)} module reports")

    def _sort_report_data_by_confidence(self, module_data: Dict[str, Any]) -> Dict[str, Any]:
        """Sort gap entries inside a module by confidence score descending."""
        sorted_data = dict(module_data)
        by_file = {}

        for file_path, gaps in module_data.get('by_file', {}).items():
            by_file[file_path] = sorted(
                gaps,
                key=lambda gap: (
                    float(gap.get('confidence_score', 0.0)),
                    str(gap.get('severity', '')),
                    int(gap.get('line_number', 0) or gap.get('line', 0) or 0),
                ),
                reverse=True,
            )

        sorted_data['by_file'] = by_file
        return sorted_data

    def _save_confidence_review(self, aggregate: Dict[str, Any]):
        """Write a high-confidence review queue across all modules."""
        review_items = []
        threshold = 0.85
        max_items = 2000
        output_file = self.output_dir / 'gap_scan_v3_confidence_review.json'

        previous_items = []
        if output_file.exists():
            try:
                with open(output_file, 'r', encoding='utf-8') as f:
                    previous_payload = json.load(f)
                previous_items = list(previous_payload.get('items', []))
            except Exception:
                previous_items = []

        for module_name, module_data in aggregate.items():
            for file_path, gaps in module_data.get('by_file', {}).items():
                for gap in gaps:
                    confidence = float(gap.get('confidence_score', 0.0) or 0.0)
                    if confidence < threshold:
                        continue
                    review_items.append({
                        'module': module_name,
                        'file': file_path,
                        'line': gap.get('line_number') or gap.get('line'),
                        'severity': gap.get('severity'),
                        'category': gap.get('category') or module_name,
                        'pattern': gap.get('gap_type') or gap.get('pattern'),
                        'confidence_score': round(confidence, 3),
                        'confidence_band': gap.get('confidence_band'),
                        'confidence_rationale': gap.get('confidence_rationale'),
                        'summary': gap.get('issue') or gap.get('description') or gap.get('reason'),
                    })

        review_items.sort(
            key=lambda item: (
                float(item.get('confidence_score', 0.0)),
                str(item.get('severity', '')),
                str(item.get('module', '')),
                str(item.get('file', '')),
                int(item.get('line') or 0),
            ),
            reverse=True,
        )
        review_items = review_items[:max_items]

        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump({
                'threshold': threshold,
                'max_items': max_items,
                'count': len(review_items),
                'items': review_items,
            }, f, indent=2)

        print(f"[OK] Saved: {output_file.name} ({len(review_items)} items >= {threshold})")
        self._save_preflight_actionable_queue(review_items, previous_items, threshold)

    def _build_review_item_key(self, item: Dict[str, Any]) -> str:
        """Build a stable identity key for confidence-review items."""
        return "|".join([
            str(item.get('module', '')),
            str(item.get('file', '')),
            str(item.get('line', '')),
            str(item.get('severity', '')),
            str(item.get('category', '')),
            str(item.get('pattern', '')),
        ])

    def _save_preflight_actionable_queue(
        self,
        review_items: list,
        previous_items: list,
        threshold: float,
    ):
        """Persist machine-readable actionable top-N queue and markdown triage summary."""
        actionable = [
            item for item in review_items
            if str(item.get('severity', '')).upper() in {'CRITICAL', 'HIGH'}
        ]
        critical_high_conf = [
            item for item in review_items
            if str(item.get('severity', '')).upper() == 'CRITICAL'
            and float(item.get('confidence_score', 0.0) or 0.0) >= threshold
        ]

        previous_critical_keys = {
            self._build_review_item_key(item)
            for item in previous_items
            if str(item.get('severity', '')).upper() == 'CRITICAL'
            and float(item.get('confidence_score', 0.0) or 0.0) >= threshold
        }
        net_new_critical = [
            item for item in critical_high_conf
            if self._build_review_item_key(item) not in previous_critical_keys
        ]

        by_category: Dict[str, int] = {}
        by_file: Dict[str, int] = {}
        for item in actionable:
            category = str(item.get('category') or 'unknown')
            file_path = str(item.get('file') or 'unknown')
            by_category[category] = by_category.get(category, 0) + 1
            by_file[file_path] = by_file.get(file_path, 0) + 1

        top_n = 200
        top_files_n = 25
        top_categories_n = 15
        queue_payload = {
            'generated_at': datetime.now().isoformat(),
            'confidence_threshold': threshold,
            'top_n': top_n,
            'actionable_count': len(actionable),
            'critical_high_confidence_count': len(critical_high_conf),
            'net_new_critical_high_confidence_count': len(net_new_critical),
            'top_actionable_items': actionable[:top_n],
            'net_new_critical_high_confidence_items': net_new_critical[:top_n],
            'top_categories': [
                {'category': category, 'count': count}
                for category, count in sorted(
                    by_category.items(),
                    key=lambda kv: kv[1],
                    reverse=True,
                )[:top_categories_n]
            ],
            'top_files': [
                {'file': file_path, 'count': count}
                for file_path, count in sorted(
                    by_file.items(),
                    key=lambda kv: kv[1],
                    reverse=True,
                )[:top_files_n]
            ],
        }

        queue_file = self.output_dir / 'gap_scan_v3_preflight_actionable_queue.json'
        with open(queue_file, 'w', encoding='utf-8') as f:
            json.dump(queue_payload, f, indent=2)
        print(f"[OK] Saved: {queue_file.name}")

        summary_file = self.output_dir / 'gap_scan_v3_preflight_summary.md'
        with open(summary_file, 'w', encoding='utf-8') as f:
            f.write("# Gap Scanner v3 Preflight Summary\n\n")
            f.write(f"Generated: {queue_payload['generated_at']}\n\n")
            f.write("## Headline\n\n")
            f.write(f"- Actionable (CRITICAL+HIGH): {len(actionable)}\n")
            f.write(f"- High-confidence CRITICAL (>= {threshold}): {len(critical_high_conf)}\n")
            f.write(f"- Net-new high-confidence CRITICAL vs previous snapshot: {len(net_new_critical)}\n\n")

            f.write("## Top Categories\n\n")
            for row in queue_payload['top_categories']:
                f.write(f"- {row['category']}: {row['count']}\n")
            f.write("\n")

            f.write("## Top Files\n\n")
            for row in queue_payload['top_files']:
                f.write(f"- {row['file']}: {row['count']}\n")
            f.write("\n")

            f.write("## Top Actionable Items (Top 25)\n\n")
            for item in actionable[:25]:
                f.write(
                    f"- [{item.get('severity')}] {item.get('category')} | {item.get('file')}:{item.get('line')} "
                    f"| conf={item.get('confidence_score')} | {item.get('summary')}\n"
                )

        print(f"[OK] Saved: {summary_file.name}")
    
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
            'determinism': 0,
            'legacy_duplication': 0
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
            'phase': 'Phase 1-11 Extended (28 scanners: 8 Phase 1-4 + 5 Phase 5 + 2 Phase 7 + 2 Phase 8 + 3 Phase 9 + 2 Phase 10 + 1 Phase 11)',
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
            'confidence_overview': self._build_confidence_overview(aggregate),
            'confidence_by_category': self._build_confidence_by_category(aggregate),
            'implementation_effort': self._estimate_effort(critical, high)
        }
        
        output_file = self.output_dir / 'gap_scan_v3_summary.json'
        with open(output_file, 'w') as f:
            json.dump(summary, f, indent=2)
        
        # Print to console
        print(f"\n[SUMMARY] Phase 1-11 Complete Gap Analysis (28 Scanners)")
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
        print(f"    Legacy/Duplication:      {category_totals.get('legacy_duplication', 0)}")
        print()
        print(f"  Modules Scanned:         {len(aggregate)}")
        print()
        print("  Confidence Overview:")
        conf = summary['confidence_overview']
        print(f"    High confidence (>=0.85): {conf['very_high']}")
        print(f"    High confidence (>=0.70): {conf['high']}")
        print(f"    Medium confidence:        {conf['medium']}")
        print(f"    Low confidence:           {conf['low']}")
        print()
        print("  Confidence by Category:")
        for category, stats in sorted(summary['confidence_by_category'].items(), key=lambda item: item[1]['avg_confidence'], reverse=True):
            print(f"    {category:24} avg={stats['avg_confidence']:.3f} high={stats['high_confidence']} total={stats['total']}")
        print()
        print(f"  Top 5 Modules by Gap Count:")
        for i, (m, c) in enumerate(module_ranking[:5], 1):
            print(f"    {i}. {m:30} {c:4} gaps")
        print()
        print(f"  Estimated Effort:        {summary['implementation_effort']}")
        print("=" * 80)
        
        print(f"[OK] Saved: {output_file.name}")
        self._save_confidence_review(aggregate)
        self._save_confidence_by_category(summary['confidence_by_category'])

    def _build_confidence_overview(self, aggregate: Dict[str, Any]) -> Dict[str, int]:
        """Count gaps by confidence band across the full aggregate."""
        overview = {
            'very_high': 0,
            'high': 0,
            'medium': 0,
            'low': 0,
        }

        for module_data in aggregate.values():
            for gaps in module_data.get('by_file', {}).values():
                for gap in gaps:
                    band = str(gap.get('confidence_band', 'medium')).lower()
                    if band in overview:
                        overview[band] += 1
                    else:
                        overview['medium'] += 1

        return overview

    def _build_confidence_by_category(self, aggregate: Dict[str, Any]) -> Dict[str, Dict[str, Any]]:
        """Compute confidence statistics per category across the full aggregate."""
        stats: Dict[str, Dict[str, Any]] = {}

        for module_data in aggregate.values():
            for gaps in module_data.get('by_file', {}).values():
                for gap in gaps:
                    category = str(gap.get('category') or 'unknown')
                    confidence = float(gap.get('confidence_score', 0.0) or 0.0)
                    entry = stats.setdefault(category, {
                        'total': 0,
                        'high_confidence': 0,
                        'very_high_confidence': 0,
                        'confidence_sum': 0.0,
                        'avg_confidence': 0.0,
                    })
                    entry['total'] += 1
                    entry['confidence_sum'] += confidence
                    if confidence >= 0.70:
                        entry['high_confidence'] += 1
                    if confidence >= 0.85:
                        entry['very_high_confidence'] += 1

        for entry in stats.values():
            if entry['total']:
                entry['avg_confidence'] = round(entry['confidence_sum'] / entry['total'], 3)
            else:
                entry['avg_confidence'] = 0.0
            del entry['confidence_sum']

        return stats

    def _save_confidence_by_category(self, stats: Dict[str, Dict[str, Any]]):
        """Persist per-category confidence statistics for triage and tuning."""
        output_file = self.output_dir / 'gap_scan_v3_confidence_by_category.json'
        with open(output_file, 'w') as f:
            json.dump({
                'categories': stats,
                'sorted_by_avg_confidence': sorted(
                    [
                        {'category': category, **values}
                        for category, values in stats.items()
                    ],
                    key=lambda item: item['avg_confidence'],
                    reverse=True,
                ),
            }, f, indent=2)

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

    def _compute_gap_confidence(self, gap: Dict[str, Any]) -> Dict[str, Any]:
        """Compute a heuristic confidence score for a single gap entry."""
        severity = str(gap.get('severity', 'MEDIUM')).upper()
        pattern = str(gap.get('pattern', '') or '')
        description = str(gap.get('description', '') or '')
        context = str(gap.get('context', '') or '')
        line = gap.get('line')

        severity_base = {
            'CRITICAL': 0.90,
            'HIGH': 0.78,
            'MEDIUM': 0.62,
            'LOW': 0.48,
        }
        score = severity_base.get(severity, 0.55)
        rationale = [f'severity={severity}']

        if pattern:
            score += 0.06
            rationale.append('pattern_present')

        if isinstance(line, int) and line > 0:
            score += 0.03
            rationale.append('line_present')

        ctx_len = len(context.strip())
        if 8 <= ctx_len <= 220:
            score += 0.03
            rationale.append('context_specific')
        elif ctx_len == 0:
            score -= 0.05
            rationale.append('context_missing')

        if any(term in description.lower() for term in ['potential', 'possible', 'might', 'review']):
            score -= 0.10
            rationale.append('hedged_description')

        # Known noisier classes get a small confidence haircut.
        noisy_patterns = {
            'missing_health_check',
            'unordered_container_iter',
            'duplicate_qualified_signature',
        }
        if pattern in noisy_patterns:
            score -= 0.08
            rationale.append('historically_noisy_pattern')

        if score < 0.05:
            score = 0.05
        if score > 0.99:
            score = 0.99

        if score >= 0.85:
            band = 'very_high'
        elif score >= 0.70:
            band = 'high'
        elif score >= 0.50:
            band = 'medium'
        else:
            band = 'low'

        enriched = dict(gap)
        enriched['confidence_score'] = round(score, 3)
        enriched['confidence_band'] = band
        enriched['confidence_rationale'] = ','.join(rationale)
        return enriched
    
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
            modules[module_name]['gaps_by_file'][gap.file_path].append(
                self._compute_gap_confidence(gap.to_dict())
            )
        
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
            modules[module_name]['gaps_by_file'][gap['file']].append(
                self._compute_gap_confidence(gap)
            )
        
        return modules

    def verify_findings(self, aggregate: Dict[str, Any]) -> Dict[str, Any]:
        """
        IMPROVEMENT: Verify findings with multi-factor analysis
        
        Eliminates false-positives by:
        1. Checking file existence
        2. Classifying gap type (Real | Guarded Stub | Test Mock | False-Positive)
        3. Re-assessing severity based on source context
        
        Returns: Verified aggregate with classification + corrected severity
        """
        
        logging.info("[VERIFIER] Starting multi-factor gap verification...")
        verified_aggregate = {}
        
        for module_name, module_data in aggregate.items():
            verified_module = dict(module_data)
            verified_by_file = {}
            stats = {'total': 0, 'file_not_found': 0, 'downgraded': 0, 'kept': 0}
            
            for file_path, gaps in module_data.get('by_file', {}).items():
                verified_gaps = []
                file_full_path = self.repo_root / file_path
                
                for gap in gaps:
                    # STEP 1: File Existence Check
                    if not file_full_path.exists():
                        gap['verification'] = {
                            'status': 'FALSE_POSITIVE',
                            'classification': 'FILE_NOT_FOUND',
                            'verified_severity': 'IGNORE',
                            'rationale': f"File does not exist: {file_path}"
                        }
                        stats['file_not_found'] += 1
                        logging.warning(f"  FALSE_POSITIVE: {file_path} (file not found)")
                        continue  # Skip this finding
                    
                    # STEP 2: Multi-Factor Classification
                    classification, severity_action = self._classify_gap(
                        file_full_path, gap
                    )
                    
                    # STEP 3: Severity Re-Assessment
                    original_severity = gap.get('severity', 'MEDIUM')
                    verified_severity = original_severity
                    downgraded = False
                    
                    if severity_action.startswith('DOWNGRADE_'):
                        new_severity = severity_action.split('_')[1]
                        verified_severity = new_severity
                        downgraded = True
                        stats['downgraded'] += 1
                        logging.info(
                            f"  DOWNGRADE: {file_path}:{gap.get('line', '?')} "
                            f"{original_severity} → {new_severity} ({classification})"
                        )
                    else:
                        stats['kept'] += 1
                    
                    # Store verification metadata
                    gap['verification'] = {
                        'status': 'VERIFIED',
                        'classification': classification,
                        'original_severity': original_severity,
                        'verified_severity': verified_severity,
                        'downgraded': downgraded,
                        'rationale': f"Classified as {classification}"
                    }
                    
                    # Update severity in gap
                    gap['severity'] = verified_severity
                    verified_gaps.append(gap)
                    stats['total'] += 1
                
                if verified_gaps:
                    verified_by_file[file_path] = verified_gaps
            
            # Recalculate totals after verification
            verified_module['by_file'] = verified_by_file
            verified_module['total'] = sum(len(gaps) for gaps in verified_by_file.values())
            verified_module['verification_stats'] = stats
            
            # Recalculate severity breakdown
            critical = high = medium = info = 0
            for gaps in verified_by_file.values():
                for gap in gaps:
                    sev = str(gap.get('severity', 'MEDIUM')).upper()
                    if sev == 'CRITICAL': critical += 1
                    elif sev == 'HIGH': high += 1
                    elif sev == 'MEDIUM': medium += 1
                    elif sev == 'INFO': info += 1
            
            verified_module['severity_breakdown'] = {
                'CRITICAL': critical,
                'HIGH': high,
                'MEDIUM': medium,
                'INFO': info
            }
            
            verified_aggregate[module_name] = verified_module
        
        logging.info("[VERIFIER] Verification complete")
        return verified_aggregate

    def _classify_gap(self, file_path: Path, gap: Dict[str, Any]) -> Tuple[str, str]:
        """
        IMPROVEMENT: Multi-factor classification of gap
        
        Returns: (classification, severity_action)
          - classification: 'Real Gap' | 'Guarded Stub' | 'Test Mock' | 'Placeholder'
          - severity_action: 'KEEP_SEVERITY' | 'DOWNGRADE_HIGH' | 'DOWNGRADE_INFO' | 'DOWNGRADE_MEDIUM'
        """
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            logging.error(f"  Error reading {file_path}: {e}")
            return 'UNKNOWN', 'KEEP_SEVERITY'
        
        line_num = int(gap.get('line', gap.get('line_number', 1)) or 1)
        if line_num < 1 or line_num > len(lines):
            return 'OUT_OF_RANGE', 'DOWNGRADE_INFO'
        
        # Extract source context (±5 lines)
        start = max(0, line_num - 6)
        end = min(len(lines), line_num + 4)
        context_lines = lines[start:end]
        source_line = lines[line_num - 1] if line_num <= len(lines) else ''
        
        # Factor 1: Test code marker?
        if str(file_path).startswith(str(self.repo_root / 'tests')):
            if any(marker in source_line for marker in ['MOCK', 'TEST', '// Mock', '// TEST']):
                return 'TEST_MOCK', 'DOWNGRADE_INFO'
        
        # Factor 2: TODO/STUB/TEMPORARY marker?
        if any(marker in source_line for marker in ['TODO', 'FIXME', 'STUB', 'TEMPORARY', 'WIP']):
            return 'PLACEHOLDER', 'DOWNGRADE_MEDIUM'
        
        # Factor 3: Guarded pattern?
        full_context = ''.join(context_lines)
        if re.search(r'(if|while|for)\s*\(', source_line):
            if 'return' in source_line or 'return' in full_context:
                return 'GUARDED_STUB', 'DOWNGRADE_HIGH'
        
        # Factor 4: Defensive error handling?
        if any(pattern in source_line for pattern in ['return {};', 'return "";', 'return null', 'return nullptr', 'return false']):
            if any(check in full_context for check in ['if (', 'if!', 'assert', 'CHECK']):
                return 'GUARDED_STUB', 'DOWNGRADE_HIGH'
        
        # Default: Real gap
        return 'REAL_GAP', 'KEEP_SEVERITY'



def main():
    """Main entry point"""
    
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    
    # Configure logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s — %(levelname)s — %(message)s'
    )
    
    scanner = UnifiedGapScannerV3(repo_root, output_dir)
    aggregate = scanner.run_complete_scan()
    
    # IMPROVEMENT: Verify findings before output (multi-factor analysis)
    print("\n" + "=" * 80)
    print("Gap Verification Phase (L0.5) — Multi-Factor Analysis")
    print("=" * 80)
    
    verified_aggregate = scanner.verify_findings(aggregate)
    
    # Write verified results
    verified_output = Path(output_dir) / 'gap_scanner_results.json'
    with open(verified_output, 'w', encoding='utf-8') as f:
        json.dump(verified_aggregate, f, indent=2)
    print(f"\n[OK] Verified findings saved to: {verified_output.name}")
    
    # Summary statistics
    total_verified = sum(data['total'] for data in verified_aggregate.values())
    total_downgraded = sum(data['verification_stats'].get('downgraded', 0) for data in verified_aggregate.values())
    total_fp_removed = sum(data['verification_stats'].get('file_not_found', 0) for data in verified_aggregate.values())
    
    print(f"\n[SUMMARY]")
    print(f"  Total findings after verification: {total_verified}")
    print(f"  Findings downgraded (lower severity): {total_downgraded}")
    print(f"  False-positives removed: {total_fp_removed}")
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
