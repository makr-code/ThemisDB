# FNV-1a 64-bit Non-Cryptographic Checksums for Data Integrity

**Metadaten:**
- Source: Fowler-Noll-Vo Hash Function Specification (public domain, 1991/1994)
- URL: http://www.isthe.com/chongo/tech/comp/fnv/
- Tags: data-integrity, hashing
- ThemisDB-Versionen: v2.0.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

Export/import workflows for large binary data bundles require a fast, lightweight checksum to detect accidental data corruption during transfer or storage, without the computational overhead of a cryptographic hash (SHA-256). The FNV-1a (Fowler-Noll-Vo, alternate version) hash function is a well-established public-domain non-cryptographic hash that is extremely simple to implement (a single XOR-then-multiply loop), produces a 64-bit value with good avalanche properties, and requires no external library.

ThemisDB uses FNV-1a 64-bit checksums in `src/prompt_engineering/prompt_library_io.cpp` to validate `PromptLibraryBundle` export files on import: the checksum is computed over the bundle payload bytes and stored in the file header; on import it is recomputed and compared before any deserialization occurs.

## 🎯 Core Principles

- **FNV-1a 64-bit variant**: The "alternate" (1a) version XORs before multiplying, which provides better avalanche for small inputs and sequential byte patterns compared to the original FNV-1 (multiply-then-XOR).
- **Standard constants**: FNV offset basis = `14695981039346656037ULL`; FNV prime = `1099511628211ULL`. These are well-known published constants; using any other values would break interoperability.
- **Checksum covers all payload bytes**: The checksum is computed over the complete serialized payload, including the length prefix but excluding the checksum field itself (which is zeroed before computation).
- **Stored in little-endian in file header**: The 64-bit checksum is serialized as 8 bytes, little-endian, in the bundle file header at a fixed offset.
- **Not a security boundary**: FNV-1a is not collision-resistant and must not be used for integrity verification in adversarial contexts. It detects accidental corruption only.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/prompt_engineering/prompt_library_io.cpp` — `computeFnv1a64(const uint8_t* data, size_t len)` inline function; called during `exportBundle()` (write checksum to header) and `importBundle()` (verify checksum before parse).
- `include/utils/fnv1a.h` — Canonical header-only implementation of FNV-1a 32 and 64-bit variants.

### What Was Adopted?

- Implementation:
  ```cpp
  constexpr uint64_t FNV_OFFSET_BASIS_64 = 14695981039346656037ULL;
  constexpr uint64_t FNV_PRIME_64        = 1099511628211ULL;

  inline uint64_t fnv1a_64(const uint8_t* data, size_t len) {
      uint64_t hash = FNV_OFFSET_BASIS_64;
      for (size_t i = 0; i < len; ++i) {
          hash ^= static_cast<uint64_t>(data[i]);
          hash *= FNV_PRIME_64;
      }
      return hash;
  }
  ```
- `exportBundle()`: serializes payload to `std::vector<uint8_t>`, writes zero checksum at header offset 0–7, computes `fnv1a_64(payload.data(), payload.size())`, writes result little-endian at header offset 0–7.
- `importBundle()`: reads header checksum, zeros header checksum field in buffer, recomputes `fnv1a_64`, compares; throws `BundleChecksumError` on mismatch.
- Bundle file format: `[8-byte FNV1a checksum][4-byte version][4-byte payload_len][payload bytes]`.

### Deviations & Rationale

- **64-bit rather than 32-bit**: FNV-1a 32-bit has a 1-in-4-billion chance of a false negative (corruption not detected). 64-bit reduces this to ~1 in 18 quintillion, which is negligible for bundle sizes up to several GiB.
- **FNV-1a over CRC32**: CRC32 provides better Hamming distance guarantees for burst errors in hardware (designed for network frames). FNV-1a is simpler to implement without lookup tables and sufficient for detecting software-level serialisation bugs and file corruption. CRC32 is reserved for lower-level storage integrity (RocksDB handles this internally).
- **Not versioned into the checksum**: The bundle format version is included in the checksum computation (it is part of the payload after the checksum field). This means a version mismatch is caught as a checksum failure. This is intentional — a version mismatch on an otherwise valid bundle is a programming error, not a corruption.

## ⚠️ Trade-offs & Limitations

- **Not collision-resistant**: FNV-1a is trivially exploitable for chosen-prefix collisions. It is appropriate only for detecting accidental corruption, not for integrity verification against a malicious actor. Bundle imports from untrusted sources should add a cryptographic signature layer.
- **No error-burst detection guarantees**: Unlike CRC32, FNV-1a has no polynomial-based burst error detection guarantees. For typical software-level corruption (serialization bugs, truncated files), it works well; for hardware-level bit-flip patterns, CRC32 or SHA would be preferred.
- **64-bit output stored in header adds 8 bytes to every bundle**: For the expected bundle sizes (kilobytes to megabytes), this overhead is negligible.
- **Endianness of checksum field**: The checksum value is stored little-endian. Importing bundles on a big-endian host requires byte-swapping the header checksum before comparison. ThemisDB currently only targets little-endian hosts (see `murmur_hash_deterministic_sharding.md`).

## 🔬 Validation

- [x] Code reviewed against FNV specification published constants
- [x] Unit tests in `tests/prompt_engineering/prompt_library_io_test.cpp` verify round-trip export/import and single-byte corruption detection
- [x] Fuzz test (`fuzz/prompt_library_io_fuzz.cpp`) verifies that `importBundle` throws `BundleChecksumError` for any mutated payload
- [x] Module README linked (`src/prompt_engineering/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [MurmurHash3 Deterministic Sharding](murmur_hash_deterministic_sharding.md)
- [RocksDB WriteBatch Atomicity](rocksdb_write_batch_atomicity.md)

---
**Last Updated:** 2026-04-06
