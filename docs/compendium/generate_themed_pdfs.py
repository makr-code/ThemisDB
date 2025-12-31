#!/usr/bin/env python3
"""
Quick PDF Generator - Creates PDFs for different visual themes
Uses WeasyPrint directly with theme-specific CSS styling
"""

import os
import sys
import re
from pathlib import Path
from datetime import datetime
import markdown

# Configuration
SCRIPT_DIR = Path(__file__).parent.absolute()
OUTPUT_DIR = SCRIPT_DIR.parent.parent / "pdf_output" / "themes"
CHAPTERS = [
    "preface.md",
    "chapter_00_genesis.md",
    "chapter_01_introduction.md",
    "chapter_02_architecture.md",
    "chapter_03_multimodel.md",
    "chapter_05_relational.md",
    "chapter_06_graph.md",
    "chapter_08_storage_layer.md",
    "chapter_10_enterprise.md",
    "chapter_11_realtime.md",
    "chapter_15_analytics.md",
    "chapter_16_sharding.md",
    "chapter_17_llm_integration.md",
    "chapter_19_monitoring_observability.md",
    "chapter_21_performance.md",
    "chapter_24_ai_ethics.md",
    "appendix_literatur.md",
    "appendix_d_feature_status.md"
]

# Theme definitions with CSS
THEMES = {
    "material": {
        "name": "Material Design",
        "primary": "#7c4dff",
        "secondary": "#ff4081",
        "text": "#2c3e50",
        "background": "#ffffff",
        "code_bg": "#f5f5f5"
    },
    "readthedocs": {
        "name": "Read the Docs",
        "primary": "#2980b9",
        "secondary": "#3498db",
        "text": "#404040",
        "background": "#fcfcfc",
        "code_bg": "#f3f3f3"
    },
    "github": {
        "name": "GitHub",
        "primary": "#0969da",
        "secondary": "#1f883d",
        "text": "#24292f",
        "background": "#ffffff",
        "code_bg": "#f6f8fa"
    },
    "dark": {
        "name": "Dark Mode",
        "primary": "#bb86fc",
        "secondary": "#03dac6",
        "text": "#e1e1e1",
        "background": "#121212",
        "code_bg": "#1e1e1e"
    }
}

def get_theme_css(theme_key):
    """Generate CSS for a specific theme"""
    theme = THEMES[theme_key]
    
    return f'''
        @page {{
            size: A4;
            margin: 25mm 20mm;
            
            @top-center {{
                content: "ThemisDB Compendium v1.3.4 - {theme['name']} Theme";
                font-size: 9pt;
                color: {theme['text']};
            }}
            
            @bottom-center {{
                content: "Seite " counter(page);
                font-size: 9pt;
                color: {theme['text']};
            }}
        }}
        
        body {{
            font-family: Georgia, "Times New Roman", serif;
            font-size: 11pt;
            line-height: 1.65;
            color: {theme['text']};
            background-color: {theme['background']};
        }}
        
        .title-page {{
            text-align: center;
            padding-top: 100px;
            background-color: {theme['background']};
        }}
        
        .title-page h1 {{
            font-size: 36pt;
            color: {theme['primary']};
            margin-bottom: 20px;
        }}
        
        .title-page h2 {{
            font-size: 20pt;
            color: {theme['secondary']};
            font-weight: normal;
            margin-bottom: 30px;
        }}
        
        .title-page .theme-badge {{
            display: inline-block;
            margin-top: 40px;
            padding: 10px 20px;
            background: linear-gradient(135deg, {theme['primary']}, {theme['secondary']});
            color: white;
            border-radius: 20px;
            font-size: 14pt;
            font-weight: bold;
        }}
        
        .page-break {{
            page-break-after: always;
        }}
        
        h1 {{
            font-size: 28pt;
            color: {theme['primary']};
            font-family: "Helvetica Neue", Arial, sans-serif;
            border-bottom: 3px solid {theme['primary']};
            padding-bottom: 10px;
            page-break-before: always;
            margin-top: 0;
        }}
        
        h2 {{
            font-size: 20pt;
            color: {theme['secondary']};
            font-family: "Helvetica Neue", Arial, sans-serif;
            border-bottom: 1px solid {theme['secondary']};
            padding-bottom: 5px;
            margin-top: 30px;
        }}
        
        h3 {{
            font-size: 16pt;
            color: {theme['primary']};
            font-family: "Helvetica Neue", Arial, sans-serif;
            margin-top: 20px;
        }}
        
        code {{
            background-color: {theme['code_bg']};
            padding: 2px 6px;
            border-radius: 3px;
            font-family: "Courier New", monospace;
            font-size: 10pt;
            color: {theme['primary']};
        }}
        
        pre {{
            background-color: {theme['code_bg']};
            border-left: 4px solid {theme['primary']};
            padding: 12px;
            margin: 15px 0;
            overflow-x: auto;
            page-break-inside: avoid;
        }}
        
        pre code {{
            background: none;
            padding: 0;
            color: {theme['text']};
            font-size: 9.5pt;
        }}
        
        table {{
            border-collapse: collapse;
            width: 100%;
            margin: 15px 0;
            page-break-inside: avoid;
        }}
        
        th {{
            background: linear-gradient(135deg, {theme['primary']}, {theme['secondary']});
            color: white;
            padding: 10px;
            text-align: left;
            font-weight: bold;
        }}
        
        td {{
            padding: 8px 10px;
            border-bottom: 1px solid #ddd;
        }}
        
        tr:nth-child(even) {{
            background-color: {theme['code_bg']};
        }}
        
        .mermaid-box {{
            margin: 25px 0;
            border: 2px solid {theme['primary']};
            border-radius: 8px;
            background: linear-gradient(135deg, {theme['code_bg']}, {theme['background']});
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
            page-break-inside: avoid;
            overflow: hidden;
        }}
        
        .mermaid-header {{
            background: linear-gradient(135deg, {theme['primary']}, {theme['secondary']});
            color: white;
            padding: 12px 20px;
            font-weight: bold;
            font-size: 12pt;
        }}
        
        .mermaid-content {{
            padding: 20px;
        }}
        
        .mermaid-content pre {{
            background-color: {theme['background']};
            border: 1px solid #e0e0e0;
            border-left: 4px solid {theme['primary']};
            margin: 0;
        }}
        
        .mermaid-footer {{
            padding: 10px 20px;
            background-color: {theme['code_bg']};
            border-top: 1px solid #e0e0e0;
            font-size: 9pt;
            font-style: italic;
        }}
        
        blockquote {{
            border-left: 4px solid {theme['primary']};
            padding-left: 15px;
            margin: 15px 0;
            color: {theme['secondary']};
            font-style: italic;
        }}
        
        a {{
            color: {theme['primary']};
            text-decoration: none;
        }}
    '''

def process_mermaid(content):
    """Style Mermaid code blocks"""
    mermaid_pattern = r'```mermaid\n(.*?)```'
    count = [0]
    
    def replace_mermaid(match):
        count[0] += 1
        code = match.group(1).strip()
        
        # Detect type
        diagram_type = "Diagram"
        if code.startswith("graph"): diagram_type = "Flowchart"
        elif code.startswith("sequenceDiagram"): diagram_type = "Sequence Diagram"
        elif code.startswith("stateDiagram"): diagram_type = "State Diagram"
        elif code.startswith("gantt"): diagram_type = "Gantt Chart"
        elif code.startswith("erDiagram"): diagram_type = "ER Diagram"
        
        return f'''

<div class="mermaid-box">
<div class="mermaid-header">
📊 {diagram_type} #{count[0]}
</div>
<div class="mermaid-content">
<pre><code>{code}</code></pre>
</div>
<div class="mermaid-footer">
<em>Hinweis: Online-Version zeigt interaktives Diagramm.</em>
</div>
</div>

'''
    
    return re.sub(mermaid_pattern, replace_mermaid, content, flags=re.DOTALL)

def generate_themed_pdf(theme_key):
    """Generate PDF for a specific theme"""
    theme = THEMES[theme_key]
    print(f"\n🎨 Generating {theme['name']} PDF...")
    
    # Collect chapters
    combined_html = []
    
    # Title page
    combined_html.append(f"""
    <div class="title-page">
        <h1>ThemisDB Compendium</h1>
        <h2>Das vollständige technische Handbuch</h2>
        <p class="version"><strong>Version 1.3.4</strong></p>
        <div class="theme-badge">{theme['name']} Theme</div>
        <p class="date">{datetime.now().strftime("%d. %B %Y")}</p>
    </div>
    <div class="page-break"></div>
    """)
    
    # Process chapters
    for chapter_file in CHAPTERS:
        chapter_path = SCRIPT_DIR / chapter_file
        if not chapter_path.exists():
            continue
            
        with open(chapter_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Process Mermaid
        content = process_mermaid(content)
        
        # Convert to HTML
        html = markdown.markdown(
            content,
            extensions=['markdown.extensions.tables', 'markdown.extensions.fenced_code',
                       'markdown.extensions.codehilite', 'markdown.extensions.toc']
        )
        
        combined_html.append(f'<div class="chapter">\n{html}\n</div>')
        combined_html.append('<div class="page-break"></div>')
    
    # Create HTML document
    full_html = f"""
    <!DOCTYPE html>
    <html lang="de">
    <head>
        <meta charset="UTF-8">
        <title>ThemisDB Compendium - {theme['name']}</title>
    </head>
    <body>
    {''.join(combined_html)}
    </body>
    </html>
    """
    
    # Generate PDF
    output_file = OUTPUT_DIR / f"ThemisDB-Kompendium-{theme_key}-{datetime.now().strftime('%Y%m%d')}.pdf"
    
    try:
        from weasyprint import HTML, CSS
        HTML(string=full_html).write_pdf(output_file, stylesheets=[CSS(string=get_theme_css(theme_key))])
        
        if output_file.exists():
            size_mb = output_file.stat().st_size / (1024 * 1024)
            print(f"  ✓ Generated: {output_file.name} ({size_mb:.2f} MB)")
            return output_file
    except Exception as e:
        print(f"  ✗ Failed: {e}")
        return None

def main():
    print("=" * 70)
    print("  ThemisDB Compendium - Multi-Theme PDF Generator")
    print("=" * 70)
    
    # Setup
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    
    # Install dependencies
    try:
        import markdown
        from weasyprint import HTML, CSS
    except ImportError:
        print("\n📦 Installing dependencies...")
        import subprocess
        subprocess.run([sys.executable, "-m", "pip", "install", "-q", "markdown", "weasyprint"], check=True)
        import markdown
        from weasyprint import HTML, CSS
    
    # Generate PDFs for each theme
    results = []
    for theme_key in THEMES.keys():
        pdf_file = generate_themed_pdf(theme_key)
        results.append((THEMES[theme_key]['name'], pdf_file))
    
    # Summary
    print("\n" + "=" * 70)
    print("  Summary")
    print("=" * 70)
    successful = sum(1 for _, pdf in results if pdf)
    print(f"\n✨ Generated {successful}/{len(THEMES)} themed PDFs")
    print(f"📁 Output: {OUTPUT_DIR}\n")
    
    for name, pdf in results:
        status = "✓" if pdf else "✗"
        print(f"  {status} {name}: {pdf.name if pdf else 'Failed'}")
    
    return 0 if successful > 0 else 1

if __name__ == "__main__":
    sys.exit(main())
