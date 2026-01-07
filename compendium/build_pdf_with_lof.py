#!/usr/bin/env python3
"""
HTML Generator for ThemisDB Compendium using Markdown
Converts all chapters to a single HTML file with professional styling
"""

import os
import sys
import re
from pathlib import Path
from datetime import datetime

import markdown
from markdown.extensions import Extension, tables, codehilite, toc

# Configuration
COMPENDIUM_DIR = Path(__file__).parent.absolute()
PDF_OUTPUT_DIR = COMPENDIUM_DIR / "pdf"
TEMP_DIR = COMPENDIUM_DIR / "temp"
PDF_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
TEMP_DIR.mkdir(parents=True, exist_ok=True)

# Updated chapter list with new numbering
CHAPTERS = [
    "preface.md",
    "chapter_00_genesis.md",
    "chapter_01_introduction.md",
    "chapter_02_architecture.md",
    "chapter_03_mvcc_timeline.md",
    "chapter_04_multimodel.md",
    "chapter_05_installation.md",
    "chapter_06_relational.md",
    "chapter_07_document.md",
    "chapter_08_graph.md",
    "chapter_09_storage_layer.md",
    "chapter_10_data_modeling_patterns.md",
    "chapter_11_vector.md",
    "chapter_12_timeseries.md",
    "chapter_13_aql_reference.md",
    "chapter_14_aql_oop_implementation.md",
    "chapter_15_query_optimization.md",
    "chapter_16_best_practices.md",
    "chapter_17_testing_qa.md",
    "chapter_18_fulltext.md",
    "chapter_19_geospatial.md",
    "chapter_20_analytics.md",
    "chapter_21_analytics_process_mining.md",
    "chapter_22_llm_integration.md",
    "chapter_23_ml.md",
    "chapter_24_computervision.md",
    "chapter_25_realtime.md",
    "chapter_26_enterprise.md",
    "chapter_27_sharding.md",
    "chapter_28_api_protocols.md",
    "chapter_29_clients.md",
    "chapter_30_ecosystem_integration.md",
    "chapter_31_monitoring.md",
    "chapter_32_monitoring_observability.md",
    "chapter_33_observability_sre.md",
    "chapter_34_performance.md",
    "chapter_35_performance_tuning_cookbook.md",
    "chapter_36_backup.md",
    "chapter_37_deployment_operations.md",
    "chapter_38_devops_infrastructure.md",
    "chapter_39_troubleshooting.md",
    "chapter_40_migration_legacy.md",
    "chapter_41_security_hardening.md",
    "chapter_42_data_governance_compliance.md",
    "chapter_43_ai_ethics.md",
    "chapter_44_hands_on_labs.md",
    "appendix_literatur.md",
    "appendix_d_feature_status.md",
    "appendix_e_incident_runbooks.md",
    "appendix_f_aql_cheatsheet.md",
    "appendix_g_configuration.md",
    "appendix_h_glossary.md",
    "appendix_i_troubleshooting.md",
]

def load_chapter(filepath):
    """Load chapter markdown content."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        print(f"⚠️ Warning: {filepath} not found, skipping")
        return None

def markdown_to_html(md_content):
    """Convert Markdown to HTML using Python-Markdown."""
    extensions = ['tables', 'codehilite', 'toc', 'nl2br', 'attr_list', 'md_in_html']
    md = markdown.Markdown(extensions=extensions)
    html = md.convert(md_content)
    return html

def process_mermaid_blocks(html):
    """Convert Mermaid blocks to code blocks (no rendering)."""
    # Replace mermaid blocks with info boxes instead of trying to render
    pattern = r'<pre><code class="language-mermaid">(.*?)</code></pre>'
    
    def replace_mermaid(match):
        code = match.group(1)
        return f'''
        <div style="background-color: #f0f0f0; border: 1px solid #ddd; padding: 10px; margin: 10px 0; border-radius: 4px;">
            <p style="color: #666; font-size: 0.9em; margin: 0 0 5px 0;"><strong>📊 Mermaid Diagram:</strong></p>
            <pre style="margin: 0;"><code>{code}</code></pre>
        </div>
        '''
    
    html = re.sub(pattern, replace_mermaid, html, flags=re.DOTALL)
    return html

def generate_pdf():
    """Generate HTML from all chapters."""
    print("\n" + "=" * 70)
    print("  ThemisDB Compendium HTML Generator")
    print("=" * 70 + "\n")
    
    print(f"📁 Compendium directory: {COMPENDIUM_DIR}")
    print(f"📁 Output directory: {PDF_OUTPUT_DIR}\n")
    
    # Load all chapters
    print("📖 Loading chapters...")
    all_html = """
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>ThemisDB - Das vollständige Handbuch v1.3.4</title>
        <style>
            :root {
                --primary: #7c4dff;
                --primary-dark: #1e3a5f;
                --secondary: #2c5aa0;
                --accent: #ff6b6b;
                --text: #333;
                --bg: #fff;
                --code-bg: #f4f4f4;
                --border: #ddd;
            }
            
            * {
                margin: 0;
                padding: 0;
                box-sizing: border-box;
            }
            
            body {
                font-family: 'Segoe UI Emoji', 'Apple Color Emoji', 'Noto Color Emoji', 'Segoe UI', 'Roboto', 'Helvetica Neue', sans-serif;
                line-height: 1.7;
                color: var(--text);
                background: var(--bg);
                padding: 40px 20px;
            }
            
            .container {
                max-width: 900px;
                margin: 0 auto;
            }
            
            h1 {
                page-break-before: always;
                color: var(--primary-dark);
                border-bottom: 3px solid var(--primary);
                padding-bottom: 15px;
                margin-bottom: 20px;
                margin-top: 40px;
                font-size: 2.5em;
                line-height: 1.2;
            }
            
            h2 {
                color: var(--secondary);
                margin-top: 30px;
                margin-bottom: 15px;
                font-size: 1.8em;
                border-left: 4px solid var(--primary);
                padding-left: 15px;
            }
            
            h3, h4, h5, h6 {
                color: #555;
                margin-top: 20px;
                margin-bottom: 10px;
            }
            
            p {
                margin-bottom: 15px;
                text-align: justify;
            }
            
            code {
                background-color: var(--code-bg);
                padding: 2px 6px;
                border-radius: 3px;
                font-family: 'Courier New', 'Monaco', monospace;
                font-size: 0.9em;
            }
            
            pre {
                background-color: var(--code-bg);
                padding: 15px;
                border-radius: 5px;
                overflow-x: auto;
                margin: 15px 0;
                border-left: 4px solid var(--primary);
                line-height: 1.4;
            }
            
            pre code {
                background: none;
                padding: 0;
                border-radius: 0;
            }
            
            table {
                border-collapse: collapse;
                width: 100%;
                margin: 15px 0;
                box-shadow: 0 1px 3px rgba(0,0,0,0.1);
            }
            
            table th, table td {
                border: 1px solid var(--border);
                padding: 12px 15px;
                text-align: left;
            }
            
            table th {
                background-color: var(--primary-dark);
                color: white;
                font-weight: 600;
            }
            
            table tr:nth-child(even) {
                background-color: #f9f9f9;
            }
            
            blockquote {
                border-left: 4px solid var(--primary);
                padding-left: 20px;
                margin-left: 0;
                margin-bottom: 15px;
                color: #666;
                font-style: italic;
            }
            
            .admonition {
                border-left: 4px solid var(--primary);
                padding: 15px;
                background-color: #f9f9f9;
                margin: 15px 0;
                border-radius: 4px;
            }
            
            .admonition-title {
                font-weight: 600;
                margin-bottom: 8px;
                color: var(--primary);
            }
            
            ul, ol {
                margin-left: 30px;
                margin-bottom: 15px;
            }
            
            li {
                margin-bottom: 8px;
            }
            
            a {
                color: var(--secondary);
                text-decoration: none;
                border-bottom: 1px solid transparent;
                transition: border-color 0.2s;
            }
            
            a:hover {
                border-bottom-color: var(--secondary);
            }
            
            img, svg {
                max-width: 100%;
                height: auto;
                margin: 15px 0;
                border-radius: 4px;
            }
            
            .toc {
                background: #f9f9f9;
                padding: 20px;
                border-radius: 4px;
                margin: 30px 0;
                border-left: 4px solid var(--primary);
            }
            
            .toc-title {
                font-weight: 600;
                margin-bottom: 15px;
                color: var(--primary);
            }
            
            .toc ul {
                list-style: none;
                margin-left: 0;
            }
            
            .toc li {
                margin-bottom: 5px;
            }
            
            .toc a {
                color: var(--secondary);
            }
            
            hr {
                border: none;
                border-top: 2px solid var(--primary);
                margin: 30px 0;
            }
            
            .mermaid-block {
                background-color: #f5f3ff;
                border: 1px solid var(--primary);
                padding: 15px;
                margin: 20px 0;
                border-radius: 4px;
            }
            
            .mermaid-label {
                color: var(--primary);
                font-size: 0.9em;
                font-weight: 600;
                margin-bottom: 10px;
            }
            
            @media print {
                body {
                    padding: 0;
                }
                h1 {
                    page-break-before: always;
                }
                pre {
                    page-break-inside: avoid;
                }
                table {
                    page-break-inside: avoid;
                }
            }
        </style>
    </head>
    <body>
        <div class="container">
"""
    
    loaded_chapters = 0
    skipped_chapters = 0
    
    for chapter in CHAPTERS:
        filepath = COMPENDIUM_DIR / chapter
        content = load_chapter(filepath)
        
        if content:
            html = markdown_to_html(content)
            html = process_mermaid_blocks(html)
            all_html += html
            loaded_chapters += 1
            print(f"  ✓ {chapter}")
        else:
            skipped_chapters += 1
    
    print(f"\n✓ Loaded {loaded_chapters} chapters")
    if skipped_chapters > 0:
        print(f"⚠️ Skipped {skipped_chapters} chapters")
    
    all_html += """
        </div>
        
        <!-- Mermaid diagram support -->
        <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
        <script>
            mermaid.contentLoaderCounter = 0;
            mermaid.initialize({ startOnLoad: true, theme: 'default' });
            mermaid.contentLoaderCounter = (mermaid.contentLoaderCounter || 0) + 1;
            if (document.readyState === 'loading') {
                document.addEventListener('DOMContentLoaded', mermaid.run);
            } else {
                mermaid.run();
            }
        </script>
    </body>
    </html>
    """
    
    # Generate HTML output
    print("\n📄 Generating HTML...")
    try:
        html_file = PDF_OUTPUT_DIR / "ThemisDB-Kompendium-v1.3.4.html"
        
        with open(html_file, 'w', encoding='utf-8') as f:
            f.write(all_html)
        
        print(f"  ✓ Saved to: {html_file}")
        print(f"\n✅ HTML generation complete!")
        print(f"\n📖 Next steps:")
        print(f"  1. Open in browser: {html_file}")
        print(f"  2. Print to PDF (Ctrl+P or File → Print)")
        print(f"  3. Save as PDF with custom options")
        print(f"  4. Or convert with: wkhtmltopdf, pandoc, or similar tools")
        print(f"\n📊 For better Mermaid SVG support:")
        print(f"  - Pandoc: pandoc -f html -t pdf --embed-resources {html_file} -o output.pdf")
        print(f"  - WeasyPrint: weasyprint {html_file} output.pdf")
        print(f"  - wkhtmltopdf: wkhtmltopdf {html_file} output.pdf")
        
        return 0
    
    except Exception as e:
        print(f"❌ Error generating HTML: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == "__main__":
    sys.exit(generate_pdf())
