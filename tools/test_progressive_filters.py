#!/usr/bin/env python3
"""Quick test of Progressive Context Filter Pipeline"""

from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).parent))

from gap_scanner_v3_progressive_context_filters import ProgressiveContextFilter

# Test data: 10 sample gaps
test_gaps = [
    {
        'file': 'src/server/wasm_handler_registry.cpp',
        'line': 155,
        'category': 'performance',
        'severity': 'MEDIUM',
        'description': 'vector::push_back in loop without prior reserve()',
    },
    {
        'file': 'src/governance/policy_review.cpp',
        'line': 719,
        'category': 'performance',
        'severity': 'MEDIUM',
        'description': 'vector::push_back in loop without prior reserve()',
    },
    {
        'file': 'src/ingestion/steps/ner_step.cpp',
        'line': 113,
        'category': 'uncaught_exception',
        'severity': 'MEDIUM',
        'description': 'Generic catch(...) – specific exception types ignored',
    },
    {
        'file': 'src/query/functions/tensor_functions.cpp',
        'line': 177,
        'category': 'pointer_arithmetic',
        'severity': 'HIGH',
        'description': 'Pointer/array access without bounds validation',
    },
    {
        'file': 'src/sharding/stream_protocol.cpp',
        'line': 856,
        'category': 'observability',
        'severity': 'MEDIUM',
        'description': 'No latency measurement for operation',
    },
    {
        'file': 'src/cdc/changefeed.cpp',
        'line': 612,
        'category': 'no_health_check',
        'severity': 'MEDIUM',
        'description': 'Status field defined but no initialization or health check',
    },
    {
        'file': 'src/performance/adaptive_query_compiler.cpp',
        'line': 118,
        'category': 'determinism',
        'severity': 'HIGH',
        'description': 'Floating-point exact comparison (use tolerance/epsilon)',
    },
    {
        'file': 'src/utils/pki_client.cpp',
        'line': 832,
        'category': 'manual_cleanup',
        'severity': 'MEDIUM',
        'description': 'Manual cleanup outside exception handler – not exception-safe',
    },
    {
        'file': 'src/server/graph_api_handler.cpp',
        'line': 581,
        'category': 'uncaught_exception',
        'severity': 'MEDIUM',
        'description': 'Generic catch(...) – specific exception types ignored',
    },
    {
        'file': 'src/llm/lora_framework/gpu_lora_layers.cpp',
        'line': 323,
        'category': 'llm_ai_safety',
        'severity': 'HIGH',
        'description': 'User input passed to LLM without normalization/sanitization',
    }
]

def main():
    print("=" * 80)
    print("[PROGRESSIVE CONTEXT FILTER - QUICK TEST]")
    print("=" * 80)
    
    repo_root = Path(__file__).parent.parent
    print(f"\nRepository root: {repo_root}\n")
    
    filter_obj = ProgressiveContextFilter(repo_root)
    
    print(f"[INFO] Input: {len(test_gaps)} test gaps\n")
    
    remaining, stats = filter_obj.apply_progressive_filtering(test_gaps)
    
    print("\n" + "=" * 80)
    print("[RESULTS]")
    print("=" * 80)
    print(f"\nFinal gap count: {len(remaining)}")
    print(f"Total eliminated: {stats['total_eliminated']}")
    print(f"Remaining: {stats['remaining']}")
    
    total_reduction_pct = (stats['total_eliminated'] / len(test_gaps) * 100) if test_gaps else 0
    print(f"\nReduction: {total_reduction_pct:.1f}%\n")
    
    print("[ELIMINATION BREAKDOWN]")
    for wave, count in stats.items():
        if wave.startswith('wave') and count > 0:
            print(f"  {wave}: {count}")
    
    if remaining:
        print(f"\n[REMAINING GAPS - {len(remaining)}]")
        for gap in remaining:
            print(f"  • {gap.get('file', 'unknown')}:{gap.get('line')} "
                  f"({gap.get('category', 'unknown')})")

if __name__ == '__main__':
    main()
