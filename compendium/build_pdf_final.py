#!/usr/bin/env python3
"""
ThemisDB PDF - Mermaid für Buchdruck optimiert
- SVG extern mit Mermaid-Config für Print
- Max 4 Objekte pro Zeile
- Bessere Lesbarkeit
- Robuste Fehlerbehandlung
- UPDATED: Improved page layout with widow/orphan control, running headers, 
  and professional typography (v1.3.4)
"""

import sys, re, subprocess, base64, time, os
from pathlib import Path
from datetime import datetime
import markdown
import json

COMPENDIUM_DIR = Path(__file__).parent
PDF_OUTPUT_DIR = COMPENDIUM_DIR / "pdf"
TEMP_DIR = COMPENDIUM_DIR / "temp"
TEMP_DIR.mkdir(exist_ok=True)

# Mermaid-Konfiguration für Buchdruck
MERMAID_CONFIG = {
    "theme": "default",
    "flowchart": {
        "useMaxWidth": True,
        "rankSpacing": 50,
        "nodeSpacing": 50,
        "htmlLabels": True,
        "curve": "linear"
    },
    "sequence": {
        "useMaxWidth": True,
        "mirrorActors": True,
        "messageAlign": "center"
    },
    "gantt": {
        "useMaxWidth": True
    },
    "fontSize": 14,
    "fontFamily": "arial"
}

rendered_count = [0]
failed_count = [0]
MERMAID_PATTERN = re.compile(r'```[ \t]*mermaid\r?\n(.*?)```[ \t]*', re.DOTALL | re.IGNORECASE)
mermaid_global_id = [0]
figure_list = []  # Liste aller Abbildungen (id, caption, chapter)
table_list = []   # Liste aller Tabellen


def render_mermaid(mermaid_code, output_path, fmt: str):
    """Render Mermaid via mmdc into given format (svg|png)."""
    try:
        full_mermaid = f"""%%{{init: {json.dumps(MERMAID_CONFIG)}}}%%
{mermaid_code}"""

        mmd_file = TEMP_DIR / "temp.mmd"
        with open(mmd_file, "w", encoding="utf-8") as f:
            f.write(full_mermaid)

        mmdc_exe = "mmdc.cmd" if os.name == "nt" else "mmdc"
        mmdc_path = COMPENDIUM_DIR / "node_modules" / ".bin" / mmdc_exe
        if not mmdc_path.exists():
            # Fallback to PATH-installed mmdc
            mmdc_path = mmdc_exe
        cmd = [
            str(mmdc_path),
            "-i",
            str(mmd_file),
            "-o",
            str(output_path),
            "-b",
            "white",
            "--outputFormat",
            fmt,
            "-w",
            "1100",
            "-H",
            "800",
            "--pdfFit"
        ]

        result = subprocess.run(
            cmd,
            capture_output=True,
            timeout=30,
            cwd=str(COMPENDIUM_DIR),
        )

        ok = result.returncode == 0 and output_path.exists() and output_path.stat().st_size > 500
        if ok:
            rendered_count[0] += 1
            return True, None

        failed_count[0] += 1
        err_msg = result.stderr.decode(errors="ignore")[:120] if result.stderr else "unknown"
        return False, err_msg
    except subprocess.TimeoutExpired:
        failed_count[0] += 1
        return False, "TIMEOUT (>30s)"
    except Exception as e:
        failed_count[0] += 1
        return False, str(e)[:120]

def clean_md(content, chapter_title=""):
    """Remove h1, convert mermaid blocks; try SVG, fallback PNG, else code."""
    content = re.sub(r'^#\s+.+?$\r?\n?', '', content, flags=re.MULTILINE)

    def replace_mermaid(match):
        mermaid_code = match.group(1)
        mermaid_global_id[0] += 1
        diagram_id = mermaid_global_id[0]

        # Diagram-Typ erkennen
        lines = mermaid_code.strip().split('\n')
        diagram_type = lines[0].split()[0].lower() if lines else "mermaid"
        
        # Kontext vor Diagramm für Caption extrahieren
        match_start = match.start()
        context_before = content[max(0, match_start-300):match_start]
        
        # Versuche Heading vor Diagramm zu finden
        heading_match = re.findall(r'#+\s+(.+?)$', context_before, re.MULTILINE)
        context_caption = heading_match[-1].strip() if heading_match else ""
        
        # Caption generieren
        type_map = {
            'graph': 'Architekturdiagramm',
            'flowchart': 'Ablaufdiagramm',
            'sequencediagram': 'Sequenzdiagramm',
            'erdiagram': 'Entitäts-Beziehungsdiagramm',
            'gantt': 'Gantt-Diagramm',
            'statediagram-v2': 'Zustandsdiagramm',
            'quadrantchart': 'Quadranten-Diagramm'
        }
        type_name = type_map.get(diagram_type, diagram_type.capitalize())
        
        if context_caption:
            caption = f"{type_name}: {context_caption}"
        else:
            caption = f"{type_name} ({chapter_title})"

        png_file = TEMP_DIR / f"mermaid_{diagram_id}.png"
        svg_file = TEMP_DIR / f"mermaid_{diagram_id}.svg"

        svg_ok = svg_file.exists() and svg_file.stat().st_size >= 200
        png_ok = False

        # Render SVG first (gewünscht). Fallback PNG nur bei Bedarf.
        if not svg_ok:
            if svg_file.exists():
                svg_file.unlink()
            sys.stdout.write(f"  [{diagram_type}] -> SVG ... ")
            sys.stdout.flush()
            ok, err = render_mermaid(mermaid_code, svg_file, "svg")
            if ok:
                svg_ok = True
                print("[OK]", flush=True)
            else:
                print(f"[FAIL] ({err})", flush=True)

        # Fallback: PNG, falls SVG fehlgeschlagen ist
        if not svg_ok:
            if png_file.exists():
                png_file.unlink()
            sys.stdout.write(f"  [{diagram_type}] -> PNG ... ")
            sys.stdout.flush()
            ok, err = render_mermaid(mermaid_code, png_file, "png")
            if ok:
                png_ok = True
                print("[OK]", flush=True)
            else:
                print(f"[FAIL] ({err})", flush=True)
                if png_file.exists():
                    png_file.unlink()
                return f'\n```mermaid\n{mermaid_code}\n```\n'

        if svg_ok and svg_file.exists() and svg_file.stat().st_size >= 200:
            with open(svg_file, 'r', encoding='utf-8') as f:
                svg_content = f.read()
            svg_content = re.sub(r'<\?xml[^?]*\?>\s*', '', svg_content)
            svg_b64 = base64.b64encode(svg_content.encode()).decode()
            full_caption = f"Abb. {diagram_id}: {caption}"
            figure_list.append((diagram_id, caption, chapter_title))
            return f'''
<figure id="fig{diagram_id}" class="figure-container">
  <img src="data:image/svg+xml;base64,{svg_b64}" alt="{full_caption}" class="figure-image" />
  <figcaption class="figure-caption">{full_caption}</figcaption>
</figure>
'''

        if png_ok and png_file.exists() and png_file.stat().st_size >= 200:
            with open(png_file, 'rb') as f:
                png_content = f.read()
            png_b64 = base64.b64encode(png_content).decode()
            full_caption = f"Abb. {diagram_id}: {caption}"
            figure_list.append((diagram_id, caption, chapter_title))
            return f'''
<figure id="fig{diagram_id}" class="figure-container">
  <img src="data:image/png;base64,{png_b64}" alt="{full_caption}" class="figure-image" />
  <figcaption class="figure-caption">{full_caption}</figcaption>
</figure>
'''

                # As last resort, keep code block
        return f'\n```mermaid\n{mermaid_code}\n```\n'
        
    content = MERMAID_PATTERN.sub(replace_mermaid, content)
    return content

def get_title(content):
    """Extract title."""
    match = re.search(r'^#\s+(.+?)(?:\n|$)', content, re.MULTILINE)
    return match.group(1).strip() if match else "Chapter"

def sort_chapters(files):
    """Sortiere: preface, chapter_XX (numerisch), appendix_XX"""
    chapters = []
    appendix = []
    for f in files:
        if "appendix" in f.name:
            appendix.append(f)
        else:
            chapters.append(f)
    
    chapters.sort(key=lambda p: int(p.name.split('_')[1]) if p.name.startswith('chapter_') else 0)
    appendix.sort(key=lambda p: p.name)
    return chapters + appendix

CHAPTERS = ["preface.md"] + [str(p) for p in sort_chapters(
    list((COMPENDIUM_DIR.glob("chapter_*.md"))) + list((COMPENDIUM_DIR.glob("appendix_*.md")))
)]

print("\n" + "="*70)
print("  ThemisDB PDF - Mermaid Print-Optimiert (Robust)")
print("="*70 + "\n")

chapters = []
for ch in CHAPTERS:
    ch_path = COMPENDIUM_DIR / ch if not str(ch).startswith("/") else Path(ch)
    if ch_path.exists():
        with open(ch_path, 'r', encoding='utf-8') as f:
            c = f.read()
        title = get_title(c)
        block_count = len(MERMAID_PATTERN.findall(c))
        print(f"[LOAD] {ch_path.name}...", flush=True)
        if block_count:
            print(f"    [Mermaid-Blocks]: {block_count}")
        chapters.append((title, clean_md(c, title)))

print(f"\n[OK] Loaded {len(chapters)} chapters")
print(f"[OK] Diagramme: {rendered_count[0]} erfolgreich, {failed_count[0]} fehler\n")

md_converter = markdown.Markdown(extensions=['tables', 'codehilite', 'fenced_code'])

html = """<!DOCTYPE html><html><head><meta charset="utf-8">
<title>ThemisDB v1.3.4</title>
<style>
/* Improved page setup with widow/orphan control */
@page { 
    size: A4; 
    margin: 2.5cm 2cm 2cm 2cm; 
    @top-center {
        content: "ThemisDB v1.3.4";
        font-size: 9pt;
        font-style: italic;
        color: #666;
    }
    @bottom-center { 
        content: counter(page); 
        font-size: 10pt; 
        font-weight: 500;
    } 
}
@page :first { 
    @top-center { content: ""; }
    @bottom-center { content: ""; } 
}
@page toc {
    @bottom-center {
        content: counter(page, lower-roman);
        font-size: 10pt;
    }
}

/* Base styles with widow/orphan control */
* { margin: 0; padding: 0; box-sizing: border-box; }
html { orphans: 3; widows: 3; }
body { 
    font-family: 'Georgia', 'Segoe UI', 'Roboto', 'Helvetica Neue', sans-serif; 
    line-height: 1.6; 
    color: #222; 
    font-size: 11pt; 
    text-align: justify;
    hyphens: auto;
}
.container { max-width: 100%; }

/* Title page */
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
    font-family: 'Helvetica Neue', 'Arial', sans-serif;
    font-size: 48pt; 
    font-weight: 700; 
    margin-bottom: 20pt; 
    border: none; 
    color: white; 
}
.title-page .subtitle { 
    font-size: 18pt; 
    margin-bottom: 40pt; 
    opacity: 0.95; 
}
.title-page .meta { 
    font-size: 11pt; 
    margin-top: 80pt; 
    opacity: 0.85; 
    border-top: 1px solid rgba(255,255,255,0.3); 
    padding-top: 30pt; 
    line-height: 1.8; 
}

/* TOC with page numbers */
.toc-page { 
    page: toc;
    page-break-after: always; 
}
.toc-page h2 { 
    font-family: 'Helvetica Neue', 'Arial', sans-serif;
    font-size: 20pt; 
    color: #1e3a5f; 
    border-bottom: 2pt solid #7c4dff; 
    padding-bottom: 12pt; 
    margin-bottom: 20pt; 
    margin-top: 0; 
    font-weight: 700;
    page-break-after: avoid;
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
    margin-bottom: 6pt; 
    padding-bottom: 4pt; 
    border-bottom: 1px dotted #ddd;
    page-break-inside: avoid;
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
    min-width: 35pt; 
}

/* Headings with proper page break control */
h1, h2, h3, h4, h5, h6 { 
    font-family: 'Helvetica Neue', 'Arial', sans-serif;
    page-break-after: avoid; 
    page-break-inside: avoid;
    orphans: 3;
    widows: 3;
}
h1 { 
    page-break-before: always; 
    font-size: 18pt; 
    font-weight: 700; 
    color: #1e3a5f; 
    border-bottom: 2pt solid #7c4dff; 
    padding-bottom: 8pt; 
    margin-bottom: 12pt; 
    margin-top: 0; 
    line-height: 1.2; 
}
h2 { 
    font-size: 13pt; 
    font-weight: 700; 
    color: #2c5aa0; 
    margin-top: 10pt; 
    margin-bottom: 6pt; 
    border-left: 3pt solid #7c4dff; 
    padding-left: 10pt; 
}
h3 { 
    font-size: 11pt; 
    font-weight: 700; 
    color: #444; 
    margin-top: 8pt; 
    margin-bottom: 4pt; 
}
h4, h5, h6 { 
    font-size: 10.5pt; 
    font-weight: 600; 
    color: #555; 
    margin-top: 6pt; 
    margin-bottom: 2pt; 
}

/* Paragraphs with widow/orphan control */
p { 
    margin-bottom: 8pt; 
    text-align: justify; 
    line-height: 1.6;
    orphans: 3;
    widows: 3;
}

/* Code blocks */
code { 
    background: #f5f5f5; 
    padding: 2pt 4pt; 
    border-radius: 2pt; 
    font-family: 'Courier New', monospace; 
    font-size: 9.5pt; 
    color: #d63384; 
}
pre { 
    background: #f9f9f9; 
    border: 1pt solid #e0e0e0; 
    border-left: 3pt solid #7c4dff; 
    padding: 8pt; 
    margin: 10pt 0; 
    font-family: 'Courier New', monospace; 
    font-size: 8.5pt; 
    line-height: 1.4; 
    page-break-inside: avoid;
    orphans: 4;
    widows: 4;
}
pre code { 
    background: none; 
    padding: 0; 
    color: #222; 
}

/* Blockquotes and tables */
blockquote { 
    border-left: 3pt solid #7c4dff; 
    padding-left: 10pt; 
    margin: 8pt 0; 
    color: #666; 
    font-style: italic; 
    font-size: 10pt; 
    page-break-inside: avoid;
    orphans: 3;
    widows: 3;
}
table { 
    border-collapse: collapse; 
    width: 100%; 
    margin: 10pt 0; 
    font-size: 9pt; 
    page-break-inside: avoid; 
}
table th, table td { 
    border: 1pt solid #ddd; 
    padding: 5pt; 
    text-align: left; 
}
table th { 
    background: #1e3a5f; 
    color: white; 
    font-weight: 600;
    page-break-after: avoid;
}
table tr:nth-child(even) { 
    background: #f9f9f9; 
}
thead { display: table-header-group; }

/* Lists with widow/orphan control */
ul, ol { 
    margin-left: 20pt; 
    margin-bottom: 8pt; 
    margin-top: 4pt;
    orphans: 3;
    widows: 3;
}
li { 
    margin-bottom: 2pt; 
    line-height: 1.5;
    page-break-inside: avoid;
}

/* Figures */
.figure-container {
    margin: 15pt 0;
    padding: 10pt;
    text-align: center;
    page-break-inside: avoid;
    orphans: 3;
    widows: 3;
    border: 1pt solid #e0e0e0;
    background: #fafafa;
}
.figure-image {
    max-width: 100%;
    height: auto;
    display: block;
    margin: 0 auto 8pt auto;
}
.figure-caption {
    font-size: 9pt;
    color: #555;
    font-style: italic;
    margin-top: 8pt;
    text-align: center;
    font-weight: 600;
}

/* Links and other elements */
a { color: #2c5aa0; text-decoration: none; }
svg { max-width: 100%; height: auto; }
.chapter-footer { 
    margin-top: 20pt; 
    padding-top: 8pt; 
    border-top: 1pt solid #ddd; 
    text-align: center; 
    font-size: 8pt; 
    color: #999; 
    page-break-after: always; 
}
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

# Abbildungsverzeichnis
if figure_list:
    html += """<div class="toc-page">
<h2>Abbildungsverzeichnis</h2>
<ol class="toc-list">
"""
    for fig_id, caption, chapter in figure_list:
        html += f'<li><a href="#fig{fig_id}">Abb. {fig_id}: {caption[:80]}</a></li>\n'
    html += """</ol></div>
"""

# Tabellenverzeichnis (placeholder für später)
if table_list:
    html += """<div class="toc-page">
<h2>Tabellenverzeichnis</h2>
<ol class="toc-list">
"""
    for table_id, caption, chapter in table_list:
        html += f'<li><a href="#tab{table_id}">Tab. {table_id}: {caption[:80]}</a></li>\n'
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
print(f"[OK] HTML generated: {size_mb:.1f} MB")
print(f"[OK] Saved to: {output_file}")
print(f"\n[READY] Ready for PDF conversion:")
print(f"   weasyprint {output_file} ThemisDB-Kompendium-v1.3.4-print.pdf")
