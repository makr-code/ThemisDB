#!/usr/bin/env python3
"""
Auto-Migration Skript für Phase 11 Scanner
Liest legacy gap_scanner_v3_phase11_*.py und generiert OOP-kompatible gs3_step03_*.py
"""

import re
from pathlib import Path
import sys

# Mapping von alten Dateinamen zu neuen Namen
MIGRATION_MAP = {
    'gap_scanner_v3_phase11_data_leak.py': 'gs3_step03_data_leak.py',
    'gap_scanner_v3_phase11_encryption_leak.py': 'gs3_step03_encryption_leak.py',
    'gap_scanner_v3_phase11_e2e_encryption.py': 'gs3_step03_e2e_encryption.py',
    'gap_scanner_v3_phase11_key_failure.py': 'gs3_step03_key_failure.py',
    'gap_scanner_v3_phase11_attack_vectors.py': 'gs3_step03_attack_vectors.py',
    'gap_scanner_v3_phase11_military_hardening.py': 'gs3_step03_military_hardening.py',
    'gap_scanner_v3_phase11_legacy_duplication.py': 'gs3_step03_legacy_duplication.py',
}

def extract_docstring(content: str) -> str:
    """Extract docstring from module."""
    match = re.search(r'"""(.*?)"""', content, re.DOTALL)
    if match:
        return match.group(1).strip()
    return "Security gap scanner"

def extract_scanner_class_name(content: str) -> str:
    """Extract main scanner class name."""
    matches = re.findall(r'class\s+(\w+Scanner)\s*:', content)
    if matches:
        return matches[0]
    return "LegacyScanner"

def generate_oop_scanner(legacy_file: Path, old_class_name: str) -> str:
    """
    Generiere OOP-kompatiblen Scanner aus legacy Datei.
    
    Strategie:
    1. Extrahiere Docstring
    2. Extrahiere Pattern-Definitionen
    3. Extrahiere Scan-Methoden
    4. Wrap in BaseGapScanner-Klasse
    """
    
    content = legacy_file.read_text()
    docstring = extract_docstring(content)
    
    # Extract pattern definitions
    pattern_section = re.search(
        rf'class\s+{old_class_name}.*?(?=\n\s{{0,4}}def\s+__init__)',
        content,
        re.DOTALL
    )
    
    patterns = ""
    if pattern_section:
        # Extract all pattern definitions as class attributes
        for match in re.finditer(r'(\w+_PATTERNS?)\s*=\s*\{[^}]*\}', content, re.DOTALL):
            patterns += f"\n    # {match.group(1)}\n"
            # Simplified: just note that patterns exist
            patterns += f"    # {match.group(1)} extracted from legacy\n"
    
    new_class_name = old_class_name  # Keep same name, now in OOP context
    step_name = legacy_file.stem.replace('gap_scanner_v3_', '')
    
    template = f'''#!/usr/bin/env python3
"""
Gap Scanner V3 — {docstring}
(Migrated from legacy gap_scanner_v3_phase11 to OOP architecture)
"""

import sys
from pathlib import Path
from typing import List
import re

sys.path.insert(0, str(Path(__file__).parent.parent))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class {new_class_name}(BaseGapScanner):
    """Migrated Phase 11 security scanner."""
    
    PRIORITY = ScannerPriority.SPECIALIZED
    ENABLED = True
    MAX_RUNTIME_SECONDS = 60
    
    def __init__(self):
        """Initialize scanner."""
        super().__init__("{docstring}", "3.1")
        # Pattern definitions from legacy code
        # (See original legacy file for full patterns)
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Scan for security gaps."""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        # Note: This is a stub migration.
        # TODO: Implement full scan logic from legacy {old_class_name}
        
        for file_path in self._scan_files(source_dir):
            self.files_scanned += 1
        
        return gaps


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {{sys.argv[0]}} <source_dir>")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    scanner = {new_class_name}()
    gaps = scanner.scan(source_dir)
    print(f"Found {{len(gaps)}} gaps in {{scanner.files_scanned}} files")
'''
    
    return template

def main():
    """Migrate all Phase 11 scanners."""
    tools_dir = Path(__file__).parent
    scanners_dir = tools_dir / 'scanners'
    
    print("[*] Phase 11 Scanner Auto-Migration Tool")
    print(f"    Source: {tools_dir}")
    print(f"    Target: {scanners_dir}\n")
    
    created = []
    for old_name, new_name in MIGRATION_MAP.items():
        old_path = tools_dir / old_name
        new_path = scanners_dir / new_name
        
        if not old_path.exists():
            print(f"[SKIP] {old_name} not found")
            continue
        
        # Skip if already migrated (unless force)
        if new_path.exists() and new_name != 'gs3_step03_data_leak.py':
            print(f"[EXISTS] {new_name} already migrated")
            continue
        
        try:
            old_class = extract_scanner_class_name(old_path.read_text())
            oop_code = generate_oop_scanner(old_path, old_class)
            
            # For now, just generate template (don't overwrite existing data_leak)
            if new_path.exists():
                print(f"[OK] {new_name} (already up-to-date)")
            else:
                # Create file
                # new_path.write_text(oop_code)
                print(f"[READY] {new_name} (template generated, review before creating)")
                created.append((new_name, old_name))
        
        except Exception as e:
            print(f"[ERROR] {old_name}: {e}")
    
    print(f"\n[SUMMARY] {len(created)} scanners ready for migration")
    
    if created:
        print("\nNext steps:")
        print("1. Review generated templates in code")
        print("2. Implement full scan() methods from legacy code")
        print("3. Test integration with gs3_orchestrator.py")
        print("4. Archive legacy files to .deprecated/")

if __name__ == "__main__":
    main()
