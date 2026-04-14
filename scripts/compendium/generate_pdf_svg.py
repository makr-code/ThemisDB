"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_pdf_svg.py                                ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:29:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     218                                            ║
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
Generate PDF with Mermaid diagrams converted to SVG images.

This script:
1. Extracts all Mermaid code blocks from chapter files
2. Converts each Mermaid block to SVG using mermaid-cli (mmdc)
3. Replaces Mermaid blocks with SVG image references
4. Generates PDF with embedded SVG images

Requirements:
- npm install -g @mermaid-js/mermaid-cli
- pip install weasyprint markdown pymdown-extensions
"""

import os
import re
import subprocess
import tempfile
import hashlib
from pathlib import Path
from datetime import datetime

# Configuration
COMPENDIUM_DIR = Path(__file__).parent
PDF_OUTPUT_DIR = COMPENDIUM_DIR / ".." / ".." / "pdf_output"
SVG_OUTPUT_DIR = PDF_OUTPUT_DIR / "mermaid_svg"
CHAPTER_FILES = sorted(COMPENDIUM_DIR.glob("chapter_*.md"))

def extract_mermaid_blocks(content):
    """Extract all Mermaid code blocks from markdown content."""
    pattern = r'```mermaid\n(.*?)```'
    matches = re.findall(pattern, content, re.DOTALL)
    return matches

def mermaid_to_svg(mermaid_code, output_path):
    """Convert Mermaid code to SVG using mermaid-cli."""
    try:
        # Create temporary mermaid file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.mmd', delete=False) as f:
            f.write(mermaid_code)
            mmd_file = f.name
        
        # Run mmdc (mermaid-cli) to convert to SVG
        result = subprocess.run(
            ['mmdc', '-i', mmd_file, '-o', str(output_path), '-b', 'transparent'],
            capture_output=True,
            text=True,
            timeout=30
        )
        
        # Clean up temp file
        os.unlink(mmd_file)
        
        if result.returncode == 0:
            print(f"✓ Generated: {output_path.name}")
            return True
        else:
            print(f"✗ Failed to generate {output_path.name}: {result.stderr}")
            return False
            
    except FileNotFoundError:
        print("Error: mermaid-cli (mmdc) not found!")
        print("Install with: npm install -g @mermaid-js/mermaid-cli")
        return False
    except Exception as e:
        print(f"Error converting Mermaid to SVG: {e}")
        return False

def process_chapter_file(chapter_path, svg_dir):
    """Process a chapter file and convert all Mermaid blocks to SVG."""
    with open(chapter_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    mermaid_blocks = extract_mermaid_blocks(content)
    print(f"\n{chapter_path.name}: Found {len(mermaid_blocks)} Mermaid diagrams")
    
    if not mermaid_blocks:
        return content
    
    # Replace each Mermaid block with SVG image reference
    diagram_counter = 0
    modified_content = content
    
    for mermaid_code in mermaid_blocks:
        diagram_counter += 1
        
        # Generate unique filename based on content hash
        content_hash = hashlib.md5(mermaid_code.encode()).hexdigest()[:8]
        chapter_name = chapter_path.stem
        svg_filename = f"{chapter_name}_diagram_{diagram_counter}_{content_hash}.svg"
        svg_path = svg_dir / svg_filename
        
        # Convert to SVG
        if mermaid_to_svg(mermaid_code, svg_path):
            # Replace Mermaid block with SVG image
            mermaid_block = f"```mermaid\n{mermaid_code}```"
            svg_reference = f'\n![Diagram {diagram_counter}](mermaid_svg/{svg_filename})\n'
            modified_content = modified_content.replace(mermaid_block, svg_reference, 1)
    
    return modified_content

def generate_pdf_with_svg(theme='material'):
    """Generate PDF with SVG-rendered Mermaid diagrams."""
    print("\n" + "="*60)
    print("ThemisDB Kompendium PDF Generator (SVG Mermaid)")
    print("="*60)
    
    # Create output directories
    PDF_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    SVG_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    
    # Check if mmdc is available
    try:
        subprocess.run(['mmdc', '--version'], capture_output=True, check=True)
    except (FileNotFoundError, subprocess.CalledProcessError):
        print("\n❌ Error: mermaid-cli (mmdc) not found!")
        print("   Install with: npm install -g @mermaid-js/mermaid-cli")
        print("   This will download Chromium (~400 MB) on first run.")
        return
    
    # Process all chapter files
    print("\n📊 Converting Mermaid diagrams to SVG...")
    print("-" * 60)
    
    temp_dir = tempfile.mkdtemp()
    temp_chapters = []
    
    for chapter_path in CHAPTER_FILES:
        modified_content = process_chapter_file(chapter_path, SVG_OUTPUT_DIR)
        
        # Save modified content to temp file
        temp_chapter_path = Path(temp_dir) / chapter_path.name
        with open(temp_chapter_path, 'w', encoding='utf-8') as f:
            f.write(modified_content)
        temp_chapters.append(temp_chapter_path)
    
    print("\n" + "="*60)
    print("📄 Generating PDF with embedded SVG diagrams...")
    print("="*60)
    
    # Generate PDF using WeasyPrint (similar to generate_pdf_with_mermaid.py)
    date_str = datetime.now().strftime("%Y%m%d")
    pdf_filename = f"ThemisDB-Kompendium-svg-{date_str}.pdf"
    pdf_path = PDF_OUTPUT_DIR / pdf_filename
    
    try:
        import weasyprint
        from markdown import markdown
        from io import BytesIO
        
        # Combine all chapters
        html_content = "<html><head><style>"
        html_content += """
        body { font-family: Georgia, serif; line-height: 1.65; max-width: 800px; margin: 0 auto; padding: 20px; }
        h1 { color: #7c4dff; border-bottom: 3px solid #7c4dff; padding-bottom: 10px; }
        h2 { color: #43e97b; margin-top: 30px; }
        h3 { color: #4facfe; }
        img { max-width: 100%; height: auto; margin: 20px 0; border: 1px solid #ddd; padding: 10px; }
        code { background: #f5f3ff; padding: 2px 5px; border-radius: 3px; font-family: 'Fira Code', monospace; }
        pre { background: #f5f3ff; padding: 15px; border-radius: 5px; overflow-x: auto; }
        blockquote { border-left: 4px solid #7c4dff; padding-left: 15px; font-style: italic; color: #666; }
        """
        html_content += "</style></head><body>"
        
        for temp_chapter in temp_chapters:
            with open(temp_chapter, 'r', encoding='utf-8') as f:
                chapter_md = f.read()
            html_content += markdown(chapter_md, extensions=['fenced_code', 'tables', 'toc'])
        
        html_content += "</body></html>"
        
        # Generate PDF
        pdf = weasyprint.HTML(string=html_content, base_url=str(COMPENDIUM_DIR)).write_pdf()
        
        with open(pdf_path, 'wb') as f:
            f.write(pdf)
        
        file_size_mb = pdf_path.stat().st_size / (1024 * 1024)
        print(f"\n✅ PDF successfully generated!")
        print(f"   Output: {pdf_path}")
        print(f"   Size: {file_size_mb:.2f} MB")
        print(f"   SVG diagrams: {len(list(SVG_OUTPUT_DIR.glob('*.svg')))}")
        
    except ImportError as e:
        print(f"\n❌ Error: Missing Python package: {e}")
        print("   Install with: pip install weasyprint markdown pymdown-extensions")
    except Exception as e:
        print(f"\n❌ Error generating PDF: {e}")
    finally:
        # Cleanup temp files
        import shutil
        shutil.rmtree(temp_dir, ignore_errors=True)

if __name__ == "__main__":
    generate_pdf_with_svg()
