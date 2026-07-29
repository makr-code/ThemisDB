#!/usr/bin/env python3
"""
Verify detached Ed25519 signatures for plugin manifests.
"""

import argparse
import base64
import os
import subprocess
import sys
import tempfile

DEFAULT_OWNER_PUBKEY_B64 = "11qYAYKxCrfVS/7TyWQHOg7hcvPapiMlrwIaaPcHURo="
_ED25519_SPKI_PREFIX = bytes.fromhex("302a300506032b6570032100")


def _decode_b64(value: str, expected_len: int, label: str) -> bytes:
    try:
        raw = base64.b64decode(value, validate=True)
    except Exception as exc:
        raise ValueError(f"invalid base64 for {label}: {exc}") from exc
    if len(raw) != expected_len:
        raise ValueError(f"{label} must be {expected_len} bytes (got {len(raw)})")
    return raw


def _run(cmd: list[str]) -> None:
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip() or "unknown OpenSSL error"
        raise RuntimeError(detail)


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify plugin manifest signature.")
    parser.add_argument("manifest", help="Path to plugin manifest file (e.g. plugin.json)")
    parser.add_argument("signature", help="Path to detached base64 signature file")
    parser.add_argument(
        "--pubkey-b64",
        default=os.environ.get("THEMIS_PLUGIN_MANIFEST_PUBKEY_B64", DEFAULT_OWNER_PUBKEY_B64),
        help="Owner Ed25519 public key in base64 (32-byte raw key).",
    )
    args = parser.parse_args()

    if not os.path.exists(args.manifest):
        print(f"ERROR: Manifest not found: {args.manifest}", file=sys.stderr)
        return 2
    if not os.path.exists(args.signature):
        print(f"ERROR: Signature not found: {args.signature}", file=sys.stderr)
        return 2

    try:
        pubkey = _decode_b64(args.pubkey_b64.strip(), 32, "public key")
        with open(args.signature, "r", encoding="utf-8") as sig_file:
            signature_b64 = sig_file.read().strip()
        signature = _decode_b64(signature_b64, 64, "signature")
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    try:
        with open(args.manifest, "rb") as manifest_file:
            manifest_bytes = manifest_file.read()
    except OSError as exc:
        print(f"ERROR: cannot read manifest: {exc}", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="themis-manifest-verify-") as tmp:
        pub_der = os.path.join(tmp, "pubkey.der")
        pub_pem = os.path.join(tmp, "pubkey.pem")
        msg_bin = os.path.join(tmp, "manifest.bin")
        sig_bin = os.path.join(tmp, "signature.bin")

        with open(pub_der, "wb") as f:
            f.write(_ED25519_SPKI_PREFIX + pubkey)
        with open(msg_bin, "wb") as f:
            f.write(manifest_bytes)
        with open(sig_bin, "wb") as f:
            f.write(signature)

        try:
            _run(["openssl", "pkey", "-pubin", "-inform", "DER", "-in", pub_der, "-out", pub_pem])
            _run(
                [
                    "openssl",
                    "pkeyutl",
                    "-verify",
                    "-pubin",
                    "-inkey",
                    pub_pem,
                    "-rawin",
                    "-in",
                    msg_bin,
                    "-sigfile",
                    sig_bin,
                ]
            )
        except RuntimeError as exc:
            print(f"ERROR: signature verification failed: {exc}", file=sys.stderr)
            return 1

    print("OK: plugin manifest signature verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

