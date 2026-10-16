#!/usr/bin/env python3
"""Extract release artifact and SBOM digests from downloaded workflow artifact ZIP files."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import zipfile

SHA_PATTERN = re.compile(r"\b([0-9a-fA-F]{64})\b")
ARCHIVE_EXTENSIONS = (".zip", ".tar.gz", ".tgz", ".msi", ".deb", ".rpm")


def extract_digests(artifacts_dir: pathlib.Path) -> dict[str, str]:
    artifact_digest = "unknown"
    sbom_digest = "unknown"

    for zip_path in sorted(artifacts_dir.glob("*.zip")):
        if not zip_path.is_file():
            continue
        try:
            with zipfile.ZipFile(zip_path) as archive:
                for member in archive.namelist():
                    low = member.lower()
                    name = pathlib.Path(member).name.lower()
                    payload = archive.read(member)

                    if artifact_digest == "unknown" and name == "sha256sums.txt":
                        text = payload.decode("utf-8", errors="ignore")
                        for line in text.splitlines():
                            if any(ext in line.lower() for ext in ARCHIVE_EXTENSIONS):
                                match = SHA_PATTERN.search(line)
                                if match:
                                    artifact_digest = match.group(1).lower()
                                    break
                        if artifact_digest == "unknown":
                            match = SHA_PATTERN.search(text)
                            if match:
                                artifact_digest = match.group(1).lower()

                    if sbom_digest == "unknown" and "sbom" in low:
                        if name.endswith((".sha256", ".sha512")):
                            text = payload.decode("utf-8", errors="ignore")
                            match = SHA_PATTERN.search(text)
                            if match:
                                sbom_digest = match.group(1).lower()
                        elif name.endswith((".json", ".spdx", ".cdx")):
                            sbom_digest = hashlib.sha256(payload).hexdigest()

                    if artifact_digest != "unknown" and sbom_digest != "unknown":
                        break

                if artifact_digest != "unknown" and sbom_digest != "unknown":
                    break
        except zipfile.BadZipFile:
            continue

    return {"artifact_digest": artifact_digest, "sbom_digest": sbom_digest}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Extract artifact/SBOM digests from ZIP artifacts.")
    parser.add_argument("--artifacts-dir", required=True, help="Directory containing downloaded artifact ZIP files.")
    parser.add_argument("--output", required=True, help="Output JSON file path.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    artifacts_dir = pathlib.Path(args.artifacts_dir).resolve()
    output_path = pathlib.Path(args.output).resolve()

    result = extract_digests(artifacts_dir)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(result), encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
