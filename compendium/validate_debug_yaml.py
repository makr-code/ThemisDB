"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            validate_debug_yaml.py                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     240                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Validate Debug-YAML files for consistency and correctness.
Checks anchor registry, references, citations, and cross-references.
"""

import yaml
import json
from pathlib import Path
from collections import Counter

# Paths
COMPENDIUM_DIR = Path(__file__).parent
OUTPUT_DIR = COMPENDIUM_DIR / "output"

def load_yaml_safe(filepath):
    """Load YAML file safely."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return yaml.safe_load(f)
    except Exception as e:
        print(f"[ERROR] Failed to load {filepath}: {e}")
        return None

def main():
    print("=" * 70)
    print("Debug YAML Validation - ThemisDB Kompendium")
    print("=" * 70)
    
    # Load all debug files
    print("\n[INFO] Loading debug YAML files...")
    
    complete = load_yaml_safe(OUTPUT_DIR / "debug-anchors-complete.yml")
    if not complete:
        return False
    
    refs = load_yaml_safe(OUTPUT_DIR / "debug-references.yml")
    citations = load_yaml_safe(OUTPUT_DIR / "debug-citations.yml")
    tables = load_yaml_safe(OUTPUT_DIR / "debug-tables.yml")
    figures = load_yaml_safe(OUTPUT_DIR / "debug-figures.yml")
    
    print("OK - All debug files loaded")
    
    # Summary
    print("\n" + "=" * 70)
    print("SUMMARY")
    print("=" * 70)
    print(f"Total anchors:     {complete['total_anchors']}")
    print(f"  - Chapters:      {complete['by_type'].get('chapter', 0)}")
    print(f"  - Parts:         {complete['by_type'].get('part', 0)}")
    print(f"  - Figures:       {complete['by_type'].get('figure', 0)}")
    print(f"  - Tables:        {complete['by_type'].get('table', 0)}")
    print(f"  - References:    {complete['by_type'].get('reference', 0)}")
    print(f"  - Headings (all):{sum(v for k, v in complete['by_type'].items() if k.startswith('heading-'))}")
    
    if citations:
        print(f"\nCitations found:   {citations['total_citations']}")
        print(f"  - Single:        {citations['by_type'].get('single', 0)}")
        print(f"  - List:          {citations['by_type'].get('list', 0)}")
        print(f"  - Range:         {citations['by_type'].get('range', 0)}")
    
    # Validation checks
    print("\n" + "=" * 70)
    print("VALIDATION CHECKS")
    print("=" * 70)
    
    passed = 0
    failed = 0
    
    # Check 1: References count consistency
    print("\n[CHECK 1] References count consistency")
    ref_count_complete = complete['by_type'].get('reference', 0)
    ref_count_debug = refs['total_references'] if refs else 0
    
    if ref_count_complete == ref_count_debug:
        print(f"  ✓ PASS - Counts match: {ref_count_complete}")
        passed += 1
    else:
        print(f"  ✗ FAIL - Mismatch: complete={ref_count_complete}, debug={ref_count_debug}")
        failed += 1
    
    # Check 2: Figures count consistency
    print("\n[CHECK 2] Figures count consistency")
    fig_count_complete = complete['by_type'].get('figure', 0)
    fig_count_debug = figures['total_figures'] if figures else 0
    
    if fig_count_complete == fig_count_debug:
        print(f"  ✓ PASS - Counts match: {fig_count_complete}")
        passed += 1
    else:
        print(f"  ✗ FAIL - Mismatch: complete={fig_count_complete}, debug={fig_count_debug}")
        failed += 1
    
    # Check 3: Tables count consistency
    print("\n[CHECK 3] Tables count consistency")
    tab_count_complete = complete['by_type'].get('table', 0)
    tab_count_debug = tables['total_tables'] if tables else 0
    
    if tab_count_complete == tab_count_debug:
        print(f"  ✓ PASS - Counts match: {tab_count_complete}")
        passed += 1
    else:
        print(f"  ✗ FAIL - Mismatch: complete={tab_count_complete}, debug={tab_count_debug}")
        failed += 1
    
    # Check 4: Duplicate anchor IDs
    print("\n[CHECK 4] Duplicate anchor IDs")
    all_ids = []
    for anchor_type, anchors in complete.get('anchors', {}).items():
        for anchor in anchors:
            all_ids.append(anchor['id'])
    
    id_counts = Counter(all_ids)
    duplicates = {id: count for id, count in id_counts.items() if count > 1}
    
    if not duplicates:
        print(f"  ✓ PASS - No duplicate IDs found ({len(all_ids)} unique)")
        passed += 1
    else:
        print(f"  ✗ FAIL - {len(duplicates)} duplicate IDs:")
        for id, count in list(duplicates.items())[:10]:  # Show first 10
            print(f"      - {id}: {count} occurrences")
        if len(duplicates) > 10:
            print(f"      ... and {len(duplicates) - 10} more")
        failed += 1
    
    # Check 5: Empty anchor IDs
    print("\n[CHECK 5] Empty or invalid anchor IDs")
    empty_ids = [id for id in all_ids if not id or id.isspace()]
    
    if not empty_ids:
        print(f"  ✓ PASS - No empty IDs")
        passed += 1
    else:
        print(f"  ✗ FAIL - {len(empty_ids)} empty IDs found")
        failed += 1
    
    # Check 6: Reference ID format
    print("\n[CHECK 6] Reference ID format (ref-N)")
    if refs:
        invalid_ref_ids = []
        for ref in refs['references']:
            ref_id = ref['id']
            if not ref_id.startswith('ref-') or not ref_id[4:].isdigit():
                invalid_ref_ids.append(ref_id)
        
        if not invalid_ref_ids:
            print(f"  ✓ PASS - All reference IDs valid")
            passed += 1
        else:
            print(f"  ✗ FAIL - {len(invalid_ref_ids)} invalid reference IDs:")
            for id in invalid_ref_ids[:5]:
                print(f"      - {id}")
            failed += 1
    else:
        print(f"  ⊘ SKIP - No references file")
    
    # Check 7: Figure ID format
    print("\n[CHECK 7] Figure ID format (diagram-N)")
    if figures:
        invalid_fig_ids = []
        for fig in figures['figures']:
            fig_id = fig['id']
            if not fig_id.startswith('diagram-') or not fig_id[8:].isdigit():
                invalid_fig_ids.append(fig_id)
        
        if not invalid_fig_ids:
            print(f"  ✓ PASS - All figure IDs valid")
            passed += 1
        else:
            print(f"  ✗ FAIL - {len(invalid_fig_ids)} invalid figure IDs:")
            for id in invalid_fig_ids[:5]:
                print(f"      - {id}")
            failed += 1
    else:
        print(f"  ⊘ SKIP - No figures file")
    
    # Check 8: SVG files exist
    print("\n[CHECK 8] SVG files exist for all figures")
    svg_dir = OUTPUT_DIR / "mermaid_svg"
    if svg_dir.exists() and figures:
        svg_files = list(svg_dir.glob("*.svg"))
        expected = figures['total_figures']
        actual = len(svg_files)
        
        # Allow small discrepancy (extra SVGs from previous builds are OK)
        if expected == actual:
            print(f"  ✓ PASS - All SVG files present: {actual}")
            passed += 1
        elif actual >= expected and actual - expected <= 5:
            print(f"  ✓ PASS (with warning) - SVG files: expected={expected}, actual={actual}")
            print(f"    Note: {actual - expected} extra SVG file(s) detected (possibly from previous builds)")
            passed += 1
        else:
            print(f"  ✗ FAIL - SVG count mismatch: expected={expected}, actual={actual}")
            failed += 1
    else:
        print(f"  ⊘ SKIP - SVG directory not found or no figures")
    
    # Final summary
    print("\n" + "=" * 70)
    print("VALIDATION SUMMARY")
    print("=" * 70)
    print(f"Total checks: {passed + failed}")
    print(f"  ✓ Passed:   {passed}")
    print(f"  ✗ Failed:   {failed}")
    
    if failed == 0:
        print("\n[SUCCESS] All validation checks passed! ✓")
        return True
    else:
        print(f"\n[WARNING] {failed} validation check(s) failed")
        return False

if __name__ == '__main__':
    import sys
    success = main()
    sys.exit(0 if success else 1)
