<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Document Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Document module public headers expose the encrypted entity interface and the deprecated document manager. Security concerns focus on correct use of `SecureDocument` and preventing accidental use of unencrypted legacy paths.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Plaintext document storage | `SecureDocument` enforces AES-256-GCM encryption; legacy path annotated `[[deprecated]]` |
| Incorrect key management | Key derivation delegated to `include/security/`; not handled in document headers |
| ABI confusion via deprecated API | `document_manager_deprecated.h` marked deprecated; compile-time warnings emitted |

## Security Controls

- All new document entities use `SecureDocument` with field-level encryption.
- Deprecated headers emit compile-time warnings to prevent inadvertent use.

## Known Limitations

- Key rotation for existing `SecureDocument` instances requires re-encryption at the `src/document/` layer.
- Implementation-level security details: `../../src/document/SECURITY.md`.
