#!/usr/bin/env python3
"""
GitHub Issues Generator from Templates
Creates issues from .github/ISSUE_TEMPLATE/*.md files and optionally deletes templates after creation.

IMPORTANT: Label Validation
    All labels used in issue templates MUST be valid labels defined in .github/labels.yml
    See .github/LABELS_GUIDE.md for the complete list of available labels.
    
    Common label categories:
    - priority:P0, priority:P1, priority:P2, priority:P3
    - type:bug, type:feature, type:enhancement, type:documentation, etc.
    - area:llm, area:storage, area:aql, area:api, etc.
    
    Before creating issue templates with labels, verify they exist in .github/labels.yml

Usage:
    python create_issues_from_templates.py [--delete-templates] [--dry-run]

Options:
    --delete-templates    Delete issue templates after successful creation
    --dry-run            Show what would be done without creating issues
"""

import subprocess
import json
import sys
import os
import re
from pathlib import Path
from typing import Optional, Dict, List, Tuple

class IssueTemplateProcessor:
    """Process GitHub issue templates and create issues"""
    
    def __init__(self, repo_root: Path = None, delete_templates: bool = False, dry_run: bool = False):
        """Initialize processor"""
        # Determine repo root: script is in .github/scripts/, so go up 2 levels
        if repo_root:
            self.repo_root = Path(repo_root)
        else:
            script_dir = Path(__file__).resolve().parent
            self.repo_root = script_dir.parent.parent
        
        self.templates_dir = self.repo_root / ".github" / "ISSUE_TEMPLATE"
        
        # Fallback: if templates dir doesn't exist, try from cwd
        if not self.templates_dir.exists():
            fallback = Path.cwd() / ".github" / "ISSUE_TEMPLATE"
            if fallback.exists():
                self.templates_dir = fallback
                self.repo_root = Path.cwd()
        
        self.delete_templates = delete_templates
        self.dry_run = dry_run
        self.created_issues: List[Dict] = []
        self.failed_issues: List[Dict] = []
        self.deleted_templates: List[Path] = []
        self.skipped_issues: List[Dict] = []
        self.existing_issue_titles: List[str] = []
        
    def get_existing_issues(self) -> bool:
        """Get existing issue titles from GitHub"""
        try:
            if self.dry_run:
                print("  [DRY-RUN] Skipping existing issue check")
                return True
            
            result = subprocess.run(
                ['gh', 'issue', 'list', '--limit', '500', '--json', 'title'],
                capture_output=True,
                text=True,
                cwd=str(self.repo_root)
            )
            
            if result.returncode == 0:
                try:
                    issues = json.loads(result.stdout)
                    self.existing_issue_titles = [issue.get('title', '') for issue in issues]
                    print(f"✓ Found {len(self.existing_issue_titles)} existing issues on GitHub\n")
                    return True
                except json.JSONDecodeError:
                    print("⚠️  Could not parse existing issues\n")
                    return False
            else:
                print(f"⚠️  Could not fetch existing issues: {result.stderr}\n")
                return False
                
        except Exception as e:
            print(f"❌ Error fetching existing issues: {e}\n")
            return False
    
    def issue_exists(self, title: str) -> bool:
        """Check if an issue with this title already exists"""
        return any(title.lower() in existing.lower() or existing.lower() in title.lower() 
                   for existing in self.existing_issue_titles)
    
    def parse_template(self, template_path: Path) -> Optional[Dict]:
        """Parse YAML frontmatter from template"""
        try:
            with open(template_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Extract frontmatter between --- markers
            match = re.match(r'^---\n(.*?)\n---\n', content, re.DOTALL)
            if not match:
                print(f"⚠️  No frontmatter found in {template_path.name}")
                return None
            
            frontmatter = match.group(1)
            body_start = match.end()
            body = content[body_start:].strip()
            
            # Parse frontmatter manually (simple YAML-like parsing)
            issue_data = {
                'template_file': template_path,
                'title': '',
                'labels': [],
                'body': body,
                'assignees': []
            }
            
            # Parse frontmatter lines
            for line in frontmatter.split('\n'):
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                
                if ':' in line:
                    key, value = line.split(':', 1)
                    key = key.strip()
                    value = value.strip().strip("'\"[]")
                    
                    if key == 'title':
                        issue_data['title'] = value
                    elif key == 'labels':
                        # Parse labels - can be comma-separated or array
                        # NOTE: Labels should be valid labels from .github/labels.yml
                        # See .github/LABELS_GUIDE.md for available labels
                        if value.startswith('['):
                            labels = re.findall(r"'([^']*)'|\"([^\"]*)\"", value)
                            issue_data['labels'] = [l[0] or l[1] for l in labels]
                        else:
                            issue_data['labels'] = [l.strip() for l in value.split(',')]
                    elif key == 'assignees':
                        if value.startswith('['):
                            assignees = re.findall(r"'([^']*)'|\"([^\"]*)\"", value)
                            issue_data['assignees'] = [a[0] or a[1] for a in assignees if a[0] or a[1]]
            
            if not issue_data['title']:
                print(f"⚠️  No title found in {template_path.name}")
                return None
            
            return issue_data
            
        except Exception as e:
            print(f"❌ Error parsing {template_path.name}: {e}")
            return None
    
    def create_issue(self, issue_data: Dict) -> bool:
        """Create issue on GitHub"""
        title = issue_data['title']
        labels = issue_data['labels']
        body = issue_data['body']
        
        if self.dry_run:
            print(f"  [DRY-RUN] Would create: {title}")
            return True
        
        try:
            # Build command
            cmd_parts = ['gh', 'issue', 'create']
            cmd_parts.extend(['--title', title])
            
            if labels:
                cmd_parts.extend(['--label', ','.join(labels)])
            
            cmd_parts.extend(['--body', body])
            
            # Execute command
            result = subprocess.run(
                cmd_parts,
                capture_output=True,
                text=True,
                cwd=str(self.repo_root)
            )
            
            if result.returncode == 0:
                # Extract issue number
                output = result.stdout.strip()
                if '#' in output:
                    issue_num = re.search(r'#(\d+)', output)
                    if issue_num:
                        issue_num = issue_num.group(1)
                        print(f"  ✅ Created: #{issue_num}")
                        return True
                elif 'issues/' in output:
                    issue_num = re.search(r'issues/(\d+)', output)
                    if issue_num:
                        issue_num = issue_num.group(1)
                        print(f"  ✅ Created: #{issue_num}")
                        return True
                else:
                    print(f"  ✅ Created: {output}")
                    return True
            else:
                print(f"  ❌ Failed: {result.stderr}")
                return False
                
        except Exception as e:
            print(f"  ❌ Error: {e}")
            return False
    
    def delete_template(self, template_path: Path) -> bool:
        """Delete template file"""
        if self.dry_run:
            print(f"  [DRY-RUN] Would delete: {template_path.name}")
            return True
        
        try:
            template_path.unlink()
            print(f"  🗑️  Deleted template: {template_path.name}")
            return True
        except Exception as e:
            print(f"  ❌ Failed to delete: {e}")
            return False
    
    def process_all_templates(self) -> Tuple[int, int, int, int]:
        """Process all templates in the template directory"""
        if not self.templates_dir.exists():
            print(f"❌ Template directory not found: {self.templates_dir}")
            return 0, 0, 0, 0
        
        # Get existing issues first
        print("Checking existing issues on GitHub...")
        self.get_existing_issues()
        
        template_files = sorted(self.templates_dir.glob('*.md'))
        
        if not template_files:
            print(f"⚠️  No templates found in {self.templates_dir}")
            return 0, 0, 0, 0
        
        print(f"Found {len(template_files)} template(s)\n")
        
        created_count = 0
        failed_count = 0
        deleted_count = 0
        skipped_count = 0
        
        for template_file in template_files:
            print(f"Processing: {template_file.name}")
            
            # Parse template
            issue_data = self.parse_template(template_file)
            if not issue_data:
                failed_count += 1
                print()
                continue
            
            # Check if issue already exists
            if self.issue_exists(issue_data['title']):
                print(f"  ⏭️  Skipped: Issue already exists")
                skipped_count += 1
                self.skipped_issues.append({
                    'title': issue_data['title'],
                    'template': template_file.name
                })
                print()
                continue
            
            # Create issue
            success = self.create_issue(issue_data)
            
            if success:
                created_count += 1
                
                # Delete template if requested
                if self.delete_templates:
                    if self.delete_template(template_file):
                        deleted_count += 1
                    else:
                        print(f"  ⚠️  Issue created but template not deleted")
                
                self.created_issues.append({
                    'title': issue_data['title'],
                    'template': template_file.name
                })
            else:
                failed_count += 1
                self.failed_issues.append({
                    'title': issue_data['title'],
                    'template': template_file.name
                })
            
            print()
        
        return created_count, failed_count, deleted_count, skipped_count
    
    def print_summary(self, created: int, failed: int, deleted: int, skipped: int):
        """Print summary of operations"""
        print("=" * 60)
        print("SUMMARY")
        print("=" * 60)
        print(f"✅ Issues Created:       {created}")
        print(f"⏭️  Issues Skipped:      {skipped}")
        print(f"❌ Issues Failed:        {failed}")
        if self.delete_templates:
            print(f"🗑️  Templates Deleted:  {deleted}")
        
        if skipped > 0:
            print("\nSkipped Issues (already exist on GitHub):")
            for issue in self.skipped_issues:
                print(f"  - {issue['title']}")
        
        if self.created_issues:
            print("\nCreated Issues:")
            for issue in self.created_issues:
                print(f"  - {issue['title']}")
        
        if self.failed_issues:
            print("\nFailed Issues:")
            for issue in self.failed_issues:
                print(f"  - {issue['title']}")
        
        if self.dry_run:
            print("\n[DRY-RUN MODE] - No actual changes made")
        
        print("=" * 60)


def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Create GitHub issues from templates',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python create_issues_from_templates.py
  python create_issues_from_templates.py --delete-templates
  python create_issues_from_templates.py --dry-run
  python create_issues_from_templates.py --delete-templates --dry-run
        """
    )
    
    parser.add_argument(
        '--delete-templates',
        action='store_true',
        help='Delete issue templates after successful creation'
    )
    
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be done without making changes'
    )
    
    parser.add_argument(
        '--repo-root',
        type=Path,
        default=Path.cwd(),
        help='Root directory of the repository (default: current directory)'
    )
    
    args = parser.parse_args()
    
    # Print mode info
    print()
    print("🚀 GitHub Issues Generator from Templates")
    print("-" * 60)
    
    if args.dry_run:
        print("📋 MODE: DRY-RUN (no changes will be made)")
    
    if args.delete_templates:
        print("🗑️  OPTION: Templates will be deleted after creation")
    
    print("-" * 60)
    print()
    
    # Process templates
    processor = IssueTemplateProcessor(
        repo_root=args.repo_root,
        delete_templates=args.delete_templates,
        dry_run=args.dry_run
    )
    
    created, failed, deleted, skipped = processor.process_all_templates()
    
    # Print summary
    processor.print_summary(created, failed, deleted, skipped)
    
    # Return appropriate exit code
    if failed > 0:
        print(f"\n⚠️  {failed} issue(s) failed to create")
        return 1
    
    if created > 0:
        if args.delete_templates and deleted != created:
            print(f"\n⚠️  {created - deleted} template(s) not deleted")
            return 1
        return 0
    
    if skipped > 0:
        print(f"\n✅ All {skipped} issue(s) already exist on GitHub")
        return 0
    
    print("\n⚠️  No issues were created or found")
    return 1


if __name__ == '__main__':
    sys.exit(main())
