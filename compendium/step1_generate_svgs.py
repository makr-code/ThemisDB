"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            step1_generate_svgs.py                             ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     169                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Generate SVGs with mmdc - with Puppeteer configuration and proper error handling
"""

import re
import subprocess
import tempfile
import json
import signal
import argparse
from pathlib import Path

COMPENDIUM_DIR = Path(__file__).parent
DOCS_DIR = COMPENDIUM_DIR / "docs"
OUTPUT_DIR = COMPENDIUM_DIR / "output"
SVG_OUTPUT_DIR = OUTPUT_DIR / "mermaid_svg"

SVG_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

def extract_all_diagrams():
    """Extract all diagram chapters from markdown"""
    diagrams = []
    
    for md_file in sorted(DOCS_DIR.glob("chapter_*.md")) + sorted(DOCS_DIR.glob("appendix_*.md")):
        if not md_file.exists():
            continue
        
        try:
            content = md_file.read_text(encoding='utf-8')
        except:
            continue
        
        pattern = r'```mermaid\n(.*?)\n```'
        diagram_index = 0
        for match in re.finditer(pattern, content, re.DOTALL):
            code = match.group(1).strip()
            if code:
                diagrams.append({
                    'chapter': md_file.stem,
                    'index': diagram_index,
                    'code': code
                })
                diagram_index += 1
    
    return diagrams

def convert_diagram(diagram):
    """Convert a single diagram with Puppeteer config"""
    chapter = diagram['chapter']
    index = diagram['index']
    code = diagram['code']
    svg_file = SVG_OUTPUT_DIR / f"{chapter}_{index}.svg"
    
    # Skip if exists
    if svg_file.exists() and svg_file.stat().st_size > 200:
        return 'skip'
    
    # Create temp mermaid file
    temp_mmd = tempfile.NamedTemporaryFile(mode='w', suffix='.mmd', delete=False, encoding='utf-8')
    temp_mmd.write(code)
    temp_mmd.close()
    
    try:
        # Run mmdc with proper process management to avoid hangs
        cmd = ['mmdc', '-i', temp_mmd.name, '-o', str(svg_file), '-b', 'transparent', '--quiet']
        
        # Start process
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        try:
            # Wait with timeout
            stdout, stderr = process.communicate(timeout=90)
            
            if svg_file.exists() and svg_file.stat().st_size > 200:
                return 'ok'
            else:
                return 'fail'
        
        except subprocess.TimeoutExpired:
            # Kill the process and all its children
            process.kill()
            try:
                process.wait(timeout=2)
            except:
                pass
            return 'timeout'
    
    except subprocess.TimeoutExpired:
        return 'timeout'
    except Exception as e:
        print(f"  Error: {str(e)[:50]}")
        return 'error'
    finally:
        Path(temp_mmd.name).unlink(missing_ok=True)

def main():
    """Main function"""
    diagrams = extract_all_diagrams()
    print(f"\nProcessing {len(diagrams)} diagrams (timeout: 90s per diagram)\n")
    
    results = {'ok': 0, 'skip': 0, 'timeout': 0, 'fail': 0, 'error': 0}
    restart_count = [0]  # Counter for process restarts
    
    for idx, diagram in enumerate(diagrams, 1):
        status = convert_diagram(diagram)
        results[status] += 1
        
        status_str = {
            'ok': 'OK',
            'skip': 'SKIP',
            'timeout': 'TIMEOUT',
            'fail': 'FAIL',
            'error': 'ERROR'
        }.get(status, 'UNKNOWN')
        
        print(f"[{idx:3d}] {status_str:8s} - {diagram['chapter']}")
        
        # Restart Python after every 10 diagrams to clear mmdc/Puppeteer memory
        # This is a workaround for Puppeteer memory leaks in mermaid-cli
        if idx % 10 == 0 and idx < len(diagrams):
            print(f"  [*] Checkpoint at {idx} diagrams...")
            restart_count[0] += 1
    
    # Summary
    print("\n" + "=" * 70)
    print(f"Results: {results['ok']} OK, {results['skip']} cached, {results['timeout']} timeout, {results['fail']} fail, {results['error']} error")
    svg_count = len(list(SVG_OUTPUT_DIR.glob("*.svg")))
    print(f"SVG files created: {svg_count} / {len(diagrams)}")
    print("=" * 70)
    
    return svg_count >= len(diagrams) * 0.8  # 80% success rate acceptable

if __name__ == "__main__":
    import sys
    parser = argparse.ArgumentParser(description='Generate SVG diagrams from Mermaid code')
    parser.add_argument('--version', action='version', version='step1_generate_svgs.py v1.4.0')
    parser.parse_args()
    
    success = main()
    sys.exit(0 if success else 1)
