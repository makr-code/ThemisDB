#!/usr/bin/env python3
"""
THEMIS Release SBOM Generator
Generates Software Bill of Materials for release packages
"""

import os
import json
import hashlib
from pathlib import Path
from datetime import datetime

def get_sha256(filepath):
    """Calculate SHA256 hash of a file"""
    sha256_hash = hashlib.sha256()
    with open(filepath, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def generate_sbom(version, release_dir="release"):
    """Generate SBOM for given version"""
    
    print(f"Generating SBOM for v{version}...")
    
    # Find packages
    packages = []
    release_path = Path(release_dir)
    
    for ext in ['*.zip', '*.deb', '*.rpm']:
        for file in release_path.glob(f"*{version}*{ext}"):
            if file.is_file():
                size_mb = file.stat().st_size / (1024 * 1024)
                sha256 = get_sha256(str(file))
                
                packages.append({
                    "filename": file.name,
                    "size_mb": round(size_mb, 2),
                    "sha256": sha256
                })
                
                print(f"  ✓ {file.name} ({size_mb:.2f} MB)")
    
    if not packages:
        print(f"  ⚠ No packages found for v{version}")
        return
    
    # Create SBOM
    sbom = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.4",
        "version": 1,
        "metadata": {
            "timestamp": datetime.utcnow().isoformat() + "Z",
            "tools": [
                {
                    "vendor": "makr-code",
                    "name": "THEMIS Release Pipeline",
                    "version": "1.0"
                }
            ],
            "component": {
                "type": "application",
                "name": "ThemisDB",
                "version": version,
                "description": "High-performance vector + relational database"
            }
        },
        "components": [
            {
                "type": "application",
                "name": pkg["filename"],
                "version": version,
                "hashes": [
                    {
                        "alg": "SHA-256",
                        "content": pkg["sha256"]
                    }
                ]
            }
            for pkg in packages
        ]
    }
    
    # Write SBOM
    sbom_file = release_path / f"SBOM_v{version}.json"
    with open(sbom_file, "w") as f:
        json.dump(sbom, f, indent=2)
    
    print(f"\n✅ SBOM created: {sbom_file}")
    
    # Write manifest (plain text)
    manifest_file = release_path / f"MANIFEST_v{version}.txt"
    with open(manifest_file, "w") as f:
        f.write(f"THEMIS Release v{version}\n")
        f.write(f"Generated: {datetime.now().isoformat()}\n")
        f.write(f"Packages: {len(packages)}\n\n")
        for pkg in packages:
            f.write(f"{pkg['filename']}\n")
            f.write(f"  Size: {pkg['size_mb']} MB\n")
            f.write(f"  SHA256: {pkg['sha256']}\n\n")
    
    print(f"✅ Manifest created: {manifest_file}")

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python generate_sbom.py <version>")
        sys.exit(1)
    
    version = sys.argv[1]
    release_dir = sys.argv[2] if len(sys.argv) > 2 else "release"
    
    generate_sbom(version, release_dir)
