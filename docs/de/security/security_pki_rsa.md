# PKI RSA-Integration

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Security


## 📑 Inhaltsverzeichnis

- [Anforderungen](#anforderungen)
- [API](#api-bleibt-stabil)
- [Konfiguration](#konfiguration-env-mvp)
- [Implementierungs-Skizze](#implementierungs-skizze)
- [Sicherheitsaspekte](#sicherheitsaspekte)
- [Status](#status)

---


Stand:  6. April 2026

Der Demo-Stub in `src/utils/pki_client.cpp` wurde durch echte RSA-Signaturen (OpenSSL) ersetzt, mit automatischem Fallback zum Stub wenn keine Schlüssel konfiguriert sind.

## Anforderungen
- RSA Sign/Verify mit OpenSSL (ohne Remote-PKI zunächst)
- Lade Private Key (PEM, optional passphrase) aus Dateipfad oder Secret-Provider
- Verwende X.509 Zertifikat (PEM) für Metadaten (cert_serial) und Verifikation
- Streaming-fähig: Eingabe ist bereits Hash (SHA-256) der Nutzdaten
- Thread-sicher (Client instanziiert pro Worker oder geschützt über Mutex)

## API (bleibt stabil)
- VCCPKIClient::signHash(hash_bytes) → SignatureResult
- VCCPKIClient::verifyHash(hash_bytes, sig) → bool

## Konfiguration (ENV, MVP)
- THEMIS_PKI_PRIVATE_KEY: Pfad zur PEM-Datei mit privatem Schlüssel
- THEMIS_PKI_PRIVATE_KEY_PASSPHRASE: Passphrase (falls gesetzt)
- THEMIS_PKI_CERTIFICATE: Pfad zur X.509 PEM-Zertifikatsdatei

Später (optional):
- Remote-PKI-Endpoint (REST/HSM), OAuth2/mTLS, KeyID statt Datei

## Implementierungs-Skizze

```cpp
// signHash()
// 1) Lade EVP_PKEY aus PEM (+Passphrase)
// 2) Erzeuge EVP_PKEY_CTX + RSA-PSS (oder PKCS#1 v1.5, je nach Vorgabe)
// 3) Setze Hash-Algorithmus (SHA-256)
// 4) Signiere hash_bytes (EVP_DigestSign* für v1.5; PSS erfordert directe Signatur über Hash via EVP_PKEY_sign)
// 5) Base64-Encode der Signatur
// 6) Lese Zertifikat, extrahiere Serial

// verifyHash()
// 1) Lade Zertifikat (oder Public Key)
// 2) Base64-Decode Signatur
// 3) Verifikation gegen hash_bytes
```

## Sicherheitsaspekte
- Key-Material nicht im Speicher persistieren (Zeroization wo möglich)
- Kein Logging von Passphrasen/Keys
- Integrierbar mit HSM (PKCS#11) in Folgeschritt

## Status

### ✅ Implementiert (v0.1.0, Nov 2025)
- OpenSSL RSA Sign/Verify in `src/utils/pki_client.cpp`
- PEM Private Key laden (mit optionaler Passphrase via `password_cb`)
- X.509 Zertifikat laden für Public Key + Serial Number
- ENV-Konfiguration: `THEMIS_PKI_PRIVATE_KEY`, `THEMIS_PKI_CERTIFICATE`, `THEMIS_PKI_PRIVATE_KEY_PASSPHRASE`
- **Automatischer Fallback**: Wenn ENV-Variablen nicht gesetzt → Stub-Modus (Base64, Development)
- Thread-sicher durch immutable Config nach Konstruktion

### Verwendung (Produktion)

```bash
# 1. Generiere Test-Keys (oder nutze existierende PKI)
openssl genrsa -aes256 -out private_key.pem 2048
openssl req -new -x509 -key private_key.pem -out certificate.pem -days 365

# 2. Setze ENV-Variablen
export THEMIS_PKI_PRIVATE_KEY=/secure/path/private_key.pem
export THEMIS_PKI_CERTIFICATE=/secure/path/certificate.pem
export THEMIS_PKI_PRIVATE_KEY_PASSPHRASE=your_passphrase

# 3. Starte Server → PKI nutzt echte RSA-Signaturen
./themis_server
```

**Ohne ENV-Variablen** (Development): Server läuft im Stub-Modus (Base64-only, nicht eIDAS-konform).

### OpenSSL API Hinweise
- Verwendet Low-Level `RSA_sign()`/`RSA_verify()` (deprecated in OpenSSL 3.x, funktioniert aber)
- Migration zu `EVP_DigestSign*()` geplant für langfristige Wartbarkeit
- Unterstützt SHA-256, SHA-384, SHA-512 (NID-basierte Auswahl)

## Nächste Schritte
1. ⏳ Unit-Tests: Sign→Verify Happy Path; Verify mit falschem Key → Fail; korrupte Signatur
2. ⏳ Interop-Test: OpenSSL CLI verifiziert Themis-erzeugte Signaturen
3. ✅ [compliance.md](../compliance.md) eIDAS-Status aktualisiert (✅ Produktiv mit ENV, ⚙️ Stub ohne ENV)
4. 🔮 HSM-Integration (PKCS#11) für Hardware-Key-Schutz (optional, Enterprise)
