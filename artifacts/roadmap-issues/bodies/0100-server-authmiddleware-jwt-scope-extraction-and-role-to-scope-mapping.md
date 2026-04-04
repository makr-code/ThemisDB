### Context

This issue implements the roadmap item '`AuthMiddleware`: JWT Scope Extraction and Role-to-Scope Mapping' for the server domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `AuthMiddleware`: JWT Scope Extraction and Role-to-Scope Mapping

### Goal

Deliver the scoped changes for `AuthMiddleware`: JWT Scope Extraction and Role-to-Scope Mapping in src/server/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `AuthMiddleware`: JWT Scope Extraction and Role-to-Scope Mapping
**Priority:** High
**Target Version:** v1.8.0

`auth_middleware.cpp` has 2 security-critical TODOs:
- Line 248: "TODO: Enhance with proper scope extraction from JWT claims" — scopes are not currently read from the JWT `scope` or `scp` claim.
- Line 399: "TODO: Check if any of the roles provide the required_scope" — role-to-scope mapping is not implemented; any role passes any scope check.

Both together mean scope-based authorization is effectively not enforced.

**Implementation Notes:**
- `[ ]` At line 248: parse the `scope` (space-separated string) or `scp` (array) claim from the validated JWT payload using `nlohmann::json`; populate `AuthContext::granted_scopes`.
- `[ ]` At line 399: load the role-to-scope mapping from `config/security/rbac_roles.yaml` via `ConfigPathResolver`; check if any of the token's roles grants the `required_scope`; deny if not found.
- `[ ]` Add unit tests: JWT with `scope: "cache:read"` passes cache read endpoint; JWT without `cache:write` scope is rejected at cache write endpoint.

---

### Acceptance Criteria

- [ ] Line 248: "TODO: Enhance with proper scope extraction from JWT claims" — scopes are not currently read from the JWT `scope` or `scp` claim.
- [ ] Line 399: "TODO: Check if any of the roles provide the required_scope" — role-to-scope mapping is not implemented; any role passes any scope check.
- [ ] At line 248: parse the `scope` (space-separated string) or `scp` (array) claim from the validated JWT payload using `nlohmann::json`; populate `AuthContext::granted_scopes`.
- [ ] At line 399: load the role-to-scope mapping from `config/security/rbac_roles.yaml` via `ConfigPathResolver`; check if any of the token's roles grants the `required_scope`; deny if not found.
- [ ] Add unit tests: JWT with `scope: "cache:read"` passes cache read endpoint; JWT without `cache:write` scope is rejected at cache write endpoint.

### Relationships

- Roadmap row: #100 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/server/FUTURE_ENHANCEMENTS.md#authmiddleware-jwt-scope-extraction-and-role-to-scope-mapping
- Source key: roadmap:100:server:v1.8.0:authmiddleware-jwt-scope-extraction-and-role-to-scope-mapping

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:100:server:v1.8.0:authmiddleware-jwt-scope-extraction-and-role-to-scope-mapping -->
<!-- roadmap-ref: row=100;module=server;target=v1.8.0 -->
<!-- roadmap-detail: src/server/FUTURE_ENHANCEMENTS.md#authmiddleware-jwt-scope-extraction-and-role-to-scope-mapping -->
