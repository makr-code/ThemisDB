#!/usr/bin/env python3
"""
Verify an Ed25519 signature using OpenSSL CLI.

Inputs:
  --pubkey-b64     Base64 raw Ed25519 public key (32 bytes)
  --signature-b64  Base64 raw Ed25519 signature (64 bytes)
  --message        UTF-8 message string
"""

import argparse
import base64
import os
import subprocess
import sys
import tempfile


_ED25519_SPKI_PREFIX = bytes.fromhex("302a300506032b6570032100")


def _b64_decode(data: str, expected_len: int, label: str) -> bytes:
    try:
        raw = base64.b64decode(data, validate=True)
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
    parser = argparse.ArgumentParser(description="Verify Ed25519 detached signature.")
    parser.add_argument("--pubkey-b64", required=True)
    parser.add_argument("--signature-b64", required=True)
    parser.add_argument("--message", required=True)
    args = parser.parse_args()

    try:
        pub_key = _b64_decode(args.pubkey_b64, 32, "public key")
        sig = _b64_decode(args.signature_b64, 64, "signature")
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="themis-ed25519-verify-") as tmp:
        pub_der = os.path.join(tmp, "pubkey.der")
        pub_pem = os.path.join(tmp, "pubkey.pem")
        msg_bin = os.path.join(tmp, "message.bin")
        sig_bin = os.path.join(tmp, "signature.bin")

        with open(pub_der, "wb") as f:
            f.write(_ED25519_SPKI_PREFIX + pub_key)
        with open(msg_bin, "wb") as f:
            f.write(args.message.encode("utf-8"))
        with open(sig_bin, "wb") as f:
            f.write(sig)

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

    return 0


if __name__ == "__main__":
    sys.exit(main())

