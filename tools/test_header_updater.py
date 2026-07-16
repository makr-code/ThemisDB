#!/usr/bin/env python3
"""
Header Update Test & Demo

Show how the updater handles different header formats:
- Preserves copyright/license blocks
- Updates existing THEMIS_GAP stats
- Inserts new headers without disruption
"""

from pathlib import Path
from header_format_detector import HeaderFormatDetector
import json

def demo_various_formats():
    """Demonstrate handling of various header formats"""
    
    examples = {
        'copyright_only': '''/*
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "myheader.h"

void myFunction() {
    // Implementation
}
''',
        
        'copyright_with_description': '''/*
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

// @file accelerator.cpp
// GPU kernel acceleration for vector operations

#include "accelerator.h"

void processGPU() {
    throw std::runtime_error("not implemented");
}
''',
        
        'with_existing_gap_stats': '''/*
 * GPU Vector Operations
 */

// THEMIS_GAP_STATS: gaps=5 unimpl=3 stub=2 scanned=2026-05-15

#include "gpu.h"

void compute() {
    // TODO: implement
}
''',
        
        'simple_comments': '''// Fast path implementation
// Copyright notice optional

#include <vector>

void process() {}
''',
    }
    
    print("=" * 70)
    print("Header Format Detection & Update Demo")
    print("=" * 70)
    
    for format_name, content in examples.items():
        print(f"\n📄 Format: {format_name}")
        print("-" * 70)
        
        # Create temp file
        temp_path = Path(f'/tmp/demo_{format_name}.cpp')
        temp_path.write_text(content)
        
        # Analyze
        detector = HeaderFormatDetector(temp_path)
        
        print(f"Detected Type: {detector.get_header_type()}")
        print(f"Header End Line: {detector.get_header_end_line()}")
        print(f"Has THEMIS_GAP: {detector.has_themis_gap_header()}")
        
        # Simulate update
        new_gap_line = "// THEMIS_GAP_STATS: gaps=8 unimpl=5 stub=3 todo=0 scanned=2026-05-18"
        updated = detector.insert_or_update_gap_stats(new_gap_line)
        
        print(f"\nUpdated Content (first 5 lines):")
        for i, line in enumerate(updated.split('\n')[:5], 1):
            print(f"  {i}: {line}")
        
        # Cleanup
        temp_path.unlink()
    
    print("\n" + "=" * 70)

def analyze_repo_headers(repo_root: str):
    """Analyze all headers in a real repo"""
    from header_format_detector import analyze_repo_headers
    
    print("\n" + "=" * 70)
    print("Repository Header Analysis")
    print("=" * 70)
    
    stats = analyze_repo_headers(repo_root)
    
    print(f"\n📊 Header Type Distribution:")
    print(f"   Total Files: {stats['total_files']}")
    print(f"   ✓ Copyright Blocks: {stats['has_copyright']} ({100*stats['has_copyright']//stats['total_files']}%)")
    print(f"   ✓ License Blocks: {stats['has_license']} ({100*stats['has_license']//stats['total_files']}%)")
    print(f"   ✓ File Descriptions: {stats['has_description']} ({100*stats['has_description']//stats['total_files']}%)")
    print(f"   ✓ THEMIS_GAP_STATS: {stats['has_themis_stats']} ({100*stats['has_themis_stats']//stats['total_files']}%)")
    print(f"   ✓ THEMIS_GAP_ANALYSIS: {stats['has_themis_analysis']} ({100*stats['has_themis_analysis']//stats['total_files']}%)")
    print(f"   - Generic Comments: {stats['has_generic_comments']} ({100*stats['has_generic_comments']//stats['total_files']}%)")
    print(f"   - No Header: {stats['no_header']} ({100*stats['no_header']//stats['total_files']}%)")
    
    print(f"\n💡 Implications:")
    print(f"   - {stats['has_copyright'] + stats['has_license']} files have copyright/license")
    print(f"   - {stats['has_themis_stats'] + stats['has_themis_analysis']} files already have THEMIS_GAP headers")
    print(f"   - {stats['total_files'] - stats['has_themis_stats'] - stats['has_themis_analysis']} files need new THEMIS_GAP headers")

def test_smart_update():
    """Test smart header updating"""
    from file_header_updater import FileGapStats, FileHeaderUpdater
    
    print("\n" + "=" * 70)
    print("Smart Header Update Test")
    print("=" * 70)
    
    test_cases = [
        {
            'name': 'Update existing stats',
            'content': '''/*
 * Copyright (c) 2026
 */

// THEMIS_GAP_STATS: gaps=3 unimpl=2 stub=1 scanned=2026-05-15

void func() {}
''',
            'expected_change': True,
        },
        {
            'name': 'Add new stats to copyright',
            'content': '''/*
 * Copyright (c) 2026
 */

void func() {}
''',
            'expected_change': True,
        },
        {
            'name': 'Preserve without changes',
            'content': '''// THEMIS_GAP_STATS: gaps=8 unimpl=5 stub=3 scanned=2026-05-18

void func() {}
''',
            'expected_change': False,  # Stats are already current
        },
    ]
    
    updater = FileHeaderUpdater()
    stats = FileGapStats(total=8, unimplemented=5, stub_documented=2, stub_undocumented=1)
    
    for i, test in enumerate(test_cases, 1):
        print(f"\n  Test {i}: {test['name']}")
        
        temp_path = Path(f'/tmp/test_update_{i}.cpp')
        temp_path.write_text(test['content'])
        
        changed = updater.update_file_header(temp_path, stats, detailed=False)
        
        print(f"     Changed: {changed}")
        print(f"     Expected: {test['expected_change']}")
        
        if changed:
            print(f"     ✓ Header was updated")
        else:
            print(f"     - No changes needed")
        
        temp_path.unlink()
    
    print("\n" + "=" * 70)

if __name__ == '__main__':
    import sys
    
    print("\n🔬 ThemisDB Header Update Test Suite\n")
    
    # Run demos
    demo_various_formats()
    
    # Analyze repo if provided
    if len(sys.argv) > 1:
        repo_root = sys.argv[1]
        analyze_repo_headers(repo_root)
    
    # Run update tests
    test_smart_update()
    
    print("\n✅ All tests complete!")
