#!/usr/bin/env python3
"""
GitHub Labels Sync Script for ThemisDB
This script synchronizes labels from .github/labels.yml to GitHub

Usage:
    python sync-labels.py                    # Dry-run mode (shows what would be done)
    python sync-labels.py --apply            # Actually apply changes to GitHub
    python sync-labels.py --delete-existing  # Delete all existing labels first (dangerous!)

Prerequisites:
    - GitHub CLI (gh) installed and authenticated
    - PyYAML: pip install pyyaml
    - Appropriate permissions on the repository
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Any

try:
    import yaml
except ImportError:
    print("Error: PyYAML is not installed.")
    print("Please install it with: pip install pyyaml")
    sys.exit(1)

# ANSI color codes
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'  # No Color


def check_gh_cli() -> bool:
    """Check if GitHub CLI is installed and authenticated."""
    try:
        subprocess.run(
            ["gh", "--version"],
            check=True,
            capture_output=True
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        print(f"{Colors.RED}Error: GitHub CLI (gh) is not installed.{Colors.NC}")
        print("Please install it from: https://cli.github.com/")
        return False

    try:
        subprocess.run(
            ["gh", "auth", "status"],
            check=True,
            capture_output=True
        )
    except subprocess.CalledProcessError:
        print(f"{Colors.RED}Error: Not authenticated with GitHub CLI.{Colors.NC}")
        print("Please run: gh auth login")
        return False

    return True


def load_labels(labels_file: Path) -> List[Dict[str, str]]:
    """Load labels from YAML file."""
    if not labels_file.exists():
        print(f"{Colors.RED}Error: Labels file not found: {labels_file}{Colors.NC}")
        sys.exit(1)

    with open(labels_file, 'r') as f:
        labels = yaml.safe_load(f)

    if not isinstance(labels, list):
        print(f"{Colors.RED}Error: Labels file must contain a list of labels{Colors.NC}")
        sys.exit(1)

    return labels


def get_existing_labels() -> Dict[str, Dict[str, str]]:
    """Fetch existing labels from GitHub."""
    print(f"{Colors.BLUE}Fetching existing labels from GitHub...{Colors.NC}")
    try:
        result = subprocess.run(
            ["gh", "label", "list", "--json", "name,description,color", "--limit", "1000"],
            capture_output=True,
            text=True,
            check=True
        )
        labels_list = json.loads(result.stdout)
        existing = {label["name"]: label for label in labels_list}
        print(f"{Colors.GREEN}Found {len(existing)} existing labels{Colors.NC}\n")
        return existing
    except subprocess.CalledProcessError as e:
        print(f"{Colors.RED}Error fetching labels: {e}{Colors.NC}")
        sys.exit(1)


def delete_all_labels(dry_run: bool = True) -> None:
    """Delete all existing labels."""
    existing = get_existing_labels()
    print(f"{Colors.RED}Deleting all existing labels...{Colors.NC}")

    for label_name in existing.keys():
        if dry_run:
            print(f"  {Colors.YELLOW}[DRY-RUN]{Colors.NC} Would delete: {label_name}")
        else:
            try:
                subprocess.run(
                    ["gh", "label", "delete", label_name, "--yes"],
                    check=True,
                    capture_output=True
                )
                print(f"  {Colors.RED}[-]{Colors.NC} Deleted: {label_name}")
            except subprocess.CalledProcessError as e:
                print(f"  {Colors.RED}Error deleting {label_name}: {e}{Colors.NC}")
    print()


def sync_labels(labels: List[Dict[str, str]], dry_run: bool = True, delete_existing: bool = False) -> Dict[str, int]:
    """Sync labels to GitHub."""
    stats = {"created": 0, "updated": 0, "unchanged": 0, "errors": 0}

    # Get existing labels
    existing_labels = {} if delete_existing else get_existing_labels()

    # Delete existing labels if requested
    if delete_existing:
        delete_all_labels(dry_run)

    print(f"{Colors.BLUE}Syncing labels...{Colors.NC}")

    for label in labels:
        name = label.get("name", "")
        color = label.get("color", "")
        description = label.get("description", "")

        if not name or not color:
            print(f"  {Colors.RED}Error: Label missing name or color: {label}{Colors.NC}")
            stats["errors"] += 1
            continue

        # Check if label exists
        if name in existing_labels and not delete_existing:
            existing = existing_labels[name]
            needs_update = (
                existing["color"] != color or
                existing.get("description", "") != description
            )

            if needs_update:
                if dry_run:
                    print(f"  {Colors.YELLOW}[DRY-RUN]{Colors.NC} Would update: {name}")
                else:
                    try:
                        subprocess.run(
                            ["gh", "label", "edit", name, "--color", color, "--description", description],
                            check=True,
                            capture_output=True
                        )
                        print(f"  {Colors.YELLOW}[~]{Colors.NC} Updated: {name}")
                        stats["updated"] += 1
                    except subprocess.CalledProcessError as e:
                        print(f"  {Colors.RED}Error updating {name}: {e}{Colors.NC}")
                        stats["errors"] += 1
            else:
                stats["unchanged"] += 1
        else:
            # Create new label
            if dry_run:
                print(f"  {Colors.YELLOW}[DRY-RUN]{Colors.NC} Would create: {name}")
            else:
                try:
                    subprocess.run(
                        ["gh", "label", "create", name, "--color", color, "--description", description],
                        check=True,
                        capture_output=True
                    )
                    print(f"  {Colors.GREEN}[+]{Colors.NC} Created: {name}")
                    stats["created"] += 1
                except subprocess.CalledProcessError as e:
                    # Label might already exist, try to update instead
                    try:
                        subprocess.run(
                            ["gh", "label", "edit", name, "--color", color, "--description", description],
                            check=True,
                            capture_output=True
                        )
                        print(f"  {Colors.YELLOW}[~]{Colors.NC} Updated (existed): {name}")
                        stats["updated"] += 1
                    except subprocess.CalledProcessError:
                        print(f"  {Colors.RED}Error creating/updating {name}: {e}{Colors.NC}")
                        stats["errors"] += 1

    return stats


def print_summary(stats: Dict[str, int], dry_run: bool) -> None:
    """Print summary of operations."""
    print()
    print(f"{Colors.BLUE}═══════════════════════════════════════════════════{Colors.NC}")
    print(f"{Colors.BLUE}  Summary{Colors.NC}")
    print(f"{Colors.BLUE}═══════════════════════════════════════════════════{Colors.NC}")

    if dry_run:
        print(f"{Colors.YELLOW}DRY-RUN MODE - No changes were made{Colors.NC}")

    print(f"{Colors.GREEN}✓{Colors.NC} Created:   {stats['created']}")
    print(f"{Colors.YELLOW}~{Colors.NC} Updated:   {stats['updated']}")
    print(f"  Unchanged: {stats['unchanged']}")

    if stats['errors'] > 0:
        print(f"{Colors.RED}✗{Colors.NC} Errors:    {stats['errors']}")

    print()

    if dry_run:
        print(f"{Colors.YELLOW}Run with --apply to actually sync these labels{Colors.NC}")


def main():
    parser = argparse.ArgumentParser(
        description="Sync GitHub labels from .github/labels.yml"
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Actually apply changes (default is dry-run)"
    )
    parser.add_argument(
        "--delete-existing",
        action="store_true",
        help="Delete all existing labels before syncing (dangerous!)"
    )

    args = parser.parse_args()

    # Print header
    print(f"{Colors.BLUE}═══════════════════════════════════════════════════{Colors.NC}")
    print(f"{Colors.BLUE}  ThemisDB GitHub Labels Sync{Colors.NC}")
    print(f"{Colors.BLUE}═══════════════════════════════════════════════════{Colors.NC}")
    print()

    if not args.apply:
        print(f"{Colors.YELLOW}⚠️  DRY-RUN MODE: No changes will be made{Colors.NC}")
        print(f"{Colors.YELLOW}   Run with --apply to actually sync labels{Colors.NC}")
        print()

    if args.delete_existing:
        print(f"{Colors.RED}⚠️  WARNING: Will delete all existing labels!{Colors.NC}")
        if args.apply:
            confirm = input("Are you sure? (yes/no): ")
            if confirm.lower() != "yes":
                print("Aborted.")
                sys.exit(0)
        print()

    # Check prerequisites
    if not check_gh_cli():
        sys.exit(1)

    # Find labels file
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent.parent
    labels_file = repo_root / ".github" / "labels.yml"

    # Load labels
    labels = load_labels(labels_file)
    print(f"Loaded {len(labels)} labels from {labels_file.name}\n")

    # Sync labels
    stats = sync_labels(labels, dry_run=not args.apply, delete_existing=args.delete_existing)

    # Print summary
    print_summary(stats, dry_run=not args.apply)

    print(f"{Colors.GREEN}Done!{Colors.NC}")


if __name__ == "__main__":
    main()
