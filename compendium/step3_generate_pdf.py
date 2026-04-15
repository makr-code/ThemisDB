"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            step3_generate_pdf.py                              ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     167                                            ║
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
        from weasyprint import HTML, CSS
        
        # Try direct conversion
        html = HTML(filename=str(html_path))
        html.write_pdf(str(pdf_path), uncompressed_pdf=False)
        
        if pdf_path.exists():
            size_mb = pdf_path.stat().st_size / (1024 * 1024)
            print(f"OK - {size_mb:.2f} MB (Native PDF - Text + Vectors)")
            return True
        else:
            print("Failed to create PDF")
            return False
            
    except ImportError as e:
        print(f"Not installed (missing dependency: {e})")
        return False
    except OSError as e:
        if "libgobject" in str(e) or "cannot load library" in str(e):
            print(f"Not installed (missing system libraries)")
            return False
        print(f"OS Error: {e}")
        return False
    except KeyboardInterrupt:
        print("Cancelled")
        return False
    except TimeoutError:
        print("Timeout: WeasyPrint took longer than expected")
        return False
    except Exception as e:
        print(f"Error: {type(e).__name__}: {e}")
        return False

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate PDF from HTML')
    parser.add_argument('--version', action='version', version=f'step3_generate_pdf.py {VERSION}')
    args = parser.parse_args()
    
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
