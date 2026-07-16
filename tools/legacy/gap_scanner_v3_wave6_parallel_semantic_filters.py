#!/usr/bin/env python3
"""
Wave 6 Parallel Semantic Filtering — Multi-threaded context-aware FP reduction

Uses ThreadPoolExecutor to process gaps in parallel with semantic context analysis
"""

import queue
import threading
from pathlib import Path
from typing import Dict, List, Any, Optional
from concurrent.futures import ThreadPoolExecutor, as_completed
import time
from dataclasses import dataclass

from gap_scanner_v3_wave6_semantic_filters import Wave6SemanticFilters, SemanticAnalyzer


@dataclass
class SemanticFilterTask:
    """Task to semantically filter gaps for a specific file"""
    module: str
    file_path: str
    gaps: List[Dict]


@dataclass
class SemanticFilterResult:
    """Result from semantic filtering"""
    module: str
    file_path: str
    original_count: int
    filtered_count: int
    gaps: List[Dict]
    eliminated_fps: int
    error: Optional[str] = None


class Wave6ParallelSemanticFilters:
    """Parallel Wave 6 semantic filtering with context analysis"""
    
    def __init__(self, num_workers: Optional[int] = None, verbose: bool = False):
        """
        Initialize parallel semantic filter engine.
        
        Args:
            num_workers: Number of worker threads (auto-detect if None)
            verbose: Print progress updates
        """
        import os
        
        cpu_count = os.cpu_count() or 4
        self.num_workers = num_workers or max(2, min(8, cpu_count - 1))
        self.verbose = verbose
        
        self.lock = threading.Lock()
        self.results_by_module = {}
        self.stats = {
            'tasks_processed': 0,
            'total_before': 0,
            'total_after': 0,
            'total_eliminated': 0,
            'errors': 0,
        }
        
        if self.verbose:
            print(f"[INFO] Wave 6 Parallel Semantic Filter initialized with {self.num_workers} workers")
    
    def _worker_semantic_filter(self, task: SemanticFilterTask) -> SemanticFilterResult:
        """
        Worker thread: Apply semantic filtering to gaps for a single file.
        
        Args:
            task: SemanticFilterTask
        
        Returns:
            SemanticFilterResult with filtered gaps
        """
        try:
            # Apply Wave 6 semantic filters
            file_gaps = task.gaps
            
            # Group by category for semantic analysis
            by_category = {}
            for gap in file_gaps:
                cat = gap.get('category', 'unknown')
                if cat not in by_category:
                    by_category[cat] = []
                by_category[cat].append(gap)
            
            # Apply semantic filtering
            gaps_dict = {'by_category': by_category}
            filtered_gaps_dict = Wave6SemanticFilters.apply_wave6_filters(
                gaps_dict, task.file_path
            )
            
            # Flatten back to gap list
            filtered_gaps = []
            for cat_gaps in filtered_gaps_dict.get('by_category', {}).values():
                filtered_gaps.extend(cat_gaps)
            
            eliminated = len(file_gaps) - len(filtered_gaps)
            
            result = SemanticFilterResult(
                module=task.module,
                file_path=task.file_path,
                original_count=len(file_gaps),
                filtered_count=len(filtered_gaps),
                gaps=filtered_gaps,
                eliminated_fps=eliminated,
                error=None
            )
            
        except Exception as e:
            result = SemanticFilterResult(
                module=task.module,
                file_path=task.file_path,
                original_count=len(task.gaps),
                filtered_count=len(task.gaps),
                gaps=task.gaps,
                eliminated_fps=0,
                error=str(e)
            )
        
        return result
    
    def _aggregate_result(self, result: SemanticFilterResult):
        """Thread-safe aggregation of semantic filter results"""
        with self.lock:
            self.stats['tasks_processed'] += 1
            self.stats['total_before'] += result.original_count
            self.stats['total_after'] += result.filtered_count
            self.stats['total_eliminated'] += result.eliminated_fps
            
            if result.error:
                self.stats['errors'] += 1
            
            # Initialize module aggregation if needed
            if result.module not in self.results_by_module:
                self.results_by_module[result.module] = {}
            
            if result.file_path not in self.results_by_module[result.module]:
                self.results_by_module[result.module][result.file_path] = []
            
            # Add filtered gaps
            self.results_by_module[result.module][result.file_path].extend(result.gaps)
            
            # Progress feedback
            if self.verbose and self.stats['tasks_processed'] % 50 == 0:
                reduction = self.stats['total_before'] - self.stats['total_after']
                reduction_pct = (reduction / self.stats['total_before'] * 100) if self.stats['total_before'] > 0 else 0
                print(f"[...] Wave 6 Progress: {self.stats['tasks_processed']} files, "
                      f"{reduction:,} FP reduced ({reduction_pct:.1f}%)")
    
    def apply_parallel_semantic_filters(self, aggregate: Dict[str, Any]) -> Dict[str, Any]:
        """
        Apply Wave 6 semantic filters in parallel across all files.
        
        Args:
            aggregate: Aggregate gap dict
        
        Returns:
            Filtered aggregate dict
        """
        
        # Build task queue (one task per file)
        tasks = []
        for module, module_data in aggregate.items():
            for file_path, gaps in module_data.get('by_file', {}).items():
                if gaps:  # Only process files with gaps
                    tasks.append(SemanticFilterTask(
                        module=module,
                        file_path=file_path,
                        gaps=gaps
                    ))
        
        if not tasks:
            return aggregate
        
        print(f"\n[...] Wave 6 Parallel Semantic Filtering: {len(tasks)} files across {self.num_workers} workers")
        start_time = time.time()
        
        # Execute tasks in parallel
        with ThreadPoolExecutor(max_workers=self.num_workers, thread_name_prefix='Wave6Worker') as executor:
            futures = {
                executor.submit(self._worker_semantic_filter, task): task
                for task in tasks
            }
            
            completed = 0
            for future in as_completed(futures):
                try:
                    result = future.result(timeout=60)
                    self._aggregate_result(result)
                    completed += 1
                    
                    # Progress feedback
                    if self.verbose and completed % 50 == 0:
                        elapsed = time.time() - start_time
                        rate = completed / elapsed if elapsed > 0 else 0
                        remaining = (len(tasks) - completed) / rate if rate > 0 else 0
                        print(f"[...] {completed}/{len(tasks)} files completed ({rate:.1f} files/sec, "
                              f"~{remaining:.0f}s remaining)")
                
                except Exception as e:
                    print(f"[WARN] Semantic filter task failed: {str(e)[:100]}")
        
        elapsed = time.time() - start_time
        print(f"[OK] Wave 6 Parallel Semantic Filtering Complete ({elapsed:.1f}s)")
        
        # Build filtered aggregate
        filtered_aggregate = {}
        for module in aggregate.keys():
            filtered_aggregate[module] = dict(aggregate[module])
            filtered_aggregate[module]['by_file'] = self.results_by_module.get(module, {})
            
            # Recalculate totals
            filtered_aggregate[module]['total'] = sum(
                len(gaps) for gaps in filtered_aggregate[module]['by_file'].values()
            )
            
            # Recalculate severity breakdown
            critical = high = medium = 0
            for gaps in filtered_aggregate[module]['by_file'].values():
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
        
        # Print summary
        total_reduction = self.stats['total_before'] - self.stats['total_after']
        total_reduction_pct = (total_reduction / self.stats['total_before'] * 100) if self.stats['total_before'] > 0 else 0
        
        print(f"\n[INFO] Wave 6 Parallel Semantic Filtering Summary:")
        print(f"  Files Processed:   {self.stats['tasks_processed']}")
        print(f"  Total Before:      {self.stats['total_before']:,}")
        print(f"  Total After:       {self.stats['total_after']:,}")
        print(f"  Reduced:           {total_reduction:,} ({total_reduction_pct:.1f}%)")
        print(f"  Errors:            {self.stats['errors']}")
        print(f"  Elapsed:           {elapsed:.1f}s")
        print(f"  Throughput:        {len(tasks) / elapsed:.1f} files/sec")
        
        return filtered_aggregate


def apply_wave6_parallel_semantic_filters(aggregate: Dict[str, Any], num_workers: Optional[int] = None, verbose: bool = True) -> Dict[str, Any]:
    """
    Convenience function: Apply Wave 6 parallel semantic filters.
    
    Args:
        aggregate: Gap aggregate dict
        num_workers: Number of worker threads (auto-detect if None)
        verbose: Print progress updates
    
    Returns:
        Filtered aggregate dict
    """
    filter_engine = Wave6ParallelSemanticFilters(num_workers=num_workers, verbose=verbose)
    return filter_engine.apply_parallel_semantic_filters(aggregate)
