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

python3 - "${VCPKG_JSON}" "${SBOM_FILE}" <<'PYTHON'
import json, sys

vcpkg_json_path, sbom_file_path = sys.argv[1], sys.argv[2]

# Load vcpkg.json to extract registered dependencies
with open(vcpkg_json_path) as f:
    vcpkg = json.load(f)

registered = set()
for dep in vcpkg.get("dependencies", []):
    if isinstance(dep, str):
        registered.add(dep.lower())
    elif isinstance(dep, dict) and "name" in dep:
        registered.add(dep["name"].lower())

# Load the CycloneDX SBOM
with open(sbom_file_path) as f:
    sbom = json.load(f)


# Non-C++ ecosystems present in a full-repo SBOM scan (npm, cargo, Go, etc.)
# are not managed via vcpkg.json and must be excluded from this check.
NON_CPP_PURL_PREFIXES = (
    "pkg:npm/",
    "pkg:cargo/",
    "pkg:golang/",
    "pkg:maven/",
    "pkg:pypi/",
    "pkg:gem/",
    "pkg:nuget/",
    "pkg:composer/",
    "pkg:swift/",
    "pkg:pub/",
    "pkg:hackage/",
    "pkg:hex/",
    "pkg:conda/",
    "pkg:conan/",
)

components = sbom.get("components", [])
unregistered = []
checked = 0
for comp in components:
    name = (comp.get("name") or "").lower()
    pkg_type = (comp.get("type") or "")
    purl = (comp.get("purl") or "").lower()

    # Only check library-type components (skip OS packages, etc.)
    if pkg_type not in ("library", "framework"):
        continue

    # Skip components from non-C++ ecosystems; they are not in vcpkg.json
    if any(purl.startswith(p) for p in NON_CPP_PURL_PREFIXES):
        continue

    # Skip standard system libraries (e.g. libssl, glibc, musl, openssl, zlib)
    if any(name.startswith(p) for p in ("lib", "glibc", "musl", "openssl", "zlib")):
        continue

    checked += 1
    if name and name not in registered:
        unregistered.append(f"{name} (type={pkg_type})")

if unregistered:
    print(f"[verify-sbom] WARNING: {len(unregistered)} unregistered C++ component(s) (checked {checked} of {len(components)} total):")
    for u in unregistered:
        print(f"  - {u}")
    sys.exit(1)
else:
    print(f"[verify-sbom] All {checked} C++ SBOM components verified against vcpkg.json ({len(components)} total in SBOM)")
    sys.exit(0)
PYTHON
