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

# Determine if we're in compendium/ or docs/compendium/
if SCRIPT_DIR.name == "compendium":
    # Check if this looks like repo_root/compendium (has chapter files)
    if (SCRIPT_DIR / "chapter_00_genesis.md").exists():
        # We're in repo_root/compendium - use this directory
        COMPENDIUM_ROOT = SCRIPT_DIR
        SOURCE_DIR = SCRIPT_DIR
    else:
        # We're in docs/compendium, target is repo_root/compendium
        REPO_ROOT = SCRIPT_DIR.parent.parent
        COMPENDIUM_ROOT = REPO_ROOT / "compendium"
        SOURCE_DIR = COMPENDIUM_ROOT
else:
    # Fallback
    REPO_ROOT = SCRIPT_DIR.parent.parent
    COMPENDIUM_ROOT = REPO_ROOT / "compendium"
    SOURCE_DIR = COMPENDIUM_ROOT

COMPENDIUM_ROOT.mkdir(parents=True, exist_ok=True)

print(f"📂 Source directory: {SOURCE_DIR}")
print(f"📂 Output directory: {COMPENDIUM_ROOT}")

# All outputs inside compendium/
TEMP_PARENT = COMPENDIUM_ROOT / "temp"
TEMP_PARENT.mkdir(parents=True, exist_ok=True)
OUTPUT_DIR = COMPENDIUM_ROOT / "pdf"
# Create a build-local temp dir inside compendium/temp
TEMP_DIR = Path(tempfile.mkdtemp(prefix="themis_pdf_", dir=TEMP_PARENT))
MERMAID_IMAGES = TEMP_DIR / "mermaid_images"

# Chrome executable for Mermaid rendering
CHROME_PATH = COMPENDIUM_ROOT / "chrome" / "linux-143.0.7499.169" / "chrome-linux64" / "chrome"
if CHROME_PATH.exists():
    os.environ['PUPPETEER_EXECUTABLE_PATH'] = str(CHROME_PATH)
    print(f"📦 Using Chromium from: {CHROME_PATH.parent}")

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
    "chapter_23_testing_qa.md",
    "chapter_24_ai_ethics.md",
    "chapter_25_devops_infrastructure.md",
    "chapter_26_migration_legacy.md",
    "chapter_27_troubleshooting.md",
    "chapter_28_aql_reference.md",
    "chapter_29_analytics_process_mining.md",
    "chapter_30_deployment_operations.md",
    "chapter_31_api_protocols.md",
    "chapter_32_aql_oop_implementation.md",
    "chapter_33_best_practices.md",
    "chapter_34_query_optimization.md",
    "chapter_35_data_modeling_patterns.md",
    "chapter_36_security_hardening.md",
    "chapter_37_ecosystem_integration.md",
    "chapter_38_observability_sre.md",
    "chapter_39_performance_tuning_cookbook.md",
    "chapter_40_data_governance_compliance.md",
    "chapter_41_hands_on_labs.md",
    "appendix_e_incident_runbooks.md",
    "appendix_f_aql_cheatsheet.md",
    "appendix_g_configuration.md",
    "appendix_h_glossary.md",
    "appendix_i_troubleshooting.md",
    "appendix_literatur.md",
    "appendix_d_feature_status.md"
]

def print_section(title):
    """Print formatted section header"""
    print("\n" + "=" * 70)
    print(f"  {title}")
    print("=" * 70 + "\n")

def get_figure_caption(diagram_type, count):
    """Generate descriptive caption for figure"""
    captions = {
        "flowchart": "Flussdiagramm zur Darstellung von Prozessabläufen und Entscheidungspfaden",
        "graph": "Graphendarstellung der Beziehungen und Abhängigkeiten zwischen Komponenten",
        "sequenceDiagram": "Sequenzdiagramm zeigt die zeitliche Abfolge von Interaktionen zwischen Komponenten",
        "gantt": "Gantt-Diagramm zur Visualisierung zeitlicher Abläufe und Zeitplanung",
        "classDiagram": "Klassendiagramm zeigt die Struktur und Beziehungen von Datenmodellen",
        "erDiagram": "Entity-Relationship-Diagramm zur Darstellung der Datenbankstruktur",
        "stateDiagram": "Zustandsdiagramm visualisiert mögliche Zustände und Übergänge im System",
        "pie": "Kreisdiagramm zur prozentualen Verteilung von Kategorien",
        "quadrantChart": "Quadranten-Diagramm zur Einordnung von Elementen nach zwei Dimensionen"
    }
    return captions.get(diagram_type, f"{diagram_type}-Diagramm")

def render_mermaid_to_png(mermaid_code, output_png):
    """Render Mermaid code to PNG using mermaid-cli via npx"""
    # Create temporary .mmd file
    mmd_file = TEMP_DIR / f"temp_{hashlib.md5(mermaid_code.encode()).hexdigest()}.mmd"
    
    # Create mermaid config for clean white background
    config_file = TEMP_DIR / "mermaid-config.json"
    config_content = '''{
  "theme": "default",
  "themeVariables": {
    "primaryColor": "#ffffff",
    "primaryTextColor": "#000000",
    "primaryBorderColor": "#cccccc",
    "lineColor": "#666666",
    "secondaryColor": "#f5f5f5",
    "tertiaryColor": "#fafafa",
    "background": "#ffffff",
    "mainBkg": "#ffffff",
    "secondBkg": "#f5f5f5",
    "tertiaryBkg": "#fafafa"
  },
  "flowchart": {
    "useMaxWidth": true
  }
}'''
    
    try:
        # Write config file
        with open(config_file, 'w', encoding='utf-8') as f:
            f.write(config_content)
        
        # Write mermaid code to file
        with open(mmd_file, 'w', encoding='utf-8') as f:
            f.write(mermaid_code)
        
        # Use npx to run mermaid-cli (no global install needed)
        cmd = [
            "npx", "-y", "@mermaid-js/mermaid-cli@latest",
            "-i", str(mmd_file),
            "-o", str(output_png),
            "-c", str(config_file),
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

def process_mermaid_in_markdown(content, chapter_name, figure_list, global_figure_counter):
    """Process Mermaid code blocks and convert to PNG images"""
    mermaid_pattern = r'```mermaid\n(.*?)```'
    mermaid_count = 0
    rendered_count = 0
    current_counter = global_figure_counter
    
    def replace_mermaid(match):
        nonlocal mermaid_count, rendered_count, current_counter
        mermaid_count += 1
        current_counter += 1
        mermaid_code = match.group(1).strip()
        
        # Detect diagram type
        diagram_type = "Diagram"
        if mermaid_code.startswith("graph"):
            diagram_type = "flowchart"
        elif mermaid_code.startswith("sequenceDiagram"):
            diagram_type = "sequenceDiagram"
        elif mermaid_code.startswith("stateDiagram"):
            diagram_type = "stateDiagram"
        elif mermaid_code.startswith("gantt"):
            diagram_type = "gantt"
        elif mermaid_code.startswith("erDiagram"):
            diagram_type = "erDiagram"
        elif mermaid_code.startswith("flowchart"):
            diagram_type = "flowchart"
        elif mermaid_code.startswith("quadrantChart"):
            diagram_type = "quadrantChart"
        elif mermaid_code.startswith("classDiagram"):
            diagram_type = "classDiagram"
        elif mermaid_code.startswith("pie"):
            diagram_type = "pie"
        
        # Generate unique filename
        diagram_id = f"{chapter_name}_{mermaid_count}"
        png_file = MERMAID_IMAGES / f"{diagram_id}.png"
        
        print(f"      Rendering {diagram_type} #{mermaid_count}...", end=" ")
        
        # Try to render as PNG
        if render_mermaid_to_png(mermaid_code, png_file):
            rendered_count += 1
            print("✓")
            # Return HTML with embedded image (use absolute file:// URL for WeasyPrint)
            absolute_png_url = f"file://{png_file.absolute()}"
            # Determine figure caption based on diagram type
            caption_text = get_figure_caption(diagram_type, current_counter)
            
            # Add to figure list
            chapter_title = chapter_name.replace("_", " ").replace("chapter ", "Kapitel ").title()
            figure_list.append({
                'number': current_counter,
                'caption': caption_text,
                'chapter': chapter_title
            })
            
            return f'''

<div class="mermaid-diagram">
<img src="{absolute_png_url}" alt="{diagram_type} {current_counter}" class="diagram-image" />
<p class="diagram-caption"><strong>Abb. {current_counter}:</strong> {caption_text}</p>
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
    
    return processed, current_counter

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
    figure_list = []  # Track all figures for list of figures
    global_figure_counter = 0
    
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
    
    # Placeholder for list of figures (will be inserted later)
    lof_placeholder_index = len(combined_html)
    
    # Process each chapter
    for chapter_file in CHAPTERS:
        chapter_path = SOURCE_DIR / chapter_file
        
        if not chapter_path.exists():
            print(f"  ⚠ Skipping (not found): {chapter_file}")
            continue
        
        print(f"  ✓ Processing: {chapter_file}")
        
        with open(chapter_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Process Mermaid diagrams (convert to PNG)
        content, global_figure_counter = process_mermaid_in_markdown(
            content, chapter_path.stem, figure_list, global_figure_counter
        )
        
        # Convert markdown to HTML
        html = markdown.markdown(
            content,
            extensions=[
                'markdown.extensions.tables',
                'markdown.extensions.fenced_code',
                'markdown.extensions.codehilite',
                'markdown.extensions.toc',
            ],
            extension_configs={
                'markdown.extensions.codehilite': {
                    'css_class': 'codehilite',
                    'linenums': False,
                    'guess_lang': True,
                }
            }
        )
        
        combined_html.append(f'<div class="chapter">\n{html}\n</div>')
        combined_html.append('<div class="page-break"></div>')
    
    # Generate List of Figures
    lof_html = """
    <div class="list-of-figures">
        <h1>Abbildungsverzeichnis</h1>
        <table class="figure-list">
            <thead>
                <tr>
                    <th style="width: 15%;">Nummer</th>
                    <th style="width: 60%;">Beschreibung</th>
                    <th style="width: 25%;">Kapitel</th>
                </tr>
            </thead>
            <tbody>
    """
    
    for fig in figure_list:
        lof_html += f"""
                <tr>
                    <td>Abb. {fig['number']}</td>
                    <td>{fig['caption']}</td>
                    <td>{fig['chapter']}</td>
                </tr>
        """
    
    lof_html += """
            </tbody>
        </table>
    </div>
    <div class="page-break"></div>
    """
    
    # Insert list of figures after title page
    combined_html.insert(lof_placeholder_index, lof_html)
    
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
        
        /* Single-column scientific layout (WeasyPrint-friendly) */
        .chapter {
            margin-bottom: 20pt;
        }
        
        .chapter h1,
        .chapter h2,
        .chapter h3,
        .chapter .mermaid-diagram,
        .chapter pre,
        .chapter table,
        .chapter .figure-list {
            page-break-inside: avoid;
        }
        
        h1 {
            font-size: 16pt;
            color: #333;
            font-family: "Helvetica Neue", Arial, sans-serif;
            font-weight: bold;
            page-break-before: always;
            margin-top: 20pt;
            margin-bottom: 8pt;
            border: none;
            padding: 0;
        }
        
        h2 {
            font-size: 13pt;
            color: #333;
            font-family: "Helvetica Neue", Arial, sans-serif;
            font-weight: bold;
            margin-top: 14pt;
            margin-bottom: 6pt;
            border: none;
            padding: 0;
        }
        
        h3 {
            font-size: 11.5pt;
            color: #333;
            font-family: "Helvetica Neue", Arial, sans-serif;
            font-weight: bold;
            margin-top: 10pt;
            margin-bottom: 4pt;
            border: none;
            padding: 0;
        }
        
        /* Inline code */
        code {
            background-color: #f6f8fa;
            padding: 2px 5px;
            border-radius: 3px;
            font-family: "Consolas", "Monaco", "Courier New", monospace;
            font-size: 9pt;
            color: #d73a49;
        }
        
        /* Code blocks - light theme for print */
        pre {
            background-color: #f8f8f8;
            border-left: 3px solid #007acc;
            padding: 8px 12px;
            margin: 12px 0;
            overflow-x: auto;
            font-size: 8.5pt;
            line-height: 1.4;
            border-radius: 4px;
            page-break-inside: avoid;
        }
        
        /* Small code blocks: avoid page breaks */
        pre.small-code {
            page-break-inside: avoid;
        }
        
        /* Large code blocks: allow breaks at logical points */
        pre.large-code {
            page-break-inside: auto;
        }
        
        pre code {
            background: none;
            padding: 0;
            color: #24292e;
            font-family: "Consolas", "Monaco", "Courier New", monospace;
            font-size: 8.5pt;
            display: block;
        }
        
        /* Light syntax highlighting (GitHub-like) */
        .codehilite { background: #f8f8f8; color: #24292e; }
        .codehilite .hll { background-color: #fff8c5; }
        .codehilite .c { color: #6e7781; font-style: italic; } /* Comment */
        .codehilite .err { color: #cf222e; } /* Error */
        .codehilite .k { color: #cf222e; } /* Keyword */
        .codehilite .l { color: #0a3069; } /* Literal */
        .codehilite .n { color: #24292e; } /* Name */
        .codehilite .o { color: #24292e; } /* Operator */
        .codehilite .p { color: #24292e; } /* Punctuation */
        .codehilite .ch { color: #6e7781; font-style: italic; }
        .codehilite .cm { color: #6e7781; font-style: italic; }
        .codehilite .cp { color: #cf222e; }
        .codehilite .cpf { color: #6e7781; font-style: italic; }
        .codehilite .c1 { color: #6e7781; font-style: italic; }
        .codehilite .cs { color: #6e7781; font-style: italic; }
        .codehilite .gd { color: #cf222e; }
        .codehilite .ge { font-style: italic; }
        .codehilite .gi { color: #116329; }
        .codehilite .gs { font-weight: bold; }
        .codehilite .gu { color: #57606a; }
        .codehilite .kc { color: #cf222e; }
        .codehilite .kd { color: #cf222e; }
        .codehilite .kn { color: #cf222e; }
        .codehilite .kp { color: #cf222e; }
        .codehilite .kr { color: #cf222e; }
        .codehilite .kt { color: #116329; }
        .codehilite .ld { color: #0a3069; }
        .codehilite .m { color: #0550ae; }
        .codehilite .s { color: #0a3069; }
        .codehilite .na { color: #24292e; }
        .codehilite .nb { color: #24292e; }
        .codehilite .nc { color: #116329; }
        .codehilite .no { color: #1f6feb; }
        .codehilite .nd { color: #1f6feb; }
        .codehilite .ni { color: #24292e; }
        .codehilite .ne { color: #116329; }
        .codehilite .nf { color: #0550ae; }
        .codehilite .nl { color: #24292e; }
        .codehilite .nn { color: #116329; }
        .codehilite .nx { color: #24292e; }
        .codehilite .py { color: #24292e; }
        .codehilite .nt { color: #cf222e; }
        .codehilite .nv { color: #24292e; }
        .codehilite .ow { color: #cf222e; }
        .codehilite .mb { color: #0550ae; }
        .codehilite .mf { color: #0550ae; }
        .codehilite .mh { color: #0550ae; }
        .codehilite .mi { color: #0550ae; }
        .codehilite .mo { color: #0550ae; }
        .codehilite .sa { color: #0a3069; }
        .codehilite .sb { color: #0a3069; }
        .codehilite .sc { color: #0a3069; }
        .codehilite .dl { color: #0a3069; }
        .codehilite .sd { color: #6e7781; }
        .codehilite .s2 { color: #0a3069; }
        .codehilite .se { color: #b35900; }
        .codehilite .sh { color: #0a3069; }
        .codehilite .si { color: #b35900; }
        .codehilite .sx { color: #0a3069; }
        .codehilite .sr { color: #cf222e; }
        .codehilite .s1 { color: #0a3069; }
        .codehilite .ss { color: #0a3069; }
        .codehilite .bp { color: #cf222e; }
        .codehilite .fm { color: #0550ae; }
        .codehilite .vc { color: #24292e; }
        .codehilite .vg { color: #24292e; }
        .codehilite .vi { color: #24292e; }
        .codehilite .vm { color: #24292e; }
        
        /* Tables - compact scientific style */
        table {
            border-collapse: collapse;
            width: 100%;
            margin: 10pt 0 4pt 0;
            page-break-inside: avoid;
            font-size: 9pt;
        }
        
        th {
            background-color: #f0f0f0;
            color: #333;
            padding: 4pt 6pt;
            text-align: left;
            font-weight: bold;
            border-bottom: 2pt solid #666;
            border-top: 1pt solid #666;
        }
        
        td {
            padding: 3pt 6pt;
            border-bottom: 0.5pt solid #ccc;
        }
        
        tr:nth-child(even) {
            background-color: transparent;
        }
        
        tr:last-child td {
            border-bottom: 1pt solid #666;
        }
        
        /* Table captions */
        .table-caption {
            color: #333;
            font-weight: normal;
            font-size: 9pt;
            margin: 2pt 0 8pt 0;
            text-align: left;
            font-family: "Helvetica Neue", Arial, sans-serif;
            font-style: italic;
        }
        
        .table-caption strong {
            font-weight: bold;
            font-style: normal;
        }
        
        /* List of Figures */
        .list-of-figures {
            margin: 40px 0;
        }
        
        .list-of-figures h1 {
            font-size: 24pt;
            color: #7c4dff;
            border-bottom: 3px solid #7c4dff;
            padding-bottom: 10px;
            margin-bottom: 30px;
        }
        
        .figure-list {
            width: 100%;
            border-collapse: collapse;
        }
        
        .figure-list th {
            background-color: #7c4dff;
            color: white;
            padding: 12px 10px;
            text-align: left;
            font-weight: bold;
            font-size: 11pt;
        }
        
        .figure-list td {
            padding: 10px;
            border-bottom: 1px solid #ddd;
            font-size: 10pt;
        }
        
        .figure-list tr:nth-child(even) {
            background-color: #f9f9f9;
        }
        
        /* Rendered Mermaid diagrams as PNG images */
        .mermaid-diagram {
            margin: 30px 0;
            padding: 0;
            background: #ffffff;
            border: none;
            text-align: center;
            page-break-inside: avoid;
        }
        
        .diagram-image {
            max-width: 100%;
            height: auto;
            display: block;
            margin: 0 auto 10px auto;
            border: none;
            box-shadow: none;
        }
        
        .diagram-caption {
            color: #333333;
            font-weight: normal;
            font-size: 10pt;
            margin: 5px 0 0 0;
            text-align: center;
            font-family: "Helvetica Neue", Arial, sans-serif;
            font-style: italic;
        }
        
        .diagram-caption strong {
            font-weight: bold;
            font-style: normal;
        }
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
    
    cleanup_done = False
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
            cleanup_done = True
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
    finally:
        if not cleanup_done:
            print(f"\n🧹 Cleaning up temporary files...")
            shutil.rmtree(TEMP_DIR, ignore_errors=True)

if __name__ == "__main__":
    sys.exit(generate_pdf())
