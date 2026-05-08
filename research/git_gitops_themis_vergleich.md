# Git, GitHub, GitOps im Vergleich zur Versionskontrolle von ThemisDB

**Stand:** 6. April 2026  
**Version:** v1.4.0  
**Kategorie:** 🧩 Architecture

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Git: Verteilte Versionskontrolle für Code](#git-verteilte-versionskontrolle-für-code)
- [GitHub: Kollaborationsplattform](#github-kollaborationsplattform)
- [GitOps: Deklarative Infrastruktur](#gitops-deklarative-infrastruktur)
- [ThemisDB MVCC: Versionskontrolle für Daten](#themisdb-mvcc-versionskontrolle-für-daten)
- [Konzeptvergleich](#konzeptvergleich)
- [YAML-Übernahme von Git](#yaml-übernahme-von-git)
- [Empfohlene Integrationen](#empfohlene-integrationen)
- [Zukunftsperspektiven](#zukunftsperspektiven)

---

## Übersicht

Dieses Dokument vergleicht die Versionskontroll- und Workflow-Konzepte von Git/GitHub/GitOps mit dem MVCC-System (Multi-Version Concurrency Control) von ThemisDB. Während Git für Code-Versionierung konzipiert ist, bietet ThemisDB MVCC für Datenbank-Transaktionen. Beide Systeme haben jedoch ähnliche Konzepte, die voneinander lernen können.

> **💡 Hinweis:** ThemisDB nutzt bereits YAML an vielen Stellen! Siehe [Bestehende YAML-Nutzung](bestehende_yaml_nutzung.md) für eine vollständige Analyse der aktuellen YAML-Konfigurationen (PII-Patterns, Retention Policies, Dokumenten-Metadaten, Kubernetes CRDs, etc.).

---

## Git: Verteilte Versionskontrolle für Code

### Kernkonzepte

**1. Commit-basierte Historie**
```bash
# Git speichert eine vollständige Historie aller Änderungen
git log --oneline
a1b2c3d feat: Add vector search
d4e5f6g fix: Query optimization
```

**Eigenschaften:**
- ✅ Vollständige Historie jeder Datei
- ✅ Branching und Merging
- ✅ Verteilte Architektur (jeder Clone ist vollständig)
- ✅ Content-addressable Storage (SHA-1/SHA-256 Hashes)
- ⚠️  Nicht für binäre Daten optimiert
- ⚠️  Keine gleichzeitigen Schreibzugriffe

### Git Branching Model

```
main (Production)
  |
  ├── release/1.4.0 (Release Vorbereitung)
  |    |
develop (Integration)
  |
  ├── feature/vector-search (Neue Features)
  ├── bugfix/query-fix (Bug Fixes)
```

**Merge-Strategien:**
- **Fast-Forward**: Lineare Historie
- **Merge Commit**: Branches zusammenführen
- **Squash**: Mehrere Commits zu einem zusammenfassen
- **Rebase**: Historie umschreiben

### Git Objekt-Modell

```
Commit
  ├── Tree (Verzeichnisstruktur)
  │    ├── Blob (Dateiinhalt)
  │    └── Blob (Dateiinhalt)
  ├── Parent Commit(s)
  └── Metadata (Author, Time, Message)
```

---

## GitHub: Kollaborationsplattform

### Workflow-Features

**1. Pull Requests (PRs)**
```yaml
# .github/workflows/pr-check.yml
name: PR Validation
on:
  pull_request:
    branches: [develop, main]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run tests
        run: npm test
```

**GitHub Flow:**
1. Branch erstellen
2. Commits hinzufügen
3. Pull Request öffnen
4. Code Review
5. CI/CD Checks
6. Merge nach Approval

**2. GitHub Actions (CI/CD)**
- Automatisierte Workflows
- Event-getrieben (Push, PR, Release)
- YAML-basierte Konfiguration
- Marketplace für wiederverwendbare Actions

**3. Issues & Project Management**
- Issue Tracking mit Labels
- Milestones und Projects
- Automatisierung via GitHub Actions

---

## GitOps: Deklarative Infrastruktur

### Kernprinzipien

**1. Deklarative Konfiguration**
```yaml
# kubernetes/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: themisdb
        image: themisdb:v1.4.0
        env:
        - name: CONFIG_FILE
          value: /config/themis.yaml
```

**2. Git als Single Source of Truth**
- Infrastructure as Code (IaC)
- Alle Änderungen via Git
- Auditierbare Historie
- Rollback über Git

**3. Automatische Synchronisation**
```
Git Repository (Desired State)
        ↓
    GitOps Operator (ArgoCD, Flux)
        ↓
  Kubernetes Cluster (Actual State)
```

**GitOps Workflow:**
1. Entwickler pusht YAML-Änderung zu Git
2. GitOps Tool detektiert Änderung
3. Automatisches Deployment
4. Continuous Reconciliation

---

## ThemisDB MVCC: Versionskontrolle für Daten

### Kernkonzepte

**1. Multi-Version Concurrency Control**
```cpp
// Transaktion startet mit Snapshot
auto txn = db.begin();  // Snapshot Version = T1

// Lesen: Sieht nur Daten bis T1
auto data = txn.get("users:alice");  

// Schreiben: Neue Version erstellen
txn.put("users:alice", new_data);    

// Commit: Atomare Persistierung
txn.commit();  // Commit Version = T2
```

**Eigenschaften:**
- ✅ Snapshot Isolation (konsistente Lesezugriffe)
- ✅ Concurrent Reads ohne Locks
- ✅ Write-Write Conflict Detection
- ✅ Atomare Multi-Index Transaktionen
- ✅ ACID-Garantien
- ⚠️  Höherer Speicherverbrauch (Versionen)
- ⚠️  Garbage Collection erforderlich

### MVCC Architektur

```
TransactionManager
  ├── begin() → Snapshot Version
  ├── get()   → Version-aware Read
  ├── put()   → Conflict Detection
  └── commit() → Atomic Persist

RocksDB TransactionDB
  ├── Pessimistic Locking
  ├── Snapshot Management
  └── WAL (Write-Ahead Log)
```

**Versioniertes Objekt-Modell:**
```cpp
Entity Version {
  primary_key: "users:alice"
  version_start: 42
  version_end: 100  // oder UINT64_MAX wenn aktiv
  data: {...}
}
```

### Index-Versionierung

**Alle Indizes unterstützen MVCC:**
- **Secondary Index**: Versionierte Einträge
- **Graph Index**: Versionierte Kanten
- **Vector Index**: Versionierte Embeddings
- **Fulltext Index**: Versionierte Dokumente

```cpp
// Atomare Index-Updates
auto txn = db.begin();
txn.put("users:alice", data);           // Primary
secIdx.put(table, entity, *txn);        // Secondary
graphIdx.addEdge(edge, *txn);           // Graph
vecIdx.addEntity(entity, *txn);         // Vector
txn.commit();  // Alles oder nichts
```

---

## Konzeptvergleich

### Ähnlichkeiten

| Konzept | Git | ThemisDB MVCC |
|---------|-----|---------------|
| **Versionierung** | Commit-basiert | Transaction-basiert |
| **Snapshot** | `git checkout <commit>` | Transaction Snapshot |
| **Branching** | Git Branches | Parallele Transaktionen |
| **Merge** | `git merge` | Transaction Commit |
| **Konflikt** | Merge Conflict | Write-Write Conflict |
| **Historie** | `git log` | Transaction Log / Changefeed |
| **Rollback** | `git reset` | Transaction Rollback |
| **Audit Trail** | Commit History | Audit Logging |

### Unterschiede

| Aspekt | Git | ThemisDB MVCC |
|--------|-----|---------------|
| **Ziel** | Code-Versionierung | Daten-Versionierung |
| **Granularität** | Datei/Zeile | Entity/Index |
| **Concurrent Writes** | Nicht unterstützt | Ja (mit Conflict Detection) |
| **Speichermodell** | Content-addressable | LSM-Tree (RocksDB) |
| **Merge-Strategie** | Manuell/Automatisch | Automatisch (Optimistic Locking) |
| **Garbage Collection** | `git gc` | Background GC (RocksDB Compaction) |
| **Verteilung** | Vollständig dezentral | Server-zentriert (mit Replication) |
| **Performance** | O(1) Read | O(log n) Read (LSM-Tree) |

### Konzeptmapping

```
Git Commit          ↔  ThemisDB Transaction
Git Branch          ↔  Concurrent Transaction
Git Merge           ↔  Transaction Commit
Git Conflict        ↔  Write-Write Conflict
Git SHA             ↔  Transaction Version Number
Git Tree            ↔  Entity Snapshot
Git Blob            ↔  Entity Data
Git HEAD            ↔  Latest Version
Git Tag             ↔  Named Snapshot
Git Remote          ↔  Replication Target
```

---

## YAML-Übernahme von Git

### 1. Deklarative Konfiguration (wie GitOps)

**Aktuell: Imperativ (API-Calls)**
```bash
# Manuelles Schema-Setup
curl -X POST /index/create \
  -d '{"table":"users","column":"email","type":"secondary"}'
```

**Vorschlag: Deklarativ (YAML)**
```yaml
# themis-schema.yaml
version: "1.0"
database: themisdb

tables:
  users:
    primary_key: user_id
    indexes:
      - column: email
        type: secondary
        unique: true
      - column: location
        type: geo
        srid: 4326
      - column: embedding
        type: vector
        dimensions: 384
        algorithm: hnsw
        
    constraints:
      - field: age
        type: range
        min: 0
        max: 150
      - field: email
        type: regex
        pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$"

  posts:
    primary_key: post_id
    relationships:
      - name: author
        target: users
        type: many_to_one
        foreign_key: author_id
```

**Anwendung:**
```bash
# Schema aus YAML laden
themis schema apply -f themis-schema.yaml

# Änderungen anzeigen
themis schema diff -f themis-schema.yaml

# Rollback
themis schema rollback --to-version v1.2.0
```

### 2. CI/CD Integration (wie GitHub Actions)

**Vorschlag: ThemisDB Actions**
```yaml
# .themis/workflows/data-validation.yml
name: Data Quality Check
on:
  entity_change:
    tables: [users, orders]
  
triggers:
  - type: pre_commit
    condition: table == "users"
    
jobs:
  validate_user:
    runs_on: themisdb
    steps:
      - name: Check email format
        aql: |
          MATCH (u:users)
          WHERE u.email NOT REGEX '^[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,}$'
          RETURN u.user_id, u.email
        on_failure: rollback
        
      - name: Check age range
        aql: |
          MATCH (u:users)
          WHERE u.age < 0 OR u.age > 150
          RETURN u.user_id, u.age
        on_failure: alert
        
      - name: Update materialized view
        aql: |
          CREATE OR REPLACE VIEW active_users AS
          MATCH (u:users)
          WHERE u.last_login > NOW() - INTERVAL '30 days'
          RETURN u.*
```

### 3. Versionierte Schemas (wie Git Commits)

**Vorschlag: Schema-Versionierung**
```yaml
# themis-migrations/001_initial_schema.yaml
version: "001"
description: "Initial schema for users and posts"
author: "developer@example.com"
timestamp: "2026-01-14T10:00:00Z"

up:
  - create_table:
      name: users
      columns:
        - name: user_id
          type: string
          primary_key: true
        - name: email
          type: string
        - name: created_at
          type: timestamp

down:
  - drop_table:
      name: users
```

```bash
# Migration anwenden
themis migrate up

# Migration rückgängig machen
themis migrate down

# Status anzeigen
themis migrate status
```

### 4. Branching-Strategie für Daten

**Vorschlag: Data Branches**
```yaml
# .themis/branches.yaml
branches:
  main:
    description: "Production data"
    protected: true
    auto_backup: true
    retention: 90d
    
  staging:
    description: "Staging environment"
    clone_from: main
    sync_interval: 1h
    
  feature/ml-training:
    description: "ML model training data"
    clone_from: main
    snapshot_at: "2026-01-01T00:00:00Z"
    read_only: true
```

**Workflow:**
```bash
# Branch erstellen
themis branch create feature/ml-training --from main

# Zu Branch wechseln
themis branch checkout feature/ml-training

# Änderungen committen
themis commit -m "Add training samples"

# Branch mergen
themis branch merge feature/ml-training --to staging
```

---

## Empfohlene Integrationen

### 1. GitOps für ThemisDB-Deployment

**Helm Chart + ArgoCD Integration**
```yaml
# values.yaml
replicaCount: 3

config:
  schema:
    source: git
    repository: https://github.com/org/themis-schemas.git
    path: schemas/production.yaml
    sync_interval: 5m
    
  backup:
    enabled: true
    schedule: "0 2 * * *"
    destination: s3://backups/themisdb
    
monitoring:
  prometheus:
    enabled: true
  grafana:
    enabled: true
```

**ArgoCD Application:**
```yaml
# argocd/themisdb-app.yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: themisdb
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/org/themis-infra.git
    path: kubernetes/themisdb
    targetRevision: main
  destination:
    server: https://kubernetes.default.svc
    namespace: themisdb
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
```

### 2. GitHub Actions für Schema-Validierung

```yaml
# .github/workflows/schema-validation.yml
name: ThemisDB Schema Validation

on:
  pull_request:
    paths:
      - 'schemas/**/*.yaml'

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install ThemisDB CLI
        run: |
          curl -L https://github.com/makr-code/ThemisDB/releases/latest/download/themis-cli-linux -o themis
          chmod +x themis
      
      - name: Validate Schema
        run: |
          ./themis schema validate schemas/production.yaml
      
      - name: Run Schema Tests
        run: |
          ./themis schema test schemas/production.yaml
      
      - name: Generate Schema Diff
        if: github.event_name == 'pull_request'
        run: |
          ./themis schema diff \
            --base origin/${{ github.base_ref }} \
            --head ${{ github.sha }} \
            --output schema-diff.md
      
      - name: Comment PR with Diff
        uses: actions/github-script@v6
        with:
          script: |
            const fs = require('fs');
            const diff = fs.readFileSync('schema-diff.md', 'utf8');
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.repo,
              body: `## Schema Changes\n\n${diff}`
            });
```

### 3. Change Data Capture (CDC) mit Git-Semantik

```yaml
# .themis/cdc.yaml
cdc:
  enabled: true
  
  streams:
    - name: user-changes
      tables: [users]
      format: git-patch
      destination: kafka://kafka:9092/user-changes
      
  format:
    type: git-diff
    include:
      - operation: insert
        format: |
          + user_id: {{.user_id}}
          + email: {{.email}}
          + created_at: {{.created_at}}
      
      - operation: update
        format: |
          ~ user_id: {{.user_id}}
          - email: {{.old.email}}
          + email: {{.new.email}}
      
      - operation: delete
        format: |
          - user_id: {{.user_id}}
          - email: {{.email}}
```

**Output-Beispiel:**
```diff
Commit: txn_0000000042
Author: app-server-01
Date: 2026-01-14 10:30:00 UTC

Update user profile

+ user_id: user_123
- email: old@example.com
+ email: new@example.com
~ updated_at: 2026-01-14 10:30:00
```

---

## Zukunftsperspektiven

### Roadmap: Git-inspirierte Features

**Phase 1: Schema Management (Q1 2026)**
- [ ] YAML-basierte Schema-Definition
- [ ] Schema-Versionierung und Migrations
- [ ] Schema-Diffing und Validation
- [ ] CLI-Tool für Schema-Management

**Phase 2: Workflow-Integration (Q2 2026)**
- [ ] GitHub Actions Integration
- [ ] GitOps-kompatibles Deployment
- [ ] Automatische Schema-Synchronisation
- [ ] CI/CD Pipelines für Datenbank-Änderungen

**Phase 3: Data Branching (Q3 2026)**
- [ ] Named Snapshots (wie Git Tags)
- [ ] Branch-ähnliche Datenviews
- [ ] Copy-on-Write Branches
- [ ] Branch-Merging mit Conflict Resolution

**Phase 4: Advanced Features (Q4 2026)**
- [ ] Time-Travel Queries (wie `git checkout <commit>`)
- [ ] Distributed Collaboration (wie Git Remotes)
- [ ] Change Proposals (wie Pull Requests für Daten)
- [ ] Auditierbare Daten-History (wie `git log`)

### Konzept-Prototypen

**1. Time-Travel Queries**
```aql
-- Daten zu einem bestimmten Zeitpunkt
AS OF TIMESTAMP '2026-01-01 00:00:00'
MATCH (u:users)
WHERE u.email = 'alice@example.com'
RETURN u.*

-- Änderungen zwischen zwei Zeitpunkten (wie git diff)
DIFF BETWEEN 
  TIMESTAMP '2026-01-01 00:00:00' 
  AND TIMESTAMP '2026-01-14 00:00:00'
FOR TABLE users
WHERE user_id = 'user_123'
```

**2. Data Pull Requests**
```yaml
# data-pr-001.yaml
title: "Update customer segmentation"
author: data-scientist@example.com
reviewers:
  - data-engineer@example.com
  - product-manager@example.com

changes:
  - table: customers
    operation: bulk_update
    affected_rows: 15000
    query: |
      UPDATE customers
      SET segment = 'premium'
      WHERE lifetime_value > 10000
      
validation:
  - check: referential_integrity
  - check: data_quality_rules
  - check: performance_impact
  
tests:
  - query: |
      SELECT COUNT(*) FROM customers WHERE segment = 'premium'
    expected: 15000
```

**3. Distributed Themis (wie Git Remotes)**
```bash
# Remote hinzufügen
themis remote add production themis://prod.example.com:18765

# Änderungen pushen
themis push production main

# Änderungen pullen
themis pull production main

# Remote synchronisieren
themis sync production --auto-merge
```

---

## Best Practices

### 1. Schema as Code
```
git-repo/
├── schemas/
│   ├── production.yaml
│   ├── staging.yaml
│   └── development.yaml
├── migrations/
│   ├── 001_initial.yaml
│   ├── 002_add_users.yaml
│   └── 003_add_indexes.yaml
└── .github/
    └── workflows/
        └── schema-validation.yml
```

### 2. Environment-Parity
```yaml
# base-schema.yaml (shared)
version: "1.0"
tables:
  users:
    primary_key: user_id

---
# production.yaml (overlay)
extends: base-schema.yaml
config:
  replication_factor: 3
  backup_enabled: true

---
# development.yaml (overlay)
extends: base-schema.yaml
config:
  replication_factor: 1
  backup_enabled: false
```

### 3. GitOps Workflow
```
1. Developer ändert schemas/production.yaml
2. Git Push → Pull Request
3. GitHub Actions validiert Schema
4. Code Review
5. Merge → Main Branch
6. ArgoCD detektiert Änderung
7. Automatisches Schema-Update in ThemisDB
8. Monitoring & Alerting
```

---

## Vergleichstabelle: Feature-Matrix

| Feature | Git | GitHub | GitOps | ThemisDB MVCC | Empfohlen |
|---------|-----|--------|--------|---------------|-----------|
| **Versionierung** | ✅ | ✅ | ✅ | ✅ | - |
| **YAML-Konfiguration** | ❌ | ✅ | ✅ | ❌ | ✅ Implementieren |
| **Branching** | ✅ | ✅ | ❌ | 🟡 (Parallel Txn) | ✅ Data Branches |
| **Merge/Conflict** | ✅ | ✅ | ❌ | ✅ | - |
| **CI/CD Integration** | ❌ | ✅ | ✅ | ❌ | ✅ Implementieren |
| **Pull Requests** | ❌ | ✅ | ❌ | ❌ | 🟡 Data PRs (Future) |
| **Audit Trail** | ✅ | ✅ | ✅ | ✅ | - |
| **Time-Travel** | ✅ | ❌ | ❌ | 🟡 (PITR) | ✅ Erweitern |
| **Distributed** | ✅ | ✅ | ❌ | 🟡 (Replication) | 🟡 Multi-Master |
| **Deklarativ** | ❌ | ❌ | ✅ | ❌ | ✅ Schema YAML |
| **Automated Sync** | ❌ | ❌ | ✅ | ❌ | ✅ GitOps Mode |

**Legende:**
- ✅ Vollständig unterstützt
- 🟡 Teilweise unterstützt
- ❌ Nicht unterstützt

---

## Zusammenfassung

### Was ThemisDB von Git lernen kann

1. **YAML-basierte Konfiguration** ✨
   - Deklarative Schema-Definition
   - Infrastructure as Code
   - GitOps-kompatibel

2. **Branching-Konzepte** ✨
   - Named Snapshots (Tags)
   - Data Branches für Staging
   - Merge-Strategien

3. **CI/CD Integration** ✨
   - GitHub Actions für Schema-Validierung
   - Automatisierte Tests
   - Deployment-Pipelines

4. **Audit & Compliance** ✨
   - Vollständige Change-Historie
   - Author-Tracking
   - Signed Commits (für Compliance)

### Was Git von ThemisDB lernen kann

1. **Concurrent Operations**
   - Multiple Writers gleichzeitig
   - Optimistic Concurrency Control

2. **ACID Transactions**
   - Atomare Multi-Objekt-Updates
   - Rollback-Garantien

3. **High-Performance Queries**
   - Index-basierte Suche
   - Vector Search
   - Graph Traversal

### Empfohlene nächste Schritte

1. **Sofort (Sprint 1-2)**
   - ✅ Schema-YAML Format definieren
   - ✅ CLI-Tool für Schema-Management
   - ✅ Basic Schema-Validierung

2. **Kurzfristig (Q1 2026)**
   - 🎯 GitHub Actions Templates
   - 🎯 GitOps-kompatibler Deployment-Mode
   - 🎯 Schema-Migrations-System

3. **Mittelfristig (Q2-Q3 2026)**
   - 🎯 Data Branching (Named Snapshots)
   - 🎯 Time-Travel Queries erweitern
   - 🎯 Distributed Synchronisation

4. **Langfristig (Q4 2026+)**
   - 🎯 Data Pull Requests
   - 🎯 Multi-Master Replication
   - 🎯 Collaborative Data Editing

---

## Ressourcen

### Git/GitHub
- [Git Documentation](https://git-scm.com/doc)
- [GitHub Flow](https://guides.github.com/introduction/flow/)
- [GitHub Actions](https://docs.github.com/en/actions)

### GitOps
- [GitOps Principles](https://opengitops.dev/)
- [ArgoCD](https://argo-cd.readthedocs.io/)
- [Flux](https://fluxcd.io/)

### ThemisDB
- [MVCC Architecture](../de/architecture/architecture_mvcc.md)
- [Transaction Management](../de/features/features_transactions.md)
- [Branching Strategy](../ci-cd/branching-release-history/BRANCHING_STRATEGY.md)
- [Bestehende YAML-Nutzung](bestehende_yaml_nutzung.md) - **NEU: Analyse existierender YAML-Konfigurationen**

---

**Autoren:** ThemisDB Architecture Team  
**Reviewers:** DevOps Team, Database Team  
**Status:** Draft → Review → Published
