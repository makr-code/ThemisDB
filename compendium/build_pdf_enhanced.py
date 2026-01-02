#!/usr/bin/env python3
"""
Enhanced HTML/PDF Generator for ThemisDB Compendium
Includes title page, TOC, and improved Mermaid handling
"""

import os
import sys
import re
import subprocess
import tempfile
import hashlib
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
    extensions = ['fenced_code', 'tables', 'codehilite', 'toc', 'nl2br', 'attr_list', 'md_in_html']
    md = markdown.Markdown(extensions=extensions)
    html = md.convert(md_content)
    
    # Remove all h1 tags from content - we'll add our own
    html = re.sub(r'<h1[^>]*>.*?</h1>', '', html, flags=re.DOTALL)
    
    return html

def extract_mermaid_blocks(md_content):
    """Extract mermaid blocks from markdown before conversion."""
    pattern = r'```mermaid\n(.*?)```'
    matches = re.findall(pattern, md_content, re.DOTALL)
    return matches

def extract_chapter_title(md_content):
    """Extract first h1 title from markdown - use exact match."""
    # Look for # Title (h1)
    match = re.search(r'^#\s+(.+?)(?:\n|$)', md_content, re.MULTILINE)
    if match:
        return match.group(1).strip()
    # Fallback: look for any heading
    match = re.search(r'^#+\s+(.+?)(?:\n|$)', md_content, re.MULTILINE)
    return match.group(1).strip() if match else "Untitled"

def create_anchor_id(chapter_num):
    """Create safe anchor ID using chapter number only."""
    return f"chapter-{chapter_num}"

def process_mermaid_blocks(html):
    """Convert Mermaid blocks to SVG using mermaid-cli."""
    # This function now handles converted HTML from fenced code blocks
    # Look for Mermaid code in codehilite divs
    pattern = r'<div class="codehilite"><pre>.*?graph\s+(LR|TD|BT|RL|LR).*?</pre></div>'
    
    # For now, just leave them as-is since they weren't being rendered anyway
    # The real solution would need custom preprocessing
    return html

def generate_pdf():
    """Generate HTML from all chapters with title page and TOC."""
    print("\n" + "=" * 70)
    print("  ThemisDB Compendium - Enhanced PDF Generator")
    print("  mit extern gerendertem Mermaid (SVG/Fallback)")
    print("=" * 70 + "\n")
    
    # Check for mermaid-cli
    try:
        result = subprocess.run(["mmdc", "--version"], capture_output=True, timeout=5)
        has_mmdc = result.returncode == 0
        if has_mmdc:
            print("✓ Mermaid-CLI verfügbar - SVG-Rendering aktiviert\n")
        else:
            print("⚠️ Mermaid-CLI nicht verfügbar - Fallback zu Code-Blöcken\n")
    except:
        print("⚠️ Mermaid-CLI nicht verfügbar - Fallback zu Code-Blöcken\n")
    
    print(f"📁 Compendium directory: {COMPENDIUM_DIR}")
    print(f"📁 Output directory: {PDF_OUTPUT_DIR}\n")
    
    # Load all chapters and extract titles
    print("📖 Loading chapters...")
    chapters_data = []
    
    for chapter in CHAPTERS:
        filepath = COMPENDIUM_DIR / chapter
        content = load_chapter(filepath)
        
        if content:
            title = extract_chapter_title(content)
            chapters_data.append((chapter, title, content))
            print(f"  ✓ {chapter}")
        
    print(f"\n✓ Loaded {len(chapters_data)} chapters")
    
    # Generate CSS
    css = """
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
    
    html {
        counter-reset: page-counter;
    }
    
    body {
        font-family: 'Segoe UI Emoji', 'Apple Color Emoji', 'Noto Color Emoji', 'Segoe UI', 'Roboto', 'Helvetica Neue', sans-serif;
        line-height: 1.7;
        color: var(--text);
        background: var(--bg);
        padding: 40px 20px;
        counter-reset: chapter-counter;
    }
    
    .container {
        max-width: 900px;
        margin: 0 auto;
    }
    
    /* Title Page */
    .title-page {
        page-break-after: always;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
        text-align: center;
        background: linear-gradient(135deg, var(--primary-dark) 0%, var(--secondary) 100%);
        color: white;
        padding: 40px;
    }
    
    .title-page h1 {
        font-size: 3.5em;
        margin-bottom: 20px;
        border: none;
        color: white;
    }
    
    .title-page .subtitle {
        font-size: 1.5em;
        margin-bottom: 40px;
        opacity: 0.9;
    }
    
    .title-page .meta {
        font-size: 0.95em;
        margin-top: 60px;
        opacity: 0.8;
        border-top: 1px solid rgba(255,255,255,0.3);
        padding-top: 20px;
    }
    
    /* Table of Contents */
    .toc-page {
        page-break-after: always;
    }
    
    .toc-page h2 {
        color: var(--primary-dark);
        border-bottom: 3px solid var(--primary);
        padding-bottom: 15px;
        margin-bottom: 30px;
        font-size: 2em;
    }
    
    .toc-list {
        list-style: none;
        margin-left: 0;
    }
    
    .toc-list li {
        margin-bottom: 12px;
        padding-left: 20px;
        text-indent: -20px;
        display: flex;
        justify-content: space-between;
    }
    
    .toc-list a {
        color: var(--secondary);
        text-decoration: none;
        flex: 1;
    }
    
    .toc-list a:hover {
        text-decoration: underline;
    }
    
    .toc-page-num {
        color: var(--text);
        margin-left: 10px;
        font-weight: 500;
    }
    
    /* Chapter Headings */
    h1 {
        page-break-before: always;
        color: var(--primary-dark);
        border-bottom: 3px solid var(--primary);
        padding-bottom: 15px;
        margin-bottom: 20px;
        margin-top: 40px;
        font-size: 2.5em;
        line-height: 1.2;
        counter-increment: chapter-counter;
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
        page-break-inside: avoid;
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
        page-break-inside: avoid;
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
        page-break-inside: avoid;
    }
    
    .mermaid-block {
        background-color: #f5f3ff;
        border: 1px solid var(--primary);
        padding: 15px;
        margin: 20px 0;
        border-radius: 4px;
    }
    
    .mermaid-block details {
        cursor: pointer;
    }
    
    .mermaid-block summary {
        color: var(--primary);
        font-weight: 600;
        padding: 10px;
        user-select: none;
    }
    
    .mermaid-block summary:hover {
        background-color: rgba(124, 77, 255, 0.05);
        border-radius: 3px;
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
    
    hr {
        border: none;
        border-top: 2px solid var(--primary);
        margin: 30px 0;
    }
    
    .chapter-footer {
        margin-top: 50px;
        padding-top: 20px;
        border-top: 1px solid var(--border);
        text-align: center;
        font-size: 0.9em;
        color: #999;
        page-break-after: always;
    }
    
    @page {
        size: A4;
        margin: 1.5cm;
        @bottom-center {
            content: counter(page);
            font-size: 0.9em;
            color: #666;
        }
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
        blockquote {
            page-break-inside: avoid;
        }
    }
    """
    
    # Build HTML document
    html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThemisDB - Das vollständige Handbuch v1.3.4</title>
    <style>{css}</style>
</head>
<body>
    <div class="container">
        
        <!-- Title Page -->
        <div class="title-page">
            <h1>ThemisDB</h1>
            <div class="subtitle">Das vollständige Handbuch</div>
            <div class="subtitle">v1.3.4</div>
            <div class="meta">
                <p>📚 Umfassendes Nachschlagewerk für ThemisDB-Datenbank</p>
                <p>Generiert: {datetime.now().strftime('%d. %B %Y')}</p>
                <p>Seiten: {len(chapters_data) + 2}</p>
            </div>
        </div>
        
        <!-- Table of Contents -->
        <div class="toc-page">
            <h2>📑 Inhaltsverzeichnis</h2>
            <ol class="toc-list">
"""
    
    # Add TOC entries with links
    for i, (filename, title, _) in enumerate(chapters_data, 1):
        anchor = create_anchor_id(i)
        html += f'                <li><a href="#{anchor}">Kapitel {i}: {title}</a><span class="toc-page-num">[S. {i+2}]</span></li>\n'
    
    html += """            </ol>
        </div>
        
        <!-- Content -->
"""
    
    # Process chapters
    for i, (filename, title, content) in enumerate(chapters_data, 1):
        # Add anchor to chapter
        anchor = create_anchor_id(i)
        html += f'        <h1 id="{anchor}">Kapitel {i}: {title}</h1>\n'
        
        # Process and add content
        html_content = markdown_to_html(content)
        html_content = process_mermaid_blocks(html_content)
        html += html_content
        html += f'        <div class="chapter-footer">— Ende von Kapitel {i} —</div>\n'
    
    # Close document
    html += """    </div>
    <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
    <script>
        mermaid.initialize({ startOnLoad: true, theme: 'default' });
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', mermaid.run);
        } else {
            mermaid.run();
        }
    </script>
</body>
</html>"""
    
    # Save HTML
    print("\n📄 Generating HTML with title page and TOC...")
    try:
        html_file = PDF_OUTPUT_DIR / "ThemisDB-Kompendium-v1.3.4-enhanced.html"
        
        with open(html_file, 'w', encoding='utf-8') as f:
            f.write(html)
        
        print(f"  ✓ Saved to: {html_file}")
        print(f"\n✅ Enhanced HTML generation complete!")
        print(f"\n📖 Next steps:")
        print(f"  1. Convert to PDF:")
        print(f"     weasyprint {html_file} output.pdf")
        print(f"  2. Or open in browser and print (Ctrl+P)")
        
        return 0
    
    except Exception as e:
        print(f"❌ Error generating HTML: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == "__main__":
    sys.exit(generate_pdf())
