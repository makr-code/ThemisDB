# MurmurHash3-32 for Deterministic Non-Cryptographic Key Hashing

**Metadaten:**
- Source: Austin Appleby — MurmurHash3 (public domain, 2011)
- URL: https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
- Tags: hashing, sharding
- ThemisDB-Versionen: v1.9.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

Non-cryptographic hash functions are used wherever deterministic, fast, and well-distributed hashing is needed but cryptographic security is not required: sharding, consistent hash rings, A/B experiment variant assignment, hash-table implementations. MurmurHash3 (Austin Appleby, 2011) is the de-facto standard for this role: it is public domain, has excellent avalanche properties, is CPU-friendly (no division, no branch), and produces a 32-bit or 128-bit output.

ThemisDB uses MurmurHash3-32 with a fixed seed (`0x9747b28c`) in two key subsystems: variant assignment in A/B experiments (`src/prompt_engineering/prompt_ab_experiment.cpp`) so that the same user always gets the same variant regardless of which node handles the request, and key sharding across storage partitions (`src/sharding/`).

## 🎯 Core Principles

- **Fixed seed for reproducibility**: The seed `0x9747b28c` is a compile-time constant shared across all nodes. Changing the seed changes all variant assignments and shard mappings, so the seed is treated as a schema-level configuration parameter with migration requirements.
- **32-bit output for range mapping**: A 32-bit hash value is mapped to a variant bucket via `hash % num_variants` for A/B experiments and to a shard via `hash % num_shards` for partitioning. 32 bits provides 4 billion distinct values — sufficient for any practical key space.
- **No cryptographic use**: MurmurHash3 is NOT collision-resistant and must never be used for security-sensitive purposes (HMAC, token generation, password hashing, etc.).
- **Inline implementation**: The MurmurHash3 implementation is inlined in a header (`include/utils/murmur_hash3.h`) to enable compiler inlining at call sites; no dynamic dispatch.
- **Input normalisation**: Keys are normalised (lowercased, trimmed) before hashing to prevent the same logical key mapping to different variants due to case differences.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/prompt_engineering/prompt_ab_experiment.cpp` — `AssignVariant(user_id, experiment_id, num_variants)` uses `MurmurHash3_32(user_id + ":" + experiment_id, 0x9747b28c) % num_variants`.
- `src/sharding/` — `ShardRouter::getShard(key)` uses `MurmurHash3_32(key, 0x9747b28c) % num_shards` for primary shard assignment.
- `src/server/distributed_gateway.cpp` — Consistent hash ring positions are computed with MurmurHash3-32 (see `consistent_hash_ring.md`).
- `include/utils/murmur_hash3.h` — Canonical inline implementation; all callers use this header.

### What Was Adopted?

- `uint32_t MurmurHash3_32(const void* key, int len, uint32_t seed)` — verbatim Appleby implementation with the `fmix32` finaliser.
- Seed `0x9747b28c` is a named constant `THEMISDB_HASH_SEED` in `include/utils/murmur_hash3.h`.
- A/B assignment: `uint32_t h = MurmurHash3_32(key.data(), key.size(), THEMISDB_HASH_SEED); variant = h % num_variants;`
- Shard assignment: same formula with `num_shards`.
- For string keys, the input is the UTF-8 byte representation of the normalised key.
- Collision rate monitoring: the experiment subsystem periodically logs the empirical variant distribution; a >5% imbalance triggers an alert.

### Deviations & Rationale

- **128-bit variant not used**: MurmurHash3_x64_128 provides better avalanche for very large key spaces but requires two 64-bit outputs and slightly more computation. For the current key spaces (user IDs, prompt IDs), 32 bits provides <0.00003% collision probability for up to 10 million unique keys — sufficient.
- **Seed is fixed, not configurable per-experiment**: Allowing per-experiment seeds would allow more statistical isolation between experiments but would complicate the seed management story. A single global seed is used; experiment isolation is achieved via the `experiment_id` concatenated into the key.
- **No smhasher validation harness in CI**: The Appleby smhasher test suite is not run as part of CI. The MurmurHash3 implementation is taken verbatim from the reference and treated as a well-known constant; modifications would require re-running smhasher validation.

## ⚠️ Trade-offs & Limitations

- **Not hash-DoS resistant**: If user-controlled keys feed directly into a hash table using MurmurHash3, an adversary can craft inputs that all map to the same bucket. ThemisDB only uses MurmurHash3 for offline shard/variant assignment on server-controlled key combinations (user_id + experiment_id), not for user-controlled hash-map keys.
- **Seed change requires migration**: Changing `THEMISDB_HASH_SEED` remaps all shard assignments and variant memberships. A migration procedure (double-write to old and new shard, then cut over) is documented in the sharding module.
- **32-bit output limits unique shards**: With 32-bit output, up to ~65,536 shards can be safely used before birthday paradox effects cause non-trivial collision rates. ThemisDB's maximum shard count is 1,024, well within this limit.
- **Endianness**: MurmurHash3 produces different results on big-endian vs. little-endian systems. ThemisDB runs exclusively on little-endian (x86-64, ARM64 LE); cross-platform compatibility is not a goal.

## 🔬 Validation

- [x] Code reviewed against Appleby's reference implementation (commit `61a0530`)
- [x] Unit tests in `tests/utils/murmur_hash3_test.cpp` verify known-answer test vectors from smhasher
- [x] A/B experiment distribution tests verify <5% imbalance across 10M synthetic user IDs
- [x] Shard distribution test confirms Pearson chi-squared p > 0.05 for uniform distribution
- [x] Module README linked (`src/prompt_engineering/README.md`, `src/sharding/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [Consistent Hash Ring](consistent_hash_ring.md)
- [FNV-1a Checksums](fnv1a_checksums.md)

---
**Last Updated:** 2026-04-06
