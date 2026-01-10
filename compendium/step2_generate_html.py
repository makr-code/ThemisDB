#!/usr/bin/env python3
"""
Step 2: Generate HTML from chapters with SCSS theme (YAML-driven structure).
Phase 1: YAML integration with TOC, Figure Index, Sections, and Appendices.
"""

import re
import yaml
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Any

COMPENDIUM_DIR = Path(__file__).parent
OUTPUT_DIR = COMPENDIUM_DIR / "output"
SVG_OUTPUT_DIR = OUTPUT_DIR / "mermaid_svg"
YAML_CONFIG = COMPENDIUM_DIR / "mkdocs-nav.yml"

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
    svg_files = sorted(svg_dir.glob("diagram_*.svg"))
    diagrams_in_chapter = []
    
    for i, part in enumerate(parts):
        if i % 2 == 0:
            # Regular markdown part
            if part.strip():
                try:
                    from markdown import markdown
                    html = markdown(part, extensions=['fenced_code', 'tables', 'toc'])
                    result_html += html
                except Exception as e:
                    print(f"[WARNING] Markdown conversion error: {e}")
                    result_html += f"<p>{part}</p>"
        else:
            # Mermaid diagram - replace with SVG
            if svg_index < len(svg_files):
                diagram_counter += 1
                svg_file = svg_files[svg_index]
                svg_abs_path = svg_file.resolve()
                
                # Get diagram title
                diagram_title = diagram_titles[svg_index] if svg_index < len(diagram_titles) else f"Diagramm {diagram_counter}"
                
                # Create figure with caption
                result_html += f'''
<figure id="diagram-{diagram_counter}" style="text-align: center; margin: 20px 0; page-break-inside: avoid;">
    <img src="file://{svg_abs_path}" alt="{diagram_title}" 
         style="max-width: 100%; height: auto; border: 1px solid {THEME_CONFIG["accent"]}; padding: 10px; border-radius: 8px;">
    <figcaption style="margin-top: 10px; font-style: italic; color: {THEME_CONFIG["secondary"]};">
        Abb. {diagram_counter}: {diagram_title}
    </figcaption>
</figure>
'''
                diagrams_in_chapter.append({
                    'num': diagram_counter,
                    'title': diagram_title
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
            html += f'<li class="toc-item"><a href="#{anchor}">{item["title"]}</a></li>'
    
    if current_section:
        html += '</ul></div>'
    
    html += '</div></div>'
    return html

def generate_figure_index(diagrams: List[Dict]) -> str:
    """Generate figure index HTML from all diagrams."""
    html = '<div class="figure-index"><h1 id="figure-index">Abbildungsverzeichnis</h1>'
    
    if not diagrams:
        html += '<p>Keine Abbildungen gefunden.</p>'
    else:
        html += '<ul class="figure-list">'
        for diagram in diagrams:
            html += f'<li><a href="#diagram-{diagram["num"]}">Abb. {diagram["num"]}: {diagram["title"]}</a></li>'
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
    
    # Load cover page
    print("\n[INFO] Processing cover page...")
    cover_file = COMPENDIUM_DIR / 'cover.md'
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
    
    # Process all pages from YAML structure
    print("\n[INFO] Processing pages from YAML structure...")
    all_content = []
    all_diagrams = []
    section_counter = 0
    
    for item in flat_nav:
        if item['type'] == 'section':
            # Add section page
            section_counter += 1
            section_html = f'''
<div class="section-page" style="page-break-before: always; page-break-after: always; min-height: 80vh; display: flex; align-items: center; justify-content: center;">
    <div style="text-align: center;">
        <h1 style="font-size: 36pt; color: {THEME_CONFIG["primary"]}; margin-bottom: 20px;">{item["title"]}</h1>
        <div style="width: 100px; height: 3px; background: {THEME_CONFIG["accent"]}; margin: 20px auto;"></div>
    </div>
</div>
'''
            all_content.append(section_html)
            print(f"  [SECTION] {item['title']}")
            
        elif item['type'] == 'page':
            # Process page file
            file_path = COMPENDIUM_DIR / item['file']
            anchor = item['file'].replace('.md', '').replace('/', '-')
            
            if file_path.exists():
                chapter_html, chapter_diagrams = process_markdown_file(file_path, SVG_OUTPUT_DIR)
                
                # Determine chapter numbering
                chapter_number = ""
                if 'chapter_' in item['file'] and not item['file'].startswith('chapter_00'):
                    # Extract chapter number from filename
                    match = re.search(r'chapter_(\d+)', item['file'])
                    if match:
                        chapter_counter = int(match.group(1))
                        chapter_number = f"Kapitel {chapter_counter}: "
                
                wrapped_html = f'''
<div id="{anchor}" class="chapter">
    <h1 class="chapter-title" style="color: {THEME_CONFIG["primary"]}; border-bottom: 2px solid {THEME_CONFIG["primary"]}; padding-bottom: 8px; margin-top: 30px;">
        {chapter_number}{item["title"]}
    </h1>
    {chapter_html}
</div>
'''
                all_content.append(wrapped_html)
                all_diagrams.extend(chapter_diagrams)
                print(f"  [PAGE] {item['title']} ({len(chapter_diagrams)} diagrams)")
            else:
                print(f"  [WARNING] File not found: {item['file']}")
    
    print(f"\nOK - Processed {len(all_content)} items, {len(all_diagrams)} diagrams total")
    
    # Generate TOC and Figure Index
    print("\n[INFO] Generating table of contents...")
    toc_html = generate_toc(flat_nav)
    print("OK")
    
    print("[INFO] Generating figure index...")
    figure_index_html = generate_figure_index(all_diagrams)
    print("OK")
    
    # Generate HTML
    print("\n[INFO] Assembling final HTML...")
    
    html_content = f"""<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThemisDB Kompendium {VERSION}</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        html, body {{
            font-family: {THEME_CONFIG['body_font']};
            line-height: 1.6;
            color: {THEME_CONFIG['text']};
            background-color: {THEME_CONFIG['background']};
        }}
        
        /* Cover Page */
        .cover {{
            width: 100%;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            background-color: white;
            color: {THEME_CONFIG['primary']};
            text-align: center;
            page-break-after: always;
            padding: 60px 40px;
        }}
        
        .cover h1 {{
            font-size: 48pt;
            font-weight: bold;
            margin-bottom: 20px;
            font-family: {THEME_CONFIG['heading_font']};
            color: {THEME_CONFIG['primary']};
        }}
        
        .cover h2 {{
            font-size: 18pt;
            color: {THEME_CONFIG['accent']};
            margin-bottom: 40px;
            font-weight: normal;
        }}
        
        .cover p {{
            font-size: 13pt;
            color: {THEME_CONFIG['text']};
            margin: 15px 0;
            max-width: 600px;
        }}
        
        /* TOC Styles */
        .toc-section, .figure-index {{
            page-break-before: always;
            page-break-after: always;
            padding: 40px 50px;
        }}
        
        .toc-section h1, .figure-index h1 {{
            color: {THEME_CONFIG['primary']};
            border-bottom: 3px solid {THEME_CONFIG['primary']};
            padding-bottom: 10px;
            margin-bottom: 30px;
            font-size: 24pt;
        }}
        
        .toc-section-group {{
            margin-bottom: 25px;
        }}
        
        .toc-section-title {{
            color: {THEME_CONFIG['accent']};
            font-size: 16pt;
            margin-bottom: 10px;
            font-weight: bold;
        }}
        
        .toc-list {{
            list-style: none;
            padding-left: 20px;
        }}
        
        .toc-item {{
            margin-bottom: 8px;
            line-height: 1.4;
        }}
        
        .toc-item a, .figure-list a {{
            color: {THEME_CONFIG['text']};
            text-decoration: none;
            border-bottom: 1px dotted {THEME_CONFIG['accent']};
        }}
        
        .toc-item a:hover, .figure-list a:hover {{
            color: {THEME_CONFIG['primary']};
            border-bottom: 1px solid {THEME_CONFIG['primary']};
        }}
        
        .figure-list {{
            list-style: none;
            padding-left: 0;
        }}
        
        .figure-list li {{
            margin-bottom: 8px;
            padding-left: 30px;
            text-indent: -30px;
        }}
        
        /* Section Pages */
        .section-page {{
            page-break-before: always;
            page-break-after: always;
        }}
        
        /* Chapter Styles */
        .chapter {{
            margin-bottom: 40px;
        }}
        
        .chapter-title {{
            page-break-after: avoid;
        }}
        
        /* Content */
        .content {{
            padding: 40px 50px;
            max-width: 950px;
            margin: 0 auto;
        }}
        
        h1 {{
            color: {THEME_CONFIG['primary']};
            border-bottom: 2px solid {THEME_CONFIG['primary']};
            padding-bottom: 8px;
            font-size: 24pt;
            margin-top: 30px;
            margin-bottom: 15px;
            page-break-after: avoid;
        }}
        
        h2 {{
            color: {THEME_CONFIG['accent']};
            font-size: 18pt;
            margin-top: 20px;
            margin-bottom: 10px;
            border-bottom: 1px solid {THEME_CONFIG['accent']};
            padding-bottom: 5px;
            page-break-after: avoid;
        }}
        
        h3 {{
            color: {THEME_CONFIG['secondary']};
            font-size: 14pt;
            margin-top: 15px;
            margin-bottom: 8px;
            page-break-after: avoid;
        }}
        
        p {{
            margin-bottom: 10px;
            text-align: justify;
        }}
        
        ul, ol {{
            margin-left: 25px;
            margin-bottom: 10px;
        }}
        
        li {{
            margin-bottom: 5px;
        }}
        
        code {{
            background-color: {THEME_CONFIG['code_bg']};
            padding: 2px 4px;
            border-radius: 3px;
            font-family: {THEME_CONFIG['code_font']};
            color: {THEME_CONFIG['primary']};
            font-size: 0.9em;
        }}
        
        pre {{
            background-color: {THEME_CONFIG['code_bg']};
            padding: 12px;
            border-left: 3px solid {THEME_CONFIG['primary']};
            border-radius: 3px;
            overflow-x: auto;
            margin: 10px 0;
            font-family: {THEME_CONFIG['code_font']};
            font-size: 0.85em;
        }}
        
        blockquote {{
            border-left: 4px solid {THEME_CONFIG['primary']};
            padding-left: 15px;
            margin-left: 0;
            margin: 15px 0;
            color: #666;
            font-style: italic;
        }}
        
        table {{
            width: 100%;
            border-collapse: collapse;
            margin: 15px 0;
            font-size: 0.95em;
        }}
        
        table th {{
            background-color: {THEME_CONFIG['primary']};
            color: white;
            padding: 8px;
            text-align: left;
            font-weight: bold;
        }}
        
        table td {{
            border: 1px solid #ddd;
            padding: 8px;
        }}
        
        table tr:nth-child(even) {{
            background-color: {THEME_CONFIG['code_bg']};
        }}
        
        figure {{
            text-align: center;
            margin: 20px 0;
            page-break-inside: avoid;
        }}
        
        img {{
            max-width: 100%;
            height: auto;
        }}
        
        .footer {{
            text-align: center;
            font-size: 9pt;
            color: #999;
            margin-top: 30px;
            padding-top: 15px;
            border-top: 1px solid #ddd;
            page-break-before: avoid;
        }}
        
        @page {{
            size: A4;
            margin: 20mm 20mm;
        }}
        
        @page :first {{
            margin: 0;
        }}
    </style>
</head>
<body>
    <!-- Cover Page -->
    <div class="cover">
        {cover_html}
    </div>
    
    <!-- Table of Contents -->
    {toc_html}
    
    <!-- Figure Index -->
    {figure_index_html}
    
    <!-- Content -->
    <div class="content">
        {''.join(all_content)}
        
        <div class="footer">
            ThemisDB Kompendium {VERSION} | {datetime.now().strftime('%d.%m.%Y %H:%M:%S')}
        </div>
    </div>
</body>
</html>
"""
    
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
    success = main()
    if not success:
        exit(1)
