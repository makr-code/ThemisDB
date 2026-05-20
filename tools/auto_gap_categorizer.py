#!/usr/bin/env python3
"""
QUICK WIN #2: Auto Gap Categorization
Effort: 2 hours
Value: Speeds up Phase 1 by 70%
"""

import json
from pathlib import Path
from typing import Dict, List, Any
from collections import defaultdict

class GapCategorizer:
    """Automatically categorize gaps by type and severity"""
    
    CATEGORIES = {
        "Security": ["buffer", "overflow", "injection", "xss", "sql", "auth", "validation", "sanitize", "encrypt"],
        "Memory": ["leak", "dangling", "use-after-free", "uninitialized", "alignment", "arena"],
        "Concurrency": ["race", "deadlock", "mutex", "atomic", "thread-safe", "lock", "synchronization"],
        "Performance": ["cache", "allocation", "loop", "string", "vector", "optimization", "benchmark"],
        "Reliability": ["error-handling", "exception", "crash", "recovery", "rollback", "fault"],
        "RAII": ["smart-pointer", "unique_ptr", "shared_ptr", "ownership", "resource", "dtor"],
        "Type Safety": ["cast", "const", "volatile", "type-checking", "narrowing", "overflow"],
        "API Design": ["interface", "contract", "signature", "documentation", "versioning"],
    }
    
    def __init__(self, aggregate_path: str):
        with open(aggregate_path, 'r') as f:
            self.aggregate = json.load(f)
    
    def categorize_gap(self, gap: Dict[str, Any]) -> str:
        """Determine category for a single gap"""
        description = gap.get("description", "").lower()
        location = gap.get("location", "").lower()
        
        # Score each category
        scores = defaultdict(int)
        for category, keywords in self.CATEGORIES.items():
            for keyword in keywords:
                if keyword in description or keyword in location:
                    scores[category] += 1
        
        # Return highest scoring category
        if scores:
            return max(scores, key=scores.get)
        return "Other"
    
    def categorize_module(self, module: str) -> Dict[str, List[Dict]]:
        """Categorize all gaps in a module"""
        module_data = self.aggregate.get(module, {})
        
        # Handle both formats: dict with "gaps" key and direct list
        if isinstance(module_data, dict):
            gaps = module_data.get("gaps", [])
        elif isinstance(module_data, list):
            gaps = module_data
        else:
            gaps = []
        
        categorized = defaultdict(list)
        for gap in gaps:
            category = self.categorize_gap(gap)
            if isinstance(gap, dict):
                gap_with_category = {**gap, "category": category}
            else:
                gap_with_category = {"gap": gap, "category": category}
            categorized[category].append(gap_with_category)
        
        return dict(categorized)
    
    def generate_phase1_report(self, module: str, categorized: Dict[str, List]) -> str:
        """Generate Phase 1 audit report"""
        
        total_gaps = sum(len(gaps) for gaps in categorized.values())
        
        report = f"""# Phase 1 Audit Report: {module.upper()}

## Executive Summary
- **Total Gaps:** {total_gaps}
- **Categories:** {len(categorized)}
- **Avg per category:** {total_gaps // len(categorized) if categorized else 0}

## Categorization Breakdown

"""
        
        # Sort by gap count descending
        sorted_cats = sorted(categorized.items(), key=lambda x: len(x[1]), reverse=True)
        
        for category, gaps in sorted_cats:
            critical_count = sum(1 for g in gaps if g.get("severity") == "CRITICAL")
            high_count = sum(1 for g in gaps if g.get("severity") == "HIGH")
            
            report += f"""### {category} ({len(gaps)} gaps)
- CRITICAL: {critical_count}
- HIGH: {high_count}

**Sample gaps:**
"""
            # Show first 3 gaps
            for gap in gaps[:3]:
                location = gap.get("location", "unknown")
                severity = gap.get("severity", "UNKNOWN")
                report += f"- [{severity}] {location}\n"
            
            report += "\n"
        
        return report
    
    def generate_json_output(self, module: str, categorized: Dict[str, List]) -> Dict[str, Any]:
        """Generate structured JSON output"""
        return {
            "module": module,
            "total_gaps": sum(len(gaps) for gaps in categorized.values()),
            "categories": {
                cat: {
                    "count": len(gaps),
                    "critical": sum(1 for g in gaps if g.get("severity") == "CRITICAL"),
                    "high": sum(1 for g in gaps if g.get("severity") == "HIGH"),
                    "medium": sum(1 for g in gaps if g.get("severity") == "MEDIUM"),
                    "samples": [g.get("location", "unknown") for g in gaps[:3]]
                }
                for cat, gaps in categorized.items()
            }
        }

def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Auto-categorize gaps by type")
    parser.add_argument("aggregate", help="Path to gap_scan_v3_aggregate.json")
    parser.add_argument("--module", required=True, help="Module to categorize")
    parser.add_argument("--output", default="phase1_report.md", help="Output file")
    
    args = parser.parse_args()
    
    # Load and categorize
    categorizer = GapCategorizer(args.aggregate)
    categorized = categorizer.categorize_module(args.module)
    
    # Generate report
    report = categorizer.generate_phase1_report(args.module, categorized)
    
    # Write to file
    with open(args.output, 'w') as f:
        f.write(report)
    
    print(f"[OK] Phase 1 report generated: {args.output}")
    
    # Also output JSON
    json_output = categorizer.generate_json_output(args.module, categorized)
    json_file = args.output.replace('.md', '.json')
    with open(json_file, 'w') as f:
        json.dump(json_output, f, indent=2)
    
    print(f"[OK] JSON output: {json_file}")

if __name__ == "__main__":
    main()

# USAGE:
# python auto_gap_categorizer.py ai_working/gap_scan_v3_aggregate.json --module llm
# python auto_gap_categorizer.py ai_working/gap_scan_v3_aggregate.json --module server --output server_phase1.md
