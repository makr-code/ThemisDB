# Schlüsselverwaltung (Key Management)

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Security


## 📑 Inhaltsverzeichnis

- [📋 Überblick](#schlüsselverwaltung-key-management)
- [✅ Implementierungsstatus](#-implementierungsstatus-november-2025)
- [🔑 Schlüsselarten](#schlüsselarten-beispiele)
- [🖥️ Server-APIs](#server-apis)
- [🔌 Provider](#provider)
- [🏭 Betrieb](#betrieb)
- [⚙️ Konfiguration](#konfiguration)
- [📚 Siehe auch](#siehe-auch)
- [📝 Changelog](#änderungsverlauf)


ThemisDB unterstützt eine externe Schlüsselverwaltung via KeyProvider-Interface. Für die Entwicklung ist standardmäßig ein MockKeyProvider aktiv; für den Produktionseinsatz stehen **vollständig implementierte** Provider für HashiCorp Vault und HSM (PKCS#11) zur Verfügung.

## ✅ Implementierungsstatus (November 2025)

| Provider | Status | Implementierung | Use Case |
|----------|--------|-----------------|----------|
| **MockKeyProvider** | ✅ Produktionsreif | `src/security/mock_key_provider.cpp` | Development/Testing |
| **VaultKeyProvider** | ✅ Produktionsreif | `src/security/vault_key_provider.cpp` (713 Zeilen) | Enterprise KMS |
| **HSMProvider (PKCS#11)** | ✅ Produktionsreif | `src/security/hsm_provider_pkcs11.cpp` (511 Zeilen) | Hardware-Schlüsselschutz |
| **PKI Client (OpenSSL)** | ✅ Produktionsreif | `src/utils/pki_client.cpp` | RSA-Signaturen |

**Wichtig:** Die VaultKeyProvider- und HSMProvider-Implementierungen sind **keine Stubs oder Mocks**, sondern vollständige, produktionsreife Integrationen mit:
- Echten HTTP/CURL-Aufrufen (Vault KV v1/v2, Transit Engine)
- PKCS#11-Bibliotheksanbindung (dlopen/dlsym)
- Caching, Retry-Logic, Thread-Sicherheit
- Fehlerbehandlung und Audit-Integration

**PKI-Integration (Nov 2025):** Die PKI-Client-Implementierung (`src/utils/pki_client.cpp`) unterstützt echte RSA-Signaturen via OpenSSL (RSA-SHA256). ENV-Variablen (`THEMIS_PKI_PRIVATE_KEY`, `THEMIS_PKI_CERTIFICATE`, `THEMIS_PKI_PRIVATE_KEY_PASSPHRASE`) aktivieren produktive Signaturen; ohne ENV läuft der Server im Fallback-Modus (Development). Details siehe `docs/security/pki_rsa_integration.md`.

## Schlüsselarten (Beispiele)
- LEK: Local Encryption Key
- KEK: Key Encryption Key
- DEK: Data Encryption Key

Hinweis: Die konkrete Nomenklatur hängt von der Deployment‑Strategie ab (siehe encryption_strategy.md).

## Server‑APIs
- GET /keys – Liste verwalteter Schlüssel
- POST /keys/rotate – Schlüsselrotation auslösen
  - Parameter: key_id (im JSON‑Body `{ "key_id": "DEK" }` oder Query `?key_id=DEK`)
  - Antworten: { success, key_id, new_version }

Fehlerfälle:
- 400 Missing key_id – Schlüssel auswählen
- 503 Keys API not available – KeyProvider nicht initialisiert

## Provider

### MockKeyProvider (Development)
- Zweck: Entwicklung und Tests
- Speicherung: In-Memory
- Keine externen Abhängigkeiten
- **Nicht für Produktion geeignet**

### VaultKeyProvider (Enterprise) ✅ Vollständig implementiert
- Vollständige HashiCorp Vault-Integration
- Features:
  - KV v1/v2 Secrets Engine Support
  - Transit Engine für Signaturen (sign/verify)
  - Token-Authentifizierung (AppRole/AWS erweiterbar)
  - Automatisches Key-Caching mit TTL (1h default, konfigurierbar)
  - Thread-sichere Operationen mit Mutex
  - Retry-Logic mit exponential backoff
  - Fehlerbehandlung (403/404/5xx)
- Konfiguration: `VaultKeyProvider::Config` (vault_addr, vault_token, kv_mount_path, kv_version, cache_ttl_seconds, etc.)
- Siehe: `src/security/vault_key_provider.cpp` (713 Zeilen Produktionscode)

### HSMProvider (PKCS#11) ✅ Vollständig implementiert
- Echte PKCS#11-Hardware-Integration
- Unterstützte HSMs:
  - SoftHSM2 (Development/CI)
  - Thales Luna HSM
  - Utimaco CryptoServer
  - AWS CloudHSM
  - YubiHSM 2
- Features:
  - Session-Pooling (konfigurierbar)
  - RSA-SHA256 Signaturen
  - Certificate-Extraktion
  - Graceful Fallback zu Stub bei fehlender Hardware
- Build: `cmake -DTHEMIS_ENABLE_HSM_REAL=ON`
- Siehe: `src/security/hsm_provider_pkcs11.cpp` (511 Zeilen Produktionscode), `docs/security/hsm_integration.md`

## Betrieb

### Produktions-Deployment
- **Empfehlung**: VaultKeyProvider oder HSMProvider für Produktion verwenden
- MockKeyProvider nur für Entwicklung/Tests

### Sicherheit
- Absicherung der Endpunkte (Reverse‑Proxy/Firewall/RBAC): Nur autorisierte Admins dürfen /keys/rotate aufrufen.
- Rotation regelmäßig in der Betriebsroutine einplanen (z. B. DEK monatlich, KEK vierteljährlich).
- Überwachung: Anzahl/Versionen der Schlüssel im Admin‑Tool; Alarme für ablaufende Schlüssel.

## Konfiguration

### VaultKeyProvider
```cpp
VaultKeyProvider::Config config;
config.vault_addr = "https://vault.example.com:8200";
config.vault_token = getenv("VAULT_TOKEN");  // Aus Secrets Manager
config.kv_mount_path = "themis";
config.kv_version = "v2";
config.cache_ttl_seconds = 3600;
config.verify_ssl = true;
```

### HSMProvider (PKCS#11)
```bash
# Environment-Variablen
export THEMIS_HSM_LIBRARY=/usr/lib/softhsm/libsofthsm2.so
export THEMIS_HSM_SLOT=0
export THEMIS_HSM_PIN=1234
export THEMIS_HSM_KEY_LABEL=themis-signing-key
```

Siehe auch:
- `docs/security/hsm_integration.md` - HSM-Setup-Guide
- `docs/security/pki_rsa_integration.md` - PKI-Integration
- Admin‑Guide (Routing‑Hinweise)
- encryption_strategy.md / encryption_deployment.md
