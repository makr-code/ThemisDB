#!/usr/bin/env python3
"""
Gap Scanner V3 — Phase 11 Security Integration Adapter

Loads and adapts legacy Phase 11 security scanners into the new OOP architecture.
Converts DataLeakGap, EncryptionGap, etc. to unified Gap representation.
"""

import sys
from pathlib import Path
from typing import List
import importlib.util

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class Phase11IntegrationScanner(BaseGapScanner):
    """Integration adapter for Phase 11 legacy security scanners."""
    
    PRIORITY = ScannerPriority.SPECIALIZED
    ENABLED = True
    MAX_RUNTIME_SECONDS = 120
    
    def __init__(self):
        """Initialize Phase 11 Integration Scanner."""
        super().__init__("Phase 11 Security Integration", "3.1")
        self.legacy_scanners = []
        self._load_legacy_scanners()
    
    def _load_legacy_scanners(self):
        """Dynamically load legacy Phase 11 scanner classes."""
        legacy_module_names = [
            'gap_scanner_v3_phase11_data_leak',
            'gap_scanner_v3_phase11_encryption_leak',
            'gap_scanner_v3_phase11_e2e_encryption',
            'gap_scanner_v3_phase11_key_failure',
            'gap_scanner_v3_phase11_attack_vectors',
            'gap_scanner_v3_phase11_military_hardening',
            'gap_scanner_v3_phase11_legacy_duplication',
        ]
        
        tools_path = Path(__file__).parent.parent
        
        for module_name in legacy_module_names:
            try:
                module_file = tools_path / f"{module_name}.py"
                if module_file.exists():
                    spec = importlib.util.spec_from_file_location(module_name, module_file)
                    module = importlib.util.module_from_spec(spec)
                    spec.loader.exec_module(module)
                    
                    # Find scanner class in module (assume class_name is CamelCase of module)
                    class_name = ''.join(
                        word.capitalize() for word in module_name.replace('gap_scanner_v3_', '')
                        .replace('_', ' ').replace('phase11 ', '').split()
                    ) + 'Scanner'
                    
                    if hasattr(module, class_name):
                        scanner_class = getattr(module, class_name)
                        self.legacy_scanners.append((module_name, scanner_class))
            except Exception as e:
                if self.name:
                    print(f"[WARNING] Failed to load {module_name}: {e}", file=sys.stderr)
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Run all legacy Phase 11 scanners and convert results to Gap format."""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        for module_name, scanner_class in self.legacy_scanners:
            try:
                # Instantiate legacy scanner
                legacy_scanner = scanner_class(str(self.source_path))
                
                # Run legacy scan method (varies by scanner)
                if hasattr(legacy_scanner, 'scan'):
                    legacy_gaps = legacy_scanner.scan()
                elif hasattr(legacy_scanner, 'run'):
                    legacy_gaps = legacy_scanner.run()
                else:
                    continue
                
                # Convert legacy gaps to new Gap format
                converted = self._convert_legacy_gaps(legacy_gaps, module_name)
                gaps.extend(converted)
            
            except Exception as e:
                print(f"[ERROR] {module_name}: {e}", file=sys.stderr)
        
        self.files_scanned = 1  # Aggregated count
        return self.deduplicate(gaps)
    
    def _convert_legacy_gaps(self, legacy_gaps: List, module_name: str) -> List[Gap]:
        """Convert legacy gap objects to unified Gap format."""
        converted = []
        
        for gap in legacy_gaps:
            try:
                # Handle different gap object types
                if hasattr(gap, 'to_dict'):
                    gap_dict = gap.to_dict()
                elif isinstance(gap, dict):
                    gap_dict = gap
                else:
                    # Try to extract attributes
                    gap_dict = {
                        'file': getattr(gap, 'file_path', str(gap)),
                        'line': getattr(gap, 'line_num', 1),
                        'type': getattr(gap, 'gap_type', type(gap).__name__),
                        'severity': getattr(gap, 'severity', 'HIGH'),
                        'confidence': getattr(gap, 'confidence', 0.7),
                        'description': getattr(gap, 'description', 'Security gap'),
                        'remediation': getattr(gap, 'remediation', 'Review and fix'),
                    }
                
                # Convert to new Gap format
                unified_gap = Gap(
                    file=gap_dict.get('file', ''),
                    line=gap_dict.get('line', 1),
                    type=str(gap_dict.get('type', 'unknown')).lower(),
                    severity=gap_dict.get('severity', 'MEDIUM'),
                    confidence=gap_dict.get('confidence', 0.65),
                    description=gap_dict.get('description', 'Security gap'),
                    remediation=gap_dict.get('remediation', 'Review'),
                    context=gap_dict.get('snippet', gap_dict.get('context', '')),
                    scanner='Phase 11 Security Suite',
                    step='03_phase11'
                )
                converted.append(unified_gap)
            
            except Exception as e:
                print(f"[WARNING] Failed to convert gap: {e}", file=sys.stderr)
        
        return converted


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <source_dir>")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    scanner = Phase11IntegrationScanner()
    
    print(f"[{scanner.name}] Starting scan...\n")
    gaps = scanner.scan(source_dir)
    
    # Organize by severity
    by_severity = {}
    by_type = {}
    for gap in gaps:
        by_severity[gap.severity] = by_severity.get(gap.severity, 0) + 1
        by_type[gap.type] = by_type.get(gap.type, 0) + 1
    
    print(f"\nFound {len(gaps)} security gaps")
    
    if by_severity:
        print(f"\nBy Severity:")
        for sev in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
            if sev in by_severity:
                print(f"  {sev}: {by_severity[sev]}")
    
    if by_type:
        print(f"\nTop Gap Types:")
        for typ, count in sorted(by_type.items(), key=lambda x: -x[1])[:10]:
            print(f"  {typ}: {count}")
