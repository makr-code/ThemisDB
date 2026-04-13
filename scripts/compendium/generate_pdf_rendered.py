"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_pdf_rendered.py                           ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:22:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     554                                            ║
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
PDF Generator with TRUE Mermaid Rendering (PNG images)
Converts Mermaid code blocks to PNG images using mermaid-cli (mmdc)
"""

import os
import sys
import re
import subprocess
import hashlib
import tempfile
import shutil
from pathlib import Path
from datetime import datetime
import markdown

# Configuration
SCRIPT_DIR = Path(__file__).parent.absolute()
OUTPUT_DIR = SCRIPT_DIR.parent.parent / "pdf_output"
TEMP_DIR = Path(tempfile.mkdtemp(prefix="themis_pdf_"))
MERMAID_IMAGES = TEMP_DIR / "mermaid_images"

# Chapters to include
CHAPTERS = [
    "preface.md",
    "chapter_00_genesis.md",
    "chapter_01_introduction.md",
    "chapter_02_architecture.md",
    "chapter_03_multimodel.md",
    "chapter_04_installation.md",
    "chapter_05_relational.md",
    "chapter_06_graph.md",
    "chapter_07_document.md",
    "chapter_08_vector.md",
    "chapter_08_storage_layer.md",
    "chapter_09_timeseries.md",
    "chapter_10_enterprise.md",
    "chapter_11_realtime.md",
    "chapter_12_computervision.md",
    "chapter_13_fulltext.md",
    "chapter_14_geospatial.md",
    "chapter_15_analytics.md",
    "chapter_16_sharding.md",
    "chapter_17_llm_integration.md",
    "chapter_18_ml.md",
    "chapter_19_monitoring_observability.md",
    "chapter_20_backup.md",
    "chapter_21_performance.md",
    "chapter_22_clients.md",
    "chapter_24_ai_ethics.md",
    "appendix_literatur.md",
    "appendix_d_feature_status.md"
]

def print_section(title):
    """Print formatted section header"""
    print("\n" + "=" * 70)
    print(f"  {title}")
    print("=" * 70 + "\n")

def render_mermaid_to_png(mermaid_code, output_png):
    """Render Mermaid code to PNG using mermaid-cli via npx"""
    # Create temporary .mmd file
    mmd_file = TEMP_DIR / f"temp_{hashlib.md5(mermaid_code.encode()).hexdigest()}.mmd"
    
    try:
        # Write mermaid code to file
        with open(mmd_file, 'w', encoding='utf-8') as f:
            f.write(mermaid_code)
        
        # Use npx to run mermaid-cli (no global install needed)
        cmd = [
            "npx", "-y", "@mermaid-js/mermaid-cli@latest",
            "-i", str(mmd_file),
            "-o", str(output_png),
            "-b", "white",
            "-s", "2"  # Scale factor for better quality
        ]
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30,
            cwd=TEMP_DIR
        )
        
        if result.returncode == 0 and output_png.exists():
            # Check if file has reasonable size
            if output_png.stat().st_size > 100:
                return True
            else:
                print(f"      ⚠ PNG file too small, might be invalid")
                return False
        else:
            if result.stderr:
                print(f"      ⚠ mmdc error: {result.stderr[:100]}")
            return False
            
    except subprocess.TimeoutExpired:
        print(f"      ⚠ Timeout rendering diagram")
        return False
    except Exception as e:
        print(f"      ⚠ Error: {e}")
        return False
    finally:
        # Cleanup temp mmd file
        if mmd_file.exists():
            mmd_file.unlink()
    
    return False

def process_mermaid_in_markdown(content, chapter_name):
    """Process Mermaid code blocks and convert to PNG images"""
    mermaid_pattern = r'```mermaid\n(.*?)```'
    mermaid_count = 0
    rendered_count = 0
    
    def replace_mermaid(match):
        nonlocal mermaid_count, rendered_count
        mermaid_count += 1
        mermaid_code = match.group(1).strip()
        
        # Detect diagram type
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
        
        # Generate unique filename
        diagram_id = f"{chapter_name}_{mermaid_count}"
        png_file = MERMAID_IMAGES / f"{diagram_id}.png"
        
        print(f"      Rendering {diagram_type} #{mermaid_count}...", end=" ")
        
        # Try to render as PNG
        if render_mermaid_to_png(mermaid_code, png_file):
            rendered_count += 1
            print("✓")
            # Return HTML with embedded image
            return f'''

<div class="mermaid-diagram">
<p class="diagram-caption"><strong>📊 {diagram_type} {mermaid_count}</strong></p>
<img src="{png_file}" alt="{diagram_type} {mermaid_count}" class="diagram-image" />
</div>

'''
        else:
            print("✗ (fallback to code)")
            # Fallback: styled code block
            return f'''

<div class="mermaid-code-fallback">
<p class="code-caption"><strong>📊 {diagram_type} {mermaid_count}</strong> (Code-Ansicht)</p>
<pre><code>{mermaid_code}</code></pre>
<p class="code-note"><em>Hinweis: Diagramm konnte nicht gerendert werden. Online-Version zeigt interaktive Grafik.</em></p>
</div>

'''
    
    processed = re.sub(mermaid_pattern, replace_mermaid, content, flags=re.DOTALL)
    
    if mermaid_count > 0:
        print(f"    ✓ Processed {mermaid_count} Mermaid diagram(s), {rendered_count} rendered as PNG")
    
    return processed

def generate_pdf():
    """Generate PDF with rendered Mermaid diagrams"""
    print_section("ThemisDB Compendium - PDF Generator (PNG Mermaid Rendering)")
    
    # Create directories
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    MERMAID_IMAGES.mkdir(parents=True, exist_ok=True)
    
    # Check dependencies
    print("📦 Checking dependencies...")
    
    try:
        import markdown
        print("  ✓ markdown library found")
    except ImportError:
        print("  Installing markdown...")
        subprocess.run([sys.executable, "-m", "pip", "install", "-q", "markdown"], check=True)
        import markdown
    
    try:
        from weasyprint import HTML, CSS
        print("  ✓ weasyprint library found")
    except ImportError:
        print("  Installing weasyprint...")
        subprocess.run([sys.executable, "-m", "pip", "install", "-q", "weasyprint"], check=True)
        from weasyprint import HTML, CSS
    
    # Test mermaid-cli
    print("  Testing mermaid-cli (via npx)...", end=" ")
    try:
        result = subprocess.run(
            ["npx", "-y", "@mermaid-js/mermaid-cli@latest", "--version"],
            capture_output=True,
            timeout=30
        )
        if result.returncode == 0:
            print("✓")
            mermaid_available = True
        else:
            print("✗")
            mermaid_available = False
    except:
        print("✗")
        mermaid_available = False
    
    if not mermaid_available:
        print("\n  ⚠️  WARNING: mermaid-cli not available!")
        print("  Diagrams will be shown as code blocks instead of images.")
        print("  Ensure Node.js is installed and network is available.\n")
    
    # Process chapters
    print(f"\n📚 Processing {len(CHAPTERS)} chapters...")
    combined_html = []
    
    # Title page
    combined_html.append("""
    <div class="title-page">
        <h1>ThemisDB Compendium</h1>
        <h2>Das vollständige technische Handbuch</h2>
        <p class="version"><strong>Version 1.3.4</strong></p>
        <p class="team">ThemisDB Development Team</p>
        <p class="date">{}</p>
        <p class="mermaid-note">Mit gerenderten Mermaid-Diagrammen</p>
    </div>
    <div class="page-break"></div>
    """.format(datetime.now().strftime("%d. %B %Y")))
    
    # Process each chapter
    for chapter_file in CHAPTERS:
        chapter_path = SCRIPT_DIR / chapter_file
        
        if not chapter_path.exists():
            print(f"  ⚠ Skipping (not found): {chapter_file}")
            continue
        
        print(f"  ✓ Processing: {chapter_file}")
        
        with open(chapter_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Process Mermaid diagrams (convert to PNG)
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
    
    # Create full HTML
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
    
    # Generate PDF
    output_file = OUTPUT_DIR / f"ThemisDB-Kompendium-v1.3.4-{datetime.now().strftime('%Y%m%d')}-rendered.pdf"
    
    print(f"\n🔨 Generating PDF...")
    print(f"  Output: {output_file}")
    
    # Enhanced CSS
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
                content: "Seite " counter(page);
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
            margin-bottom: 30px;
        }
        
        .title-page .mermaid-note {
            margin-top: 50px;
            font-size: 12pt;
            color: #7c4dff;
            font-weight: bold;
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
        
        /* Rendered Mermaid diagrams as PNG images */
        .mermaid-diagram {
            margin: 25px 0;
            padding: 20px;
            background: linear-gradient(135deg, #f5f3ff 0%, #ffffff 100%);
            border: 2px solid #7c4dff;
            border-radius: 8px;
            text-align: center;
            page-break-inside: avoid;
            box-shadow: 0 4px 6px rgba(124, 77, 255, 0.1);
        }
        
        .diagram-caption {
            color: #7c4dff;
            font-weight: bold;
            font-size: 12pt;
            margin: 0 0 15px 0;
            font-family: "Helvetica Neue", Arial, sans-serif;
        }
        
        .diagram-image {
            max-width: 100%;
            height: auto;
            display: block;
            margin: 0 auto;
        }
        
        /* Fallback for diagrams that couldn't be rendered */
        .mermaid-code-fallback {
            margin: 25px 0;
            padding: 0;
            border: 2px solid #ff9800;
            border-radius: 8px;
            background-color: #fff9e6;
            page-break-inside: avoid;
        }
        
        .code-caption {
            background: linear-gradient(135deg, #ff9800 0%, #ffa726 100%);
            color: white;
            font-weight: bold;
            font-size: 11pt;
            margin: 0;
            padding: 12px 20px;
            border-radius: 6px 6px 0 0;
            font-family: "Helvetica Neue", Arial, sans-serif;
        }
        
        .mermaid-code-fallback pre {
            margin: 15px;
            border-left: 4px solid #ff9800;
        }
        
        .code-note {
            margin: 0;
            padding: 10px 20px;
            font-size: 9pt;
            color: #666;
            font-style: italic;
            background-color: #f0f0f0;
            border-top: 1px solid #e0e0e0;
        }
        
        blockquote {
            border-left: 4px solid #7c4dff;
            padding-left: 15px;
            margin: 15px 0;
            color: #666;
            font-style: italic;
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
            print(f"   📚 Chapters: {len([c for c in CHAPTERS if (SCRIPT_DIR / c).exists()])}")
            print(f"   📅 Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
            print(f"\n   ✨ Mermaid diagrams are rendered as PNG images!")
            
            # Cleanup
            print(f"\n🧹 Cleaning up temporary files...")
            shutil.rmtree(TEMP_DIR, ignore_errors=True)
            
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
    sys.exit(generate_pdf())
