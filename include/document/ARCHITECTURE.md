<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/document/ -->

# Document Module — Public Header Architecture
**Version:** 1.0  
**Module Path:** `include/document/`  
**Implementation:** `../../src/document/`

---

## Overview

The Document module exposes public headers for encrypted entity storage and the legacy document manager interface. It provides typed wrappers for document-level encryption and the deprecated `DocumentManager` API maintained for backward compatibility.

## Design Principles

- **Encryption First** — `encrypted_entities.h` wraps all document CRUD operations with field-level encryption.
- **Deprecation Safety** — `document_manager_deprecated.h` preserves ABI stability while signalling migration path to `src/document/`.
- **Minimal Public Surface** — only headers required by external consumers are exposed; implementation details remain in `src/document/`.

## Interface Inventory

| Header | Classes / Structs | Purpose |
|--------|-------------------|---------|
| `encrypted_entities.h` | `SecureDocument` | Encrypted document wrapper with AES-256-GCM field-level encryption |
| `document_manager_deprecated.h` | `User`, `Customer` | Deprecated compatibility layer; use `include/projects/DocumentManager/document_manager.h` instead |

## References

- Implementation details: `../../src/document/`
- Encryption primitives: `include/security/`
- Migration guide: `../../src/document/README.md`
