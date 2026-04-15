"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_pdf_all_themes.py                         ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:07:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     273                                            ║
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
PDF Generation Script for ThemisDB Compendium - Multiple Themes
Generates PDF versions using different MkDocs themes for comparison
"""

import os
import sys
import shutil
import subprocess
import yaml
from pathlib import Path
from datetime import datetime

# Configuration
SCRIPT_DIR = Path(__file__).parent.absolute()
OUTPUT_DIR = SCRIPT_DIR.parent.parent / "pdf_output" / "themes"
TEMP_DIR = Path("/tmp/themis_pdf_themes")
BASE_CONFIG = SCRIPT_DIR / "mkdocs-compendium.yml"

# Themes to generate PDFs for
THEMES = [
    {
        "name": "material",
        "display_name": "Material for MkDocs",
        "package": "mkdocs-material",
        "config": {
            "name": "material",
            "palette": {
                "scheme": "default",
                "primary": "deep purple",
                "accent": "amber"
            }
        }
    },
    {
        "name": "readthedocs",
        "display_name": "ReadTheDocs",
        "package": None,  # Built-in
        "config": {
            "name": "readthedocs",
            "highlightjs": True
        }
    },
    {
        "name": "mkdocs",
        "display_name": "MkDocs Default",
        "package": None,  # Built-in
        "config": {
            "name": "mkdocs"
        }
    }
]

def print_section(title):
    """Print a formatted section header"""
    print("\n" + "=" * 60)
    print(f"  {title}")
    print("=" * 60 + "\n")

def install_theme(theme):
    """Install theme if needed"""
    if theme["package"]:
        print(f"📦 Installing {theme['package']}...")
        try:
            subprocess.run(
                [sys.executable, "-m", "pip", "install", "--quiet", theme["package"]],
                check=True
            )
            print(f"  ✓ {theme['display_name']} installed")
        except subprocess.CalledProcessError as e:
            print(f"  ✗ Failed to install {theme['package']}: {e}")
            return False
    else:
        print(f"  ✓ {theme['display_name']} (built-in)")
    return True

def create_theme_config(theme):
    """Create a modified mkdocs config for the theme"""
    # Load base config
    with open(BASE_CONFIG, 'r') as f:
        config = yaml.safe_load(f)
    
    # Update theme
    config['theme'] = theme['config']
    
    # Disable PDF plugin to avoid conflicts
    if 'plugins' in config:
        config['plugins'] = [
            p for p in config['plugins'] 
            if not (isinstance(p, dict) and 'with-pdf' in p)
        ]
    
    # Save to temp config
    temp_config = TEMP_DIR / f"mkdocs-{theme['name']}.yml"
    with open(temp_config, 'w') as f:
        yaml.dump(config, f, default_flow_style=False, allow_unicode=True)
    
    return temp_config

def generate_pdf_for_theme(theme):
    """Generate PDF for a specific theme"""
    print(f"\n🎨 Generating PDF with {theme['display_name']} theme...")
    
    # Create theme-specific config
    theme_config = create_theme_config(theme)
    
    # Build site
    site_dir = TEMP_DIR / f"site-{theme['name']}"
    print(f"  📝 Building site...")
    
    try:
        result = subprocess.run(
            ["mkdocs", "build", "-f", str(theme_config), "-d", str(site_dir), "--quiet"],
            capture_output=True,
            text=True,
            timeout=180
        )
        
        if result.returncode != 0:
            print(f"  ✗ Build failed: {result.stderr}")
            return None
            
    except subprocess.TimeoutExpired:
        print(f"  ✗ Build timeout")
        return None
    except Exception as e:
        print(f"  ✗ Build error: {e}")
        return None
    
    print(f"  ✓ Site built successfully")
    
    # Convert to PDF using WeasyPrint
    output_file = OUTPUT_DIR / f"ThemisDB-Kompendium-{theme['name']}-{datetime.now().strftime('%Y%m%d')}.pdf"
    
    # Use the WeasyPrint method with the built HTML
    print(f"  🔨 Converting to PDF...")
    
    try:
        # Import WeasyPrint
        from weasyprint import HTML, CSS
        
        # Find the main index.html
        index_html = site_dir / "index.html"
        
        if not index_html.exists():
            print(f"  ✗ index.html not found")
            return None
        
        # Generate PDF
        HTML(filename=str(index_html)).write_pdf(
            str(output_file),
            stylesheets=[CSS(string='''
                @page {
                    size: A4;
                    margin: 20mm;
                }
                body {
                    font-family: Georgia, serif;
                    font-size: 11pt;
                    line-height: 1.6;
                }
                h1 {
                    color: #7c4dff;
                    page-break-before: always;
                }
                code {
                    background: #f5f5f5;
                    padding: 2px 4px;
                    font-size: 10pt;
                }
                pre {
                    background: #f8f8f8;
                    padding: 10px;
                    border-left: 3px solid #7c4dff;
                    page-break-inside: avoid;
                }
            ''')]
        )
        
        if output_file.exists():
            size_mb = output_file.stat().st_size / (1024 * 1024)
            print(f"  ✓ PDF generated: {output_file.name} ({size_mb:.2f} MB)")
            return output_file
        else:
            print(f"  ✗ PDF file not created")
            return None
            
    except ImportError:
        print(f"  ✗ WeasyPrint not available")
        return None
    except Exception as e:
        print(f"  ✗ PDF generation failed: {e}")
        return None

def main():
    print_section("ThemisDB Compendium - Multi-Theme PDF Generator")
    
    # Create directories
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    TEMP_DIR.mkdir(parents=True, exist_ok=True)
    
    # Install dependencies
    print("📦 Installing dependencies...")
    deps = ["mkdocs", "mkdocs-material", "pymdown-extensions", "weasyprint"]
    for dep in deps:
        subprocess.run(
            [sys.executable, "-m", "pip", "install", "--quiet", dep],
            check=False
        )
    
    # Generate PDFs for each theme
    results = []
    
    for theme in THEMES:
        print_section(f"Theme: {theme['display_name']}")
        
        # Install theme
        if not install_theme(theme):
            results.append((theme['display_name'], None))
            continue
        
        # Generate PDF
        pdf_file = generate_pdf_for_theme(theme)
        results.append((theme['display_name'], pdf_file))
    
    # Summary
    print_section("Generation Summary")
    
    print("📊 Results:")
    for theme_name, pdf_file in results:
        if pdf_file:
            print(f"  ✓ {theme_name}: {pdf_file.name}")
        else:
            print(f"  ✗ {theme_name}: Failed")
    
    # Count successes
    successful = sum(1 for _, pdf in results if pdf)
    total = len(results)
    
    print(f"\n✨ Generated {successful}/{total} PDFs successfully")
    print(f"📁 Output directory: {OUTPUT_DIR}")
    
    # Cleanup
    print("\n🧹 Cleaning up temporary files...")
    shutil.rmtree(TEMP_DIR, ignore_errors=True)
    
    return 0 if successful > 0 else 1

if __name__ == "__main__":
    sys.exit(main())
