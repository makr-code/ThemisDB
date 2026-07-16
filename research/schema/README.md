# ThemisDB Schema Examples

Dieses Verzeichnis enthält Beispiele für deklarative Schema-Definitionen im YAML-Format, inspiriert von Git und GitOps.

## Übersicht

Die YAML-basierten Schema-Definitionen bieten eine deklarative Möglichkeit, ThemisDB-Datenbanken zu konfigurieren, ähnlich wie Infrastructure as Code (IaC) für Infrastruktur.

## Konzept

### Git-inspirierte Elemente

1. **Deklarative Konfiguration**: Schema wird als Code definiert
2. **Versionskontrolle**: Schema-Dateien können in Git versioniert werden
3. **Auditierbarkeit**: Änderungen sind nachvollziehbar
4. **GitOps-kompatibel**: Automatische Synchronisation möglich

## Verfügbare Beispiele

### `themis-schema.example.yaml`

Vollständiges Beispiel-Schema für eine Social Media Anwendung mit:
- Tabellendefinitionen (users, posts)
- Secondary, Vector und Fulltext Indexes
- Foreign Key Relationships
- Backup-Konfiguration
- Security & RBAC
- Performance-Tuning

## Verwendung (Zukünftig)

> **Hinweis**: Die YAML-basierte Schema-Definition ist derzeit ein **Konzept** und noch nicht implementiert. Siehe [git_gitops_themis_vergleich.md](../git_gitops_themis_vergleich.md) für Details.

### Geplante CLI-Befehle

```bash
# Schema validieren
themis schema validate themis-schema.yaml

# Schema anwenden
themis schema apply themis-schema.yaml

# Schema-Unterschiede anzeigen
themis schema diff themis-schema.yaml

# Schema exportieren
themis schema export > current-schema.yaml
```

### GitOps-Workflow (Geplant)

```yaml
# .github/workflows/schema-deploy.yml
name: Deploy Schema
on:
  push:
    branches: [main]
    paths:
      - 'schemas/**/*.yaml'

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Validate Schema
        run: themis schema validate schemas/production.yaml
      
      - name: Apply Schema
        run: themis schema apply schemas/production.yaml
        env:
          THEMIS_URL: ${{ secrets.THEMIS_URL }}
          THEMIS_TOKEN: ${{ secrets.THEMIS_TOKEN }}
```

## Schema-Struktur

### Grundlegende Struktur

```yaml
version: "1.0"
metadata:
  name: my-database
  description: "Description"
  git_revision: "commit-sha"

config:
  storage:
    engine: rocksdb
  transaction:
    isolation_level: snapshot

tables:
  my_table:
    description: "Table description"
    primary_key: id
    
    fields:
      id:
        type: string
        required: true
      
      data:
        type: json
        required: false
    
    indexes:
      - name: idx_data
        columns: [data]
        type: secondary

backup:
  enabled: true
  schedule: "0 2 * * *"

security:
  tls:
    enabled: true
  authentication:
    method: jwt
```

## Vorteile

### 1. Infrastructure as Code

- Schema wird wie Code behandelt
- Änderungen sind nachvollziehbar
- Code Reviews für Schema-Änderungen
- Automatisierte Tests möglich

### 2. GitOps-Integration

- Schema in Git versioniert
- Automatische Synchronisation
- Rollback über Git möglich
- Audit Trail durch Git History

### 3. Deklarativ statt Imperativ

**Vorher (Imperativ)**:
```bash
curl -X POST /index/create -d '{"table":"users","column":"email"}'
curl -X POST /index/create -d '{"table":"users","column":"username"}'
```

**Nachher (Deklarativ)**:
```yaml
tables:
  users:
    indexes:
      - name: idx_email
        columns: [email]
      - name: idx_username
        columns: [username]
```

### 4. Environment-Parity

```yaml
# base-schema.yaml
tables:
  users:
    primary_key: user_id

---
# production.yaml
extends: base-schema.yaml
config:
  replication:
    factor: 3
  backup:
    enabled: true

---
# development.yaml
extends: base-schema.yaml
config:
  replication:
    factor: 1
  backup:
    enabled: false
```

## Best Practices

### 1. Schema-Versionierung

```yaml
version: "1.0"
metadata:
  schema_version: "2.3.1"
  git_revision: "a1b2c3d4"
  last_modified: "2026-01-14T10:00:00Z"
  changelog: |
    v2.3.1: Added embedding vector index
    v2.3.0: Added posts table
    v2.2.0: Initial schema
```

### 2. Dokumentation

```yaml
tables:
  users:
    description: |
      User accounts and profiles.
      This table stores all registered users.
    
    fields:
      user_id:
        description: "Unique user identifier (UUID v4)"
        type: string
        format: uuid
```

### 3. Validierung

```yaml
fields:
  email:
    type: string
    required: true
    validation:
      pattern: "^[a-z0-9._%+-]+@[a-z0-9.-]+\\.[a-z]{2,}$"
      error_message: "Invalid email format"
  
  age:
    type: integer
    validation:
      min: 0
      max: 150
      error_message: "Age must be between 0 and 150"
```

### 4. Migration-Strategie

```yaml
# migrations/001_initial_schema.yaml
version: "001"
description: "Initial schema"
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

## Zukunftsperspektive

### Roadmap

**Phase 1: Schema Definition (Q1 2026)**
- ✅ YAML-Format definieren
- ⏳ Schema-Parser implementieren
- ⏳ Validierung implementieren

**Phase 2: CLI-Tools (Q2 2026)**
- ⏳ `themis schema` Befehle
- ⏳ Schema-Diff-Tool
- ⏳ Schema-Migration-Tool

**Phase 3: GitOps-Integration (Q3 2026)**
- ⏳ GitHub Actions Templates
- ⏳ Automatische Synchronisation
- ⏳ ArgoCD-Integration

**Phase 4: Advanced Features (Q4 2026)**
- ⏳ Schema-Branching
- ⏳ Schema-Pull-Requests
- ⏳ Multi-Environment-Support

## Weitere Ressourcen

- [Git vs ThemisDB Vergleich](../git_gitops_themis_vergleich.md)
- [MVCC Architecture](../../de/architecture/architecture_mvcc.md)
- [Branching Strategy](../../ci-cd/branching-release-history/BRANCHING_STRATEGY.md)

## Feedback

Feedback zu diesen Konzepten ist willkommen! Bitte erstelle ein Issue oder eine Discussion auf GitHub:

- [Issues](https://github.com/makr-code/ThemisDB/issues)
- [Discussions](https://github.com/makr-code/ThemisDB/discussions)

---

**Status**: Konzept  
**Version**: 1.0  
**Erstellt**: 2026-01-14  
**Autoren**: ThemisDB Architecture Team
