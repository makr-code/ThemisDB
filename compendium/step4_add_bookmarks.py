"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            step4_add_bookmarks.py                             ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     629                                            ║
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
"""
Step 4: Add PDF Bookmarks/Outline for navigation.
Phase 3A: PDF Enhancement with navigation outline.
"""

import yaml
import tempfile
import re
import json
import argparse
from html import escape
from pathlib import Path
from typing import List, Dict, Any

try:
    from PyPDF2 import PdfReader, PdfWriter
    from PyPDF2.generic import AnnotationBuilder
    PYPDF2_AVAILABLE = True
except ImportError:
    print("[WARNING] PyPDF2 not available. Install with: pip install PyPDF2")
    PYPDF2_AVAILABLE = False

try:
    from weasyprint import HTML
    WEASY_AVAILABLE = True
except Exception:
    print("[WARNING] WeasyPrint not available. TOC page insertion will be skipped.")
    WEASY_AVAILABLE = False

COMPENDIUM_DIR = Path(__file__).parent
DOCS_DIR = COMPENDIUM_DIR / "docs"
OUTPUT_DIR = COMPENDIUM_DIR / "output"
YAML_CONFIG = COMPENDIUM_DIR / "mkdocs-nav.yml"
# Default flags (can be overridden via CLI)
INLINE_TOC_ENABLED = True
FIGURE_INDEX_ENABLED = True

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


def anchor_for_item(item: Dict) -> str:
    """Derive anchor id (matches step2_generate_html anchors)."""
    if item.get('file'):
        return item['file'].replace('.md', '').replace('/', '-').upper()
    return item['title'].replace(' ', '-').upper()


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
                file_path = DOCS_DIR / item['file']
                pages = estimate_page_count(file_path)
                current_page += pages
            else:
                current_page += 3  # Default estimate
    
    return page_map


def extract_marker_pages(pdf_path: Path) -> Dict[str, int]:
    """Scan PDF for invisible markers @@ANCHOR@@ to map anchors to real pages."""
    marker_map = {}
    try:
        reader = PdfReader(pdf_path)
        for idx, page in enumerate(reader.pages, start=1):
            try:
                text = page.extract_text() or ""
            except Exception:
                text = ""
            for token in re.findall(r'@@([A-Z0-9_\-]+)@@', text):
                marker_map[token] = idx
    except Exception as e:
        print(f"[WARNING] Could not extract markers: {e}")
    return marker_map


def load_figures_meta() -> list:
    """Load figure metadata dumped by step2 (if present)."""
    figures_meta_path = OUTPUT_DIR / "figures_meta.json"
    if not figures_meta_path.exists():
        return []
    try:
        return json.loads(figures_meta_path.read_text(encoding="utf-8"))
    except Exception as e:
        print(f"[WARNING] Could not read figures metadata: {e}")
        return []


def build_toc_pdf(flat_nav: List[Dict], page_map: Dict[str, int], insert_after_page: int) -> tuple:
    """Create a simple TOC PDF (Word-style) and return (pdf_path, page_count)."""

    if not WEASY_AVAILABLE:
        return None, 0

    # Build HTML for TOC
    rows = []
    for item in flat_nav:
        page_num = page_map.get(item['title'])
        if not page_num:
            continue
        depth = max(0, item.get('depth', 0))
        indent_px = depth * 16
        title_html = escape(item['title'])
        row_class = "section" if item.get('type') == 'section' else "page-item"
        rows.append(
            f"<li class='row {row_class}'><span class='page'>{page_num}</span><span class='dots'></span><span class='title level-{depth}' style='margin-left:{indent_px}px'>{title_html}</span></li>"
        )

    toc_html = f"""
    <html>
    <head>
    <style>
      body {{ font-family: 'Segoe UI', Arial, sans-serif; font-size: 10pt; margin: 20mm 18mm; color: #1f2f3b; }}
      h1 {{ font-size: 14pt; margin: 0 0 12pt 0; font-weight: 600; }}
      ul {{ list-style: none; padding: 0; margin: 0; }}
      li {{ margin: 2pt 0; }}
      .row {{ display: flex; align-items: flex-end; gap: 6pt; }}
      .page {{ width: 32pt; text-align: left; font-variant-numeric: tabular-nums; }}
      .dots {{ flex: 1; border-bottom: 1px dotted #bbb; height: 10pt; }}
      .title {{ flex: 1; text-align: right; }}
      .row.section .title {{ font-weight: 700; text-transform: uppercase; letter-spacing: 0.05em; }}
    </style>
    </head>
    <body>
      <h1>Inhaltsverzeichnis</h1>
      <ul>
        {''.join(rows)}
      </ul>
    </body>
    </html>
    """

    tmp_dir = tempfile.mkdtemp(prefix="toc_build_")
    toc_pdf_path = Path(tmp_dir) / "toc.pdf"
    HTML(string=toc_html).write_pdf(str(toc_pdf_path))

    toc_reader = PdfReader(str(toc_pdf_path))
    toc_pages = len(toc_reader.pages)
    return toc_pdf_path, toc_pages


def build_figure_index_pdf(figures: list, fig_page_map: Dict[str, int]) -> tuple:
    """Build figure index PDF with page numbers (returns path, page_count)."""
    if not WEASY_AVAILABLE or not figures:
        return None, 0

    rows = []
    for fig in figures:
        num = fig.get('num')
        title = fig.get('title', f'Abbildung {num}')
        page = fig_page_map.get(f"FIG-{num}")
        if not page:
            continue
        rows.append(f"<li class='row'><span class='page'>{page}</span><span class='dots'></span><span class='title'>Abb. {num}: {escape(title)}</span></li>")

    if not rows:
        return None, 0

    fig_html = f"""
    <html>
    <head>
    <style>
      body {{ font-family: 'Segoe UI', Arial, sans-serif; font-size: 10pt; margin: 20mm 18mm; color: #1f2f3b; }}
      h1 {{ font-size: 14pt; margin: 0 0 12pt 0; font-weight: 600; }}
      ul {{ list-style: none; padding: 0; margin: 0; }}
      li {{ margin: 2pt 0; }}
      .row {{ display: flex; align-items: flex-end; gap: 6pt; }}
      .page {{ width: 32pt; text-align: left; font-variant-numeric: tabular-nums; }}
      .dots {{ flex: 1; border-bottom: 1px dotted #bbb; height: 10pt; }}
      .title {{ flex: 1; text-align: right; }}
    </style>
    </head>
    <body>
      <h1>Abbildungen &amp; Diagramme</h1>
      <ul>
        {''.join(rows)}
      </ul>
    </body>
    </html>
    """

    tmp_dir = tempfile.mkdtemp(prefix="figindex_build_")
    fig_pdf_path = Path(tmp_dir) / "figure_index.pdf"
    HTML(string=fig_html).write_pdf(str(fig_pdf_path))
    fig_reader = PdfReader(str(fig_pdf_path))
    fig_pages = len(fig_reader.pages)
    return fig_pdf_path, fig_pages


def add_bookmarks_to_pdf(pdf_path: Path, output_path: Path, flat_nav: List[Dict], inline_toc: bool = True, figure_index: bool = True):
    """Add hierarchical bookmarks and optionally rebuild TOC with real pages."""
    if not PYPDF2_AVAILABLE:
        print("[ERROR] PyPDF2 not available. Skipping bookmark generation.")
        print("[INFO] Install with: pip install PyPDF2")
        return False
    
    print("\n[INFO] Adding PDF bookmarks...")
    
    try:
        # Read PDF
        reader = PdfReader(pdf_path)

        # Build page map (markers preferred)
        print("[INFO] Generating page mapping...")
        marker_map = extract_marker_pages(pdf_path)
        print(f"[INFO] Marker hits: {len(marker_map)}")
        page_map = generate_page_mapping(flat_nav)

        figures_meta = load_figures_meta()

        # Determine insertion point (after Vorwort if present)
        insert_after_title = "Vorwort"
        insert_after_page = page_map.get(insert_after_title, 2)

        toc_pages = 0
        toc_pdf_path = None
        if inline_toc:
            # Prefer marker-based page numbers
            for item in flat_nav:
                anchor = anchor_for_item(item)
                if anchor in marker_map:
                    page_map[item['title']] = marker_map[anchor]

            toc_pdf_path, toc_pages = build_toc_pdf(flat_nav, page_map, insert_after_page)
            if toc_pages > 0:
                print(f"[INFO] Replacing existing TOC with generated TOC ({toc_pages} pages) after '{insert_after_title}' (page {insert_after_page})")
            else:
                print("[INFO] Skipping TOC insertion (WeasyPrint unavailable or build failed)")
        else:
            print("[INFO] Inline TOC insertion disabled; using existing TOC from HTML")

        # Figure index generation setup
        fig_pages = 0
        fig_pdf_path = None
        fig_insert_after_page = page_map.get('__figure_index__', insert_after_page + toc_pages)
        if figure_index and figures_meta:
            # Map figure numbers to pages via markers
            fig_page_map = {}
            for key, val in marker_map.items():
                if key.startswith('FIG-'):
                    fig_page_map[key] = val
            fig_pdf_path, fig_pages = build_figure_index_pdf(figures_meta, fig_page_map)
            if fig_pages > 0:
                print(f"[INFO] Replacing existing figure index with generated version ({fig_pages} pages) around page {fig_insert_after_page}")
            else:
                print("[INFO] Skipping figure index insertion (no figures or build failed)")
        else:
            print("[INFO] Figure index rebuild disabled or no figures detected")

        writer = PdfWriter()

        # Detect existing TOC pages (contains "Inhaltsverzeichnis")
        toc_pages_existing = set()
        if inline_toc:
            for idx, page in enumerate(reader.pages, start=1):
                if idx > 25:  # TOC should be early
                    break
                try:
                    text = page.extract_text() or ""
                    if "Inhaltsverzeichnis" in text:
                        toc_pages_existing.add(idx)
                except Exception:
                    continue

        # Detect existing Figure Index pages
        fig_pages_existing = set()
        if figure_index:
            for idx, page in enumerate(reader.pages, start=1):
                if idx > 40:
                    break
                try:
                    text = page.extract_text() or ""
                    if "Abbildungen" in text and "Diagramme" in text:
                        fig_pages_existing.add(idx)
                except Exception:
                    continue

        insert_done_toc = False
        insert_done_fig = False
        toc_first_page_idx = None  # 0-based index where TOC starts in writer
        fig_first_page_idx = None
        for idx, page in enumerate(reader.pages, start=1):
            # Skip old TOC pages and insert new
            if inline_toc and idx in toc_pages_existing:
                if not insert_done_toc and toc_pages > 0:
                    toc_reader = PdfReader(str(toc_pdf_path))
                    toc_first_page_idx = len(writer.pages)
                    for toc_page in toc_reader.pages:
                        writer.add_page(toc_page)
                    insert_done_toc = True
                continue

            # Skip old figure index pages and insert new
            if figure_index and idx in fig_pages_existing:
                if not insert_done_fig and fig_pages > 0:
                    fig_reader = PdfReader(str(fig_pdf_path))
                    fig_first_page_idx = len(writer.pages)
                    for fig_page in fig_reader.pages:
                        writer.add_page(fig_page)
                    insert_done_fig = True
                continue

            writer.add_page(page)

            # Fallback insertion if placeholders not found
            if not insert_done_toc and toc_pages > 0 and idx == insert_after_page and idx not in toc_pages_existing:
                toc_reader = PdfReader(str(toc_pdf_path))
                toc_first_page_idx = len(writer.pages)
                for toc_page in toc_reader.pages:
                    writer.add_page(toc_page)
                insert_done_toc = True

            if not insert_done_fig and fig_pages > 0 and idx == fig_insert_after_page and idx not in fig_pages_existing:
                fig_reader = PdfReader(str(fig_pdf_path))
                fig_first_page_idx = len(writer.pages)
                for fig_page in fig_reader.pages:
                    writer.add_page(fig_page)
                insert_done_fig = True

        total_pages = len(writer.pages)

        # Adjust page map for bookmark destinations after inserts
        if toc_pages > 0:
            for title, page_no in list(page_map.items()):
                if page_no > insert_after_page:
                    page_map[title] = page_no + toc_pages

        if fig_pages > 0:
            for title, page_no in list(page_map.items()):
                if page_no > fig_insert_after_page:
                    page_map[title] = page_no + fig_pages

        # Add link annotations to TOC entries (page -> target page)
        if toc_pages > 0 and toc_first_page_idx is not None:
            # Rebuild TOC item list in order
            toc_items = []
            for item in flat_nav:
                target = page_map.get(item['title'])
                if target:
                    toc_items.append(target)

            # Layout assumptions must match build_toc_pdf CSS
            margin_left = 51  # ~18mm in points
            margin_right = 51
            margin_top = 56  # ~20mm
            margin_bottom = 56
            heading_offset = 24
            row_height = 14
            # compute rows per page from page size
            if toc_pages > 0:
                page_box = writer.pages[toc_first_page_idx].mediabox
                page_height = float(page_box.top) - float(page_box.bottom)
                available_height = page_height - margin_top - margin_bottom - heading_offset
                rows_per_page = max(1, int(available_height // row_height))
            else:
                rows_per_page = 45

            for i, target in enumerate(toc_items):
                page_from = toc_first_page_idx + (i // rows_per_page)
                row_in_page = i % rows_per_page
                page_box = writer.pages[page_from].mediabox
                page_height = float(page_box.top) - float(page_box.bottom)
                y_top = page_height - margin_top - heading_offset - row_in_page * row_height
                y_bottom = y_top - (row_height - 2)
                x1 = margin_left
                x2 = float(page_box.right) - margin_right
                # Clamp target
                target_page_idx = min(max(0, target - 1), total_pages - 1)
                annotation = AnnotationBuilder.link(
                    rect=(x1, y_bottom, x2, y_top),
                    target_page_index=target_page_idx
                )
                writer.add_annotation(page_from, annotation)

        # Adjust page map for bookmark destinations after TOC insertion
        if toc_pages > 0:
            for title, page_no in list(page_map.items()):
                if page_no > insert_after_page:
                    page_map[title] = page_no + toc_pages

        # Add bookmarks hierarchically
        print("[INFO] Adding bookmarks...")
        parent_bookmark = None
        bookmark_count = 0
        
        for item in flat_nav:
            page_num = page_map.get(item['title'], 1)
            
            # Ensure page number is within bounds
            if page_num > total_pages:
                page_num = total_pages
            
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
        print(f"  - Estimated pages: {total_pages}")
        
        return True
    
    except Exception as e:
        print(f"[ERROR] Failed to add bookmarks: {e}")
        import traceback
        traceback.print_exc()
        return False


def main(inline_toc=True, figure_index=True):
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
    success = add_bookmarks_to_pdf(pdf_path, output_path, flat_nav, inline_toc, figure_index)
    
    if success:
        # Replace original PDF with bookmarked version
        print("\n[INFO] Replacing original PDF with bookmarked version...")
        import shutil
        shutil.move(str(output_path), str(pdf_path))
        print(f"[SUCCESS] PDF updated: {pdf_filename}")
        
        return True
    
    return False


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Add PDF bookmarks and rebuild TOC/Figure Index')
    parser.add_argument('--version', action='version', version=f'step4_add_bookmarks.py {VERSION}')
    parser.add_argument('--no-toc', action='store_true', help='Disable TOC rebuild')
    parser.add_argument('--no-figures', action='store_true', help='Disable figure index rebuild')
    args = parser.parse_args()
    
    # Call main with flags from CLI
    success = main(inline_toc=not args.no_toc, figure_index=not args.no_figures)
    exit(0 if success else 1)
