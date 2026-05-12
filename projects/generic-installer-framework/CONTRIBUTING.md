# CONTRIBUTING

## Workflow

1. Open an issue describing the change.
2. Create a feature branch.
3. Keep changes small and focused.
4. Add/update tests and docs in the same change.
5. Open a pull request with rationale and test evidence.

## Coding Guidelines

- C++20, clear and explicit code
- Prefer RAII and const-correctness
- Avoid hidden side effects
- Keep public interfaces stable and documented

## Security Rules

- Do not add code paths that bypass signature or hash checks
- Treat trust configuration as security-critical input
- Validate all external inputs strictly

## Documentation Rules

- Update architecture/security docs for behavior changes
- Update ROADMAP status when completing roadmap tasks
- Document known limitations and migration impacts

## Testing Expectations

- Unit tests for new logic
- Integration tests for workflow changes
- Include negative tests for failure paths

## Pull Request Checklist

- [ ] Build passes locally
- [ ] Tests added/updated
- [ ] Docs updated
- [ ] Security impact reviewed
- [ ] Backward compatibility evaluated
