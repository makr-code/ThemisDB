#!/usr/bin/env python3
"""
ThemisDB Kompendium - PDF Generator mit professionellem Buchlayout

Dieser Generator erstellt PDFs mit:
- Durchgehender Seitennummerierung
- Seitenzahlen im Inhaltsverzeichnis
- Widow/Orphan Control (keine abgeschnittenen Absätze)
- Professionellem Buchlayout
- Running Headers
- Intelligenten Seitenumbrüchen

Verwendung:
    python3 generate_pdf_book.py

Ausgabe:
    pdf/ThemisDB-Kompendium-v1.3.4-professional.pdf
"""

import subprocess
import sys
from pathlib import Path

COMPENDIUM_DIR = Path(__file__).parent
BUILDER_SCRIPT = COMPENDIUM_DIR / "build_pdf_professional.py"
OUTPUT_HTML = COMPENDIUM_DIR / "pdf" / "ThemisDB-Kompendium-v1.3.4-professional.html"
OUTPUT_PDF = COMPENDIUM_DIR / "pdf" / "ThemisDB-Kompendium-v1.3.4-professional.pdf"

def main():
    print("\n" + "="*70)
    print("  ThemisDB Kompendium - Professional Book PDF Generator")
    print("="*70 + "\n")
    
    # Step 1: Generate HTML
    print("📄 Schritt 1: HTML mit professionellem Layout generieren...")
    try:
        result = subprocess.run(
            [sys.executable, str(BUILDER_SCRIPT)],
            cwd=str(COMPENDIUM_DIR),
            check=True,
            capture_output=False
        )
        print("✅ HTML erfolgreich generiert\n")
    except subprocess.CalledProcessError as e:
        print(f"❌ Fehler beim Generieren des HTML: {e}")
        return 1
    
    # Step 2: Convert to PDF
    print("📘 Schritt 2: PDF mit WeasyPrint generieren...")
    print("   (Dies kann einige Minuten dauern...)\n")
    
    try:
        # Check if weasyprint is installed
        result = subprocess.run(
            ["weasyprint", "--version"],
            capture_output=True,
            check=True
        )
        print(f"   WeasyPrint Version: {result.stdout.decode().strip()}")
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("❌ WeasyPrint ist nicht installiert!")
        print("\n   Installation:")
        print("   pip install weasyprint")
        return 1
    
    try:
        result = subprocess.run(
            ["weasyprint", str(OUTPUT_HTML), str(OUTPUT_PDF)],
            cwd=str(COMPENDIUM_DIR),
            check=True,
            capture_output=True,
            text=True
        )
        
        # Show warnings but not errors (they're expected)
        if result.stderr:
            warnings = [line for line in result.stderr.split('\n') 
                       if line.startswith('WARNING:') and 'notdef glyph' not in line]
            if warnings[:5]:  # Show first 5 warnings
                print("   Hinweise:")
                for warning in warnings[:5]:
                    print(f"   {warning}")
        
        print("\n✅ PDF erfolgreich generiert!")
        
        # Get file size
        if OUTPUT_PDF.exists():
            size_mb = OUTPUT_PDF.stat().st_size / 1024 / 1024
            print(f"   Dateigröße: {size_mb:.1f} MB")
            print(f"   Speicherort: {OUTPUT_PDF}")
        
    except subprocess.CalledProcessError as e:
        print(f"❌ Fehler beim Generieren des PDF: {e}")
        if e.stderr:
            print(f"   Details: {e.stderr}")
        return 1
    
    print("\n" + "="*70)
    print("  Professionelle Features im generierten PDF:")
    print("="*70)
    print("  ✓ Durchgehende Seitennummerierung")
    print("  ✓ Seitenzahlen im Inhaltsverzeichnis")
    print("  ✓ Widow/Orphan Control (min. 3 Zeilen)")
    print("  ✓ Keine abgeschnittenen Kapitel/Absätze")
    print("  ✓ Professionelles Buchlayout")
    print("  ✓ Running Headers mit Buchtitel")
    print("  ✓ Intelligente Seitenumbrüche")
    print("  ✓ Blocksatz mit automatischer Silbentrennung")
    print("="*70 + "\n")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
