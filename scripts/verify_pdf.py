"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            verify_pdf.py                                      ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:23:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     89                                             ║
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
Verify PDF structure and functionality.
Checks for pages, bookmarks, and internal links.
"""

import sys
import os

def verify_pdf(pdf_path):
    """Verify a PDF file has proper structure and navigation."""
    try:
        from pypdf import PdfReader
    except ImportError:
        print("⚠️  pypdf not available for verification")
        print("   Install with: pip install pypdf")
        return False
    
    if not os.path.exists(pdf_path):
        print(f"❌ Error: PDF file not found: {pdf_path}")
        return False
    
    try:
        reader = PdfReader(pdf_path)
        
        # Check pages
        page_count = len(reader.pages)
        print(f"✓ PDF has {page_count} pages")
        
        # Check bookmarks (counts only top-level, nested structure may contain more)
        if reader.outline:
            bookmark_count = len(reader.outline) if isinstance(reader.outline, list) else 0
            print(f"✓ PDF has {bookmark_count}+ top-level bookmarks")
        else:
            print("⚠️  Warning: No bookmarks found")
        
        # Check for internal links
        links_found = False
        for i, page in enumerate(reader.pages[:5]):
            if '/Annots' in page and page['/Annots']:
                links_found = True
                break
        
        if links_found:
            print("✓ PDF has internal links")
        else:
            print("⚠️  Warning: No internal links found in first 5 pages")
        
        # File size
        size_mb = os.path.getsize(pdf_path) / (1024 * 1024)
        print(f"✓ File size: {size_mb:.2f} MB")
        
        print("\n✅ PDF verification complete")
        return True
        
    except Exception as e:
        print(f"❌ Verification error: {e}")
        return False

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: verify_pdf.py <pdf_file>")
        sys.exit(1)
    
    pdf_path = sys.argv[1]
    success = verify_pdf(pdf_path)
    sys.exit(0 if success else 1)
