# Test-Zertifikate

⚠️ **NUR FÜR DEVELOPMENT UND TESTING!**

Dieses Verzeichnis enthält Test-Zertifikate für lokale Entwicklung.

**NIEMALS in Production verwenden!**

---

## Verfügbare Test-Zertifikate

- `test-ca.crt` - Test Root CA
- `test-plugin-signer.crt` - Test Signing Certificate

---

## Generierung

```bash
# Test CA generieren
openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout test-ca.key -out test-ca.crt \
  -subj "/CN=Test CA/O=ThemisDB Test/C=DE" \
  -days 365

# Test Signing Cert generieren
openssl req -newkey rsa:2048 -nodes \
  -keyout test-plugin-signer.key \
  -out test-plugin-signer.csr \
  -subj "/CN=Test Plugin Signer/O=ThemisDB Test/C=DE"

openssl x509 -req -in test-plugin-signer.csr \
  -CA test-ca.crt -CAkey test-ca.key \
  -out test-plugin-signer.crt -days 365 -sha256
```

---

**Version:** 1.0.0  
**Zuletzt aktualisiert:** 2026-01-20
