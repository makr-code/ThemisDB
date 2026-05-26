#!/usr/bin/env python3
"""
Generate 7-phase GitHub issues for remaining 55 modules.
"""

import json
from pathlib import Path
from datetime import datetime

def analyze_remaining_modules():
    """Analyze which modules need GitHub issues"""
    
    # Load aggregate
    with open('ai_working/gap_scan_v3_aggregate.json', 'r') as f:
        agg = json.load(f)
    
    all_modules = sorted(agg.keys())
    top_10_modules = ['llm', 'server', 'query', 'sharding', 'index', 'storage', 'analytics', 'rag', 'security', 'content']
    remaining_modules = [m for m in all_modules if m not in top_10_modules]
    
    print(f"""
╔════════════════════════════════════════════════════════════════════════════════╗
║    GENERATING 7-PHASE ISSUES FOR REMAINING 55 MODULES                         ║
╚════════════════════════════════════════════════════════════════════════════════╝

CURRENT STATUS:
  Total Modules: {len(all_modules)}
  With Issues: 10 (GitHub #5245-#5254) ✅
  Remaining: {len(remaining_modules)} ⏳
  
REMAINING MODULES BY GAP COUNT:
""")
    
    # Sort remaining by gap count
    remaining_with_gaps = [(m, len(agg[m])) for m in remaining_modules]
    remaining_with_gaps.sort(key=lambda x: -x[1])
    
    for i, (mod, gaps) in enumerate(remaining_with_gaps[:20], 1):
        print(f"  {i:2d}. {mod:30s} {gaps:6,d} gaps")
    
    if len(remaining_with_gaps) > 20:
        print(f"  ... and {len(remaining_with_gaps) - 20} more")
    
    # Estimate effort
    total_remaining_gaps = sum(gaps for _, gaps in remaining_with_gaps)
    avg_per_module = total_remaining_gaps / len(remaining_modules) if remaining_modules else 0
    
    print(f"""
EFFORT ESTIMATION:
  Total Gaps (Remaining): {total_remaining_gaps:,}
  Average per Module: {avg_per_module:.0f} gaps
  Estimated Effort: ~300-400 hours manual (with automation: 90-120 hours)
  
NEXT STEPS:
  1. Generate 7-phase markdown for each remaining module
  2. Create GitHub issues via gh CLI
  3. Link all issues in master tracker
  4. Test AI agent execution workflow
  5. Scale Phase 3-7 automation
  
TIMELINE:
  Issue Generation: ~30 minutes (automated)
  GitHub Creation: ~10 minutes (batch API)
  Total: ~40 minutes to complete full 65-module coverage
""")
    
    return remaining_modules, remaining_with_gaps

if __name__ == '__main__':
    remaining, remaining_gaps = analyze_remaining_modules()
    
    # Save for next phase
    analysis = {
        'timestamp': datetime.now().isoformat(),
        'remaining_count': len(remaining),
        'modules': [{'name': m, 'gaps': gaps} for m, gaps in remaining_gaps]
    }
    
    with open('ai_working/remaining_modules_analysis.json', 'w') as f:
        json.dump(analysis, f, indent=2)
    
    print(f"\n[OK] Analysis saved: ai_working/remaining_modules_analysis.json")
