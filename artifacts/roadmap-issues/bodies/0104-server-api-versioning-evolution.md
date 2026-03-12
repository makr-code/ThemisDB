### Context

This issue implements the roadmap item 'API Versioning & Evolution' for the server domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: API Versioning & Evolution

### Goal

Deliver the scoped changes for API Versioning & Evolution in src/server/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### API Versioning & Evolution

#### Automatic API Versioning
**Priority:** High  
**Target Version:** v1.6.0

Automatic version negotiation and compatibility checks.

**Features:**
- Semantic versioning for APIs (v1.0.0, v1.1.0, v2.0.0)
- Client declares supported version range
- Server responds with best match
- Deprecation warnings for old versions
- Breaking change detection

**Headers:**
```http
Request:
  API-Version: 1.2
  Accept-API-Version: 1.0-2.0

Response:
  API-Version: 1.5
  API-Deprecated: v1.0 (remove 2026-12-31)
```

---

#### API Evolution without Breaking Changes
**Priority:** Medium  
**Target Version:** v1.7.0

Support multiple API versions simultaneously.

**Strategy:**
- Request transformation layer
- Version-specific serializers
- Field renaming and restructuring
- Default values for new fields

**Example:**
```cpp
// v1 API: {"user_id": 123}
// v2 API: {"id": 123, "type": "user"}

ResponseTransformer transformer;
transformer.registerVersion("v1", [](Response res) {
    return {{"user_id": res["id"]}};
});
transformer.registerVersion("v2", [](Response res) {
    return res;  // Native format
});
```

---

### Acceptance Criteria

- [ ] Semantic versioning for APIs (v1.0.0, v1.1.0, v2.0.0)
- [ ] Client declares supported version range
- [ ] Server responds with best match
- [ ] Deprecation warnings for old versions
- [ ] Breaking change detection
- [ ] Request transformation layer
- [ ] Version-specific serializers
- [ ] Field renaming and restructuring
- [ ] Default values for new fields

### Relationships

- Roadmap row: #104 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/server/FUTURE_ENHANCEMENTS.md#api-versioning--evolution
- Source key: roadmap:104:server:v1.6.0:api-versioning-evolution

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:104:server:v1.6.0:api-versioning-evolution -->
<!-- roadmap-ref: row=104;module=server;target=v1.6.0 -->
<!-- roadmap-detail: src/server/FUTURE_ENHANCEMENTS.md#api-versioning--evolution -->
