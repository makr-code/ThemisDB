#!/usr/bin/env python3
from pathlib import Path

pdf_file = Path("pdf/ThemisDB-Kompendium-v1.3.4-print.pdf")

if pdf_file.exists():
    size_mb = pdf_file.stat().st_size / 1024 / 1024
    print(f"✅ PDF existiert")
    print(f"📄 Größe: {size_mb:.1f} MB")
    
    try:
        from PyPDF2 import PdfReader
        reader = PdfReader(str(pdf_file))
        pages = len(reader.pages)
        print(f"📖 Seitenzahl: {pages}")
        print(f"✓ PDF ist lesbar und konsistent")
    except ImportError:
        print("(PyPDF2 nicht installiert)")
    except Exception as e:
        print(f"⚠️ Fehler beim PDF-Lesen: {e}")
else:
    print(f"❌ PDF nicht gefunden: {pdf_file}")
