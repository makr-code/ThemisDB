#!/usr/bin/env python3
"""
Gap Scanner V3 — Unified Orchestrator

Main entry point for the gap scanner pipeline.
Loads scanners by priority tier and executes end-to-end.
"""

import sys
import json
import os
import re
import logging
from pathlib import Path
import time
from datetime import datetime
from collections import Counter
from typing import Dict, List, Tuple
import subprocess

# Add tools/ to path
sys.path.insert(0, str(Path(__file__).parent))

from gs3_base_scanner import BaseGapScanner, Gap, ScannerRegistry, GapScannerPipeline, ScannerPriority

# Import all scanner classes
from scanners.gs3_step00_uniform_full import UniformFullScanner


def _normalize_path(path: str) -> str:
    return (path or '').replace('\\', '/').lower()


# PHASE 5: External Submodule Boundary Exclusion
EXTERNAL_SUBMODULES = {
    'llama.cpp',
    'whisper.cpp',
    'vcpkg',
    'vcpkg_installed',
    'vcpkg_installed_linux',
    'onnx-clip',
}


def is_external_submodule(file_path: str) -> bool:
    """Check if a finding belongs to an external GitHub submodule"""
    normalized = _normalize_path(file_path)
    return any(sub.lower() in normalized for sub in EXTERNAL_SUBMODULES)


def filter_external_submodules(gaps: list[Gap]) -> Tuple[list[Gap], int]:
    """
    Remove findings from external submodules
    
    Returns: (filtered_gaps, count_removed)
    """
    removed_count = 0
    filtered = []
    
    for gap in gaps:
        if is_external_submodule(gap.file):
            removed_count += 1
            logging.debug(f"  EXTERNAL_SUBMODULE_FILTERED: {gap.file}:{gap.line}")
        else:
            filtered.append(gap)
    
    return filtered, removed_count


def _resolve_gaps_to_repo_paths(gaps: list[Gap], source_dir: str, repo_root: str = '.') -> Tuple[list[Gap], int]:
    """
    Resolve relative file paths from scanner to absolute repo paths
    
    The scanner stores paths relative to source_dir (e.g., 'explain_plan.cpp' when scanning './src/graph')
    This function reconstructs the absolute repo path (e.g., 'src/graph/explain_plan.cpp')
    needed for accurate scope classification.
    
    Returns: (gaps with reconstructed paths, count_resolved)
    """
    source_path = Path(source_dir).resolve()
    repo_path = Path(repo_root).resolve()
    count_resolved = 0
    
    for gap in gaps:
        file_path = Path(gap.file)
        
        # Skip if already absolute or starts with src/include/etc
        if file_path.is_absolute():
            continue
        if gap.file.startswith(('src/', 'include/', 'tests/', 'benchmarks/')):
            continue  # Already in correct form
        
        # Reconstruct: source_dir + relative_file
        reconstructed = source_path / file_path
        
        # Get path relative to repo root
        try:
            rel_to_repo = reconstructed.relative_to(repo_path)
            gap.file = str(rel_to_repo).replace('\\', '/')
            count_resolved += 1
        except ValueError:
            # Path is outside repo, keep as-is
            pass
    
    return gaps, count_resolved


def _classify_scope(path: str) -> str:
    normalized = _normalize_path(path)
    if normalized.startswith('tests/'):
        return 'themis_tests'
    if normalized.startswith('benchmarks/'):
        return 'themis_benchmarks'
    if normalized.startswith(('src/', 'include/', 'tools/', 'scripts/', 'cmake/', 'docs/', 'examples/')):
        return 'themis_core'
    return 'third_party'


def _build_scope_breakdown(gaps: list[Gap]) -> dict:
    """Build scope-aware summary used by console output and exported JSON metadata."""
    scope_counts = Counter()
    for gap in gaps:
        scope_counts[_classify_scope(gap.file)] += 1

    total = len(gaps)
    percentages = {
        key: round((count * 100.0 / total), 2) if total else 0.0
        for key, count in scope_counts.items()
    }

    # Ensure stable keys in output, even when count is zero.
    for key in ('themis_core', 'themis_tests', 'themis_benchmarks', 'third_party'):
        scope_counts.setdefault(key, 0)
        percentages.setdefault(key, 0.0)

    return {
        'policy': {
            'themis_core': ['src/', 'include/', 'tools/', 'scripts/', 'cmake/', 'docs/', 'examples/'],
            'themis_tests': ['tests/'],
            'themis_benchmarks': ['benchmarks/'],
            'third_party': ['all other paths']
        },
        'counts': dict(scope_counts),
        'percentages': percentages
    }


def _classify_module(path: str) -> str:
    normalized = _normalize_path(path).lstrip('./')
    parts = [p for p in normalized.split('/') if p]
    if not parts:
        return '_unscoped'
    if parts[0] == 'src' and len(parts) > 1:
        return parts[1]
    if parts[0] in ('include', 'tests', 'benchmarks', 'internal', 'docs', 'tools'):
        return parts[0]
    return parts[0]


def _classify_gap_class(pattern: str) -> str:
    p = (pattern or '').lower()
    if any(k in p for k in ('data_race', 'deadlock', 'race', 'atomic')):
        return 'Concurrency'
    if any(k in p for k in ('null_dereference', 'buffer', 'pointer', 'use_after', 'double_free', 'size_assumption', 'leak')):
        return 'MemorySafety'
    if any(k in p for k in ('version_tracking', 'conflict_resolution', 'stale_write', 'consistency')):
        return 'VersioningConflict'
    if any(k in p for k in ('missing_doxygen', 'docs_', 'markdown_anchor', 'markdown_link')):
        return 'Documentation'
    if any(k in p for k in ('no_retry', 'timeout', 'circuit_breaker', 'fallback')):
        return 'ReliabilityRetry'
    if any(k in p for k in ('integrity', 'injection', 'signature', 'auth', 'crypto', 'tls')):
        return 'SecurityIntegrity'
    if any(k in p for k in ('layer_dependency', 'interface', 'abstract_', 'i_prefix', 'module_doc', 'adr_reference')):
        return 'ArchitectureContract'
    return 'General'


def _build_ollama_markdown_report(
    gaps: list[Gap],
    output_path: Path,
    source_json_path: Path,
    scan_mode: str,
    docs_doxygen: bool,
    scope_breakdown: dict,
    max_prompt_buckets: int = 0,
    context_chars: int = 120,
    template_lines: int = 10,
) -> None:
    actionable_rows: List[Tuple[int, str, int, str, str, str]] = []
    grouped_actionable: Dict[Tuple[str, str, str, str], int] = Counter()
    grouped_actionable_meta: Dict[Tuple[str, str, str, str], Dict[str, object]] = {}

    for gap in gaps:
        sev = str(gap.severity or 'LOW').upper()
        if sev not in ('CRITICAL', 'HIGH'):
            continue

        scope = _classify_scope(gap.file)
        if scope == 'third_party':
            continue

        module = _classify_module(gap.file)
        pattern = str(gap.type or 'unknown')
        key = (module, gap.file, pattern, sev)
        grouped_actionable[key] += 1

        meta = grouped_actionable_meta.setdefault(
            key,
            {
                'line': int(getattr(gap, 'line', 0) or 0),
                'description': str(getattr(gap, 'description', '') or ''),
                'context': str(getattr(gap, 'context', '') or ''),
            },
        )
        current_line = int(getattr(gap, 'line', 0) or 0)
        if current_line and (not meta.get('line') or current_line < int(meta.get('line') or 0)):
            meta['line'] = current_line
        if not meta.get('description'):
            meta['description'] = str(getattr(gap, 'description', '') or '')
        if not meta.get('context'):
            meta['context'] = str(getattr(gap, 'context', '') or '')

    for (module, file_path, pattern, sev), count in grouped_actionable.items():
        score = count * (2 if sev == 'CRITICAL' else 1)
        actionable_rows.append((score, module, count, sev, file_path, pattern))

    actionable_rows.sort(key=lambda x: (-x[0], -x[2], x[3], x[1], x[4], x[5]))
    prompt_rows = actionable_rows if max_prompt_buckets <= 0 else actionable_rows[:max_prompt_buckets]

    lines: List[str] = []
    lines.append('# ThemisDB Gap Worklist for Remote Ollama gemma4')
    lines.append('')
    lines.append('- [ ] Scope: actionable themis_core findings only (third_party is informational).')
    lines.append('')
    lines.append('## Work Items')
    lines.append('')

    for i, (_score, module, _count, sev, file_path, pattern) in enumerate(prompt_rows, 1):
        meta = grouped_actionable_meta.get((module, file_path, pattern, sev), {})
        line = int(meta.get('line') or 0)
        description = str(meta.get('description') or '').strip() or 'n/a'
        context = str(meta.get('context') or '').strip()
        short_context = 'n/a'
        if context:
            short_context = context.replace('\n', ' ').strip()
            if len(short_context) > context_chars:
                short_context = short_context[:max(0, context_chars - 3)] + '...'

        cls = _classify_gap_class(pattern)
        loc = f'{file_path}:{line if line else "n/a"}'

        lines.append(f'- [ ] {sev} | {module} | {pattern} | {loc}')
        lines.append('```text')
        if template_lines <= 6:
            # strict 6-line ultra-compact block per item
            lines.append('// ROUTING HINT: ollama-local | Model: gemma4:latest')
            lines.append(f'// Class: {cls} | Problem: {pattern}')
            lines.append(f'// Location: {loc}')
            lines.append(f'// Description: {description}')
            lines.append(f'// Context: {short_context}')
            lines.append('Task+Output+Constraints: fix only this finding; return files+diff+tests+scanner-delta; max 3 files; preserve API/ABI; update Doxygen for public C++ API changes.')
        else:
            # strict 10-line compact block per item
            lines.append('// ROUTING HINT: ollama-local')
            lines.append('// Model: gemma4:latest')
            lines.append(f'// Class: {cls}')
            lines.append(f'// Location: {loc}')
            lines.append(f'// Problem: {pattern}')
            lines.append(f'// Description: {description}')
            lines.append(f'// Context: {short_context}')
            lines.append('Task: Fix this finding only with minimal changes.')
            lines.append('Output: files, diff, tests, scanner-delta; keep it concise.')
            lines.append('Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.')
        lines.append('```')
        lines.append('')

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text('\n'.join(lines) + '\n', encoding='utf-8')


def check_cache_freshness_phase3(cache_file: Path, findings: list[Gap], repo_root: str, force_refresh: bool = False) -> Tuple[Dict[str, int], List[str]]:
    """
    IMPROVEMENT PHASE 3: Cache stale detection
    
    Validates cached findings against actual files on disk.
    Returns: (stats, missing_files)
    
    Stats:
    - cache_age_hours: How old the cache is (if exists)
    - files_checked: Total findings checked
    - files_missing: Non-existent files found
    - cache_valid: Is cache > 10% corrupt (missing files)?
    """
    
    print("\n" + "=" * 80)
    print("Phase 3 — Cache Stale Detection")
    print("=" * 80)
    
    repo_path = Path(repo_root)
    stats = {
        'cache_exists': cache_file.exists(),
        'cache_age_hours': 0.0,
        'files_checked': len(findings),
        'files_missing': 0,
        'cache_valid': True,
        'missing_threshold': 0.1,
    }
    
    missing_files = []
    
    # Check cache age if it exists
    if cache_file.exists():
        cache_mtime = cache_file.stat().st_mtime
        cache_age_seconds = time.time() - cache_mtime
        stats['cache_age_hours'] = round(cache_age_seconds / 3600.0, 2)
        logging.info(f"  Cache age: {stats['cache_age_hours']} hours")
    else:
        logging.info(f"  No cache file found: {cache_file}")
    
    # Validate files exist
    for gap in findings:
        file_path = repo_path / gap.file
        if not file_path.exists():
            stats['files_missing'] += 1
            missing_files.append(str(gap.file))
            logging.warning(f"  Missing: {gap.file}")
    
    # Check if > 10% of files are missing (cache corruption indicator)
    if stats['files_checked'] > 0:
        missing_ratio = stats['files_missing'] / stats['files_checked']
        stats['cache_valid'] = missing_ratio <= stats['missing_threshold']
        
        if not stats['cache_valid']:
            logging.warning(f"  ⚠️  Cache INVALID: {stats['files_missing']}/{stats['files_checked']} files missing ({missing_ratio*100:.1f}% > 10% threshold)")
        elif missing_ratio > 0:
            logging.info(f"  ℹ️  Cache partially stale: {stats['files_missing']}/{stats['files_checked']} files missing ({missing_ratio*100:.1f}%)")
        else:
            logging.info(f"  ✓ Cache valid: all {stats['files_checked']} files exist")
    
    if force_refresh and stats['files_missing'] > 0:
        logging.info(f"  [--force-refresh] Would trigger rescan due to {stats['files_missing']} missing files")
    
    return stats, missing_files


def enrich_output_metadata_phase4(gaps: list[Gap], repo_root: str, verify_stats: Dict, cache_stats: Dict, missing_files: List[str]) -> Dict:
    """
    IMPROVEMENT PHASE 4: Enriched output metadata
    
    Adds comprehensive context to exported findings for debugging + transparency.
    Returns: metadata dict
    
    Includes:
    - Scanner version + timestamp
    - Python version + environment
    - File statistics
    - Cache validation status
    - Missing file recommendations
    """
    
    print("\n" + "=" * 80)
    print("Phase 4 — Enriched Output Metadata")
    print("=" * 80)
    
    import platform
    import sys as sys_module
    
    repo_path = Path(repo_root)
    
    # Count files by scope
    scope_counts = {}
    for gap in gaps:
        scope = _classify_scope(gap.file)
        scope_counts[scope] = scope_counts.get(scope, 0) + 1
    
    # Count findings by severity
    severity_counts = {}
    for gap in gaps:
        sev = str(gap.severity or 'UNKNOWN')
        severity_counts[sev] = severity_counts.get(sev, 0) + 1
    
    # Build recommendations for missing files
    recommendations = []
    if missing_files:
        recommendations.append(f"Found {len(missing_files)} files referenced in findings but not on disk")
        recommendations.append(f"Potential causes: build artifacts, git-ignored test files, or stale cache")
        recommendations.append(f"Action: Run with --force-refresh to regenerate cache, or check git status")
    
    if not cache_stats['cache_valid']:
        recommendations.append(f"Cache is {cache_stats['files_missing']}/{cache_stats['files_checked']} files corrupt")
        recommendations.append(f"Action: Force refresh with --force-refresh or delete cache file manually")
    
    metadata = {
        'version': '3.0-Phase4',
        'timestamp': datetime.now().isoformat(),
        'python_version': f"{sys_module.version_info.major}.{sys_module.version_info.minor}.{sys_module.version_info.micro}",
        'platform': platform.platform(),
        'repo_root': str(repo_path),
        
        'scan_statistics': {
            'total_gaps': len(gaps),
            'by_severity': severity_counts,
            'by_scope': scope_counts,
        },
        
        'verification_phase1_phase2': {
            'file_existence_checked': True,
            'file_not_found_removed': verify_stats.get('file_not_found', 0),
            'external_submodule_removed': verify_stats.get('external_submodule_filtered', 0),
            'findings_downgraded': verify_stats.get('downgraded', 0),
            'findings_kept': verify_stats.get('kept', 0),
            'classifications': {
                'test_mock': verify_stats.get('test_mock', 0),
                'guarded_stub': verify_stats.get('guarded_stub', 0),
                'placeholder': verify_stats.get('placeholder', 0),
                'real_gap': verify_stats.get('real_gap', 0),
            }
        },
        
        'cache_phase3': {
            'cache_exists': cache_stats['cache_exists'],
            'cache_age_hours': cache_stats['cache_age_hours'],
            'cache_valid': cache_stats['cache_valid'],
            'files_missing': cache_stats['files_missing'],
            'missing_file_threshold_pct': cache_stats['missing_threshold'] * 100,
        },
        
        'missing_files_detailed': missing_files[:100] if missing_files else [],  # Cap to first 100
        'missing_files_count': len(missing_files),
        'recommendations': recommendations,
    }
    
    logging.info(f"  Metadata enriched: {len(metadata)} top-level keys")
    logging.info(f"  Recommendations: {len(recommendations)} items")
    
    return metadata


def verify_gaps_phase1_phase2(gaps: list[Gap], repo_root: str) -> Tuple[list[Gap], Dict[str, int]]:
    """
    IMPROVEMENT PHASE 1 & 2: Multi-factor gap verification
    
    Returns verified gaps with false-positives removed and severity re-assessed
    Returns: (verified_gaps, stats)
    
    Phase 1: File existence check — removes FILE_NOT_FOUND false-positives
    Phase 2: Multi-factor classification — re-assesses severity based on context
    Phase 5: External submodule filtering — removes findings from external GitHub submodules
    """
    
    logging.basicConfig(level=logging.INFO, format='%(levelname)s — %(message)s')
    
    print("\n" + "=" * 80)
    print("Gap Verification Phase (L0.5) — Phase 1 & 2: File Existence + Classification")
    print("=" * 80)
    
    repo_path = Path(repo_root)
    verified_gaps = []
    stats = {
        'total_input': len(gaps),
        'file_not_found': 0,
        'external_submodule_filtered': 0,
        'downgraded': 0,
        'kept': 0,
        'test_mock': 0,
        'guarded_stub': 0,
        'placeholder': 0,
        'real_gap': 0,
        'unknown': 0,
        'out_of_range': 0,
    }
    
    for gap in gaps:
        file_full_path = repo_path / gap.file
        
        # PHASE 1: File Existence Check
        if not file_full_path.exists():
            stats['file_not_found'] += 1
            logging.warning(f"  FALSE_POSITIVE (FILE_NOT_FOUND): {gap.file}:{gap.line}")
            continue  # Skip this finding
        
        # PHASE 5: External Submodule Check
        if is_external_submodule(gap.file):
            stats['external_submodule_filtered'] += 1
            logging.info(f"  EXTERNAL_SUBMODULE_FILTERED: {gap.file}:{gap.line}")
            continue  # Skip external submodule findings
        
        # PHASE 2: Multi-Factor Classification + Severity Re-Assessment
        classification, severity_action = _classify_gap_and_assess(file_full_path, gap, repo_path)
        
        # Update gap with verification metadata
        gap.verification = {
            'status': 'VERIFIED',
            'classification': classification,
            'original_severity': gap.severity,
            'severity_action': severity_action,
        }
        
        # Apply severity re-rating
        if severity_action.startswith('DOWNGRADE_'):
            new_sev = severity_action.replace('DOWNGRADE_', '')
            gap.severity = new_sev
            stats['downgraded'] += 1
            logging.info(f"  DOWNGRADE: {gap.file}:{gap.line} ({gap.severity} → {new_sev}, {classification})")
        else:
            stats['kept'] += 1
        
        stats[classification.lower().replace(' ', '_')] += 1
        verified_gaps.append(gap)
    
    print(f"\n[VERIFICATION SUMMARY]")
    print(f"  Input gaps: {stats['total_input']}")
    print(f"  Output gaps: {len(verified_gaps)}")
    print(f"  Removed (FILE_NOT_FOUND): {stats['file_not_found']}")
    print(f"  Removed (EXTERNAL_SUBMODULE): {stats['external_submodule_filtered']}")
    print(f"  Downgraded (severity reduced): {stats['downgraded']}")
    print(f"  Classifications:")
    print(f"    - TEST_MOCK: {stats['test_mock']}")
    print(f"    - GUARDED_STUB: {stats['guarded_stub']}")
    print(f"    - PLACEHOLDER: {stats['placeholder']}")
    print(f"    - REAL_GAP: {stats['real_gap']}")
    
    return verified_gaps, stats


def _classify_gap_and_assess(file_path: Path, gap: Gap, repo_root: Path) -> Tuple[str, str]:
    """
    Multi-factor classification with severity re-assessment
    
    Returns: (classification, severity_action)
      - classification: 'TEST_MOCK' | 'GUARDED_STUB' | 'PLACEHOLDER' | 'REAL_GAP'
      - severity_action: 'KEEP_SEVERITY' | 'DOWNGRADE_HIGH' | 'DOWNGRADE_MEDIUM' | 'DOWNGRADE_LOW'
    """
    
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception as e:
        logging.error(f"  Error reading {file_path}: {e}")
        return 'UNKNOWN', 'KEEP_SEVERITY'
    
    line_num = int(gap.line or 1)
    if line_num < 1 or line_num > len(lines):
        return 'OUT_OF_RANGE', 'DOWNGRADE_LOW'
    
    # Extract source context (±5 lines)
    start = max(0, line_num - 6)
    end = min(len(lines), line_num + 4)
    context_lines = lines[start:end]
    source_line = lines[line_num - 1] if line_num <= len(lines) else ''
    
    # Factor 1: Test code marker?
    if str(file_path).startswith(str(repo_root / 'tests')):
        if any(marker in source_line for marker in ['MOCK', 'TEST', '// Mock', '// TEST']):
            return 'TEST_MOCK', 'DOWNGRADE_LOW'
    
    # Factor 2: TODO/STUB/TEMPORARY marker?
    if any(marker in source_line for marker in ['TODO', 'FIXME', 'STUB', 'TEMPORARY', 'WIP']):
        return 'PLACEHOLDER', 'DOWNGRADE_MEDIUM'
    
    # Factor 3: Guarded pattern (defensive code)?
    full_context = ''.join(context_lines)
    if re.search(r'(if|while|for)\s*\(', source_line):
        if 'return' in source_line or 'return' in full_context:
            return 'GUARDED_STUB', 'DOWNGRADE_HIGH'
    
    # Factor 4: Defensive error handling (simple returns)?
    if any(pattern in source_line for pattern in ['return {};', 'return "";', 'return null', 'return nullptr', 'return false']):
        if any(check in full_context for check in ['if (', 'if!', 'assert', 'CHECK']):
            return 'GUARDED_STUB', 'DOWNGRADE_HIGH'
    
    # Default: Real gap
    return 'REAL_GAP', 'KEEP_SEVERITY'



def main():
    """Main orchestrator entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description="ThemisDB Gap Scanner V3 Pipeline")
    parser.add_argument('source_dirs', nargs='*',
                        help='Source directories to scan (default: ./src)')
    parser.add_argument('--output', '-o', default='ai_working/gap_scan_results.json',
                        help='Output JSON file (default: ai_working/gap_scan_results.json)')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Verbose output')
    parser.add_argument('--scan-mode', choices=['fast', 'full', 'thorough'], default='full',
                        help='Scanner mode: fast skips expensive docs checks, full/thorough runs all checks (default: full)')
    parser.add_argument('--docs-doxygen', action='store_true',
                        help='Run optional XML-first Doxygen checks inside docs scanner (prefers Doxyfile.audit and validates XML index)')
    parser.add_argument('--md-report', default='ai_working/gap_scan_report_ollama_gemma4.md',
                        help='Write markdown remediation report template for remote Ollama gemma4 (default: ai_working/gap_scan_report_ollama_gemma4.md)')
    parser.add_argument('--md-report-max-prompt-buckets', type=int, default=0,
                        help='Maximum actionable buckets to turn into copy/paste gemma4 prompts (default: 0 = all)')
    parser.add_argument('--md-report-context-chars', type=int, default=120,
                        help='Maximum context characters embedded in each gemma4 prompt (default: 120)')
    parser.add_argument('--md-report-template-lines', type=int, choices=[6, 10], default=10,
                        help='Prompt template size per work item (6 ultra-compact or 10 compact, default: 10)')
    parser.add_argument('--force-refresh', action='store_true',
                        help='Force rescan (Phase 3) even if cache exists; removes stale findings (default: False)')
    parser.add_argument('--file-centric', dest='file_centric', action='store_true',
                        help='Run scanners in file-centric mode (read each file once and call scan_file hooks). Default: enabled')
    parser.add_argument('--no-file-centric', dest='file_centric', action='store_false',
                        help='Disable file-centric mode and run scanners as before')
    parser.set_defaults(file_centric=True)
    parser.add_argument('--include-graph', default='ai_working/include_graph.json',
                        help='Path to include/markdown graph JSON to inject into file contexts (default: ai_working/include_graph.json)')
    parser.add_argument('--open-visualizer', action='store_true',
                        help='Open GUI visualizer after run using include-graph or output JSON')
    
    args = parser.parse_args()

    # Resolve source directories; default to './src' when none provided.
    source_dirs = args.source_dirs if args.source_dirs else ['./src']
    # Treat 'thorough' as an alias for 'full'.
    scan_mode_effective = 'full' if args.scan_mode == 'thorough' else args.scan_mode

    # Create registry
    registry = ScannerRegistry()

    registry.register(UniformFullScanner(scan_mode=scan_mode_effective, docs_doxygen=args.docs_doxygen))
    
    # Create and run pipeline
    pipeline = GapScannerPipeline(registry)
    # Configure file-centric mode and include graph
    pipeline.file_centric_mode = args.file_centric
    pipeline.include_graph_path = args.include_graph
    
    print("\n" + "=" * 80)
    print("ThemisDB Gap Scanner V3 Pipeline")
    print("=" * 80)
    print(f"[CONFIG] scan_mode={args.scan_mode} (effective={scan_mode_effective}), docs_doxygen={args.docs_doxygen}")
    print(f"[CONFIG] source_dirs={source_dirs}")
    
    start_time = time.time()

    # Scan each requested directory and merge gaps.
    gaps: list = []
    for source_dir in source_dirs:
        print(f"\n[SCAN] Scanning directory: {source_dir}")
        dir_gaps = pipeline.execute(source_dir, verbose=args.verbose)
        # Resolve paths relative to this specific source directory.
        dir_gaps, dir_paths_resolved = _resolve_gaps_to_repo_paths(dir_gaps, source_dir, '.')
        if dir_paths_resolved:
            print(f"  Paths resolved: {dir_paths_resolved}")
        gaps.extend(dir_gaps)

    elapsed = time.time() - start_time

    # Final path resolution pass for any remaining relative paths.
    print("\n[PATH RESOLUTION] Final path resolution pass...")
    gaps, paths_resolved = _resolve_gaps_to_repo_paths(gaps, '.', '.')
    print(f"  Paths resolved: {paths_resolved}")
    
    # IMPROVEMENT: Apply Phase 1 & 2 verification before export (use '.' as repo_root, not source_dir)
    gaps, verify_stats = verify_gaps_phase1_phase2(gaps, '.')  # Use repo root, not scan dir
    
    # IMPROVEMENT: Apply Phase 3 — Cache stale detection
    cache_file = Path(args.output)
    cache_stats, missing_files = check_cache_freshness_phase3(cache_file, gaps, '.', args.force_refresh)
    
    # IMPROVEMENT: Apply Phase 4 — Enriched metadata
    metadata_enriched = enrich_output_metadata_phase4(gaps, '.', verify_stats, cache_stats, missing_files)
    
    # Filter external submodules (Phase 5)
    print("\n[PHASE 5] Filtering external GitHub submodules...")
    gaps_before_ext_filter = len(gaps)
    gaps, ext_filtered = filter_external_submodules(gaps)
    print(f"  External submodules filtered: {ext_filtered}")
    if ext_filtered > 0:
        logging.info(f"  Removed {ext_filtered} findings from external submodules")
    
    scope_breakdown = _build_scope_breakdown(gaps)
    
    # Export results
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    pipeline.export_json(output_path)

    # Enrich exported JSON with scope-separated summary + Phase 3-4 metadata
    with open(output_path, 'r', encoding='utf-8') as f:
        exported = json.load(f)
    exported.setdefault('metadata', {})['scope_breakdown'] = scope_breakdown
    exported.setdefault('metadata', {})['verification_phase_3_4'] = metadata_enriched
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(exported, f, indent=2)

    md_report_path = Path(args.md_report)
    if not md_report_path.is_absolute():
        md_report_path = Path.cwd() / md_report_path
    _build_ollama_markdown_report(
        gaps=gaps,
        output_path=md_report_path,
        source_json_path=output_path,
        scan_mode=args.scan_mode,
        docs_doxygen=args.docs_doxygen,
        scope_breakdown=scope_breakdown,
        max_prompt_buckets=args.md_report_max_prompt_buckets,
        context_chars=args.md_report_context_chars,
        template_lines=args.md_report_template_lines,
    )
    
    print(f"\n[OK] Results exported to {output_path}")
    print(f"[OK] Markdown report exported to {md_report_path}")
    print(f"[OK] Completed in {elapsed:.2f}s")

    # Optionally open visualizer GUI (non-blocking)
    if getattr(args, 'open_visualizer', False):
        graph_to_open = args.include_graph if os.path.exists(args.include_graph) else str(output_path)
        if not os.path.exists(graph_to_open):
            print(f"Visualizer: graph file not found: {graph_to_open}")
        else:
            try:
                cmd = [sys.executable, str(Path(__file__).parent / 'visualizer_tk.py'), '--graph', str(graph_to_open)]
                # start detached
                subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                print(f"[OK] Launched visualizer for {graph_to_open}")
            except Exception as e:
                print(f"Failed to launch visualizer: {e}")
    
    # Print summary
    by_severity = {}
    by_type = {}
    
    for gap in gaps:
        sev = gap.severity
        by_severity[sev] = by_severity.get(sev, 0) + 1
        
        typ = gap.type
        by_type[typ] = by_type.get(typ, 0) + 1
    
    print(f"\n[SUMMARY — After Verification (Phase 1, 2 & 5)]")
    print(f"Total gaps: {len(gaps)}")
    print(f"  Input (raw scanner): {verify_stats['total_input']}")
    print(f"  Removed (FILE_NOT_FOUND): {verify_stats['file_not_found']}")
    print(f"  Removed (EXTERNAL_SUBMODULE): {verify_stats['external_submodule_filtered']}")
    print(f"  Downgraded (severity re-assessed): {verify_stats['downgraded']}")
    print(f"  Kept (unchanged): {verify_stats['kept']}")
    
    print(f"\nBy Severity:")
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        if sev in by_severity:
            print(f"  {sev}: {by_severity[sev]}")
    
    print(f"\nTop Gap Types:")
    for typ, count in sorted(by_type.items(), key=lambda x: -x[1])[:10]:
        print(f"  {typ}: {count}")

    print(f"\nBy Scope (ThemisDB vs Tests/Benchmarks vs Third-Party):")
    for key in ('themis_core', 'themis_tests', 'themis_benchmarks', 'third_party'):
        count = scope_breakdown['counts'].get(key, 0)
        pct = scope_breakdown['percentages'].get(key, 0.0)
        print(f"  {key}: {count} ({pct}%)")
    
    print("=" * 80)
    
    return 0 if gaps else 1


if __name__ == "__main__":
    sys.exit(main())
