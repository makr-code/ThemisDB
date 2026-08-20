# ThemisDB.org Herstellerlizenz und Zertifikate

Dieses Verzeichnis enthält die **öffentlichen** Zertifikate und Lizenzen für die offizielle ThemisDB.org Plugin-Signierung.

⚠️ **WICHTIG:** Private Keys gehören **NICHT** in dieses Repository!

> ⚠️ **[PRIVATE] Hyperscaler Governance:** Signatur-Erzeugung für Assets/Plugins
> ist owner-kontrolliert und liegt in privaten Repositories unter
> `plugins/private/*`. Dieses Verzeichnis enthält nur öffentliche Trust-Artefakte.

---

## 📁 Verzeichnisstruktur

```
certs/
├── README.md                          # Diese Datei
├── GITHUB_SECRETS.md                  # GitHub Secrets Dokumentation
├── manufacturer/                      # Herstellerzertifikate
│   ├── themisdb-ca.crt               # ThemisDB Root CA (öffentlich)
│   ├── themisdb-plugin-signer.crt    # Plugin Code-Signing Cert (öffentlich)
│   ├── LICENSE                        # Herstellerlizenz-Text
│   └── certificate-chain.pem         # Komplette Zertifikatskette
├── intermediate/                      # Intermediate CAs (optional)
│   └── themisdb-intermediate-ca.crt
├── crl/                              # Certificate Revocation Lists
│   └── themisdb-plugins.crl
├── test/                             # Test-Zertifikate (NUR für Development!)
│   ├── test-ca.crt
│   └── test-plugin-signer.crt
├── community/license_test.json       # TEST-ONLY License Fixture (not for production)
├── enterprise/license_test.json      # TEST-ONLY License Fixture (not for production)
├── hyperscaler/license_test.json     # TEST-ONLY License Fixture (not for production)
├── military/license_test.json        # TEST-ONLY License Fixture (not for production)
├── minimal/license_test.json         # TEST-ONLY License Fixture (not for production)
└── ...
```

> ⚠️ Diese License-JSON-Dateien sind explizit `development_and_ci_only`-Fixtures. Sie dienen nur für lokale Builds, CI-Tests und Demos und dürfen niemals für reale Deployments, Produkt-Signierung oder produktive Umgebungen verwendet werden.

> Jeder Fixture-Import sollte `test_only: true` und `do_not_use_in_production: true` liefern; wenn ein Build ein echtes Produktions-Lizenz-Set verwendet, muss dies separat und nicht über die Test-Fixtures erfolgen.

---

## 🔐 Herstellerlizenz

### ThemisDB.org Plugin Manufacturing License

**Version:** 1.0.0  
**Gültig ab:** 2026-01-20  
**Gültig bis:** 2029-01-20  
**Lizenzinhaber:** ThemisDB.org  
**Aussteller:** ThemisDB.org Certificate Authority

Diese Lizenz autorisiert ThemisDB.org zur Signierung offizieller ThemisDB-Plugins mit dem Code-Signing-Zertifikat:

- **Subject:** `CN=ThemisDB Plugin Signer, O=ThemisDB.org, C=DE`
- **Issuer:** `CN=ThemisDB Official Plugins CA, O=ThemisDB.org, C=DE`
- **Serial Number:** `01`
- **Fingerprint (SHA-256):** `[Wird nach Generierung eingefügt]`

---

## 📜 Zertifikatshierarchie

```
ThemisDB Root CA (Self-Signed)
└── ThemisDB Plugin Signer (Code-Signing)
    └── [Signierte Plugins]
```

**Trust Anchor:** `themisdb-ca.crt` (muss in ThemisDB eingebettet sein)

---

## 🔑 GitHub Secrets (Zukünftig)

Für automatisierte Signierung in GitHub Actions werden folgende Secrets benötigt:

| Secret Name | Beschreibung | Typ |
|------------|--------------|-----|
| `THEMISDB_PLUGIN_CERT` | Plugin Code-Signing Zertifikat (PEM) | Certificate |
| `THEMISDB_PLUGIN_KEY` | Private Key (verschlüsselt!) | Private Key |
| `THEMISDB_KEY_PASSWORD` | Passwort für Private Key | Password |
| `THEMISDB_CA_CERT` | Root CA Zertifikat | Certificate |

Siehe [GITHUB_SECRETS.md](GITHUB_SECRETS.md) für Details.

---

## 🧾 CRL/Revocation-Strategie (Hyperscaler)

- Für Hyperscaler-Signing wird eine **offline CRL** in der privaten Signing-Infrastruktur geführt.
- Öffentliche Community/Minimal-Artefakte erhalten **keine** privaten CRL-Daten oder Credentials.
- Bei Schlüsselkompromittierung muss die private CRL aktualisiert und ein Re-Signing aller betroffenen Artefakte erzwungen werden.

---

## 🛡️ Sicherheitsrichtlinien

### DO ✅

1. **Öffentliche Zertifikate** in diesem Verzeichnis speichern
2. **Fingerprints** aller Zertifikate dokumentieren
3. **CRL (Certificate Revocation List)** regelmäßig aktualisieren
4. **Test-Zertifikate** deutlich markieren
5. **Zertifikatsablauf** überwachen

### DON'T ❌

1. **Private Keys** NIEMALS committen!
2. **Passwörter** NIEMALS im Repository speichern!
3. **HSM-Credentials** NIEMALS exponieren!
4. **Production-Keys** in Test-Code verwenden!
5. **Abgelaufene Zertifikate** verwenden!

---

## 🔧 Verwendung

### Plugin-Signierung (Build-Zeit)

```cpp
// In CMakeLists.txt
sign_plugin(themis_plugin_onnx_clip)
```

### Plugin-Verifikation (Laufzeit)

```cpp
#include "acceleration/plugin_security.h"

// Embedded CA wird automatisch verwendet
PluginSecurityVerifier verifier(policy);
std::string error;
if (verifier.verifyPlugin("./plugins/plugin.so", error)) {
    // Plugin ist von ThemisDB.org signiert
}
```

### Manuelle Verifikation

```bash
# Zertifikat-Info anzeigen
openssl x509 -in manufacturer/themisdb-plugin-signer.crt -text -noout

# Signatur verifizieren
openssl cms -verify \
  -in plugin.so.p7s \
  -content plugin.so \
  -CAfile manufacturer/themisdb-ca.crt \
  -inform DER
```

---

## 📋 Zertifikatsverwaltung

### Erneuerung

Zertifikate müssen vor Ablauf erneuert werden:

1. **6 Monate vor Ablauf:** Warning-Notice
2. **3 Monate vor Ablauf:** Erneuerung starten
3. **1 Monat vor Ablauf:** Neue Zertifikate deployen
4. **Nach Ablauf:** Alte Zertifikate in CRL eintragen

### Widerruf

Bei Kompromittierung eines Zertifikats:

1. Zertifikat-Serial-Number in CRL eintragen
2. CRL aktualisieren und publizieren
3. Alle betroffenen Plugins neu signieren
4. Security-Advisory veröffentlichen

---

## 🔍 Audit-Trail

Alle Signierungsvorgänge werden protokolliert:

```
[2026-01-20 10:30:00] Signed: themis_plugin_onnx_clip.so
  - Certificate: CN=ThemisDB Plugin Signer
  - Serial: 01
  - Hash (SHA-256): a3f2c1b8d9e4f5a6b7c8d9e0f1a2b3c4...
  - Signature Algorithm: RSA-4096 + SHA-256
  - Timestamp: RFC 3161 (timestamp.digicert.com)
```

---

## 📞 Kontakt

**Security Issues:** security@themisdb.org  
**Certificate Requests:** certificates@themisdb.org  
**General:** info@themisdb.org

---

## 📚 Standards & Compliance

- **X.509 v3** - Public Key Certificates
- **RFC 5280** - Internet X.509 PKI Certificate and CRL Profile
- **RFC 3161** - Time-Stamp Protocol (TSP)
- **RFC 5652** - Cryptographic Message Syntax (CMS)
- **FIPS 140-2** - Cryptographic Module Security Requirements

---

## ⚖️ Lizenz

Die Zertifikate in diesem Verzeichnis sind urheberrechtlich geschützt:

```
Copyright (c) 2026 ThemisDB.org

Diese Zertifikate dürfen nur zur Verifikation offizieller ThemisDB-Plugins
verwendet werden. Jegliche andere Verwendung ist untersagt.
```

---

**Zuletzt aktualisiert:** 2026-01-20  
**Version:** 1.0.0
