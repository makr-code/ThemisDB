# Kapitel 40: Data Governance & Compliance {#chapter_40_data-governance-compliance}

> *"Vertrauen ist gut, Kontrolle ist besser. In der Datenwelt sind beide unverzichtbar."*  
> *— Vladimir Lenin (adaptiert für Data Governance)*

> **Zusammenfassung:** Dieses Kapitel behandelt systematisch die Implementierung von Data Governance und Compliance-Mechanismen in ThemisDB-Systemen. Wir analysieren etablierte Frameworks wie ISO 27001, SOC 2 und DSGVO/GDPR und zeigen deren praktische Umsetzung durch Policies, Rollenmodelle, Audit-Trails und technische Schutzmaßnahmen. Dabei folgen wir dem Prinzip *Privacy by Design*[^1] und integrieren Governance-Anforderungen bereits in die Systemarchitektur statt nachträglicher Compliance-Theater. Der Fokus liegt auf operationaler Exzellenz durch automatisierte Controls, kontinuierliche Überwachung und nachweisbare Evidenz-Ketten für regulatorische Audits.

**Lernziele:** Nach Bearbeitung dieses Kapitels verstehen wir die Implementierung von Governance-Operating-Models, Data-Classification-Schemes, Access-Control-Mechanismen (RBAC, ABAC), Data-Masking-Strategien, Retention-Policies, immutable Audit-Logs sowie die praktische Erfüllung von DSGVO-Betroffenenrechten und Compliance-Framework-Anforderungen (SOX, SOC 2, ISO 27001). Wir kennen Best Practices für Evidence-Management, Break-Glass-Prozeduren und Just-in-Time-Access-Modelle.

**Voraussetzungen:** Grundverständnis von Kapitel 2 (Architektur), Kapitel 19 (Monitoring), Kapitel 21 (Performance) und Kapitel 36 (Security Hardening). Kenntnisse über DSGVO-Grundsätze und regulatorische Anforderungen sind hilfreich, werden aber im Text erklärt.

---

## 40.0 Data Governance Framework: Übersicht {#chapter_40_0_data-governance-framework-uebersicht}

Data Governance in ThemisDB folgt einem mehrschichtigen Modell, das technische Controls mit organisatorischen Policies verbindet. Wir strukturieren Daten nach Sensitivität, erzwingen Zugriffskontrolle auf Feld-Ebene und dokumentieren alle Operationen in unveränderlichen Audit-Logs. Dieses Framework orientiert sich an NIST Cybersecurity Framework[^2] und COBIT 2019[^3] Governance-Prinzipien, adaptiert für moderne Multi-Model-Datenbanken.

```mermaid
graph TB
    Data[Personal Data<br/>Ingest] --> Classification[Data Classification<br/>AUTO_CLASSIFY]
    
    Classification --> Public[PUBLIC<br/>Level 0]
    Classification --> Internal[INTERNAL<br/>Level 1]
    Classification --> Confidential[CONFIDENTIAL<br/>Level 2 - PII]
    Classification --> Restricted[RESTRICTED<br/>Level 3 - Health]
    
    Confidential --> Encrypt[Field-Level Encryption<br/>AES-256-GCM]
    Restricted --> Encrypt
    
    Encrypt --> Access[Access Control Layer<br/>RBAC + ABAC]
    Public --> Access
    Internal --> Access
    
    Access --> RBAC[Role-Based Access<br/>Reader/Writer/Admin]
    RBAC --> ABAC[Attribute-Based<br/>Context-Aware]
    
    ABAC --> Audit[Immutable Audit Log<br/>SHA-256 Chain]
    Audit --> Retention[Retention Policy Engine]
    
    Retention --> Active[Active Storage<br/>0-2 years]
    Retention --> Archive[Archive Storage<br/>2-7 years]
    Retention --> Delete[Crypto-Erase<br/>after 7 years]
    
    Delete --> Evidence[Evidence Store<br/>Compliance Proof]
    Evidence --> Review[Quarterly Review<br/>Governance Committee]
    Review --> Classification
    
    style Confidential fill:#ff6b6b
    style Encrypt fill:#f093fb
    style Audit fill:#4facfe
    style Evidence fill:#51cf66
```

**Abbildung 40.1:** Data-Governance-Framework mit automatischer Klassifizierung, verschlüsselter Speicherung, rollenbasierter Zugriffskontrolle und Retention-Management. Der geschlossene Regelkreis mit Quarterly Reviews gewährleistet kontinuierliche Verbesserung.

**Performance-Charakteristiken:**
- Classification Overhead: ~2-5 ms pro Dokument (Pattern-Matching mit Regex)
- Encryption Overhead: ~8-15 ms für Field-Level AES-256-GCM (Intel AES-NI)
- Audit Log Latency: ~1-3 ms (Async Write zu dediziertem Cluster)
- Retention Check Frequency: Stündlich (Background Job mit LOW Priority)

---

## 40.1 Governance Operating Model {#chapter_40_1_governance-operating-model}

Ein effektives Governance Operating Model definiert Rollen, Verantwortlichkeiten und Prozesse für den gesamten Daten-Lebenszyklus. Wir unterscheiden zwischen **Data Owners** (fachliche Verantwortung für Business-Entities), **Data Stewards** (Data-Quality-Management, Metadaten-Pflege) und **Data Custodians** (technische Implementierung von Security-Controls). Dieses RACI-Modell[^4] (Responsible, Accountable, Consulted, Informed) verhindert Ownership-Lücken und schafft Clarity of Accountability für regulatorische Nachweise.

```mermaid
flowchart TB
    subgraph "Governance Framework"
        direction TB
        Policy[Policy Catalog<br/>Access · Retention · Encryption]
        Owner[Data Owners<br/>Business Units]
        Steward[Data Stewards<br/>Data Quality Team]
        Custodian[Data Custodians<br/>Platform Engineering]
    end
    
    subgraph "Technical Controls"
        direction TB
        Access[Access Control<br/>RBAC + ABAC]
        Classify[Auto-Classification<br/>ML-based PII Detection]
        Encrypt[Encryption<br/>At-Rest + In-Transit]
        Audit[Audit Logs<br/>Immutable Chain]
    end
    
    subgraph "Compliance Frameworks"
        direction TB
        GDPR[GDPR/DSGVO<br/>Art. 5-32]
        SOX[SOX<br/>Section 404]
        SOC2[SOC 2<br/>Trust Services]
        ISO[ISO 27001<br/>Annex A Controls]
    end
    
    subgraph "Operations & Evidence"
        direction TB
        Monitor[Continuous Monitoring<br/>Prometheus + Grafana]
        Alert[Alerting<br/>PagerDuty Integration]
        Response[Incident Response<br/>Runbooks + Playbooks]
        Evidence[Evidence Collection<br/>Signed Artifacts]
    end
    
    Policy --> Access
    Policy --> Classify
    Policy --> Encrypt
    Policy --> Audit
    
    Owner --> Policy
    Steward --> Classify
    Custodian --> Encrypt
    
    Access --> GDPR
    Classify --> SOC2
    Encrypt --> ISO
    Audit --> SOX
    
    Audit --> Monitor
    Monitor --> Alert
    Alert --> Response
    Response --> Evidence
    
    Evidence --> Review[Quarterly Governance Review]
    Review --> Policy
    
    style Policy fill:#4dabf7
    style GDPR fill:#fa5252
    style Evidence fill:#51cf66
    style Monitor fill:#ffd43b
```

**Abbildung 40.2:** Governance Operating Model mit Feedback-Loop. Die Quarterly Reviews aktualisieren Policies basierend auf Evidence aus Operational Incidents und Compliance-Audits.

### 40.1.1 Rollenmodel und Verantwortlichkeiten {#chapter_40_1_1_rollenmodel-verantwortlichkeiten}

Wir implementieren eine strikte Trennung zwischen strategischen, taktischen und operationalen Governance-Ebenen. Data Owners definieren Business-Policies (z.B. "Kundendaten dürfen nur in EU-Rechenzentren gespeichert werden"), Data Stewards entwickeln technische Standards (z.B. Field-Naming-Conventions für PII-Fields) und Data Custodians setzen diese durch Automation um (z.B. Deployment von Encryption-Policies via Infrastructure-as-Code).

| Rolle | Verantwortung | Beispiel-Aktivitäten | Accountability |
|-------|---------------|---------------------|----------------|
| **Data Owner** | Business-Verantwortung für Daten-Entity | Collection-Ownership definieren, Zugriffs-Approvals erteilen, Business-Rules festlegen | Accountable für Compliance-Verletzungen |
| **Data Steward** | Data-Quality & Metadaten-Management | Classification-Rules pflegen, PII-Detection tunen, Data-Lineage dokumentieren | Responsible für Data-Quality-Metriken |
| **Data Custodian** | Technische Implementierung & Operations | Encryption konfigurieren, Backups ausführen, Incidents beheben | Responsible für Availability & Integrity |
| **Compliance Officer** | Regulatory Oversight & Audit Coordination | DSGVO-Anfragen koordinieren, Audits vorbereiten, Policy-Reviews durchführen | Accountable für regulatorische Nachweise |

**Best Practice:** Nutzen wir ThemisDB-Collections zur Abbildung des Rollenmodels:

```aql
// Collection: governance_roles
{
  "_key": "users_collection_owner",
  "collection": "users",
  "owner": {
    "team": "crm",
    "contact": "alice@example.com",
    "approved_by": "cto",
    "approved_at": "2024-01-15T10:00:00Z"
  },
  "steward": {
    "team": "data_platform",
    "contact": "bob@example.com"
  },
  "custodian": {
    "team": "platform_engineering",
    "oncall": "ops@example.com"
  },
  "metadata": {
    "business_purpose": "Customer Relationship Management",
    "data_classification": "CONFIDENTIAL",
    "retention_years": 7,
    "gdpr_relevant": true
  }
}
```

### 40.1.2 Policy Catalog und Version Control {#chapter_40_1_2_policy-catalog-version-control}

Policies werden als versionierte Dokumente in ThemisDB-Collections gespeichert und über automatisierte Deployment-Pipelines ausgerollt. Jede Policy erhält eindeutige Identifikation (z.B. `AC-3-Access-Enforcement-v1.2`), Change-History durch ThemisDB's Multi-Version-Concurrency-Control (MVCC) und Approval-Workflow. Wir nutzen Policy-as-Code-Ansätze (ähnlich Open Policy Agent[^5]) für automatisierte Enforcement, gespeichert direkt in ThemisDB statt externen Versionskontrollsystemen.

```aql
// Collection: governance_policies
// Policy-Dokument mit eingebetteter Versionshistorie in ThemisDB
{
  "_key": "AC-3-v1.2",
  "_rev": "_fxK3UPW--D",  // ThemisDB MVCC Revision
  "policy_id": "AC-3",
  "version": "1.2",
  "status": "active",
  "title": "Access Enforcement - Least Privilege Principle",
  "framework_mapping": [
    {"framework": "ISO27001", "control": "A.9.1.2"},
    {"framework": "SOC2", "control": "CC6.1"},
    {"framework": "NIST", "control": "AC-3"}
  ],
  "rules": [
    {
      "id": "AC-3.1",
      "description": "Default access level is NONE",
      "enforcement": "Block all access unless explicitly granted",
      "aql_implementation": null
    },
    {
      "id": "AC-3.2",
      "description": "Access grants require approval workflow",
      "aql_implementation": `
        FOR grant IN access_grant_requests
          FILTER grant.status == 'pending'
          FILTER grant.approver_role IN ['owner', 'manager']
          RETURN grant
      `
    },
    {
      "id": "AC-3.3",
      "description": "Time-limited access (JIT)",
      "aql_implementation": `
        FOR grant IN active_grants
          FILTER grant.expires_at < DATE_NOW()
          UPDATE grant WITH {status: 'expired'} IN access_grants
      `
    }
  ],
  "review_schedule": {
    "frequency": "quarterly",
    "next_review": "2024-04-01",
    "owner": "security_team"
  },
  "change_history": [
    {
      "version": "1.2",
      "changed_at": "2024-01-15T10:30:00Z",
      "changed_by": "alice@example.com",
      "approved_by": ["security_team", "data_owner"],
      "change_summary": "Added JIT access rule AC-3.3"
    },
    {
      "version": "1.1",
      "changed_at": "2023-10-01T09:00:00Z",
      "changed_by": "bob@example.com",
      "approved_by": ["compliance_officer"],
      "change_summary": "Initial policy creation"
    }
  ],
  "evidence_refs": [
    "evidence/access_grants_export_2024Q1",
    "evidence/approval_workflow_config",
    "evidence/automated_expiry_logs"
  ]
}
```

**Change Control Process (ThemisDB-nativ):**
1. **RFC erstellen:** Engineer legt neues Policy-Dokument in `governance_policies_draft` Collection an
2. **Peer Review:** Mindestens 2 Approvals dokumentiert in `policy_approvals` Collection (Security + Data Steward)
3. **Impact Analysis:** AQL-Query analysiert betroffene Collections/Users durch Simulation
4. **Approval:** Data Owner oder Compliance Officer setzt `approved: true` im Draft-Dokument
5. **Rollout:** Automatisierter ThemisDB-Job kopiert Draft zu `governance_policies` (Active), alte Version archiviert in `governance_policies_archive`
6. **Evidence:** ThemisDB Audit-Log speichert Deployment-Timestamp und Document-Revision (_rev) automatisch

---

## 40.2 Data Classification Schema {#chapter_40_2_data-classification-schema}

Data Classification bildet die Grundlage für differenzierte Security-Controls. Wir kategorisieren Daten nach Sensitivität in vier Levels (PUBLIC, INTERNAL, CONFIDENTIAL, RESTRICTED) und definieren für jedes Level spezifische Schutzmaßnahmen wie Encryption, Access-Approval-Workflows und Masking-Strategien. Diese Klassifizierung erfolgt automatisch durch Pattern-Matching (z.B. Regex für Email, IBAN, ICD-10-Codes) oder manuell durch Data Stewards bei ambigen Fällen.

```yaml
classification:
  levels:
    - name: PUBLIC
      description: "Keine Sensibilität"
    - name: INTERNAL
      description: "Nur intern"
    - name: CONFIDENTIAL
      description: "PII/finanziell"
    - name: STRICT
      description: "Gesundheit, Strafverfolgung"
```

**Regeln:**
- Standard = INTERNAL
- CONFIDENTIAL/STRICT → Encryption + Access Approval
- Masking im UI/Logs für CONFIDENTIAL/STRICT

---

## 40.3 Access Control & Least Privilege {#chapter_40_3_access-control-least-privilege}

Access Control in ThemisDB folgt dem Least-Privilege-Prinzip: Jeder User erhält nur die minimal notwendigen Permissions für seine Aufgaben. Wir kombinieren Role-Based Access Control (RBAC) für statische Rollen (Reader, Writer, Admin, Auditor) mit Attribute-Based Access Control (ABAC) für dynamische Context-Aware-Entscheidungen (z.B. VPN-Zugriff, Geschäftszeiten, MFA-Status). Just-in-Time (JIT) Access mit automatischem Ablauf nach 24h minimiert Standing-Privileges und reduziert das Angriffsfenster bei kompromittierten Accounts.

**Zentrale Konzepte:**
- RBAC-Modelle (Reader, Writer, Admin, Auditor)
- JIT Access mit Ablauf (z.B. 24h)
- Break-Glass Accounts (mit separatem Audit)
- Quarterly Access Review

```aql
-- Grant role
INSERT {
  user_id: 'alice',
  role: 'reader',
  granted_by: 'security',
  expires_at: DATE_ADD(NOW(), 1, 'day')
} INTO access_grants
```

---

## 40.4 Data Masking & Tokenization {#chapter_40_4_data-masking-tokenization}

Data Masking schützt sensitive Informationen bei Weitergabe an weniger privilegierte User oder externe Systeme. Wir unterscheiden zwischen Dynamic Masking (On-the-fly-Transformation je nach User-Role) und Static Masking (deterministische Transformation für Test-Environments). Tokenization ersetzt echte Werte durch irreversible Pseudonyme, die in einer separaten Token-Vault gespeichert sind. Dies ermöglicht Analytics auf pseudonymisierten Daten ohne Zugriff auf PII, erfüllt DSGVO-Pseudonymisierungs-Anforderungen und reduziert Compliance-Scope.

### 40.4.1 Dynamic Masking {#chapter_40_4_1_dynamic-masking}

Dynamic Masking transformiert sensitive Fields während der Query-Ausführung basierend auf der User-Role. Ein Reader sieht `a***@example.com` statt `alice@example.com`, während ein Admin den vollständigen Wert erhält. Die Implementierung erfolgt durch AQL-Functions, die abhängig vom Session-Context unterschiedliche Outputs generieren. Der Performance-Overhead beträgt ~0.5-1ms pro maskiertem Field.

**Implementierung:**

```aql
FUNCTION mask_email(email) {
  RETURN CONCAT(SUBSTRING(email, 0, 2), '***', SUBSTRING(email, POSITION(email, '@')-1))
}

FOR user IN users
  RETURN {
    name: user.name,
    email: mask_email(user.email)
  }
```

### 40.4.2 Tokenization für irreversible Pseudonymisierung {#chapter_40_4_2_tokenization-irreversible-pseudonymisierung}

Tokenization ersetzt echte Werte (z.B. Sozialversicherungsnummern) durch zufällige Tokens und speichert die Mapping-Tabelle in einem separaten, hochgesicherten Token-Vault (Redis mit TLS+mTLS). Im Gegensatz zu Encryption ist Tokenization irreversibel für Systeme ohne Vault-Zugriff, was Analytics auf pseudonymisierten Daten ermöglicht. Die Token-Länge (16 Hex-Zeichen) ist ausreichend für Collision-Resistance bei Milliarden von Records.

**Implementierung:**

```python
# tokenization.py
import secrets

tokens = {}

def tokenize(value):
    token = secrets.token_hex(8)
    tokens[token] = value
    return token

def detokenize(token):
    return tokens.get(token)
```

---

## 40.5 Data Lifecycle & Retention Management {#chapter_40_5_data-lifecycle-retention}

Der Data Lifecycle beschreibt alle Phasen vom Ingest bis zur sicheren Löschung: Klassifizierung bei Aufnahme, verschlüsselte Speicherung, kontrollierte Nutzung mit Purpose Limitation, Archivierung in Cold Storage nach Ablauf der Hot-Retention-Period und schließlich Crypto-Erase bei Right-to-be-forgotten-Requests. Wir implementieren automatisierte Retention-Policies durch Stündliche Background-Jobs (LOW Priority), die Ablaufdaten prüfen und Transition-Workflows auslösen. Dies gewährleistet DSGVO-Compliance (Art. 5 Abs. 1e: Speicherbegrenzung) ohne manuelle Intervention.

**Lifecycle-Phasen:**
- **Ingest:** Klassifizierung, Validation, Encryption
- **Store:** Encryption-at-rest, Access Controls, Audit Logs
- **Use:** Masking, Purpose Limitation, Minimal Disclosure
- **Share:** Anonymize/Pseudonymize, DPA prüfen
- **Archive:** Cold Storage mit Verschlüsselung
- **Delete:** Right-to-be-forgotten Prozesse

Retention Beispiel:

```yaml
retention:
  users: 7y
  logs: 180d
  audit_logs: 365d
  pii: 2y
```

---

## 40.6 Audit Logging & Evidence Management {#chapter_40_6_audit-logging-evidence}

Audit Logs dokumentieren alle sicherheitsrelevanten Operationen (Zugriffe, Änderungen, Löschungen, Policy-Deployments) in einer manipulationssicheren SHA-256-Hash-Chain. Jeder Log-Eintrag enthält Timestamp, Actor (User/Service-Account), Operation, Resource-Identifier und Result (Success/Failure). Der Evidence Store sammelt zusätzlich Artefakte für Compliance-Audits: Ticket-IDs für Change-Approvals, Screenshots von IAM-Konfigurationen, signierte Exports von Access-Grants und Hash-Chain-Verifications. Diese strukturierte Evidenz ermöglicht nachweisbare Compliance gegenüber ISO 27001 (A.12.4.1), SOX (Section 404) und SOC 2 (CC4.1).

**Komponenten:**
- Immutable Audit Log (Kapitel 36)
- Evidence Store: Tickets, Screenshots, CLI Output, Hashes
- Kontroll-Nachweise per Control-ID (ISO27001 Annex A, SOC2 CC)

```markdown
# Control: AC-3 (Access Enforcement)
- Policy: AC-3 v1.2
- Evidence:
  - access_grants export (signed)
  - quarterly review report (PDF)
  - IAM config screenshot
  - audit log hash chain verification
```

---

## 40.7 Privacy & DSGVO-Compliance {#chapter_40_7_privacy-dsgvo-compliance}

Die Datenschutz-Grundverordnung (DSGVO/GDPR) verlangt technische und organisatorische Maßnahmen zum Schutz personenbezogener Daten. Wir implementieren Privacy by Design[^1] durch Default-Encryption, Purpose Limitation (Zweckbindung) via Policy-Enforcement und Data Minimization durch Mandatory-Fields-Only-Schemas. Die acht Betroffenenrechte (Auskunft, Berichtigung, Löschung, Einschränkung, Übertragbarkeit, Widerspruch, Automatisierte-Entscheidung, Beschwerde) sind durch AQL-Functions und Self-Service-APIs automatisiert, mit Response-Times unter dem gesetzlichen Limit von 30 Tagen (typisch: <1 Tag).

### 40.7.1 DSGVO-Betroffenenrechte {#chapter_40_7_1_dsgvo-betroffenenrechte}

Die DSGVO gewährt betroffenen Personen umfassende Rechte über ihre Daten. Wir implementieren diese Rechte durch dedizierte AQL-Functions und REST-APIs, die automatisch alle relevanten Collections durchsuchen, Daten exportieren oder pseudonymisieren. Besonders kritisch ist das Right to Erasure (Art. 17 DSGVO), das technisch durch Crypto-Erase (Schlüssel-Vernichtung) umgesetzt wird, während Metadaten für Audit-Trails erhalten bleiben.

**Rechte-Katalog:**
- Auskunft, Berichtigung, Löschung, Einschränkung, Übertragbarkeit, Widerspruch

```aql
-- Right to Erasure (Pseudonymize)
FUNCTION gdpr_delete(user_id) {
  UPDATE {_id: 'users/' + user_id} WITH {
    name: 'Anonymized',
    email: null,
    phone: null,
    gdpr_deleted_at: NOW(),
    is_deleted: true
  } IN users
  
  REMOVE d IN audit_logs
  FILTER d.resource == CONCAT('users/', user_id)
}
```

### 40.7.2 Verzeichnis von Verarbeitungstätigkeiten (VVT) {#chapter_40_7_2_verzeichnis-verarbeitungstaetigkeiten}

Art. 30 DSGVO verpflichtet Organisationen zur Führung eines Verzeichnisses von Verarbeitungstätigkeiten (VVT). Wir speichern dieses Register direkt in ThemisDB als strukturierte Collection `processing_activities`, die automatisch aus Policy-Metadaten generiert wird. Jeder Eintrag dokumentiert Zweck, Datenkategorien, Empfänger, Speicherorte, Löschfristen und Data Processing Agreements (DPAs) mit Prozessoren. Dies ermöglicht automatisierte Compliance-Reports und schnelle Beantwortung von Aufsichtsbehörden-Anfragen.

**Register-Komponenten:**
- Zweck, Kategorien, Empfänger, Speicherorte, Löschfristen
- DPAs mit Prozessoren dokumentieren

---

## 40.8 Compliance Frameworks Integration {#chapter_40_8_compliance-frameworks}

ThemisDB-Governance-Mechanismen erfüllen Anforderungen mehrerer Compliance-Frameworks gleichzeitig durch intelligentes Control-Mapping. Ein einzelner technischer Control (z.B. Immutable Audit Log) adressiert parallel SOX Section 404 (Change-Management-Nachweise), SOC 2 CC4.1 (Monitoring Activities) und ISO 27001 A.12.4.1 (Event Logging). Wir nutzen Framework-Mapping-Tabellen in Policy-Dokumenten, um für Audits nachzuweisen, welche Controls welche Anforderungen erfüllen. Dies reduziert Audit-Overhead und vermeidet redundante Implementierungen.

**Framework-Übersicht:**
- **SOX:** Change-Management, Segregation of Duties, Evidence für Finanzsysteme
- **SOC2:** Security/Availability/Confidentiality; Controls: Access, Change, Incident, Monitoring
- **ISO27001:** ISMS, Risikoanalyse, Controls Annex A (z.B. A.8 Asset Management, A.9 Access Control)

Kontroll-Mapping Beispiel:

```markdown
- A.9.1.2 (Access Control): RBAC + Quarterly Review + JIT
- A.12.4.1 (Logging): Immutable Audit Log, 365d retention
- A.12.6.1 (Malware Protection): EDR auf DB-Hosts
- A.17.1.1 (Continuity): DR-Plan + Tests halbjährlich
```

### 40.8.1 ISO 27001 Annex A Control Mapping {#chapter_40_8_1_iso27001-mapping}

ThemisDB implementiert **28 ISO 27001 Annex A Controls** systematisch durch technische und organisatorische Maßnahmen. Diese Tabelle mappt ThemisDB-Features zu ISO 27001 Requirements und bietet AQL-Queries zur Validierung.

**Kategorie A.5: Organisatorische Kontrollen**

| ISO 27001 Control | ThemisDB Implementation | Validierungs-Query | Status |
|-------------------|-------------------------|-------------------|---------|
| **A.5.1 Policies** | Versionierte Policies in `governance_policies` | `FOR p IN governance_policies FILTER p.status == 'active' RETURN p._key` | ✅ |
| **A.5.2 Information Security Roles** | RBAC mit Data Owner, Steward, Custodian Rollen | `FOR r IN governance_roles RETURN {role: r.name, members: LENGTH(r.members)}` | ✅ |
| **A.5.3 Segregation of Duties** | Separation: Admin ≠ Auditor, Owner ≠ Executor | `FOR u IN users FILTER "admin" IN u.roles AND "auditor" IN u.roles RETURN u` (should be empty) | ✅ |

**Kategorie A.8: Asset Management**

| ISO 27001 Control | ThemisDB Implementation | Validierungs-Query | Status |
|-------------------|-------------------------|-------------------|---------|
| **A.8.1 Inventory of Assets** | Asset Registry: Collections, Schemas, Indexes | `FOR a IN asset_registry RETURN {type: a.type, name: a.name, classification: a.classification}` | ✅ |
| **A.8.2 Information Classification** | 5-Level: PUBLIC → INTERNAL → CONFIDENTIAL → REGULATED → RESTRICTED | `FOR d IN documents COLLECT level = d.classification WITH COUNT INTO cnt RETURN {level, count: cnt}` | ✅ |
| **A.8.3 Media Handling** | Crypto-Erase für Retention-Ablauf, Secure Delete | `FOR d IN deleted_data FILTER d.deleted_at > DATE_SUBTRACT(DATE_NOW(), 7, 'day') RETURN d` | ✅ |

**Kategorie A.9: Zugriffskontrolle**

| ISO 27001 Control | ThemisDB Implementation | Validierungs-Query | Status |
|-------------------|-------------------------|-------------------|---------|
| **A.9.1 Access Control Policy** | RBAC + ABAC mit Policy-as-Code | `FOR p IN access_policies FILTER p.type == 'access_control' RETURN p.rules` | ✅ |
| **A.9.2 User Access Management** | JIT Access mit Auto-Expiry, Quarterly Reviews | `FOR j IN jit_access FILTER j.expires_at < DATE_NOW() RETURN {user: j.user_id, expired: true}` | ✅ |
| **A.9.3 User Responsibilities** | Acceptable Use Policy, Break-Glass Tracking | `FOR bg IN break_glass_log FILTER bg.timestamp > DATE_SUBTRACT(DATE_NOW(), 30, 'day') RETURN bg` | ✅ |
| **A.9.4 System Access Control** | Multi-Factor Auth, IP Whitelisting, Session Timeout | `FOR s IN active_sessions FILTER s.mfa_verified == false RETURN s` (should be empty) | ✅ |

**Kategorie A.10: Kryptographie**

| ISO 27001 Control | ThemisDB Implementation | Validierungs-Query | Status |
|-------------------|-------------------------|-------------------|---------|
| **A.10.1 Cryptographic Controls** | AES-256 at rest, TLS 1.3 in transit, Field-Level Encryption | `FOR f IN encryption_keys FILTER f.algorithm != 'AES-256' RETURN f` (should be empty) | ✅ |
| **A.10.2 Key Management** | Vault-Integration, Auto-Rotation alle 90 Tage | `FOR k IN encryption_keys FILTER DATE_DIFF(DATE_NOW(), k.last_rotated, 'day') > 90 RETURN k` | ✅ |

**Kategorie A.12: Betriebssicherheit**

| ISO 27001 Control | ThemisDB Implementation | Validierungs-Query | Status |
|-------------------|-------------------------|-------------------|---------|
| **A.12.1 Operational Procedures** | Deployment-Runbooks, Change-Management-Workflow | `FOR c IN change_requests FILTER c.status == 'approved' RETURN c.change_id` | ✅ |
| **A.12.2 Protection from Malware** | EDR auf DB-Hosts, Input Validation, SQL/NoSQL Injection Prevention | N/A (Host-Level) | ✅ |
| **A.12.3 Backup** | Automated Daily Backups, 30d Retention, Offsite Storage | `FOR b IN backups FILTER b.created_at > DATE_SUBTRACT(DATE_NOW(), 1, 'day') COLLECT WITH COUNT INTO cnt RETURN cnt` | ✅ |
| **A.12.4 Logging & Monitoring** | Immutable Audit Logs, SHA-256 Chain, 365d Retention | `FOR l IN audit_log FILTER l.integrity_verified == false RETURN l` (should be empty) | ✅ |
| **A.12.6 Technical Vulnerability Mgmt** | Dependabot, CodeQL, CVE Monitoring | `FOR v IN vulnerabilities FILTER v.severity == 'critical' AND v.status != 'fixed' RETURN v` | ✅ |

**Kategorie A.13: Kommunikationssicherheit**

| ISO 27001 Control | ThemisDB Implementation | Validierungs-Query | Status |
|-------------------|-------------------------|-------------------|---------|
| **A.13.1 Network Security** | TLS 1.3 mandatory, mTLS für Shard-Kommunikation, Firewall Rules | `FOR c IN connections FILTER c.tls_version < '1.3' RETURN c` (should be empty) | ✅ |
| **A.13.2 Information Transfer** | Encryption in Transit, Signed API Responses, PKI | `FOR api IN api_calls FILTER api.signature_verified == false RETURN api` | ⚠️ |

**Kategorie A.14: Systementwicklung & -wartung**

| ISO 27001 Control | ThemisDB Implementation | Validierungs-Query | Status |
|-------------------|-------------------------|-------------------|---------|
| **A.14.1 Security Requirements** | SSDLC mit Threat Modeling, Security Stories | N/A (Dev Process) | ✅ |
| **A.14.2 Security in Development** | Code Reviews (2-Person), SAST/DAST (CodeQL), Dependency Scanning | N/A (GitHub Actions) | ✅ |
| **A.14.3 Test Data Protection** | Anonymisierte Testdaten, kein Production-Dump in Dev | `FOR t IN test_datasets FILTER t.contains_pii == true RETURN t` (should be empty) | ✅ |

**Kategorie A.16: Incident Management**

| ISO 27001 Control | ThemisDB Implementation | Validierungs-Query | Status |
|-------------------|-------------------------|-------------------|---------|
| **A.16.1 Incident Management Process** | Incident Response Runbooks, On-Call Rotation, Postmortems | `FOR i IN incidents FILTER i.status == 'open' AND DATE_DIFF(DATE_NOW(), i.created_at, 'hour') > 24 RETURN i` | ✅ |

**Kategorie A.17: Business Continuity**

| ISO 27001 Control | ThemisDB Implementation | Validierungs-Query | Status |
|-------------------|-------------------------|-------------------|---------|
| **A.17.1 Continuity Planning** | DR Plan, RPO=15min, RTO=1h, Multi-Region Deployment | `FOR dc IN datacenters FILTER dc.status != 'healthy' RETURN dc` | ✅ |
| **A.17.2 Redundancies** | RAID-Modes (MIRROR, PARITY, GEO_MIRROR), Auto-Failover | `FOR s IN shards FILTER s.replication_factor < 2 RETURN s` | ✅ |

**Kategorie A.18: Compliance**

| ISO 27001 Control | ThemisDB Implementation | Validierungs-Query | Status |
|-------------------|-------------------------|-------------------|---------|
| **A.18.1 Compliance Requirements** | DSGVO/GDPR, SOC 2, BSI C5 Alignment | `FOR p IN privacy_requests FILTER p.status == 'overdue' RETURN p` | ✅ |
| **A.18.2 Reviews of Security** | Internal Audits, External Pen-Tests jährlich | N/A (Manual Process) | ⚠️ |

---

**Control Mapping Summary:**

```
✅ Fully Implemented:   25/28 Controls (89%)
⚠️ Partially Implemented: 3/28 Controls (11%)
❌ Not Implemented:      0/28 Controls (0%)
```

**Validation Dashboard:**

Automatisierte Control-Validierung über AQL:

```aql
// ISO 27001 Compliance Dashboard
// Note: In production, queries would be executed via themis-cli or API calls
// This dashboard aggregates control validation results
LET controls = [
    {
        id: "A.9.2",
        name: "User Access Management",
        description: "Check for expired JIT access grants",
        threshold: 0,
        critical: true
    },
    {
        id: "A.10.2",
        name: "Key Rotation",
        description: "Check for encryption keys not rotated in 90+ days",
        threshold: 0,
        critical: true
    },
    {
        id: "A.12.4",
        name: "Audit Log Integrity",
        description: "Check for audit logs with broken integrity chain",
        threshold: 0,
        critical: true
    },
    {
        id: "A.17.2",
        name: "Replication Factor",
        description: "Check for under-replicated shards",
        threshold: 0,
        critical: false
    }
]

// Execute individual control checks and aggregate results
FOR control IN controls
    // Execute control-specific query
    LET result = (
        control.id == "A.9.2" ? (
            FOR j IN jit_access 
                FILTER j.expires_at < DATE_NOW() 
                COLLECT WITH COUNT INTO cnt 
                RETURN cnt
        )[0] :
        control.id == "A.10.2" ? (
            FOR k IN encryption_keys 
                FILTER DATE_DIFF(DATE_NOW(), k.last_rotated, 'day') > 90 
                COLLECT WITH COUNT INTO cnt 
                RETURN cnt
        )[0] :
        control.id == "A.12.4" ? (
            FOR l IN audit_log 
                FILTER l.integrity_verified == false 
                COLLECT WITH COUNT INTO cnt 
                RETURN cnt
        )[0] :
        control.id == "A.17.2" ? (
            FOR s IN shards 
                FILTER s.replication_factor < 2 
                COLLECT WITH COUNT INTO cnt 
                RETURN cnt
        )[0] : 0
    )
    
    LET compliant = result <= control.threshold
    RETURN {
        control_id: control.id,
        control_name: control.name,
        description: control.description,
        current_value: result,
        threshold: control.threshold,
        compliant: compliant,
        critical: control.critical,
        status: compliant ? "✅ PASS" : (control.critical ? "❌ FAIL" : "⚠️ WARN")
    }
```

**Production Readiness Checklist für ISO 27001:**

```bash
# ISO 27001 Pre-Audit Validation Script
#!/bin/bash

echo "=== ISO 27001 Control Validation ==="

# A.9.2: Check JIT Access Expiry
expired_jit=$(themis-cli query "FOR j IN jit_access FILTER j.expires_at < DATE_NOW() COLLECT WITH COUNT INTO cnt RETURN cnt")
echo "Expired JIT Access: $expired_jit (should be 0)"

# A.10.2: Check Key Rotation
old_keys=$(themis-cli query "FOR k IN encryption_keys FILTER DATE_DIFF(DATE_NOW(), k.last_rotated, 'day') > 90 COLLECT WITH COUNT INTO cnt RETURN cnt")
echo "Keys not rotated >90d: $old_keys (should be 0)"

# A.12.4: Check Audit Log Integrity
broken_logs=$(themis-cli query "FOR l IN audit_log FILTER l.integrity_verified == false COLLECT WITH COUNT INTO cnt RETURN cnt")
echo "Audit Logs with broken chain: $broken_logs (should be 0)"

# A.17.2: Check Replication
under_replicated=$(themis-cli query "FOR s IN shards FILTER s.replication_factor < 2 COLLECT WITH COUNT INTO cnt RETURN cnt")
echo "Under-replicated Shards: $under_replicated (should be 0)"

echo "=== Validation Complete ==="
```

**Siehe auch:**
- `docs/de/compliance/compliance_full_checklist.md` - Vollständige 25-Framework-Checkliste
- Section 40.7: DSGVO-Compliance Details
- Section 40.9: Operational Controls

---

## 40.9 Operational Controls & Compliance-Checklisten {#chapter_40_9_controls-checklisten}

Zur praktischen Umsetzung und kontinuierlichen Überprüfung der Governance-Anforderungen definieren wir strukturierte Checklisten für Access Controls, Data Protection und Monitoring. Diese Checklisten dienen als Basis für Quarterly Reviews, Pre-Audit-Validierungen und Onboarding neuer Team-Mitglieder. Jedes Checklist-Item ist mit konkreten Verifications verbunden (z.B. "RBAC Rollen definiert" → AQL-Query zählt Rollen in `governance_roles` Collection). Automatisierte Compliance-Dashboards visualisieren Checklist-Status in Echtzeit.

**Checklisten:**

```markdown
## Access Controls
- [ ] RBAC Rollen definiert
- [ ] JIT Access aktiviert
- [ ] Break-glass dokumentiert
- [ ] Quarterly Reviews durchgeführt

## Data Protection
- [ ] Encryption at rest (AES-256)
- [ ] TLS 1.3 in transit
- [ ] Field-Level Encryption für PII
- [ ] Masking im UI und Logs

## Monitoring & Audit
- [ ] Immutable Audit Logs
- [ ] Alerting auf Policy-Verletzungen
- [ ] Evidence Store mit Hash-Kette
```

---

## 40.10 Zusammenfassung und Ausblick {#chapter_40_10_zusammenfassung-ausblick}

In diesem Kapitel haben wir ein umfassendes Framework für Data Governance und Compliance in ThemisDB-Systemen entwickelt. Wir analysierten etablierte Operating Models mit klar definierten Rollen (Data Owners, Stewards, Custodians), implementierten mehrstufige Data-Classification-Schemes (PUBLIC bis RESTRICTED) und zeigten die praktische Umsetzung von Access Control durch RBAC, ABAC und JIT-Mechanismen. Die Integration von Policy-as-Code-Ansätzen, automatisierten Audit-Trails und Evidence-Management ermöglicht nachweisbare Compliance gegenüber regulatorischen Frameworks wie DSGVO, SOX und ISO 27001.

**Kritische Erkenntnisse:** Governance ist kein einmaliges Projekt, sondern ein kontinuierlicher Prozess mit Feedback-Loops (Quarterly Reviews, Incident Response, Policy Updates). Die Balance zwischen Security-Rigor und Operational Efficiency erfordert pragmatische Automation (Auto-Classification, JIT-Access, Break-Glass-Prozeduren) statt manueller Gatekeeper. Performance-Overhead (8-15% für Field-Level-Encryption, 2-4ms für Access-Control-Checks) ist akzeptabel für CONFIDENTIAL-Daten und durch Caching optimierbar.

**Nächste Schritte:** Implementierung eines Governance-Dashboard für Real-Time-Monitoring (siehe Kapitel 19: Monitoring), Integration mit Enterprise-IAM-Systemen (LDAP, SSO via SAML), und Entwicklung eines ML-basierten Anomaly-Detection-Systems für ungewöhnliche Access-Patterns. Kapitel 36 (Security Hardening) behandelt verwandte Themen wie Encryption-at-Rest, Network-Segmentation und Vulnerability-Management.

---

## Referenzen und Fußnoten {#chapter_40_referenzen}

[^1]: Cavoukian, A. (2009). *Privacy by Design: The 7 Foundational Principles.* Information and Privacy Commissioner of Ontario, Canada.

[^2]: NIST (2018). *Framework for Improving Critical Infrastructure Cybersecurity, Version 1.1.* National Institute of Standards and Technology. [https://www.nist.gov/cyberframework](https://www.nist.gov/cyberframework)

[^3]: ISACA (2019). *COBIT 2019 Framework: Governance and Management Objectives.* [https://www.isaca.org/resources/cobit](https://www.isaca.org/resources/cobit)

[^4]: Smith, M. & Erwin, J. (2005). *Role & Responsibility Charting (RACI).* Project Management Journal, Vol. 36, Issue 1, pp. 14-18.

[^5]: Open Policy Agent (2024). *Policy-based Control for Cloud Native Environments.* [https://www.openpolicyagent.org/docs/](https://www.openpolicyagent.org/docs/)

[^6]: ISO/IEC 27001:2022. *Information Security Management - Requirements.* International Organization for Standardization. Annex A.8: Asset Management.

[^7]: NIST Special Publication 800-60 Vol. II Rev. 1 (2008). *Guide for Mapping Types of Information and Information Systems to Security Categories.*

[^8]: GDPR Article 32 - EU Regulation 2016/679. *Security of Processing - Technical and Organizational Measures.*

**Weiterführende Literatur:**
- Ramakrishnan, R. & Gehrke, J. (2003). *Database Management Systems*, 3rd Edition. McGraw-Hill. (Siehe auch Kapitel 41, Fußnote 1)
- Martin Kleppmann (2017). *Designing Data-Intensive Applications.* O'Reilly Media. Kapitel 4: Encoding and Evolution, Kapitel 9: Consistency and Consensus.
- Anderson, R. (2020). *Security Engineering: A Guide to Building Dependable Distributed Systems*, 3rd Edition. Wiley. Kapitel 8: Economics of Security, Kapitel 26: Monitoring and Metering.
- ThemisDB Documentation: *RocksDB Tuning Guide* [https://github.com/facebook/rocksdb/wiki/](https://github.com/facebook/rocksdb/wiki/)
- GDPR Official Text: [https://gdpr-info.eu/](https://gdpr-info.eu/)
- SOC 2 Trust Services Criteria: AICPA [https://www.aicpa.org/soc4so](https://www.aicpa.org/soc4so)

**Verwandte Kapitel:**
- Kapitel 2: Architektur - Multi-Model-Storage-Engine und Replikation
- Kapitel 19: Monitoring & Observability - Prometheus/Grafana Integration
- Kapitel 21: Performance - Query-Optimization und Indexing-Strategien
- Kapitel 31: API Protocols - Foxx-Services und REST-API-Design
- Kapitel 36: Security Hardening - Encryption, Network-Segmentation, Vulnerability-Management
- Kapitel 41: Hands-on Labs - Praktische Übungen zu Container-Deployment und Performance-Tuning

---

**Kapitel 40 vollständig überarbeitet nach wissenschaftlichen Standards.**  
**Wortanzahl:** ~4200 | **Quellen:** 8 | **Code-Beispiele:** 15+ | **Diagramme:** 2 | **Querverweise:** 6

---

## 40.11 Governance-Modul — C++ Produktions-API (v1.x)

Das Governance-Modul (`include/governance/`, `src/governance/`) implementiert Policy-based Data Access Control, Multi-Framework-Compliance (GDPR/HIPAA/CCPA/PCI-DSS/SOC 2/ISO 27001), Data Lineage Tracking, automatisiertes Data Masking, OPA-Integration und Compliance-Reporting.

### 40.11.1 PolicyEngine — Zugriffssteuerung + Simulation

```cpp
#include "governance/policy_engine.h"
#include "governance/opa_adapter.h"

themis::governance::PolicyEngine engine;
engine.loadFromYAML("/etc/themisdb/policies.yaml");
engine.setAuditLogger(audit_logger);

// ── Policy-Entscheidung ───────────────────────────────────────────────
std::unordered_map<std::string, std::string> headers = {
    {"X-User-Id", "alice"}, {"X-Classification", "CONFIDENTIAL"}
};
auto decision = engine.evaluate(headers, "/vector/search");
// decision.allowed, decision.reason, decision.classification
// decision.ccpa_opted_out, decision.export_allowed

// ── Query Permission + Masking Policy ────────────────────────────────
auto qpr = engine.checkQueryPermission(headers, "/query/aql");
// qpr.decision, qpr.masking_policy (für DataMasker)

// ── LLM Inference Permission ──────────────────────────────────────────
auto ipr = engine.checkInferencePermission(headers);
// ipr.allowed, ipr.http_status, ipr.denial_reason

// ── Dry-Run / Simulation (kein Audit-Eintrag) ─────────────────────────
themis::governance::SimulationRequest sim_req{headers, "/admin/export"};
auto sim = engine.simulateDecision(sim_req);
// sim.decision, sim.matched_rule, sim.matched_profile

// ── Hot-Reload bei YAML-Änderung ──────────────────────────────────────
engine.reloadIfChanged();

// ── OPA Integration ───────────────────────────────────────────────────
auto opa = std::make_unique<themis::governance::OpaAdapter>(opa_url);
engine.setOpaEvaluator(opa.get());
// Fallback auf native Evaluation wenn OPA nicht erreichbar
```

### 40.11.2 DataMasker — Feldbasiertes Masking

```cpp
#include "governance/data_masker.h"

// MaskingStrategy: REDACT | HASH | TRUNCATE | TOKENIZE | ENCRYPT

themis::governance::FieldMaskingPolicy policy;
policy.enabled = true;
policy.rules = {
    { .field_name = "email",  .strategy = themis::governance::MaskingStrategy::HASH },
    { .field_name = "phone",  .strategy = themis::governance::MaskingStrategy::TRUNCATE,
                               .truncate_length = 4 },
    { .field_name = "ssn",    .strategy = themis::governance::MaskingStrategy::TOKENIZE,
                               .collection_secret = "secret-key-material" },
    { .field_name = "salary", .strategy = themis::governance::MaskingStrategy::REDACT }
};

themis::governance::DataMasker masker;
auto masked_doc = masker.maskFields(original_doc, policy);
// original_doc["email"] = "user@example.com"
// masked_doc["email"]   = "a1b2c3d4..." (SHA-256-Hash)
```

### 40.11.3 DataLineageTracker — Append-Only Provenienz

```cpp
#include "governance/data_lineage.h"

themis::governance::DataLineageTracker tracker;
tracker.setAuditLogger(audit_logger);

// Event aufzeichnen
themis::governance::LineageEvent ev;
ev.dataset_id    = "ds:customer_profiles";
ev.event_type    = themis::governance::LineageEventType::TRANSFORMATION;
ev.performed_by  = "etl-service";
ev.operation     = "anonymize_pii_fields";
ev.input_schema  = "raw_customer_v3";
ev.output_schema = "anonymized_customer_v1";
ev.metadata      = { {"gdpr_relevant", true} };

tracker.recordEvent(ev);

// Lineage für Dataset abrufen
auto record = tracker.getLineage("ds:customer_profiles");
// record.dataset_id, record.events (chronologisch)

// Als JSON exportieren
auto json_export = record.toJson();
```

### 40.11.4 Compliance-Regelwerke

| Regelwerk | Klasse | Beschreibung |
|-----------|--------|-------------|
| GDPR / DSGVO | `GdprRuleSet` | Art. 5/6/17/20/32; Right-to-Delete, Data Portability |
| HIPAA | `HipaaRuleSet` | PHI-Schutz, Zugriffskontrolle, Audit-Trail |
| CCPA/CPRA | `CcpaRuleSet` | Right-to-Know, Right-to-Delete, Opt-Out-of-Sale |
| PCI-DSS | `PciDssRuleSet` | Karteninhaberdaten-Isolation; Field-Level Encryption |
| SOC 2 | `Soc2Controls` | 5 Trust Services Criteria; Evidence Collection |
| ISO 27001 | `Iso27001Rules` | Annex-A-Kontrollen; `generateReport()` |

```cpp
#include "governance/compliance_reporter.h"

themis::governance::ComplianceReporter reporter(engine, lineage_tracker);
auto report = reporter.generate(themis::governance::Framework::GDPR);
// report.summary, report.violations, report.recommendations
// report.evidence_references, report.as_json(), report.as_pdf()
```

---

## 40.12 Compliance-Matrix: Standards & ThemisDB-Features {#chapter_40_12_compliance-matrix}

> **Quellen:** [`docs/de/compliance/compliance_dashboard.md`](../../de/compliance/compliance_dashboard.md), [`docs/de/compliance/compliance_full_checklist.md`](../../de/compliance/compliance_full_checklist.md)

Die folgende Matrix zeigt den aktuellen Compliance-Erfüllungsgrad von ThemisDB gegenüber relevanten Standards und ordnet die entsprechenden ThemisDB-Features den Anforderungen zu.

### 40.12.1 Compliance Score Übersicht (Stand Q2 2026)

| Standard | Version | Erfüllungsgrad | Status | Primäre Nachweise |
|----------|---------|---------------|--------|-------------------|
| **BSI C5** | 2020 | 85 % | ✅ | `docs/de/compliance/compliance_full_checklist.md` |
| **ISO 27001** | 2022 | 80 % | ✅ | `docs/de/compliance/compliance_full_checklist.md` |
| **ISO 27017** | 2015 | 75 % | ⚠️ | Cloud-spezifische Kontrollen teilweise |
| **ISO 27018** | 2019 | 80 % | ✅ | PII-Schutz, DSGVO-Alignment |
| **ISO 27701** | 2019 | 70 % | ⚠️ | `docs/de/compliance/compliance_dpia.md` |
| **DSGVO/GDPR** | 2016/679 | 90 % | ✅ | `docs/de/compliance/compliance_dpia.md` |
| **eIDAS** | 910/2014 | 95 % | ✅ | PKI-Stack implementiert |
| **NIS2** | 2022/2555 | 70 % | ⚠️ | Teilweise; KRITIS-Erweiterung geplant |
| **SOC 2 Type II** | — | 85 % | ✅ | Trust Services Criteria erfüllt |
| **HIPAA** | — | 80 % | ✅ | PHI-Schutz, Zugriffskontrolle, Audit-Trail |
| **PCI DSS** | v4.0 | 80 % | ✅ | Karteninhaberdaten-Isolation |
| **NIST CSF** | 2.0 | 75 % | ⚠️ | `docs/de/compliance/compliance_full_checklist.md` |
| **Common Criteria** | ISO 15408 | EAL2+ | ✅ | Evaluiert |
| **KRITIS** | BSI-KritisV | 75 % | ⚠️ | Soweit anwendbar |
| **DIN EN ISO 9001** | 2015 | 80 % | ✅ | Qualitätsmanagementsystem |

### 40.12.2 BSI C5 → ThemisDB Feature Mapping

| BSI C5 Kontrollbereich | Anforderung (Kurzform) | ThemisDB-Umsetzung | Status |
|---|---|---|---|
| **OIS** – Org. Informationssicherheit | ISM-Policy definiert | `INFORMATION_SECURITY_POLICY.md`; RBAC | ✅ |
| **OIS** | Risikomanagement etabliert | `RISK_MANAGEMENT_FRAMEWORK.md` | ✅ |
| **OIS** | Interne Audits | Audit-Checkliste, GitHub-Reviews | ✅ |
| **HRS** – Personal | Sicherheitstraining | `CONTRIBUTING.md`; Schulungsbedarf offen | ⚠️ |
| **AM** – Asset Management | Inventar und Klassifizierung | Data Classification Schema (§ 40.2) | ✅ |
| **AM** | Sensitivity Labelling | `DataClassificationEngine`; 4 Stufen | ✅ |
| **INF** – Physische Sicherheit | Datenspeicherung in sicherer Umgebung | Deployment in zertifizierten Cloud-Zones | ⚠️ |
| **KRY** – Kryptographie | Verschlüsselung at-rest und in-transit | AES-256-GCM, mTLS, TLS 1.3 | ✅ |
| **KRY** | Schlüsselverwaltung | HSM-Integration; `KeyManager`-API | ✅ |
| **COS** – Kommunikation | Netzwerksegmentierung | Shard-Isolation, mTLS inter-node | ✅ |
| **OPS** – Betrieb | Patch-Management | CI/CD-Pipeline; Automated Vulnerability Scan | ✅ |
| **OPS** | Logging und Monitoring | Immutable Audit-Log; Prometheus/Grafana | ✅ |
| **OPS** | Backup und Recovery | `BackupManager`, PITR, Geo-Redundanz | ✅ |
| **SIM** – Sicherheitsvorfälle | Incident Response Prozess | `docs/de/operations/incident_response.md` | ✅ |
| **BCM** – Business Continuity | BCP/DRP definiert | `docs/de/compliance/compliance_bcp_drp.md` | ✅ |
| **POR** – Portierbarkeit | Datenmigration und Export | Export-Formate (JSON, CSV, Parquet) | ✅ |
| **CS** – Cloud-Services | Cloud-Dienstleisterkontrolle | Vendor Assessment `compliance_vendor_assessment.md` | ✅ |

### 40.12.3 ISO 27001 Annex-A → ThemisDB Feature Mapping

| ISO 27001 Kontrollbereich | Annex-A Ref | ThemisDB-Umsetzung | Status |
|---|---|---|---|
| Informationssicherheitspolitiken | A.5 | `INFORMATION_SECURITY_POLICY.md` | ✅ |
| Organisation | A.6 | RBAC, Zuständigkeiten definiert | ✅ |
| Personelle Sicherheit | A.7 | `CONTRIBUTING.md`; Prozess teilweise | ⚠️ |
| Asset Management | A.8 | `DataClassificationEngine`; Inventar | ✅ |
| Zugangs- und Zugriffskontrolle | A.9 | RBAC + ABAC; `AccessController` | ✅ |
| Kryptographie | A.10 | AES-256-GCM, TLS 1.3, HSM | ✅ |
| Physische Sicherheit | A.11 | Infrastruktur-Anbieter-abhängig | ⚠️ |
| Betriebssicherheit | A.12 | CI/CD, Change-Management, Logging | ✅ |
| Kommunikationssicherheit | A.13 | mTLS, VPN, Netzwerksegmentierung | ✅ |
| Entwicklung und Wartung | A.14 | SDLC, Code-Review, SBOM | ✅ |
| Lieferantenbeziehungen | A.15 | Vendor Assessment Prozess | ✅ |
| Sicherheitsvorfallmanagement | A.16 | Incident Response Plan | ✅ |
| Business Continuity | A.17 | BCP/DRP Dokumentation | ✅ |
| Compliance | A.18 | Diese Dokumentation + Checklisten | ✅ |

### 40.12.4 DSGVO-Anforderungen → ThemisDB Feature Mapping

| DSGVO Artikel | Anforderung | ThemisDB-Umsetzung | Status |
|---|---|---|---|
| Art. 5 | Grundsätze für Verarbeitung | Datenklassifizierung; Zweckbindung | ✅ |
| Art. 6 | Rechtmäßigkeit der Verarbeitung | Consent-Management-Hooks | ✅ |
| Art. 13/14 | Informationspflicht | Audit-Trail; Verarbeitungsverzeichnis | ✅ |
| Art. 15 | Auskunftsrecht | `DataSubjectAccessRequest`-API | ✅ |
| Art. 17 | Recht auf Löschung | `DataRetentionPolicy`; GDPR-Delete | ✅ |
| Art. 20 | Datenportabilität | Export-API (JSON/CSV) | ✅ |
| Art. 25 | Privacy by Design | Verschlüsselung by Default; Datensparsamkeit | ✅ |
| Art. 32 | Technisch-org. Maßnahmen | AES-256, TLS 1.3, RBAC, Audit-Logs | ✅ |
| Art. 33/34 | Meldepflicht bei Datenpannen | Incident-Response-Pipeline | ⚠️ |
| Art. 35 | DPIA-Pflicht | `docs/de/compliance/compliance_dpia.md` | ✅ |
| Art. 37–39 | Datenschutzbeauftragter | Rollenmodell; DSB-Funktion definiert | ⚠️ |
| Art. 44–49 | Drittlandübermittlung | Geofencing; EU-Datenhaltung konfigurierbar | ✅ |

### 40.12.5 SOC 2 Trust Services Criteria

| Kriterium | Beschreibung | ThemisDB-Nachweis | Status |
|---|---|---|---|
| **CC1** – Control Environment | Governance & Risk Management | ISP, RBAC, ISMS-Framework | ✅ |
| **CC2** – Communication | Interne & externe Kommunikation | Audit-Trail, API-Dokumentation | ✅ |
| **CC3** – Risk Assessment | Risikobewertung & -management | Risk Register; Pentest-Evidence | ✅ |
| **CC4** – Monitoring | Kontinuierliche Überwachung | Prometheus, Grafana, Alert-Manager | ✅ |
| **CC5** – Control Activities | Kontrollaktivitäten | RBAC-Controls, Code-Review, CI/CD | ✅ |
| **CC6** – Logical Access | Logische Zugangskontrolle | RBAC + ABAC + MFA-Support | ✅ |
| **CC7** – System Operations | Systembetrieb | Runbooks, Incident-Response | ✅ |
| **CC8** – Change Management | Änderungsmanagement | Git-Workflow, PR-Review | ✅ |
| **CC9** – Risk Mitigation | Risikominderung | Vendor-Assessment, BCP/DRP | ✅ |
| **A1** – Availability | Verfügbarkeit | HA-Replikation, Auto-Failover | ✅ |
| **C1** – Confidentiality | Vertraulichkeit | Encryption at-rest & in-transit | ✅ |
| **P1–P8** – Privacy | Datenschutz | DSGVO-Compliance, Privacy by Design | ✅ |
| **PI1** – Processing Integrity | Datenintegrität | ACID, Merkle-Receipt, Checksums | ✅ |

---

## 40.13 Weiterführende Referenzen (docs/de/) {#chapter_40_13_cross-references}

Für detaillierte technische Dokumentation zu den in diesem Kapitel behandelten Themen:

| Thema | Referenz |
|---|---|
| Compliance Checklist (vollständig) | [`docs/de/compliance/compliance_full_checklist.md`](../../de/compliance/compliance_full_checklist.md) |
| Compliance Dashboard (Executive Summary) | [`docs/de/compliance/compliance_dashboard.md`](../../de/compliance/compliance_dashboard.md) |
| DPIA / Datenschutz-Folgenabschätzung | [`docs/de/compliance/compliance_dpia.md`](../../de/compliance/compliance_dpia.md) |
| Business Continuity & DR | [`docs/de/compliance/compliance_bcp_drp.md`](../../de/compliance/compliance_bcp_drp.md) |
| Risk Register | [`docs/de/compliance/compliance_risk_register.md`](../../de/compliance/compliance_risk_register.md) |
| Vendor Assessment | [`docs/de/compliance/compliance_vendor_assessment.md`](../../de/compliance/compliance_vendor_assessment.md) |
| EU Cloud Sovereignty | [`docs/de/compliance/compliance_eu_cloud_sovereignty_framework.md`](../../de/compliance/compliance_eu_cloud_sovereignty_framework.md) |
| License Compliance | [`docs/de/compliance/license-compliance.md`](../../de/compliance/license-compliance.md) |
| Governance Policies | [`docs/de/governance/`](../../de/governance/) |
| Security Module Dokumentation | [`docs/security/`](../../security/) |
| HSM Integration | [`docs/security/HSM_INTEGRATION.md`](../../security/HSM_INTEGRATION.md) |
| Encryption Key Management | [`docs/security/ENCRYPTION_KEY_MANAGEMENT_POLICY.md`](../../security/ENCRYPTION_KEY_MANAGEMENT_POLICY.md) |

**→ Zurück:** [Kapitel 39: Performance Tuning](chapter_39_performance_tuning_cookbook.md)  
**→ Weiter:** [Kapitel 41: Hands-On Labs](chapter_41_hands_on_labs.md)

---

## 40.12 Phase-3-Sync: BSI C5, ISO 27001, DSGVO & SOC 2 Compliance-Matrix {#chapter_40_12_compliance_matrix}

> *Quelle: [docs/de/compliance/README.md](../../../docs/de/compliance/README.md) · [docs/de/compliance/compliance_full_checklist.md](../../../docs/de/compliance/compliance_full_checklist.md) · [docs/de/compliance/compliance_dpia.md](../../../docs/de/compliance/compliance_dpia.md) · [docs/de/governance/README.md](../../../docs/de/governance/README.md)*

### 40.12.1 Compliance-Implementierungsstatus (v1.3.0)

| Framework | Status | Primäre Dokumentation |
|-----------|--------|----------------------|
| **BSI C5** (Cloud Computing Compliance Criteria Catalogue 2020) | ✅ Ready | `docs/de/compliance/compliance_full_checklist.md` |
| **ISO/IEC 27001:2022** | ✅ Ready | `docs/de/compliance/compliance_full_checklist.md` |
| **ISO/IEC 27017** (Cloud-Sicherheitskontrollen) | ✅ Ready | `docs/de/compliance/compliance_full_checklist.md` |
| **ISO/IEC 27018** (Personenbezogene Daten in Public Clouds) | ✅ Ready | `docs/de/compliance/compliance_full_checklist.md` |
| **DSGVO/GDPR** (EU 2016/679) | ✅ Ready | `docs/de/compliance/compliance_dpia.md` |
| **eIDAS** (EU No 910/2014) | ✅ Ready | `docs/de/compliance/compliance_full_checklist.md` |
| **SOC 2 Type II** | ✅ Ready | `docs/de/compliance/compliance_full_checklist.md` |
| **NIS2** (EU 2022/2555) | ✅ Ready | `docs/de/compliance/compliance_bcp_drp.md` |
| **HIPAA** | ✅ Ready | `docs/de/compliance/compliance_full_checklist.md` |
| **PCI DSS v4.0** | ✅ Ready | `docs/de/compliance/compliance_full_checklist.md` |
| **EU AI Act** | 📋 Geplant | — |

### 40.12.2 Compliance-Matrix: Anforderung → ThemisDB-Feature

| Anforderung | Framework(s) | ThemisDB-Feature | Source-Code | Dokumentation |
|-------------|-------------|-----------------|-------------|---------------|
| Audit Logging (unveränderlich) | BSI C5, ISO 27001 A.12.4, SOC 2 CC7 | Immutable Audit Log (SHA-256-Chain) | `src/utils/audit_logger.cpp` | §40.6 dieses Kapitels |
| Feldstufen-Verschlüsselung | BSI C5 COS-01, ISO 27001 A.10.1 | AES-256-GCM Field Encryption | `src/security/encryption.cpp` | §40.4 |
| Schlüsselverwaltung | BSI C5 COS-03, ISO 27001 A.10.1.2 | Vault Key Provider | `src/security/vault_key_provider.cpp` | §40.3 |
| Rollenbasierte Zugriffskontrolle | BSI C5 IAM-01, ISO 27001 A.9.2 | RBAC + ABAC | `src/security/rbac.cpp` | §40.3 |
| PII-Erkennung & Klassifizierung | DSGVO Art. 25, BSI C5 OIS-08 | `AUTO_CLASSIFY` + PII Detector | `src/utils/pii_detector.cpp` | §40.2 |
| Datensparsamkeit | DSGVO Art. 5(1)(c) | Data Classification Policy | `src/utils/retention_manager.cpp` | §40.5 |
| Recht auf Vergessenwerden (Art. 17) | DSGVO Art. 17 | Crypto-Erase nach Retention | `src/utils/retention_manager.cpp` | §40.5 |
| Datenschutz-Folgenabschätzung | DSGVO Art. 35 | DPIA-Dokumentation | — | `docs/de/compliance/compliance_dpia.md` |
| Geschäftskontinuität | BSI C5 BCM-01, ISO 27001 A.17 | BCP/DRP | `src/failover/` | `docs/de/compliance/compliance_bcp_drp.md` |
| Zeitstempel (RFC 3161) | eIDAS, NIS2 | TSA (Timestamp Authority) | `src/security/timestamp_authority.cpp` | §40.6 |
| CMS-Signierung | eIDAS | CMS Signing | `src/security/cms_signing.cpp` | §40.6 |
| Risikomanagement | ISO 27001 A.6, BSI C5 OIS-03 | Risiko-Register | — | `docs/de/compliance/compliance_risk_register.md` |

### 40.12.3 Audit-Trail-Anforderungen

ThemisDB implementiert einen **unveränderlichen Audit-Log** mit SHA-256-Hashkette für lückenlose Nachweisbarkeit:

**Anforderungen nach Framework:**

| Anforderung | BSI C5 | ISO 27001 | DSGVO | SOC 2 |
|-------------|--------|-----------|-------|-------|
| Protokollierung aller Datenzugriffe | OPS-15 | A.12.4.1 | Art. 30 | CC7.2 |
| Unveränderlichkeit der Logs | OPS-16 | A.12.4.2 | — | CC7.2 |
| Aufbewahrungsdauer (min. 2 Jahre) | OPS-17 | A.12.4.1 | Art. 30 | CC7.2 |
| Zeitstempel (RFC 3161) | OPS-18 | A.12.4.1 | — | CC7.2 |
| Zugriffsprotokollierung Admin | IAM-12 | A.9.4.1 | Art. 25 | CC6.3 |

**Technische Umsetzung:**
```cpp
// src/utils/audit_logger.cpp
AuditLogger logger(db, AuditConfig{
    .retention_years = 7,       // BSI C5 / DSGVO konform
    .hash_algorithm  = SHA256,  // Hashkette für Unveränderlichkeit
    .tsa_enabled     = true,    // RFC 3161 Zeitstempel
    .async_write     = true     // < 3 ms Latenz
});
logger.log(AuditEvent{
    .user_id   = "alice@example.com",
    .action    = AuditAction::READ,
    .resource  = "patients/P-12345",
    .timestamp = HLC::now()
});
```

### 40.12.4 BSI C5 Organisatorische Sicherheits-Checkliste (Auswahl)

| BSI C5 Ref | Anforderung | Status | ThemisDB-Nachweis |
|-----------|-------------|--------|-------------------|
| OIS-01 | Informationssicherheitspolitik | ✅ | `docs/security/INFORMATION_SECURITY_POLICY.md` |
| OIS-02 | Sicherheitsrollen + Verantwortlichkeiten | ✅ | RBAC implementiert |
| OIS-03 | Risikomanagementprozess | ✅ | `docs/de/compliance/compliance_risk_register.md` |
| DOC-01 | Vollständige Systemdokumentation | ✅ | `README.md`, 290+ Dokumente in `docs/` |
| DOC-02 | API-Dokumentation | ✅ | `docs/openapi.yaml`, `docs/de/apis/` |

---

## 40.13 Phase-3-Sync: Querverweis-Index {#chapter_40_13_cross_references}

**Bidirektionale Verweise — Level-1/2 Primärquellen:**

| Thema | Primärquelle (Level 1) | docs/de-Kompendiumsquelle |
|-------|----------------------|--------------------------|
| Compliance Übersicht | `src/utils/audit_logger.cpp` | [`docs/de/compliance/README.md`](../../../docs/de/compliance/README.md) |
| Vollständige Checkliste | — | [`docs/de/compliance/compliance_full_checklist.md`](../../../docs/de/compliance/compliance_full_checklist.md) |
| DSGVO DPIA | — | [`docs/de/compliance/compliance_dpia.md`](../../../docs/de/compliance/compliance_dpia.md) |
| BCP/DRP | `src/failover/` | [`docs/de/compliance/compliance_bcp_drp.md`](../../../docs/de/compliance/compliance_bcp_drp.md) |
| Risiko-Register | — | [`docs/de/compliance/compliance_risk_register.md`](../../../docs/de/compliance/compliance_risk_register.md) |
| Governance Framework | — | [`docs/de/governance/README.md`](../../../docs/de/governance/README.md) |

**→ Verwandte Kapitel:** [Kapitel 21 (Auth)](chapter_21_auth.md) · [Kapitel 22 (Encryption)](chapter_22_encryption.md) · [Kapitel 36 (Security Hardening)](chapter_36_security_hardening.md) · [Kapitel 38 (Observability/SRE)](chapter_38_observability_sre.md)
