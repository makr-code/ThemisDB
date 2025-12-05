# semantic_cache.cpp

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Src

---


Path: `src/cache/semantic_cache.cpp`

Purpose: Implements an exact‑match semantic cache used for LLM responses. Uses RocksDB Column Family, supports TTL and basic statistics.

Public functions / symbols:
- `for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {`
- `if (entry.ttl_seconds <= 0) {`
- `if (cf_handle_) {`
- `if (total_queries > 0) {`
- ``

Notes / TODOs:
- Link to header `include/cache/semantic_cache.h` for API details.
