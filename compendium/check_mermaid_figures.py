#!/usr/bin/env python3
"""
Check if all Mermaid diagrams are properly wrapped in <figure> tags with captions.
"""

import re
from pathlib import Path

def analyze_mermaid_diagrams(filepath):
    """Analyze a file for Mermaid diagrams and their figure wrapping."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Find all Mermaid blocks
    mermaid_pattern = r'```mermaid\n(.*?)\n```'
    mermaid_blocks = list(re.finditer(mermaid_pattern, content, re.DOTALL))
    
    if not mermaid_blocks:
        return None
    
    results = {
        'file': filepath.name,
        'total': len(mermaid_blocks),
        'with_figure': 0,
        'with_caption': 0,
        'missing_figure': [],
        'missing_caption': []
    }
    
    for i, match in enumerate(mermaid_blocks, 1):
        start_pos = match.start()
        end_pos = match.end()
        
        # Check for <figure> before the block (within 200 chars)
        context_before = content[max(0, start_pos-200):start_pos]
        context_after = content[end_pos:min(len(content), end_pos+200)]
        
        has_figure = '<figure>' in context_before
        has_figcaption = '<figcaption>' in context_after or 'Abb.' in context_after
        
        if has_figure:
            results['with_figure'] += 1
        else:
            # Get line number
            line_num = content[:start_pos].count('\n') + 1
            results['missing_figure'].append(line_num)
        
        if has_figcaption:
            results['with_caption'] += 1
        else:
            line_num = content[:start_pos].count('\n') + 1
            results['missing_caption'].append(line_num)
    
    return results

def main():
    compendium_dir = Path(__file__).parent
    chapter_files = sorted(compendium_dir.glob("chapter_*.md"))
    
    print("="*80)
    print("  Mermaid Diagram Figure/Caption Analysis")
    print("="*80)
    print()
    
    total_diagrams = 0
    total_with_figure = 0
    total_with_caption = 0
    files_with_issues = []
    
    for filepath in chapter_files:
        result = analyze_mermaid_diagrams(filepath)
        if result is None:
            continue
        
        total_diagrams += result['total']
        total_with_figure += result['with_figure']
        total_with_caption += result['with_caption']
        
        if result['missing_figure'] or result['missing_caption']:
            files_with_issues.append(result)
            print(f"\n{result['file']}")
            print(f"  Total diagrams: {result['total']}")
            print(f"  With <figure>: {result['with_figure']}/{result['total']}")
            print(f"  With caption: {result['with_caption']}/{result['total']}")
            
            if result['missing_figure']:
                print(f"  Missing <figure> at lines: {result['missing_figure']}")
            if result['missing_caption']:
                print(f"  Missing caption at lines: {result['missing_caption']}")
    
    print("\n" + "="*80)
    print("  Summary")
    print("="*80)
    print(f"Total Mermaid diagrams: {total_diagrams}")
    print(f"With <figure> tags: {total_with_figure} ({100*total_with_figure/total_diagrams:.1f}%)")
    print(f"With captions: {total_with_caption} ({100*total_with_caption/total_diagrams:.1f}%)")
    print(f"Files with issues: {len(files_with_issues)}")
    print()

if __name__ == "__main__":
    main()
