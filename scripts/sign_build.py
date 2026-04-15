"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sign_build.py                                      ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-15 04:15:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     145                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0ae938d481  2026-04-15  feat(updates): anonymous hardware telemetry + Ed25519 bui... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
scripts/sign_build.py
─────────────────────────────────────────────────────────────────────────────
ThemisDB official-build manifest signer.

Constructs the canonical manifest string:
    "<channel>|<version>|<build_id>|<timestamp>"

Signs it with an Ed25519 private key and outputs the Base64-encoded signature
so the CI can pass it to CMake as -DTHEMIS_BUILD_SIG=<sig>.

Usage (CI):
    python3 scripts/sign_build.py \\
        --channel   official \\
        --version   2.0.0 \\
        --build-id  $(git rev-parse --short HEAD) \\
        --timestamp $(date -u +%Y-%m-%dT%H:%M:%SZ) \\
        --key       <base64-ed25519-private-key>

    # Pipe output into a CMake flag:
    SIG=$(python3 scripts/sign_build.py ...)
    cmake ... -DTHEMIS_BUILD_CHANNEL=official -DTHEMIS_BUILD_SIG="$SIG"

The private key is passed via the --key argument or the
THEMIS_BUILD_PRIVKEY environment variable (env takes precedence over --key).
Never commit a real private key to the repository.

Key generation (one-time, by the ThemisDB project maintainers):
    python3 -c "
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
    from cryptography.hazmat.primitives.serialization import (
        Encoding, PrivateFormat, PublicFormat, NoEncryption)
    import base64, sys

    priv = Ed25519PrivateKey.generate()
    pub  = priv.public_key()

    priv_b64 = base64.b64encode(
        priv.private_bytes(Encoding.Raw, PrivateFormat.Raw, NoEncryption())
    ).decode()
    pub_b64  = base64.b64encode(
        pub.public_bytes(Encoding.Raw, PublicFormat.Raw)
    ).decode()

    print('PRIVATE (keep secret, store as GitHub Secret THEMIS_BUILD_PRIVKEY):')
    print(priv_b64)
    print()
    print('PUBLIC (embed in include/updates/build_info.h.in as THEMIS_BUILD_PUBKEY):')
    print(pub_b64)
    "

Dependencies:
    pip install cryptography
─────────────────────────────────────────────────────────────────────────────
"""

import argparse
import base64
import os
import sys


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Sign a ThemisDB build manifest with Ed25519."
    )
    parser.add_argument("--channel",   required=True,
                        help="Build channel, e.g. 'official'")
    parser.add_argument("--version",   required=True,
                        help="ThemisDB version string, e.g. '2.0.0'")
    parser.add_argument("--build-id",  required=True,
                        help="Short Git SHA, e.g. 'a1b2c3d'")
    parser.add_argument("--timestamp", required=True,
                        help="UTC ISO-8601 timestamp, e.g. '2026-04-14T19:00:00Z'")
    parser.add_argument("--key",       default="",
                        help="Base64-encoded Ed25519 raw private key (32 bytes). "
                             "Falls back to THEMIS_BUILD_PRIVKEY env var.")
    parser.add_argument("--verify",    action="store_true",
                        help="After signing, verify the signature and print the result.")
    args = parser.parse_args()

    # Resolve private key (env var takes precedence).
    key_b64: str = os.environ.get("THEMIS_BUILD_PRIVKEY", args.key).strip()
    if not key_b64:
        print("ERROR: No private key provided via --key or THEMIS_BUILD_PRIVKEY.",
              file=sys.stderr)
        sys.exit(1)

    try:
        from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
        from cryptography.hazmat.primitives.serialization import (
            Encoding, PublicFormat
        )
    except ImportError:
        print("ERROR: 'cryptography' package not installed.  "
              "Run: pip install cryptography", file=sys.stderr)
        sys.exit(1)

    # Decode private key.
    try:
        priv_raw = base64.b64decode(key_b64)
    except Exception as exc:
        print(f"ERROR: Cannot Base64-decode private key: {exc}", file=sys.stderr)
        sys.exit(1)

    if len(priv_raw) != 32:
        print(f"ERROR: Private key must be 32 bytes raw (got {len(priv_raw)}).",
              file=sys.stderr)
        sys.exit(1)

    try:
        priv_key = Ed25519PrivateKey.from_private_bytes(priv_raw)
    except Exception as exc:
        print(f"ERROR: Cannot load Ed25519 private key: {exc}", file=sys.stderr)
        sys.exit(1)

    # Build canonical manifest.
    manifest = f"{args.channel}|{args.version}|{args.build_id}|{args.timestamp}"
    print(f"[sign_build] manifest = \"{manifest}\"", file=sys.stderr)

    # Sign.
    sig_raw = priv_key.sign(manifest.encode())
    sig_b64 = base64.b64encode(sig_raw).decode()

    # Optional verification.
    if args.verify:
        pub_key = priv_key.public_key()
        pub_b64 = base64.b64encode(
            pub_key.public_bytes(Encoding.Raw, PublicFormat.Raw)
        ).decode()
        print(f"[sign_build] public key = {pub_b64}", file=sys.stderr)
        try:
            pub_key.verify(sig_raw, manifest.encode())
            print("[sign_build] ✓ Signature verified successfully.", file=sys.stderr)
        except Exception as exc:
            print(f"[sign_build] ✗ Verification failed: {exc}", file=sys.stderr)
            sys.exit(1)

    # Print the Base64 signature to stdout for capture by CI.
    print(sig_b64)


if __name__ == "__main__":
    main()
