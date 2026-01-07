# USB Admin Security Feature - Implementation Summary

## Überblick (Overview)

Diese Implementation erfüllt die Anforderung für ein Security-Feature basierend auf einem verschlüsselten USB-Speichermedium, das pro Lizenz die Administration am Hardware-Server erlaubt.

This implementation fulfills the requirement for a security feature based on an encrypted USB storage medium that enables per-license administration on hardware servers.

## Hauptmerkmale (Key Features)

### 1. Hardware-Basierte Authentifizierung
- **USB-Gerät erforderlich**: Administrative Operationen benötigen physisches USB-Gerät
- **Hardware-Bindung**: Lizenz an spezifische Hardware-ID gebunden (machine-id auf Linux)
- **Lizenzdatei**: JSON-Format mit RSA-Signatur zur Manipulation-Verhinderung

### 2. Nahtlose Integration
- **AuthMiddleware**: Integriert mit bestehender Token/JWT-Authentifizierung
- **RBAC-Kompatibel**: Arbeitet zusammen mit Role-Based Access Control
- **Konfigurierbar**: Admin-Scopes via Konfiguration definierbar

### 3. Sicherheit Gegen Manipulation
- **Silent Failure**: Unautorisierte Versuche schlagen still fehl
- **Audit-Logging**: Alle Versuche werden protokolliert
- **Lockout-Mechanismus**: Automatische Sperre nach wiederholten Fehlversuchen
- **Challenge-Response**: Replay-Schutz (Placeholder für Produktion)

### 4. Administrationsebenen-Schutz
- **Scope-Basiert**: Nur konfigurierte Admin-Scopes benötigen USB
- **Granular**: Unterschiedliche Operationen (config:write, admin:backup, etc.)
- **Metriken**: Überwachung aller Validierungsversuche

## Architektur

```
┌─────────────────────────────────────────────┐
│         Admin-Anforderung                    │
│  (Token: admin-123, Scope: config:write)    │
└──────────────────┬──────────────────────────┘
                   │
                   ▼
       ┌───────────────────────┐
       │   AuthMiddleware      │
       │   - Token-Validierung │
       │   - Scope-Check       │
       └───────────┬───────────┘
                   │
                   ▼ (wenn admin scope)
       ┌───────────────────────────┐
       │ USBAdminAuthenticator     │
       │ - USB-Erkennung           │
       │ - Lizenz-Validierung      │
       │ - Hardware-Binding-Check  │
       │ - Signatur-Verifikation   │
       └───────────┬───────────────┘
                   │
                   ▼
       ┌───────────────────────────┐
       │   Verschlüsselter USB     │
       │   /mnt/themis-admin/      │
       │   themis_admin.lic        │
       └───────────────────────────┘
```

## Implementierte Dateien

### Core Components
1. **include/security/usb_admin_authenticator.h**
   - `USBAdminLicense` - Lizenz-Struktur
   - `USBAdminConfig` - Konfiguration
   - `USBAdminAuthenticator` - Hauptklasse
   - ~150 Zeilen

2. **src/security/usb_admin_authenticator.cpp**
   - USB-Mount-Erkennung (Linux/Windows)
   - Lizenz-Laden und Parsing (JSON)
   - Hardware-ID-Erkennung (machine-id)
   - Signatur-Validierung (Placeholder mit Security-Warnungen)
   - Challenge-Response (Placeholder)
   - Audit-Logging
   - ~450 Zeilen

### Integration
3. **include/server/auth_middleware.h** (modifiziert)
   - `enableUSBAdminAuth()` Methode
   - Konfigurierbare Scope-Liste
   - USB-Status-Check

4. **src/server/auth_middleware.cpp** (modifiziert)
   - USB-Validierung bei Admin-Scopes
   - Silent Failure Implementation
   - Audit-Integration

### Tests
5. **tests/test_usb_admin_authenticator.cpp**
   - 14 Test-Cases
   - USB-Erkennung, Lizenz-Validierung, Hardware-Binding
   - Expiry-Check, Scope-Authorization, Lockout
   - Metriken-Tracking
   - ~250 Zeilen

### Dokumentation & Konfiguration
6. **config/usb_admin_license.example.json**
   - Beispiel-Lizenzdatei mit allen Feldern
   
7. **docs/security/usb_admin_feature.md**
   - Kompakte Dokumentation
   - Konfigurationsbeispiele
   - Usage-Anleitung

### Build-System
8. **cmake/CMakeLists.txt** (modifiziert)
   - Source-Dateien hinzugefügt
   - Test-Dateien hinzugefügt
   - Pre-existierende Build-Fehler behoben

## Verwendung (Usage)

### Server-Konfiguration

```cpp
#include "server/auth_middleware.h"

// AuthMiddleware erstellen
AuthMiddleware auth;

// USB-Admin-Authentifizierung aktivieren
auth.enableUSBAdminAuth(
    "/mnt/themis-admin",  // USB Mount-Point
    {                      // Geschützte Scopes (optional)
        "admin",
        "config:write",
        "admin:backup",
        "admin:restore"
    }
);

// Normale Tokens hinzufügen
AuthMiddleware::TokenConfig admin_token{
    .token = "admin-token-123",
    .user_id = "admin",
    .scopes = {"admin", "config:write"}
};
auth.addToken(admin_token);
```

### USB-Vorbereitung

#### 1. USB Verschlüsseln (Linux - LUKS)
```bash
# Verschlüsselte Partition erstellen
sudo cryptsetup luksFormat /dev/sdX1

# Öffnen
sudo cryptsetup open /dev/sdX1 themis-admin

# Dateisystem erstellen
sudo mkfs.ext4 /dev/mapper/themis-admin

# Mounten
sudo mount /dev/mapper/themis-admin /mnt/themis-admin
```

#### 2. Lizenzdatei Erstellen
```bash
# Hardware-ID ermitteln
cat /etc/machine-id

# Lizenzdatei erstellen
cat > /mnt/themis-admin/themis_admin.lic << EOF
{
  "license_key": "THEMIS-ENT-ADMIN-12345678-ABCDEF90",
  "organization": "Example Corporation",
  "hardware_id": "550e8400-e29b-41d4-a716-446655440000",
  "issued_date": "2026-01-01",
  "expiry_date": "2027-12-31",
  "admin_scopes": ["admin", "config:write", "cdc:admin"],
  "signature": "RSA-SIGNATURE-HERE"
}
EOF

# WICHTIG: Signatur mit RSA-Private-Key erstellen!
# (Placeholder akzeptiert derzeit jede nicht-leere Signatur)
```

#### 3. Verwendung
```bash
# USB einsetzen und mounten
sudo cryptsetup open /dev/sdX1 themis-admin
sudo mount /dev/mapper/themis-admin /mnt/themis-admin

# ThemisDB starten
./themis-server

# Admin-Operation durchführen
curl -X POST https://themis:8765/admin/backup \
  -H "Authorization: Bearer admin-token-123"

# Ohne USB -> Silent Failure (403)
```

## Sicherheitshinweise (Security Notes)

### ⚠️ Produktions-TODOs

Die aktuelle Implementation enthält Placeholders für:

1. **RSA-Signatur-Verifikation** (KRITISCH)
   - Aktuell: Akzeptiert jede nicht-leere Signatur
   - TODO: OpenSSL RSA_verify implementieren
   - File: `usb_admin_authenticator.cpp:341-371`

2. **Challenge-Response** (KRITISCH)
   - Aktuell: Kein Replay-Schutz
   - TODO: Kryptographisches Challenge-Response
   - File: `usb_admin_authenticator.cpp:408-431`

3. **Windows Hardware-ID** (WICHTIG)
   - Aktuell: Placeholder-String
   - TODO: Registry MachineGuid auslesen
   - File: `usb_admin_authenticator.cpp:381`

### ✅ Implementierte Sicherheitsmaßnahmen

1. **Hardware-Binding**: Lizenz an machine-id gebunden
2. **Silent Failure**: Keine Information-Disclosure
3. **Audit-Logging**: Vollständige Protokollierung
4. **Lockout-Mechanismus**: Schutz gegen Brute-Force
5. **Metriken**: Überwachung und Alerting
6. **Konfigurierbare Scopes**: Flexible Kontrolle
7. **Fail-Secure Fallback**: Hardware-ID-Fehler führt zu Denial

## Metriken (Metrics)

Das System tracked folgende Metriken:

```cpp
struct Metrics {
    uint64_t admin_ops_allowed;              // Erfolgreiche Admin-Ops
    uint64_t admin_ops_denied_no_usb;        // Verweigert: Kein USB
    uint64_t admin_ops_denied_invalid_license; // Verweigert: Ungültige Lizenz
    uint64_t admin_ops_denied_expired;       // Verweigert: Abgelaufen
    uint64_t admin_ops_denied_lockout;       // Verweigert: Lockout
    uint64_t usb_mount_checks;               // USB-Checks gesamt
    uint64_t usb_mount_detected;             // USB erfolgreich erkannt
    time_point last_valid_check;             // Letzte erfolgreiche Validierung
};
```

## Compliance

Diese Implementation unterstützt folgende Compliance-Anforderungen:

- **BSI C5**: Physische Zugangskontrollen für privilegierte Operationen
- **ISO 27001**: A.9.2.3 Management privilegierter Zugriffsrechte
- **DSGVO/GDPR**: Art. 32 - Sicherheit der Verarbeitung
- **SOC 2**: CC6.1 - Logische und physische Zugriffskontrollen

## Zusammenfassung (Summary)

✅ **Vollständig implementiert:**
- USB-basierte Hardware-Authentifizierung
- Nahtlose Integration in bestehende Sicherheitsarchitektur
- Schutz administrativer Funktionen
- Silent Failure mit Logging
- Umfassende Tests (14 Test-Cases)
- Dokumentation

⚠️ **Für Produktion erforderlich:**
- RSA-Signatur-Verifikation implementieren
- Challenge-Response-Mechanismus implementieren
- Windows Hardware-ID-Erkennung vervollständigen
- Kryptographische Schlüsselverwaltung einrichten
- USB-Lizenz-Signierung-Tool erstellen

🔒 **Sicherheit:**
- Code Review durchgeführt und Findings adressiert
- Explizite Security-Warnungen für Placeholder-Code
- Fail-Secure Design bei Hardware-ID-Erkennung
- Konfigurierbare und erweiterbare Architektur
