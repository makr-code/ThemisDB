#!/usr/bin/env python3
"""
ThemisDB Gap Clustering: Group implementation gaps into actionable meta-issues.

Groups gaps by:
1. Category (stub, todo, unimplemented) — system-wide patterns
2. Module severity — critical, high-priority modules
3. Correlation — related functionality across modules
"""

import json
from pathlib import Path
from typing import Dict, List, Tuple
from dataclasses import dataclass
from datetime import datetime
from collections import defaultdict

@dataclass
class ClusteredIssue:
    id: str  # e.g., "META-001", "MOD-acceleration-001"
    title: str
    affected_modules: List[str]
    gap_categories: Dict[str, int]  # {"stub": 50, "todo": 30, ...}
    total_gaps: int
    severity: str  # "critical", "high", "medium"
    description: str
    acceptance_criteria: List[str]
    example_gaps: List[Dict]

class GapClusterer:
    def __init__(self, scan_dir: str = "ai_working"):
        self.scan_dir = Path(scan_dir)
        self.aggregate = json.load(open(self.scan_dir / "gap_scan_aggregate.json"))
        
        # Load all module reports
        self.modules_data = {}
        for module in self.aggregate.keys():
            filepath = self.scan_dir / f"gap_scan_{module}.json"
            if filepath.exists():
                with open(filepath) as f:
                    self.modules_data[module] = json.load(f)
    
    def categorize_all_gaps(self) -> Dict[str, List]:
        """Categorize all gaps by type across all modules"""
        categories = defaultdict(list)
        
        for module, data in self.modules_data.items():
            for gap in data.get("gaps", []):
                category = gap.get("category", "unknown")
                gap_with_module = {**gap, "module": module}
                categories[category].append(gap_with_module)
        
        return categories
    
    def get_critical_modules(self) -> List[Tuple[str, int]]:
        """Get modules sorted by gap count (severity)"""
        return sorted(
            self.aggregate.items(),
            key=lambda x: x[1]["total"],
            reverse=True
        )
    
    def create_meta_issue_unimplemented(self) -> ClusteredIssue:
        """Meta-issue: Complete unimplemented code paths"""
        categories = self.categorize_all_gaps()
        unimpl = categories.get("unimplemented", [])
        
        # Group by module
        by_module = defaultdict(list)
        for gap in unimpl:
            by_module[gap["module"]].append(gap)
        
        affected_modules = sorted(by_module.keys())
        critical_count = sum(1 for g in unimpl if g.get("severity") == "critical")
        
        example_gaps = unimpl[:5]
        
        return ClusteredIssue(
            id="META-001",
            title="Complete unimplemented code paths (throw not implemented)",
            affected_modules=affected_modules,
            gap_categories={"unimplemented": len(unimpl)},
            total_gaps=len(unimpl),
            severity="critical",
            description=f"""
This meta-issue tracks all code paths that throw "not implemented" errors or return empty results.

**Scope:** {len(unimpl)} unimplemented code paths across {len(affected_modules)} modules.
**Critical Paths:** {critical_count} paths that block core functionality.

These are production readiness blockers — each represents either:
1. A genuine feature stub waiting for implementation
2. A design choice that needs documentation
3. Dead code that should be removed

**Affected Modules:** {', '.join(affected_modules[:5])} (and {len(affected_modules)-5} more)
""",
            acceptance_criteria=[
                "All critical unimplemented paths have either real implementations or design decision docs",
                "Each remaining unimplemented path is marked with STUB/SIMULATION with expiration date",
                "Tests verify either the implementation or the documented design choice",
                "No production-blocking code paths remain marked as 'not implemented'",
            ],
            example_gaps=example_gaps
        )
    
    def create_meta_issue_stubs(self) -> ClusteredIssue:
        """Meta-issue: Audit and document STUB/MOCK markers"""
        categories = self.categorize_all_gaps()
        stubs = categories.get("stub", [])
        
        by_module = defaultdict(list)
        for gap in stubs:
            by_module[gap["module"]].append(gap)
        
        affected_modules = sorted(by_module.keys())
        
        return ClusteredIssue(
            id="META-002",
            title="Audit STUB/MOCK/SIMULATION markers: add expiration & removal plans",
            affected_modules=affected_modules,
            gap_categories={"stub": len(stubs)},
            total_gaps=len(stubs),
            severity="high",
            description=f"""
This meta-issue ensures all STUB/MOCK/SIMULATION markers follow the documentation standard from COPILOT_INSTRUCTIONS.md.

**Standard Template (from .github/copilot-instructions.md § 8):**
```cpp
// STUB/SIMULATION NOTE:
// Purpose: <why this non-production path exists>
// Activation: <build flag/runtime condition/test-only gate>
// Production Delta: <how behavior differs from production>
// Removal Plan: <when/how this path will be removed>
```

**Scope:** {len(stubs)} stub markers across {len(affected_modules)} modules.
**Target:** 100% of STUB/MOCK markers comply with standard.
""",
            acceptance_criteria=[
                "All STUB/MOCK/SIMULATION markers include 4-line template",
                "Each marker has documented expiration date or condition",
                "Removal plan is specific and tracked (e.g., 'Remove after feature X ships')",
                "Tests explicitly verify stub behavior vs production behavior",
                "No stubs without clear purpose or removal plan remain",
            ],
            example_gaps=stubs[:5]
        )
    
    def create_meta_issue_todos(self) -> ClusteredIssue:
        """Meta-issue: Resolve TODO/FIXME comments"""
        categories = self.categorize_all_gaps()
        todos = categories.get("todo", [])
        
        by_module = defaultdict(list)
        for gap in todos:
            by_module[gap["module"]].append(gap)
        
        affected_modules = sorted(by_module.keys())
        
        return ClusteredIssue(
            id="META-003",
            title="Resolve all TODO/FIXME comments: create linked issues or complete",
            affected_modules=affected_modules,
            gap_categories={"todo": len(todos)},
            total_gaps=len(todos),
            severity="medium",
            description=f"""
This meta-issue ensures all TODO/FIXME comments are either:
1. Completed (code implemented)
2. Linked to a GitHub issue
3. Explicitly decided as "not needed"

**Scope:** {len(todos)} TODO/FIXME items across {len(affected_modules)} modules.

Each TODO should be actionable and tracked. This prevents technical debt from silently accumulating.
""",
            acceptance_criteria=[
                "All TODO/FIXME comments are reviewed",
                "Each TODO either: (a) completed, (b) linked to GitHub issue, or (c) deleted with reason",
                "TODO count reduced by >75% or all remaining TODOs have linked issues",
                "New code additions have zero TODO comments (enforce at PR review)",
            ],
            example_gaps=todos[:5]
        )
    
    def create_module_issue(self, module: str, summary: Dict) -> ClusteredIssue:
        """Create an issue for a specific module"""
        data = self.modules_data.get(module)
        if not data:
            return None
        
        gaps = data.get("gaps", [])
        
        # Categorize gaps in this module
        categories = defaultdict(int)
        for gap in gaps:
            categories[gap.get("category", "unknown")] += 1
        
        # Determine severity
        total = summary["total"]
        critical = summary["critical"]
        
        if critical > 50 or total > 100:
            severity = "critical"
        elif critical > 20 or total > 50:
            severity = "high"
        else:
            severity = "medium"
        
        # Create title
        if critical > 10:
            title = f"[{module}] Fix {critical} unimplemented paths + {total} total gaps"
        elif total > 50:
            title = f"[{module}] Clean up {total} implementation gaps (TODOs, stubs, empty code)"
        else:
            title = f"[{module}] Address {total} implementation gaps"
        
        # Example gaps: show mix of types
        example_gaps = []
        for category in ["unimplemented", "stub", "todo"]:
            matching = [g for g in gaps if g.get("category") == category]
            example_gaps.extend(matching[:2])
        
        return ClusteredIssue(
            id=f"MOD-{module}",
            title=title,
            affected_modules=[module],
            gap_categories=dict(categories),
            total_gaps=total,
            severity=severity,
            description=f"Implementation gap audit for `{module}` module.",
            acceptance_criteria=[
                f"Reduce gaps from {total} to <{max(5, total//5)}",
                "All critical unimplemented paths resolved or documented",
                "STUB markers updated with expiration criteria",
            ],
            example_gaps=example_gaps[:5]
        )
    
    def create_grouped_module_issue(self, modules: List[str], group_name: str, group_id: str) -> ClusteredIssue:
        """Group several related modules into one meta-issue"""
        total_gaps = sum(self.aggregate[m]["total"] for m in modules)
        total_critical = sum(self.aggregate[m]["critical"] for m in modules)
        
        # Collect all gaps from these modules
        all_gaps = []
        categories = defaultdict(int)
        
        for module in modules:
            data = self.modules_data.get(module)
            if data:
                gaps = data.get("gaps", [])
                all_gaps.extend(gaps)
                for gap in gaps:
                    categories[gap.get("category")] += 1
        
        # Pick diverse examples
        example_gaps = []
        for category in ["unimplemented", "stub", "todo"]:
            matching = [g for g in all_gaps if g.get("category") == category]
            example_gaps.extend(matching[:2])
        
        return ClusteredIssue(
            id=group_id,
            title=f"{group_name}: Fix {total_gaps} gaps across {len(modules)} modules",
            affected_modules=modules,
            gap_categories=dict(categories),
            total_gaps=total_gaps,
            severity="high" if total_critical > 10 else "medium",
            description=f"Coordinated cleanup across {len(modules)} related modules: {', '.join(modules)}",
            acceptance_criteria=[
                f"Reduce gaps from {total_gaps} to <{max(10, total_gaps//5)}",
                "All critical unimplemented paths resolved",
                "Consistent stub documentation across all modules",
            ],
            example_gaps=example_gaps[:5]
        )
    
    def cluster_all(self) -> List[ClusteredIssue]:
        """Generate all clustered issues"""
        issues = []
        
        # Meta-issues
        print("Creating meta-issues...", flush=True)
        issues.append(self.create_meta_issue_unimplemented())
        issues.append(self.create_meta_issue_stubs())
        issues.append(self.create_meta_issue_todos())
        
        print("Creating grouped module issues...", flush=True)
        
        # Grouped module issues
        critical_modules = self.get_critical_modules()
        
        # Top 6 critical modules: 1 issue each
        for module, summary in critical_modules[:6]:
            if summary["total"] >= 50:
                issue = self.create_module_issue(module, summary)
                if issue:
                    issues.append(issue)
        
        # Group mid-tier modules
        mid_tier = [m for m, s in critical_modules[6:12]]
        if mid_tier:
            issues.append(self.create_grouped_module_issue(
                mid_tier,
                "Data Layer & Indexing Completeness",
                "GROUP-001"
            ))
        
        # Group search/query modules
        search_modules = [m for m in ["query", "search", "rag", "scheduler"] 
                         if m in self.aggregate and self.aggregate[m]["total"] > 10]
        if search_modules:
            issues.append(self.create_grouped_module_issue(
                search_modules,
                "Query/Search Engine Completeness",
                "GROUP-002"
            ))
        
        # Group ML/AI modules
        ml_modules = [m for m in ["llm", "ai", "training", "tensor", "prompt_engineering"]
                     if m in self.aggregate and self.aggregate[m]["total"] > 5]
        if ml_modules:
            issues.append(self.create_grouped_module_issue(
                ml_modules,
                "ML/AI Integration Hardening",
                "GROUP-003"
            ))
        
        # Group infra modules
        infra_modules = [m for m in ["network", "cache", "replication", "distributed_knowledge"]
                        if m in self.aggregate and self.aggregate[m]["total"] > 5]
        if infra_modules:
            issues.append(self.create_grouped_module_issue(
                infra_modules,
                "Distributed Infrastructure Completeness",
                "GROUP-004"
            ))
        
        return issues
    
    def issue_to_github_markdown(self, issue: ClusteredIssue) -> str:
        """Convert issue to GitHub markdown format"""
        md = f"""# {issue.title}

**Issue Type:** Implementation Gap Audit  
**Priority:** {issue.severity.upper()}  
**Affected Modules:** {len(issue.affected_modules)}  
**Total Gaps:** {issue.total_gaps}  

## Summary

{issue.description}

## Gap Breakdown

"""
        for category, count in sorted(issue.gap_categories.items(), key=lambda x: x[1], reverse=True):
            md += f"- **{category.title()}:** {count}\n"
        
        md += "\n## Example Gaps\n\n"
        for gap in issue.example_gaps[:3]:
            if isinstance(gap, dict):
                file_str = gap.get("file", "?")
                line_str = gap.get("line_num", "?")
                ctx = gap.get("context", "?")[:80]
            else:
                file_str = getattr(gap, "file", "?")
                line_str = getattr(gap, "line_num", "?")
                ctx = getattr(gap, "context", "?")[:80]
            md += f"- `{file_str}:{line_str}` — {ctx}\n"
        
        if len(issue.example_gaps) > 3:
            md += f"- ... and {len(issue.example_gaps) - 3} more\n"
        
        md += "\n## Acceptance Criteria\n\n"
        for i, criterion in enumerate(issue.acceptance_criteria, 1):
            md += f"{i}. {criterion}\n"
        
        md += f"\n## Related\n\n"
        md += f"- [Gap Scan Results](ai_working/) — Full scan reports\n"
        md += f"- [ROADMAP.md](ROADMAP.md) — Project roadmap\n"
        
        return md
    
    def save_clustered_issues(self, issues: List[ClusteredIssue]) -> Path:
        """Save clustered issues to JSON and markdown"""
        issues_dir = self.scan_dir / "clustered_issues"
        issues_dir.mkdir(exist_ok=True)
        
        # Save JSON (simplified - just metadata, not full gaps)
        json_data = {
            "generated": datetime.now().isoformat(),
            "total_issues": len(issues),
            "issues": [
                {
                    "id": i.id,
                    "title": i.title,
                    "affected_modules": i.affected_modules,
                    "gap_categories": i.gap_categories,
                    "total_gaps": i.total_gaps,
                    "severity": i.severity,
                    "example_count": len(i.example_gaps)
                }
                for i in issues
            ]
        }
        
        json_file = issues_dir / "clustered_issues.json"
        with open(json_file, 'w', encoding='utf-8') as f:
            json.dump(json_data, f, indent=2)
        
        # Save individual markdown files
        for issue in issues:
            md_file = issues_dir / f"{issue.id}.md"
            with open(md_file, 'w', encoding='utf-8') as f:
                f.write(self.issue_to_github_markdown(issue))
        
        return issues_dir
    
    def save_github_commands(self, issues: List[ClusteredIssue]) -> Path:
        """Save gh commands to create issues"""
        issues_dir = self.scan_dir / "clustered_issues"
        issues_dir.mkdir(exist_ok=True)
        
        # Create batch script
        batch_file = issues_dir / "create_issues.sh"
        with open(batch_file, 'w', encoding='utf-8') as f:
            f.write("#!/bin/bash\n")
            f.write("# Create clustered gap scan issues\n")
            f.write("set -e\n\n")
            
            for i, issue in enumerate(issues, 1):
                md_file = f"{issue.id}.md"
                f.write(f"echo 'Creating issue {i}/{len(issues)}: {issue.title}'\n")
                f.write(f"gh issue create --title '{issue.title}' ")
                f.write(f"--body-file {md_file} ")
                f.write(f"--label 'gap-scan,{issue.severity}' ")
                f.write(f"--repo makr-code/ThemisDB\n")
                f.write(f"sleep 2\n\n")
        
        return batch_file

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Cluster implementation gaps into meta-issues")
    parser.add_argument("--scan-dir", default="ai_working", help="Gap scan directory")
    args = parser.parse_args()
    
    clusterer = GapClusterer(args.scan_dir)
    
    print("="*60)
    print("THEMISDB GAP CLUSTERING")
    print("="*60)
    
    issues = clusterer.cluster_all()
    
    print(f"\nGenerated {len(issues)} clustered issues:\n")
    for issue in issues:
        mod_str = ", ".join(issue.affected_modules[:3])
        if len(issue.affected_modules) > 3:
            mod_str += f" (+{len(issue.affected_modules)-3})"
        print(f"  {issue.id:12} | {issue.severity:8} | {issue.total_gaps:3} gaps | {mod_str}")
    
    # Save results
    print("\nSaving clustered issues...")
    issues_dir = clusterer.save_clustered_issues(issues)
    print(f"✓ Saved to: {issues_dir}/")
    
    batch_file = clusterer.save_github_commands(issues)
    print(f"✓ Batch script: {batch_file}")
    
    print("\n" + "="*60)
    print("NEXT STEPS")
    print("="*60)
    print(f"1. Review issues: {issues_dir}/*.md")
    print(f"2. Create on GitHub: bash {batch_file}")
    print(f"3. Or one-by-one: cd {issues_dir} && gh issue create --title '...' --body-file <issue>.md")

if __name__ == "__main__":
    main()
