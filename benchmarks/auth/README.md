# Auth benchmarks

- **AI_Used:** true
- **AI_Models:** unknown
- **AI_Contributor:** GitHub Copilot
- **Human_Review:** false
- **Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

`bench_auth_hotpaths.cpp` registers the auth hot-path cases AHP-02 through
AHP-11. Its file header describes AHP-01 JWT parsing, but no AHP-01 benchmark
implementation or registration is present. The registered cases measure
in-memory blacklist lookups, session operations, single-node RocksDB revocation,
realm lookups, an in-memory
LDAP-bind simulation, and OIDC token parsing. AHP-09 through AHP-11 are cases
in the same `bench_auth_hotpaths` executable; `bench_auth_protocol_micro` is
their label, not a separately registered target.

The benchmark source and target are registered conditionally in
[`../CMakeLists.txt`](../CMakeLists.txt) as `bench_auth_hotpaths`. The code
defines release gates GATE-AHP-01 through GATE-AHP-06, mapped to AHP-02 through
AHP-07. The remaining cases do not establish additional release gates. Source thresholds are targets, not
hardware-measured results; measured baselines remain unverified until produced
on representative hardware.

See [`../../src/auth/PERFORMANCE_EXPECTATIONS.md`](../../src/auth/PERFORMANCE_EXPECTATIONS.md)
for the documented benchmark mapping and baseline procedure.
