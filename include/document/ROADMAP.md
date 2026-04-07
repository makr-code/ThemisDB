<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/document/ROADMAP.md -->

# Roadmap — Document Module (Public Headers)

> Implementation roadmap: `../../src/document/ROADMAP.md`

## Current Status

v1.2.0 — Public headers production-ready. `encrypted_entities.h` is production-ready. `xdomea_connector.h` provides full XDOMEA 2.1/3.0 DMS/RM connector. `document_manager_deprecated.h` is maintained for backward compatibility only.

## Completed ✅

- [x] `SecureDocument` encrypted entity interface (`encrypted_entities.h`)
- [x] Deprecated document manager headers with migration warnings
- [x] `IXDOMEAConnector` / `InMemoryXDOMEAConnector` — XDOMEA 3.0/2.1 document management and records management (`xdomea_connector.h`)

## Planned

- [x] Remove `document_manager_deprecated.h` after migration period (Target: v2.0.0)
- [ ] Expose `IDocumentStore` abstract interface for pluggable backends (Target: v1.3.0)

## Production Readiness Checklist

- [x] Public headers compile cleanly with `-Wall -Wextra`
- [x] Deprecated symbols annotated with `[[deprecated]]`
- [x] `IXDOMEAConnector` header + 30 acceptance-criteria tests
- [ ] `IDocumentStore` abstract interface published
- [ ] Migration guide from deprecated API completed
