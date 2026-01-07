#!/usr/bin/env python3
"""
ThemisDB PDF - Schnell-Generator
Vereinfachte Version für schnelle PDF-Generierung
"""

import sys, re, subprocess, hashlib, base64
from pathlib import Path
from datetime import datetime
import markdown

COMPENDIUM_DIR = Path(__file__).parent
PDF_OUTPUT_DIR = COMPENDIUM_DIR / "pdf"
TEMP_DIR = COMPENDIUM_DIR / "temp"
TEMP_DIR.mkdir(exist_ok=True)

# Dynamisch alle Kapitel und Anhänge sammeln - mit korrekter Sortierung
def sort_chapters(files):
    """Sortiere: preface, chapter_XX (numerisch), appendix_XX"""
    chapters = []
    appendix = []
    for f in files:
        if "appendix" in f.name:
            appendix.append(f)
        else:
            chapters.append(f)
    
    # Sortiere numerisch nach Nummer
    chapters.sort(key=lambda p: int(p.name.split('_')[1]) if p.name.startswith('chapter_') else 0)
    appendix.sort(key=lambda p: p.name)
    return chapters + appendix

CHAPTERS = ["preface.md"] + [str(p) for p in sort_chapters(
    list((COMPENDIUM_DIR.glob("chapter_*.md"))) + list((COMPENDIUM_DIR.glob("appendix_*.md")))
)]

def render_mermaid_to_png(mermaid_code, output_path):
    """Rendere Mermaid-Code zu PNG mit mermaid-cli."""
    try:
        # Schreibe Mermaid-Code in temp-Datei
        mmd_file = TEMP_DIR / "temp.mmd"
        with open(mmd_file, 'w', encoding='utf-8') as f:
            f.write(mermaid_code)
        
        # Versuche lokalen mmdc, dann globalen
        mmdc_paths = [
            COMPENDIUM_DIR / "node_modules" / ".bin" / "mmdc",
            "mmdc"  # Global
        ]
        
        for mmdc_path in mmdc_paths:
            try:
                result = subprocess.run(
                    [str(mmdc_path), '-i', str(mmd_file), '-o', str(output_path), '-b', 'transparent'],
                    capture_output=True, timeout=10, cwd=str(COMPENDIUM_DIR)
                )
                if result.returncode == 0:
                    return True
            except (FileNotFoundError, subprocess.SubprocessError):
                continue
        
        return False
    except Exception as e:
        print(f"⚠️  Mermaid-Fehler: {e}")
        return False

def clean_md(content):
    """Remove h1, convert mermaid blocks to PNG with Base64 embedding."""
    content = re.sub(r'^#\s+.+?$\n?', '', content, flags=re.MULTILINE)
    
    # Finde und ersetze Mermaid-Blöcke
    mermaid_counter = [0]
    def replace_mermaid(match):
        mermaid_code = match.group(1)
        mermaid_counter[0] += 1
        
        # Erzeuge eindeutigen Dateinamen
        code_hash = hashlib.md5(mermaid_code.encode()).hexdigest()[:8]
        png_file = TEMP_DIR / f"mermaid_{mermaid_counter[0]}_{code_hash}.png"
        
        # Rendere zu PNG
        if not png_file.exists():
            if render_mermaid_to_png(mermaid_code, png_file):
                print(f"  ✓ Mermaid → PNG: {png_file.name}")
            else:
                # Fallback: zeige Code-Block
                return f"\n```\n{mermaid_code}\n```\n"
        
        # Lese PNG und konvertiere zu Base64
        with open(png_file, 'rb') as f:
            png_data = base64.b64encode(f.read()).decode('utf-8')
        
        # Gebe Data-URL zurück
        return f'\n<img src="data:image/png;base64,{png_data}" alt="Mermaid Diagram" style="max-width:100%; height:auto; margin:10pt 0;" />\n'
    
    content = re.sub(r'```mermaid\n(.*?)```', replace_mermaid, content, flags=re.DOTALL)
    return content

def get_title(content):
    """Extract title."""
    match = re.search(r'^#\s+(.+?)(?:\n|$)', content, re.MULTILINE)
    return match.group(1).strip() if match else "Chapter"

print("\n" + "="*60)
print("  ThemisDB PDF - Schnell-Generator")
print("="*60 + "\n")

chapters = []
for ch in CHAPTERS:
    ch_path = COMPENDIUM_DIR / ch if not str(ch).startswith("/") else Path(ch)
    if ch_path.exists():
        with open(ch_path, 'r', encoding='utf-8') as f:
            c = f.read()
        title = get_title(c)
        chapters.append((title, clean_md(c)))
        print(f"✓ {ch_path.name}")

print(f"\n✓ Loaded {len(chapters)} chapters\n")

md_converter = markdown.Markdown(extensions=['tables', 'codehilite', 'fenced_code'])

html = """<!DOCTYPE html><html><head><meta charset="utf-8">
<title>ThemisDB v1.3.4</title>
<style>
@page { size: A4; margin: 2cm; @bottom-center { content: counter(page); font-size: 9pt; } }
@page :first { @bottom-center { content: ""; } }
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: 'Segoe UI', 'Roboto', 'Helvetica Neue', sans-serif; line-height: 1.5; color: #222; font-size: 11pt; }
.container { max-width: 100%; }
.title-page { page-break-after: always; display: flex; flex-direction: column; justify-content: center; align-items: center; min-height: 29.7cm; text-align: center; background: linear-gradient(135deg, #1e3a5f 0%, #2c5aa0 100%); color: white; padding: 4cm 2cm; }
.title-page h1 { font-size: 48pt; font-weight: 700; margin-bottom: 20pt; border: none; color: white; }
.title-page .subtitle { font-size: 18pt; margin-bottom: 40pt; opacity: 0.95; }
.title-page .meta { font-size: 11pt; margin-top: 80pt; opacity: 0.85; border-top: 1px solid rgba(255,255,255,0.3); padding-top: 30pt; line-height: 1.8; }
.toc-page { page-break-after: always; }
.toc-page h2 { font-size: 20pt; color: #1e3a5f; border-bottom: 2pt solid #7c4dff; padding-bottom: 12pt; margin-bottom: 20pt; margin-top: 0; font-weight: 700; }
.toc-list { list-style: none; margin-left: 0; font-size: 10pt; line-height: 1.8; }
.toc-list li { display: flex; justify-content: space-between; margin-bottom: 6pt; padding-bottom: 4pt; border-bottom: 1px dotted #ddd; }
.toc-list a { color: #2c5aa0; text-decoration: none; flex: 1; padding-right: 10pt; }
.toc-page-num { color: #666; font-weight: 500; text-align: right; min-width: 35pt; }
h1 { page-break-before: always; font-size: 18pt; font-weight: 700; color: #1e3a5f; border-bottom: 2pt solid #7c4dff; padding-bottom: 8pt; margin-bottom: 12pt; margin-top: 0; line-height: 1.2; }
h2 { font-size: 13pt; font-weight: 700; color: #2c5aa0; margin-top: 10pt; margin-bottom: 6pt; border-left: 3pt solid #7c4dff; padding-left: 10pt; }
h3 { font-size: 11pt; font-weight: 700; color: #444; margin-top: 8pt; margin-bottom: 4pt; }
h4, h5, h6 { font-size: 10.5pt; font-weight: 600; color: #555; margin-top: 6pt; margin-bottom: 2pt; }
p { margin-bottom: 8pt; text-align: justify; line-height: 1.6; }
code { background: #f5f5f5; padding: 2pt 4pt; border-radius: 2pt; font-family: 'Courier New', monospace; font-size: 9.5pt; color: #d63384; }
pre { background: #f9f9f9; border: 1pt solid #e0e0e0; border-left: 3pt solid #7c4dff; padding: 8pt; margin: 10pt 0; font-family: 'Courier New', monospace; font-size: 8.5pt; line-height: 1.4; page-break-inside: avoid; }
pre code { background: none; padding: 0; color: #222; }
blockquote { border-left: 3pt solid #7c4dff; padding-left: 10pt; margin: 8pt 0; color: #666; font-style: italic; font-size: 10pt; page-break-inside: avoid; }
table { border-collapse: collapse; width: 100%; margin: 10pt 0; font-size: 9pt; page-break-inside: avoid; }
table th, table td { border: 1pt solid #ddd; padding: 5pt; text-align: left; }
table th { background: #1e3a5f; color: white; font-weight: 600; }
table tr:nth-child(even) { background: #f9f9f9; }
ul, ol { margin-left: 20pt; margin-bottom: 8pt; margin-top: 4pt; }
li { margin-bottom: 2pt; line-height: 1.5; }
a { color: #2c5aa0; text-decoration: none; }
.chapter-footer { margin-top: 20pt; padding-top: 8pt; border-top: 1pt solid #ddd; text-align: center; font-size: 8pt; color: #999; page-break-after: always; }
img { max-width: 100%; height: auto; margin: 10pt 0; border: 1pt solid #eee; page-break-inside: avoid; }
</style></head><body><div class="container">
<div class="title-page">
<h1>ThemisDB</h1>
<div class="subtitle">Das vollständige Handbuch</div>
<div class="subtitle" style="font-size: 14pt; font-weight: 400; margin-bottom: 20pt;">v1.3.4</div>
<div class="meta">
<p>Umfassendes Nachschlagewerk für ThemisDB-Datenbank</p>
<p>Generiert: """ + datetime.now().strftime('%d. %B %Y') + """</p>
<p>Kapitel: """ + str(len(chapters)) + """</p>
</div>
</div>
<div class="toc-page">
<h2>Inhaltsverzeichnis</h2>
<ol class="toc-list">
"""

for i, (title, _) in enumerate(chapters, 1):
    html += f'<li><a href="#c{i}">{i}. {title[:50]}</a><span class="toc-page-num">{i+2}</span></li>\n'

html += """</ol></div>
"""

for i, (title, content) in enumerate(chapters, 1):
    html += f'<h1 id="c{i}">Kapitel {i}: {title}</h1>\n'
    html += md_converter.convert(content) + '\n'
    md_converter.reset()
    html += '<div class="chapter-footer"></div>\n'

html += """</div></body></html>"""

output_file = PDF_OUTPUT_DIR / "ThemisDB-Kompendium-v1.3.4-print.html"
with open(output_file, 'w', encoding='utf-8') as f:
    f.write(html)

size_mb = len(html) / 1024 / 1024
print(f"✓ HTML generated: {size_mb:.1f} MB")
print(f"✓ Saved to: {output_file}")
print(f"\n✅ Done! Convert with:")
print(f"   weasyprint {output_file} ThemisDB-Kompendium-v1.3.4-print.pdf")
