### Context

This issue implements the roadmap item 'Federated Identity Manager: Token Exchange (RFC 8693)' for the auth domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.4.0.

Primary detail section: 16. Federated Identity Manager: Token Exchange (RFC 8693)

### Goal

Deliver the scoped changes for Federated Identity Manager: Token Exchange (RFC 8693) in src/auth/ and complete the linked detail section in a release-ready state for v1.4.0.

### Detailed Scope

### 16. Federated Identity Manager: Token Exchange (RFC 8693)

**Priority:** Low  
**Target Version:** v1.4.0

`federated_identity_manager.cpp:187` returns `false` in the token exchange path. RFC 8693 (OAuth 2.0 Token Exchange) is required for service-to-service impersonation and delegation in federated scenarios.

**Implementation Notes:**
- `[ ]` Implement `exchangeToken(subject_token, subject_token_type, requested_token_type)` in `federated_identity_manager.cpp` calling the IdP's `token_endpoint` with `grant_type=urn:ietf:params:oauth:grant-type:token-exchange`
- `[ ]` Validate the returned token through the existing `JWTValidator` pipeline
- `[ ]` Scope the exchanged token to the minimum required permissions for the target service

---

### Acceptance Criteria

- [ ] Implement `exchangeToken(subject_token, subject_token_type, requested_token_type)` in `federated_identity_manager.cpp` calling the IdP's `token_endpoint` with `grant_type=urn:ietf:params:oauth:grant-type:token-exchange`
- [ ] Validate the returned token through the existing `JWTValidator` pipeline
- [ ] Scope the exchanged token to the minimum required permissions for the target service

### Relationships

- Roadmap row: #242 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#16-federated-identity-manager-token-exchange-rfc-8693
- Source key: roadmap:242:auth:v1.4.0:16-federated-identity-manager-token-exchange-rfc-8693

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:242:auth:v1.4.0:16-federated-identity-manager-token-exchange-rfc-8693 -->
<!-- roadmap-ref: row=242;module=auth;target=v1.4.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#16-federated-identity-manager-token-exchange-rfc-8693 -->
