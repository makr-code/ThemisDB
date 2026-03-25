<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Document Module (Public Headers)

All notable changes to the Document module public headers are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).  
For implementation-level changes see `../../src/document/CHANGELOG.md`.

## [Unreleased]

## [1.2.0] — 2026-03-24
### Added
- `xdomea_connector.h`: `XDOMEAVersion`, `XDOMEAObjectType`, `XDOMEARetentionCategory`, `XDOMEADocument`, `XDOMEAImportResult`, `XDOMEAExportResult`, `IXDOMEAConnector`, `InMemoryXDOMEAConnector` — thread-safe connector for XDOMEA 2.1/3.0 (KoSIT) document management and records management; supports storeDocument, getDocument, removeDocument, listByType, listByRetention, listChildren, importFromXML (Nachrichtentypen 0201/0202/0203/0401/0501/0601), exportToXML with XML escaping; 30 tests in `tests/test_xdomea_connector.cpp`; CI: `xdomea-connector-ci.yml`

## [1.0.0] — 2026-03-22
### Added
- `encrypted_entities.h`: `SecureDocument` with AES-256-GCM field-level encryption
- `document_manager_deprecated.h`: legacy `DocumentManager` interface (`User`, `Customer` structs) retained for ABI backward compatibility
