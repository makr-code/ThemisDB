<!-- Status: current | validated: 2026-04-06 -->

# Security — include/themis/

> Security scope, threat model, and mitigations for the public themis core headers.
> Implementation-level security controls are documented in [`../../src/themis/SECURITY.md`](../../src/themis/SECURITY.md).

---

## Scope

This document covers security considerations for code that **includes** the
public headers in `include/themis/`.  It addresses:

- License gate bypass and feature-tier escalation
- Module integrity (hash and signature verification)
- Edition spoofing and downgrade attacks
- Build-info disclosure
- Plugin/extension trust model

---

## Threat Model

| Threat | Impact | Mitigation |
|--------|--------|-----------|
| **License gate bypass** — caller ignores `GateResult` from `RuntimeLicenseGate::check()` | Unlicensed access to enterprise/cloud features | `check()` is `[[nodiscard]]`; compiler warning on ignored result; internal enforcement in feature implementations independently of gate |
| **Module hash collision / substitution** — attacker replaces module binary with a crafted one sharing the same registered SHA-256 hash | Arbitrary code execution via trusted module path | SHA-256 preimage resistance makes accidental collision negligible; `ModuleSignatureVerifier` provides a second independent check using asymmetric cryptography |
| **Signature key compromise** — attacker obtains ThemisDB module-signing private key | Attacker can sign and load malicious modules | Signing key stored in HSM; key rotation procedure documented in `../../src/themis/SECURITY.md`; `module_signature_verifier.h` will support certificate-chain revocation in v1.8.0 |
| **Edition downgrade attack** — attacker patches binary to report `Edition::COMMUNITY` at runtime | Enterprise/cloud features silently disabled; DoS | `EditionManager` reads edition from a signed license token, not from a plaintext config; tampered token fails signature verification |
| **License replay** — attacker replays an expired license token | Continued use after license expiry | License tokens include `not_after` timestamp; `LicenseInfo` expiry is checked on every `check()` call; `GRACE_PERIOD` status is surfaced to the application |
| **Build-info probing** — attacker reads `BuildInfo::features()` bitmask to enumerate available attack surface | Feature map disclosure aids targeted exploitation | `BuildInfo` is informational; sensitive capability gates are enforced by `RuntimeLicenseGate`, not by presence/absence in the bitmask; bitmask should not be logged in production |
| **`THEMISDB_DEPRECATED` suppression** — caller suppresses deprecation warnings and continues to use removed-in-next-version symbols | Use of insecure legacy API paths | Deprecation warnings are compiler-enforced; deprecated symbols will be removed on the documented schedule; security-critical deprecated APIs are additionally guarded with `[[deprecated("security: use X instead")]]` |
| **Subdirectory header injection** (`base/`, `gpu/`, `network/`) — attacker places malicious headers on the include path before `include/themis/` | Malicious header silently replaces framework types | Use of CMake `target_include_directories` with explicit absolute paths; do not rely on system-wide include-path ordering for security-critical headers |
| **GateResult denial reason absent** (see AUDIT.md Finding 1) | Operators cannot distinguish license-expired from tier-too-low; may mask security events | Mitigated in v1.7.1 by adding `LicenseDenialReason`; until then, consult `LicenseInfo::status()` separately |

---

## Security Controls

### Fail-Closed Verification
- `ModuleHashVerifier::verify()` and `ModuleSignatureVerifier::verify()` return a
  typed `*VerificationResult`; the `allowed()` method returns `false` on any
  error, including missing hash/signature data
- `RuntimeLicenseGate::check()` returns `GateResult` with `allowed = false` on
  expired license, tier mismatch, or verification failure — never a silent pass

### `[[nodiscard]]` Enforcement
All security-critical functions are annotated `[[nodiscard]]`:
- `ModuleHashVerifier::verify()`
- `ModuleSignatureVerifier::verify()`
- `RuntimeLicenseGate::check()`

Ignoring the return value is a compile-time warning (treated as error in CI).

### No Private Material in Public Headers
- `LicenseInfo` stores an opaque token handle; the raw license bytes and
  signature are held in the implementation (`src/themis/`)
- `ModuleSignatureVerifier` accepts public keys only; private signing keys are
  never present in any public header or public API

### Symbol Visibility
- `export.h` restricts exported symbols to those explicitly annotated with
  `THEMISDB_API`; all internal implementation symbols have default hidden
  visibility (`-fvisibility=hidden` / `/d2hiddenregions`)

---

## Known Limitations

1. **`GateResult` lacks denial reason** (AUDIT.md Finding 1) — until v1.7.1,
   callers cannot programmatically distinguish the reason for a gate denial;
   workaround: call `LicenseInfo::status()` and `EditionCapabilities::has(flag)`
   separately to diagnose.
2. **No certificate revocation in v1.7.0** — `ModuleSignatureVerifier` does not
   perform OCSP or CRL checks; revocation is planned for v1.8.0.  Operators
   should monitor for signing key compromise and deploy updated public-key pins
   via the normal upgrade path.
3. **Async license refresh not yet available** — `RuntimeLicenseGate` does not
   self-refresh; long-running daemons must implement their own refresh loop using
   `LicenseInfo::status()` until v1.9.0.
4. **`BuildInfo::features()` bitmask disclosure** — the feature bitmask reveals
   the compiled-in capability set; do not expose this value in unauthenticated
   API responses or log files.
5. **Subdirectory headers (`base/`, `gpu/`, `network/`) not covered** — the
   scope of this document is the top-level `include/themis/` headers.  Security
   documentation for subdirectories is maintained separately.
