#!/usr/bin/env python3
"""
ThemisDB Compendium - Professional Book PDF Generator
Optimiert für Druckausgabe mit professionellem Buchtheme
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

CHAPTERS = [
    "preface.md",
    "chapter_00_genesis.md", "chapter_01_introduction.md", "chapter_02_architecture.md",
    "chapter_03_mvcc_timeline.md", "chapter_04_multimodel.md", "chapter_05_installation.md",
    "chapter_06_relational.md", "chapter_07_document.md", "chapter_08_graph.md",
    "chapter_09_storage_layer.md", "chapter_10_data_modeling_patterns.md", "chapter_11_vector.md",
    "chapter_12_timeseries.md", "chapter_13_aql_reference.md", "chapter_14_aql_oop_implementation.md",
    "chapter_15_query_optimization.md", "chapter_16_best_practices.md", "chapter_17_testing_qa.md",
    "chapter_18_fulltext.md", "chapter_19_geospatial.md", "chapter_20_analytics.md",
    "chapter_21_analytics_process_mining.md", "chapter_22_llm_integration.md", "chapter_23_ml.md",
    "chapter_24_computervision.md", "chapter_25_realtime.md", "chapter_26_enterprise.md",
    "chapter_27_sharding.md", "chapter_28_api_protocols.md", "chapter_29_clients.md",
    "chapter_30_ecosystem_integration.md", "chapter_31_monitoring.md", "chapter_32_monitoring_observability.md",
    "chapter_33_observability_sre.md", "chapter_34_performance.md", "chapter_35_performance_tuning_cookbook.md",
    "chapter_36_backup.md", "chapter_37_deployment_operations.md", "chapter_38_devops_infrastructure.md",
    "chapter_39_troubleshooting.md", "chapter_40_migration_legacy.md", "chapter_41_security_hardening.md",
    "chapter_42_data_governance_compliance.md", "chapter_43_ai_ethics.md", "chapter_44_hands_on_labs.md",
    "appendix_literatur.md", "appendix_d_feature_status.md", "appendix_e_incident_runbooks.md",
    "appendix_f_aql_cheatsheet.md", "appendix_g_configuration.md", "appendix_h_glossary.md",
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

def extract_chapter_title(md_content):
    """Extract first h1 title from markdown."""
    match = re.search(r'^#\s+(.+?)(?:\n|$)', md_content, re.MULTILINE)
    if match:
        return match.group(1).strip()
    match = re.search(r'^#+\s+(.+?)(?:\n|$)', md_content, re.MULTILINE)
    return match.group(1).strip() if match else "Untitled"

def create_anchor_id(chapter_num):
    """Create safe anchor ID using chapter number only."""
    return f"chapter-{chapter_num}"

def markdown_to_html(md_content):
    """Convert Markdown to HTML using Python-Markdown."""
    # Remove h1 from content - we'll add our own with anchors
    md_content = re.sub(r'^#\s+.+?$', '', md_content, flags=re.MULTILINE)
    
    extensions = ['fenced_code', 'tables', 'codehilite', 'toc', 'nl2br', 'attr_list']
    md = markdown.Markdown(extensions=extensions)
    html = md.convert(md_content)
    
    return html

def generate_pdf():
    """Generate professional book-layout PDF."""
    print("\n" + "=" * 70)
    print("  ThemisDB Compendium - Professional Book PDF Generator")
    print("  Print-optimiertes Theme")
    print("=" * 70 + "\n")
    
    print(f"📁 Compendium directory: {COMPENDIUM_DIR}")
    print(f"📁 Output directory: {PDF_OUTPUT_DIR}\n")
    
    # Load chapters
    print("📖 Loading chapters...")
    chapters_data = []
    for chapter in CHAPTERS:
        filepath = COMPENDIUM_DIR / chapter
        content = load_chapter(filepath)
        if content:
            title = extract_chapter_title(content)
            chapters_data.append((chapter, title, content))
            print(f"  ✓ {chapter}")
    
    print(f"\n✓ Loaded {len(chapters_data)} chapters\n")
    
    # Professional book CSS
    css = """
    * { margin: 0; padding: 0; box-sizing: border-box; }
    
    html {
        font-size: 11pt;
        -webkit-print-color-adjust: exact;
        print-color-adjust: exact;
    }
    
    @page {
        size: A4;
        margin-top: 2cm;
        margin-bottom: 1.5cm;
        margin-left: 2cm;
        margin-right: 2cm;
        
        @bottom-center {
            content: counter(page);
            font-size: 10pt;
            color: #666;
        }
    }
    
    @page :first {
        @bottom-center { content: ""; }
    }
    
    body {
        font-family: 'Segoe UI', 'Roboto', 'Helvetica Neue', sans-serif;
        line-height: 1.5;
        color: #222;
        background: white;
        text-rendering: optimizeLegibility;
    }
    
    .container { max-width: 100%; }
    
    /* Title Page */
    .title-page {
        page-break-after: always;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;
        min-height: 29.7cm;
        text-align: center;
        background: linear-gradient(135deg, #1e3a5f 0%, #2c5aa0 100%);
        color: white;
        padding: 4cm 2cm;
    }
    
    .title-page h1 {
        font-size: 48pt;
        font-weight: 700;
        margin-bottom: 20pt;
        border: none;
        color: white;
        letter-spacing: 0.5pt;
    }
    
    .title-page .subtitle {
        font-size: 24pt;
        margin-bottom: 40pt;
        opacity: 0.95;
        font-weight: 300;
    }
    
    .title-page .meta {
        font-size: 11pt;
        margin-top: 80pt;
        opacity: 0.85;
        border-top: 1px solid rgba(255,255,255,0.3);
        padding-top: 30pt;
        line-height: 1.8;
    }
    
    /* TOC */
    .toc-page { page-break-after: always; }
    
    .toc-page h2 {
        font-size: 24pt;
        color: #1e3a5f;
        border-bottom: 2pt solid #7c4dff;
        padding-bottom: 12pt;
        margin-bottom: 20pt;
        margin-top: 0;
        font-weight: 700;
    }
    
    .toc-list {
        list-style: none;
        margin-left: 0;
        font-size: 10pt;
        line-height: 1.8;
    }
    
    .toc-list li {
        display: flex;
        justify-content: space-between;
        margin-bottom: 8pt;
        padding-bottom: 4pt;
        border-bottom: 1px dotted #ddd;
    }
    
    .toc-list a {
        color: #2c5aa0;
        text-decoration: none;
        flex: 1;
        padding-right: 10pt;
    }
    
    .toc-page-num {
        color: #666;
        font-weight: 500;
        text-align: right;
        min-width: 40pt;
    }
    
    /* Chapters */
    h1 {
        page-break-before: always;
        font-size: 20pt;
        font-weight: 700;
        color: #1e3a5f;
        border-bottom: 2pt solid #7c4dff;
        padding-bottom: 8pt;
        margin-bottom: 12pt;
        margin-top: 0;
        line-height: 1.2;
    }
    
    h2 {
        font-size: 14pt;
        font-weight: 700;
        color: #2c5aa0;
        margin-top: 12pt;
        margin-bottom: 8pt;
        border-left: 3pt solid #7c4dff;
        padding-left: 10pt;
        line-height: 1.3;
    }
    
    h3 {
        font-size: 12pt;
        font-weight: 700;
        color: #444;
        margin-top: 10pt;
        margin-bottom: 6pt;
        line-height: 1.3;
    }
    
    h4, h5, h6 {
        font-size: 11pt;
        font-weight: 600;
        color: #555;
        margin-top: 8pt;
        margin-bottom: 4pt;
        line-height: 1.3;
    }
    
    p {
        margin-bottom: 10pt;
        text-align: justify;
        font-size: 11pt;
        line-height: 1.6;
    }
    
    code {
        background-color: #f5f5f5;
        padding: 2pt 4pt;
        border-radius: 2pt;
        font-family: 'Courier New', monospace;
        font-size: 10pt;
        color: #d63384;
    }
    
    pre {
        background-color: #f9f9f9;
        border: 1pt solid #e0e0e0;
        border-left: 3pt solid #7c4dff;
        padding: 10pt;
        margin: 12pt 0;
        overflow-x: auto;
        font-family: 'Courier New', monospace;
        font-size: 9pt;
        line-height: 1.4;
        page-break-inside: avoid;
        border-radius: 2pt;
    }
    
    pre code {
        background: none;
        padding: 0;
        color: #222;
    }
    
    blockquote {
        border-left: 3pt solid #7c4dff;
        padding-left: 12pt;
        margin-left: 0;
        margin-bottom: 10pt;
        margin-top: 10pt;
        color: #666;
        font-style: italic;
        font-size: 10.5pt;
        page-break-inside: avoid;
    }
    
    table {
        border-collapse: collapse;
        width: 100%;
        margin: 12pt 0;
        font-size: 10pt;
        page-break-inside: avoid;
    }
    
    table th, table td {
        border: 1pt solid #ddd;
        padding: 6pt;
        text-align: left;
    }
    
    table th {
        background-color: #1e3a5f;
        color: white;
        font-weight: 600;
        padding: 8pt;
    }
    
    table tr:nth-child(even) {
        background-color: #f9f9f9;
    }
    
    ul, ol {
        margin-left: 20pt;
        margin-bottom: 10pt;
        margin-top: 6pt;
    }
    
    li {
        margin-bottom: 4pt;
        line-height: 1.5;
    }
    
    a {
        color: #2c5aa0;
        text-decoration: none;
    }
    
    hr {
        border: none;
        border-top: 1pt solid #ddd;
        margin: 20pt 0;
    }
    
    .chapter-footer {
        margin-top: 30pt;
        padding-top: 10pt;
        border-top: 1pt solid #ddd;
        text-align: center;
        font-size: 9pt;
        color: #999;
        page-break-after: always;
        margin-bottom: 0;
    }
    
    .codehilite {
        background-color: #f9f9f9;
        border: 1pt solid #e0e0e0;
        padding: 8pt;
        margin: 10pt 0;
        border-radius: 2pt;
        font-size: 9pt;
        line-height: 1.4;
        page-break-inside: avoid;
    }
    
    img {
        max-width: 100%;
        height: auto;
        margin: 12pt 0;
        border: 1pt solid #eee;
        border-radius: 2pt;
        page-break-inside: avoid;
    }
    
    @media print {
        * { box-shadow: none !important; text-shadow: none !important; }
        a { text-decoration: underline; }
        pre { page-break-inside: avoid; }
        h1, h2, h3, h4 { page-break-after: avoid; page-break-inside: avoid; }
        table { page-break-inside: avoid; }
        blockquote { page-break-inside: avoid; }
    }
    """
    
    # Build HTML
    print("📄 Generating professional book PDF HTML...")
    
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
            <div class="subtitle" style="font-size: 16pt; font-weight: 400; margin-bottom: 20pt;">v1.3.4</div>
            <div class="meta">
                <p>📚 Umfassendes Nachschlagewerk für ThemisDB-Datenbank</p>
                <p>Generiert: {datetime.now().strftime('%d. %B %Y')}</p>
                <p>Kapitel: {len(chapters_data)}</p>
            </div>
        </div>
        
        <!-- Table of Contents -->
        <div class="toc-page">
            <h2>📑 Inhaltsverzeichnis</h2>
            <ol class="toc-list">
"""
    
    for i, (_, title, _) in enumerate(chapters_data, 1):
        anchor = create_anchor_id(i)
        html += f'                <li><a href="#{anchor}">Kapitel {i}: {title}</a><span class="toc-page-num">{i+2}</span></li>\n'
    
    html += """            </ol>
        </div>
        
        <!-- Content -->
"""
    
    # Add chapters
    for i, (_, title, content) in enumerate(chapters_data, 1):
        anchor = create_anchor_id(i)
        html += f'        <h1 id="{anchor}">Kapitel {i}: {title}</h1>\n'
        html += markdown_to_html(content) + '\n'
        html += f'        <div class="chapter-footer"></div>\n'
    
    html += """    </div>
</body>
</html>"""
    
    # Save HTML
    html_file = PDF_OUTPUT_DIR / "ThemisDB-Kompendium-v1.3.4-print.html"
    with open(html_file, 'w', encoding='utf-8') as f:
        f.write(html)
    
    print(f"  ✓ Saved HTML: {html_file}")
    print(f"\n✅ HTML generation complete! ({len(html)/1024/1024:.1f} MB)")
    print(f"\n📖 Converting to PDF...")
    print(f"  Command: weasyprint {html_file} ThemisDB-Kompendium-v1.3.4-print.pdf")
    
    return 0

if __name__ == "__main__":
    sys.exit(generate_pdf())
