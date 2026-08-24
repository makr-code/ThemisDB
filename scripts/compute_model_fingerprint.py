#!/usr/bin/env python3
"""
compute_model_fingerprint.py — AdaLoRA rebuild-gate helper.

Computes a stable fingerprint for a base language model file and writes it to
a cache directory so that the CMake AdaLoRA build gate can decide whether the
adapter needs to be rebuilt.

Exit codes:
  0   — fingerprint written / verified, adapter is UP-TO-DATE (no rebuild needed)
  1   — error (missing model path, I/O failure)
  2   — fingerprint CHANGED or cache absent (rebuild required)

Environment variables:
  THEMIS_LLM_MODEL_PATH      — path to the GGUF (or other) model file
  THEMIS_ADALORA_CACHE_DIR   — override for the cache directory
                               (default: ~/.themisdb/adalora_cache)
  THEMIS_ADALORA_FORCE_REBUILD — if set to "1" or "true", always exit 2
  THEMIS_FINGERPRINT_MB       — how many MB from the file head to hash
                               (default: 64)
"""

from __future__ import annotations

import hashlib
import json
import os
import pathlib
import sys
import time
import struct


# ---------------------------------------------------------------------------
# Configuration helpers
# ---------------------------------------------------------------------------

def _cache_dir() -> pathlib.Path:
    env = os.environ.get("THEMIS_ADALORA_CACHE_DIR", "")
    if env:
        return pathlib.Path(env)
    return pathlib.Path.home() / ".themisdb" / "adalora_cache"


def _model_path() -> pathlib.Path | None:
    env = os.environ.get("THEMIS_LLM_MODEL_PATH", "")
    if env:
        p = pathlib.Path(env)
        if p.is_file():
            return p
    return None


def _fingerprint_mb() -> int:
    try:
        return max(1, int(os.environ.get("THEMIS_FINGERPRINT_MB", "64")))
    except ValueError:
        return 64


def _force_rebuild() -> bool:
    val = os.environ.get("THEMIS_ADALORA_FORCE_REBUILD", "").lower()
    return val in ("1", "true", "yes")


# ---------------------------------------------------------------------------
# Fingerprint computation
# ---------------------------------------------------------------------------

def _gguf_metadata_hash(model_file: pathlib.Path) -> str:
    """
    Extract GGUF file header metadata (magic + version + quant type when present)
    as a secondary hash component to detect in-place model replacements that
    preserve file size but change the model identity.

    If the file is not GGUF or the header is malformed the function returns an
    empty string (the caller falls back to the byte-hash alone).
    """
    try:
        with open(model_file, "rb") as fh:
            magic = fh.read(4)
            if magic != b"GGUF":
                return ""
            version_bytes = fh.read(4)
            if len(version_bytes) < 4:
                return ""
            version = struct.unpack_from("<I", version_bytes)[0]
            # Include magic + version in the metadata digest
            meta_hash = hashlib.sha256(magic + version_bytes)
            # Read another 8 bytes (tensor count / metadata kv count) if available
            extra = fh.read(8)
            if extra:
                meta_hash.update(extra)
            return meta_hash.hexdigest()[:16]  # short prefix only
    except OSError:
        return ""


def compute_fingerprint(model_file: pathlib.Path, head_mb: int) -> str:
    """
    Return a 64-char hex SHA-256 fingerprint for the given model file.

    The fingerprint is derived from:
      - the first ``head_mb`` megabytes of the file (fast partial hash)
      - the file size (to catch truncation / partial downloads)
      - optional GGUF metadata hash (version + tensor descriptor prefix)

    Rationale: reading only the head avoids hashing multi-GB files on every
    CMake configure, while being robust enough to detect model updates.
    """
    head_bytes = head_mb * 1024 * 1024
    hasher = hashlib.sha256()

    # 1. File size
    file_size = model_file.stat().st_size
    hasher.update(struct.pack("<Q", file_size))

    # 2. Head bytes
    with open(model_file, "rb") as fh:
        remaining = head_bytes
        while remaining > 0:
            chunk = fh.read(min(65536, remaining))
            if not chunk:
                break
            hasher.update(chunk)
            remaining -= len(chunk)

    # 3. GGUF metadata suffix (empty string for non-GGUF files → no-op)
    meta = _gguf_metadata_hash(model_file)
    if meta:
        hasher.update(meta.encode())

    return hasher.hexdigest()  # 64 hex chars


# ---------------------------------------------------------------------------
# Cache read / write
# ---------------------------------------------------------------------------

FINGERPRINT_FILENAME = "model.fingerprint"


def _fingerprint_path(cache_dir: pathlib.Path) -> pathlib.Path:
    return cache_dir / FINGERPRINT_FILENAME


def _read_cached_fingerprint(cache_dir: pathlib.Path) -> str | None:
    fp_file = _fingerprint_path(cache_dir)
    if not fp_file.is_file():
        return None
    try:
        data = json.loads(fp_file.read_text(encoding="utf-8"))
        return data.get("fingerprint", None)
    except (json.JSONDecodeError, OSError):
        return None


def _write_fingerprint(cache_dir: pathlib.Path, fingerprint: str,
                       model_path: pathlib.Path) -> None:
    cache_dir.mkdir(parents=True, exist_ok=True)
    data = {
        "fingerprint": fingerprint,
        "model_path": str(model_path),
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "format_version": 1,
    }
    fp_file = _fingerprint_path(cache_dir)
    fp_file.write_text(json.dumps(data, indent=2), encoding="utf-8")


# ---------------------------------------------------------------------------
# Stamp-file for CMake dependency tracking
# ---------------------------------------------------------------------------

STAMP_REBUILD_REQUIRED = "adalora_rebuild_required.stamp"
STAMP_UP_TO_DATE = "adalora_up_to_date.stamp"


def _write_stamp(cache_dir: pathlib.Path, rebuild: bool) -> None:
    cache_dir.mkdir(parents=True, exist_ok=True)
    if rebuild:
        (cache_dir / STAMP_REBUILD_REQUIRED).touch()
        stale = cache_dir / STAMP_UP_TO_DATE
        if stale.exists():
            stale.unlink()
    else:
        (cache_dir / STAMP_UP_TO_DATE).touch()
        stale = cache_dir / STAMP_REBUILD_REQUIRED
        if stale.exists():
            stale.unlink()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    if _force_rebuild():
        print("[adalora-fingerprint] THEMIS_ADALORA_FORCE_REBUILD set — rebuild required",
              flush=True)
        cache_dir = _cache_dir()
        _write_stamp(cache_dir, rebuild=True)
        return 2

    model_path = _model_path()
    if model_path is None:
        env_val = os.environ.get("THEMIS_LLM_MODEL_PATH", "<not set>")
        print(f"[adalora-fingerprint] ERROR: model file not found "
              f"(THEMIS_LLM_MODEL_PATH={env_val})", flush=True)
        return 1

    head_mb = _fingerprint_mb()
    print(f"[adalora-fingerprint] model: {model_path} "
          f"(size={model_path.stat().st_size:,} bytes, head={head_mb} MB)", flush=True)

    current_fp = compute_fingerprint(model_path, head_mb)
    print(f"[adalora-fingerprint] fingerprint: {current_fp}", flush=True)

    cache_dir = _cache_dir()
    cached_fp = _read_cached_fingerprint(cache_dir)

    if cached_fp == current_fp:
        print("[adalora-fingerprint] cache HIT — adapter is up-to-date, no rebuild needed",
              flush=True)
        _write_stamp(cache_dir, rebuild=False)
        return 0

    if cached_fp is None:
        reason = "no existing cache"
    else:
        reason = f"fingerprint changed ({cached_fp[:12]}… → {current_fp[:12]}…)"

    print(f"[adalora-fingerprint] cache MISS ({reason}) — rebuild required", flush=True)
    _write_fingerprint(cache_dir, current_fp, model_path)
    _write_stamp(cache_dir, rebuild=True)
    return 2


if __name__ == "__main__":
    sys.exit(main())
