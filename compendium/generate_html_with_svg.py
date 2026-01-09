#!/usr/bin/env python3
"""
Generate HTML with embedded SVG Mermaid diagrams from Markdown.
Converts Markdown to HTML and embeds all SVG diagrams inline.
"""

import os
import re
from pathlib import Path
from datetime import datetime
import markdown
from markdown.extensions import toc, tables, codehilite, fenced_code

# Configuration
COMPENDIUM_DIR = Path(__file__).parent
PDF_OUTPUT_DIR = COMPENDIUM_DIR / ".." / ".." / "pdf_output"
SVG_OUTPUT_DIR = PDF_OUTPUT_DIR / "mermaid_svg"
HTML_OUTPUT_DIR = PDF_OUTPUT_DIR / "html"

# Chapters to include
CHAPTERS = [
    "index.md",
    "preface.md",
    "chapter_00_genesis.md",
    "chapter_01_introduction.md",
    "chapter_02_architecture.md",
    "chapter_03_multimodel.md",
    "chapter_04_installation.md",
    "chapter_05_relational.md",
    "chapter_06_graph.md",
    "chapter_07_document.md",
    "chapter_08_storage_layer.md",
    "chapter_08_vector.md",
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
    "appendix_literatur.md",
    "appendix_d_feature_status.md",
    "appendix_e_incident_runbooks.md",
    "appendix_f_aql_cheatsheet.md",
    "appendix_g_configuration.md",
    "appendix_h_glossary.md",
    "appendix_i_troubleshooting.md",
]

def embed_svg_images(html_content):
    """Replace SVG image references with embedded SVG content."""
    # Find all image references like ![...](mermaid_svg/chapter_*.svg)
    pattern = r'<img[^>]*src="mermaid_svg/([^"]+\.svg)"[^>]*alt="([^"]*)"[^>]*>'
    
    def replace_with_svg(match):
        svg_filename = match.group(1)
        alt_text = match.group(2)
        svg_path = SVG_OUTPUT_DIR / svg_filename
        
        if svg_path.exists():
            try:
                with open(svg_path, 'r', encoding='utf-8') as f:
                    svg_content = f.read()
                # Add title attribute to SVG for accessibility
                svg_content = svg_content.replace('<svg', f'<svg title="{alt_text}"', 1)
                # Wrap in a div with styling
                return f'<div class="diagram-container">{svg_content}</div>'
            except Exception as e:
                print(f"Warning: Could not embed SVG {svg_filename}: {e}")
                return match.group(0)  # Return original if error
        else:
            print(f"Warning: SVG file not found: {svg_path}")
            return match.group(0)  # Return original if file not found
    
    return re.sub(pattern, replace_with_svg, html_content)

def process_markdown_file(filepath):
    """Convert Markdown file to HTML with embedded SVGs."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Replace Mermaid blocks with image references
        # Pattern: ```mermaid\n...\n```
        pattern = r'```mermaid\n(.*?)\n```'
        
        def replace_mermaid(match):
            # For now, keep as is - they should already be converted
            return match.group(0)
        
        content = re.sub(pattern, replace_mermaid, content, flags=re.DOTALL)
        
        # Convert Markdown to HTML
        extensions = [
            'fenced_code',
            'tables',
            'toc',
            'codehilite',
            'extra',
        ]
        html = markdown.markdown(content, extensions=extensions)
        
        # Embed SVG images
        html = embed_svg_images(html)
        
        return html
    except Exception as e:
        print(f"Error processing {filepath}: {e}")
        return ""

def generate_html_document():
    """Generate HTML document with all chapters and embedded SVG diagrams."""
    print("\n" + "="*70)
    print("  ThemisDB Kompendium HTML Generator (Embedded SVG)")
    print("="*70)
    
    # Create output directories
    HTML_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    
    print("\n📖 Processing Markdown chapters...")
    print("-"*70)
    
    all_html = ""
    
    for chapter_file in CHAPTERS:
        chapter_path = COMPENDIUM_DIR / chapter_file
        if not chapter_path.exists():
            print(f"⚠️  Skipped (not found): {chapter_file}")
            continue
        
        print(f"📄 Processing: {chapter_file}")
        chapter_html = process_markdown_file(chapter_path)
        all_html += f"\n<!-- ========== {chapter_file} ========== -->\n"
        all_html += chapter_html
        all_html += "\n"
    
    # Generate complete HTML document
    html_document = f"""<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThemisDB Kompendium - v1.4.0</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: 'Georgia', 'Noto Serif', serif;
            line-height: 1.65;
            color: #333;
            background-color: #fafafa;
            max-width: 900px;
            margin: 0 auto;
            padding: 40px 20px;
        }}
        
        h1 {{
            color: #7c4dff;
            border-bottom: 4px solid #7c4dff;
            padding-bottom: 15px;
            margin-top: 50px;
            margin-bottom: 20px;
            font-size: 2.5em;
        }}
        
        h2 {{
            color: #43e97b;
            border-left: 4px solid #43e97b;
            padding-left: 15px;
            margin-top: 40px;
            margin-bottom: 15px;
            font-size: 2em;
        }}
        
        h3 {{
            color: #4facfe;
            margin-top: 30px;
            margin-bottom: 10px;
            font-size: 1.5em;
        }}
        
        h4 {{
            color: #555;
            margin-top: 20px;
            margin-bottom: 10px;
            font-size: 1.2em;
        }}
        
        p {{
            margin-bottom: 15px;
            text-align: justify;
        }}
        
        code {{
            background-color: #f5f3ff;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: 'Fira Code', 'Courier New', monospace;
            font-size: 0.9em;
            color: #d63384;
        }}
        
        pre {{
            background-color: #f5f3ff;
            border: 1px solid #e0d9ff;
            border-radius: 5px;
            padding: 15px;
            overflow-x: auto;
            margin: 15px 0;
            line-height: 1.4;
        }}
        
        pre code {{
            background-color: transparent;
            padding: 0;
            color: #333;
        }}
        
        blockquote {{
            border-left: 4px solid #7c4dff;
            padding: 15px 20px;
            margin: 15px 0;
            background-color: #f9f7ff;
            font-style: italic;
            color: #555;
        }}
        
        table {{
            border-collapse: collapse;
            width: 100%;
            margin: 20px 0;
        }}
        
        table th {{
            background-color: #7c4dff;
            color: white;
            padding: 12px;
            text-align: left;
            font-weight: bold;
        }}
        
        table td {{
            border: 1px solid #ddd;
            padding: 12px;
        }}
        
        table tr:nth-child(even) {{
            background-color: #f9f9f9;
        }}
        
        table tr:hover {{
            background-color: #f0e6ff;
        }}
        
        .diagram-container {{
            margin: 30px 0;
            padding: 20px;
            background-color: white;
            border: 1px solid #e0e0e0;
            border-radius: 5px;
            text-align: center;
            display: flex;
            justify-content: center;
        }}
        
        .diagram-container svg {{
            max-width: 100%;
            height: auto;
            display: inline-block;
        }}
        
        ul, ol {{
            margin: 15px 0 15px 30px;
        }}
        
        li {{
            margin-bottom: 8px;
        }}
        
        a {{
            color: #7c4dff;
            text-decoration: none;
            border-bottom: 1px dotted #7c4dff;
        }}
        
        a:hover {{
            background-color: #f5f3ff;
        }}
        
        .toc {{
            background-color: #f5f3ff;
            border: 1px solid #e0d9ff;
            border-radius: 5px;
            padding: 20px;
            margin: 20px 0;
        }}
        
        .toc ul {{
            margin: 10px 0 10px 20px;
        }}
        
        .toc li {{
            list-style-type: none;
            margin: 5px 0;
        }}
        
        @media print {{
            body {{
                background-color: white;
                padding: 0;
            }}
            
            .diagram-container {{
                page-break-inside: avoid;
            }}
            
            h1, h2, h3 {{
                page-break-after: avoid;
            }}
            
            p {{
                orphans: 3;
                widows: 3;
            }}
        }}
    </style>
</head>
<body>
    <h1>🗄️ ThemisDB Kompendium</h1>
    <p><strong>Version:</strong> 1.4.0 | <strong>Erstellt:</strong> {datetime.now().strftime('%d. %B %Y')}</p>
    
    {all_html}
    
    <hr style="margin-top: 50px; border: none; border-top: 2px solid #7c4dff;">
    <p style="text-align: center; margin-top: 30px; color: #999; font-size: 0.9em;">
        © 2026 ThemisDB | HTML mit eingebetteten SVG-Diagrammen
    </p>
</body>
</html>
"""
    
    # Save HTML file
    date_str = datetime.now().strftime("%Y%m%d")
    html_filename = f"ThemisDB-Kompendium-svg-{date_str}.html"
    html_path = HTML_OUTPUT_DIR / html_filename
    
    with open(html_path, 'w', encoding='utf-8') as f:
        f.write(html_document)
    
    file_size_mb = html_path.stat().st_size / (1024 * 1024)
    svg_count = len(list(SVG_OUTPUT_DIR.glob("*.svg")))
    
    print("\n" + "="*70)
    print("✅ HTML successfully generated!")
    print("="*70)
    print(f"📄 Output: {html_path}")
    print(f"📊 Size: {file_size_mb:.2f} MB")
    print(f"🎨 Embedded SVG diagrams: {svg_count}")
    print("\n📖 Öffne die HTML-Datei im Browser oder konvertiere sie zu PDF:")
    print(f"   • Mit Chrome: chrome --headless --print-to-pdf={html_path.with_suffix('.pdf')} {html_path}")
    print(f"   • Mit Firefox: firefox --headless --print-to-file --outfile={html_path.with_suffix('.pdf')} {html_path}")

if __name__ == "__main__":
    generate_html_document()
