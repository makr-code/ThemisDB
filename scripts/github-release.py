"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            github-release.py                                  ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:23:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     224                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c7f3d5d227  2026-01-12  fix: Disable docs database generation and fix fmt compile... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB v1.4.0 GitHub Release Automation Script

Creates a GitHub Release with:
- Release notes from RELEASE_NOTES.md
- All binaries and packages
- SHA256 checksums
- GPG signatures (optional)

Requirements:
  pip install PyGithub
  
Usage:
  python3 scripts/github-release.py --version v1.4.0 --token <GitHub PAT>
"""

import os
import sys
import argparse
import hashlib
import json
from pathlib import Path
from datetime import datetime

def get_sha256(file_path):
    """Calculate SHA256 checksum of a file."""
    sha256_hash = hashlib.sha256()
    with open(file_path, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def read_release_notes(repo_root, version):
    """Read RELEASE_NOTES.md for version."""
    release_notes_path = Path(repo_root) / "release" / version / "RELEASE_NOTES.md"
    if release_notes_path.exists():
        with open(release_notes_path, "r", encoding="utf-8") as f:
            return f.read()
    return "See CHANGELOG.md for details."

def create_checksums_file(packages_dir, output_file):
    """Create SHA256SUMS file from all packages."""
    checksums = []
    for package_file in sorted(Path(packages_dir).glob("themisdb-*.zip")) + \
                         sorted(Path(packages_dir).glob("themisdb-*.tar.gz")):
        if package_file.is_file():
            sha256 = get_sha256(str(package_file))
            checksums.append(f"{sha256}  {package_file.name}")
            print(f"  {package_file.name}: {sha256}")
    
    if checksums:
        with open(output_file, "w", encoding="utf-8") as f:
            f.write("\n".join(checksums) + "\n")
        print(f"\n✅ Checksums written to {output_file}")
    return checksums

def main():
    parser = argparse.ArgumentParser(
        description="Create GitHub Release for ThemisDB"
    )
    parser.add_argument(
        "--version",
        default="v1.4.0",
        help="Release version (default: v1.4.0)"
    )
    parser.add_argument(
        "--token",
        help="GitHub Personal Access Token"
    )
    parser.add_argument(
        "--repo",
        default="makr-code/ThemisDB",
        help="GitHub repository (owner/repo)"
    )
    parser.add_argument(
        "--release-dir",
        default="release",
        help="Release directory"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be done without uploading"
    )
    
    args = parser.parse_args()
    
    repo_root = Path(__file__).parent.parent
    packages_dir = repo_root / args.release_dir / args.version
    
    print("=" * 60)
    print(f"ThemisDB {args.version} GitHub Release Automation")
    print("=" * 60)
    print(f"Repository: {args.repo}")
    print(f"Packages directory: {packages_dir}")
    print(f"Dry run: {args.dry_run}")
    print()
    
    # Create checksums
    print("[1/3] Creating checksums...")
    checksums_file = packages_dir / "SHA256SUMS"
    checksums = create_checksums_file(str(packages_dir), str(checksums_file))
    
    # Prepare release notes
    print("\n[2/3] Reading release notes...")
    release_notes = read_release_notes(str(repo_root), args.version)
    print(f"  Release notes: {len(release_notes)} bytes")
    
    # List files to upload
    print("\n[3/3] Files for GitHub Release:")
    files_to_upload = []
    
    # Find all release artifacts
    for ext in ["zip", "tar.gz", "gz", "deb", "rpm"]:
        for file_path in packages_dir.glob(f"themisdb-*.{ext}"):
            if file_path.is_file():
                files_to_upload.append(file_path)
                print(f"  - {file_path.name} ({file_path.stat().st_size / (1024*1024):.1f} MB)")
    
    # Add checksums
    if checksums_file.exists():
        files_to_upload.append(checksums_file)
        print(f"  - {checksums_file.name}")
    
    # Add signature file if exists
    sig_file = checksums_file.with_suffix(".gpg")
    if sig_file.exists():
        files_to_upload.append(sig_file)
        print(f"  - {sig_file.name}")
    
    if not files_to_upload:
        print("❌ No release files found!")
        return 1
    
    print(f"\nTotal files: {len(files_to_upload)}")
    
    if args.dry_run:
        print("\n⚠️  DRY RUN: Would create GitHub Release with above files")
        print("   Run without --dry-run to actually create the release")
        return 0
    
    # Try to create GitHub Release using PyGithub
    if not args.token:
        print("\n❌ GitHub token not provided (use --token)")
        return 1
    
    try:
        from github import Github
    except ImportError:
        print("\n⚠️  PyGithub not installed, showing manual instructions...")
        print("\nTo create the release manually:")
        print(f"  1. Go to: https://github.com/{args.repo}/releases/new")
        print(f"  2. Tag version: {args.version}")
        print(f"  3. Title: ThemisDB {args.version}")
        print(f"  4. Description: (paste content of RELEASE_NOTES.md)")
        print(f"  5. Attach files from: {packages_dir}")
        return 0
    
    print("\nCreating GitHub Release with PyGithub...")
    try:
        g = Github(args.token)
        repo = g.get_repo(args.repo)
        
        # Get release notes
        body = release_notes + "\n\n## Checksums\n\n```\n" + \
               "\n".join(checksums) + "\n```"
        
        # Create release
        release = repo.create_git_release(
            tag=args.version,
            name=f"ThemisDB {args.version}",
            message=body,
            draft=False,
            prerelease=False
        )
        
        print(f"✅ Release created: {release.html_url}")
        
        # Upload assets
        print("\nUploading assets...")
        for file_path in files_to_upload:
            print(f"  Uploading {file_path.name}...")
            with open(file_path, "rb") as f:
                release.upload_asset(
                    name=file_path.name,
                    content_type="application/octet-stream",
                    asset_file=f
                )
        
        print("\n✅ GitHub Release completed successfully!")
        print(f"   Release URL: {release.html_url}")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
