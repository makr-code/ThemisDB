<!-- Status: current | validated: 2026-03-12 -->
# Changelog — Security Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- Post-quantum cryptography support (Kyber KEM, Dilithium signatures)
- HSM-backed SigningService for hardware-protected key operations
- QueryMaskingPolicy for PII field masking in query results
- Secret manager with vault integration (Vault, AWS Secrets Manager)
- Security evidence collector for compliance reporting
- Certificate rotation automation

## [1.0.0] — 2024-01-01
### Added
- AES-256-GCM field-level encryption
- PKI certificate management (X.509, GPG)
- RBAC policy enforcement
