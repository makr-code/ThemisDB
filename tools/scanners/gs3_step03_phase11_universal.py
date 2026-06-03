#!/usr/bin/env python3
"""
Gap Scanner V3 — Universal Phase 11 Legacy Adapter

Loads all legacy gap_scanner_v3_phase11_*.py files dynamically and adapts them
to the new BaseGapScanner OOP architecture. This is a bridge for rapid integration
without rewriting all Phase 11 scanners individually.
"""

import sys
import importlib.util
from pathlib import Path
from typing import List, Dict, Any
import traceback

sys.path.insert(0, str(Path(__file__).parent.parent))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class Phase11UniversalAdapter(BaseGapScanner):
    """
    Universal adapter for all legacy Phase 11 security scanners.
    
    Dynamically loads legacy scanners and converts their output format to unified Gap.
    Supports multiple output types: to_dict(), dict fields, dataclass attributes.
    """
    
    PRIORITY = ScannerPriority.SPECIALIZED
    ENABLED = True
    MAX_RUNTIME_SECONDS = 120
    
    # Map of legacy scanner files to expected class names
    LEGACY_SCANNERS = {
        'gap_scanner_v3_phase11_data_leak': 'DataLeakScanner',
        'gap_scanner_v3_phase11_encryption_leak': 'EncryptionLeakScanner',
        'gap_scanner_v3_phase11_e2e_encryption': 'E2EEncryptionScanner',
        'gap_scanner_v3_phase11_key_failure': 'KeyFailureScanner',
        'gap_scanner_v3_phase11_attack_vectors': 'AttackVectorScanner',
        'gap_scanner_v3_phase11_military_hardening': 'MilitaryHardeningScanner',
        'gap_scanner_v3_phase11_legacy_duplication': 'LegacyDuplicationScanner',
    }
    
    def __init__(self):
        """Initialize Universal Phase 11 Adapter."""
        super().__init__("Phase 11 Security Suite (Universal Adapter)", "3.1")
        self.legacy_scanners = {}
        self._load_all_legacy_scanners()
    
    def _load_all_legacy_scanners(self):
        """Dynamically load all Phase 11 legacy scanners."""
        tools_path = Path(__file__).parent.parent
        
        for module_name, class_name in self.LEGACY_SCANNERS.items():
            try:
                module_file = tools_path / f"{module_name}.py"
                
                if not module_file.exists():
                    continue  # Skip if file doesn't exist
                
                # Dynamically import module
                spec = importlib.util.spec_from_file_location(module_name, module_file)
                if spec and spec.loader:
                    module = importlib.util.module_from_spec(spec)
                    spec.loader.exec_module(module)
                    
                    # Get the scanner class from module
                    if hasattr(module, class_name):
                        scanner_class = getattr(module, class_name)
                        self.legacy_scanners[module_name] = {
                            'class': scanner_class,
                            'instance': None,  # Will instantiate per scan
                        }
            
            except Exception as e:
                # Log but don't fail on individual scanner load errors
                pass  # Silently skip failed scanners
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Run all legacy Phase 11 scanners and convert results to Gap format."""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        for module_name, scanner_info in self.legacy_scanners.items():
            try:
                scanner_class = scanner_info['class']
                
                # Instantiate legacy scanner
                legacy_scanner = scanner_class(str(self.source_path))
                
                # Try different scan method names
                legacy_gaps = None
                if hasattr(legacy_scanner, 'scan'):
                    legacy_gaps = legacy_scanner.scan()
                elif hasattr(legacy_scanner, 'run'):
                    legacy_gaps = legacy_scanner.run()
                elif hasattr(legacy_scanner, 'scan_repo'):
                    legacy_gaps = legacy_scanner.scan_repo()
                
                if legacy_gaps:
                    # Convert legacy gaps to new Gap format
                    converted = self._convert_legacy_gaps(legacy_gaps, module_name)
                    gaps.extend(converted)
            
            except Exception as e:
                # Silently handle individual scanner errors; continue with others
                pass
        
        self.files_scanned = 1  # Aggregate count
        return self.deduplicate(gaps)
    
    def _convert_legacy_gaps(self, legacy_gaps: List[Any], module_name: str) -> List[Gap]:
        """
        Convert legacy gap objects (various formats) to unified Gap format.
        
        Handles multiple gap object types:
        - to_dict() method
        - Dictionary format
        - Dataclass with attributes
        - Named tuple
        """
        converted = []
        
        for gap in legacy_gaps:
            try:
                # Extract gap data from various formats
                gap_dict = self._extract_gap_data(gap)
                
                if not gap_dict:
                    continue
                
                # Normalize severity
                severity = self._normalize_severity(gap_dict.get('severity', 'MEDIUM'))
                
                # Build unified Gap object
                unified_gap = Gap(
                    file=gap_dict.get('file', gap_dict.get('file_path', '')),
                    line=gap_dict.get('line', gap_dict.get('line_num', 1)),
                    type=str(gap_dict.get('type', gap_dict.get('gap_type', 'unknown'))).lower(),
                    severity=severity,
                    confidence=float(gap_dict.get('confidence', 0.70)),
                    description=gap_dict.get('description', 'Security gap'),
                    remediation=gap_dict.get('remediation', 'Review and fix'),
                    context=gap_dict.get('snippet', gap_dict.get('context', '')),
                    scanner=f"Phase 11: {module_name.replace('gap_scanner_v3_phase11_', '')}",
                    step='03_phase11'
                )
                converted.append(unified_gap)
            
            except Exception:
                # Skip individual gaps that can't be converted
                pass
        
        return converted
    
    def _extract_gap_data(self, gap: Any) -> Dict[str, Any]:
        """Extract gap data from various object types."""
        
        # Try to_dict() method first (dataclass style)
        if hasattr(gap, 'to_dict') and callable(gap.to_dict):
            try:
                return gap.to_dict()
            except:
                pass
        
        # Try direct dict conversion
        if isinstance(gap, dict):
            return gap
        
        # Try dataclass fields (check for __dataclass_fields__)
        if hasattr(gap, '__dataclass_fields__'):
            result = {}
            for field_name in gap.__dataclass_fields__:
                result[field_name] = getattr(gap, field_name, None)
            return result
        
        # Fallback: extract known attributes
        result = {}
        for attr in ['file', 'file_path', 'line', 'line_num', 'type', 'gap_type', 
                     'severity', 'confidence', 'description', 'remediation', 'snippet', 'context']:
            if hasattr(gap, attr):
                value = getattr(gap, attr)
                # Normalize attribute names
                if 'file' in attr:
                    result['file'] = str(value)
                elif 'line' in attr:
                    result['line'] = int(value) if value else 1
                elif 'type' in attr:
                    result['type'] = str(value)
                else:
                    result[attr] = value
        
        return result if result else None
    
    def _normalize_severity(self, severity: Any) -> str:
        """Normalize severity to standard format."""
        sev_str = str(severity).upper()
        
        # Handle enum values
        if '.' in sev_str:
            sev_str = sev_str.split('.')[-1]
        
        # Map to standard severities
        if any(x in sev_str for x in ['CRITICAL', 'CRITICAL_FAILURE', 'EXTREME']):
            return 'CRITICAL'
        elif any(x in sev_str for x in ['HIGH', 'SEVERE']):
            return 'HIGH'
        elif any(x in sev_str for x in ['MEDIUM', 'WARNING', 'MODERATE']):
            return 'MEDIUM'
        else:
            return 'LOW'


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <source_dir>")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    adapter = Phase11UniversalAdapter()
    
    print(f"[{adapter.name}]")
    print(f"  Loaded scanners: {len(adapter.legacy_scanners)}")
    print(f"  Starting scan...\n")
    
    gaps = adapter.scan(source_dir)
    
    # Organize by severity
    by_severity = {}
    by_type = {}
    for gap in gaps:
        by_severity[gap.severity] = by_severity.get(gap.severity, 0) + 1
        by_type[gap.type] = by_type.get(gap.type, 0) + 1
    
    print(f"\n[RESULTS]")
    print(f"Total gaps: {len(gaps)}")
    
    if by_severity:
        print(f"\nBy Severity:")
        for sev in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
            if sev in by_severity:
                print(f"  {sev}: {by_severity[sev]}")
    
    if by_type:
        print(f"\nTop Gap Types:")
        for typ, count in sorted(by_type.items(), key=lambda x: -x[1])[:10]:
            print(f"  {typ}: {count}")
