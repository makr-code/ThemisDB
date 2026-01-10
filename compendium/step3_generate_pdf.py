#!/usr/bin/env python3
"""
Step 3: Generate PDF from HTML with wkhtmltopdf (Native PDF - Text + Vektoren)
Fallback: WeasyPrint
"""

import subprocess
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
    
    # Method 1: Try wkhtmltopdf (faster than WeasyPrint for large documents)
    print("[1/2] Trying wkhtmltopdf...")
    if try_wkhtmltopdf(html_path, pdf_path):
        return True
    
    # Method 2: Fallback to WeasyPrint
    print("[2/2] Trying WeasyPrint...")
    if try_weasyprint(html_path, pdf_path):
        return True
    
    print("ERROR: No PDF generation method available!")
    return False

def create_header_footer_html():
    """Create header and footer HTML files for wkhtmltopdf."""
    header_html = f'''<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body {{ margin: 0; padding: 0; font-family: Georgia, serif; font-size: 9pt; }}
        .header {{ text-align: center; color: #1a4d2e; padding: 5px 0; border-bottom: 1px solid #2a7f62; }}
    </style>
</head>
<body>
    <div class="header">
        ThemisDB Kompendium {VERSION} | Seite <span class="page"></span>
    </div>
</body>
</html>
'''
    
    footer_html = f'''<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body {{ margin: 0; padding: 0; font-family: Georgia, serif; font-size: 8pt; }}
        .footer {{ text-align: center; color: #999; padding: 5px 0; border-top: 1px solid #ddd; }}
    </style>
</head>
<body>
    <div class="footer">
        © 2026 ThemisDB Team | Seite <span class="page"></span> von <span class="topage"></span>
    </div>
</body>
</html>
'''
    
    # Write header and footer files
    header_path = OUTPUT_DIR / "header.html"
    footer_path = OUTPUT_DIR / "footer.html"
    
    with open(header_path, 'w', encoding='utf-8') as f:
        f.write(header_html)
    
    with open(footer_path, 'w', encoding='utf-8') as f:
        f.write(footer_html)
    
    return str(header_path), str(footer_path)

def try_wkhtmltopdf(html_path, pdf_path):
    """Try to convert HTML to PDF using wkhtmltopdf with headers/footers."""
    try:
        # Create header and footer HTML files
        header_path, footer_path = create_header_footer_html()
        
        cmd = [
            'wkhtmltopdf',
            '--quiet',
            '--enable-local-file-access',
            '--header-html', header_path,
            '--footer-html', footer_path,
            '--margin-top', '25mm',
            '--margin-bottom', '25mm',
            '--margin-left', '20mm',
            '--margin-right', '20mm',
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
        
        html = HTML(filename=str(html_path))
        html.write_pdf(str(pdf_path), uncompressed_pdf=False)
        
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
