# GitHub Secrets für Plugin-Signierung

Diese Datei dokumentiert die benötigten GitHub Secrets für die automatische Plugin-Signierung in CI/CD.

⚠️ **WICHTIG:** Diese Datei enthält **KEINE** echten Secrets, nur die Dokumentation!

---

## 📋 Erforderliche Secrets

### 1. THEMISDB_PLUGIN_CERT
**Beschreibung:** Plugin Code-Signing Zertifikat (öffentlicher Teil)  
**Format:** PEM-encoded X.509 Certificate

### 2. THEMISDB_PLUGIN_KEY
**Beschreibung:** Private Key für Plugin-Signierung (verschlüsselt!)  
**Format:** PEM-encoded RSA Private Key (AES-256 encrypted)

### 3. THEMISDB_KEY_PASSWORD
**Beschreibung:** Passwort für den verschlüsselten Private Key  
**Format:** String (Plain Text in Secret, verschlüsselt in GitHub)

### 4. THEMISDB_CA_CERT
**Beschreibung:** Root CA Zertifikat für Zertifikatskettenverifizierung  
**Format:** PEM-encoded X.509 Certificate

---

## 🔧 Setup

```bash
# Certificate und Key generieren (siehe manufacturer/LICENSE)

# Als GitHub Secrets hinzufügen
gh secret set THEMISDB_PLUGIN_CERT < cert.pem
gh secret set THEMISDB_PLUGIN_KEY < key-encrypted.pem
gh secret set THEMISDB_KEY_PASSWORD  # Interaktiv eingeben
gh secret set THEMISDB_CA_CERT < ca.pem
```

---

## 🔒 Sicherheits-Best-Practices

### DO ✅
1. Private Keys mit AES-256 verschlüsseln
2. Starke Passwörter (>20 Zeichen) verwenden
3. Secrets regelmäßig rotieren (alle 90 Tage)
4. 2FA für alle Secret-Manager aktivieren

### DON'T ❌
1. Niemals unverschlüsselte Keys als Secret speichern
2. Niemals Secrets in Logs schreiben
3. Niemals Secrets in Artefakte hochladen

---

Siehe vollständige Dokumentation im Repository für Details zu GitHub Actions Integration.

---

**Version:** 1.0.0  
**Zuletzt aktualisiert:** 2026-01-20
