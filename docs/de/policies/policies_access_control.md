# ThemisDB – Access Control Policy

**Version:** 1.0  
**Stand:** 6. April 2026  
**Klassifizierung:** Intern  
**Basis:** ISO 27001 (A.9), BSI C5 (IDM), NIST SP 800-53 (AC)

---

## 📋 Übersicht

Diese Access Control Policy definiert die Regeln und Verfahren zur Steuerung des Zugriffs auf ThemisDB-Systeme, Daten und Ressourcen. Sie gewährleistet das Prinzip der geringsten Berechtigung (Least Privilege) und die Einhaltung von Compliance-Anforderungen.

---

## 1. Grundsätze

### 1.1 Kernprinzipien

| Prinzip | Beschreibung |
|---------|--------------|
| **Least Privilege** | Nur minimal notwendige Rechte vergeben |
| **Need-to-Know** | Zugriff nur bei begründetem Bedarf |
| **Separation of Duties** | Kritische Funktionen auf mehrere Rollen verteilen |
| **Defense in Depth** | Mehrere Zugriffskontrollen kombinieren |
| **Default Deny** | Kein Zugriff ohne explizite Genehmigung |

### 1.2 Anwendungsbereich

| System | Abgedeckt |
|--------|-----------|
| ThemisDB Server | ✅ |
| Admin-Tools (WPF) | ✅ |
| Client SDKs | ✅ |
| Audit-Logs | ✅ |
| Backup-Systeme | ✅ |
| Entwicklungsumgebungen | ✅ |
| Dokumentation (öffentlich) | ⚠️ Teilweise |

---

## 2. Rollen und Berechtigungen

### 2.1 ThemisDB RBAC-Modell

ThemisDB implementiert ein 4-stufiges Role-Based Access Control (RBAC) System:

```
┌─────────────────────────────────────────────────────────────┐
│                         ADMIN                                │
│  Vollzugriff, Benutzerverwaltung, Key Rotation, Backup      │
├─────────────────────────────────────────────────────────────┤
│                       OPERATOR                               │
│  CRUD, Import/Export, Monitoring, keine Benutzerverwaltung  │
├─────────────────────────────────────────────────────────────┤
│                        ANALYST                               │
│  Lesen, Queries, Reports, keine Schreibrechte               │
├─────────────────────────────────────────────────────────────┤
│                       READONLY                               │
│  Nur Lesen, eingeschränkte Collections                      │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Berechtigungsmatrix

| Aktion | ADMIN | OPERATOR | ANALYST | READONLY |
|--------|:-----:|:--------:|:-------:|:--------:|
| **Entitäten lesen** | ✅ | ✅ | ✅ | ✅ |
| **Entitäten erstellen** | ✅ | ✅ | ❌ | ❌ |
| **Entitäten aktualisieren** | ✅ | ✅ | ❌ | ❌ |
| **Entitäten löschen** | ✅ | ✅ | ❌ | ❌ |
| **Queries ausführen** | ✅ | ✅ | ✅ | ✅ |
| **Bulk Import** | ✅ | ✅ | ❌ | ❌ |
| **Export** | ✅ | ✅ | ✅ | ❌ |
| **Backup erstellen** | ✅ | ✅ | ❌ | ❌ |
| **Backup wiederherstellen** | ✅ | ❌ | ❌ | ❌ |
| **Schema ändern** | ✅ | ❌ | ❌ | ❌ |
| **Benutzer verwalten** | ✅ | ❌ | ❌ | ❌ |
| **Token verwalten** | ✅ | ❌ | ❌ | ❌ |
| **Key Rotation** | ✅ | ❌ | ❌ | ❌ |
| **Audit-Logs lesen** | ✅ | ✅ | ✅ | ❌ |
| **Konfiguration ändern** | ✅ | ❌ | ❌ | ❌ |
| **Server stoppen** | ✅ | ❌ | ❌ | ❌ |

### 2.3 Zusätzliche Rollen (Anwendungsabhängig)

| Rolle | Beschreibung | Basis |
|-------|--------------|-------|
| **DATA_STEWARD** | Datenqualität, Klassifizierung | ANALYST + Governance |
| **SECURITY_ADMIN** | Security-Konfiguration | ADMIN (eingeschränkt) |
| **BACKUP_OPERATOR** | Nur Backup-Operationen | OPERATOR (eingeschränkt) |
| **AUDITOR** | Nur Audit-Log-Zugriff | READONLY + Audit |

---

## 3. Authentifizierung

### 3.1 Authentifizierungsmethoden

| Methode | Unterstützt | Empfehlung |
|---------|:-----------:|------------|
| **Bearer Token** | ✅ | Standard für API-Zugriff |
| **mTLS (Client Certificates)** | ✅ | Empfohlen für Produktion |
| **API Key** | ⚠️ | Nur für Service-Accounts |
| **Basic Auth** | ❌ | Nicht unterstützt |
| **OAuth 2.0** | 📋 | Geplant |
| **SAML/SSO** | 📋 | Geplant |

### 3.2 Token-Management

| Aspekt | Konfiguration |
|--------|---------------|
| **Token-Format** | JWT oder opak |
| **Gültigkeitsdauer** | Default: 1 Stunde |
| **Refresh** | Über Admin-API |
| **Revocation** | Sofort wirksam |
| **Rotation** | Empfohlen: 90 Tage |

### 3.3 Passwort-Policy (falls anwendbar)

| Anforderung | Wert |
|-------------|------|
| **Mindestlänge** | 12 Zeichen |
| **Komplexität** | Groß/Klein/Zahl/Sonderzeichen |
| **Maximales Alter** | 90 Tage |
| **Historie** | Letzte 10 nicht wiederverwendbar |
| **Sperrung nach Fehlversuchen** | 5 Versuche, 15 Minuten |

---

## 4. Autorisierung

### 4.1 Zugriffsentscheidungsfluss

```
┌─────────────────┐
│ Request eingeht │
└────────┬────────┘
         ▼
┌─────────────────┐     ┌───────────────┐
│ Token vorhanden?├─No──▶│ 401 Unauthorized│
└────────┬────────┘     └───────────────┘
         │Yes
         ▼
┌─────────────────┐     ┌───────────────┐
│ Token gültig?   ├─No──▶│ 401 Unauthorized│
└────────┬────────┘     └───────────────┘
         │Yes
         ▼
┌─────────────────┐     ┌───────────────┐
│ Rolle erlaubt?  ├─No──▶│ 403 Forbidden │
└────────┬────────┘     └───────────────┘
         │Yes
         ▼
┌─────────────────┐     ┌───────────────┐
│ Ressourcenzugriff├─No──▶│ 403 Forbidden │
│ erlaubt?        │     └───────────────┘
└────────┬────────┘
         │Yes
         ▼
┌─────────────────┐
│ Request erlaubt │
└─────────────────┘
```

### 4.2 Ressourcen-basierte Zugriffssteuerung

| Ebene | Beschreibung |
|-------|--------------|
| **Collection** | Zugriff auf bestimmte Collections einschränken |
| **Entity-Type** | Zugriff auf bestimmte Entitätstypen |
| **Field** | Feldbasierte Zugriffskontrolle (sensible Felder) |
| **Row-Level** | Zugriff basierend auf Dateninhalt |

### 4.3 Governance-Klassifizierung

| Stufe | Zugriffsanforderung |
|-------|---------------------|
| **OFFEN** | Alle authentifizierten Benutzer |
| **INTERN** | ANALYST und höher |
| **VERTRAULICH** | OPERATOR und höher |
| **STRENG_GEHEIM** | Nur ADMIN |

---

## 5. Lifecycle-Management

### 5.1 Benutzer-Onboarding

| Schritt | Verantwortlich | Dokumentation |
|---------|----------------|---------------|
| 1. Antrag stellen | Abteilungsleiter | Ticket-System |
| 2. Genehmigung | Data Owner | Schriftlich |
| 3. Rolle zuweisen | IT-Admin | Access Request Form |
| 4. Token erstellen | IT-Admin | Token-Management |
| 5. Schulung | Security | Nachweisdokument |
| 6. Aktivierung | IT-Admin | Audit-Log |

### 5.2 Benutzer-Offboarding

| Schritt | Frist | Verantwortlich |
|---------|-------|----------------|
| 1. Antrag (Kündigung, Wechsel) | Sofort | HR |
| 2. Zugriff deaktivieren | < 24h | IT-Admin |
| 3. Token widerrufen | Sofort | IT-Admin |
| 4. Datenübergabe | < 7 Tage | Abteilung |
| 5. Account löschen | < 30 Tage | IT-Admin |
| 6. Audit | Nach Abschluss | Security |

### 5.3 Zugriffs-Reviews

| Review-Typ | Frequenz | Verantwortlich |
|------------|----------|----------------|
| **Privilegierte Accounts** | Monatlich | Security |
| **Alle Benutzer** | Vierteljährlich | Manager |
| **Service Accounts** | Halbjährlich | IT-Admin |
| **Externe Zugänge** | Monatlich | Security |
| **Inaktive Accounts** | Monatlich | IT-Admin |

---

## 6. Privilegierte Zugänge

### 6.1 Privileged Access Management (PAM)

| Maßnahme | Status |
|----------|--------|
| **Separate Admin-Accounts** | ✅ Erforderlich |
| **Session Recording** | ⚠️ Empfohlen |
| **Just-in-Time Access** | 📋 Geplant |
| **Approval Workflow** | ⚠️ Empfohlen |
| **Time-limited Access** | ✅ Implementiert |

### 6.2 Break-Glass-Verfahren

Für Notfallsituationen, wenn reguläre Zugangswege nicht verfügbar sind:

1. **Dokumentierter Notfall** erforderlich
2. **Zwei-Personen-Regel** (Dual Control)
3. **Sofortige Benachrichtigung** an Security
4. **Vollständiges Audit-Logging**
5. **Post-Incident Review** innerhalb 24h

### 6.3 Admin-Token-Verwaltung

| Aspekt | Anforderung |
|--------|-------------|
| **Speicherung** | Vault/HSM oder verschlüsselt |
| **Rotation** | Alle 30 Tage |
| **Zugriff** | Nur autorisiertes Personal |
| **Logging** | Jede Verwendung protokollieren |

---

## 7. Externe Zugänge

### 7.1 Partner/Lieferanten

| Anforderung | Beschreibung |
|-------------|--------------|
| **Vertrag** | NDA und AVV erforderlich |
| **Befristung** | Maximale Gültigkeit 1 Jahr |
| **Minimale Rechte** | Nur projektbezogene Rechte |
| **Monitoring** | Erweiterte Überwachung |
| **Review** | Monatliche Überprüfung |

### 7.2 Remote Access

| Methode | Anforderung |
|---------|-------------|
| **VPN** | Erforderlich für Admin-Zugang |
| **MFA** | Erforderlich für alle Remote-Zugänge |
| **IP-Whitelist** | Empfohlen für Produktionssysteme |
| **Session Timeout** | Max. 8 Stunden |

---

## 8. Audit und Überwachung

### 8.1 Protokollierte Ereignisse

| Ereignis | Logging-Level |
|----------|---------------|
| Login-Erfolg | INFO |
| Login-Fehler | WARNING |
| Passwort-Änderung | INFO |
| Rollenänderung | IMPORTANT |
| Privilegierte Aktionen | IMPORTANT |
| Token-Erstellung | INFO |
| Token-Widerruf | INFO |
| Zugriffsverweigerung | WARNING |

### 8.2 Alarmierung

| Trigger | Aktion |
|---------|--------|
| > 5 fehlgeschlagene Logins | Alert an Security |
| Admin-Login außerhalb Bürozeiten | Alert an Security |
| Rollenänderung | Benachrichtigung an Manager |
| Bulk-Datenexport | Alert an Data Owner |

### 8.3 Aufbewahrung

| Log-Typ | Aufbewahrungsdauer |
|---------|-------------------|
| Access Logs | 1 Jahr |
| Audit Logs | 7 Jahre |
| Security Events | 3 Jahre |

---

## 9. Compliance-Mapping

| Standard | Anforderung | Erfüllt |
|----------|-------------|:-------:|
| **ISO 27001** | A.9 Access Control | ✅ |
| **BSI C5** | IDM-01 bis IDM-08 | ✅ |
| **NIST SP 800-53** | AC-1 bis AC-25 | ✅ |
| **DSGVO** | Art. 32 (Zugriffskontrolle) | ✅ |
| **SOC 2** | CC6.1 bis CC6.8 | ✅ |

---

## 10. Verantwortlichkeiten

| Rolle | Verantwortlichkeiten |
|-------|---------------------|
| **Data Owner** | Genehmigung von Zugriffen auf ihre Daten |
| **IT-Admin** | Technische Umsetzung, Token-Management |
| **Security** | Policy-Enforcement, Audits, Reviews |
| **Manager** | Genehmigung für Mitarbeiter |
| **Benutzer** | Schutz von Credentials, Meldung von Vorfällen |

---

## 11. Ausnahmen

Ausnahmen von dieser Policy erfordern:

1. **Schriftlichen Antrag** mit Begründung
2. **Risikobewertung** durch Security
3. **Genehmigung** durch CISO/IT-Leitung
4. **Befristung** (max. 6 Monate)
5. **Dokumentation** im Ausnahme-Register
6. **Regelmäßige Review** (monatlich)

---

## 12. Anhänge

### A. Token-Konfiguration (themis.yaml)

```yaml
authentication:
  token:
    type: bearer
    expiry_hours: 1
    refresh_enabled: true
    
authorization:
  rbac:
    enabled: true
    default_role: readonly
    
  roles:
    - name: admin
      permissions: [all]
    - name: operator
      permissions: [read, write, export, backup]
    - name: analyst
      permissions: [read, query, export]
    - name: readonly
      permissions: [read]
```

### B. Referenzen

| Dokument | Pfad |
|----------|------|
| RBAC Dokumentation | `docs/features/rbac.md` |
| Security Architecture | `docs/architecture/security.md` |
| Audit Logging | `docs/operations/audit_logging.md` |
| Risk Register | `docs/compliance/RISK_REGISTER.md` |

---

**Letzte Aktualisierung:** November 2025  
**Dokumentverantwortlicher:** ThemisDB Security Team  
**Nächstes Review:** [Datum + 12 Monate]
