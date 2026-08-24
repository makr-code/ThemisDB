#!/usr/bin/env python3
"""
L0.5 Gap Verification Runner
- Aggregates all module gap scans into comprehensive L0 findings
- Performs semantic code review with false-positive elimination
- Re-assesses severity based on source context
- Generates verified findings JSON and markdown report
"""

import json
import os
import re
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple
from collections import defaultdict
from datetime import datetime

# ============================================================================
# Configuration
# ============================================================================

WORKSPACE_ROOT = Path("c:\\Projects\\ThemisDB")
AI_WORKING = WORKSPACE_ROOT / "ai_working"
SRC_ROOT = WORKSPACE_ROOT / "src"

# Module scanning file patterns (v3 latest version)
MODULE_SCAN_PATTERN = "gap_scan_v3_*.json"

# Classification types
ClassificationType = type("ClassificationType", (), {
    "REAL_GAP": "REAL_GAP",
    "GUARDED_STUB": "GUARDED_STUB",
    "TEST_MOCK": "TEST_MOCK",
    "PLACEHOLDER": "PLACEHOLDER",
    "FALSE_POSITIVE": "FALSE_POSITIVE",
})

# ============================================================================
# Utilities
# ============================================================================

def read_json_file(path: Path) -> Optional[Dict]:
    """Safely read JSON file."""
    try:
        with open(path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception as e:
        print(f"[WARN] Failed to read {path}: {e}")
        return None

def write_json_file(path: Path, data: Dict, indent: int = 2) -> None:
    """Write JSON file with formatting."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=indent, ensure_ascii=False)
    print(f"[OK] Written: {path}")

def read_source_context(file_path: Path, line_num: int, context_lines: int = 5) -> Optional[str]:
    """Read source code context around a line."""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        start = max(0, line_num - context_lines - 1)
        end = min(len(lines), line_num + context_lines)
        
        context = "".join(lines[start:end])
        return context
    except Exception as e:
        return None

def normalize_path(path_str: str) -> str:
    """Normalize path separators to forward slashes."""
    return path_str.replace("\\", "/")

# ============================================================================
# Classification Logic
# ============================================================================

class GapClassifier:
    """Classifies gaps as REAL_GAP or FALSE_POSITIVE with reasons."""
    
    def __init__(self, workspace_root: Path):
        self.workspace_root = workspace_root
        # Patterns that clearly indicate false-positives (scanner misidentifications)
        self.clear_fp_patterns = [
            (r"\.load\(.*memory_order", "atomic::load() is not a resource acquire"),
            (r"\.store\(.*memory_order", "atomic::store() is not a resource release"),
            (r"\.exchange\(", "atomic::exchange() is not resource management"),
            (r"std::memory_order_", "Memory ordering flag, not resource allocation"),
            (r"// Register.*backend", "Comment marking registration, not code gap"),
            (r"// Supports.*OpenCL|// Register OpenGL", "Comment explaining support, not code gap"),
            (r"std::vector.*reserve", "Vector reserve() is optimization, not gap"),
            (r"\[\s*nodiscard\s*\]", "nodiscard attribute is not a gap"),
            (r"_WIN32", "Platform macro, not a gap"),
            (r"inline\s+constexpr", "Compiler directive, not a gap"),
            (r"using.*=.*;", "Type alias, not a gap"),
            (r"static.*const.*=", "Static const definition, not a gap"),
            (r"\[\[.*\]\]", "C++ attribute, not a gap"),
        ]
    
    def classify(self, finding: Dict) -> Tuple[str, str, str]:
        """
        Classify a finding and return (classification, verified_severity, rationale).
        
        Returns:
            Tuple of (classification, severity, rationale)
        """
        file_path = finding.get("file", "")
        line_num = finding.get("line", 0)
        pattern = finding.get("pattern", "")
        description = finding.get("description", "")
        context = finding.get("context", "")
        original_severity = finding.get("severity", "MEDIUM")
        snippet = finding.get("snippet", "")
        
        # Resolve actual file path
        if file_path.startswith("src"):
            actual_path = self.workspace_root / file_path
        else:
            actual_path = self.workspace_root / "src" / file_path
        
        # Read source context
        source_context = read_source_context(actual_path, line_num, context_lines=7)
        
        # ======== CLASSIFICATION DECISION MATRIX ========
        
        # 1. FILE_NOT_FOUND - Should have been filtered in Phase 1
        if not actual_path.exists():
            return (ClassificationType.FALSE_POSITIVE, "—", "File not found (Phase 1 miss)")
        
        # 2. CLEAR FALSE-POSITIVE PATTERNS
        full_context = (source_context or "") + " " + (snippet or "") + " " + (context or "")
        for fp_pattern, reason in self.clear_fp_patterns:
            if re.search(fp_pattern, full_context, re.IGNORECASE):
                return (ClassificationType.FALSE_POSITIVE, "—", f"Scanner false-positive: {reason}")
        
        # 3. Misidentified resource patterns
        misidentified_patterns = {
            "db_connection_leak": r"(\.load\(|\.store\(|memory_order)",
            "uninitialized_access": r"PR History|commit_history|git_history",
            "pointer_arithmetic": r"(\[\[nodiscard\]\]|unique_ptr.*create)",
        }
        for pattern_key, pattern_re in misidentified_patterns.items():
            if pattern == pattern_key and re.search(pattern_re, full_context):
                return (ClassificationType.FALSE_POSITIVE, "—", f"Misidentified pattern: {pattern_key}")
        
        # 4. TEST_MOCK - In test files with explicit markers
        if "test_" in file_path or "_test.cpp" in file_path:
            if any(marker in (source_context or "") for marker in ["// MOCK", "// TEST", "MOCK:", "TEST_FIXTURE"]):
                return (ClassificationType.TEST_MOCK, "INFO", "Test fixture mock, not production")
        
        # 5. PLACEHOLDER - Has TODO/FIXME/STUB/TEMPORARY markers
        todo_markers = ["// TODO", "// FIXME", "// STUB", "// TEMPORARY", "// PLACEHOLDER"]
        if source_context and any(marker in source_context for marker in todo_markers):
            return (ClassificationType.PLACEHOLDER, "MEDIUM", f"Marked placeholder (Phase N+1 work)")
        
        # 6. GUARDED_STUB - Early return with guard checks
        if source_context:
            # Pattern: `if (!condition) return {}` or `if (error_condition) return error;`
            guard_patterns = [
                r"if\s*\(\s*![a-zA-Z_].*\)\s*return",
                r"if\s*\(\s*nullptr\s*==",
                r"if\s*\(\s*!initialized",
                r"if\s*\(\s*error",
                r"if\s*\(\s*fail",
            ]
            for pattern_re in guard_patterns:
                if re.search(pattern_re, source_context):
                    # Downgrade from CRITICAL to HIGH
                    new_severity = "HIGH" if original_severity == "CRITICAL" else original_severity
                    return (ClassificationType.GUARDED_STUB, new_severity, 
                            "Defensive pattern with early return guard. Not an unimplemented gap.")
        
        # 7. Legacy/compat markers (these might be intentional in compatibility layers)
        if "legacy_or_compat_path" == pattern:
            # If it's just a comment about legacy support, it's informational
            if source_context and ("// " in source_context or "/*" in source_context):
                # Count if it's mostly comments
                comment_lines = sum(1 for line in (source_context or "").split("\n") if "//" in line or "/*" in line)
                total_lines = len((source_context or "").split("\n"))
                if comment_lines > total_lines * 0.7:
                    return (ClassificationType.FALSE_POSITIVE, "—", "Legacy compatibility documentation, not a code gap")
        
        # 8. REAL_GAP - Unimplemented production code
        # Default: if none of the above patterns match, it's a real gap
        return (ClassificationType.REAL_GAP, original_severity, "Unimplemented production code")
    
    def get_severity_downgrade(self, classification: str, original: str) -> str:
        """Determine if severity should be downgraded based on classification."""
        downgrades = {
            ClassificationType.GUARDED_STUB: {
                "CRITICAL": "HIGH",
                "HIGH": "HIGH",
                "MEDIUM": "MEDIUM",
                "LOW": "LOW",
            },
            ClassificationType.PLACEHOLDER: {
                "CRITICAL": "MEDIUM",
                "HIGH": "MEDIUM",
                "MEDIUM": "LOW",
                "LOW": "LOW",
            },
            ClassificationType.TEST_MOCK: {
                "CRITICAL": "INFO",
                "HIGH": "INFO",
                "MEDIUM": "INFO",
                "LOW": "INFO",
            },
        }
        
        if classification in downgrades:
            return downgrades[classification].get(original, original)
        return original

# ============================================================================
# L0 Aggregation & Verification
# ============================================================================

class L0Verifier:
    """Aggregates module scans and performs verification."""
    
    def __init__(self, workspace_root: Path):
        self.workspace_root = workspace_root
        self.ai_working = workspace_root / "ai_working"
        self.classifier = GapClassifier(workspace_root)
        self.findings_raw: List[Dict] = []
        self.findings_verified: List[Dict] = []
        self.stats = {
            "total_reviewed": 0,
            "verified_gaps": 0,
            "false_positives": 0,
            "downgrades": 0,
            "severity_changes": defaultdict(int),
            "by_classification": defaultdict(int),
        }
    
    def aggregate_module_scans(self) -> None:
        """Load all module scan files and aggregate findings."""
        print(f"\n[*] Aggregating module scans from {self.ai_working}...")
        
        # Find all module scan files
        scan_files = sorted(self.ai_working.glob(MODULE_SCAN_PATTERN))
        print(f"Found {len(scan_files)} module scan files")
        
        for scan_file in scan_files:
            data = read_json_file(scan_file)
            if not data:
                continue
            
            # V3 structure: {module_name: {by_file: {file: [findings...]}, ...}}
            # Iterate through module entries
            for module_name, module_data in data.items():
                if isinstance(module_data, dict):
                    by_file = module_data.get("by_file", {})
                    if isinstance(by_file, dict):
                        # Extract findings from all files in this module
                        for file_path, file_findings in by_file.items():
                            if isinstance(file_findings, list):
                                self.findings_raw.extend(file_findings)
        
        # Normalize and deduplicate
        self.findings_raw = self._deduplicate_findings(self.findings_raw)
        self.stats["total_reviewed"] = len(self.findings_raw)
        print(f"[+] Aggregated {len(self.findings_raw)} raw findings")
    
    def _deduplicate_findings(self, findings: List[Dict]) -> List[Dict]:
        """Remove duplicate findings by (file, line, pattern)."""
        seen = set()
        unique = []
        for f in findings:
            key = (f.get("file"), f.get("line"), f.get("pattern"))
            if key not in seen:
                seen.add(key)
                unique.append(f)
        return unique
    
    def verify_findings(self) -> None:
        """Verify each finding and classify as REAL_GAP or FALSE_POSITIVE."""
        print(f"\n[*] Verifying {len(self.findings_raw)} findings...")
        
        for idx, finding in enumerate(self.findings_raw, 1):
            if idx % 500 == 0:
                print(f"  Progress: {idx}/{len(self.findings_raw)}")
            
            classification, verified_severity, rationale = self.classifier.classify(finding)
            
            # Build verified finding
            verified_finding = {
                **finding,
                "l0_5_classification": classification,
                "l0_5_original_severity": finding.get("severity", "MEDIUM"),
                "l0_5_verified_severity": verified_severity,
                "l0_5_rationale": rationale,
                "l0_5_verified": True,
            }
            
            # Track statistics
            self.stats["by_classification"][classification] += 1
            
            if classification == ClassificationType.FALSE_POSITIVE:
                self.stats["false_positives"] += 1
            else:
                self.stats["verified_gaps"] += 1
                self.findings_verified.append(verified_finding)
                
                if verified_severity != finding.get("severity"):
                    self.stats["downgrades"] += 1
                    key = f"{finding.get('severity')}->{verified_severity}"
                    self.stats["severity_changes"][key] += 1
        
        print(f"[OK] Verification complete")
        self._print_stats()
    
    def _print_stats(self) -> None:
        """Print verification statistics."""
        total = self.stats["total_reviewed"]
        verified = self.stats["verified_gaps"]
        fp_rate = (self.stats["false_positives"] / total * 100) if total > 0 else 0
        
        print(f"\n[STATS] Verification Statistics:")
        print(f"  Total Reviewed:        {total}")
        print(f"  Verified Gaps:         {verified}")
        print(f"  False Positives:       {self.stats['false_positives']} ({fp_rate:.1f}%)")
        print(f"  Downgrades:            {self.stats['downgrades']}")
        
        print(f"\n  Classification Breakdown:")
        for cls_type, count in sorted(self.stats["by_classification"].items()):
            pct = count / total * 100 if total > 0 else 0
            print(f"    {cls_type:20s}: {count:5d} ({pct:5.1f}%)")
        
        if self.stats["severity_changes"]:
            print(f"\n  Severity Changes:")
            for change, count in sorted(self.stats["severity_changes"].items()):
                # Replace arrow with text for terminal compatibility
                display_change = change.replace("→", "->")
                print(f"    {display_change:20s}: {count}")
    
    def generate_verified_json(self) -> Dict:
        """Generate final verified findings JSON."""
        # Group by severity for summary
        severity_dist = defaultdict(int)
        for finding in self.findings_verified:
            severity = finding.get("l0_5_verified_severity", "UNKNOWN")
            severity_dist[severity] += 1
        
        output = {
            "metadata": {
                "orchestration_level": "L0.5",
                "operation": "Gap Verification with False-Positive Elimination (Full L0 Scan)",
                "execution_timestamp": datetime.now().isoformat(),
                "verification_type": "Semantic Code Pattern Analysis + Source Context Review",
                "target_fp_removal_rate": "70-80%",
                "actual_fp_removal_rate": f"{(self.stats['false_positives'] / self.stats['total_reviewed'] * 100):.1f}%" if self.stats['total_reviewed'] > 0 else "0%",
            },
            "summary": {
                "total_reviewed": self.stats["total_reviewed"],
                "false_positives_removed": self.stats["false_positives"],
                "verified_gaps": self.stats["verified_gaps"],
                "downgrades": self.stats["downgrades"],
                "severity_distribution": dict(severity_dist),
            },
            "classification_breakdown": dict(self.stats["by_classification"]),
            "findings": self.findings_verified,
        }
        
        return output
    
    def generate_markdown_report(self, verified_data: Dict) -> str:
        """Generate human-readable markdown report."""
        report = []
        report.append("# L0.5 Gap Verification Report\n")
        report.append(f"**Generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        report.append(f"**Operation**: Full L0 Scan Gap Verification\n\n")
        
        # Summary
        summary = verified_data["summary"]
        total = summary["total_reviewed"]
        report.append("## Summary\n")
        report.append(f"| Metric | Value |\n")
        report.append(f"|--------|-------|\n")
        report.append(f"| Total Reviewed | {total} |\n")
        report.append(f"| Verified Gaps | {summary['verified_gaps']} |\n")
        report.append(f"| False Positives Removed | {summary['false_positives_removed']} |\n")
        
        if total > 0:
            fp_rate = (summary['false_positives_removed']/total*100)
            report.append(f"| False-Positive Rate | {fp_rate:.1f}% |\n")
        else:
            report.append(f"| False-Positive Rate | N/A (no findings) |\n")
        
        report.append(f"| Downgrades | {summary['downgrades']} |\n\n")
        
        # If no findings, return early
        if total == 0:
            report.append("**Note**: No findings were aggregated from module scans.\n")
            return "".join(report)
        
        # Severity Distribution
        report.append("## Severity Distribution (Verified)\n")
        report.append("| Severity | Count |\n")
        report.append("|----------|-------|\n")
        for severity in ["CRITICAL", "HIGH", "MEDIUM", "LOW", "INFO"]:
            count = summary["severity_distribution"].get(severity, 0)
            report.append(f"| {severity} | {count} |\n")
        report.append("\n")
        
        # Classification Breakdown
        report.append("## Classification Breakdown\n")
        report.append("| Classification | Count | Rate |\n")
        report.append("|----------------|-------|------|\n")
        for cls_type in sorted(verified_data["classification_breakdown"].keys()):
            count = verified_data["classification_breakdown"][cls_type]
            rate = (count / total * 100) if total > 0 else 0
            report.append(f"| {cls_type} | {count} | {rate:.1f}% |\n")
        report.append("\n")
        
        # Top Finding Patterns (first 10)
        report.append("## Top Finding Patterns (Sample)\n")
        pattern_counts = defaultdict(int)
        for finding in verified_data["findings"][:100]:
            pattern = finding.get("pattern", "unknown")
            pattern_counts[pattern] += 1
        
        for pattern, count in sorted(pattern_counts.items(), key=lambda x: -x[1])[:10]:
            report.append(f"- **{pattern}**: {count} occurrences\n")
        report.append("\n")
        
        # Recommendations
        report.append("## Recommendations for L1 Remediation\n")
        fp_rate = (summary['false_positives_removed'] / total * 100) if total > 0 else 0
        
        if fp_rate >= 70:
            report.append(f"[OK] **False-Positive Removal Target Met** ({fp_rate:.1f}%)\n")
        else:
            report.append(f"[!] **False-Positive Removal Below Target** ({fp_rate:.1f}% vs 70% target)\n")
        
        report.append(f"- {summary['verified_gaps']} verified real gaps ready for L1 remediation\n")
        report.append(f"- Focus on {summary['severity_distribution'].get('CRITICAL', 0)} CRITICAL and {summary['severity_distribution'].get('HIGH', 0)} HIGH severity items first\n")
        report.append(f"- {summary['downgrades']} findings downgraded due to defensive patterns (guarded stubs)\n")
        report.append("\n")
        
        return "".join(report)
    
    def run(self) -> None:
        """Execute full L0.5 verification workflow."""
        print("=" * 70)
        print("L0.5 GAP VERIFICATION RUNNER - FULL L0 SCAN")
        print("=" * 70)
        
        self.aggregate_module_scans()
        self.verify_findings()
        
        # Generate outputs
        verified_data = self.generate_verified_json()
        report_md = self.generate_markdown_report(verified_data)
        
        # Write outputs
        verified_json_path = self.ai_working / "gap_scan_results_verified_L0.5_full.json"
        report_md_path = self.ai_working / "gap_verifier_report_L0.5_full.md"
        
        write_json_file(verified_json_path, verified_data)
        
        with open(report_md_path, 'w', encoding='utf-8') as f:
            f.write(report_md)
        print(f"[OK] Written: {report_md_path}")
        
        print("\n" + "=" * 70)
        print("[OK] L0.5 VERIFICATION COMPLETE")
        print("=" * 70)

# ============================================================================
# Main
# ============================================================================

if __name__ == "__main__":
    verifier = L0Verifier(WORKSPACE_ROOT)
    verifier.run()
