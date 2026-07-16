### Context

This issue implements the roadmap item 'Constant-Time Comparison for Recovery Codes and Session IDs' for the auth domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.1.0.

Primary detail section: 4. Constant-Time Comparison for Recovery Codes and Session IDs

### Goal

Deliver the scoped changes for Constant-Time Comparison for Recovery Codes and Session IDs in src/auth/ and complete the linked detail section in a release-ready state for v1.1.0.

### Detailed Scope

### 4. Constant-Time Comparison for Recovery Codes and Session IDs

**Priority:** High (Security)  
**Target Version:** v1.1.0

`api_key_authenticator.cpp:272` already uses `CRYPTO_memcmp()` for secret comparison — correct. However, other comparators in the module are not constant-time:

- `mfa_authenticator.cpp:173`: recovery code lookup uses `std::find` / iterator comparison (`it != enrollment.recovery_codes.end()`). An attacker who can measure sub-microsecond timing differences can deduce the position of the matching recovery code in the list via early-exit short-circuit.
- `session_manager.cpp:205`: session ID lookup uses `unordered_map::find`, which compares `std::string` keys with `operator==` — subject to timing oracle for session token brute-force.
- `totp_replay_cache.cpp`: TOTP code comparison inside `markUsed` uses `std::unordered_set` integer hash lookup — effectively constant-time for integers, but worth documenting explicitly.

**Implementation Notes:**
- `[ ]` In `mfa_authenticator.cpp`, replace `std::find` over recovery codes with a loop using `CRYPTO_memcmp()` that always iterates all entries regardless of match, then returns true/false after full traversal (prevents early-exit timing leak) (line 173)
- `[ ]` In `session_manager.cpp`, store session IDs as their SHA-256 hash in the lookup map; compare incoming tokens by hashing them first, which normalises comparison time regardless of input content (`session_manager.cpp:205`, `sessions_` member)
- `[ ]` Add microbenchmark that measures TOTP/recovery-code verification latency variance under ThreadSanitizer to confirm constant-time behaviour

**Performance Targets:**
- Recovery code verification time must vary by < 100 ns regardless of match position in a list of 10 codes

---

### Acceptance Criteria

- [ ] `mfa_authenticator.cpp:173`: recovery code lookup uses `std::find` / iterator comparison (`it != enrollment.recovery_codes.end()`). An attacker who can measure sub-microsecond timing differences can deduce the position of the matching recovery code in the list via early-exit short-circuit.
- [ ] `session_manager.cpp:205`: session ID lookup uses `unordered_map::find`, which compares `std::string` keys with `operator==` — subject to timing oracle for session token brute-force.
- [ ] `totp_replay_cache.cpp`: TOTP code comparison inside `markUsed` uses `std::unordered_set` integer hash lookup — effectively constant-time for integers, but worth documenting explicitly.
- [ ] In `mfa_authenticator.cpp`, replace `std::find` over recovery codes with a loop using `CRYPTO_memcmp()` that always iterates all entries regardless of match, then returns true/false after full traversal (prevents early-exit timing leak) (line 173)
- [ ] In `session_manager.cpp`, store session IDs as their SHA-256 hash in the lookup map; compare incoming tokens by hashing them first, which normalises comparison time regardless of input content (`session_manager.cpp:205`, `sessions_` member)
- [ ] Add microbenchmark that measures TOTP/recovery-code verification latency variance under ThreadSanitizer to confirm constant-time behaviour
- [ ] Recovery code verification time must vary by < 100 ns regardless of match position in a list of 10 codes

### Relationships

- Roadmap row: #5 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#4-constant-time-comparison-for-recovery-codes-and-session-ids
- Source key: roadmap:5:auth:v1.1.0:4-constant-time-comparison-for-recovery-codes-and-session-ids

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:5:auth:v1.1.0:4-constant-time-comparison-for-recovery-codes-and-session-ids -->
<!-- roadmap-ref: row=5;module=auth;target=v1.1.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#4-constant-time-comparison-for-recovery-codes-and-session-ids -->
