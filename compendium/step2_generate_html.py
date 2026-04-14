"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            step2_generate_html.py                             ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     478                                            ║
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
Step 2: Generate HTML from chapters with SCSS theme (YAML-driven structure).
Phase 1: YAML integration with TOC, Figure Index, Sections, and Appendices.
"""

import re
import yaml
import shutil
import argparse
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Any

COMPENDIUM_DIR = Path(__file__).parent
OUTPUT_DIR = COMPENDIUM_DIR / "output"
SVG_OUTPUT_DIR = OUTPUT_DIR / "mermaid_svg"
YAML_CONFIG = COMPENDIUM_DIR / "mkdocs-nav.yml"

# Placeholders for generated blocks controlled via YAML order
GENERATED_PLACEHOLDERS = {
    "__GENERATED_COVER__": "cover",
    "__GENERATED_TOC__": "toc",
    "__GENERATED_FIGURES__": "figures",
    "__GENERATED_KEYWORDS__": "keywords",
}

# ThemisDB Corporate Theme
THEME_CONFIG = {
    "name": "ThemisDB Corporate",
    "primary": "#1a4d2e",
    "secondary": "#0f3d5c",
    "accent": "#2a7f62",
    "text": "#2c3e50",
    "background": "#ffffff",
    "code_bg": "#f0f7f4",
    "body_font": "Georgia, serif",
    "heading_font": "Helvetica Neue, Arial, sans-serif",
    "code_font": "Courier New, monospace"
}

# Read version
VERSION_FILE = COMPENDIUM_DIR / "VERSION"
VERSION = "v1.4.0"
if VERSION_FILE.exists():
    VERSION = VERSION_FILE.read_text(encoding='utf-8').strip()

# Global counters
diagram_counter = 0
chapter_counter = 0

def load_yaml_structure():
    """Load and parse mkdocs-nav.yml structure (clean nav section only)."""
    if not YAML_CONFIG.exists():
        print(f"[WARNING] YAML config not found: {YAML_CONFIG}")
        return None
    
    with open(YAML_CONFIG, 'r', encoding='utf-8') as f:
        config = yaml.safe_load(f)
    
    return config.get('nav', [])

def flatten_nav_items(nav_items: List[Any], parent_title: str = "") -> List[Dict]:
    """Flatten nav structure to list of items with metadata."""
    items = []
    
    for item in nav_items:
        if isinstance(item, dict):
            for title, content in item.items():
                if isinstance(content, list):
                    # Section with children (Teil I, II, etc.)
                    items.append({
                        'type': 'section',
                        'title': title,
                        'parent': parent_title
                    })
                    # Recursively add children
                    items.extend(flatten_nav_items(content, title))
                elif isinstance(content, str):
                    placeholder_type = GENERATED_PLACEHOLDERS.get(content)
                    if placeholder_type:
                        # Generated block placeholder (cover, toc, figures)
                        items.append({
                            'type': 'generated',
                            'generated': placeholder_type,
                            'title': title,
                            'parent': parent_title
                        })
                    else:
                        # Single page
                        items.append({
                            'type': 'page',
                            'title': title,
                            'file': content,
                            'parent': parent_title
                        })
    
    return items

def extract_diagrams_from_content(content: str) -> List[str]:
    """Extract Mermaid diagram titles from content."""
    diagrams = []
    matches = re.findall(r'```mermaid\n(.*?)\n```', content, flags=re.DOTALL)
    
    for diagram_code in matches:
        # Try to extract title from comment
        title_match = re.search(r'%%\s*(.+?)(?:\n|$)', diagram_code)
        if title_match:
            diagrams.append(title_match.group(1).strip())
        else:
            diagrams.append(f"Diagramm {len(diagrams) + 1}")
    
    return diagrams

def load_svg_as_data_uri(svg_path: Path) -> str:
    """Load SVG file and convert to Base64 data URI for PDF embedding."""
    try:
        import base64
        svg_content = svg_path.read_bytes()
        b64_content = base64.b64encode(svg_content).decode('utf-8')
        return f"data:image/svg+xml;base64,{b64_content}"
    except Exception as e:
        print(f"[WARNING] Failed to convert SVG to data URI: {e}")
        return None

def process_markdown_file(file_path: Path, svg_dir: Path) -> tuple:
    """Process markdown file: convert to HTML, reference SVG diagrams.
    Returns (html_content, diagram_list)."""
    global diagram_counter
    
    if not file_path.exists():
        print(f"[WARNING] File not found: {file_path}")
        return "", []
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Extract diagram titles first
    diagram_titles = extract_diagrams_from_content(content)
    
    # Split on Mermaid blocks
    parts = re.split(r'```mermaid\n(.*?)\n```', content, flags=re.DOTALL)
    
    result_html = ""
    svg_index = 0
    # Get SVG files for this chapter (named as {chapter_stem}_N.svg)
    chapter_stem = file_path.stem
    svg_files = sorted([f for f in svg_dir.glob(f"{chapter_stem}_*.svg")])
    diagrams_in_chapter = []
    
    for i, part in enumerate(parts):
        if i % 2 == 0:
            # Regular markdown part
            if part.strip():
                try:
                    from markdown import markdown
                    html = markdown(part, extensions=['fenced_code', 'tables', 'toc'])
                    # Remove leading h1 tags to avoid duplication (we have chapter-title wrapper)
                    html = re.sub(r'^<h1[^>]*>.*?</h1>\s*', '', html, flags=re.DOTALL)
                    result_html += html
                except Exception as e:
                    print(f"[WARNING] Markdown conversion error: {e}")
                    result_html += f"<p>{part}</p>"
        else:
            # Mermaid diagram - replace with SVG
            if svg_index < len(svg_files):
                diagram_counter += 1
                svg_file = svg_files[svg_index]
                
                # Get diagram title
                diagram_title = diagram_titles[svg_index] if svg_index < len(diagram_titles) else f"Diagramm {diagram_counter}"
                
                # Convert SVG to Base64 data URI for reliable PDF embedding
                svg_src = load_svg_as_data_uri(svg_file)
                if not svg_src:
                    # Fallback to relative path if data URI fails
                    svg_src = f"mermaid_svg/{svg_file.name}"
                
                # Create figure with caption
                result_html += f'''
<div class="page-marker">@@FIG-{diagram_counter}@@</div>
<figure id="diagram-{diagram_counter}" style="text-align: center; margin: 20px 0; page-break-inside: avoid;">
    <img src="{svg_src}" alt="{diagram_title}" 
         style="max-width: 70%; height: auto; border: 1px solid {THEME_CONFIG["accent"]}; padding: 10px; border-radius: 8px; margin-left: auto; margin-right: auto; display: block;">
    <figcaption style="margin-top: 10px; font-size: 0.95em; font-style: italic; color: {THEME_CONFIG["secondary"]};">
        Abb. {diagram_counter}: {diagram_title}
    </figcaption>
</figure>
'''
                diagrams_in_chapter.append({
                    'num': diagram_counter,
                    'title': diagram_title,
                    'anchor': f"diagram-{diagram_counter}"
                })
                svg_index += 1
    
    return result_html, diagrams_in_chapter

def generate_toc(nav_items: List[Dict]) -> str:
    """Generate table of contents HTML from navigation structure."""
    html = '<div class="toc-section"><h1 id="toc">Inhaltsverzeichnis</h1><div class="toc-content">'
    
    current_section = None
    
    for item in nav_items:
        if item['type'] == 'section':
            # New section (Teil I, II, etc.)
            if current_section:
                html += '</ul></div>'
            html += f'<div class="toc-section-group"><h2 class="toc-section-title">{item["title"]}</h2><ul class="toc-list">'
            current_section = item['title']
        elif item['type'] == 'page':
            # Page entry
            anchor = item['file'].replace('.md', '').replace('/', '-')
            html += f'<li class="toc-item"><a class="toc-link" href="#{anchor}"><span class="toc-text">{item["title"]}</span><span class="toc-page"></span></a></li>'
    
    if current_section:
        html += '</ul></div>'
    
    html += '</div></div>'
    return html

def generate_figure_index(diagrams: List[Dict]) -> str:
    """Generate figure index HTML from all diagrams."""
    html = '<div class="figure-index"><h1 id="figure-index">Abbildungen & Diagramme</h1>'
    
    if not diagrams:
        html += '<p>Keine Abbildungen gefunden.</p>'
    else:
        html += '<ul class="figure-list">'
        for diagram in diagrams:
            html += f'<li><a class="figure-link" href="#diagram-{diagram["num"]}"><span class="figure-text">Abb. {diagram["num"]}: {diagram["title"]}</span><span class="figure-page"></span></a></li>'
        html += '</ul>'
    
    html += '</div>'
    return html

def convert_internal_links(html_content: str, flat_nav: List[Dict]) -> str:
    """Convert internal markdown links to anchor links."""
    # Build map of files to anchors
    file_to_anchor = {}
    for item in flat_nav:
        if item['type'] == 'page':
            anchor = item['file'].replace('.md', '').replace('/', '-')
            file_to_anchor[item['file']] = anchor
    
    # Replace markdown links [text](file.md) with <a href="#anchor">text</a>
    for file, anchor in file_to_anchor.items():
        # Match [text](file.md) or [text](./file.md)
        pattern = rf'\[([^\]]+)\]\(\.?/?{re.escape(file)}\)'
        replacement = rf'<a href="#{anchor}">\1</a>'
        html_content = re.sub(pattern, replacement, html_content)
    
    return html_content

def main():
    global chapter_counter, diagram_counter
    
    print("=" * 70)
    print("Step 2: Generate HTML (YAML-driven)")
    print("=" * 70)
    
    print(f"\n[INFO] Version: {VERSION}")
    print(f"[INFO] Theme: {THEME_CONFIG['name']}")
    print("[INFO] YAML-driven structure from mkdocs-compendium.yml")
    
    # Load YAML structure
    print("\n[INFO] Loading YAML structure...")
    nav_items = load_yaml_structure()
    if not nav_items:
        print("[ERROR] Could not load YAML structure")
        return False
    
    flat_nav = flatten_nav_items(nav_items)
    print(f"OK - Found {len(flat_nav)} items ({len([x for x in flat_nav if x['type']=='section'])} sections, {len([x for x in flat_nav if x['type']=='page'])} pages)")
    
    # Load cover page (used when placeholder is present)
    print("\n[INFO] Preparing cover page...")
    cover_file = COMPENDIUM_DIR / 'docs' / 'cover_book.md'
    if not cover_file.exists():
        alt_cover = COMPENDIUM_DIR / 'cover.md'
        if alt_cover.exists():
            cover_file = alt_cover

    if cover_file.exists():
        with open(cover_file, 'r', encoding='utf-8') as f:
            cover_md = f.read()
        
        if cover_md.startswith('```'):
            cover_md = cover_md.split('```')[1].lstrip('\nmarkdown\n')
            if cover_md.endswith('```'):
                cover_md = cover_md[:-3]
        
        cover_md = cover_md.replace('{{VERSION}}', f'Version {VERSION}')
        cover_md = cover_md.replace('{{DATE}}', datetime.now().strftime('%d. %B %Y'))
        cover_md = cover_md.replace('{{THEME}}', THEME_CONFIG['name'])
        
        try:
            from markdown import markdown
            cover_html = markdown(cover_md)
        except:
            cover_html = f"<h1>ThemisDB Kompendium {VERSION}</h1>"
    else:
        cover_html = f"<h1>ThemisDB Kompendium {VERSION}</h1>"
    
    print("OK")
    
    # Process all items from YAML structure in order
    print("\n[INFO] Processing items from YAML structure...")
    render_queue = []
    all_diagrams = []
    section_counter = 0
    
    for item in flat_nav:
        if item['type'] == 'section':
            section_counter += 1
            section_html = f'''
<div class="page-marker">@@SECTION-{section_counter}@@</div>
<div class="section-page" style="page-break-before: always; page-break-after: always; min-height: 80vh; display: flex; align-items: center; justify-content: center;">
    <div style="text-align: center;">
        <h1 style="font-size: 36pt; color: {THEME_CONFIG["primary"]}; margin-bottom: 20px;">{item["title"]}</h1>
        <div style="width: 100px; height: 3px; background: {THEME_CONFIG["accent"]}; margin: 20px auto;"></div>
    </div>
</div>
'''
            render_queue.append({'type': 'section', 'html': section_html, 'title': item['title']})
            print(f"  [SECTION] {item['title']}")
            
        elif item['type'] == 'generated':
            render_queue.append({'type': 'generated', 'generated': item['generated'], 'title': item['title']})
            print(f"  [GENERATED] {item['generated'].upper()} placeholder: {item['title']}")
            
        elif item['type'] == 'page':
            file_path = COMPENDIUM_DIR / "docs" / item['file']
            anchor = item['file'].replace('.md', '').replace('/', '-')
            
            if file_path.exists():
                chapter_html, chapter_diagrams = process_markdown_file(file_path, SVG_OUTPUT_DIR)
                
                chapter_number = ""
                if 'chapter_' in item['file'] and not item['file'].startswith('chapter_00'):
                    match = re.search(r'chapter_(\d+)', item['file'])
                    if match:
                        chapter_counter = int(match.group(1))
                        chapter_number = f"Kapitel {chapter_counter}: "
                
                wrapped_html = f'''
<div class="page-marker">@@{anchor.upper()}@@</div>
<div id="{anchor}" class="chapter">
    <h1 class="chapter-title" style="color: {THEME_CONFIG["primary"]}; border-bottom: 2px solid {THEME_CONFIG["primary"]}; padding-bottom: 8px; margin-top: 30px;">
        {chapter_number}{item["title"]}
    </h1>
    {chapter_html}
</div>
'''
                
                render_queue.append({'type': 'page', 'html': wrapped_html, 'title': item['title']})
                all_diagrams.extend(chapter_diagrams)
                print(f"  [PAGE] {item['title']} ({len(chapter_diagrams)} diagrams)")
            else:
                print(f"  [WARNING] File not found: {item['file']}")
    
    print(f"\nOK - Processed {len(render_queue)} items, {len(all_diagrams)} diagrams total")
    
    # Generate TOC and Figure Index (after all pages are processed)
    print("\n[INFO] Generating table of contents...")
    toc_html = generate_toc(flat_nav)
    print("OK")
    
    print("[INFO] Generating figure index...")
    figure_index_html = generate_figure_index(all_diagrams)
    print("OK")
    
    # Assemble HTML following YAML order (placeholders included)
    print("\n[INFO] Assembling final HTML...")
    assembled_parts = []
    for entry in render_queue:
        if entry['type'] == 'generated':
            gen_type = entry.get('generated')
            if gen_type == 'cover':
                assembled_parts.append(f'<div class="cover">{cover_html}</div>')
            elif gen_type == 'toc':
                assembled_parts.append(toc_html)
            elif gen_type in ('figures', 'figure_index'):
                assembled_parts.append(figure_index_html)
            elif gen_type == 'keywords':
                assembled_parts.append('<div class="keyword-index"><h1 id="keyword-index">Stichwortverzeichnis</h1><p>(Stichworte werden hier eingetragen.)</p></div>')
            else:
                assembled_parts.append(f"<!-- Unbekannter Platzhalter: {gen_type} -->")
        else:
            assembled_parts.append(entry['html'])
    
    # Footer appended at the end
    footer_html = f'''<div class="footer">
        ThemisDB Kompendium {VERSION} | {datetime.now().strftime('%d.%m.%Y %H:%M:%S')}
    </div>'''
    assembled_parts.append(footer_html)
    
    html_content = f"""<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThemisDB Kompendium {VERSION}</title>
    <link rel="stylesheet" href="styles_modern_book.css">
</head>
<body>
    <div class="content">
        {''.join(assembled_parts)}
    </div>
</body>
</html>
"""

    # Persist figure metadata for downstream processing
    figures_meta_path = OUTPUT_DIR / "figures_meta.json"
    try:
        import json
        figures_meta_path.write_text(json.dumps(all_diagrams, ensure_ascii=False, indent=2), encoding="utf-8")
    except Exception as e:
        print(f"[WARNING] Could not write figures metadata: {e}")
    
    # Copy external stylesheet to output directory
    print("[INFO] Preparing external stylesheet...")
    css_source_path = COMPENDIUM_DIR / "styles_modern_book.scss"
    css_output_path = OUTPUT_DIR / "styles_modern_book.css"
    
    if css_source_path.exists():
        css_output_path.write_text(css_source_path.read_text(encoding="utf-8"), encoding="utf-8")
        print("OK")
    else:
        print(f"[WARNING] Stylesheet not found: {css_source_path}")
    
    # Save HTML
    html_filename = f"ThemisDB-Kompendium-{VERSION}.html"
    html_path = OUTPUT_DIR / html_filename
    with open(html_path, 'w', encoding='utf-8') as f:
        f.write(html_content)
    
    print("OK")
    print(f"\n[SUCCESS] HTML file: {html_filename}")
    print(f"  - Sections: {len([x for x in flat_nav if x['type']=='section'])}")
    print(f"  - Pages: {len([x for x in flat_nav if x['type']=='page'])}")
    print(f"  - Diagrams: {len(all_diagrams)}")
    
    return str(html_path)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate HTML from Markdown chapters')
    parser.add_argument('--version', action='version', version=f'step2_generate_html.py {VERSION}')
    parser.parse_args()
    
    success = main()
    if not success:
        exit(1)
