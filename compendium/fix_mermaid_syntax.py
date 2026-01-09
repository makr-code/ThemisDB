#!/usr/bin/env python3
"""
Fix Mermaid syntax errors in all chapter MD files.
Replaces problematic Unicode characters and syntax issues.
"""

import re
from pathlib import Path

def fix_mermaid_syntax(filepath):
    """Fix Mermaid syntax errors in a Markdown file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original_size = len(content)
        
        # Fix within mermaid blocks only - use negative lookahead/lookbehind
        # Split by mermaid blocks
        parts = re.split(r'(```mermaid\n.*?\n```)', content, flags=re.DOTALL)
        
        fixed_parts = []
        for i, part in enumerate(parts):
            if part.startswith('```mermaid'):
                # This is a mermaid block - fix it
                mermaid_content = part
                
                # Replace problematic characters
                replacements = {
                    '✓': '[OK]',
                    '✅': '[OK]',
                    '❌': '[ERROR]',
                    '→': '->',
                    '←': '<-',
                    '–': '-',
                    '\u2018': "'",  # Left single quote
                    '\u2019': "'",  # Right single quote
                    '\u201c': '"',  # Left double quote
                    '\u201d': '"',  # Right double quote
                }
                
                for old, new in replacements.items():
                    mermaid_content = mermaid_content.replace(old, new)
                
                # Remove % from percentages (replace with "percent")
                mermaid_content = re.sub(r'(\d+)%', r'\1 percent', mermaid_content)
                
                # Fix Sovränität -> Souveraenitaet for better encoding
                mermaid_content = mermaid_content.replace('Sovränität', 'Souveraenitaet')
                mermaid_content = mermaid_content.replace('Sensitivität', 'Sensitivitaet')
                
                fixed_parts.append(mermaid_content)
            else:
                # Not a mermaid block - keep as is
                fixed_parts.append(part)
        
        new_content = ''.join(fixed_parts)
        
        if new_content != content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"✓ Fixed: {filepath.name}")
            return True
        else:
            print(f"  No changes: {filepath.name}")
            return False
            
    except Exception as e:
        print(f"✗ Error processing {filepath.name}: {e}")
        return False

def main():
    compendium_dir = Path(__file__).parent
    chapter_files = sorted(compendium_dir.glob("chapter_*.md"))
    
    print("="*70)
    print("  Fixing Mermaid Syntax Errors")
    print("="*70)
    print()
    
    fixed_count = 0
    for filepath in chapter_files:
        if fix_mermaid_syntax(filepath):
            fixed_count += 1
    
    print()
    print("="*70)
    print(f"✅ Processed {len(chapter_files)} files, fixed {fixed_count}")
    print("="*70)

if __name__ == "__main__":
    main()
