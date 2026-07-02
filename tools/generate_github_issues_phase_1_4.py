#!/usr/bin/env python3
"""
Generate GitHub Issues for Phase 1-4 Scanner Gaps

This script converts Phase 1-4 gap reports into GitHub issues with proper
categorization, labeling, and priority assignment.

Usage:
    python3 tools/generate_github_issues_phase_1_4.py \
        --security ai_working/fp_tuning_after/gap_scan_v3_security_aggregate.json \
        --memory ai_working/fp_tuning_after/gap_scan_v3_memory_aggregate.json \
        --concurrency ai_working/fp_tuning_after/gap_scan_v3_concurrency_aggregate.json \
        --batch A \
        --priority CRITICAL \
        --dry-run

Author: AI Agent (Phase 1-4 Implementation)
Date: 2026-07-02
"""

import json
import argparse
import sys
from pathlib import Path
from typing import Dict, List, Any, Tuple
from dataclasses import dataclass


@dataclass
class GithubIssueTemplate:
    """Template for GitHub issue generation"""
    title: str
    body: str
    labels: List[str]
    priority: str
    batch: str
    pattern_type: str


class PhaseGapIssueGenerator:
    """Generate GitHub issues from Phase 1-4 gap reports"""

    def __init__(self, batch: str = "A", priority: str = "CRITICAL"):
        self.batch = batch
        self.priority = priority
        self.batch_mapping = {
            "A": ("XXE", ["security", "phase-1-4-batch-a"]),
            "B": ("Format-Strings & ReDoS", ["security", "phase-1-4-batch-b"]),
            "C": ("Iterator-Invalidation", ["memory", "phase-1-4-batch-c"]),
            "D": ("Use-After-Move", ["memory", "phase-1-4-batch-d"]),
            "E": ("Concurrency & Misc", ["concurrency", "phase-1-4-batch-e"]),
        }
        self.cwe_descriptions = {
            "CWE-611": "Improper Restriction of XML External Entity Reference",
            "CWE-134": "Use of Externally-Controlled Format String",
            "CWE-1333": "Inefficient Regular Expression Complexity",
            "CWE-416": "Use After Free",
            "CWE-362": "Concurrent Execution using Shared Resource",
            "CWE-327": "Use of a Broken or Risky Cryptographic Algorithm",
            "CWE-798": "Use of Hard-Coded Credentials",
        }

    def load_gap_report(self, filepath: str) -> Dict[str, Any]:
        """Load JSON gap report"""
        try:
            with open(filepath, 'r') as f:
                return json.load(f)
        except (FileNotFoundError, json.JSONDecodeError) as e:
            print(f"Error loading {filepath}: {e}", file=sys.stderr)
            return {}

    def extract_top_gaps(self, report: Dict[str, Any], limit: int = 20) -> List[Dict[str, Any]]:
        """Extract top gaps by severity and module"""
        gaps = []

        for module, module_data in report.items():
            if not isinstance(module_data, dict) or 'gaps_by_file' not in module_data:
                continue

            module_total = module_data.get('total', 0)
            severity_critical = module_data.get('severity_critical', 0)
            severity_high = module_data.get('severity_high', 0)

            # Filter by priority
            if self.priority == "CRITICAL" and severity_critical == 0:
                continue
            if self.priority == "HIGH" and severity_high == 0 and severity_critical == 0:
                continue

            # Extract individual gaps
            for filepath, file_gaps in module_data['gaps_by_file'].items():
                for gap in file_gaps:
                    gap['module'] = module
                    gap['severity_critical'] = severity_critical
                    gap['module_total'] = module_total
                    gaps.append(gap)

        # Sort by module impact (total gaps in module)
        gaps.sort(key=lambda g: (g.get('module_total', 0)), reverse=True)
        return gaps[:limit]

    def create_issue_template(self, gap: Dict[str, Any], gap_index: int) -> GithubIssueTemplate:
        """Create GitHub issue template from gap"""
        module = gap.get('module', 'unknown')
        filepath = gap.get('file', 'unknown').replace('\\', '/')
        line_no = gap.get('line', '?')
        gap_type = gap.get('type', 'unknown')
        severity = gap.get('severity', 'UNKNOWN')
        snippet = gap.get('snippet', '').strip()[:100]
        description = gap.get('description', 'Security gap detected by Phase 1-4 scanner')
        remediation = gap.get('remediation', 'Review and apply secure coding practices')

        batch_name, batch_labels = self.batch_mapping.get(self.batch, ("Unknown", []))

        title = f"[Phase 1-4 Batch {self.batch}] {gap_type} in {module} module ({severity})"
        
        labels = batch_labels + [
            f"severity-{severity.lower()}",
            f"cwe-{gap_type.split('_')[0]}",
            "gap-remediation",
            "phase-1-4-enhancements",
        ]

        body = f"""## {description}

**Module**: `{module}`  
**Severity**: {severity}  
**Pattern**: {gap_type}  
**Batch**: Phase 1-4 Batch {self.batch} ({batch_name})

### Location
- **File**: `{filepath}`
- **Line**: {line_no}
- **Code**: `{snippet}`

### Gap Details
{description}

### Remediation Steps
{remediation}

### Acceptance Criteria
- [ ] Gap verified as true positive (code review)
- [ ] Fix implemented and tested
- [ ] Regression test added
- [ ] No new gaps introduced

### References
- Phase 1-4 Enhancements: [PHASE_1_4_ENHANCEMENTS_DELIVERY_REPORT.md](ai_working/PHASE_1_4_ENHANCEMENTS_DELIVERY_REPORT.md)
- Remediation Batches: [PHASE_1_4_REMEDIATION_BATCHES.md](ai_working/PHASE_1_4_REMEDIATION_BATCHES.md)

---
**Generated by**: Phase 1-4 Issue Generator  
**Date**: 2026-07-02  
**Gap Index**: {gap_index}
"""

        return GithubIssueTemplate(
            title=title,
            body=body,
            labels=labels,
            priority=severity,
            batch=self.batch,
            pattern_type=gap_type
        )

    def generate_batch_a_issues(self, security_report: Dict[str, Any]) -> List[GithubIssueTemplate]:
        """Generate issues for Batch A (XXE vulnerabilities)"""
        gaps = self.extract_top_gaps(security_report, limit=25)
        
        # Filter to XXE-related gaps
        xxe_gaps = [g for g in gaps if 'xxe' in g.get('type', '').lower() or 'xml' in g.get('description', '').lower()]
        
        if not xxe_gaps:
            # If no XXE-specific filtering, use all top gaps
            xxe_gaps = gaps[:10]
        
        templates = []
        for idx, gap in enumerate(xxe_gaps, 1):
            templates.append(self.create_issue_template(gap, idx))
        
        return templates

    def generate_batch_b_issues(self, security_report: Dict[str, Any]) -> List[GithubIssueTemplate]:
        """Generate issues for Batch B (Format Strings + ReDoS)"""
        gaps = self.extract_top_gaps(security_report, limit=25)
        
        format_redos_gaps = [g for g in gaps if any(
            x in g.get('type', '').lower() 
            for x in ['format', 'regex', 'redos']
        )]
        
        if not format_redos_gaps:
            format_redos_gaps = gaps[:10]
        
        templates = []
        for idx, gap in enumerate(format_redos_gaps, 1):
            templates.append(self.create_issue_template(gap, idx))
        
        return templates

    def generate_batch_c_issues(self, memory_report: Dict[str, Any]) -> List[GithubIssueTemplate]:
        """Generate issues for Batch C (Iterator Invalidation)"""
        gaps = self.extract_top_gaps(memory_report, limit=25)
        
        iterator_gaps = [g for g in gaps if 'iterator' in g.get('type', '').lower()]
        
        if not iterator_gaps:
            iterator_gaps = gaps[:10]
        
        templates = []
        for idx, gap in enumerate(iterator_gaps, 1):
            templates.append(self.create_issue_template(gap, idx))
        
        return templates

    def generate_batch_d_issues(self, memory_report: Dict[str, Any]) -> List[GithubIssueTemplate]:
        """Generate issues for Batch D (Use-After-Move)"""
        gaps = self.extract_top_gaps(memory_report, limit=25)
        
        move_gaps = [g for g in gaps if 'move' in g.get('type', '').lower()]
        
        if not move_gaps:
            move_gaps = gaps[:10]
        
        templates = []
        for idx, gap in enumerate(move_gaps, 1):
            templates.append(self.create_issue_template(gap, idx))
        
        return templates

    def generate_batch_e_issues(self, concurrency_report: Dict[str, Any]) -> List[GithubIssueTemplate]:
        """Generate issues for Batch E (Concurrency & Misc)"""
        gaps = self.extract_top_gaps(concurrency_report, limit=20)
        
        templates = []
        for idx, gap in enumerate(gaps, 1):
            templates.append(self.create_issue_template(gap, idx))
        
        return templates

    def export_issues_json(self, templates: List[GithubIssueTemplate], output_file: str) -> None:
        """Export issue templates to JSON"""
        issues = []
        for template in templates:
            issues.append({
                "title": template.title,
                "body": template.body,
                "labels": template.labels,
                "priority": template.priority,
                "batch": template.batch,
                "pattern_type": template.pattern_type,
            })
        
        with open(output_file, 'w') as f:
            json.dump(issues, f, indent=2)
        
        print(f"✅ Exported {len(issues)} issues to {output_file}")

    def export_issues_markdown(self, templates: List[GithubIssueTemplate], output_file: str) -> None:
        """Export issue templates to Markdown"""
        with open(output_file, 'w') as f:
            f.write(f"# Phase 1-4 Batch {self.batch} GitHub Issues\n\n")
            f.write(f"**Generated**: 2026-07-02\n")
            f.write(f"**Total Issues**: {len(templates)}\n")
            f.write(f"**Priority**: {self.priority}\n\n")
            
            for idx, template in enumerate(templates, 1):
                f.write(f"## Issue {idx}: {template.title}\n\n")
                f.write(f"**Labels**: {', '.join(template.labels)}\n\n")
                f.write(template.body)
                f.write("\n\n---\n\n")
        
        print(f"✅ Exported {len(templates)} issues to {output_file}")

    def print_issue_summary(self, templates: List[GithubIssueTemplate]) -> None:
        """Print issue summary to console"""
        print(f"\n📊 Phase 1-4 Batch {self.batch} Issue Summary")
        print(f"=" * 60)
        print(f"Total Issues: {len(templates)}")
        print(f"Priority: {self.priority}")
        print(f"Batch Name: {self.batch_mapping.get(self.batch, ('Unknown', []))[0]}")
        print()
        
        # Group by type
        types = {}
        for t in templates:
            pattern = t.pattern_type
            types[pattern] = types.get(pattern, 0) + 1
        
        print("Issues by Type:")
        for pattern, count in sorted(types.items(), key=lambda x: -x[1]):
            print(f"  - {pattern}: {count}")
        
        print()


def main():
    parser = argparse.ArgumentParser(description="Generate GitHub issues from Phase 1-4 gap reports")
    parser.add_argument("--security", help="Path to security gap report JSON")
    parser.add_argument("--memory", help="Path to memory gap report JSON")
    parser.add_argument("--concurrency", help="Path to concurrency gap report JSON")
    parser.add_argument("--batch", choices=["A", "B", "C", "D", "E"], default="A", help="Batch to generate issues for")
    parser.add_argument("--priority", choices=["CRITICAL", "HIGH"], default="CRITICAL", help="Priority filter")
    parser.add_argument("--output-json", help="Output JSON file for issues")
    parser.add_argument("--output-md", help="Output Markdown file for issues")
    parser.add_argument("--dry-run", action="store_true", help="Print summary without generating files")
    args = parser.parse_args()

    generator = PhaseGapIssueGenerator(batch=args.batch, priority=args.priority)

    # Load reports
    security_report = generator.load_gap_report(args.security) if args.security else {}
    memory_report = generator.load_gap_report(args.memory) if args.memory else {}
    concurrency_report = generator.load_gap_report(args.concurrency) if args.concurrency else {}

    # Generate batch-specific issues
    batch_generators = {
        "A": lambda: generator.generate_batch_a_issues(security_report),
        "B": lambda: generator.generate_batch_b_issues(security_report),
        "C": lambda: generator.generate_batch_c_issues(memory_report),
        "D": lambda: generator.generate_batch_d_issues(memory_report),
        "E": lambda: generator.generate_batch_e_issues(concurrency_report),
    }

    templates = batch_generators[args.batch]()

    # Print summary
    generator.print_issue_summary(templates)

    if not args.dry_run:
        # Export if output files specified
        if args.output_json:
            generator.export_issues_json(templates, args.output_json)
        if args.output_md:
            generator.export_issues_markdown(templates, args.output_md)
        
        if not args.output_json and not args.output_md:
            print("✅ Use --output-json or --output-md to export issues")


if __name__ == "__main__":
    main()
