# ThemisDB Best Practices Guide

> Bewährte Muster und Empfehlungen für den produktiven Einsatz von ThemisDB — Performance, Sicherheit, Betrieb und Datenmodellierung.

---

## Inhaltsverzeichnis

1. [Query-Optimierung](#1-query-optimierung)
2. [Index-Best-Practices](#2-index-best-practices)
3. [Transaktionen](#3-transaktionen)
4. [Sicherheit](#4-sicherheit)
5. [Deployment & Operations](#5-deployment--operations)
6. [Fehlerbehandlung](#6-fehlerbehandlung)
7. [Datenmodellierung](#7-datenmodellierung)
8. [Monitoring](#8-monitoring)

---

## 1. Query-Optimierung

### ✅ Bind-Variablen immer verwenden

```aql
-- ✅ GUT: Parameterisiert, Query-Plan wird gecacht
FOR u IN users
  FILTER u.age > @min_age AND u.city == @city
  RETURN u

-- ❌ SCHLECHT: Literale, kein Plan-Caching, Injection-Risiko
FOR u IN users
  FILTER u.age > 18 AND u.city == "Berlin"
  RETURN u
```

### ✅ LIMIT so früh wie möglich

```aql
-- ✅ GUT: Sortierung und Limit auf wenige Dokumente beschränkt
FOR u IN users
  FILTER u.active == true
  SORT u.score DESC
  LIMIT 10          -- erst limitieren
  LET orders = (FOR o IN orders FILTER o.user_id == u._key RETURN o)
  RETURN { user: u, orders }

-- ❌ SCHLECHT: Joins auf allen Dokumenten, dann erst limitieren
FOR u IN users
  FOR o IN orders
    FILTER o.user_id == u._key
    SORT u.score DESC
    LIMIT 10
    RETURN { u, o }
```

### ✅ Projektion in RETURN

```aql
-- ✅ GUT: Nur benötigte Felder zurückgeben
FOR u IN users
  RETURN { id: u._key, name: u.name, email: u.email }

-- ❌ SCHLECHT: Gesamtes Dokument übertragen (bei großen Dokumenten kostspielig)
FOR u IN users
  RETURN u
```

### ✅ Subquery-Optimierung

```aql
-- ✅ Subquery mit LIMIT begrenzen
LET recent = (
  FOR o IN orders
    FILTER o.user_id == user._key
    SORT o.created DESC
    LIMIT 5           -- Subquery begrenzen!
    RETURN o
)

-- ❌ Unbegrenzte Subquery über große Collections
LET all_orders = (
  FOR o IN orders
    FILTER o.user_id == user._key
    RETURN o           -- Kann riesig werden
)
```

---

## 2. Index-Best-Practices

### Index-Auswahl

| Abfrage-Typ | Index-Typ |
|---|---|
| Exakte Gleichheit (`==`) | Hash-Index |
| Bereichsabfragen (`>`, `<`, `BETWEEN`) | Skiplist-Index |
| Eindeutige Werte (E-Mail, SKU) | Hash UNIQUE |
| Sortierung + Bereich | Skiplist |
| Volltextsuche | Fulltext (BM25) |
| Geo-Abfragen | Geo-Index |
| Vektorähnlichkeit | HNSW-Vektorindex |
| Multi-Feld-Queries | Composite Index |

### Composite Indexes — Reihenfolge beachten

```aql
-- Abfrage: FILTER status == "active" AND created >= "2025-01-01"
-- Index auf (status, created) ist optimal — Diskriminantestes Feld zuerst!
CREATE INDEX idx_users_status_created
  ON users(status, created)
  TYPE SKIPLIST

-- Dieser Index kann NICHT für "FILTER created >= ..." ohne status genutzt werden
-- → Separaten Index für created anlegen wenn benötigt
```

### Anti-Pattern: Over-Indexing

```
Problem: Zu viele Indizes verlangsamen Schreiboperationen.
Jeder Index muss bei INSERT/UPDATE/DELETE mitgepflegt werden.

Faustregel:
- Maximal 5–7 Indizes pro Collection
- Indizes regelmäßig auf Nutzung prüfen
- Ungenutzte Indizes entfernen
```

---

## 3. Transaktionen

### ✅ Explizite Transaktionen für Konsistenz

```aql
BEGIN TRANSACTION

  -- Validierung
  LET balance = DOCUMENT("accounts", @from_account).balance
  LET amount  = @amount

  // Strikter Vergleich — kein implizites Null-Handling
  FILTER balance != null AND balance >= amount

  -- Schreiboperationen
  UPDATE @from_account WITH { balance: balance - amount } IN accounts
  UPDATE @to_account   WITH { balance: DOCUMENT("accounts", @to_account).balance + amount } IN accounts

  -- Audit-Log
  INSERT {
    from:      @from_account,
    to:        @to_account,
    amount:    amount,
    timestamp: DATE_NOW()
  } INTO transfer_log

COMMIT
```

### ✅ Deadlock-Vermeidung

```python
# Locks immer in konsistenter Reihenfolge erwerben
# Ressourcen alphabetisch/numerisch ordnen verhindert Circular Waits
def transfer(from_id, to_id, amount):
    # Immer die kleinere ID zuerst sperren
    first, second = sorted([from_id, to_id])
    with lock(first), lock(second):
        execute_transfer(from_id, to_id, amount)
```

### ✅ Retry-Logik bei Konflikten

```python
import time

def execute_with_retry(client, query, bind_vars=None, max_retries=3):
    for attempt in range(max_retries):
        try:
            return client.query(query, bind_vars=bind_vars)
        except ThemisConflictError:
            if attempt == max_retries - 1:
                raise
            time.sleep(0.05 * (2 ** attempt))  # Exponential backoff: 50ms, 100ms, 200ms
```

---

## 4. Sicherheit

### ✅ Authentifizierung in Produktion aktivieren

```json
{
  "auth": {
    "enabled":      true,
    "jwt_secret":   "min_32_zeichen_starkes_geheimnis_hier",
    "token_expiry": "8h",
    "refresh":      true
  }
}
```

### ✅ Minimale Berechtigungen (Principle of Least Privilege)

```aql
-- Nur notwendige Berechtigungen vergeben
CREATE USER api_service PASSWORD @secure_password

-- Nur lesen auf benötigte Collections
GRANT READ ON COLLECTION products   TO api_service
GRANT READ ON COLLECTION categories TO api_service

-- Schreiben nur auf notwendige Collections
GRANT READ, WRITE ON COLLECTION orders TO api_service
```

### ✅ TLS 1.3 in Produktion

```json
{
  "server": {
    "tls": {
      "enabled":      true,
      "cert_path":    "/etc/ssl/themisdb/cert.pem",
      "key_path":     "/etc/ssl/themisdb/key.pem",
      "min_version":  "TLS1.3",
      "ciphers":      ["TLS_AES_256_GCM_SHA384", "TLS_CHACHA20_POLY1305_SHA256"]
    }
  }
}
```

### ✅ Datenverschlüsselung at Rest

```json
{
  "storage": {
    "encryption": {
      "enabled":   true,
      "algorithm": "AES-256-GCM",
      "key_source": "vault"
    }
  }
}
```

### ✅ Input-Validierung — Bind-Variablen verhindern Injection

```python
# ✅ SICHER: Bind-Variablen
result = client.query(
    "FOR u IN users FILTER u.email == @email RETURN u",
    bind_vars={"email": user_input}  # Automatisch escaped
)

# ❌ UNSICHER: String-Konkatenation
query = f"FOR u IN users FILTER u.email == '{user_input}' RETURN u"  # AQL-Injection möglich!
```

---

## 5. Deployment & Operations

### ✅ Ressourcenlimits setzen

```yaml
# docker-compose.yml
services:
  themisdb:
    image: themisdb/themisdb:latest
    deploy:
      resources:
        limits:
          cpus: '4.0'
          memory: 8G
        reservations:
          cpus: '2.0'
          memory: 4G
    environment:
      THEMIS_MAX_MEMORY: 6144   # MB — unter docker limit
      THEMIS_MAX_THREADS: 8
```

### ✅ Regelmäßige Backups

```bash
# Tägliches Backup via Cron
0 2 * * * curl -X POST http://localhost:8080/api/v1/backup \
  -d "{\"path\": \"/backups/themisdb-$(date +%Y%m%d)\"}"

# Backup-Integrität prüfen
curl http://localhost:8080/api/v1/backup/verify \
  -d '{"path": "/backups/themisdb-20250101"}'

# Alte Backups aufräumen (älter als 30 Tage)
find /backups -name "themisdb-*" -mtime +30 -delete
```

### ✅ Graceful Shutdown

```bash
# SIGTERM für graceful shutdown senden (kein SIGKILL!)
docker stop --time=30 themisdb  # 30 Sekunden Wartezeit

# In Kubernetes
spec:
  terminationGracePeriodSeconds: 60
```

### ✅ Umgebungsspezifische Konfiguration

```
Entwicklung:  auth.enabled=false, log.level=debug
Staging:      auth.enabled=true, TLS self-signed, log.level=info
Produktion:   auth.enabled=true, TLS CA-signiert, log.level=warn, encryption=true
```

---

## 6. Fehlerbehandlung

### ✅ Fehler-Codes kennen

| Code | Bedeutung | Reaktion |
|---|---|---|
| `1200` | Conflict / Optimistic Lock | Retry mit Backoff |
| `1202` | Document not found | Anwendungslogik |
| `1203` | Collection not found | Initialisierung prüfen |
| `1210` | Unique constraint violated | Duplikat behandeln |
| `1501` | Transaction aborted | Retry |
| `403` | Forbidden | Berechtigungen prüfen |
| `503` | Service unavailable | Circuit Breaker |

### ✅ Circuit Breaker Pattern

```python
from themis_client import ThemisClient
import time

class ResilientThemisClient:
    def __init__(self, base_url, failure_threshold=5, recovery_time=60):
        self._client = ThemisClient(base_url)
        self._failures = 0
        self._threshold = failure_threshold
        self._recovery_time = recovery_time
        self._last_failure = 0
        self._state = "CLOSED"  # CLOSED, OPEN, HALF_OPEN

    def query(self, aql, **kwargs):
        if self._state == "OPEN":
            if time.time() - self._last_failure > self._recovery_time:
                self._state = "HALF_OPEN"
            else:
                raise RuntimeError("Circuit breaker OPEN — ThemisDB nicht erreichbar")
        try:
            result = self._client.query(aql, **kwargs)
            self._failures = 0
            self._state = "CLOSED"
            return result
        except Exception as e:
            self._failures += 1
            self._last_failure = time.time()
            if self._failures >= self._threshold:
                self._state = "OPEN"
            raise
```

---

## 7. Datenmodellierung

### ✅ Konsistente Schlüssel-Konventionen

```
_key-Format: {entity_type}_{uuid4_ohne_bindestrich}
Beispiele:
  users:    usr_550e8400e29b41d4a716446655440000
  orders:   ord_6ba7b810-9dad-11d1-80b4-00c04fd430c8
  products: prd_6ba7b811-9dad-11d1-80b4-00c04fd430c8

Alternativ: Natürlicher Schlüssel wenn eindeutig
  products: SKU = "DB-GUIDE-2025-DE"
  users:    email (UNIQUE Hash-Index)
```

### ✅ Datumsfelder standardisieren

```
Immer ISO 8601 UTC: "2025-01-15T10:30:00Z"
Niemals: "15.01.2025", "Jan 15 2025", Unix-Timestamps (schwer lesbar)

AQL: DATE_NOW() gibt UTC-Timestamp in ms
ISO: DATE_ISO8601(DATE_NOW()) → "2025-01-15T10:30:00.000Z"
```

### ✅ Soft Delete statt Hard Delete

```aql
-- ✅ Soft Delete: Dokument behalten, markieren
UPDATE document WITH {
  deleted_at: DATE_NOW(),
  deleted_by: @user_id
} IN collection

-- In Abfragen immer filtern:
FILTER doc.deleted_at == null

-- ❌ Hard Delete (Datenverlust, keine Audit-Trail)
REMOVE document IN collection
```

---

## 8. Monitoring

### ✅ Key-Metrics überwachen

| Metrik | Warnschwelle | Kritische Schwelle |
|---|---|---|
| Query-Latenz P99 | > 100 ms | > 500 ms |
| Error-Rate | > 0.1% | > 1% |
| Memory-Nutzung | > 70% | > 90% |
| Disk-Nutzung | > 75% | > 90% |
| Connection-Pool | > 80% | > 95% |
| Replication-Lag | > 5 s | > 30 s |

### ✅ Prometheus-Alerts konfigurieren

```yaml
# prometheus-rules.yml
groups:
  - name: themisdb
    rules:
      - alert: ThemisDBHighLatency
        expr: themisdb_query_duration_seconds{quantile="0.99"} > 0.5
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "ThemisDB P99-Latenz > 500ms"

      - alert: ThemisDBHighErrorRate
        expr: rate(themisdb_queries_total{status="error"}[5m]) > 0.01
        for: 2m
        labels:
          severity: critical
```

### ✅ Slow Query Log aktivieren

```json
{
  "query": {
    "slow_query_log": {
      "enabled":     true,
      "threshold_ms": 100,
      "log_path":    "/var/log/themisdb/slow-queries.log"
    }
  }
}
```

---

## Checkliste für Produktion

### Security
- [ ] Authentifizierung aktiviert (JWT oder mTLS)
- [ ] TLS 1.3 konfiguriert mit gültigem Zertifikat
- [ ] Minimale RBAC-Berechtigungen eingerichtet
- [ ] Verschlüsselung at Rest aktiviert
- [ ] Netzwerk-Policies: nur notwendige Ports offen
- [ ] Audit-Logging aktiviert

### Performance
- [ ] Alle FILTER-Felder haben Indizes
- [ ] Queries mit Bind-Variablen
- [ ] Slow-Query-Log aktiviert und überwacht
- [ ] Query-Plan für kritische Queries geprüft (`EXPLAIN`)

### Operations
- [ ] Automatische Backups eingerichtet
- [ ] Backup-Restore getestet
- [ ] Monitoring mit Prometheus/Grafana
- [ ] Alerting auf kritische Metriken
- [ ] Graceful Shutdown konfiguriert
- [ ] Ressourcenlimits gesetzt

### Datenmodell
- [ ] Schema-Dokumentation aktuell
- [ ] Migrations-Skripte versioniert
- [ ] Rollback-Strategie definiert

## 🔗 Weiterführende Ressourcen

- [Deployment-Dokumentation](../../docs/)
- [Sicherheitshandbuch](../../SECURITY.md)
- [AQL-Optimierungs-Guide](../../aql/README.md)
- [Vollständige Beispiele](../../examples/)
