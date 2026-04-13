"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_pdf_weasyprint.py                         ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:22:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     307                                            ║
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
Modern PDF Generator for ThemisDB Compendium
Uses WeasyPrint for better CSS support and professional typography
"""

import os
import sys
import subprocess
from datetime import datetime
from pathlib import Path

def main():
    print("=" * 50)
    print("ThemisDB Compendium PDF Generator (WeasyPrint)")
    print("=" * 50)
    print()
    
    # Configuration
    script_dir = Path(__file__).parent
    output_dir = script_dir.parent.parent / "pdf_output"
    output_file = output_dir / f"ThemisDB-Compendium-v1.3.4-{datetime.now().strftime('%Y%m%d')}.pdf"
    
    output_dir.mkdir(exist_ok=True)
    
    # Check dependencies
    print("📦 Checking dependencies...")
    try:
        import markdown
        import weasyprint
        print("  ✓ markdown library found")
        print("  ✓ weasyprint library found")
    except ImportError as e:
        print(f"❌ Missing dependency: {e}")
        print("\nInstall required packages:")
        print("  pip install markdown weasyprint pygments")
        sys.exit(1)
    
    # Chapter list
    chapters = [
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
        "appendix_d_feature_status.md",
    ]
    
    print(f"\n📚 Collecting {len(chapters)} chapters...")
    
    # Build HTML content
    html_parts = []
    html_parts.append("""
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThemisDB Compendium v1.3.4</title>
    <style>
        /* Import modern book styles */
        @page {
            size: A4;
            margin: 25mm 20mm 25mm 20mm;
            
            @top-left {
                content: "ThemisDB Compendium";
                font-family: "Helvetica Neue", Arial, sans-serif;
                font-size: 9pt;
                color: #666;
            }
            
            @top-right {
                content: "Version 1.3.4";
                font-family: "Helvetica Neue", Arial, sans-serif;
                font-size: 9pt;
                color: #666;
            }
            
            @bottom-center {
                content: "Seite " counter(page);
                font-family: "Helvetica Neue", Arial, sans-serif;
                font-size: 10pt;
                color: #333;
            }
        }
        
        body {
            font-family: Georgia, "Times New Roman", serif;
            font-size: 11pt;
            line-height: 1.65;
            color: #2c3e50;
            hyphens: auto;
        }
        
        h1, h2, h3, h4, h5, h6 {
            font-family: "Helvetica Neue", Arial, sans-serif;
            font-weight: 600;
            color: #1a1a1a;
            page-break-after: avoid;
            margin-top: 1.5em;
            margin-bottom: 0.75em;
            line-height: 1.3;
        }
        
        h1 {
            font-size: 28pt;
            font-weight: 700;
            border-bottom: 3px solid #7c4dff;
            padding-bottom: 0.3em;
            page-break-before: always;
            margin-top: 0;
        }
        
        h2 {
            font-size: 20pt;
            font-weight: 600;
            border-bottom: 2px solid #ddd;
            padding-bottom: 0.25em;
            margin-top: 2em;
        }
        
        h3 {
            font-size: 16pt;
            font-weight: 600;
            color: #7c4dff;
        }
        
        code {
            font-family: "Courier New", monospace;
            font-size: 9.5pt;
            background-color: #f8f8f8;
            border: 1px solid #e0e0e0;
            border-radius: 3px;
            padding: 2px 5px;
            color: #c7254e;
        }
        
        pre {
            background-color: #f8f8f8;
            border: 1px solid #ddd;
            border-left: 4px solid #7c4dff;
            border-radius: 4px;
            padding: 12px;
            margin: 1em 0;
            page-break-inside: avoid;
            overflow-x: auto;
        }
        
        pre code {
            font-size: 9pt;
            line-height: 1.5;
            background-color: transparent;
            border: none;
            padding: 0;
            color: #333;
        }
        
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 1em 0;
            page-break-inside: avoid;
            font-size: 10pt;
        }
        
        thead tr {
            background-color: #7c4dff;
            color: white;
            font-weight: 600;
        }
        
        th {
            padding: 10px;
            text-align: left;
            border-bottom: 2px solid #fff;
        }
        
        td {
            padding: 8px 10px;
            border-bottom: 1px solid #e0e0e0;
        }
        
        tbody tr:nth-child(even) {
            background-color: #f9f9f9;
        }
        
        blockquote {
            border-left: 5px solid #7c4dff;
            background-color: #f5f5f5;
            padding: 12px 20px;
            margin: 1em 0;
            font-style: italic;
            color: #555;
            page-break-inside: avoid;
        }
        
        .title-page {
            text-align: center;
            padding-top: 30%;
            page-break-after: always;
        }
        
        .title-page h1 {
            font-size: 36pt;
            border: none;
            page-break-before: avoid;
        }
        
        .title-page .subtitle {
            font-size: 18pt;
            color: #666;
            margin-top: 1em;
        }
        
        .title-page .version {
            font-size: 14pt;
            color: #999;
            margin-top: 2em;
        }
    </style>
</head>
<body>
    <div class="title-page">
        <h1>ThemisDB Compendium</h1>
        <p class="subtitle">Das vollständige technische Handbuch</p>
        <p class="version">Version 1.3.4</p>
        <p class="version">~135.500 Wörter | ~271 Seiten</p>
        <p class="version">Stand: """ + datetime.now().strftime('%d.%m.%Y') + """</p>
    </div>
""")
    
    # Convert and add each chapter
    md = markdown.Markdown(extensions=['extra', 'codehilite', 'tables', 'toc'])
    
    for chapter_file in chapters:
        chapter_path = script_dir / chapter_file
        if chapter_path.exists():
            print(f"  ✓ Processing: {chapter_file}")
            with open(chapter_path, 'r', encoding='utf-8') as f:
                content = f.read()
                html_content = md.convert(content)
                html_parts.append(html_content)
                html_parts.append('<div style="page-break-before: always;"></div>')
        else:
            print(f"  ⚠ Skipping (not found): {chapter_file}")
    
    html_parts.append("</body></html>")
    
    html_content = "\n".join(html_parts)
    
    print("\n🔨 Generating PDF with WeasyPrint...")
    print(f"  Output: {output_file}")
    
    try:
        weasyprint.HTML(string=html_content).write_pdf(str(output_file))
        
        file_size = output_file.stat().st_size / (1024 * 1024)  # MB
        print(f"\n✅ PDF generated successfully!")
        print(f"   📄 File: {output_file}")
        print(f"   📊 Size: {file_size:.2f} MB")
        print(f"   📋 Chapters: {len(chapters)}")
        print(f"   📅 Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print("\n" + "=" * 50)
        print("✨ PDF Generation Complete!")
        print("=" * 50)
        
    except Exception as e:
        print(f"\n❌ PDF generation failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
