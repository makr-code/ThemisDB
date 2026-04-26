# Sicherheit & Governance – Überblick

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Security


## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✅ Implementierungsstatus](#-implementierungsstatus-dezember-2025)
- [🧩 Kernbereiche](#kernbereiche)
- [📚 Weiterlesen](#weiterlesen)

---


Dieser Überblick fasst die sicherheitsrelevanten Bausteine von ThemisDB zusammen und verlinkt die Detailseiten.

## ✅ Implementierungsstatus (Dezember 2025)

| Komponente | Status | Implementierung |
|------------|--------|-----------------|
| **RBAC/ABAC Policy Engine** | ✅ Produktionsreif | Ranger-kompatibel |
| **Apache Ranger Integration** | ✅ Produktionsreif | `src/server/ranger_adapter.cpp` |
| **VaultKeyProvider (KMS)** | ✅ Produktionsreif | `src/security/vault_key_provider.cpp` |
| **HSMProvider (PKCS#11)** | ✅ Produktionsreif | `src/security/hsm_provider_pkcs11.cpp` |
| **PKI Client (OpenSSL)** | ✅ Produktionsreif | `src/utils/pki_client.cpp` |
| **Audit Logging** | ✅ Produktionsreif | Hash-Chain, PKI-Signaturen |
| **Field Encryption** | ✅ Produktionsreif | AES-256-GCM |
| **Timestamp Authority** | ✅ Produktionsreif | RFC 3161 via OpenSSL |

## Kernbereiche

- Schlüsselverwaltung (Key Management): Schlüsselarten, Rotation, Server-APIs, Provider
- Verschlüsselung: Strategie, Deployment, Spaltenverschlüsselung
- PII-Erkennung & Klassifizierung: Regeln, Engine, Admin-APIs
- Audit & Retention: Changefeed (Audit-Trail), Statistiken, Aufbewahrung
- Threat Model (light): Assets, Akteure, Vertrauensgrenzen, Risiken, Gegenmaßnahmen

## Weiterlesen

- Schlüsselverwaltung: `security/key_management.md`
- Apache Ranger: `security/policies.md`
- HSM-Integration: `security/hsm_integration.md`
- PKI-Integration: `security/pki_rsa_integration.md`
- PII-Detection: `security/pii_detection.md`
- Audit & Retention: `security/audit_and_retention.md`
- Threat-Model: `security/threat_model.md`
- Verschlüsselung: `encryption_strategy.md`, `encryption_deployment.md`, `column_encryption.md`
