# Interactive PDF with Embedded Mermaid Diagrams - Technical Guide

## Ziel

Echte **interaktive PDFs** mit reaktiven, klickbaren Mermaid-Diagrammen direkt im PDF-Format.

## Technische Herausforderung

PDFs können JavaScript enthalten, aber die Unterstützung ist limitiert:

### PDF JavaScript Support

| Feature | Support | Details |
|---------|---------|---------|
| Basic JavaScript | ✅ Adobe Reader, Foxit | PDF 1.3+ |
| Form Actions | ✅ Alle modernen Reader | Buttons, Formulare |
| Page Actions | ✅ Adobe Reader | On open, close |
| Rich Media (Flash) | ❌ Deprecated | Nicht mehr unterstützt |
| HTML5/Mermaid.js | ❌ Keine direkte Unterstützung | JavaScript-Sandbox zu limitiert |

## Lösungsansätze

### Ansatz 1: ✅ **3D/Rich Media Annotations (PDF 1.7)** - EMPFOHLEN

PDFs unterstützen eingebettete Rich Media Annotations (RMA), die HTML5-Inhalte anzeigen können.

**Vorteile:**
- Echte Interaktivität im PDF
- HTML/CSS/JS voll unterstützt in Annotation
- Mermaid.js kann eingebettet werden
- Funktioniert in Adobe Reader DC/Acrobat

**Nachteile:**
- Nur Adobe Reader DC/Acrobat unterstützt RMA vollständig
- Preview.app (macOS), Firefox PDF Viewer: eingeschränkt
- Mobile Reader: oft keine Unterstützung

**Implementierung:**
```python
from pypdf import PdfWriter, PdfReader
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4
import base64

def embed_interactive_mermaid(pdf_writer, page_num, mermaid_code, position):
    """
    Embeds interactive Mermaid diagram as Rich Media Annotation
    """
    # Create HTML with embedded Mermaid
    html_content = f'''
    <!DOCTYPE html>
    <html>
    <head>
        <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
        <style>
            body {{ margin: 0; padding: 20px; background: white; }}
            .mermaid {{ text-align: center; }}
        </style>
    </head>
    <body>
        <div class="mermaid">
{mermaid_code}
        </div>
        <script>
            mermaid.initialize({{ startOnLoad: true, theme: 'default' }});
        </script>
    </body>
    </html>
    '''
    
    # Encode as base64 for embedding
    html_base64 = base64.b64encode(html_content.encode()).decode()
    
    # Create Rich Media Annotation
    annotation = {
        '/Type': '/Annot',
        '/Subtype': '/RichMedia',
        '/Rect': position,  # [x1, y1, x2, y2]
        '/Contents': 'Interactive Mermaid Diagram',
        '/RichMediaContent': {
            '/Type': '/RichMediaContent',
            '/Assets': {
                '/Names': [
                    'diagram.html',
                    {
                        '/Type': '/Filespec',
                        '/F': 'diagram.html',
                        '/EF': {
                            '/F': {
                                '/Type': '/EmbeddedFile',
                                '/Params': {'/Size': len(html_content)},
                                '/Length': len(html_base64)
                            }
                        },
                        '/UF': 'diagram.html'
                    }
                ]
            },
            '/Configurations': {
                '/Type': '/RichMediaConfiguration',
                '/Instances': [
                    {
                        '/Type': '/RichMediaInstance',
                        '/Subtype': '/HTML',
                        '/Asset': 'diagram.html'
                    }
                ]
            }
        }
    }
    
    page = pdf_writer.pages[page_num]
    page.add_annotation(annotation)
```

### Ansatz 2: ✅ **Embedded File Attachments + Hyperlinks**

PDF-Anhänge mit interaktiven HTML-Dateien, verlinkt aus dem PDF.

**Vorteile:**
- Funktioniert in allen PDF-Readern
- Komplette HTML/CSS/JS Unterstützung
- Einfache Implementierung
- Fallback: statische SVG-Images im PDF

**Nachteile:**
- Benötigt Benutzer-Aktion (Anhang öffnen)
- Nicht "inline" im PDF sichtbar

**Implementierung:**
```python
from pypdf import PdfWriter, PdfReader
import io

def attach_interactive_html(pdf_writer, html_content, filename):
    """
    Attach interactive HTML as PDF attachment
    """
    # Create file specification
    file_spec = {
        '/Type': '/Filespec',
        '/F': filename,
        '/UF': filename,
        '/Desc': 'Interactive Mermaid Diagram',
        '/EF': {
            '/F': {
                '/Type': '/EmbeddedFile',
                '/Params': {
                    '/Size': len(html_content),
                    '/ModDate': '(D:20260108000000)'
                }
            }
        }
    }
    
    # Add to PDF catalog
    if '/Names' not in pdf_writer._root_object:
        pdf_writer._root_object['/Names'] = {}
    
    if '/EmbeddedFiles' not in pdf_writer._root_object['/Names']:
        pdf_writer._root_object['/Names']['/EmbeddedFiles'] = {
            '/Names': []
        }
    
    pdf_writer._root_object['/Names']['/EmbeddedFiles']['/Names'].extend([
        filename, file_spec
    ])

def create_hyperlink_to_attachment(pdf_writer, page_num, text, link_rect, attachment_name):
    """
    Create clickable link to attachment
    """
    annotation = {
        '/Type': '/Annot',
        '/Subtype': '/Link',
        '/Rect': link_rect,
        '/Border': [0, 0, 1],
        '/C': [0, 0, 1],  # Blue color
        '/A': {
            '/S': '/GoToE',
            '/D': [0, '/XYZ', 0, 0, 0],
            '/NewWindow': True,
            '/T': attachment_name
        },
        '/Contents': text
    }
    
    page = pdf_writer.pages[page_num]
    page.add_annotation(annotation)
```

### Ansatz 3: ✅ **JavaScript-Enhanced PDF Forms**

PDF-Formulare mit JavaScript für limitierte Interaktivität.

**Vorteile:**
- Breite Reader-Unterstützung
- PDF-Standard JavaScript API
- Keine externen Dateien nötig

**Nachteile:**
- Sehr limitierte JavaScript-API (keine DOM, kein Canvas)
- Kann Mermaid.js nicht direkt ausführen
- Nur einfache Interaktionen möglich

**Verwendung:**
- Toggle-Buttons für verschiedene Diagramm-Ansichten
- Zoom/Pan-Funktionen
- Layer-Visibility-Steuerung

### Ansatz 4: ✅ **Hybrid: SVG Images + QR Codes zu Online-Version**

Statische SVG-Bilder im PDF mit QR-Codes zu interaktiver Web-Version.

**Vorteile:**
- Funktioniert in ALLEN PDF-Readern
- Beste Druckqualität
- Mobile-friendly (QR-Scan)
- Zuverlässig

**Nachteile:**
- Nicht direkt im PDF interaktiv
- Benötigt Online-Hosting

**Implementierung:**
```python
import qrcode
from io import BytesIO

def generate_qr_code_for_diagram(diagram_url):
    """
    Generate QR code pointing to interactive web version
    """
    qr = qrcode.QRCode(version=1, box_size=10, border=2)
    qr.add_data(diagram_url)
    qr.make(fit=True)
    
    img = qr.make_image(fill_color="black", back_color="white")
    
    # Convert to bytes
    buffer = BytesIO()
    img.save(buffer, format='PNG')
    buffer.seek(0)
    
    return buffer.read()

def add_diagram_with_qr(canvas, mermaid_svg_path, qr_code_bytes, x, y, width, height):
    """
    Add SVG diagram with QR code to PDF page
    """
    # Draw SVG
    canvas.drawImage(mermaid_svg_path, x, y, width=width, height=height)
    
    # Draw QR code in corner
    qr_size = 50
    canvas.drawImage(
        BytesIO(qr_code_bytes),
        x + width - qr_size - 10,
        y + height - qr_size - 10,
        width=qr_size,
        height=qr_size
    )
    
    # Add caption
    canvas.setFont("Helvetica", 8)
    canvas.drawString(
        x + width - qr_size - 10,
        y + height - qr_size - 15,
        "Scan für interaktive Version"
    )
```

## Empfohlene Lösung: **Multi-Layer Approach**

Kombiniere mehrere Ansätze für maximale Kompatibilität:

### Layer 1: Statische SVG-Bilder (Baseline)
- Funktioniert in allen Readern
- Beste Druckqualität
- Erzeugt mit `mmdc` (mermaid-cli)

### Layer 2: PDF Attachments (Interaktiv)
- HTML-Dateien mit vollem Mermaid.js
- Verlinkt aus PDF mit Icons/Buttons
- "📊 Klicken für interaktive Version"

### Layer 3: QR Codes (Mobile)
- Links zu gehosteter Web-Version
- Ideal für mobile Nutzer
- Funktioniert mit Handy-Kamera

### Layer 4: Rich Media Annotations (Premium)
- Für Adobe Reader DC Nutzer
- Inline-Interaktivität
- Optionales Feature

## Implementierungsplan

### Phase 1: Core SVG Generation ✅
```bash
# Bereits implementiert in generate_pdf_rendered.py
python3 generate_pdf_rendered.py
```

### Phase 2: Embedded HTML Attachments
```python
# Neu: generate_pdf_with_attachments.py
- Generiere HTML für jedes Mermaid-Diagramm
- Bette HTML als PDF-Anhang ein
- Füge Hyperlinks/Buttons im PDF hinzu
```

### Phase 3: QR Code Integration
```python
# Erweitere Generator mit QR-Codes
- Hoste Diagramme online (GitHub Pages)
- Generiere QR-Codes
- Platziere neben SVG-Bildern
```

### Phase 4: Rich Media (Optional)
```python
# Für Premium-Ausgabe
- Implementiere RMA für Adobe Reader
- Fallback auf andere Methoden
```

## Browser-Based Alternative

Für maximale Interaktivität: **PDF.js + Custom Mermaid Renderer**

Erstelle einen Custom PDF Viewer mit:
- PDF.js für PDF-Rendering
- Mermaid.js overlay für Diagramme
- Volle Interaktivität im Browser

```html
<!DOCTYPE html>
<html>
<head>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.11.174/pdf.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
</head>
<body>
    <canvas id="pdf-canvas"></canvas>
    <div id="mermaid-overlay"></div>
    
    <script>
        // Load PDF
        pdfjsLib.getDocument('compendium.pdf').promise.then(function(pdf) {
            // Render PDF pages
            // Detect Mermaid markers
            // Overlay interactive Mermaid diagrams
        });
    </script>
</body>
</html>
```

## Vergleichstabelle

| Ansatz | Kompatibilität | Interaktivität | Komplexität | Empfehlung |
|--------|---------------|----------------|-------------|------------|
| Rich Media Annotations | ⚠️ Nur Adobe | ⭐⭐⭐⭐⭐ | 🔥🔥🔥🔥 | Optional |
| Embedded Attachments | ✅ Alle | ⭐⭐⭐⭐ | 🔥🔥 | **Ja** |
| PDF Forms + JS | ✅ Die meisten | ⭐⭐ | 🔥🔥 | Nein |
| SVG + QR Codes | ✅ Alle | ⭐⭐⭐ | 🔥 | **Ja** |
| Custom PDF.js Viewer | ✅ Browser | ⭐⭐⭐⭐⭐ | 🔥🔥🔥 | **Ja** |

## Nächste Schritte

1. ✅ Bestehende SVG-Generation nutzen (`generate_pdf_rendered.py`)
2. ➡️ Embedded HTML Attachments implementieren
3. ➡️ QR-Code-Generator erstellen
4. ➡️ Custom PDF.js Viewer erstellen
5. ➡️ (Optional) Rich Media Annotations für Adobe Reader

## Dependencies

```bash
# Python
pip install pypdf reportlab qrcode[pil] pillow

# Node.js (für Mermaid CLI)
npm install -g @mermaid-js/mermaid-cli

# System
apt-get install chromium-browser  # Für Puppeteer/mermaid-cli
```

## Beispiel-Workflow

```bash
# 1. Generiere PDFs mit allen Features
cd compendium
python3 generate_pdf_complete_interactive.py

# Output:
# - ThemisDB-Compendium-v1.3.4-interactive.pdf
#   ├── SVG Diagrams (inline)
#   ├── HTML Attachments (embedded)
#   ├── QR Codes (zu GitHub Pages)
#   └── (optional) Rich Media Annotations

# 2. Starte Custom Viewer
python3 -m http.server 8080
# Öffne: http://localhost:8080/viewer.html
```

---

**Version:** 1.0  
**Datum:** 2026-01-08  
**Autor:** ThemisDB Documentation Team
