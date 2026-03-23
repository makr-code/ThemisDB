<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/document/ROADMAP.md -->

# Roadmap — Document Module (Public Headers)

> Implementation roadmap: `../../src/document/ROADMAP.md`

## Current Status

Public header surface is stable. `encrypted_entities.h` is production-ready. `document_manager_deprecated.h` is maintained for backward compatibility only.

## Completed ✅

- [x] `SecureDocument` encrypted entity interface (`encrypted_entities.h`)
- [x] Deprecated document manager headers with migration warnings

## Planned

- [ ] Remove `document_manager_deprecated.h` after migration period (Target: v2.0.0)
- [ ] Expose `IDocumentStore` abstract interface for pluggable backends (Target: v1.2.0)

## Production Readiness Checklist

- [x] Public headers compile cleanly with `-Wall -Wextra`
- [x] Deprecated symbols annotated with `[[deprecated]]`
- [ ] `IDocumentStore` abstract interface published
- [ ] Migration guide from deprecated API completed
