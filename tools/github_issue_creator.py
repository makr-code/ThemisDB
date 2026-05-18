#!/usr/bin/env python3
"""
ThemisDB GitHub Issue Creator — Automated Issue Generation from Gap Data

Transforms:
  ai_working/clustered_issues/*.md → GitHub Issues
  ai_working/gap_scan_v3_aggregate.json → Prioritized Issues

Features:
  - Batch create issues via gh CLI
  - Auto-assign labels (critical, high, medium)
  - Auto-assign milestone (Current Sprint, Next Sprint, Backlog)
  - Link to module documentation
  - Track issue creation in CSV log
"""

import json
import subprocess
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import List, Optional, Dict, Tuple
from datetime import datetime
import re


@dataclass
class GitHubIssue:
    """Represents a GitHub issue to create"""
    title: str
    body: str
    labels: List[str]
    milestone: Optional[str] = None
    assignees: List[str] = None
    
    def __post_init__(self):
        if self.assignees is None:
            self.assignees = []


class GitHubIssueCreator:
    """Create GitHub issues from gap analysis results"""
    
    def __init__(self, repo: str = "makr-code/ThemisDB", dry_run: bool = False):
        self.repo = repo
        self.dry_run = dry_run
        self.created_issues: List[Tuple[str, str]] = []  # (title, url)
        self._check_gh_cli()
    
    def _check_gh_cli(self):
        """Verify gh CLI is installed and authenticated"""
        try:
            result = subprocess.run(['gh', 'auth', 'status'], 
                                  capture_output=True, text=True, timeout=5)
            if result.returncode != 0:
                print("[WARN] gh CLI not authenticated")
                print("[INFO] Run: gh auth login")
                return False
        except FileNotFoundError:
            print("[ERROR] gh CLI not found. Install: https://cli.github.com")
            return False
        return True
    
    def create_issue_from_file(self, md_file: Path) -> Optional[str]:
        """Create single issue from markdown file"""
        if not md_file.exists():
            return None
        
        try:
            with open(md_file, 'r', encoding='utf-8') as f:
                content = f.read()
        except Exception as e:
            print(f"[ERROR] Failed to read {md_file}: {e}")
            return None
        
        # Parse title from first heading
        title_match = re.search(r'^#\s+(.+)$', content, re.MULTILINE)
        title = title_match.group(1).strip() if title_match else md_file.stem
        
        # Extract metadata (labels, milestone)
        labels = self._extract_labels(content)
        milestone = self._extract_milestone(content)
        
        issue = GitHubIssue(
            title=title,
            body=content,
            labels=labels,
            milestone=milestone
        )
        
        return self._create_github_issue(issue)
    
    def _extract_labels(self, content: str) -> List[str]:
        """Extract labels from issue body (e.g., gap-scanner, critical, acceleration)"""
        labels = ['gap-scanner', 'automated']  # Base labels
        
        # Infer severity
        if 'CRITICAL' in content or 'critical' in content:
            labels.append('P0-critical')
        elif 'HIGH' in content or 'high' in content:
            labels.append('P1-high')
        else:
            labels.append('P2-medium')
        
        # Infer module
        module_match = re.search(r'(acceleration|index|storage|llm|network|query|security|ingestion)', 
                                content, re.IGNORECASE)
        if module_match:
            labels.append(module_match.group(1).lower())
        
        return labels
    
    def _extract_milestone(self, content: str) -> Optional[str]:
        """Extract or infer milestone"""
        if 'CRITICAL' in content or 'Unimplemented' in content:
            return 'Current Sprint'
        elif 'HIGH' in content:
            return 'Next Sprint'
        else:
            return 'Backlog'
    
    def _create_github_issue(self, issue: GitHubIssue) -> Optional[str]:
        """Create issue via gh CLI"""
        if self.dry_run:
            print(f"[DRY_RUN] Would create issue: {issue.title}")
            print(f"  Labels: {', '.join(issue.labels)}")
            print(f"  Milestone: {issue.milestone}")
            return f"https://github.com/{self.repo}/issues/dry-run"
        
        # Build gh command
        cmd = [
            'gh', 'issue', 'create',
            '--repo', self.repo,
            '--title', issue.title,
            '--body', issue.body,
        ]
        
        # Add labels
        for label in issue.labels:
            cmd.extend(['--label', label])
        
        # Add milestone if specified
        if issue.milestone:
            cmd.extend(['--milestone', issue.milestone])
        
        # Add assignees if specified
        for assignee in issue.assignees:
            cmd.extend(['--assignee', assignee])
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            
            if result.returncode == 0:
                # Extract issue URL from output (e.g., "Created issue #123 at https://...")
                output = result.stdout + result.stderr
                url_match = re.search(r'(https://github\.com/[^ ]+)', output)
                if url_match:
                    issue_url = url_match.group(1)
                    self.created_issues.append((issue.title, issue_url))
                    print(f"[OK] Created: {issue.title}")
                    print(f"     {issue_url}")
                    return issue_url
                else:
                    print(f"[WARN] Issue created but URL not parsed")
                    return None
            else:
                error = result.stderr if result.stderr else result.stdout
                print(f"[ERROR] Failed to create issue: {error[:100]}")
                return None
        
        except subprocess.TimeoutExpired:
            print("[ERROR] gh CLI command timed out")
            return None
        except Exception as e:
            print(f"[ERROR] {e}")
            return None
    
    def batch_create_from_cluster_dir(self, cluster_dir: Path) -> Dict[str, str]:
        """Create issues from all markdown files in directory"""
        cluster_dir = Path(cluster_dir)
        if not cluster_dir.exists():
            print(f"[ERROR] Directory not found: {cluster_dir}")
            return {}
        
        results = {}
        md_files = sorted(cluster_dir.glob('*.md'))
        
        if not md_files:
            print(f"[WARN] No markdown files found in {cluster_dir}")
            return results
        
        print(f"\n[INFO] Found {len(md_files)} cluster files to create as issues")
        print("=" * 80)
        
        for md_file in md_files:
            print(f"\n[...] Processing {md_file.name}")
            issue_url = self.create_issue_from_file(md_file)
            if issue_url:
                results[md_file.stem] = issue_url
        
        return results
    
    def create_from_gap_data(self, gap_json: Path, 
                           min_severity: str = 'high') -> Dict[str, str]:
        """Create issues from raw gap data JSON"""
        gap_json = Path(gap_json)
        if not gap_json.exists():
            print(f"[ERROR] Gap data file not found: {gap_json}")
            return {}
        
        try:
            with open(gap_json, 'r') as f:
                data = json.load(f)
        except Exception as e:
            print(f"[ERROR] Failed to parse gap data: {e}")
            return {}
        
        results = {}
        severity_order = {'critical': 0, 'high': 1, 'medium': 2, 'low': 3}
        min_level = severity_order.get(min_severity.lower(), 2)
        
        # Transform gap data into issues
        for module, gaps in data.items():
            critical = gaps.get('severity_critical', 0)
            high = gaps.get('severity_high', 0)
            
            # Skip low-severity modules
            if critical == 0 and high == 0:
                continue
            
            # Create module issue
            title = f"[GAP] {module.upper()}: {critical} critical + {high} high gaps"
            body = self._generate_module_issue_body(module, gaps)
            
            labels = ['gap-scanner', 'automated']
            if critical >= 10:
                labels.append('P0-critical')
            elif high >= 5:
                labels.append('P1-high')
            else:
                labels.append('P2-medium')
            labels.append(module)
            
            issue = GitHubIssue(
                title=title,
                body=body,
                labels=labels,
                milestone='Current Sprint' if critical >= 10 else 'Next Sprint'
            )
            
            url = self._create_github_issue(issue)
            if url:
                results[f"MOD-{module}"] = url
        
        return results
    
    def _generate_module_issue_body(self, module: str, gaps: Dict) -> str:
        """Generate GitHub issue body from gap data"""
        body = f"""# {module.upper()} Module — Gap Analysis Report

## Summary
- **Total Gaps:** {gaps.get('total', 0)}
- **CRITICAL:** {gaps.get('severity_critical', 0)}
- **HIGH:** {gaps.get('severity_high', 0)}
- **MEDIUM:** {gaps.get('severity_medium', 0)}

## Breakdown by Category
"""
        
        for cat, count in gaps.get('by_category', {}).items():
            body += f"\n- **{cat}:** {count} gaps"
        
        body += f"""

## Top Files by Gap Count
"""
        
        # Get top files
        files_by_gap = sorted(
            gaps.get('gaps_by_file', {}).items(),
            key=lambda x: len(x[1]),
            reverse=True
        )[:10]
        
        for file_path, gaps_in_file in files_by_gap:
            body += f"\n- {file_path}: {len(gaps_in_file)} gaps"
        
        body += f"""

## Implementation Priority

### Critical Fixes
Focus on CRITICAL severity gaps first (data safety, security):
"""
        
        critical_files = [
            f for f, g in files_by_gap 
            if any(gap.get('severity') == 'CRITICAL' for gap in g)
        ][:5]
        
        for f in critical_files:
            body += f"\n- [ ] {f}"
        
        body += f"""

### High Priority
Address HIGH severity gaps (performance, reliability):
"""
        
        high_files = [
            f for f, g in files_by_gap 
            if any(gap.get('severity') == 'HIGH' for gap in g)
        ][:5]
        
        for f in high_files:
            body += f"\n- [ ] {f}"
        
        body += f"""

## Related Documentation
- [Module Documentation](../../src/{module}/MODULE_GAPS.md)
- [Gap Analysis Index](../../ai_working/module_gaps/MODULE_GAPS_INDEX.md)

## Next Steps
1. Review gap details in [MODULE_GAPS.md](../../src/{module}/MODULE_GAPS.md)
2. Assign team members by file
3. Create sub-issues for critical gaps
4. Update as progress is made

---
*Generated by gap_scanner_v3.py on {datetime.now().isoformat()}*
"""
        
        return body
    
    def save_creation_log(self, log_file: Path = Path('ai_working/github_issues_log.csv')):
        """Save log of created issues for reference"""
        log_file = Path(log_file)
        log_file.parent.mkdir(parents=True, exist_ok=True)
        
        with open(log_file, 'w', encoding='utf-8') as f:
            f.write("created_at,title,url\n")
            for title, url in self.created_issues:
                f.write(f'"{datetime.now().isoformat()}","{title}","{url}"\n')
        
        print(f"\n[OK] Issue log saved: {log_file}")
    
    def print_summary(self):
        """Print summary of created issues"""
        if not self.created_issues:
            print("\n[INFO] No issues created")
            return
        
        print("\n" + "=" * 80)
        print(f"[SUMMARY] Created {len(self.created_issues)} GitHub Issues")
        print("=" * 80)
        
        for i, (title, url) in enumerate(self.created_issues, 1):
            print(f"{i:2}. {title}")
            print(f"    {url}")
        
        print("\n[ACTION] Next steps:")
        print(f"  1. Review issues on GitHub: {self.repo}/issues")
        print(f"  2. Add to GitHub Project: Project → Add Items")
        print(f"  3. Assign team members: Issue → Assignees")
        print(f"  4. Set priorities: Issue → Labels")


def main():
    """CLI entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Create GitHub issues from gap analysis results'
    )
    parser.add_argument('--repo', default='makr-code/ThemisDB',
                       help='GitHub repository (owner/name)')
    parser.add_argument('--cluster-dir', 
                       default='ai_working/clustered_issues',
                       help='Directory with clustered issue markdown files')
    parser.add_argument('--gap-json',
                       help='Alternative: raw gap data JSON file')
    parser.add_argument('--dry-run', action='store_true',
                       help='Simulate issue creation without actually creating')
    parser.add_argument('--min-severity', default='high',
                       choices=['critical', 'high', 'medium', 'low'],
                       help='Minimum severity to create issues for')
    
    args = parser.parse_args()
    
    print("\n[INFO] GitHub Issue Creator v1")
    print("=" * 80)
    print(f"Repository: {args.repo}")
    print(f"Dry run: {args.dry_run}")
    print("=" * 80)
    
    creator = GitHubIssueCreator(repo=args.repo, dry_run=args.dry_run)
    
    if args.gap_json:
        print(f"\n[...] Creating issues from gap data: {args.gap_json}")
        results = creator.create_from_gap_data(Path(args.gap_json), args.min_severity)
    else:
        print(f"\n[...] Creating issues from cluster directory: {args.cluster_dir}")
        results = creator.batch_create_from_cluster_dir(Path(args.cluster_dir))
    
    creator.save_creation_log()
    creator.print_summary()
    
    return 0 if results else 1


if __name__ == '__main__':
    sys.exit(main())
