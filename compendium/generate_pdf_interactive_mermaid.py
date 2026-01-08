#!/usr/bin/env python3
"""
Enhanced PDF Generation Script for ThemisDB Compendium with Interactive Mermaid Support
- Generates interactive HTML with embedded Mermaid.js
- Converts Mermaid diagrams to SVG for PDF export
- Supports reactive, clickable Mermaid diagrams
"""

import os
import sys
import re
import subprocess
import hashlib
import shutil
import tempfile
from pathlib import Path
from datetime import datetime
import markdown

# Configuration
SCRIPT_DIR = Path(__file__).parent.absolute()
OUTPUT_DIR = SCRIPT_DIR.parent.parent / "pdf_output"
# Cross-platform temp directory
TEMP_DIR = Path(tempfile.gettempdir()) / "themis_pdf_interactive_mermaid"
MERMAID_CACHE = TEMP_DIR / "mermaid_cache"
HTML_OUTPUT = TEMP_DIR / "interactive"

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

def check_mermaid_cli():
    """Check if mermaid-cli (mmdc) is available"""
    try:
        result = subprocess.run(["mmdc", "--version"], capture_output=True, check=True, timeout=5)
        print("  ✓ mermaid-cli (mmdc) found")
        return True
    except (FileNotFoundError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        print("  ⚠ mermaid-cli not found - will use embedded JavaScript for HTML")
        print("    Install with: npm install -g @mermaid-js/mermaid-cli")
        return False

def convert_mermaid_to_svg(mermaid_code, output_file):
    """Convert Mermaid code to SVG using mermaid-cli"""
    mmd_file = TEMP_DIR / f"temp_{hashlib.md5(mermaid_code.encode()).hexdigest()}.mmd"
    
    with open(mmd_file, 'w', encoding='utf-8') as f:
        f.write(mermaid_code)
    
    try:
        result = subprocess.run(
            ["mmdc", "-i", str(mmd_file), "-o", str(output_file), "-b", "transparent"],
            capture_output=True,
            timeout=30
        )
        
        if result.returncode == 0 and output_file.exists():
            return True
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    
    return False

def process_mermaid_for_interactive_html(content, chapter_name):
    """Process Mermaid code blocks for interactive HTML with embedded JavaScript"""
    mermaid_pattern = r'```mermaid\n(.*?)```'
    mermaid_count = 0
    
    def replace_mermaid(match):
        nonlocal mermaid_count
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
        elif mermaid_code.startswith("classDiagram"):
            diagram_type = "Class Diagram"
        elif mermaid_code.startswith("journey"):
            diagram_type = "User Journey"
        elif mermaid_code.startswith("gitGraph"):
            diagram_type = "Git Graph"
        
        # Create unique ID for this diagram
        diagram_id = f"{chapter_name}_diagram_{mermaid_count}"
        
        # Return interactive HTML with embedded Mermaid
        return f'''
<div class="mermaid-container" id="container_{diagram_id}">
    <div class="mermaid-header">
        <span class="mermaid-icon">📊</span>
        <span class="mermaid-title">{diagram_type} #{mermaid_count}</span>
        <button class="mermaid-fullscreen-btn" onclick="toggleFullscreen('{diagram_id}')" title="Vollbild">⛶</button>
    </div>
    <div class="mermaid-diagram" id="{diagram_id}">
{mermaid_code}
    </div>
    <div class="mermaid-footer">
        <em>💡 Interaktives Diagramm - Klicken Sie auf Elemente für Details</em>
    </div>
</div>
'''
    
    processed = re.sub(mermaid_pattern, replace_mermaid, content, flags=re.DOTALL)
    
    if mermaid_count > 0:
        print(f"    ✓ Processed {mermaid_count} Mermaid diagram(s) for interactive HTML")
    
    return processed

def process_mermaid_for_pdf(content, chapter_name, use_svg=True):
    """Process Mermaid code blocks for PDF generation"""
    mermaid_pattern = r'```mermaid\n(.*?)```'
    mermaid_count = 0
    
    def replace_mermaid(match):
        nonlocal mermaid_count
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
        
        diagram_id = f"{chapter_name}_{mermaid_count}"
        svg_file = MERMAID_CACHE / f"{diagram_id}.svg"
        
        # Try to convert to SVG if requested and available
        if use_svg and convert_mermaid_to_svg(mermaid_code, svg_file):
            return f'''
<div class="mermaid-diagram-pdf">
<p class="diagram-title">📊 {diagram_type} {mermaid_count}</p>
<img src="{svg_file}" alt="Mermaid Diagram {mermaid_count}" style="max-width: 100%; height: auto;" />
</div>
'''
        else:
            # Fallback to styled code block
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
<em>Hinweis: Interaktive Version verfügbar in HTML-Output</em>
</div>
</div>
'''
    
    processed = re.sub(mermaid_pattern, replace_mermaid, content, flags=re.DOTALL)
    
    if mermaid_count > 0:
        print(f"    ✓ Processed {mermaid_count} Mermaid diagram(s) for PDF")
    
    return processed

def generate_interactive_html():
    """Generate interactive HTML with embedded Mermaid.js"""
    print_section("Generating Interactive HTML with Mermaid.js")
    
    # Create directories
    HTML_OUTPUT.mkdir(parents=True, exist_ok=True)
    
    # Collect and process chapters
    print(f"📚 Collecting {len(CHAPTERS)} chapters...")
    combined_html = []
    
    # Process each chapter
    for chapter_file in CHAPTERS:
        chapter_path = SCRIPT_DIR / chapter_file
        
        if not chapter_path.exists():
            print(f"  ⚠ Skipping (not found): {chapter_file}")
            continue
        
        print(f"  ✓ Processing: {chapter_file}")
        
        with open(chapter_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Process Mermaid diagrams for interactive HTML
        content = process_mermaid_for_interactive_html(content, chapter_path.stem)
        
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
        
        combined_html.append(f'<div class="chapter" id="{chapter_path.stem}">\n{html}\n</div>')
    
    # Create full HTML document with embedded Mermaid.js
    full_html = f"""<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThemisDB Compendium - Interactive Mermaid Edition</title>
    
    <!-- Mermaid.js CDN -->
    <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
    
    <style>
        * {{
            box-sizing: border-box;
        }}
        
        body {{
            font-family: Georgia, "Times New Roman", serif;
            font-size: 16px;
            line-height: 1.65;
            color: #2c3e50;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
        }}
        
        .header {{
            text-align: center;
            padding: 40px 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border-radius: 12px;
            margin-bottom: 30px;
            box-shadow: 0 8px 16px rgba(0,0,0,0.2);
        }}
        
        .header h1 {{
            margin: 0;
            font-size: 2.5em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }}
        
        .header p {{
            margin: 10px 0 0 0;
            font-size: 1.2em;
            opacity: 0.95;
        }}
        
        .chapter {{
            background: white;
            padding: 30px;
            margin-bottom: 20px;
            border-radius: 8px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }}
        
        h1 {{
            font-size: 2.2em;
            color: #7c4dff;
            border-bottom: 3px solid #7c4dff;
            padding-bottom: 10px;
        }}
        
        h2 {{
            font-size: 1.8em;
            color: #555;
            border-bottom: 1px solid #ddd;
            padding-bottom: 5px;
            margin-top: 30px;
        }}
        
        h3 {{
            font-size: 1.4em;
            color: #7c4dff;
            margin-top: 20px;
        }}
        
        code {{
            background-color: #f5f5f5;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: "Courier New", monospace;
            font-size: 0.9em;
            color: #c7254e;
        }}
        
        pre {{
            background-color: #f8f8f8;
            border-left: 4px solid #7c4dff;
            padding: 15px;
            margin: 15px 0;
            overflow-x: auto;
            border-radius: 4px;
        }}
        
        pre code {{
            background: none;
            padding: 0;
            color: #333;
        }}
        
        table {{
            border-collapse: collapse;
            width: 100%;
            margin: 15px 0;
        }}
        
        th {{
            background-color: #7c4dff;
            color: white;
            padding: 12px;
            text-align: left;
            font-weight: bold;
        }}
        
        td {{
            padding: 10px 12px;
            border-bottom: 1px solid #ddd;
        }}
        
        tr:nth-child(even) {{
            background-color: #f9f9f9;
        }}
        
        tr:hover {{
            background-color: #f0f0f0;
        }}
        
        /* Interactive Mermaid Styles */
        .mermaid-container {{
            margin: 30px 0;
            border: 2px solid #7c4dff;
            border-radius: 12px;
            background: linear-gradient(135deg, #f5f3ff 0%, #ffffff 100%);
            box-shadow: 0 6px 12px rgba(124, 77, 255, 0.15);
            overflow: hidden;
            transition: all 0.3s ease;
        }}
        
        .mermaid-container:hover {{
            box-shadow: 0 8px 20px rgba(124, 77, 255, 0.25);
            transform: translateY(-2px);
        }}
        
        .mermaid-header {{
            background: linear-gradient(135deg, #7c4dff 0%, #9c4dff 100%);
            color: white;
            padding: 15px 20px;
            font-weight: bold;
            font-size: 1.1em;
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 2px solid #6c3ddf;
        }}
        
        .mermaid-icon {{
            font-size: 1.3em;
            margin-right: 10px;
        }}
        
        .mermaid-title {{
            flex: 1;
        }}
        
        .mermaid-fullscreen-btn {{
            background: rgba(255, 255, 255, 0.2);
            border: 1px solid rgba(255, 255, 255, 0.3);
            color: white;
            padding: 5px 12px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 1.2em;
            transition: all 0.2s;
        }}
        
        .mermaid-fullscreen-btn:hover {{
            background: rgba(255, 255, 255, 0.3);
            transform: scale(1.1);
        }}
        
        .mermaid-diagram {{
            padding: 30px;
            background-color: #fafafa;
            min-height: 200px;
            display: flex;
            justify-content: center;
            align-items: center;
            overflow-x: auto;
        }}
        
        .mermaid-footer {{
            padding: 12px 20px;
            background-color: #f0f0f0;
            border-top: 1px solid #e0e0e0;
            font-size: 0.9em;
            color: #666;
            font-style: italic;
            text-align: center;
        }}
        
        /* Fullscreen mode */
        .mermaid-container.fullscreen {{
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            z-index: 9999;
            margin: 0;
            border-radius: 0;
            display: flex;
            flex-direction: column;
        }}
        
        .mermaid-container.fullscreen .mermaid-diagram {{
            flex: 1;
            overflow: auto;
        }}
        
        /* Responsive */
        @media (max-width: 768px) {{
            body {{
                padding: 10px;
                font-size: 14px;
            }}
            
            .header h1 {{
                font-size: 1.8em;
            }}
            
            .mermaid-diagram {{
                padding: 15px;
            }}
        }}
        
        /* Navigation */
        .nav-container {{
            background: white;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }}
        
        .nav-container h3 {{
            margin-top: 0;
            color: #7c4dff;
        }}
        
        .nav-container a {{
            color: #7c4dff;
            text-decoration: none;
            display: block;
            padding: 5px 0;
            transition: all 0.2s;
        }}
        
        .nav-container a:hover {{
            color: #9c4dff;
            padding-left: 10px;
        }}
    </style>
</head>
<body>
    <div class="header">
        <h1>📊 ThemisDB Compendium</h1>
        <p>Interactive Mermaid Edition - v1.3.4</p>
        <p>Generiert: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
    </div>
    
    {''.join(combined_html)}
    
    <script>
        // Initialize Mermaid with enhanced configuration
        mermaid.initialize({{
            startOnLoad: true,
            theme: 'default',
            themeVariables: {{
                primaryColor: '#7c4dff',
                primaryTextColor: '#fff',
                primaryBorderColor: '#6c3ddf',
                lineColor: '#7c4dff',
                secondaryColor: '#f5f3ff',
                tertiaryColor: '#fafafa'
            }},
            flowchart: {{
                useMaxWidth: true,
                htmlLabels: true,
                curve: 'basis'
            }},
            sequence: {{
                diagramMarginX: 50,
                diagramMarginY: 10,
                actorMargin: 50,
                width: 150,
                height: 65,
                boxMargin: 10,
                boxTextMargin: 5,
                noteMargin: 10,
                messageMargin: 35
            }},
            gantt: {{
                titleTopMargin: 25,
                barHeight: 20,
                barGap: 4,
                topPadding: 50,
                leftPadding: 75,
                gridLineStartPadding: 35,
                fontSize: 11,
                numberSectionStyles: 4
            }}
        }});
        
        // Fullscreen toggle function
        function toggleFullscreen(diagramId) {{
            const container = document.getElementById('container_' + diagramId);
            if (container) {{
                container.classList.toggle('fullscreen');
            }}
        }}
        
        // ESC key to exit fullscreen
        document.addEventListener('keydown', function(e) {{
            if (e.key === 'Escape') {{
                const fullscreenElements = document.querySelectorAll('.mermaid-container.fullscreen');
                fullscreenElements.forEach(el => el.classList.remove('fullscreen'));
            }}
        }});
        
        // Add click listeners to Mermaid diagrams for interactivity
        document.addEventListener('DOMContentLoaded', function() {{
            // Wait for Mermaid to render
            setTimeout(function() {{
                const mermaidDiagrams = document.querySelectorAll('.mermaid-diagram');
                mermaidDiagrams.forEach(function(diagram) {{
                    // Add click event to all SVG elements
                    const svgElements = diagram.querySelectorAll('g[class*="node"], g[class*="edgePath"]');
                    svgElements.forEach(function(element) {{
                        element.style.cursor = 'pointer';
                        element.addEventListener('click', function() {{
                            // Get element text
                            const textElement = this.querySelector('text, span');
                            if (textElement) {{
                                const text = textElement.textContent;
                                // Show info (could be enhanced with tooltips, modals, etc.)
                                console.log('Clicked element:', text);
                                // Optional: Add visual feedback
                                this.style.opacity = '0.7';
                                setTimeout(() => {{
                                    this.style.opacity = '1';
                                }}, 200);
                            }}
                        }});
                    }});
                }});
            }}, 1000);
        }});
    </script>
</body>
</html>"""
    
    # Save HTML file
    html_file = HTML_OUTPUT / "index.html"
    with open(html_file, 'w', encoding='utf-8') as f:
        f.write(full_html)
    
    print(f"\n✅ Interactive HTML generated successfully!")
    print(f"   📄 File: {html_file}")
    print(f"   📂 Open with: file://{html_file}")
    
    return html_file

def generate_pdf_weasyprint(use_svg=True):
    """Generate PDF using WeasyPrint with Mermaid preprocessing"""
    print_section("Generating PDF with WeasyPrint")
    
    # Create directories
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    TEMP_DIR.mkdir(parents=True, exist_ok=True)
    MERMAID_CACHE.mkdir(parents=True, exist_ok=True)
    
    # Check dependencies
    print("📦 Checking dependencies...")
    try:
        from weasyprint import HTML, CSS
        print("  ✓ weasyprint library found")
    except ImportError:
        print("  ✗ weasyprint library not found. Installing...")
        subprocess.run([sys.executable, "-m", "pip", "install", "weasyprint"], check=True)
        from weasyprint import HTML, CSS
    
    mermaid_available = check_mermaid_cli()
    
    # Collect and process chapters
    print(f"\n📚 Collecting {len(CHAPTERS)} chapters for PDF...")
    combined_html = []
    
    # Add title page
    combined_html.append(f"""
    <div class="title-page">
        <h1>ThemisDB Compendium</h1>
        <h2>Das vollständige technische Handbuch</h2>
        <p><strong>Version 1.3.4</strong></p>
        <p><strong>Interactive Mermaid Edition</strong></p>
        <p>ThemisDB Development Team</p>
        <p>{datetime.now().strftime("%Y-%m-%d")}</p>
    </div>
    <div class="page-break"></div>
    """)
    
    # Process each chapter
    for chapter_file in CHAPTERS:
        chapter_path = SCRIPT_DIR / chapter_file
        
        if not chapter_path.exists():
            print(f"  ⚠ Skipping (not found): {chapter_file}")
            continue
        
        print(f"  ✓ Processing: {chapter_file}")
        
        with open(chapter_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Process Mermaid diagrams for PDF
        content = process_mermaid_for_pdf(content, chapter_path.stem, use_svg=use_svg and mermaid_available)
        
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
    html_file = TEMP_DIR / "compendium_for_pdf.html"
    with open(html_file, 'w', encoding='utf-8') as f:
        f.write(full_html)
    
    # Generate PDF
    suffix = "-interactive-mermaid" if mermaid_available else "-mermaid-styled"
    output_file = OUTPUT_DIR / f"ThemisDB-Compendium-v1.3.4-{datetime.now().strftime('%Y%m%d')}{suffix}.pdf"
    
    print(f"\n🔨 Generating PDF with WeasyPrint...")
    print(f"  Output: {output_file}")
    
    # Enhanced CSS for PDF
    css = CSS(string='''
        @page {
            size: A4;
            margin: 25mm 20mm;
            
            @top-center {
                content: "ThemisDB Compendium v1.3.4 - Interactive Mermaid Edition";
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
        
        .mermaid-diagram-pdf {
            margin: 20px 0;
            padding: 15px;
            background-color: #fafafa;
            border: 2px solid #7c4dff;
            border-radius: 8px;
            text-align: center;
            page-break-inside: avoid;
        }
        
        .mermaid-diagram-pdf .diagram-title {
            font-weight: bold;
            color: #7c4dff;
            margin-bottom: 10px;
            font-size: 11pt;
        }
        
        .mermaid-diagram-pdf img {
            max-width: 100%;
            height: auto;
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
                print(f"\n   ℹ️  Note: Mermaid diagrams shown as styled code blocks")
                print(f"   💡 For SVG rendering, install: npm install -g @mermaid-js/mermaid-cli")
            
            return 0
        else:
            print("\n❌ PDF generation failed!")
            return 1
            
    except Exception as e:
        print(f"\n❌ PDF generation failed: {e}")
        import traceback
        traceback.print_exc()
        return 1

def main():
    """Main execution function"""
    print_section("ThemisDB Compendium - Interactive Mermaid Generator")
    
    # Check dependencies
    print("📦 Checking dependencies...")
    try:
        import markdown
        print("  ✓ markdown library found")
    except ImportError:
        print("  ✗ markdown library not found. Installing...")
        subprocess.run([sys.executable, "-m", "pip", "install", "markdown"], check=True)
    
    # Generate interactive HTML
    html_file = generate_interactive_html()
    
    # Generate PDF
    pdf_result = generate_pdf_weasyprint()
    
    print_section("Summary")
    print(f"✅ Interactive HTML: {html_file}")
    print(f"✅ PDF generation: {'Success' if pdf_result == 0 else 'Failed'}")
    print(f"\n💡 Next steps:")
    print(f"   1. Open interactive HTML: file://{html_file}")
    print(f"   2. Check PDF output in: {OUTPUT_DIR}")
    print(f"   3. For SVG rendering in PDF, install mermaid-cli:")
    print(f"      npm install -g @mermaid-js/mermaid-cli")
    
    return pdf_result

if __name__ == "__main__":
    sys.exit(main())
