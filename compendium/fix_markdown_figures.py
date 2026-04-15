"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            fix_markdown_figures.py                            ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     103                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Fix Markdown files: Remove <figure> HTML wrappers around mermaid blocks.
The correct format is just the mermaid block followed by a caption line.
"""

import re
from pathlib import Path

DOCS_DIR = Path(__file__).parent / "docs"

def fix_markdown_file(file_path: Path) -> int:
    """Remove <figure> wrappers from markdown file.
    Returns number of replacements made."""
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    
    # Pattern: <figure> followed by optional whitespace/newlines, then ```mermaid
    # Match everything until closing </figure>
    pattern = r'<figure>\s*\n\s*```mermaid(.*?)\n```\s*\n\s*<figcaption>(.*?)</figcaption>\s*\n\s*</figure>'
    
    def replace_func(match):
        mermaid_content = match.group(1)
        figcaption_content = match.group(2)
        
        # Clean up figcaption - remove HTML tags, keep just the text
        # Example: "<b>Abb. 0.0:</b> Some text" -> "Abb. 0.0: Some text"
        caption_text = re.sub(r'</?b>', '', figcaption_content)
        
        # Return the correct format: mermaid block followed by caption
        return f'```mermaid{mermaid_content}\n```\n\n{caption_text}'
    
    # Apply replacement
    content = re.sub(pattern, replace_func, content, flags=re.DOTALL)
    
    # Count replacements
    replacements = len(re.findall(pattern, original_content, flags=re.DOTALL))
    
    # Write back if changed
    if content != original_content:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)
    
    return replacements

def main():
    print("=" * 70)
    print("Fix Markdown Figures - Remove <figure> HTML wrappers")
    print("=" * 70)
    
    markdown_files = sorted(DOCS_DIR.glob("*.md"))
    
    total_replacements = 0
    fixed_files = []
    
    for file_path in markdown_files:
        replacements = fix_markdown_file(file_path)
        
        if replacements > 0:
            fixed_files.append((file_path.name, replacements))
            total_replacements += replacements
            print(f"  ✓ {file_path.name}: {replacements} figure(s) fixed")
    
    print("\n" + "=" * 70)
    print(f"Summary: Fixed {total_replacements} figure(s) in {len(fixed_files)} file(s)")
    print("=" * 70)
    
    if fixed_files:
        print("\nFixed files:")
        for name, count in fixed_files:
            print(f"  - {name}: {count}")
    
    return len(fixed_files) > 0

if __name__ == '__main__':
    success = main()
    import sys
    sys.exit(0 if success else 1)
