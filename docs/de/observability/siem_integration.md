# SIEM-Integration für ThemisDB

**Version:** 1.4.0  
**Stand:** 27. April 2026  
**Zielgruppe:** Security Operations Center (SOC) Engineers, SIEM-Administratoren, Sicherheitsarchitekten, Compliance-Beauftragte

---

## Inhaltsverzeichnis

1. [Überblick](#überblick)
2. [SIEM-relevante Metriken](#siem-relevante-metriken)
3. [Dashboard-Setup](#dashboard-setup)
4. [Alarm-Konfiguration](#alarm-konfiguration)
5. [Integrationsbeispiele](#integrationsbeispiele)
6. [Compliance-Mapping](#compliance-mapping)
7. [Export-Formate](#export-formate)
8. [Best Practices](#best-practices)

---

## Überblick

ThemisDB bietet umfassende Sicherheitsüberwachungsfunktionen, die speziell für Security Information and Event Management (SIEM)-Systeme entwickelt wurden. Diese Anleitung behandelt die Integration von ThemisDB-Metriken in Ihre SIEM-Infrastruktur für Echtzeit-Sicherheitsüberwachung, Bedrohungserkennung und Compliance-Berichterstattung.

### Hauptfunktionen

- **Echtzeit-Sicherheitsüberwachung**: Kontinuierliche Überwachung von Authentifizierungs-, Autorisierungs- und Audit-Ereignissen
- **Anomalieerkennung**: Erkennung von Baseline-Abweichungen und ungewöhnlichen Mustern
- **Compliance-Berichterstattung**: SOC2-, GDPR-, HIPAA-konforme Metriken und Alarme
- **Bedrohungserkennung**: Brute-Force-Angriffe, Privilege Escalation, Datenexfiltration
- **Audit-Trail**: Vollständige Audit-Protokollierung für forensische Analysen

### Architektur

```
┌─────────────┐
│  ThemisDB   │
│   Metriken  │──┐
└─────────────┘  │
                 │    ┌───────────────┐
                 ├───▶│  Prometheus   │
                 │    └───────┬───────┘
┌─────────────┐  │            │
│  ThemisDB   │  │            │    ┌──────────────┐
│  Audit-Logs │──┼────────────┼───▶│   Grafana    │
└─────────────┘  │            │    │  Dashboard   │
                 │            │    └──────────────┘
┌─────────────┐  │            │
│  ThemisDB   │  │            │    ┌──────────────┐
│  Ereignisse │──┘            └───▶│ SIEM-Systeme │
└─────────────┘                    │(Splunk/ELK)  │
                                   └──────────────┘
```

---

## SIEM-relevante Metriken

### Authentifizierungs- und Autorisierungsmetriken

Diese Metriken verfolgen Benutzerauthentifizierung, Autorisierungsversuche und Session-Management - kritisch für die Erkennung von Credential-Angriffen und unbefugten Zugriffsversuchen.

#### Authentifizierungsversuche

**Metrik**: `themis_auth_attempts_total{method, status}`

- **Typ**: Counter
- **Labels**:
  - `method`: Authentifizierungsmethode (password, token, oauth, certificate)
  - `status`: Ergebnis (success, failure)
  - `user`: Benutzername
  - `source_ip`: Quell-IP-Adresse
- **SIEM-Relevanz**: Erkennung von Brute-Force-Angriffen, Credential Stuffing, Password Spraying
- **Compliance**: SOC2 CC6.1, GDPR Artikel 32, HIPAA Access Control

**Beispiel-PromQL**:
```promql
# Fehlgeschlagene Login-Versuche pro Benutzer/IP in den letzten 5 Minuten
sum by (user, source_ip) (increase(themis_auth_attempts_total{status="failure"}[5m]))

# Authentifizierungserfolgsrate
(sum(rate(themis_auth_attempts_total{status="success"}[5m])) 
 / 
 sum(rate(themis_auth_attempts_total[5m]))) * 100
```

**Alarm-Schwellenwert**: >5 fehlgeschlagene Versuche von derselben IP in 2 Minuten

#### Fehlgeschlagene Authentifizierungen

**Metrik**: `themis_auth_failures_total{user, source_ip, reason}`

- **Typ**: Counter
- **Labels**:
  - `user`: Benutzername
  - `source_ip`: Quell-IP
  - `reason`: Fehlergrund (invalid_password, user_not_found, account_locked, expired_credentials)
- **SIEM-Relevanz**: Identifizierung gezielter Angriffe, Account-Enumeration
- **Compliance**: SOC2 CC6.1

#### Privilege-Escalation-Ereignisse

**Metrik**: `themis_privilege_escalation_total{user, from_role, to_role}`

- **Typ**: Counter
- **Labels**:
  - `user`: Benutzername
  - `from_role`: Ursprüngliche Rolle
  - `to_role`: Neue Rolle
  - `authorized_by`: Genehmiger (falls zutreffend)
- **SIEM-Relevanz**: **KRITISCH** - Erkennung unbefugter Rechteausweitung
- **Compliance**: SOC2 CC6.2, GDPR Artikel 32

**Alarm-Schwellenwert**: JEDE unbefugte Rechteausweitung

---

### Audit- und Sicherheitsereignis-Metriken

#### Audit-Ereignisse

**Metrik**: `themis_audit_events_total{operation, user, resource_type}`

- **Typ**: Counter
- **Labels**:
  - `operation`: CRUD-Operation (create, read, update, delete)
  - `user`: Benutzername
  - `resource_type`: Ressourcentyp (user, role, data, config)
  - `data_classification`: Datensensibilität (public, internal, confidential, restricted)
- **SIEM-Relevanz**: Vollständiger Audit-Trail für alle Datenoperationen
- **Compliance**: SOC2 CC6.1, GDPR Artikel 5, HIPAA Audit Controls

#### Sicherheitsvorfälle

**Metrik**: `themis_security_incidents_total{incident_type, severity}`

- **Typ**: Counter
- **Labels**:
  - `incident_type`: Vorfalltyp (brute_force, data_exfiltration, unauthorized_access, malware)
  - `severity`: Schweregrad (low, medium, high, critical)
  - `source_ip`: Quell-IP (falls zutreffend)
- **SIEM-Relevanz**: **KRITISCH** - Sicherheitsvorfall-Tracking
- **Compliance**: Alle Compliance-Frameworks

**Alarm-Schwellenwert**: JEDER kritische Vorfall

#### Datenexport-Ereignisse

**Metrik**: `themis_data_export_events_total{user, destination, data_classification}`

- **Typ**: Counter
- **Labels**:
  - `user`: Benutzername
  - `destination`: Export-Ziel (file, api, email)
  - `data_classification`: Datensensibilität
  - `authorized`: Autorisierungsstatus (true, false)
- **SIEM-Relevanz**: **KRITISCH** - Erkennung von Datenexfiltration
- **Compliance**: GDPR Artikel 33, HIPAA Breach Notification

**Alarm-Schwellenwert**: JEDER unbefugte Export

---

### Query- und Performance-Metriken

#### Query-Anfragerate

**Metrik**: `themis_query_requests_total{query_type}`

- **Typ**: Counter
- **Labels**:
  - `query_type`: Abfragetyp (select, insert, update, delete)
  - `user`: Benutzername
- **SIEM-Relevanz**: Erkennung anomaler Abfragemuster, Data Scraping
- **Compliance**: SOC2 CC7.2

#### Query-Fehlerrate

**Metrik**: `themis_query_errors_total{error_type}`

- **Typ**: Counter
- **Labels**:
  - `error_type`: Fehlertyp (syntax, permission, timeout, resource)
  - `user`: Benutzername
- **SIEM-Relevanz**: Hohe Fehlerraten können auf Angriffe oder Fehlkonfigurationen hinweisen
- **Compliance**: SOC2 CC7.1

---

### Infrastruktur-Metriken

#### CPU-Auslastung

**Metrik**: `node_cpu_seconds_total{mode}`

- **Typ**: Counter
- **SIEM-Relevanz**: Hohe CPU-Auslastung kann auf DoS-Angriff oder Crypto-Mining hinweisen
- **Compliance**: SOC2 CC7.2

#### Replikationsstatus

**Metrik**: `themis_replication_lag_seconds{replica_id}`

- **Typ**: Gauge
- **Labels**:
  - `replica_id`: Replikat-Kennung
- **SIEM-Relevanz**: Replikationsprobleme beeinträchtigen Datenverfügbarkeit und Backup
- **Compliance**: SOC2 CC7.2, HIPAA Disaster Recovery

---

## Dashboard-Setup

### Schnellstart

1. **SIEM-Dashboard importieren**:
   ```bash
   cp grafana/siem-security-monitoring.json /etc/grafana/provisioning/dashboards/
   systemctl restart grafana-server
   ```

2. **Prometheus konfigurieren**:
   ```yaml
   # prometheus.yml
   scrape_configs:
     - job_name: 'themisdb'
       static_configs:
         - targets: ['localhost:9091']
   
   rule_files:
     - 'alerts/siem_security_alerts.yaml'
   ```

3. **Dashboard aufrufen**:
   - URL: http://localhost:3000
   - Navigation: Dashboards → ThemisDB SIEM Security Monitoring

---

## Alarm-Konfiguration

### Kritische Sicherheitsalarme

Die folgenden Alarme sind für die SIEM-Integration vorkonfiguriert:

- **BruteForceAttackDetected**: Mehrere fehlgeschlagene Authentifizierungsversuche
- **PrivilegeEscalationDetected**: Unbefugte Rechteänderungen
- **UnauthorizedDataExport**: Datenexfiltrationsversuche
- **AuditLogTamperingAttempt**: Integritätsverletzungen
- **BackupFailure**: Compliance-kritische Backup-Probleme

Siehe `grafana/alerts/siem_security_alerts.yaml` für vollständige Alarmdefinitionen.

---

## Integrationsbeispiele

### Splunk-Integration

```bash
# Prometheus Exporter für Splunk installieren
pip install prometheus-splunk-exporter

# Abfragen konfigurieren
cat > config.yml <<EOF
splunk:
  host: splunk.example.com
  token: ihr-hec-token
  index: themisdb_metrics

queries:
  - name: auth_failures
    query: 'sum by (user, source_ip) (increase(themis_auth_failures_total[5m]))'
    interval: 5m
EOF
```

### ELK Stack-Integration

```ruby
# Logstash-Konfiguration
input {
  http_poller {
    urls => {
      prometheus => "http://localhost:9090/api/v1/query?query=themis_auth_failures_total"
    }
    schedule => { every => "30s" }
  }
}

output {
  elasticsearch {
    hosts => ["localhost:9200"]
    index => "themisdb-security-%{+YYYY.MM.dd}"
  }
}
```

### Syslog-Integration

```yaml
# themisdb.yaml
logging:
  syslog:
    enabled: true
    server: syslog.example.com
    port: 514
    format: rfc5424
```

---

## Compliance-Mapping

### SOC2 Trust Service-Kriterien

| Kriterium | Kontrolle | ThemisDB-Metriken | Alarm |
|-----------|-----------|-------------------|-------|
| CC6.1 | Logischer Zugriff | `themis_auth_attempts_total` | BruteForceAttackDetected |
| CC6.2 | Privilegierter Zugriff | `themis_privilege_escalation_total` | PrivilegeEscalationDetected |
| CC7.2 | Systemüberwachung | `themis_replication_lag_seconds` | BackupFailure |

### GDPR-Artikel

| Artikel | Anforderung | ThemisDB-Metriken | Alarm |
|---------|-------------|-------------------|-------|
| Artikel 5 | Zweckbindung | `themis_data_access_total` | SensitiveDataAccessWithoutJustification |
| Artikel 32 | Sicherheit der Verarbeitung | `themis_auth_failures_total` | BruteForceAttackDetected |
| Artikel 33 | Meldung von Verstößen | `themis_security_incidents_total` | UnauthorizedDataExport |

### HIPAA Security Rule

| Standard | ThemisDB-Metriken | Alarm |
|----------|-------------------|-------|
| 164.308(a)(1) | `themis_security_incidents_total` | SecurityPolicyViolation |
| 164.312(a)(1) | `themis_auth_attempts_total` | FailedAuthenticationAfterSuccess |
| 164.312(b) | `themis_audit_events_total` | AuditLogTamperingAttempt |

---

## Export-Formate

### JSON-Export-Beispiel

```json
{
  "timestamp": "2026-01-27T10:30:00Z",
  "event_type": "authentication_failure",
  "severity": "high",
  "user": "admin",
  "source_ip": "192.168.1.100",
  "failed_attempts": 15,
  "threat_category": "brute_force",
  "compliance": ["soc2", "gdpr"],
  "action_required": "IP blockieren, Authentifizierungsprotokolle prüfen"
}
```

### Syslog-Format (RFC 5424)

```
<134>1 2026-01-27T10:30:00Z themisdb-prod themisdb - SECURITY [themis@32473 event_type="auth_failure" user="admin" source_ip="192.168.1.100"] Brute-Force-Angriff von IP 192.168.1.100 erkannt
```

### CSV-Export für Compliance-Reporting

```csv
timestamp,event_type,user,resource,operation,data_classification,compliance_framework,authorized
2026-01-27T10:00:00Z,data_access,user1,customer_records,read,confidential,gdpr,true
2026-01-27T10:05:00Z,data_export,user2,financial_data,export,restricted,soc2,false
```

---

## Best Practices

### Metrik-Erfassung

✅ **TUN**:
- Metriken in konsistenten Intervallen erfassen (10-30 Sekunden)
- Angemessene Aufbewahrungsfristen verwenden (15+ Tage für Sicherheitsmetriken)
- Metrik-Kardinalitätslimits aktivieren
- Labels für Kategorisierung verwenden

❌ **NICHT TUN**:
- Zu häufig erfassen (< 5 Sekunden)
- PII in Metrik-Labels speichern
- Verschiedene Zeitskalen im selben Dashboard mischen

### Alarm-Konfiguration

✅ **TUN**:
- Angemessene `for`-Dauern festlegen
- Alarm-Gruppierung verwenden
- Umsetzbare Informationen in Annotationen einfügen
- Alarme regelmäßig testen

❌ **NICHT TUN**:
- Auf alles alarmieren
- Alarm-Ermüdung ignorieren
- Alarme ohne Handlungsanweisung erstellen

### Sicherheit

✅ **TUN**:
- Kommunikation verschlüsseln (TLS)
- Authentifizierung und RBAC implementieren
- Zugriff auf Überwachungssysteme prüfen
- Dashboard-Konfigurationen sichern

❌ **NICHT TUN**:
- Metrik-Endpunkte öffentlich ohne Authentifizierung verfügbar machen
- Geheimnisse in Dashboard-Variablen speichern
- Admin-Anmeldedaten teilen

### SIEM-Integration

✅ **TUN**:
- Ereignisformate systemübergreifend normalisieren
- Strukturierte Protokollierung verwenden (JSON)
- Kontext in Sicherheitsereignissen einbeziehen
- Ereignisse über mehrere Datenquellen korrelieren

❌ **NICHT TUN**:
- Alle Metriken an SIEM senden - selektiv sein
- Datenvolumenkosten ignorieren
- SIEM-Integration nicht regelmäßig testen

---

## Unterstützung und Ressourcen

- **GitHub-Repository**: https://github.com/makr-code/ThemisDB
- **Dokumentation**: Im `docs/`-Verzeichnis
- **Grafana-Dashboards**: `grafana/`-Verzeichnis
- **Alarmregeln**: `grafana/alerts/`-Verzeichnis

---

**Dokumentversion**: 1.0  
**Letzte Überprüfung**: 27. April 2026  
**Eigentümer**: Security Team
