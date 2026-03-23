<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Document Module (Public Headers)

All notable changes to the Document module public headers are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).  
For implementation-level changes see `../../src/document/CHANGELOG.md`.

## [Unreleased]

## [1.0.0] — 2026-03-22
### Added
- `encrypted_entities.h`: `SecureDocument` with AES-256-GCM field-level encryption
- `document_manager_deprecated.h`: legacy `DocumentManager` interface (`User`, `Customer` structs) retained for ABI backward compatibility
