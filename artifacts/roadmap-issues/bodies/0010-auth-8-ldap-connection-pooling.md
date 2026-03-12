### Context

This issue implements the roadmap item 'LDAP Connection Pooling' for the auth domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.2.0.

Primary detail section: 8. LDAP Connection Pooling

### Goal

Deliver the scoped changes for LDAP Connection Pooling in src/auth/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### 8. LDAP Connection Pooling

**Priority:** High  
**Target Version:** v1.2.0

`ldap_authenticator.cpp` opens a new LDAP connection (TCP + TLS handshake + bind) for **every authentication call** (`performBind()`, lines 188-286 on Windows; lines 307-395 on POSIX). LDAP connection setup including TLS typically takes 10–50 ms. Under load (e.g., 500 concurrent logins) this exhausts file descriptors and introduces severe latency.

**Implementation Notes:**
- `[ ]` Implement `LDAPConnectionPool` class in `ldap_authenticator.cpp` (or new `ldap_connection_pool.cpp`): pool of pre-bound `LDAP*` handles, protected by `std::mutex` + `std::condition_variable`, configurable `min_idle`, `max_size`, and `checkout_timeout`
- `[ ]` On `authenticate()`, checkout a connection from the pool (blocking up to `checkout_timeout`), perform bind/search, return connection to pool (RAII via `PooledConnection` wrapper)
- `[ ]` Implement connection health check: on checkout, test the connection with `ldap_search_ext_s` to `""` base with scope `LDAP_SCOPE_BASE` requesting `supportedLDAPVersion`; evict and re-create if stale
- `[ ]` Expose `pool_size`, `idle_connections`, `active_connections` via `auth_metrics.cpp` counters

**Performance Targets:**
- Average LDAP authentication latency reduced from ~30 ms to < 5 ms under sustained load via connection reuse

---

### Acceptance Criteria

- [ ] Implement `LDAPConnectionPool` class in `ldap_authenticator.cpp` (or new `ldap_connection_pool.cpp`): pool of pre-bound `LDAP*` handles, protected by `std::mutex` + `std::condition_variable`, configurable `min_idle`, `max_size`, and `checkout_timeout`
- [ ] On `authenticate()`, checkout a connection from the pool (blocking up to `checkout_timeout`), perform bind/search, return connection to pool (RAII via `PooledConnection` wrapper)
- [ ] Implement connection health check: on checkout, test the connection with `ldap_search_ext_s` to `""` base with scope `LDAP_SCOPE_BASE` requesting `supportedLDAPVersion`; evict and re-create if stale
- [ ] Expose `pool_size`, `idle_connections`, `active_connections` via `auth_metrics.cpp` counters
- [ ] Average LDAP authentication latency reduced from ~30 ms to < 5 ms under sustained load via connection reuse

### Relationships

- Roadmap row: #10 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#8-ldap-connection-pooling
- Source key: roadmap:10:auth:v1.2.0:8-ldap-connection-pooling

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:10:auth:v1.2.0:8-ldap-connection-pooling -->
<!-- roadmap-ref: row=10;module=auth;target=v1.2.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#8-ldap-connection-pooling -->
