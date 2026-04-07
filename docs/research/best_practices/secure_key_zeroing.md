# Explicit Memory Zeroing of Cryptographic Key Material

**Metadaten:**
- Source: CERT C Coding Standard — MSC06-C "Be aware of compiler optimization when dealing with sensitive data"; GCC/Clang `explicit_bzero` (glibc 2.25+, POSIX.1-2024)
- URL: https://wiki.sei.cmu.edu/confluence/display/c/MSC06-C | https://man7.org/linux/man-pages/man3/explicit_bzero.3.html
- Tags: security, memory-safety
- ThemisDB-Versionen: v0.1.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

When a local array or `std::string` holding a passphrase or encryption key goes out of scope, the compiler may legally optimise away a `memset` call to zero it because the memory is "dead" (no subsequent read through the original pointer). This means the secret can persist in stack or heap memory and may be exposed via process memory dumps, core files, `/proc/<pid>/mem` reads, or side-channel attacks. CERT C MSC06-C documents this exact vulnerability and mandates using a zeroing function that the compiler cannot optimise away.

ThemisDB uses `explicit_bzero()` (glibc ≥2.25, available on all supported Linux targets) in the encrypted user-storage plugin to zero passphrase and derived key material immediately after use. On Windows, `SecureZeroMemory()` (Win32 API) is the equivalent; the cross-platform wrapper `secure_zero()` in `include/utils/secure_memory.h` selects the correct call.

## 🎯 Core Principles

- **Explicit_bzero over memset**: `explicit_bzero(ptr, size)` is defined to perform the zeroing even when the compiler determines the memory is dead; it is a compiler barrier. `memset` with a "dead write" elimination optimisation is unsafe for this purpose.
- **Volatile barrier alternative**: Where `explicit_bzero` is unavailable (old compilers, exotic targets), `volatile char* vp = ptr; while (size--) *vp++ = 0;` achieves the same effect because `volatile` writes cannot be elided.
- **Zero immediately after last use**: Key material is zeroed in the same function scope where it was used, as close to the last use as possible, before any early returns or exceptions can leave a cleanup gap.
- **RAII wrapper for automatic zeroing**: Where keys are stored in objects with non-trivial lifetimes, a `SecureBuffer` RAII wrapper calls `explicit_bzero` in its destructor, ensuring zeroing even on exception paths.
- **No key material in std::string**: `std::string` may leave copies in small-string-optimisation (SSO) buffers or reallocated heap blocks that are not zeroed. Key bytes are stored in fixed-size `std::array<uint8_t, N>` or `SecureBuffer` instead.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `plugins/user_storage_encrypted/` — `deliverKeyViaStdin` function: reads passphrase from stdin into a `char[512]` buffer; after `argon2id_hash_raw()`, calls `explicit_bzero(passphrase_buf, sizeof(passphrase_buf))` and `explicit_bzero(derived_key, key_len)`.
- `src/user_storage_encrypted/` — `encryptStore()` and `decryptStore()`: derived key is stored in `std::array<uint8_t, 32> key{}; /* ... */ explicit_bzero(key.data(), key.size());` on all exit paths.
- `include/utils/secure_memory.h` — `secure_zero(void* ptr, size_t len)` cross-platform wrapper; `SecureBuffer<N>` RAII class that calls `secure_zero` in destructor.

### What Was Adopted?

- `explicit_bzero(ptr, size)` from `<string.h>` (glibc 2.25+) used directly in all key-handling code paths.
- `SecureBuffer<32>` for AES-256 keys: `explicit_bzero` called in destructor and on move-assignment (zeroes old contents of moved-from buffer).
- CMake feature check (`check_function_exists(explicit_bzero)`) provides fallback to the `volatile` loop implementation for targets lacking glibc 2.25.
- Compiler sanitizer integration: AddressSanitizer tests are run with `ASAN_OPTIONS=detect_stack_use_after_return=1` to catch any accidental post-zero access.
- Code review checklist includes a mandatory "key zeroing" item for any PR touching `src/user_storage_encrypted/` or `plugins/user_storage_encrypted/`.

### Deviations & Rationale

- **Not applied to std::string passphrases received via HTTP API**: HTTP request body strings (user-supplied passphrases for the REST API) are `std::string` objects managed by the HTTP framework; their internal buffer cannot be reliably zeroed without custom allocators. The threat model accepts this limitation because network passphrases are transmitted over TLS and HTTP framework memory is outside the key-material security boundary. Mitigation: passphrases are not logged; HTTP connection buffers are not core-dumped.
- **No custom allocator for std::string**: A `secure_allocator<char>` that calls `explicit_bzero` on deallocation was evaluated but rejected as too invasive. `SecureBuffer<N>` covers the high-value cases (derived keys, session keys) with lower complexity.

## ⚠️ Trade-offs & Limitations

- **explicit_bzero availability**: Not available on all platforms (MSVC, older glibc, macOS < 10.12). The fallback volatile loop is functionally equivalent but not guaranteed by any standard. `SecureZeroMemory` (Windows) and `memset_s` (C11 Annex K) are alternatives on those platforms.
- **Compiler barriers may not prevent CPU caching**: `explicit_bzero` prevents the compiler from optimising away the write but does not flush CPU caches. An attacker with physical access and cache-probing capability (e.g., Meltdown/Spectre) might still observe the key in cache lines. This is out of scope for ThemisDB's threat model.
- **SecureBuffer does not protect against heap scanning**: The derived key lives on the heap between KDF completion and zeroing. A heap dump taken in that window would expose the key. Minimising the key's lifetime (zero immediately after last use) reduces the window.
- **Performance**: `explicit_bzero` is called at most once per key derivation operation, which is already orders of magnitude more expensive (Argon2id, 200–500 ms). The zeroing overhead is immeasurable.

## 🔬 Validation

- [x] Code reviewed against CERT C MSC06-C and the Linux `explicit_bzero` man page
- [x] Compiler output inspected with `objdump -d` to confirm zeroing instructions are not elided at `-O2`
- [x] Unit test verifies that `SecureBuffer` destructor overwrites contents (checked via `ASAN_OPTIONS=poison_after_alloc`)
- [x] Module README linked (`plugins/user_storage_encrypted/README.md`, `src/user_storage_encrypted/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [Argon2id KDF](argon2id_kdf.md)
- [TLS 1.3 Cipher Hardening](tls13_cipher_hardening.md)

---
**Last Updated:** 2026-04-06
