# Signed Plugin Repository with Key Pinning

**Date:** 26 February 2026  
**Version:** 1.0.0  
**Category:** Plugins / Security  
**Implementation:** `include/plugins/signed_plugin_repository.h`, `src/plugins/signed_plugin_repository.cpp`

---

## Overview

`SignedPluginRepository` is a thread-safe, in-process catalog for plugin entries where every entry must carry a valid **Ed25519 signature**. The signature is verified against an explicitly managed set of **pinned public keys** (*key pinning*) before an entry is accepted.

Key pinning prevents:
- **Man-in-the-middle attacks** on the plugin distribution channel
- **Registry substitution attacks** (replacement of the plugin index by an attacker)
- Loading plugins signed by a key that has since been revoked

The concept is analogous to HTTP Public Key Pinning (HPKP) for TLS or GPG keyring management in package managers such as apt/dnf.

---

## Core Concepts

### PinnedKey

```cpp
struct PinnedKey {
    std::string fingerprint;         // Hex-SHA-256 of the raw 32-byte Ed25519 public key
    std::vector<uint8_t> public_key; // 32-byte raw Ed25519 public key
    std::string label;               // Human-readable label, e.g. "ThemisDB Official"
    bool active = true;              // false = key deactivated (key rotation)
};
```

The `active` flag allows keys to be deactivated during key rotation without deleting them. Deactivated keys are retained for audit purposes.

### RepositoryEntry

```cpp
struct RepositoryEntry {
    MarketplaceManifest manifest;  // Full plugin manifest
    std::string signature_b64;    // Base64-encoded Ed25519 signature over the canonical JSON
    std::string key_fingerprint;  // Fingerprint of the signing key
};
```

The signature is created over the **canonical JSON** of the manifest (fields in alphabetical order, no whitespace) — not over the serialised `MarketplaceManifest` struct directly.

### Canonical JSON

The signing/verification payload contains the following fields in alphabetical order:

```json
{
  "author":             "...",
  "binary_linux":       "plugin.so",
  "binary_macos":       "plugin.dylib",
  "binary_windows":     "plugin.dll",
  "description":        "...",
  "expected_hash":      "<sha256-hex>",
  "license":            "MIT",
  "min_themis_version": "1.0.0",
  "name":               "my_plugin",
  "type":               "custom",
  "verified_publisher": false,
  "version":            "1.0.0"
}
```

> Only these fields are part of the signed payload. Display fields such as `homepage`, `repository`, `tags`, and `category` are explicitly excluded.

---

## Usage

### 1. Pin a key

```cpp
#include "plugins/signed_plugin_repository.h"

themis::plugins::SignedPluginRepository repo;

// Load the public key from configuration / certificate
std::vector<uint8_t> raw_pub_key = load_file_bytes("/etc/themis/certs/repo_pub.bin");

themis::plugins::PinnedKey official;
official.fingerprint = themis::plugins::SignedPluginRepository::computeKeyFingerprint(raw_pub_key);
official.public_key  = raw_pub_key;
official.label       = "ThemisDB Official Repository";
official.active      = true;

repo.addPinnedKey(official);
```

### 2. Load a signed index

```cpp
// Receive entries from the registry endpoint
for (const auto& entry : fetch_registry_index()) {
    if (!repo.addEntry(entry)) {
        // Signature invalid or key not pinned
        spdlog::error("Rejected repository entry: {}", entry.manifest.name);
    }
}
```

### 3. Look up a plugin at load time

```cpp
// Find the latest version
auto entry = repo.findEntry("s3_blob_storage");
if (!entry) {
    throw std::runtime_error("Plugin not found in signed repository");
}

// Find a specific version
auto entry_v2 = repo.findEntry("s3_blob_storage", "2.0.0");
```

---

## Security Model

### Verification flow

```
addEntry(RepositoryEntry)
    │
    ▼
[Lock mutex_]
    │
    ├── findPinnedKeyLocked(entry.key_fingerprint)
    │       └── key not found / inactive → return false
    │
    ├── base64Decode(entry.signature_b64)
    │       └── empty/invalid → return false
    │
    ├── canonicalManifestJson(entry.manifest) → payload
    │
    ├── verifyEd25519Signature(key.public_key, payload, sig)
    │       └── sig invalid → return false
    │
    └── Insert/replace entry in catalog
[Unlock mutex_]
```

> **Important:** Verification and insertion are performed under the same mutex.
> This prevents TOCTOU attacks (Time-of-Check/Time-of-Use) where a key could be
> deactivated between verification and insertion.

### Key Rotation

```cpp
// Add the new key
PinnedKey new_key;
new_key.fingerprint = computeKeyFingerprint(new_pub_key);
new_key.public_key  = new_pub_key;
new_key.active      = true;
repo.addPinnedKey(new_key);

// Deactivate the old key (retains the key for audit)
PinnedKey old_deactivated = old_key;
old_deactivated.active = false;
repo.addPinnedKey(old_deactivated);  // replaces the existing entry

// Fully remove (when no longer needed)
repo.removePinnedKey(old_fingerprint);
```

---

## Performance

| Operation                        | Target     | Measured        |
|----------------------------------|------------|-----------------|
| Ed25519 signature verification   | < 1 ms     | ~0.1–0.3 ms     |
| SHA-256 fingerprint computation  | < 0.1 ms   | ~0.01 ms        |
| Canonical JSON (100 chars)       | < 0.1 ms   | < 0.01 ms       |

Performance is validated by an automated test in `tests/test_signed_plugin_repository.cpp` (`VerifyEntryMeetsPerformanceTarget`).

---

## Thread Safety

All public methods are thread-safe and protected by a single `std::mutex`. The mutex is held for the entire duration of `addEntry` (including signature verification) to prevent TOCTOU windows.

---

## Signing Workflow

### Signing a plugin entry (Python example)

```python
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat
import json, base64, hashlib

def canonical_json(manifest: dict) -> str:
    fields = ["author", "binary_linux", "binary_macos", "binary_windows",
              "description", "expected_hash", "license", "min_themis_version",
              "name", "type", "verified_publisher", "version"]
    ordered = {k: manifest[k] for k in fields if k in manifest}
    return json.dumps(ordered, separators=(',', ':'), sort_keys=True)

# Sign
private_key = Ed25519PrivateKey.generate()
payload = canonical_json(manifest).encode()
signature = private_key.sign(payload)
signature_b64 = base64.b64encode(signature).decode()

# Compute fingerprint of the public key
pub_raw = private_key.public_key().public_bytes(Encoding.Raw, PublicFormat.Raw)
fingerprint = hashlib.sha256(pub_raw).hexdigest()
```

---

## Related Files

| File                                               | Description                             |
|----------------------------------------------------|-----------------------------------------|
| `include/plugins/signed_plugin_repository.h`      | API definition                          |
| `src/plugins/signed_plugin_repository.cpp`        | Implementation                          |
| `tests/test_signed_plugin_repository.cpp`         | Unit tests (36 tests)                   |
| `include/plugins/plugin_interface.h`              | `MarketplaceManifest`, `PluginSignatureInfo` |
| `docs/de/plugins/SIGNED_PLUGIN_REPOSITORY.md`    | German version of this document         |

---

## Related Issues and PRs

- Issue #1571: Signed plugin repository with key pinning (this implementation)
- Issue #1554: Hot-reload support (Q2 2026)
- Issue #1572: WASM-based plugin isolation (planned)
