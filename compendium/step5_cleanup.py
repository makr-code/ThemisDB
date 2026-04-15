"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            step5_cleanup.py                                   ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     138                                            ║
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
Step 4: Cleanup all generated artifacts.
Removes: output directory with PDF, HTML, and SVG files.
"""

import shutil
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
    print("Step 4: Cleanup Generated Artifacts")
    print("=" * 70)
    
    artifacts = []
    
    # Check output directory
    if OUTPUT_DIR.exists() and OUTPUT_DIR.is_dir():
        # Count all files recursively
        all_files = list(OUTPUT_DIR.rglob("*"))
        file_count = len([f for f in all_files if f.is_file()])
        total_size = sum(f.stat().st_size for f in all_files if f.is_file()) / (1024 * 1024)
        
        # List specific types
        pdf_files = list(OUTPUT_DIR.rglob("*.pdf"))
        html_files = list(OUTPUT_DIR.rglob("*.html"))
        svg_files = list(OUTPUT_DIR.rglob("*.svg"))
        
        artifacts.append(("Output Directory", OUTPUT_DIR, total_size, file_count))
        
        print(f"\nContents of {OUTPUT_DIR}:")
        print(f"  - PDF files:  {len(pdf_files)}")
        print(f"  - HTML files: {len(html_files)}")
        print(f"  - SVG files:  {len(svg_files)}")
        print(f"  - Total:      {file_count} files ({total_size:.2f} MB)")
    
    # 4. Temporary files
    temp_files = [
        COMPENDIUM_DIR / f"ThemisDB-Kompendium-{VERSION}_temp.pdf",
        COMPENDIUM_DIR / "__pycache__"
    ]
    
    # Also check for __pycache__
    pycache_dir = COMPENDIUM_DIR / "__pycache__"
    if pycache_dir.exists() and pycache_dir.is_dir():
        artifacts.append(("Cache", pycache_dir, 0, 0))
    
    if not artifacts:
        print("\n[INFO] No artifacts found - workspace already clean")
        return True
    
    # Show what will be deleted
    print(f"\n[INFO] Found {len(artifacts)} artifact(s) to clean:\n")
    for artifact in artifacts:
        if len(artifact) == 4:
            artifact_type, path, size, count = artifact
            if count > 0:
                print(f"  [{artifact_type}] {path.name} ({size:.2f} MB, {count} files)")
            else:
                print(f"  [{artifact_type}] {path.name}")
        else:
            artifact_type, path, size = artifact
            print(f"  [{artifact_type}] {path.name} ({size:.2f} MB)")
    
    # Confirm deletion
    print(f"\n[WARNING] This will permanently delete all generated files!")
    response = input("Continue? [y/N]: ").strip().lower()
    
    if response != 'y':
        print("\n[INFO] Cleanup cancelled")
        return False
    
    # Delete artifacts
    print(f"\n[INFO] Cleaning up...")
    deleted_count = 0
    
    for artifact in artifacts:
        path = artifact[1]
        try:
            if path.is_dir():
                shutil.rmtree(path)
                print(f"  ✓ Deleted directory: {path.name}")
            else:
                path.unlink()
                print(f"  ✓ Deleted file: {path.name}")
            deleted_count += 1
        except Exception as e:
            print(f"  ✗ Failed to delete {path.name}: {e}")
    
    print(f"\n[INFO] Cleanup complete: {deleted_count}/{len(artifacts)} items deleted")
    return True

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Cleanup intermediate build artifacts')
    parser.add_argument('--version', action='version', version=f'step5_cleanup.py {VERSION}')
    parser.add_argument('--force', action='store_true', help='Skip confirmation prompts')
    args = parser.parse_args()
    
    success = main()
    print("\n" + "=" * 70)
    if success:
        print("CLEANUP COMPLETE")
    else:
        print("CLEANUP CANCELLED OR FAILED")
    print("=" * 70)
