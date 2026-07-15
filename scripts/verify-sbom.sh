#!/usr/bin/env bash
# verify-sbom.sh — Verify SBOM against vcpkg.json registered dependencies
#
# Phase 6.1: Cross-checks the generated CycloneDX SBOM against vcpkg.json to
# detect unregistered transitive dependencies.
#
# Usage:
#   verify-sbom.sh <sbom-file.cyclonedx.json>
#
# Exit codes:
#   0  All dependencies accounted for
#   1  Unregistered dependencies found
#   2  Missing inputs or parse error

set -euo pipefail

SBOM_FILE="${1:-}"
VCPKG_JSON="vcpkg.json"

if [[ -z "${SBOM_FILE}" || ! -f "${SBOM_FILE}" ]]; then
    echo "Usage: verify-sbom.sh <sbom-file.cyclonedx.json>" >&2
    exit 2
fi

if [[ ! -f "${VCPKG_JSON}" ]]; then
    echo "ERROR: ${VCPKG_JSON} not found in $(pwd)" >&2
    exit 2
fi

echo "[verify-sbom] SBOM file:    ${SBOM_FILE}"
echo "[verify-sbom] vcpkg.json:   ${VCPKG_JSON}"

python3 - <<PYTHON
import json, sys

# Load vcpkg.json to extract registered dependencies
with open("${VCPKG_JSON}") as f:
    vcpkg = json.load(f)

registered = set()
for dep in vcpkg.get("dependencies", []):
    if isinstance(dep, str):
        registered.add(dep.lower())
    elif isinstance(dep, dict) and "name" in dep:
        registered.add(dep["name"].lower())

# Load the CycloneDX SBOM
with open("${SBOM_FILE}") as f:
    sbom = json.load(f)

components = sbom.get("components", [])
unregistered = []
for comp in components:
    name = (comp.get("name") or "").lower()
    pkg_type = (comp.get("type") or "")
    # Only check library-type components (skip OS packages, etc.)
    if pkg_type not in ("library", "framework"):
        continue
    # Skip standard system libraries
    if any(name.startswith(p) for p in ("lib", "glibc", "musl", "openssl", "zlib")):
        # Only flag if not in vcpkg.json
        pass
    if name and name not in registered:
        unregistered.append(f"{name} (type={pkg_type})")

if unregistered:
    print(f"[verify-sbom] WARNING: {len(unregistered)} unregistered component(s):")
    for u in unregistered:
        print(f"  - {u}")
    sys.exit(1)
else:
    print(f"[verify-sbom] All {len(components)} SBOM components verified against vcpkg.json")
    sys.exit(0)
PYTHON
