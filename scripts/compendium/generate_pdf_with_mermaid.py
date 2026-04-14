"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_pdf_with_mermaid.py                       ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:59:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     540                                            ║
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
Enhanced PDF Generation Script for ThemisDB Compendium with Mermaid Rendering
Converts Mermaid diagrams to images before PDF generation
"""

import os
import sys
import re
import subprocess
import hashlib
from pathlib import Path
from datetime import datetime
import markdown
from markdown.extensions import Extension
from markdown.preprocessors import Preprocessor

# Configuration
SCRIPT_DIR = Path(__file__).parent.absolute()
OUTPUT_DIR = SCRIPT_DIR.parent.parent / "pdf_output"
TEMP_DIR = Path("/tmp/themis_pdf_mermaid")
MERMAID_CACHE = TEMP_DIR / "mermaid_cache"

# Chapters to include
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

def print_section(title):
    """Print formatted section header"""
    print("\n" + "=" * 70)
    print(f"  {title}")
    print("=" * 70 + "\n")

def convert_mermaid_to_svg(mermaid_code, output_file):
    """Convert Mermaid code to SVG using mermaid-cli"""
    # Create temp mermaid file
    mmd_file = TEMP_DIR / f"temp_{hashlib.md5(mermaid_code.encode()).hexdigest()}.mmd"
    
    with open(mmd_file, 'w') as f:
        f.write(mermaid_code)
    
    try:
        # Try using mmdc (mermaid-cli) if available
        result = subprocess.run(
            ["mmdc", "-i", str(mmd_file), "-o", str(output_file), "-b", "transparent"],
            capture_output=True,
            timeout=10
        )
        
        if result.returncode == 0 and output_file.exists():
            return True
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    
    # Fallback: Keep as code block with note
    return False

def process_mermaid_in_markdown(content, chapter_name):
    """Process Mermaid code blocks and convert to nicely formatted boxes"""
    mermaid_pattern = r'```mermaid\n(.*?)```'
    mermaid_count = 0
    
    def replace_mermaid(match):
        nonlocal mermaid_count
        mermaid_count += 1
        mermaid_code = match.group(1).strip()
        
        # Detect diagram type from code
        diagram_type = "Diagram"
        if mermaid_code.startswith("graph"):
            diagram_type = "Flowchart"
        elif mermaid_code.startswith("sequenceDiagram"):
            diagram_type = "Sequence Diagram"
        elif mermaid_code.startswith("stateDiagram"):
            diagram_type = "State Diagram"
        elif mermaid_code.startswith("gantt"):
            diagram_type = "Gantt Chart"
        elif mermaid_code.startswith("erDiagram"):
            diagram_type = "ER Diagram"
        elif mermaid_code.startswith("flowchart"):
            diagram_type = "Flowchart"
        elif mermaid_code.startswith("quadrantChart"):
            diagram_type = "Quadrant Chart"
        
        # Create unique ID for this diagram
        diagram_id = f"{chapter_name}_{mermaid_count}"
        svg_file = MERMAID_CACHE / f"{diagram_id}.svg"
        
        # Try to convert to SVG
        if convert_mermaid_to_svg(mermaid_code, svg_file):
            # Return image reference
            return f'\n\n<div class="mermaid-diagram">\n<p class="diagram-title">📊 {diagram_type} {mermaid_count}</p>\n<img src="{svg_file}" alt="Mermaid Diagram {mermaid_count}" style="max-width: 100%; height: auto;" />\n</div>\n\n'
        else:
            # Format as nicely styled code block with better visual presentation
            # Clean up the code for better readability
            lines = mermaid_code.split('\n')
            formatted_lines = []
            for line in lines:
                stripped = line.strip()
                if stripped:
                    formatted_lines.append(f"    {stripped}")
            
            formatted_code = '\n'.join(formatted_lines)
            
            return f'''

<div class="mermaid-box">
<div class="mermaid-header">
<span class="mermaid-icon">📊</span>
<span class="mermaid-title">{diagram_type} #{mermaid_count}</span>
</div>
<div class="mermaid-content">
<pre><code>{mermaid_code}</code></pre>
</div>
<div class="mermaid-footer">
<em>Hinweis: Dieses Diagramm kann in der Online-Version interaktiv angezeigt werden.</em>
</div>
</div>

'''
    
    processed = re.sub(mermaid_pattern, replace_mermaid, content, flags=re.DOTALL)
    
    if mermaid_count > 0:
        print(f"    ✓ Processed {mermaid_count} Mermaid diagram(s)")
    
    return processed

def generate_pdf_weasyprint():
    """Generate PDF using WeasyPrint with Mermaid preprocessing"""
    print_section("ThemisDB Compendium PDF Generator (with Mermaid)")
    
    # Create directories
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    TEMP_DIR.mkdir(parents=True, exist_ok=True)
    MERMAID_CACHE.mkdir(parents=True, exist_ok=True)
    
    # Check dependencies
    print("📦 Checking dependencies...")
    try:
        import markdown
        print("  ✓ markdown library found")
    except ImportError:
        print("  ✗ markdown library not found. Installing...")
        subprocess.run([sys.executable, "-m", "pip", "install", "markdown"], check=True)
        import markdown
    
    try:
        from weasyprint import HTML, CSS
        print("  ✓ weasyprint library found")
    except ImportError:
        print("  ✗ weasyprint library not found. Installing...")
        subprocess.run([sys.executable, "-m", "pip", "install", "weasyprint"], check=True)
        from weasyprint import HTML, CSS
    
    # Check for mermaid-cli
    try:
        subprocess.run(["mmdc", "--version"], capture_output=True, check=True)
        print("  ✓ mermaid-cli (mmdc) found")
        mermaid_available = True
    except (FileNotFoundError, subprocess.CalledProcessError):
        print("  ⚠ mermaid-cli not found - diagrams will be shown as code")
        print("    Install with: npm install -g @mermaid-js/mermaid-cli")
        mermaid_available = False
    
    # Collect and process chapters
    print(f"\n📚 Collecting {len(CHAPTERS)} chapters...")
    combined_html = []
    
    # Add title page
    combined_html.append("""
    <div class="title-page">
        <h1>ThemisDB Compendium</h1>
        <h2>Das vollständige technische Handbuch</h2>
        <p><strong>Version 1.3.4</strong></p>
        <p>ThemisDB Development Team</p>
        <p>{}</p>
    </div>
    <div class="page-break"></div>
    """.format(datetime.now().strftime("%Y-%m-%d")))
    
    # Process each chapter
    for chapter_file in CHAPTERS:
        chapter_path = SCRIPT_DIR / chapter_file
        
        if not chapter_path.exists():
            print(f"  ⚠ Skipping (not found): {chapter_file}")
            continue
        
        print(f"  ✓ Processing: {chapter_file}")
        
        with open(chapter_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Process Mermaid diagrams
        content = process_mermaid_in_markdown(content, chapter_path.stem)
        
        # Convert markdown to HTML
        html = markdown.markdown(
            content,
            extensions=[
                'markdown.extensions.tables',
                'markdown.extensions.fenced_code',
                'markdown.extensions.codehilite',
                'markdown.extensions.toc',
            ]
        )
        
        combined_html.append(f'<div class="chapter">\n{html}\n</div>')
        combined_html.append('<div class="page-break"></div>')
    
    # Create full HTML document
    full_html = f"""
    <!DOCTYPE html>
    <html lang="de">
    <head>
        <meta charset="UTF-8">
        <title>ThemisDB Compendium</title>
    </head>
    <body>
    {''.join(combined_html)}
    </body>
    </html>
    """
    
    # Save HTML for debugging
    html_file = TEMP_DIR / "compendium.html"
    with open(html_file, 'w', encoding='utf-8') as f:
        f.write(full_html)
    
    # Generate PDF
    output_file = OUTPUT_DIR / f"ThemisDB-Compendium-v1.3.4-{datetime.now().strftime('%Y%m%d')}-mermaid.pdf"
    
    print(f"\n🔨 Generating PDF with WeasyPrint...")
    print(f"  Output: {output_file}")
    
    # Enhanced CSS for better rendering
    css = CSS(string='''
        @page {
            size: A4;
            margin: 25mm 20mm;
            
            @top-center {
                content: "ThemisDB Compendium v1.3.4";
                font-size: 9pt;
                color: #666;
            }
            
            @bottom-center {
                content: counter(page);
                font-size: 9pt;
            }
        }
        
        body {
            font-family: Georgia, "Times New Roman", serif;
            font-size: 11pt;
            line-height: 1.65;
            color: #2c3e50;
        }
        
        .title-page {
            text-align: center;
            padding-top: 100px;
        }
        
        .title-page h1 {
            font-size: 36pt;
            color: #7c4dff;
            margin-bottom: 20px;
        }
        
        .title-page h2 {
            font-size: 20pt;
            color: #555;
            font-weight: normal;
        }
        
        .page-break {
            page-break-after: always;
        }
        
        h1 {
            font-size: 28pt;
            color: #7c4dff;
            font-family: "Helvetica Neue", Arial, sans-serif;
            border-bottom: 3px solid #7c4dff;
            padding-bottom: 10px;
            page-break-before: always;
            margin-top: 0;
        }
        
        h2 {
            font-size: 20pt;
            color: #555;
            font-family: "Helvetica Neue", Arial, sans-serif;
            border-bottom: 1px solid #ddd;
            padding-bottom: 5px;
            margin-top: 30px;
        }
        
        h3 {
            font-size: 16pt;
            color: #7c4dff;
            font-family: "Helvetica Neue", Arial, sans-serif;
            margin-top: 20px;
        }
        
        code {
            background-color: #f5f5f5;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: "Courier New", monospace;
            font-size: 10pt;
            color: #c7254e;
        }
        
        pre {
            background-color: #f8f8f8;
            border-left: 4px solid #7c4dff;
            padding: 12px;
            margin: 15px 0;
            overflow-x: auto;
            page-break-inside: avoid;
        }
        
        pre code {
            background: none;
            padding: 0;
            color: #333;
            font-size: 9.5pt;
        }
        
        table {
            border-collapse: collapse;
            width: 100%;
            margin: 15px 0;
            page-break-inside: avoid;
        }
        
        th {
            background-color: #7c4dff;
            color: white;
            padding: 10px;
            text-align: left;
            font-weight: bold;
        }
        
        td {
            padding: 8px 10px;
            border-bottom: 1px solid #ddd;
        }
        
        tr:nth-child(even) {
            background-color: #f9f9f9;
        }
        
        .mermaid-box {
            margin: 25px 0;
            border: 2px solid #7c4dff;
            border-radius: 8px;
            background: linear-gradient(135deg, #f5f3ff 0%, #ffffff 100%);
            box-shadow: 0 4px 6px rgba(124, 77, 255, 0.1);
            page-break-inside: avoid;
            overflow: hidden;
        }
        
        .mermaid-header {
            background: linear-gradient(135deg, #7c4dff 0%, #9c4dff 100%);
            color: white;
            padding: 12px 20px;
            font-weight: bold;
            font-size: 12pt;
            border-bottom: 2px solid #6c3ddf;
        }
        
        .mermaid-icon {
            font-size: 14pt;
            margin-right: 8px;
        }
        
        .mermaid-title {
            font-family: "Helvetica Neue", Arial, sans-serif;
        }
        
        .mermaid-content {
            padding: 20px;
            background-color: #fafafa;
        }
        
        .mermaid-content pre {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
            border-left: 4px solid #7c4dff;
            padding: 15px;
            margin: 0;
            border-radius: 4px;
            font-size: 9pt;
            line-height: 1.4;
        }
        
        .mermaid-content code {
            background: none;
            color: #333;
            font-family: "Courier New", "Consolas", monospace;
        }
        
        .mermaid-footer {
            padding: 10px 20px;
            background-color: #f0f0f0;
            border-top: 1px solid #e0e0e0;
            font-size: 9pt;
            color: #666;
            font-style: italic;
        }
        
        .mermaid-diagram {
            margin: 20px 0;
            padding: 15px;
            background-color: #fafafa;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            text-align: center;
            page-break-inside: avoid;
        }
        
        .mermaid-diagram .diagram-title {
            font-weight: bold;
            color: #7c4dff;
            margin-bottom: 10px;
            font-size: 11pt;
        }
        
        .mermaid-diagram img {
            max-width: 100%;
            height: auto;
        }
        
        .mermaid-code {
            margin: 20px 0;
            padding: 15px;
            background-color: #fff9e6;
            border-left: 4px solid #ffb74d;
            page-break-inside: avoid;
        }
        
        .mermaid-code p {
            margin-top: 0;
            color: #ff6f00;
            font-weight: bold;
        }
        
        blockquote {
            border-left: 4px solid #7c4dff;
            padding-left: 15px;
            margin: 15px 0;
            color: #666;
            font-style: italic;
        }
        
        img {
            max-width: 100%;
            height: auto;
            page-break-inside: avoid;
        }
        
        a {
            color: #7c4dff;
            text-decoration: none;
        }
    ''')
    
    try:
        HTML(string=full_html).write_pdf(output_file, stylesheets=[css])
        
        if output_file.exists():
            size_mb = output_file.stat().st_size / (1024 * 1024)
            print(f"\n✅ PDF generated successfully!")
            print(f"   📄 File: {output_file.name}")
            print(f"   📊 Size: {size_mb:.2f} MB")
            print(f"   📚 Chapters: {len(CHAPTERS)}")
            print(f"   📅 Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
            
            if not mermaid_available:
                print(f"\n   ℹ️  Note: Mermaid diagrams are shown as code blocks")
                print(f"   To render as images, install: npm install -g @mermaid-js/mermaid-cli")
            
            return 0
        else:
            print("\n❌ PDF generation failed!")
            return 1
            
    except Exception as e:
        print(f"\n❌ PDF generation failed: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == "__main__":
    sys.exit(generate_pdf_weasyprint())
