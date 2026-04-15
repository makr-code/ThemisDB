"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            verify-readme-links.py                             ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:11:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     129                                            ║
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
README Link Verification Script
Verifies all links in README.md files to ensure they point to existing files.
"""

import os
import re
import sys
from pathlib import Path

def extract_links(content):
    """Extract all markdown links from content."""
    link_pattern = r'\[([^\]]+)\]\(([^\)]+)\)'
    return re.findall(link_pattern, content)

def verify_readme_links(readme_path='README.md', base_dir='.'):
    """Verify all links in a README file."""
    if not os.path.exists(readme_path):
        print(f"❌ README file not found: {readme_path}")
        return False
    
    # Read README content
    with open(readme_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Extract links
    links = extract_links(content)
    
    # Statistics
    total_links = len(links)
    local_links = []
    external_links = []
    anchor_links = []
    broken_links = []
    
    # Categorize and verify links
    for text, url in links:
        if url.startswith('http://') or url.startswith('https://'):
            external_links.append((text, url))
        elif url.startswith('#'):
            anchor_links.append((text, url))
        else:
            local_links.append((text, url))
            
            # Check if local file exists
            clean_url = url.split('#')[0] if '#' in url else url
            if clean_url:
                full_path = os.path.join(base_dir, clean_url)
                if not os.path.exists(full_path):
                    broken_links.append((text, url, clean_url))
    
    # Print results
    print(f"\n{'='*70}")
    print(f"README Link Verification: {readme_path}")
    print(f"{'='*70}\n")
    
    print(f"📊 Statistics:")
    print(f"  Total links: {total_links}")
    print(f"  Local file links: {len(local_links)}")
    print(f"  External links: {len(external_links)}")
    print(f"  Internal anchors: {len(anchor_links)}")
    print()
    
    if broken_links:
        print(f"❌ Found {len(broken_links)} broken links:\n")
        for text, url, clean_url in broken_links:
            print(f"  Text: '{text}'")
            print(f"  Link: {url}")
            print(f"  Path: {clean_url}")
            print(f"  Status: FILE NOT FOUND")
            print()
        return False
    else:
        print(f"✅ All {len(local_links)} local file links are valid!")
        print()
        
        # Show sample of verified links
        if local_links:
            print("Sample of verified links:")
            for text, url in local_links[:5]:
                print(f"  ✓ {url}")
            if len(local_links) > 5:
                print(f"  ... and {len(local_links) - 5} more")
        
        return True

def main():
    """Main entry point."""
    # Get README path from command line or use default
    readme_path = sys.argv[1] if len(sys.argv) > 1 else 'README.md'
    
    # Find repository root
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent
    
    # Change to repo root
    os.chdir(repo_root)
    
    # Verify links
    success = verify_readme_links(readme_path)
    
    # Exit with appropriate code
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
