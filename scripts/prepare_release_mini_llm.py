#!/usr/bin/env python3
"""Prepare a small GGUF model for release bundles and runtime images."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
import urllib.request
from pathlib import Path
from typing import Optional


DEFAULT_MODEL_NAME = "tinyllama-1.1b-chat-v1.0-q4_k_m.gguf"
DEFAULT_ALIAS = "default.gguf"
DEFAULT_SOURCE_URL = (
    "https://huggingface.co/bartowski/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/"
    "TinyLlama-1.1B-Chat-v1.0-Q4_K_M.gguf?download=true"
)
DEFAULT_MODEL_ID = "TinyLlama-1.1B-Chat-v1.0"
DEFAULT_QUANTIZATION = "Q4_K_M"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def download_file(url: str, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(url, headers={"User-Agent": "ThemisDB Release Builder/1.0"})
    with urllib.request.urlopen(request) as response, destination.open("wb") as output:
        shutil.copyfileobj(response, output)


def ensure_alias(source: Path, alias_path: Path) -> str:
    if alias_path.exists() or alias_path.is_symlink():
        alias_path.unlink()

    try:
        os.link(source, alias_path)
        return "hardlink"
    except OSError:
        pass

    try:
        alias_path.symlink_to(source.name)
        return "symlink"
    except OSError:
        shutil.copy2(source, alias_path)
        return "copy"


def prepare_model(
    output_dir: Path,
    model_name: str,
    alias_name: str,
    source_url: Optional[str],
    source_file: Optional[Path],
    expected_sha256: Optional[str],
    force: bool,
) -> dict:
    output_dir.mkdir(parents=True, exist_ok=True)
    model_path = output_dir / model_name

    if force and model_path.exists():
        model_path.unlink()

    if not model_path.exists():
        if source_file is not None:
            shutil.copy2(source_file, model_path)
            source_desc = str(source_file)
        elif source_url:
            download_file(source_url, model_path)
            source_desc = source_url
        else:
            raise ValueError("Either --source-url or --source-file is required when the model is not present")
    else:
        source_desc = "existing-file"

    actual_sha256 = sha256_file(model_path)
    if expected_sha256 and actual_sha256.lower() != expected_sha256.lower():
        raise RuntimeError(
            f"SHA256 mismatch for {model_path.name}: expected {expected_sha256}, got {actual_sha256}"
        )

    alias_method = ensure_alias(model_path, output_dir / alias_name)
    model_size = model_path.stat().st_size

    manifest = {
        "model_id": DEFAULT_MODEL_ID,
        "model_name": model_path.name,
        "alias": alias_name,
        "quantization": DEFAULT_QUANTIZATION,
        "backend": "llama.cpp",
        "source": source_desc,
        "sha256": actual_sha256,
        "size_bytes": model_size,
        "alias_method": alias_method,
    }

    manifest_path = output_dir / "mini-llm.manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare a deterministic mini GGUF model for release packaging")
    parser.add_argument("--output-dir", required=True, help="Directory where the model bundle should be created")
    parser.add_argument("--model-name", default=DEFAULT_MODEL_NAME, help="File name to use inside the bundle")
    parser.add_argument("--alias", default=DEFAULT_ALIAS, help="Compatibility alias file name")
    parser.add_argument("--source-url", default=DEFAULT_SOURCE_URL, help="Download URL for the GGUF model")
    parser.add_argument("--source-file", help="Use a local GGUF file instead of downloading")
    parser.add_argument("--expected-sha256", help="Optional checksum verification for the downloaded model")
    parser.add_argument("--force", action="store_true", help="Replace existing model files")
    args = parser.parse_args()

    source_file = Path(args.source_file).resolve() if args.source_file else None
    if source_file is not None and not source_file.exists():
        print(f"Local source file not found: {source_file}", file=sys.stderr)
        return 1

    try:
        manifest = prepare_model(
            output_dir=Path(args.output_dir).resolve(),
            model_name=args.model_name,
            alias_name=args.alias,
            source_url=args.source_url,
            source_file=source_file,
            expected_sha256=args.expected_sha256,
            force=args.force,
        )
    except Exception as exc:
        print(f"Failed to prepare mini LLM bundle: {exc}", file=sys.stderr)
        return 1

    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())