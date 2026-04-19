# include/stable_diffusion

## Scope
- API evolution for richer safety policy and generation controls.

### Design Constraints
- Maintain compatibility with `IImageGenerationBackend` semantics.

### Required Interfaces
- `SDPlugin`
- `ISDGenerator`
- `SDPromptSanitizer`
- `SDConfig`

### Implementation Notes
- Prefer additive config fields and explicit defaults.

### Test Strategy
- Contract tests for config parsing and prompt-sanitizer edge cases.

### Performance Targets
- Keep API-layer overhead negligible versus generator inference cost.

### Security / Reliability
- Enforce prompt sanitization and provenance metadata consistency.