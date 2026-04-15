"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            add_doc_metadata.py                                ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:11:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     341                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Add structured metadata to markdown files in YAML format.

This script automatically appends metadata to markdown files including:
- Author (default: Themis DevTeam & Copilot)
- Document number (release/tag or date)
- Creation date (from first commit)
- Last modification (from last commit)
- Commit title (from first commit)
- Reviewer (empty)
- Title (first markdown heading)
- File path (relative to repo root)

Requires: Python 3.9+
"""

import os
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Optional, Tuple


def run_git_command(args: list[str], cwd: Optional[str] = None) -> str:
    """Run a git command and return its output."""
    try:
        result = subprocess.run(
            ["git"] + args,
            cwd=cwd,
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"Git command failed: {' '.join(args)}", file=sys.stderr)
        print(f"Error: {e.stderr}", file=sys.stderr)
        return ""


def get_repo_root() -> str:
    """Get the repository root directory."""
    return run_git_command(["rev-parse", "--show-toplevel"])


def get_file_first_commit(file_path: str, repo_root: str) -> Tuple[str, str, str]:
    """
    Get first commit date, hash, and title for a file.
    
    Returns:
        Tuple of (date, hash, commit_title)
    """
    # Get first commit hash and date, following renames
    log_output = run_git_command(
        ["log", "--follow", "--format=%H|%ai|%s", "--", file_path],
        cwd=repo_root
    )
    
    if log_output:
        lines = log_output.strip().split("\n")
        # Get the last line (earliest commit)
        parts = lines[-1].split("|", 2)
        if len(parts) >= 3:
            commit_hash = parts[0]
            commit_date = parts[1].split()[0]  # Extract date part (YYYY-MM-DD)
            commit_title = parts[2]
            return commit_date, commit_hash, commit_title
    
    return "", "", ""


def get_file_last_commit(file_path: str, repo_root: str) -> str:
    """Get last commit date for a file."""
    log_output = run_git_command(
        ["log", "-1", "--format=%ai", "--", file_path],
        cwd=repo_root
    )
    
    if log_output:
        return log_output.split()[0]  # Extract date part (YYYY-MM-DD)
    
    return ""


def get_tag_at_commit(commit_hash: str, repo_root: str) -> str:
    """Get tag at a specific commit."""
    if not commit_hash:
        return ""
    
    tag = run_git_command(
        ["tag", "--points-at", commit_hash],
        cwd=repo_root
    )
    
    return tag.split("\n")[0] if tag else ""


def extract_first_heading(content: str) -> str:
    """Extract the first markdown heading from content."""
    lines = content.split("\n")
    for line in lines:
        # Match markdown headings (# Header)
        match = re.match(r"^#+\s+(.+)$", line.strip())
        if match:
            return match.group(1).strip()
    return ""


def has_metadata_section(content: str) -> bool:
    """Check if the file already has a metadata section."""
    # Look for metadata marker at the end of the file
    return "---\n## Dokumenten-Metadaten" in content or "## Dokumenten-Metadaten\n\n```yaml" in content


def escape_yaml_string(s: str) -> str:
    """
    Escape a string for safe use in YAML.
    
    Handles quotes, colons, newlines, and other special characters.
    """
    # Replace newlines with spaces
    s = s.replace("\n", " ").replace("\r", " ")
    # Escape double quotes
    s = s.replace('"', '\\"')
    # Limit length to avoid extremely long lines
    if len(s) > 200:
        s = s[:197] + "..."
    return s


def generate_metadata_block(
    file_path: str,
    repo_root: str,
    title: str,
    creation_date: str,
    last_modified: str,
    commit_title: str,
    doc_number: str
) -> str:
    """Generate YAML metadata block."""
    # Get relative path from repo root
    abs_path = os.path.abspath(file_path)
    abs_repo = os.path.abspath(repo_root)
    rel_path = os.path.relpath(abs_path, abs_repo)
    
    # Escape strings for YAML safety
    safe_title = escape_yaml_string(title)
    safe_commit_title = escape_yaml_string(commit_title)
    
    metadata = f"""
---

## Dokumenten-Metadaten

```yaml
Urheber: Themis DevTeam & Copilot
Dokumenten-Nr: {doc_number}
Erstelldatum: {creation_date}
Letzte Änderung: {last_modified}
Commit-Titel: "{safe_commit_title}"
Reviewer: 
Titel: "{safe_title}"
Dateipfad: {rel_path}
```
"""
    return metadata


def process_markdown_file(file_path: str, repo_root: str, dry_run: bool = False) -> bool:
    """
    Process a single markdown file and add metadata if not present.
    
    Returns:
        True if file was modified, False otherwise
    """
    try:
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()
        
        # Check if metadata already exists
        if has_metadata_section(content):
            print(f"Skipping {file_path} - metadata already present")
            return False
        
        # Extract title
        title = extract_first_heading(content)
        if not title:
            title = os.path.basename(file_path)
        
        # Get git information
        creation_date, commit_hash, commit_title = get_file_first_commit(file_path, repo_root)
        last_modified = get_file_last_commit(file_path, repo_root)
        
        # If no git info, use current date
        if not creation_date:
            creation_date = datetime.now().strftime("%Y-%m-%d")
        if not last_modified:
            last_modified = datetime.now().strftime("%Y-%m-%d")
        if not commit_title:
            commit_title = "Initial commit"
        
        # Get document number (tag or date)
        doc_number = get_tag_at_commit(commit_hash, repo_root)
        if not doc_number:
            doc_number = f"Stand: {creation_date}"
        
        # Generate metadata
        metadata = generate_metadata_block(
            file_path,
            repo_root,
            title,
            creation_date,
            last_modified,
            commit_title,
            doc_number
        )
        
        # Append metadata
        new_content = content.rstrip() + "\n" + metadata
        
        if dry_run:
            print(f"Would add metadata to: {file_path}")
            print(f"  Title: {title}")
            print(f"  Created: {creation_date}")
            print(f"  Modified: {last_modified}")
            print(f"  Doc Number: {doc_number}")
            return False
        else:
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(new_content)
            print(f"Added metadata to: {file_path}")
            return True
    
    except Exception as e:
        print(f"Error processing {file_path}: {e}", file=sys.stderr)
        return False


def find_markdown_files(root_dir: str, exclude_dirs: list[str] = None) -> list[str]:
    """Find all markdown files in the repository."""
    if exclude_dirs is None:
        exclude_dirs = [".git", "node_modules", "venv", ".venv", "build", "dist", "site"]
    
    markdown_files = []
    root_path = Path(root_dir)
    
    for md_file in root_path.rglob("*.md"):
        # Check if file is in excluded directory
        relative = md_file.relative_to(root_path)
        if any(part in exclude_dirs for part in relative.parts):
            continue
        markdown_files.append(str(md_file))
    
    return sorted(markdown_files)


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Add structured metadata to markdown files"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be done without making changes"
    )
    parser.add_argument(
        "--files",
        nargs="+",
        help="Specific files to process (default: all markdown files)"
    )
    parser.add_argument(
        "--exclude-dirs",
        nargs="+",
        default=[".git", "node_modules", "venv", ".venv", "build", "dist", "site"],
        help="Directories to exclude from search"
    )
    
    args = parser.parse_args()
    
    # Get repository root
    repo_root = get_repo_root()
    if not repo_root:
        print("Error: Not in a git repository", file=sys.stderr)
        sys.exit(1)
    
    print(f"Repository root: {repo_root}")
    
    # Find markdown files
    if args.files:
        markdown_files = args.files
    else:
        markdown_files = find_markdown_files(repo_root, args.exclude_dirs)
    
    print(f"Found {len(markdown_files)} markdown files")
    
    if args.dry_run:
        print("\n=== DRY RUN MODE ===\n")
    
    # Process files
    modified_count = 0
    for md_file in markdown_files:
        if process_markdown_file(md_file, repo_root, args.dry_run):
            modified_count += 1
    
    print(f"\n{'Would modify' if args.dry_run else 'Modified'} {modified_count} files")
    
    if not args.dry_run and modified_count > 0:
        print("\nChanges made. Please review and commit.")


if __name__ == "__main__":
    main()
