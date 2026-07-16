### Context

This issue implements the roadmap item 'LDAP DN and Filter Injection Prevention' for the auth domain. It is sourced from the consolidated roadmap under 🔴 Critical Priority and targets milestone v1.1.0.

Primary detail section: 3. LDAP DN and Filter Injection Prevention

### Goal

Deliver the scoped changes for LDAP DN and Filter Injection Prevention in src/auth/ and complete the linked detail section in a release-ready state for v1.1.0.

### Detailed Scope

### 3. LDAP DN and Filter Injection Prevention

**Priority:** Critical (Security)  
**Target Version:** v1.1.0

`ldap_authenticator.cpp:buildUserDN()` (lines 90-97) substitutes the raw `username` string into a DN template by replacing the `{username}` placeholder with no escaping at all. An attacker supplying a username containing DN special characters (`,`, `=`, `+`, `<`, `>`, `#`, `;`, `\`, `"`) can manipulate the constructed DN to bind as a different directory entry. This is a textbook LDAP injection vulnerability.

**Implementation Notes:**
- `[ ]` Implement `escapeLDAPDNComponent(const std::string& value)` in `ldap_authenticator.cpp` following RFC 4514 Section 2.4: escape characters `,`, `+`, `"`, `\`, `<`, `>`, `;`, and leading/trailing spaces and `#`
- `[ ]` Implement `escapeLDAPFilterValue(const std::string& value)` following RFC 4515 Section 3: escape `*`, `(`, `)`, `\`, NUL
- `[ ]` Call `escapeLDAPDNComponent()` on `username` inside `buildUserDN()` before string substitution (line 96)
- `[ ]` Call `escapeLDAPFilterValue()` on all user-controlled values inserted into LDAP search filter strings (lines 257, 379)
- `[ ]` Add `LDAP_OPT_REFERRALS = LDAP_OPT_OFF` to both the Windows path (line 208) and the POSIX path (line 317) — referral chasing with attacker-controlled usernames can redirect authentication to a rogue LDAP server
- `[ ]` Add fuzz test (libFuzzer) targeting `buildUserDN()` with adversarial username inputs

**Performance Targets:**
- Escaping adds < 5 µs overhead per authentication call

---

### Acceptance Criteria

- [ ] Implement `escapeLDAPDNComponent(const std::string& value)` in `ldap_authenticator.cpp` following RFC 4514 Section 2.4: escape characters `,`, `+`, `"`, `\`, `<`, `>`, `;`, and leading/trailing spaces and `#`
- [ ] Implement `escapeLDAPFilterValue(const std::string& value)` following RFC 4515 Section 3: escape `*`, `(`, `)`, `\`, NUL
- [ ] Call `escapeLDAPDNComponent()` on `username` inside `buildUserDN()` before string substitution (line 96)
- [ ] Call `escapeLDAPFilterValue()` on all user-controlled values inserted into LDAP search filter strings (lines 257, 379)
- [ ] Add `LDAP_OPT_REFERRALS = LDAP_OPT_OFF` to both the Windows path (line 208) and the POSIX path (line 317) — referral chasing with attacker-controlled usernames can redirect authentication to a rogue LDAP server
- [ ] Add fuzz test (libFuzzer) targeting `buildUserDN()` with adversarial username inputs
- [ ] Escaping adds < 5 µs overhead per authentication call

### Relationships

- Roadmap row: #2 (🔴 Critical Priority)
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#3-ldap-dn-and-filter-injection-prevention
- Source key: roadmap:2:auth:v1.1.0:3-ldap-dn-and-filter-injection-prevention

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:2:auth:v1.1.0:3-ldap-dn-and-filter-injection-prevention -->
<!-- roadmap-ref: row=2;module=auth;target=v1.1.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#3-ldap-dn-and-filter-injection-prevention -->
