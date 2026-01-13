#!/usr/bin/env python3
"""
Step 3: Generate PDF from HTML with wkhtmltopdf (Native PDF - Text + Vektoren)
Fallback: WeasyPrint
"""

import subprocess
import argparse
from pathlib import Path

COMPENDIUM_DIR = Path(__file__).parent
OUTPUT_DIR = COMPENDIUM_DIR / "output"

# Read version
VERSION_FILE = COMPENDIUM_DIR / "VERSION"
VERSION = "v1.4.0"
if VERSION_FILE.exists():
    VERSION = VERSION_FILE.read_text(encoding='utf-8').strip()

def main():
    print("=" * 70)
    print("Step 3: Generate PDF (Native PDF - Text + Vectors)")
    print("=" * 70)
    
    html_filename = f"ThemisDB-Kompendium-{VERSION}.html"
    html_path = OUTPUT_DIR / html_filename
    
    if not html_path.exists():
        print(f"ERROR: HTML file not found: {html_filename}")
        return False
    
    pdf_filename = f"ThemisDB-Kompendium-{VERSION}.pdf"
    pdf_path = OUTPUT_DIR / pdf_filename
    
    print(f"\n[INFO] Input:  {html_filename}")
    print(f"[INFO] Output: {pdf_filename}\n")
    
    # Preferred for proper TOC page numbers and CSS margin boxes: WeasyPrint
    print("[1/2] Trying WeasyPrint...")
    if try_weasyprint(html_path, pdf_path):
        return True
    
    # Fallback: wkhtmltopdf (no TOC page numbers)
    print("[2/2] Trying wkhtmltopdf...")
    if try_wkhtmltopdf(html_path, pdf_path):
        return True
    
    print("ERROR: No PDF generation method available!")
    return False

def try_wkhtmltopdf(html_path, pdf_path):
    """Try to convert HTML to PDF using wkhtmltopdf."""
    try:
        # wkhtmltopdf with built-in TOC and header/footer page numbers
        cmd = [
            'wkhtmltopdf',
            '--quiet',
            '--enable-local-file-access',
            '--margin-top', '18mm',
            '--margin-bottom', '16mm',
            '--margin-left', '15mm',
            '--margin-right', '15mm',
            '--page-size', 'A4',
            '--dpi', '150',
            str(html_path),
            str(pdf_path)
        ]
        
        print(f"  Running wkhtmltopdf...")
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
        
        if result.returncode == 0 and pdf_path.exists():
            size_mb = pdf_path.stat().st_size / (1024 * 1024)
            print(f"OK - {size_mb:.2f} MB (Native PDF - Text + Vectors)")
            return True
        else:
            if result.stderr:
                print(f"  Error: {result.stderr[:150]}")
            print(f"Failed (return code {result.returncode})")
            return False
            
    except FileNotFoundError:
        print(f"Not installed")
        return False
    except subprocess.TimeoutExpired:
        print(f"Timeout")
        return False
    except Exception as e:
        print(f"Error: {type(e).__name__}: {e}")
        return False

def try_weasyprint(html_path, pdf_path):
    """Generate PDF with WeasyPrint (fallback)."""
    try:
        print("  (This may take several minutes)...")
        from weasyprint import HTML
        
        # Use subprocess with timeout to prevent hanging
        import subprocess
        cmd = [
            'python3', '-c',
            f"""
import sys
from weasyprint import HTML
try:
    html = HTML(filename='{html_path}')
    html.write_pdf('{pdf_path}', uncompressed_pdf=False)
    print('SUCCESS')
except Exception as e:
    print(f'ERROR: {{e}}', file=sys.stderr)
    sys.exit(1)
"""
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=1200)
        
        if result.returncode != 0:
            if result.stderr:
                print(f"  Error: {result.stderr[:200]}")
            return False
        
        if pdf_path.exists():
            size_mb = pdf_path.stat().st_size / (1024 * 1024)
            print(f"OK - {size_mb:.2f} MB (Native PDF - Text + Vectors)")
            return True
        else:
            print("Failed to create PDF")
            return False
            
    except ImportError:
        print("Not installed")
        return False
    except KeyboardInterrupt:
        print("Cancelled")
        return False
    except subprocess.TimeoutExpired:
        print("Timeout: WeasyPrint took longer than 600 seconds")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

if __name__ == "__main__":
    success = main()
    print("\n" + "=" * 70)
    if success:
        pdf_path = OUTPUT_DIR / f"ThemisDB-Kompendium-{VERSION}.pdf"
        if pdf_path.exists():
            size_mb = pdf_path.stat().st_size / (1024 * 1024)
            print(f"BUILD COMPLETE - PDF generated: {size_mb:.2f} MB")
        else:
            print("BUILD COMPLETE")
    else:
        print("BUILD FAILED")
    print("=" * 70)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate PDF from HTML')
    parser.add_argument('--version', action='version', version=f'step3_generate_pdf.py {VERSION}')
    args = parser.parse_args()
    
    main()
