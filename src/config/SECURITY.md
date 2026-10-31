---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Security Notes — Config Module

Report vulnerabilities through the project-level
[`SECURITY.md`](../../SECURITY.md).

## Scope and trust boundaries

The config components receive caller-selected paths and JSON/YAML/schema
content, observe filesystem events, and may hold encrypted-store state. They do
not authenticate callers, authorize service actions, provide external key
management, or define the consuming application's reload policy.

## Implemented controls and limits

- The resolver normalizes paths, checks path constraints in its own code, and
  returns an optional result or throws rather than silently reporting an
  unavailable file as resolved. Do not infer sandboxing from path mapping.
- The validator supports document-local `$ref`; external refs are rejected.
  It evaluates a subset of schema keywords. Unknown formats are accepted and
  the implementation uses simplified regular-expression checks for some
  formats; this is not a security policy engine.
- Watcher notification clears the resolver cache through its callback; it does
  not reload or atomically apply settings. Debouncing may combine event bursts.
- The resolver audit log is disabled by default and stores path metadata in
  bounded process memory. It is not a durable, operator-attributed security
  audit trail.
- `ConfigEncryptedStore` uses AES-256-GCM and verifies IV/tag lengths during
  decryption. `serialize()` contains the current key bytes (base64-encoded);
  protect the entire snapshot as key material. This component is not an
  external KMS.
- `tryGet()` catches decryption/decoding failures and returns `std::nullopt`;
  use `get()` where callers must distinguish absence from crypto failure.

## Verification gap

`include/config/config_contract.h` declares additional bounds and failure
semantics. The current focused tests and implementation have not been compared
against every declaration. **TODO:** complete a contract-to-code audit before
claiming these declarations as enforced security controls. See
[`MODULE_GAPS.md`](MODULE_GAPS.md).

For incidents, preserve redacted diagnostics, restrict access to affected
artifacts, and follow the owning service's response procedure and the project
security reporting process.
