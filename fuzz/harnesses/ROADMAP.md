# fuzz/harnesses ROADMAP

## Current Status
- [x] 10 harnesses implemented (aql_parser, gguf_loader, grammar, http_parser, jwt_rbac_config, ldap_dn, pii_redaction, postgres_importer, security_input_validator, security_policy_engine)
- [x] Corpus seeds for all 11 target areas
- [x] Dictionaries for aql, crypto, json, pii
- [ ] CI workflow for automated fuzzing not yet active

## In Progress
- [~] fuzzing.yml CI workflow implementation (Target: Q3 2026)

## Planned Features
- [ ] Add dictionary for jwt, ldap_dn, policy_engine, rbac, http, importer targets (Target: Q3 2026)
- [ ] Crash triage automation with CASR (Target: Q4 2026)
- [ ] Coverage-guided corpus minimization (Target: Q4 2026)
- [ ] OSS-Fuzz integration (Target: 2027-Q1)

## Implementation Phases
### Phase 1: Design / API-Vertrag
- [x] Harness interface: LLVMFuzzerTestOneInput(const uint8_t*, size_t)
- [x] Corpus seed structure per target
- [ ] CMake fuzz_targets integration (Target: Q3 2026)

### Phase 2: Core-Implementierung
- [x] 10 harnesses implemented
- [x] Corpus seeds for all targets
- [ ] CI workflow active (Target: Q3 2026)

### Phase 3: Fehlerbehandlung & Edge Cases
- [ ] Dictionary coverage for all targets (Target: Q3 2026)
- [ ] Sanitizer configuration (ASan + UBSan) verified per harness (Target: Q3 2026)

### Phase 4: Tests
- [ ] Sanity-test: each harness runs without crash on seed corpus (Target: Q3 2026)
- [ ] CI gate: no new crashes on seed inputs (Target: Q4 2026)

### Phase 5: Performance/Hardening
- [ ] Persistent mode enabled in all harnesses (Target: Q4 2026)
- [ ] Corpus minimization (afl-cmin) for all targets (Target: Q4 2026)

### Phase 6: Dokumentation & Abnahme
- [ ] Runbook: crash triage and severity classification (Target: Q4 2026)
- [ ] Integration with security pentest evidence bundle (Target: Q4 2026)

## Production Readiness Checklist
- [x] Harness implementations present
- [x] Seed corpus present for all targets
- [ ] CI workflow active
- [ ] Crash-free on seed corpus verified
- [ ] Dictionary coverage ≥ 80% of targets
- [ ] Crash triage runbook available

## Known Issues & Limitations
- [!] `fuzzing.yml` workflow referenced in README but not yet implemented (fixed by this change)
- [!] Dictionary coverage: only aql, crypto, json, pii — missing jwt, ldap_dn, policy_engine, rbac, http, importer
- [!] CMake fuzz_targets build target not yet verified in CI

## Breaking Changes
- None.
