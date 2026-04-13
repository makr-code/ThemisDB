"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            debug_anchors.py                                   ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     108                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""Debug script to validate anchor consistency in generated HTML."""

import re
from pathlib import Path
import sys

# Configuration
COMPENDIUM_DIR = Path(__file__).parent
OUTPUT_FILE = COMPENDIUM_DIR / "output" / "ThemisDB-Kompendium-v1.4.0.html"

def extract_anchors_from_html(html_content):
    """Extract all div IDs (chapter anchors) from HTML."""
    pattern = r'<div\s+id="([^"]+)"\s+class="chapter"'
    return re.findall(pattern, html_content)

def extract_toc_links(html_content):
    """Extract all TOC links to verify they point to valid anchors."""
    pattern = r'<a\s+href="#([^"]+)">[^<]+</a>'
    return re.findall(pattern, html_content)

def main():
    print("=" * 70)
    print("DEBUG: Anchor Consistency Validator")
    print("=" * 70)
    
    if not OUTPUT_FILE.exists():
        print(f"[ERROR] Output file not found: {OUTPUT_FILE}")
        return False
    
    print(f"\n[INFO] Reading HTML from: {OUTPUT_FILE}")
    with open(OUTPUT_FILE, 'r', encoding='utf-8') as f:
        html_content = f.read()
    
    # Extract all chapter divs with IDs
    chapter_ids = extract_anchors_from_html(html_content)
    print(f"\n[CHAPTERS] Found {len(chapter_ids)} chapter divs with IDs:")
    for i, anchor in enumerate(chapter_ids[:5], 1):
        print(f"  {i}. {anchor}")
    if len(chapter_ids) > 5:
        print(f"  ... and {len(chapter_ids) - 5} more")
    
    # Extract all TOC links
    toc_links = extract_toc_links(html_content)
    print(f"\n[TOC LINKS] Found {len(toc_links)} links in TOC:")
    
    # Check which TOC links have matching chapter divs
    missing_anchors = set()
    matched_anchors = set()
    
    for link in toc_links:
        if link == 'toc':  # Skip the TOC link to itself
            continue
        if link in chapter_ids:
            matched_anchors.add(link)
        else:
            missing_anchors.add(link)
    
    print(f"\n[RESULTS]")
    print(f"  TOC Links with matching divs: {len(matched_anchors)}")
    print(f"  TOC Links with MISSING divs: {len(missing_anchors)}")
    
    if missing_anchors:
        print(f"\n[ERROR] Missing anchor mappings:")
        for link in sorted(missing_anchors)[:10]:
            print(f"  - {link}")
        if len(missing_anchors) > 10:
            print(f"  ... and {len(missing_anchors) - 10} more")
    
    # Check for chapter divs without TOC links
    unlinked_chapters = set(chapter_ids) - matched_anchors
    if unlinked_chapters:
        print(f"\n[WARNING] Chapter divs not referenced in TOC:")
        for anchor in sorted(unlinked_chapters)[:10]:
            print(f"  - {anchor}")
        if len(unlinked_chapters) > 10:
            print(f"  ... and {len(unlinked_chapters) - 10} more")
    
    success = len(missing_anchors) == 0
    print(f"\n[STATUS] {'✓ OK' if success else '✗ FAILED'} - Anchor mapping is {'consistent' if success else 'inconsistent'}")
    
    return success

if __name__ == '__main__':
    sys.exit(0 if main() else 1)
