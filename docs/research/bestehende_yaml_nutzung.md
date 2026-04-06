# Bestehende YAML-Nutzung in ThemisDB

**Stand:** 6. April 2026  
**Version:** 1.0  
**Kategorie:** 🔍 Research

---

## Übersicht

ThemisDB nutzt bereits an vielen Stellen YAML als deklarative Konfigurationssprache, sowohl für externe Schnittstellen (PII-Erkennung, Compliance) als auch für interne Konfigurationen (Server, Indizes, Sharding). Dieses Dokument erfasst die bestehende YAML-Nutzung und zeigt, wie diese als Grundlage für erweiterte Schema-Definitionen dienen kann.

---

## Externe YAML-Nutzung

### 1. PII (Personally Identifiable Information) Patterns

**Datei:** `config/pii_patterns.yaml`

**Zweck:** Deklarative Definition von Mustern zur Erkennung personenbezogener Daten

**Struktur:**
```yaml
version: "1.0"

detection_engines:
  - type: "regex"
    enabled: true
    version: "1.0.0"
    
    patterns:
      - name: EMAIL
        regex: '[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}'
        confidence: 0.95
        redaction_mode: "partial"
        field_hints:
          - "email"
          - "e_mail"
          - "mail"
      
      - name: PHONE
        regex: '(?:\+\d{1,3}[\-.\s]?)?(?:\(\d{2,4}\)?[\-.\s]?)?[\d\-.\s]{7,15}'
        confidence: 0.85
        field_hints:
          - "phone"
          - "telephone"
      
      - name: SSN
        regex: '\b\d{3}\-?\d{2}\-?\d{4}\b'
        confidence: 0.98
        redaction_mode: "strict"
      
      - name: CREDIT_CARD
        regex: '(?:\b|^)[3456][0-9]{3}(?:-[0-9]{4}){3}(?:\b|$)'
        validation: "luhn"
        confidence: 0.90
```

**Features:**
- ✅ Plugin-basierte Architektur
- ✅ PKI-Signaturen für Konfigurationen
- ✅ Runtime Reload mit Validierung
- ✅ Fallback zu sicheren Defaults
- ✅ Field hints für kontextbasierte Erkennung

### 2. Retention Policies (GDPR/eIDAS Compliance)

**Datei:** `config/retention_policies.yaml`

**Zweck:** Deklarative Datenlöschungsrichtlinien gemäß GDPR/eIDAS

**Struktur:**
```yaml
global:
  enabled: true
  check_interval_hours: 24
  default_retention_days: 2555  # 7 Jahre
  audit_enabled: true

policies:
  - name: "user_personal_data"
    description: "Personal data subject to GDPR right to erasure"
    retention_days: 1095  # 3 Jahre
    archive_days: 730     # 2 Jahre
    auto_purge: false
    legal_basis: "GDPR Art. 6(1)(b) - Contract performance"
    categories:
      - "user_profiles"
      - "contact_information"
    compliance:
      - "GDPR"
      - "DSGVO"
  
  - name: "transaction_logs"
    retention_days: 3650  # 10 Jahre (HGB §147)
    archive_days: 2555
    auto_purge: true
    legal_basis: "HGB §147, AO §147"
    categories:
      - "invoices"
      - "payment_records"
    compliance:
      - "HGB"
      - "AO"
      - "GoBD"

archive:
  storage_path: "./data/archive"
  compression:
    enabled: true
    algorithm: "zstd"
  encryption:
    enabled: true
    algorithm: "AES-256-GCM"

purge:
  strategy: "batch"
  batch_size: 1000
  secure_delete:
    enabled: true
    overwrite_passes: 3
```

**Features:**
- ✅ GDPR/eIDAS-konforme Löschfristen
- ✅ Automatisierte Archivierung
- ✅ Sichere Löschung (DoD 5220.22-M)
- ✅ Audit Trail für alle Aktionen
- ✅ Legal Holds für Ermittlungen

### 3. Ethical Guidelines (KI-Ethik)

**Datei:** `config/ethical_guidelines.yaml`

**Zweck:** Ethische Richtlinien für LLM-Antworten basierend auf UN-Menschenrechten

**Struktur:**
```yaml
foundation:
  basis: "Universal Declaration of Human Rights (UN, 1948)"
  key_articles:
    - article: 1
      text_de: "Alle Menschen sind frei und gleich an Würde"
      relevance: "AI respecting human dignity and autonomy"
    
    - article: 18
      text_de: "Gedanken-, Gewissens- und Religionsfreiheit"
      relevance: "AI must not impose moral views"

core_principles:
  - id: "human_autonomy"
    description: "Die KI unterstützt Entscheidungen, ersetzt sie nie"
    priority: 1
    basis: "Asimov's Second Law (adapted)"
  
  - id: "no_patronizing"
    description: "Präsentiert Fakten, gibt keine Befehle"
    priority: 1
  
  - id: "respect_for_moral_diversity"
    description: "Verschiedene moralische Perspektiven respektieren"
    priority: 1

context_detection:
  ethical_keywords:
    german:
      - "ethisch"
      - "moralisch"
      - "Gewissen"
      - "Entscheidung"
    english:
      - "ethical"
      - "moral"
      - "conscience"
      - "decision"

prompt_augmentation:
  default:
    system_prefix: |
      GRUNDLAGE: Menschenrechte (UN, 1948)
      
      1. Menschliche Autonomie respektieren
      2. Keine Bevormundung bei moralischen Imperativen
      3. Moralische Vielfalt anerkennen
```

**Features:**
- ✅ Basierend auf UN-Menschenrechten
- ✅ Asimov's Robotergesetze (angepasst)
- ✅ Kontext-Erkennung via Keywords
- ✅ LLM-as-Judge für implizite Ethik-Erkennung
- ✅ Automatische Prompt-Augmentierung

### 4. Dokumenten-Metadaten-Schema

**Datei:** `projects/Themis.DocumentManager/Config/metadata_dokument.yaml`

**Zweck:** Strukturierte Metadaten für Dokumente im Verwaltungssystem

**Struktur:**
```yaml
entityType: Dokument
version: 1.0
description: "Metadatenschema für offizielle Dokumente"

fields:
  - name: dokumentnummer
    displayName: "Dokumentnummer"
    type: Text
    required: true
    description: "Eindeutige Dokumentnummer"
  
  - name: dokumenttyp
    displayName: "Dokumenttyp"
    type: Dropdown
    required: true
    options:
      - "Vertrag"
      - "Rechnung"
      - "Protokoll"
      - "Bericht"
  
  - name: ausstellDatum
    displayName: "Ausstellungsdatum"
    type: DateTime
    required: true
  
  - name: gültigkeitsstatus
    displayName: "Gültigkeitsstatus"
    type: Dropdown
    options:
      - "Draft"
      - "For Signature"
      - "Signed"
      - "Expired"
      - "Revoked"
  
  - name: klassifizierung
    displayName: "Klassifizierung"
    type: Dropdown
    options:
      - "Public"
      - "Internal"
      - "Confidential"
      - "Restricted"
```

**Features:**
- ✅ Typisierte Felder (Text, DateTime, Dropdown, MultiSelect)
- ✅ Validierung (required, readonly)
- ✅ Mehrsprachige Display-Namen
- ✅ Vordefinierte Optionen für Dropdowns
- ✅ Verwaltungsspezifische Felder (Aussteller, Unterzeichner)

---

## Interne YAML-Nutzung

### 1. Server-Hauptkonfiguration

**Datei:** `config/config.yaml`

**Zweck:** Zentrale Server-Konfiguration

**Struktur:**
```yaml
storage:
  rocksdb_path: "./data/rocksdb"
  wal_dir: ""
  memtable_size_mb: 256
  block_cache_mb: 512
  
server:
  host: "0.0.0.0"
  port: 8080
  binary_port: 18765
  max_connections: 1000
  
llm:
  enabled: true
  model_path: "./models/llama-2-7b-chat.gguf"
  context_size: 4096
  threads: 8
  
security:
  tls_enabled: true
  cert_file: "/certs/server.crt"
  key_file: "/certs/server.key"
```

### 2. Kubernetes Custom Resource Definition (CRD)

**Datei:** `deploy/kubernetes/crds/themisdb.vcc.io_themisdbs.yaml`

**Zweck:** Kubernetes-Operator für deklarative ThemisDB-Deployments

**Struktur:**
```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: themisdbs.vcc.io
spec:
  group: vcc.io
  names:
    kind: ThemisDB
    plural: themisdbs
    shortNames:
      - tdb
  versions:
    - name: v1alpha1
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              properties:
                replicas:
                  type: integer
                  minimum: 1
                  maximum: 99
                version:
                  type: string
                  default: "latest"
                storage:
                  properties:
                    size:
                      type: string
                      default: "100Gi"
                sharding:
                  properties:
                    enabled:
                      type: boolean
                      default: false
                    shards:
                      type: integer
                      default: 3
```

**Features:**
- ✅ GitOps-kompatibel (kubectl apply)
- ✅ Validierung via OpenAPI Schema
- ✅ Default-Werte
- ✅ Min/Max-Constraints
- ✅ Declarative Sharding Configuration

### 3. NLP-Konfiguration

**Datei:** `config/nlp/nlp_config.yaml`

**Zweck:** Natural Language Processing Konfiguration

**Struktur:**
```yaml
stopwords:
  enabled: true
  languages:
    - de: config/nlp/stopwords/de.yaml
    - en: config/nlp/stopwords/en.yaml
    - fr: config/nlp/stopwords/fr.yaml

tokenization:
  method: "unicode"
  lowercase: true

stemming:
  enabled: true
  algorithm: "porter"
```

### 4. LLM-Modell-Konfiguration

**Datei:** `config/llm-models.yaml`

**Zweck:** Verfügbare LLM-Modelle und deren Parameter

**Struktur:**
```yaml
models:
  - id: "llama-2-7b"
    name: "Llama 2 7B Chat"
    path: "./models/llama-2-7b-chat.gguf"
    type: "gguf"
    context_size: 4096
    default_temperature: 0.7
    capabilities:
      - "chat"
      - "completion"
  
  - id: "codellama-13b"
    name: "CodeLlama 13B"
    path: "./models/codellama-13b.gguf"
    type: "gguf"
    context_size: 16384
    capabilities:
      - "code_completion"
      - "code_review"
```

### 5. Sharding-Konfiguration

**Datei:** `config/sharding/shard-router-example.yaml`

**Zweck:** RAID-Sharding-Konfiguration

**Struktur:**
```yaml
sharding:
  enabled: true
  mode: "raid5"  # raid0, raid1, raid5, raid6
  
shards:
  - id: "shard-1"
    host: "localhost"
    port: 18766
    weight: 1.0
  
  - id: "shard-2"
    host: "localhost"
    port: 18767
    weight: 1.0
  
  - id: "shard-3"
    host: "localhost"
    port: 18768
    weight: 1.0

routing:
  strategy: "consistent_hashing"
  virtual_nodes: 150
```

### 6. OpenAPI-Spezifikation

**Datei:** `docs/openapi.yaml` und `openapi/openapi.yaml`

**Zweck:** REST-API-Definition

**Struktur:**
```yaml
openapi: 3.0.0
info:
  title: ThemisDB API
  version: 1.4.0
  description: Multi-Model Database with AI Integration

paths:
  /entities/{table}:{pk}:
    get:
      summary: Get entity by primary key
      parameters:
        - name: table
          in: path
          required: true
          schema:
            type: string
      responses:
        '200':
          description: Success
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/BaseEntity'
  
  /pii/classify:
    post:
      summary: Classify PII in text
      tags: [pii]
      requestBody:
        content:
          application/json:
            schema:
              type: object
              properties:
                text:
                  type: string
```

---

## Analyse: Muster und Best Practices

### Gemeinsame Muster

1. **Versionierung**
   ```yaml
   version: "1.0"
   ```
   - Alle Konfigurationen haben eine Version
   - Ermöglicht Migration und Kompatibilitätsprüfung

2. **Hierarchische Struktur**
   ```yaml
   global:
     ...
   policies:
     - name: ...
       settings: ...
   ```
   - Klare Trennung global/spezifisch
   - Verschachtelte Konfigurationen

3. **Typed Configuration**
   ```yaml
   - name: field_name
     type: Text | DateTime | Dropdown | Integer
     required: true | false
     default: "value"
   ```
   - Starke Typisierung
   - Validierungsregeln embedded

4. **Lists vs. Objects**
   ```yaml
   policies:         # List
     - name: policy1
       ...
   
   archive:          # Object
     storage_path: ...
     compression:
       enabled: true
   ```
   - Listen für mehrere gleichartige Elemente
   - Objekte für strukturierte Konfigurationen

5. **Beschreibungen und Kommentare**
   ```yaml
   # Human-readable comments
   description: "Machine-readable description"
   ```
   - Inline-Dokumentation
   - Self-documenting configuration

### Best Practices in ThemisDB

**1. Plugin-Architektur mit YAML**
```yaml
detection_engines:
  - type: "regex"
    enabled: true
    settings:
      ...
```
- Erweiterbar ohne Code-Änderungen
- Hot-Reload-fähig

**2. Compliance-First Design**
```yaml
legal_basis: "GDPR Art. 6(1)(b)"
compliance: ["GDPR", "DSGVO"]
```
- Gesetzesreferenzen direkt im Config
- Auditierbarkeit

**3. Security-by-Default**
```yaml
auto_purge: false  # Requires manual review
require_confirmation: true
```
- Sichere Defaults
- Explizite Opt-ins für kritische Features

**4. Mehrsprachigkeit**
```yaml
text_de: "Deutsche Beschreibung"
text_en: "English description"
```
- i18n direkt in Config
- Mehrsprachige Deployment-Optionen

---

## Erweiterungspotential

### 1. Schema-Definition (wie vorgeschlagen)

**Aktuell:** Dokumenten-Metadaten nutzen bereits YAML-Schemas  
**Erweiterung:** Auf Datenbank-Tabellen/Entitäten ausweiten

```yaml
# Bestehend (DocumentManager):
entityType: Dokument
fields:
  - name: dokumentnummer
    type: Text

# Vorgeschlagen (ThemisDB Core):
tables:
  documents:
    primary_key: doc_id
    fields:
      doc_id:
        type: string
        format: uuid
    indexes:
      - name: idx_doc_nummer
        columns: [doc_number]
        type: secondary
```

**Vorteil:** Konzept ist bereits etabliert und verstanden!

### 2. Index-Definition

**Aktuell:** Imperative API
```bash
curl -X POST /index/create -d '{"table":"users","column":"email"}'
```

**Vorgeschlagen:** Deklarativ via YAML
```yaml
indexes:
  users:
    - name: idx_users_email
      columns: [email]
      type: secondary
      unique: true
    
    - name: idx_users_embedding
      columns: [embedding]
      type: vector
      algorithm: hnsw
      config:
        m: 16
        ef_construction: 200
```

**Vorbild:** Sharding-Config nutzt bereits ähnliches Format

### 3. Migration-Files

**Aktuell:** Manuelles SQL/API
**Vorgeschlagen:** Git-ähnliche Migrations
```yaml
# migrations/001_add_users_table.yaml
version: "001"
up:
  - create_table:
      name: users
      columns:
        - name: user_id
          type: string
down:
  - drop_table:
      name: users
```

**Vorbild:** Kubernetes CRD Updates funktionieren ähnlich

### 4. Policies as Code

**Aktuell:** retention_policies.yaml (extern)  
**Erweiterung:** Inline table policies

```yaml
tables:
  user_sessions:
    ttl:
      enabled: true
      column: created_at
      duration: 30d
    
    pii_detection:
      enabled: true
      fields: [email, phone]
    
    audit:
      enabled: true
      track_changes: true
```

**Vorbild:** Ethical Guidelines nutzen bereits Policy-Pattern

---

## Zusammenfassung

### ✅ ThemisDB nutzt YAML bereits umfangreich:

**Extern (Compliance & Security):**
- PII-Patterns mit Regex und Confidence
- Retention Policies (GDPR/eIDAS)
- Ethical Guidelines (UN-Menschenrechte)
- Dokumenten-Metadaten-Schemas

**Intern (Betrieb & Konfiguration):**
- Server-Konfiguration
- Kubernetes CRDs
- NLP-Konfiguration
- LLM-Modell-Definitionen
- Sharding-Router
- OpenAPI-Spezifikation

### 🚀 Erweiterungsmöglichkeiten:

1. **Schema-Definition** - Datenbank-Tabellen deklarativ definieren
2. **Index-Management** - Indizes via YAML statt API
3. **Migration-System** - Git-ähnliche Versionierung
4. **Policy-Integration** - TTL, PII, Audit direkt im Schema

### 💡 Lessons Learned:

- YAML ist bereits **etablierte Praxis** in ThemisDB
- **Plugin-Architektur** ermöglicht Hot-Reload
- **Compliance-First**: Gesetzesreferenzen im Config
- **Security-by-Default**: Sichere Defaults, Opt-in für kritische Features
- **Mehrsprachigkeit**: i18n direkt in Konfiguration
- **Self-Documenting**: Beschreibungen und Kommentare embedded

---

## Empfehlung

Die vorgeschlagene YAML-basierte Schema-Definition in [git_gitops_themis_vergleich.md](git_gitops_themis_vergleich.md) ist **nicht neu**, sondern eine **logische Erweiterung** bestehender Patterns:

1. **DocumentManager nutzt bereits YAML-Schemas** für Metadaten
2. **Kubernetes CRD zeigt** deklaratives Deployment funktioniert
3. **PII/Retention Policies** demonstrieren komplexe Regelwerke
4. **Plugin-Architektur** ist Hot-Reload-fähig

→ **Umsetzung ist evolutionär, nicht revolutionär!**

---

**Autoren:** ThemisDB Research Team  
**Erstellt:** 2026-01-14  
**Status:** Research Complete
