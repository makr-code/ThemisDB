#!/usr/bin/env python3
"""
Step 4: Add PDF Bookmarks/Outline for navigation.
Phase 3A: PDF Enhancement with navigation outline.
"""

import yaml
from pathlib import Path
from typing import List, Dict, Any

try:
    from PyPDF2 import PdfReader, PdfWriter
    PYPDF2_AVAILABLE = True
except ImportError:
    print("[WARNING] PyPDF2 not available. Install with: pip install PyPDF2")
    PYPDF2_AVAILABLE = False

COMPENDIUM_DIR = Path(__file__).parent
OUTPUT_DIR = COMPENDIUM_DIR / "output"
YAML_CONFIG = COMPENDIUM_DIR / "mkdocs-nav.yml"

# Read version
VERSION_FILE = COMPENDIUM_DIR / "VERSION"
VERSION = "v1.4.0"
if VERSION_FILE.exists():
    VERSION = VERSION_FILE.read_text(encoding='utf-8').strip()


def load_yaml_structure():
    """Load and parse mkdocs-nav.yml structure."""
    if not YAML_CONFIG.exists():
        print(f"[ERROR] YAML config not found: {YAML_CONFIG}")
        return None
    
    with open(YAML_CONFIG, 'r', encoding='utf-8') as f:
        config = yaml.safe_load(f)
    
    return config.get('nav', [])


def flatten_nav_items(nav_items: List[Any], depth=0) -> List[Dict]:
    """
    Flatten navigation hierarchy into list of items with metadata.
    
    Returns:
        List of dicts with keys: type, title, file, depth
    """
    result = []
    
    for item in nav_items:
        if isinstance(item, dict):
            for key, value in item.items():
                if isinstance(value, list):
                    # Section (e.g., "Teil I - Grundlagen": [...])
                    result.append({
                        'type': 'section',
                        'title': key,
                        'file': None,
                        'depth': depth
                    })
                    # Recursively process children
                    result.extend(flatten_nav_items(value, depth + 1))
                elif isinstance(value, str):
                    # Page (e.g., "Kapitel 1": "chapter_01.md")
                    result.append({
                        'type': 'page',
                        'title': key,
                        'file': value,
                        'depth': depth
                    })
        elif isinstance(item, str):
            # Direct file reference
            result.append({
                'type': 'page',
                'title': Path(item).stem.replace('_', ' ').title(),
                'file': item,
                'depth': depth
            })
    
    return result


def estimate_page_count(md_file: Path) -> int:
    """
    Estimate page count from markdown file.
    Approximation: ~3000 characters per page in PDF.
    """
    if not md_file.exists():
        return 1
    
    try:
        with open(md_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        char_count = len(content)
        # Rough estimate: 3000 chars per page
        # Add extra for diagrams (assume 0.5 pages per diagram)
        diagram_count = content.count('```mermaid')
        
        text_pages = max(1, char_count // 3000)
        diagram_pages = diagram_count * 0.5
        
        return int(text_pages + diagram_pages)
    except Exception as e:
        print(f"[WARNING] Could not estimate page count for {md_file}: {e}")
        return 5  # Default fallback


def generate_page_mapping(flat_nav: List[Dict]) -> Dict[str, int]:
    """
    Generate approximate page mapping for each item.
    
    Returns:
        Dict mapping item titles to page numbers
    """
    page_map = {}
    current_page = 1  # Start at page 1 (cover)
    
    # Fixed pages at start
    current_page += 1  # Cover page
    page_map['__cover__'] = 1
    
    current_page += 1  # TOC page
    page_map['__toc__'] = current_page
    
    current_page += 2  # Figure Index (usually 2 pages for 101 diagrams)
    page_map['__figure_index__'] = current_page - 1
    
    # Process navigation items
    for item in flat_nav:
        if item['type'] == 'section':
            # Section pages
            page_map[item['title']] = current_page
            current_page += 1  # Section page takes 1 page
        
        elif item['type'] == 'page':
            # Chapter pages
            page_map[item['title']] = current_page
            
            # Estimate pages for this chapter
            if item['file']:
                file_path = COMPENDIUM_DIR / item['file']
                pages = estimate_page_count(file_path)
                current_page += pages
            else:
                current_page += 3  # Default estimate
    
    return page_map


def add_bookmarks_to_pdf(pdf_path: Path, output_path: Path, flat_nav: List[Dict]):
    """
    Add hierarchical bookmarks to PDF using PyPDF2.
    
    Args:
        pdf_path: Input PDF file
        output_path: Output PDF with bookmarks
        flat_nav: Navigation structure from YAML
    """
    if not PYPDF2_AVAILABLE:
        print("[ERROR] PyPDF2 not available. Skipping bookmark generation.")
        print("[INFO] Install with: pip install PyPDF2")
        return False
    
    print("\n[INFO] Adding PDF bookmarks...")
    
    try:
        # Read PDF
        reader = PdfReader(pdf_path)
        writer = PdfWriter()
        
        # Copy all pages
        print(f"[INFO] Copying {len(reader.pages)} pages...")
        for page in reader.pages:
            writer.add_page(page)
        
        # Generate page mapping
        print("[INFO] Generating page mapping...")
        page_map = generate_page_mapping(flat_nav)
        
        # Add bookmarks hierarchically
        print("[INFO] Adding bookmarks...")
        parent_bookmark = None
        bookmark_count = 0
        
        for item in flat_nav:
            page_num = page_map.get(item['title'], 1)
            
            # Ensure page number is within bounds
            if page_num > len(reader.pages):
                page_num = len(reader.pages)
            
            if item['type'] == 'section':
                # Top-level bookmark (Part I, II, etc.)
                parent_bookmark = writer.add_outline_item(
                    item['title'],
                    page_num - 1,  # PyPDF2 uses 0-indexed pages
                    parent=None
                )
                bookmark_count += 1
                print(f"  [SECTION] {item['title']} → Page {page_num}")
            
            elif item['type'] == 'page':
                # Child bookmark (Chapters under Parts)
                writer.add_outline_item(
                    item['title'],
                    page_num - 1,  # PyPDF2 uses 0-indexed pages
                    parent=parent_bookmark
                )
                bookmark_count += 1
        
        # Write output PDF
        print(f"[INFO] Writing PDF with {bookmark_count} bookmarks...")
        with open(output_path, 'wb') as f:
            writer.write(f)
        
        print(f"[SUCCESS] PDF with bookmarks: {output_path}")
        print(f"  - Total bookmarks: {bookmark_count}")
        print(f"  - Estimated pages: {len(reader.pages)}")
        
        return True
    
    except Exception as e:
        print(f"[ERROR] Failed to add bookmarks: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    print("=" * 70)
    print("Step 4: Add PDF Bookmarks (Phase 3A)")
    print("=" * 70)
    
    if not PYPDF2_AVAILABLE:
        print("\n[ERROR] PyPDF2 is required for bookmark generation.")
        print("[INFO] Install with: pip install PyPDF2")
        print("[INFO] Skipping bookmark generation.")
        return False
    
    # Load YAML structure
    print("\n[INFO] Loading YAML structure...")
    nav_items = load_yaml_structure()
    if not nav_items:
        print("[ERROR] Could not load YAML structure")
        return False
    
    flat_nav = flatten_nav_items(nav_items)
    print(f"OK - Found {len(flat_nav)} items")
    
    # Find PDF file
    pdf_filename = f"ThemisDB-Kompendium-{VERSION}.pdf"
    pdf_path = OUTPUT_DIR / pdf_filename
    
    if not pdf_path.exists():
        print(f"[ERROR] PDF not found: {pdf_path}")
        print("[INFO] Run step3_generate_pdf.py first")
        return False
    
    print(f"[INFO] Input PDF: {pdf_filename}")
    print(f"[INFO] Size: {pdf_path.stat().st_size / (1024*1024):.2f} MB")
    
    # Generate output filename
    output_filename = f"ThemisDB-Kompendium-{VERSION}-bookmarked.pdf"
    output_path = OUTPUT_DIR / output_filename
    
    # Add bookmarks
    success = add_bookmarks_to_pdf(pdf_path, output_path, flat_nav)
    
    if success:
        # Replace original PDF with bookmarked version
        print("\n[INFO] Replacing original PDF with bookmarked version...")
        import shutil
        shutil.move(str(output_path), str(pdf_path))
        print(f"[SUCCESS] PDF updated: {pdf_filename}")
        
        return True
    
    return False


if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)
