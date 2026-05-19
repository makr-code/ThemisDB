#!/usr/bin/env python3
"""
Phase 3: Gap Loader - Extract scan results and organize gaps for code generation.

Loads gap_scan_v2_*.json and provides structured access to gaps grouped by:
- File path
- Severity (CRITICAL, HIGH, MEDIUM, LOW)
- Category (unimplemented, incomplete, etc.)
"""

import json
from pathlib import Path
from typing import Dict, List, Optional
from collections import defaultdict


class GapLoader:
    """Load and organize gap scan results for code generation."""
    
    def __init__(self, module: str):
        self.module = module
        self.scan_file = Path(f"ai_working/gap_scan_v2_{module}.json")
        self.data = None
        self.gaps_by_file = {}
        self.gaps_by_severity = defaultdict(list)
        
    def load(self) -> bool:
        """Load scan results from JSON file."""
        if not self.scan_file.exists():
            print(f"[ERROR] Scan file not found: {self.scan_file}")
            return False
        
        try:
            with open(self.scan_file) as f:
                self.data = json.load(f)
            self._organize_gaps()
            return True
        except Exception as e:
            print(f"[ERROR] Failed to load {self.scan_file}: {e}")
            return False
    
    def _organize_gaps(self):
        """Organize gaps by file and severity."""
        self.gaps_by_file = self.data.get("gaps_by_file", {})
        
        # Group by severity
        for filepath, gaps in self.gaps_by_file.items():
            for gap in gaps:
                severity = gap.get("severity", "unknown").upper()
                self.gaps_by_severity[severity].append({
                    "file": filepath,
                    **gap
                })
    
    def get_stats(self) -> Dict:
        """Return module statistics."""
        return self.data.get("stats", {})
    
    def get_gaps_by_severity(self, severity: str) -> List[Dict]:
        """Get all gaps of a specific severity level.
        
        Args:
            severity: "CRITICAL", "HIGH", "MEDIUM", "LOW"
            
        Returns:
            List of gap entries with file path included
        """
        return self.gaps_by_severity.get(severity.upper(), [])
    
    def get_gaps_by_file(self, filepath: str) -> List[Dict]:
        """Get all gaps in a specific file."""
        return self.gaps_by_file.get(filepath, [])
    
    def get_top_files_by_gap_count(self, limit: int = 10) -> List[tuple]:
        """Return files with most gaps.
        
        Returns:
            List of (filepath, gap_count) tuples
        """
        files = [
            (path, len(gaps))
            for path, gaps in self.gaps_by_file.items()
        ]
        return sorted(files, key=lambda x: x[1], reverse=True)[:limit]
    
    def get_gap_summary(self) -> Dict:
        """Get summary statistics."""
        stats = self.get_stats()
        severity_counts = {
            severity: len(gaps)
            for severity, gaps in self.gaps_by_severity.items()
        }
        
        return {
            "module": self.module,
            "total_gaps": stats.get("total", 0),
            "by_severity": severity_counts,
            "by_category": {
                "unimplemented": stats.get("unimplemented", 0),
                "incomplete": stats.get("incomplete", 0),
                "stub_documented": stats.get("stub_documented", 0),
                "platform_fallback": stats.get("platform_fallback", 0),
                "other": stats.get("total", 0) - (
                    stats.get("unimplemented", 0) +
                    stats.get("incomplete", 0) +
                    stats.get("stub_documented", 0) +
                    stats.get("platform_fallback", 0)
                )
            },
            "files_affected": len(self.gaps_by_file)
        }
    
    def get_batch_for_generation(self, 
                                 severity: str = "CRITICAL",
                                 limit: int = 10,
                                 exclude_test_files: bool = True) -> List[Dict]:
        """Get a batch of gaps ready for code generation.
        
        Args:
            severity: Filter by severity level
            limit: Max number of gaps to return
            exclude_test_files: Skip test files
            
        Returns:
            List of gap entries with file + line context
        """
        gaps = self.get_gaps_by_severity(severity)
        
        if exclude_test_files:
            gaps = [g for g in gaps if not g.get("is_test", False)]
        
        return gaps[:limit]
    
    def print_summary(self):
        """Print human-readable summary."""
        summary = self.get_gap_summary()
        
        print(f"\n{'='*70}")
        print(f"  GAP SCAN SUMMARY: {self.module.upper()}")
        print(f"{'='*70}\n")
        
        print(f"Total Gaps: {summary['total_gaps']}")
        print(f"Files Affected: {summary['files_affected']}")
        print(f"\nBy Severity:")
        for severity, count in sorted(summary['by_severity'].items()):
            print(f"  {severity:12} {count:5} gaps")
        
        print(f"\nBy Category:")
        for category, count in sorted(summary['by_category'].items()):
            if count > 0:
                print(f"  {category:20} {count:5}")
        
        print(f"\nTop 5 Files by Gap Count:")
        for filepath, count in self.get_top_files_by_gap_count(5):
            print(f"  {filepath}: {count} gaps")
        
        print(f"\n{'='*70}\n")
    
    def export_batch_plan(self, output_file: str = None) -> Dict:
        """Export a generation plan as JSON.
        
        Plan structure:
        {
          "module": "index",
          "batch": [
            {
              "id": "1.1",
              "file": "src/index/ann_index.cpp",
              "line": 293,
              "category": "unimplemented",
              "severity": "critical",
              "snippet": "...",
              "context": {
                "before": ["line 288", "line 289", ...],
                "after": ["line 294", "line 295", ...]
              }
            }
          ]
        }
        """
        batch = self.get_batch_for_generation(severity="CRITICAL", limit=5)
        
        plan = {
            "module": self.module,
            "batch_count": len(batch),
            "batch": batch
        }
        
        if output_file:
            with open(output_file, 'w') as f:
                json.dump(plan, f, indent=2)
            print(f"[OK] Exported plan to {output_file}")
        
        return plan


def main():
    """Demo: Load and display gap information for a module."""
    
    import sys
    
    module = sys.argv[1] if len(sys.argv) > 1 else "index"
    
    loader = GapLoader(module)
    if not loader.load():
        return 1
    
    loader.print_summary()
    
    print(f"Sampling CRITICAL gaps for {module}:")
    print(f"{'='*70}\n")
    
    critical = loader.get_gaps_by_severity("CRITICAL")[:3]
    for i, gap in enumerate(critical, 1):
        print(f"{i}. {gap['file']}:{gap['line']}")
        print(f"   Category: {gap['category']}")
        print(f"   Severity: {gap['severity']}")
        print(f"   Snippet: {gap['snippet'][:60]}...")
        print()
    
    return 0


if __name__ == "__main__":
    exit(main())
