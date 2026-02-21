# ThemisDB Laufzeit-Lizenzsystem – Administrator- und Kundenleitfaden

**Gilt für**: v1.5.0+  
**Zielgruppe**: Administratoren, DevOps, Endkunden

---

## Übersicht

Ab v1.5.0 ergänzt ThemisDB die bestehenden Compile-Time-Feature-Gates durch eine
**Laufzeit-Lizenzvalidierung**. Das System besteht aus drei Komponenten:

| Komponente | Ort | Zweck |
|-----------|-----|-------|
| **C++ LicenseClient / RuntimeLicenseGate** | `ThemisDB`-Server-Binary | Validiert die Lizenz beim Start und periodisch |
| **WordPress REST API** | Plugin `themisdb-order-request` | Erstellt, validiert und widerruft Lizenzen |
| **Lizenz-Validierungsserver** | `scripts/license-server/` | Eigenständiger FastAPI-Dienst (optional) |

Der Leitfaden zur Compile-Time-Einbettung (`LICENSE_EMBEDDING_GUIDE.md`) gilt weiterhin
für **Offline- / Air-Gapped-Deployments**. Für internetverbundene Produktionsumgebungen
wird das Laufzeitsystem empfohlen.

---

## Schnellstart für Administratoren

### 1 – WordPress-Plugin konfigurieren

1. WordPress Admin → **ThemisDB Orders → Einstellungen** öffnen.
2. Abschnitt **License API Settings** ausfüllen:

   | Einstellung | Beschreibung |
   |------------|--------------|
   | **License API Key** | Gemeinsames Geheimnis, das ThemisDB-Server als `Authorization: Bearer <key>` senden. Mit `openssl rand -hex 32` generieren. |
   | **Admin Secret** | Zusätzlicher Header (`X-ThemisDB-Admin-Secret`) für Verlängerungs-/Widerruf-Endpunkte. |
   | **Renewal Reminder (days)** | Wie viele Tage vor Ablauf eine automatische Erinnerungs-E-Mail gesendet wird (Standard: 30). |

3. **Änderungen speichern** klicken.

### 2 – ThemisDB-Server auf die Lizenz-API zeigen

Diese Werte in der ThemisDB-Konfiguration (YAML oder Umgebungsvariablen) setzen:

```yaml
# themis.yaml
license:
  server_url: "https://ihre-wordpress-seite.de/wp-json/themisdb/v1"
  api_key: "<gleicher Wert wie License API Key oben>"
  allow_offline: true      # Fallback auf eingebettete Lizenz, wenn Server nicht erreichbar
  grace_period_days: 7     # Zusätzliche Tage, bevor der Start nach Offline-Betrieb verweigert wird
```

Als Umgebungsvariablen:

```bash
export THEMIS_LICENSE_SERVER_URL="https://ihre-wordpress-seite.de/wp-json/themisdb/v1"
export THEMIS_LICENSE_API_KEY="<api-key>"
```

### 3 – (Optional) Eigenständigen Lizenzserver deployen

```bash
cd scripts/license-server
pip install -r requirements.txt

export THEMIS_LS_API_KEY="<starker-zufälliger-schlüssel>"
export THEMIS_LS_ADMIN_SECRET="<starkes-admin-geheimnis>"
export THEMIS_LS_DB_PATH="/var/lib/themisdb-ls/licenses.db"

uvicorn app:app --host 0.0.0.0 --port 8765
```

Oder mit Docker:

```bash
docker build -t themisdb-license-server scripts/license-server/

docker run -d \
  -e THEMIS_LS_API_KEY="<key>" \
  -e THEMIS_LS_ADMIN_SECRET="<secret>" \
  -e THEMIS_LS_DB_PATH="/data/licenses.db" \
  -v /var/lib/themisdb-ls:/data \
  -p 8765:8765 \
  themisdb-license-server
```

---

## REST-API-Referenz (WordPress-Plugin)

Basis-URL: `https://ihre-seite.de/wp-json/themisdb/v1`

Alle Endpunkte erfordern:
```
Authorization: Bearer <license-api-key>
```

### POST `/license/validate`

Wird von ThemisDB-Servern aufgerufen, um zu prüfen, ob eine Lizenz aktiv ist.

**Anfrage**
```json
{
  "license_key": "THEMIS-ENT-AABBCCDD-11223344",
  "machine_fingerprint": "<sha256-der-mac-adresse>",
  "edition": "ENTERPRISE"
}
```

**Erfolgsantwort (HTTP 200)**
```json
{
  "valid": true,
  "status": "active",
  "license_key": "THEMIS-ENT-AABBCCDD-11223344",
  "tier": "enterprise",
  "organization": "Beispiel GmbH",
  "limits": { "max_nodes": 100, "max_cores": -1, "max_storage_tb": -1 },
  "start_date": "2026-01-01T00:00:00+00:00",
  "end_date": "2027-01-01T00:00:00+00:00",
  "days_remaining": 344,
  "timestamp": "2026-02-21T09:00:00+00:00",
  "signature": "<hmac-sha256>"
}
```

**Fehlerantwort (HTTP 402)**
```json
{
  "valid": false,
  "status": "expired",
  "error": "Lizenz abgelaufen am 2025-12-31",
  "timestamp": "2026-02-21T09:00:00+00:00",
  "signature": "<hmac-sha256>"
}
```

Mögliche `status`-Werte: `active`, `expired`, `suspended`, `cancelled`, `pending_payment`, `invalid`, `not_found`.

### GET `/license/download/{license_key}`

Gibt die Lizenz-JSON-Datei für die automatisierte Bereitstellung zurück.

```bash
curl -H "Authorization: Bearer <api-key>" \
  "https://ihre-seite.de/wp-json/themisdb/v1/license/download/THEMIS-ENT-AABBCCDD-11223344"
```

### POST `/license/renew` *(nur Admin)*

Verlängert das Ablaufdatum einer Lizenz.

Zusätzlich erforderlicher Header: `X-ThemisDB-Admin-Secret: <admin-geheimnis>`

```json
{
  "license_key": "THEMIS-ENT-AABBCCDD-11223344",
  "extend_days": 365
}
```

### POST `/license/revoke` *(nur Admin)*

Sperrt eine Lizenz sofort.

```json
{
  "license_key": "THEMIS-ENT-AABBCCDD-11223344",
  "reason": "Abonnement gekündigt"
}
```

---

## Antwort-Signaturprüfung (C++-Server)

Jede API-Antwort enthält ein `signature`-Feld: ein HMAC-SHA256-Wert über den sortierten,
JSON-kodierten Antwortkörper, gehasht mit `THEMIS_LS_API_KEY`.

Der C++ `LicenseClient` verifiziert diese Signatur automatisch. Manuelle Überprüfung:

```python
import hmac, hashlib, json

def verify(response_body: dict, api_key: str) -> bool:
    sig = response_body.pop("signature", "")
    canonical = json.dumps(response_body, sort_keys=True, separators=(",", ":"))
    expected  = hmac.new(api_key.encode(), canonical.encode(), hashlib.sha256).hexdigest()
    return hmac.compare_digest(expected, sig)
```

---

## Kunden-Self-Service-Portal

Portal auf einer beliebigen WordPress-Seite einbetten:

```
[themisdb_license_portal]
```

Funktionen für eingeloggte Kunden:

- **Lizenzübersicht** – Liste aller Lizenzen mit Status-Badges und Ablaufdaten
- **Download** – `themis-license.json`-Datei herunterladen
- **Verlängerung** – Link zum Verlängerungs-Kontaktformular
- **Trial starten** – Einmaliger 30-Tage-Community-Trial (ein Trial pro Kunde)

### Heruntergeladene Lizenzdatei installieren

Die heruntergeladene `themis-license.json` an einem dieser Orte ablegen
(ThemisDB prüft sie in dieser Reihenfolge):

1. Pfad per `--license-file /pfad/zu/themis-license.json` CLI-Flag
2. `$THEMIS_HOME/.themis-license`
3. `~/.themisdb/license.json`
4. `/etc/themisdb/license.json`

Nach dem Platzieren der Datei den ThemisDB-Server neu starten. Das Startup-Log bestätigt:

```
[INFO] License gate: runtime validation successful (status: active).
[INFO] ThemisDB License: ENTERPRISE – Beispiel GmbH (expires 2027-01-01, 344 days)
```

---

## Fehlerszenarien & Fehlerbehebung

### Lizenz abgelaufen

**Server-Log:**
```
[ERROR] WARNING: License has expired!
[ERROR] Please contact admin@example.com to renew your license.
```

**Lösung:**
1. Kunden-Portal aufrufen und **Verlängern** klicken.
2. Nach Zahlungsverarbeitung die aktualisierte `themis-license.json` herunterladen.
3. Alte Lizenzdatei ersetzen und Server neu starten.

Bei ENTERPRISE- und HYPERSCALER-Builds verweigert der Server den Start bei
abgelaufener Lizenz. Community-Builds protokollieren nur eine Warnung.

### Ungültige oder manipulierte Lizenz

**Server-Log:**
```
[ERROR] License signature verification FAILED!
[WARN]  License signature is invalid. This may indicate a tampered license.
```

**Lösung:**
- Lizenzdatei erneut aus dem Kunden-Portal herunterladen.
- Sicherstellen, dass die Datei nach dem Download nicht verändert wurde.
- Bei anhaltendem Problem: `lizenzierung@themisdb.com` kontaktieren.

### Server nicht erreichbar (Grace Period)

Wenn der Online-Lizenzserver vorübergehend nicht erreichbar ist, verwendet der
C++-Client die **eingebettete / gecachte Lizenz** für `grace_period_days` (Standard: 7).

**Server-Log:**
```
[WARN] License: running in grace period (5 days remaining). Ensure the license server is reachable.
```

Nach Ablauf der Grace Period verweigern ENTERPRISE/HYPERSCALER-Builds den Start.

### Lizenz nicht gefunden

```json
{ "valid": false, "status": "not_found", "error": "Lizenzschlüssel nicht gefunden." }
```

**Lösung:** Prüfen, ob der `THEMIS_LICENSE_KEY`-Build-Parameter oder das
`license_key`-Feld in der Lizenzdatei mit einem im ThemisDB-Lizenzportal ausgestellten
Schlüssel übereinstimmt.

### API-Schlüssel nicht konfiguriert (WordPress)

```json
{ "code": "rest_forbidden", "message": "License API key not configured." }
```

**Lösung:** WordPress Admin → ThemisDB Orders → Einstellungen → License API Settings →
License API Key eintragen.

---

## Verlängerungs-Erinnerungs-E-Mails

Das Plugin sendet täglich automatische Verlängerungs-E-Mails per WP-Cron für jede aktive
Lizenz, deren Ablaufdatum innerhalb des konfigurierten Fensters liegt.

- Fenster konfigurieren: **Einstellungen → License API Settings → Renewal Reminder (days)**
- E-Mails werden vom Absender aus **Einstellungen → E-Mail Einstellungen** versendet
- Jede Lizenz erhält maximal eine Erinnerungs-E-Mail pro Tag
- Erinnerungen enden, sobald die Lizenz verlängert oder abgelaufen ist

---

## Audit-Log

Jede Lizenzvalidierungs-, Download-, Verlängerungs- und Widerruf-Anfrage wird protokolliert.

**Im WordPress-Admin anzeigen:** ThemisDB Orders → License Audit Log

Die Log-Tabelle (`{prefix}themisdb_license_audit_log`) enthält:

| Spalte | Beschreibung |
|--------|-------------|
| `created_at` | UTC-Zeitstempel der Anfrage |
| `license_key` | Betroffener Lizenzschlüssel |
| `action` | `validate`, `download`, `renew`, `revoke`, `auth_failed` |
| `result` | `success`, `expired`, `not_found`, `invalid`, `db_error` usw. |
| `ip_address` | Client-IP-Adresse |
| `user_agent` | HTTP-User-Agent-Header |

---

## Sicherheitshinweise

1. **API-Schlüssel-Rotation** – `themisdb_license_api_key` und `themisdb_license_admin_secret`
   regelmäßig rotieren. Nach der Rotation `THEMIS_LICENSE_API_KEY` auf allen
   ThemisDB-Instanzen aktualisieren.
2. **Nur HTTPS** – Die WordPress REST API immer über TLS bereitstellen. Der ThemisDB
   C++-Client erzwingt `CURLOPT_SSL_VERIFYPEER=1`.
3. **Antwort-Signatur** – Der C++-Client verifiziert die HMAC-SHA256-Antwortsignatur,
   sodass ein Man-in-the-Middle keine gefälschte „gültig"-Antwort einschleusen kann.
4. **Rate Limiting** – Für die `/wp-json/themisdb/v1/license/*`-Endpunkte einen
   WordPress-Rate-Limiter (z. B. Wordfence) einsetzen, um Brute-Force-Schlüsselprüfungen
   zu verhindern.

---

## Support

| Kanal | Kontakt |
|-------|---------|
| Lizenzierungsfragen | lizenzierung@themisdb.com |
| Enterprise-Support | enterprise@themisdb.com |
| Dokumentation | https://docs.themisdb.org |
| GitHub | https://github.com/makr-code/ThemisDB |
