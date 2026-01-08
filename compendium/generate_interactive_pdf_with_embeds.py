#!/usr/bin/env python3
"""
Interactive PDF Generator with Embedded Mermaid HTML Attachments
Creates PDFs with:
1. Static SVG images for all readers
2. Embedded HTML files with interactive Mermaid.js
3. Clickable links to attachments
4. QR codes to online versions
"""

import os
import sys
import re
import subprocess
import hashlib
import json
from pathlib import Path
from datetime import datetime
from io import BytesIO

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent))

try:
    from reportlab.pdfgen import canvas
    from reportlab.lib.pagesizes import A4
    from reportlab.lib.units import mm
    from reportlab.lib.colors import HexColor
    from reportlab.pdfbase import pdfmetrics
    from reportlab.pdfbase.ttfonts import TTFont
    from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, PageBreak, Image, Table
    from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
    from reportlab.lib.enums import TA_CENTER, TA_LEFT
except ImportError:
    print("Installing reportlab...")
    subprocess.run([sys.executable, "-m", "pip", "install", "reportlab"], check=True)
    from reportlab.pdfgen import canvas
    from reportlab.lib.pagesizes import A4
    from reportlab.lib.units import mm
    from reportlab.lib.colors import HexColor

try:
    from pypdf import PdfWriter, PdfReader
except ImportError:
    print("Installing pypdf...")
    subprocess.run([sys.executable, "-m", "pip", "install", "pypdf"], check=True)
    from pypdf import PdfWriter, PdfReader

try:
    import qrcode
except ImportError:
    print("Installing qrcode...")
    subprocess.run([sys.executable, "-m", "pip", "install", "qrcode[pil]"], check=True)
    import qrcode

try:
    import markdown
except ImportError:
    print("Installing markdown...")
    subprocess.run([sys.executable, "-m", "pip", "install", "markdown"], check=True)
    import markdown

# Configuration
SCRIPT_DIR = Path(__file__).parent.absolute()
OUTPUT_DIR = SCRIPT_DIR / "pdf"
TEMP_DIR = Path("/tmp/themis_interactive_pdf")
MERMAID_CACHE = TEMP_DIR / "mermaid_svgs"
HTML_CACHE = TEMP_DIR / "mermaid_htmls"
QR_CACHE = TEMP_DIR / "qr_codes"

# GitHub Pages URL (adjust as needed)
GITHUB_PAGES_BASE = "https://makr-code.github.io/ThemisDB/compendium/interactive/"

# Chapters
CHAPTERS = [
    "preface.md",
    "chapter_01_introduction.md",
    "chapter_02_architecture.md",
    "chapter_06_graph.md",
]

def print_section(title):
    """Print formatted section header"""
    print("\n" + "=" * 70)
    print(f"  {title}")
    print("=" * 70 + "\n")

def check_mermaid_cli():
    """Check if mermaid-cli is available"""
    try:
        subprocess.run(["mmdc", "--version"], capture_output=True, check=True, timeout=5)
        print("  ✓ mermaid-cli (mmdc) found")
        return True
    except (FileNotFoundError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        print("  ⚠ mermaid-cli not found")
        print("    Install with: npm install -g @mermaid-js/mermaid-cli")
        return False

def convert_mermaid_to_svg(mermaid_code, output_file):
    """Convert Mermaid code to SVG"""
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

def create_interactive_html(mermaid_code, diagram_id, diagram_type):
    """Create standalone interactive HTML with Mermaid"""
    html_content = f'''<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Interaktives {diagram_type} - {diagram_id}</title>
    <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
    <style>
        * {{
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }}
        
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            padding: 20px;
        }}
        
        .header {{
            background: rgba(255, 255, 255, 0.95);
            padding: 20px;
            border-radius: 12px;
            margin-bottom: 20px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.2);
            text-align: center;
        }}
        
        .header h1 {{
            color: #7c4dff;
            margin: 0;
            font-size: 1.8em;
        }}
        
        .header p {{
            color: #666;
            margin: 5px 0 0 0;
        }}
        
        .diagram-container {{
            background: white;
            border-radius: 12px;
            padding: 30px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.25);
            flex: 1;
            display: flex;
            flex-direction: column;
            overflow: auto;
        }}
        
        .mermaid {{
            flex: 1;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 400px;
        }}
        
        .controls {{
            margin-top: 20px;
            display: flex;
            gap: 10px;
            justify-content: center;
            flex-wrap: wrap;
        }}
        
        button {{
            background: linear-gradient(135deg, #7c4dff 0%, #9c4dff 100%);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 8px;
            font-size: 14px;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 4px 12px rgba(124, 77, 255, 0.3);
        }}
        
        button:hover {{
            transform: translateY(-2px);
            box-shadow: 0 6px 16px rgba(124, 77, 255, 0.4);
        }}
        
        button:active {{
            transform: translateY(0);
        }}
        
        .info {{
            background: #f5f3ff;
            padding: 15px;
            border-radius: 8px;
            margin-top: 20px;
            border-left: 4px solid #7c4dff;
        }}
        
        .info strong {{
            color: #7c4dff;
        }}
    </style>
</head>
<body>
    <div class="header">
        <h1>📊 {diagram_type}</h1>
        <p>ThemisDB Compendium - Interactive Diagram Viewer</p>
        <p><strong>ID:</strong> {diagram_id}</p>
    </div>
    
    <div class="diagram-container">
        <div class="mermaid" id="diagram">
{mermaid_code}
        </div>
        
        <div class="controls">
            <button onclick="zoomIn()">🔍 Zoom In</button>
            <button onclick="zoomOut()">🔍 Zoom Out</button>
            <button onclick="resetZoom()">↺ Reset</button>
            <button onclick="toggleTheme()">🎨 Theme</button>
            <button onclick="downloadSVG()">💾 Download SVG</button>
        </div>
        
        <div class="info">
            <strong>💡 Interaktiv:</strong> Klicken Sie auf Elemente im Diagramm für Details. 
            Verwenden Sie die Buttons oben für Zoom und weitere Optionen.
        </div>
    </div>
    
    <script>
        let currentZoom = 1;
        let currentTheme = 'default';
        
        // Initialize Mermaid
        mermaid.initialize({{
            startOnLoad: true,
            theme: currentTheme,
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
            }}
        }});
        
        function zoomIn() {{
            currentZoom += 0.2;
            applyZoom();
        }}
        
        function zoomOut() {{
            currentZoom = Math.max(0.2, currentZoom - 0.2);
            applyZoom();
        }}
        
        function resetZoom() {{
            currentZoom = 1;
            applyZoom();
        }}
        
        function applyZoom() {{
            const diagram = document.getElementById('diagram');
            const svg = diagram.querySelector('svg');
            if (svg) {{
                svg.style.transform = `scale(${{currentZoom}})`;
                svg.style.transformOrigin = 'center center';
            }}
        }}
        
        function toggleTheme() {{
            currentTheme = currentTheme === 'default' ? 'dark' : 'default';
            mermaid.initialize({{
                startOnLoad: false,
                theme: currentTheme
            }});
            location.reload();
        }}
        
        function downloadSVG() {{
            const svg = document.querySelector('#diagram svg');
            if (svg) {{
                const svgData = new XMLSerializer().serializeToString(svg);
                const svgBlob = new Blob([svgData], {{type: 'image/svg+xml;charset=utf-8'}});
                const url = URL.createObjectURL(svgBlob);
                const link = document.createElement('a');
                link.href = url;
                link.download = '{diagram_id}.svg';
                link.click();
                URL.revokeObjectURL(url);
            }}
        }}
        
        // Add click interactivity to diagram elements
        document.addEventListener('DOMContentLoaded', function() {{
            setTimeout(function() {{
                const elements = document.querySelectorAll('#diagram svg g[class*="node"], #diagram svg g[class*="edgePath"]');
                elements.forEach(function(element) {{
                    element.style.cursor = 'pointer';
                    element.addEventListener('click', function(e) {{
                        const textElement = this.querySelector('text, span');
                        if (textElement) {{
                            const text = textElement.textContent;
                            alert('Element: ' + text);
                            console.log('Clicked:', text, this);
                        }}
                    }});
                    
                    element.addEventListener('mouseenter', function() {{
                        this.style.opacity = '0.8';
                    }});
                    
                    element.addEventListener('mouseleave', function() {{
                        this.style.opacity = '1';
                    }});
                }});
            }}, 1000);
        }});
    </script>
</body>
</html>'''
    
    return html_content

def generate_qr_code(url, size=100):
    """Generate QR code for URL"""
    qr = qrcode.QRCode(
        version=1,
        error_correction=qrcode.constants.ERROR_CORRECT_L,
        box_size=10,
        border=2,
    )
    qr.add_data(url)
    qr.make(fit=True)
    
    img = qr.make_image(fill_color="black", back_color="white")
    
    # Save to bytes
    buffer = BytesIO()
    img.save(buffer, format='PNG')
    buffer.seek(0)
    
    return buffer.read()

def extract_mermaid_diagrams(markdown_content):
    """Extract all Mermaid diagrams from markdown"""
    pattern = r'```mermaid\n(.*?)```'
    matches = re.findall(pattern, markdown_content, re.DOTALL)
    return matches

def detect_diagram_type(mermaid_code):
    """Detect Mermaid diagram type"""
    code = mermaid_code.strip()
    if code.startswith("graph"):
        return "Flowchart"
    elif code.startswith("sequenceDiagram"):
        return "Sequence Diagram"
    elif code.startswith("stateDiagram"):
        return "State Diagram"
    elif code.startswith("gantt"):
        return "Gantt Chart"
    elif code.startswith("erDiagram"):
        return "ER Diagram"
    elif code.startswith("flowchart"):
        return "Flowchart"
    elif code.startswith("classDiagram"):
        return "Class Diagram"
    elif code.startswith("journey"):
        return "User Journey"
    elif code.startswith("gitGraph"):
        return "Git Graph"
    elif code.startswith("quadrantChart"):
        return "Quadrant Chart"
    else:
        return "Diagram"

def main():
    """Main execution"""
    print_section("Interactive PDF Generator with Embedded Mermaid")
    
    # Create directories
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    TEMP_DIR.mkdir(parents=True, exist_ok=True)
    MERMAID_CACHE.mkdir(parents=True, exist_ok=True)
    HTML_CACHE.mkdir(parents=True, exist_ok=True)
    QR_CACHE.mkdir(parents=True, exist_ok=True)
    
    # Check dependencies
    mermaid_available = check_mermaid_cli()
    
    # Process chapters and extract diagrams
    print("\n📚 Processing chapters...")
    all_diagrams = []
    diagram_metadata = []
    
    for chapter_file in CHAPTERS:
        chapter_path = SCRIPT_DIR / chapter_file
        
        if not chapter_path.exists():
            print(f"  ⚠ Skipping (not found): {chapter_file}")
            continue
        
        print(f"  ✓ Processing: {chapter_file}")
        
        with open(chapter_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        diagrams = extract_mermaid_diagrams(content)
        
        for idx, diagram in enumerate(diagrams, 1):
            diagram_id = f"{chapter_path.stem}_diagram_{idx}"
            diagram_type = detect_diagram_type(diagram)
            
            all_diagrams.append({
                'id': diagram_id,
                'code': diagram,
                'type': diagram_type,
                'chapter': chapter_file
            })
            
            print(f"    - Found: {diagram_type} ({diagram_id})")
    
    print(f"\n✅ Total diagrams found: {len(all_diagrams)}")
    
    # Generate SVGs, HTMLs, and QR codes
    print("\n🎨 Generating interactive content...")
    
    for diagram in all_diagrams:
        diagram_id = diagram['id']
        mermaid_code = diagram['code']
        diagram_type = diagram['type']
        
        # Generate SVG if mermaid-cli available
        svg_file = MERMAID_CACHE / f"{diagram_id}.svg"
        if mermaid_available:
            if convert_mermaid_to_svg(mermaid_code, svg_file):
                diagram['svg_file'] = svg_file
                print(f"  ✓ SVG: {diagram_id}")
        
        # Generate interactive HTML
        html_content = create_interactive_html(mermaid_code, diagram_id, diagram_type)
        html_file = HTML_CACHE / f"{diagram_id}.html"
        with open(html_file, 'w', encoding='utf-8') as f:
            f.write(html_content)
        diagram['html_file'] = html_file
        print(f"  ✓ HTML: {diagram_id}")
        
        # Generate QR code
        diagram_url = f"{GITHUB_PAGES_BASE}{diagram_id}.html"
        qr_bytes = generate_qr_code(diagram_url)
        qr_file = QR_CACHE / f"{diagram_id}_qr.png"
        with open(qr_file, 'wb') as f:
            f.write(qr_bytes)
        diagram['qr_file'] = qr_file
        diagram['url'] = diagram_url
        print(f"  ✓ QR Code: {diagram_id}")
    
    # Save metadata
    metadata_file = TEMP_DIR / "diagrams_metadata.json"
    with open(metadata_file, 'w', encoding='utf-8') as f:
        json.dump(all_diagrams, f, indent=2, default=str)
    
    print(f"\n✅ Generated {len(all_diagrams)} interactive diagrams")
    print(f"   📄 SVGs: {MERMAID_CACHE}")
    print(f"   🌐 HTMLs: {HTML_CACHE}")
    print(f"   📱 QR Codes: {QR_CACHE}")
    print(f"   📋 Metadata: {metadata_file}")
    
    print("\n💡 Next steps:")
    print("   1. Upload HTML files to GitHub Pages")
    print("   2. Run PDF generation with embeddings")
    print("   3. Test in Adobe Reader / Preview")
    
    print(f"\n📂 Files ready for web hosting:")
    for html_file in HTML_CACHE.glob("*.html"):
        print(f"   - {html_file.name}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
