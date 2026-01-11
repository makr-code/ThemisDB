#!/usr/bin/env python3
"""
Auto-create GitHub Issues from .github/issues/*.md files
Usage: python create_github_issues.py

IMPORTANT: This script reads labels from issue markdown files.
           Ensure that labels used in issue files are valid labels defined in .github/labels.yml
           See .github/LABELS_GUIDE.md for the complete list of available labels.
           
           Common label categories:
           - priority:P0, priority:P1, priority:P2, priority:P3
           - type:bug, type:feature, type:enhancement, type:documentation, etc.
           - area:llm, area:storage, area:aql, area:api, etc.
           
           For label validation, consider adding a check against .github/labels.yml
"""

import os
import sys
import subprocess
import json
import re
from pathlib import Path
import time

# ANSI color codes
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'  # No Color

def print_colored(text, color):
    """Print text with color"""
    print(f"{color}{text}{Colors.NC}")

def check_gh_cli():
    """Check if GitHub CLI is installed and authenticated"""
    try:
        subprocess.run(['gh', '--version'], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print_colored("Error: GitHub CLI (gh) is not installed.", Colors.RED)
        print("Install it from: https://cli.github.com/")
        sys.exit(1)
    
    # Check authentication
    try:
        subprocess.run(['gh', 'auth', 'status'], capture_output=True, check=True)
    except subprocess.CalledProcessError:
        print_colored("Error: Not authenticated with GitHub CLI.", Colors.RED)
        print("Run: gh auth login")
        sys.exit(1)

def extract_frontmatter(content):
    """Extract YAML frontmatter from markdown content"""
    frontmatter = {}
    
    # Match frontmatter between --- delimiters
    match = re.match(r'^---\s*\n(.*?)\n---\s*\n', content, re.DOTALL)
    if match:
        frontmatter_text = match.group(1)
        
        # Extract title
        title_match = re.search(r'^title:\s*["\']?(.+?)["\']?\s*$', frontmatter_text, re.MULTILINE)
        if title_match:
            frontmatter['title'] = title_match.group(1).strip('"\'')
        
        # Extract labels
        # NOTE: Labels should be valid labels from .github/labels.yml
        # See .github/LABELS_GUIDE.md for available labels
        labels_match = re.search(r'^labels:\s*(.+?)\s*$', frontmatter_text, re.MULTILINE)
        if labels_match:
            labels_text = labels_match.group(1)
            # Handle comma-separated labels
            frontmatter['labels'] = [l.strip() for l in labels_text.replace(',', ' ').split()]
        
        # Extract milestone
        milestone_match = re.search(r'^milestone:\s*(.+?)\s*$', frontmatter_text, re.MULTILINE)
        if milestone_match:
            frontmatter['milestone'] = milestone_match.group(1).strip()
        
        # Remove frontmatter from body
        body = content[match.end():]
    else:
        body = content
    
    return frontmatter, body

def check_issue_exists(title):
    """Check if an issue with the same title already exists"""
    try:
        result = subprocess.run(
            ['gh', 'issue', 'list', '--search', f'in:title "{title}"', '--json', 'number,title'],
            capture_output=True,
            text=True,
            check=True
        )
        
        issues = json.loads(result.stdout)
        for issue in issues:
            if issue['title'] == title:
                return issue['number']
        return None
    except (subprocess.CalledProcessError, json.JSONDecodeError):
        return None

def create_issue(issue_file):
    """Create a GitHub issue from a markdown file"""
    basename = os.path.basename(issue_file)
    
    print_colored(f"Processing: {basename}", Colors.BLUE)
    
    # Read file content
    try:
        with open(issue_file, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print_colored(f"  ✗ Error reading file: {e}", Colors.RED)
        return {'status': 'error', 'basename': basename}
    
    # Extract frontmatter and body
    frontmatter, body = extract_frontmatter(content)
    
    # Get title
    title = frontmatter.get('title')
    if not title:
        # Fallback: use filename as title
        title = basename.replace('.md', '').replace('-', ' ').lstrip('0123456789 ').title()
        print_colored(f"  Warning: No title found in frontmatter, using filename: {title}", Colors.YELLOW)
    
    # Check if issue already exists
    existing_number = check_issue_exists(title)
    if existing_number:
        print_colored(f"  ⊘ Skipped: Issue already exists (#{existing_number})", Colors.YELLOW)
        return {'status': 'skipped', 'basename': basename, 'number': existing_number}
    
    # Build gh issue create command
    cmd = ['gh', 'issue', 'create', '--title', title, '--body', body]
    
    # Add labels if present
    if 'labels' in frontmatter and frontmatter['labels']:
        for label in frontmatter['labels']:
            cmd.extend(['--label', label])
    
    # Add milestone if present
    if 'milestone' in frontmatter:
        cmd.extend(['--milestone', frontmatter['milestone']])
    
    # Create the issue
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        issue_url = result.stdout.strip()
        
        # Extract issue number from URL
        issue_number = issue_url.split('/')[-1]
        
        print_colored(f"  ✓ Created: Issue #{issue_number}", Colors.GREEN)
        print(f"    {issue_url}")
        
        return {
            'status': 'created',
            'basename': basename,
            'number': issue_number,
            'title': title,
            'url': issue_url
        }
    except subprocess.CalledProcessError as e:
        print_colored(f"  ✗ Error creating issue", Colors.RED)
        print(e.stderr)
        return {'status': 'error', 'basename': basename}

def main():
    """Main function"""
    # Print header
    print_colored("╔════════════════════════════════════════════════════════════════╗", Colors.BLUE)
    print_colored("║          ThemisDB GitHub Issues Auto-Creator                  ║", Colors.BLUE)
    print_colored("╔════════════════════════════════════════════════════════════════╗", Colors.BLUE)
    print()
    
    # Check GitHub CLI
    check_gh_cli()
    
    # Get script directory
    script_dir = Path(__file__).parent.resolve()
    issues_dir = script_dir / 'issues'
    
    # Check if issues directory exists
    if not issues_dir.exists():
        print_colored(f"Error: Issues directory not found: {issues_dir}", Colors.RED)
        sys.exit(1)
    
    # Find all issue markdown files (excluding specific documentation files)
    exclude_files = {'README.md', 'COMPLETE_TODO_INVENTORY.md', 'DOCUMENTATION_TODOS_ANALYSIS.md'}
    issue_files = sorted([
        f for f in issues_dir.glob('*.md')
        if f.is_file() and f.name not in exclude_files
    ])
    
    if not issue_files:
        print_colored(f"No issue files found in {issues_dir}", Colors.YELLOW)
        sys.exit(0)
    
    print_colored(f"Found {len(issue_files)} issue files:", Colors.GREEN)
    for f in issue_files:
        print(f"  - {f.name}")
    print()
    
    # Ask for confirmation
    try:
        response = input("Create GitHub issues for all files? (y/N): ").strip().lower()
        if response not in ['y', 'yes']:
            print_colored("Aborted.", Colors.YELLOW)
            sys.exit(0)
    except KeyboardInterrupt:
        print()
        print_colored("Aborted.", Colors.YELLOW)
        sys.exit(0)
    
    print()
    print_colored("════════════════════════════════════════════════════════════════", Colors.BLUE)
    print_colored("                    Creating Issues...                          ", Colors.BLUE)
    print_colored("════════════════════════════════════════════════════════════════", Colors.BLUE)
    print()
    
    # Process each file
    results = []
    for issue_file in issue_files:
        result = create_issue(issue_file)
        results.append(result)
        print()
        
        # Rate limiting: wait 2 seconds between API calls
        if result['status'] == 'created':
            time.sleep(2)
    
    # Print summary
    created = [r for r in results if r['status'] == 'created']
    skipped = [r for r in results if r['status'] == 'skipped']
    errors = [r for r in results if r['status'] == 'error']
    
    print()
    print_colored("════════════════════════════════════════════════════════════════", Colors.BLUE)
    print_colored("                         Summary                                ", Colors.BLUE)
    print_colored("════════════════════════════════════════════════════════════════", Colors.BLUE)
    print()
    print_colored(f"Created:  {len(created)} issues", Colors.GREEN)
    print_colored(f"Skipped:  {len(skipped)} issues (already exist)", Colors.YELLOW)
    print_colored(f"Errors:   {len(errors)} issues", Colors.RED)
    print()
    
    if created:
        print_colored("Created Issues:", Colors.GREEN)
        for r in created:
            print(f"  ✓ {r['basename']} → #{r['number']} - {r['title']}")
        print()
    
    if len(created) > 0:
        print_colored(f"✓ Successfully created {len(created)} GitHub issues!", Colors.GREEN)
        print()
        print_colored("View all issues:", Colors.BLUE)
        print("  gh issue list")
        print()
        
        # Try to get repo name
        try:
            result = subprocess.run(
                ['gh', 'repo', 'view', '--json', 'nameWithOwner'],
                capture_output=True,
                text=True,
                check=True
            )
            repo_data = json.loads(result.stdout)
            repo_name = repo_data.get('nameWithOwner', '')
            if repo_name:
                print_colored("Or visit:", Colors.BLUE)
                print(f"  https://github.com/{repo_name}/issues")
        except:
            pass

if __name__ == '__main__':
    main()
