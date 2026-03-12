### Context

This issue implements the roadmap item 'ConfigSchemaValidator: Extended JSON Schema Keyword Support' for the config domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v2.0.0.

Primary detail section: ConfigSchemaValidator: Extended JSON Schema Keyword Support

### Goal

Deliver the scoped changes for ConfigSchemaValidator: Extended JSON Schema Keyword Support in src/config/ and complete the linked detail section in a release-ready state for v2.0.0.

### Detailed Scope

### ConfigSchemaValidator: Extended JSON Schema Keyword Support
**Priority:** Low
**Target Version:** v2.0.0

`ConfigSchemaValidator` currently implements a Draft 7 subset. Extend it to cover the most commonly needed remaining keywords.

**Implementation Notes:**
- `[x]` Add `allOf` / `anyOf` / `oneOf` — combine multiple sub-schemas; collect errors from all branches for `allOf`.
- `[ ]` Add `not` — assert a value does NOT match a sub-schema.
- `[x]` Add `$ref` with a local `$defs` / `definitions` lookup table to allow reusable schema fragments.
- `[x]` Add `format` keyword (informational only): `date`, `date-time`, `email`, `uri`, `ipv4`, `ipv6`.
- `[x]` Add `uniqueItems` for array validation.
- `[x]` Extend `ConfigSchemaValidator::loadAsJson()` to accept an in-memory YAML string (not only a file path) to support inline config parsing in tests and server-side config hot-checks.
- `[x]` Add `ConfigSchemaValidator::validateFromString(content, is_yaml, schema)` to validate an in-memory YAML or JSON string against a JSON Schema without writing it to disk (Milestone: v2.0.0, Issue: in-memory YAML loading feature).

**Performance Targets:**
- `validate()` for a 100-field JSON config against a 200-rule schema completes in < 5 ms on a single thread.
- Memory allocation per validation call < 1 MB (no large intermediate copies).

**Security / Reliability:**
- `$ref` resolution must be restricted to `$defs`/`definitions` within the same schema document; external URI resolution is explicitly out of scope to prevent SSRF.
- Recursive `$ref` cycles must be detected and reported as a schema error, not trigger infinite recursion.

### Acceptance Criteria

- [ ] Add `allOf` / `anyOf` / `oneOf` — combine multiple sub-schemas; collect errors from all branches for `allOf`.
- [ ] Add `not` — assert a value does NOT match a sub-schema.
- [ ] Add `$ref` with a local `$defs` / `definitions` lookup table to allow reusable schema fragments.
- [ ] Add `format` keyword (informational only): `date`, `date-time`, `email`, `uri`, `ipv4`, `ipv6`.
- [ ] Add `uniqueItems` for array validation.
- [ ] Extend `ConfigSchemaValidator::loadAsJson()` to accept an in-memory YAML string (not only a file path) to support inline config parsing in tests and server-side config hot-checks.
- [ ] Add `ConfigSchemaValidator::validateFromString(content, is_yaml, schema)` to validate an in-memory YAML or JSON string against a JSON Schema without writing it to disk (Milestone: v2.0.0, Issue: in-memory YAML loading feature).
- [ ] `validate()` for a 100-field JSON config against a 200-rule schema completes in < 5 ms on a single thread.
- [ ] Memory allocation per validation call < 1 MB (no large intermediate copies).
- [ ] `$ref` resolution must be restricted to `$defs`/`definitions` within the same schema document; external URI resolution is explicitly out of scope to prevent SSRF.
- [ ] Recursive `$ref` cycles must be detected and reported as a schema error, not trigger infinite recursion.

### Relationships

- Roadmap row: #248 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/config/FUTURE_ENHANCEMENTS.md#configschemavalidator-extended-json-schema-keyword-support
- Source key: roadmap:248:config:v2.0.0:configschemavalidator-extended-json-schema-keyword-support

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:248:config:v2.0.0:configschemavalidator-extended-json-schema-keyword-support -->
<!-- roadmap-ref: row=248;module=config;target=v2.0.0 -->
<!-- roadmap-detail: src/config/FUTURE_ENHANCEMENTS.md#configschemavalidator-extended-json-schema-keyword-support -->
