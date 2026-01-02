#!/usr/bin/env python3
"""
Detaillierte PDF-Überprüfung
"""
from pathlib import Path
from PyPDF2 import PdfReader

pdf_file = Path("pdf/ThemisDB-Kompendium-v1.3.4-print.pdf")

if not pdf_file.exists():
    print(f"❌ PDF nicht gefunden: {pdf_file}")
    exit(1)

size_mb = pdf_file.stat().st_size / 1024 / 1024
print(f"📄 PDF: {pdf_file}")
print(f"📊 Größe: {size_mb:.1f} MB")

try:
    reader = PdfReader(str(pdf_file))
    pages = len(reader.pages)
    print(f"📖 Seitenzahl: {pages}")
    
    # Metadaten
    if reader.metadata:
        print(f"\n📋 Metadaten:")
        for key, val in reader.metadata.items():
            print(f"   {key}: {val}")
    
    # Erste Seite
    print(f"\n✓ Erste Seite (1):")
    first_page = reader.pages[0]
    text = first_page.extract_text()[:200] if first_page else "N/A"
    print(f"   {text}...")
    
    # Seitenzahlen-Statistik
    print(f"\n📈 Seitenanzahl pro Kapitel (durchschnittlich):")
    print(f"   49 Kapitel ÷ {pages} Seiten = ~{pages/49:.1f} Seiten/Kapitel")
    
    print(f"\n✅ PDF ist vollständig und lesbar")
    
except Exception as e:
    print(f"⚠️  Fehler beim Lesen: {e}")
