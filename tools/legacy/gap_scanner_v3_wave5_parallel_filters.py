#!/usr/bin/env python3
"""
Wave 5 Parallel FP Reduction — Multi-threaded filtering for all 21 categories

Strategy: Use thread pool to process 21 categories in parallel
- Worker threads: 4-8 (configurable, auto-detect CPU cores)
- Queue-based task distribution
- Thread-safe aggregation of filtered results
- Progress tracking with locks

Expected speedup: 4-8x faster than sequential
"""

import queue
import threading
from pathlib import Path
from typing import Dict, List, Any, Optional
from concurrent.futures import ThreadPoolExecutor, as_completed
import time
from dataclasses import dataclass

from gap_scanner_v3_wave5_aggressive_fp_filters import Wave5AggressiveFilters


@dataclass
class FilterTask:
    """Task to filter gaps for a specific module/file/category combination"""
    module: str
    file_path: str
    category: str
    gaps: List[Dict]


@dataclass
class FilterResult:
    """Result from filtering a single task"""
    module: str
    file_path: str
    category: str
    original_count: int
    filtered_count: int
    gaps: List[Dict]
    error: Optional[str] = None


class Wave5ParallelFilters:
    """Parallel Wave 5 FP reduction using thread pool and queues"""
    
    def __init__(self, num_workers: Optional[int] = None, verbose: bool = False):
        """
        Initialize parallel filter engine.
        
        Args:
            num_workers: Number of worker threads (auto-detect if None)
            verbose: Print progress updates
        """
        import os
        
        # Auto-detect CPU cores
        cpu_count = os.cpu_count() or 4
        self.num_workers = num_workers or max(2, min(8, cpu_count - 1))
        self.verbose = verbose
        
        # Thread-safe aggregation
        self.lock = threading.Lock()
        self.results_by_module = {}
        self.stats = {
            'tasks_processed': 0,
            'total_before': 0,
            'total_after': 0,
            'errors': 0,
        }
        
        if self.verbose:
            print(f"[INFO] Wave 5 Parallel Filter initialized with {self.num_workers} workers")
    
    def _worker_filter_task(self, task: FilterTask) -> FilterResult:
        """
        Worker thread function: filter gaps for a single category/file combo.
        
        Args:
            task: FilterTask with module/file/category/gaps
        
        Returns:
            FilterResult with filtered gaps and metadata
        """
        try:
            # Apply Wave 5 aggressive filter for this category
            filter_fn = getattr(
                Wave5AggressiveFilters,
                f'filter_{task.category}',
                lambda gaps, fp: gaps  # Fallback: no filtering
            )
            
            filtered_gaps = filter_fn(task.gaps, task.file_path)
            
            result = FilterResult(
                module=task.module,
                file_path=task.file_path,
                category=task.category,
                original_count=len(task.gaps),
                filtered_count=len(filtered_gaps),
                gaps=filtered_gaps,
                error=None
            )
            
        except Exception as e:
            result = FilterResult(
                module=task.module,
                file_path=task.file_path,
                category=task.category,
                original_count=len(task.gaps),
                filtered_count=len(task.gaps),
                gaps=task.gaps,
                error=str(e)
            )
        
        return result
    
    def _aggregate_result(self, result: FilterResult):
        """Thread-safe aggregation of filter results"""
        with self.lock:
            # Track statistics
            self.stats['tasks_processed'] += 1
            self.stats['total_before'] += result.original_count
            self.stats['total_after'] += result.filtered_count
            
            if result.error:
                self.stats['errors'] += 1
            
            # Initialize module aggregation if needed
            if result.module not in self.results_by_module:
                self.results_by_module[result.module] = {}
            
            if result.file_path not in self.results_by_module[result.module]:
                self.results_by_module[result.module][result.file_path] = []
            
            # Add filtered gaps
            self.results_by_module[result.module][result.file_path].extend(result.gaps)
            
            # Progress feedback (every 50 tasks)
            if self.verbose and self.stats['tasks_processed'] % 50 == 0:
                reduction = self.stats['total_before'] - self.stats['total_after']
                reduction_pct = (reduction / self.stats['total_before'] * 100) if self.stats['total_before'] > 0 else 0
                print(f"[...] Wave 5 Progress: {self.stats['tasks_processed']} tasks, "
                      f"{reduction:,} FP reduced ({reduction_pct:.1f}%)")
    
    def apply_parallel_filters(self, aggregate: Dict[str, Any]) -> Dict[str, Any]:
        """
        Apply Wave 5 filters in parallel across all modules/files/categories.
        
        Args:
            aggregate: Aggregate gap dict with by_file/by_category structure
        
        Returns:
            Filtered aggregate dict
        """
        
        # Build task queue
        tasks = []
        for module, module_data in aggregate.items():
            for file_path, gaps in module_data.get('by_file', {}).items():
                # Group gaps by category
                by_category = {}
                for gap in gaps:
                    cat = gap.get('category', 'unknown')
                    if cat not in by_category:
                        by_category[cat] = []
                    by_category[cat].append(gap)
                
                # Create task per category
                for category, cat_gaps in by_category.items():
                    tasks.append(FilterTask(
                        module=module,
                        file_path=file_path,
                        category=category,
                        gaps=cat_gaps
                    ))
        
        if not tasks:
            return aggregate
        
        print(f"\n[...] Wave 5 Parallel Filtering: {len(tasks)} tasks across {self.num_workers} workers")
        start_time = time.time()
        
        # Execute tasks in parallel
        with ThreadPoolExecutor(max_workers=self.num_workers, thread_name_prefix='Wave5Worker') as executor:
            futures = {
                executor.submit(self._worker_filter_task, task): task
                for task in tasks
            }
            
            completed = 0
            for future in as_completed(futures):
                try:
                    result = future.result(timeout=30)
                    self._aggregate_result(result)
                    completed += 1
                    
                    # Progress feedback
                    if self.verbose and completed % 100 == 0:
                        elapsed = time.time() - start_time
                        rate = completed / elapsed if elapsed > 0 else 0
                        remaining = (len(tasks) - completed) / rate if rate > 0 else 0
                        print(f"[...] {completed}/{len(tasks)} tasks completed ({rate:.1f} tasks/sec, "
                              f"~{remaining:.0f}s remaining)")
                
                except Exception as e:
                    print(f"[WARN] Task failed: {str(e)[:100]}")
        
        elapsed = time.time() - start_time
        print(f"[OK] Wave 5 Parallel Filtering Complete ({elapsed:.1f}s)")
        
        # Build filtered aggregate from results
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
        
        print(f"\n[INFO] Wave 5 Parallel FP Reduction Summary:")
        print(f"  Tasks Processed:   {self.stats['tasks_processed']}")
        print(f"  Total Before:      {self.stats['total_before']:,}")
        print(f"  Total After:       {self.stats['total_after']:,}")
        print(f"  Reduced:           {total_reduction:,} ({total_reduction_pct:.1f}%)")
        print(f"  Errors:            {self.stats['errors']}")
        print(f"  Elapsed:           {elapsed:.1f}s")
        print(f"  Throughput:        {len(tasks) / elapsed:.1f} tasks/sec")
        
        return filtered_aggregate


def apply_wave5_parallel_filters(aggregate: Dict[str, Any], num_workers: Optional[int] = None, verbose: bool = True) -> Dict[str, Any]:
    """
    Convenience function: Apply Wave 5 parallel filters to aggregate.
    
    Args:
        aggregate: Gap aggregate dict
        num_workers: Number of worker threads (auto-detect if None)
        verbose: Print progress updates
    
    Returns:
        Filtered aggregate dict
    """
    filter_engine = Wave5ParallelFilters(num_workers=num_workers, verbose=verbose)
    return filter_engine.apply_parallel_filters(aggregate)
