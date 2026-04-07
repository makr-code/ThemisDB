# Argon2id Key Derivation for Passphrase-to-Encryption-Key

**Metadaten:**
- Source: RFC 9106 — Argon2 Memory-Hard Functions for Password Hashing and Proof-of-Work Applications
- URL: https://www.rfc-editor.org/rfc/rfc9106
- Tags: security, encryption
- ThemisDB-Versionen: v0.1.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

Deriving an encryption key directly from a user passphrase using a fast hash (SHA-256, BLAKE2) is insecure because an attacker can test billions of candidates per second on commodity GPU hardware. RFC 9106 standardises Argon2id — the winner of the Password Hashing Competition — as the recommended memory-hard KDF for exactly this scenario. Argon2id combines the side-channel resistance of Argon2i (data-independent memory access) with the GPU-resistance of Argon2d (data-dependent memory access) using a hybrid two-pass algorithm.

ThemisDB uses Argon2id in its encrypted user-storage plugin (`src/user_storage_encrypted/`) to derive the 256-bit AES key from the user-supplied passphrase at both encryption and decryption time. The parameters m=65536 KiB, t=3 iterations, p=4 parallelism lanes are chosen to meet RFC 9106 §4 recommendations while keeping derivation time under 500 ms on typical server hardware.

## 🎯 Core Principles

- **Memory-hard KDF, never a fast hash**: Use Argon2id (or scrypt/bcrypt) whenever deriving a key from a passphrase; never SHA-256 or BLAKE2 alone.
- **Recommended RFC 9106 parameters**: m ≥ 64 MiB, t ≥ 3 iterations, p ≥ 4 lanes for interactive logins; higher for offline key derivation.
- **Random salt per derivation**: A 16-byte cryptographically random salt is generated at key-creation time and stored alongside the ciphertext; the same salt is used for all subsequent decryption attempts.
- **Side-channel hybrid mode (Argon2id)**: Argon2id is preferred over Argon2d (timing side-channels) or Argon2i (GPU attacks) for general use.
- **Key separation**: The derived key is used *only* for data-encryption-key (DEK) wrapping, not for authentication or signature verification.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/user_storage_encrypted/` — KDF invocation during `encryptStore()` and `decryptStore()`; salt is prepended to the encrypted blob.
- `plugins/user_storage_encrypted/` — `deliverKeyViaStdin` helper calls `argon2id_hash_raw()` from the `libargon2` C library.

### What Was Adopted?

- `argon2id_hash_raw(t_cost=3, m_cost=65536, parallelism=4, pwd, pwdlen, salt, saltlen, hash, hashlen)` is called to produce a 32-byte key.
- A 16-byte salt is generated with `RAND_bytes()` (OpenSSL) on first use and stored in the first 16 bytes of the encrypted file header.
- The derived key is fed directly to AES-256-GCM encryption; the GCM auth tag provides authenticated encryption.
- The same Argon2id call (with the persisted salt) is replayed on decryption; if the resulting key fails GCM tag verification, decryption is rejected.

### Deviations & Rationale

- **No pepper**: A server-side pepper (secret salt) was considered but omitted because the encrypted store is designed for scenarios where the server itself may be compromised (the passphrase remains the sole secret). Adding a pepper would provide marginal benefit while introducing key-management complexity.
- **m=65536 rather than RFC §4.5 minimum of 64 MiB for offline use**: 65536 KiB = 64 MiB exactly meets the RFC recommendation. For future HSM-backed tiers, higher memory cost may be used.
- **`libargon2` (C library) rather than a C++ wrapper**: The raw C API is used directly to avoid adding a heavy C++ dependency; this is consistent with the plugin's minimal-dependency philosophy.

## ⚠️ Trade-offs & Limitations

- **Derivation latency**: At m=65536, t=3, derivation takes ~200–500 ms on a single core. This is intentional (brute-force deterrent) but means the encrypted store cannot be unlocked at high frequency (e.g., per-request).
- **Memory allocation**: Each derivation allocates 64 MiB of working memory. Under concurrent unlock requests this can spike RSS. The encrypted store is designed for infrequent unlock (once per process startup).
- **No built-in key stretching for low-entropy passwords**: Argon2id is not a substitute for strong password policies. Low-entropy passphrases remain vulnerable to offline dictionary attacks given enough time and hardware.
- **Platform dependency**: `explicit_bzero` / `SecureZeroMemory` must be called after KDF to erase the passphrase and intermediate key from memory (see `secure_key_zeroing.md`).

## 🔬 Validation

- [x] Code reviewed against RFC 9106 parameter tables
- [x] Unit tests in `tests/user_storage_encrypted/` verify round-trip encrypt/decrypt with known vectors
- [x] Integration test verifies wrong passphrase → decryption failure (GCM tag mismatch)
- [x] Module README linked (`src/user_storage_encrypted/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [Secure Key Zeroing](secure_key_zeroing.md)
- [TLS 1.3 Cipher Hardening](tls13_cipher_hardening.md)

---
**Last Updated:** 2026-04-06
