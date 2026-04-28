# 🛡️ AI Safety Layer — Dokumentationsmodul

> **ThemisDB AI Safety Layer** schützt produktive Datenbankinstanzen vor unkontrollierten,
> destruktiven Operationen durch KI-Agenten (MCP-Clients, LLM-Orchestrator, agentic Workflows).

## Hintergrund

Der [Cursor-KI-Vorfall (April 2026)](https://www.golem.de/news/cursor-ki-agent-loescht-aus-versehen-produktiv-datenbank-2604-208074.html)
zeigte, dass KI-Agenten mit Datenbankzugriff ohne geeignete Schutzmaßnahmen zu **Totalverlust von
Produktionsdaten** führen können. Der Agenten löschte eine Produktionsdatenbank, weil:

- keine menschliche Bestätigung für destruktive Operationen erforderlich war
- kein Dry-Run-Modus existierte
- keine Unterscheidung zwischen Produktions- und Entwicklungsumgebung gemacht wurde
- kein automatischer Snapshot vor der Operation erstellt wurde

ThemisDB begegnet diesem Risiko mit einem **mehrschichtigen, deterministischen AI Safety Layer**.

---

## Modulstruktur

| Dokument | Inhalt | Zielgruppe |
|---|---|---|
| [Architektur](AI_SAFETY_ARCHITECTURE.md) | Vollständige 7-Schichten-Architektur, Risikobewertung, Designentscheidungen | Architekten, Senior Engineers |
| [Destructive Operation Guard](AI_SAFETY_OPERATION_GUARD.md) | Schicht 1+2: Klassifikation + Human-in-the-Loop Approval | Backend-Entwickler |
| [AQL Read-Only Enforcer](AI_SAFETY_AQL_VALIDATOR.md) | Schicht 3: AQL-Query-Sicherheitsvalidierung | Query-Engine-Entwickler |
| [IntentClassifier AQL-Erweiterung](AI_SAFETY_INTENT_CLASSIFIER.md) | Schicht 4: AQL-spezifische Angriffserkennung | Security-Entwickler |
| [Pre-Operation Snapshot](AI_SAFETY_SNAPSHOT.md) | Schicht 5: Automatische Checkpoints vor KI-Writes | Storage-Entwickler, DBA |
| [Environment Isolation Guard](AI_SAFETY_ENVIRONMENT.md) | Schicht 6: Produktions-/Entwicklungsisolation | DevOps, Platform-Teams |
| [AI Session Audit Trail](AI_SAFETY_AUDIT_TRAIL.md) | Schicht 7: Manipulationssicheres KI-Audit-Log | Compliance, Security-Ops |
| [Betriebshandbuch](AI_SAFETY_RUNBOOK.md) | Konfiguration, Monitoring, Incident Response | Operatoren, SRE |
| [Schnellreferenz](AI_SAFETY_QUICK_REFERENCE.md) | Konfigurationsübersicht, Checklisten | Alle |

---

## Wo greift KI in ThemisDB ein?

KI-Agenten können an folgenden Stellen in ThemisDB operieren — alle sind durch den AI Safety Layer abgesichert:

```
┌─────────────────────────────────────────────────────────────────┐
│                     ThemisDB AI Touchpoints                     │
├─────────────────┬───────────────────────────────────────────────┤
│ MCP Server      │ Tools: query, delete_entity, drop_index,      │
│                 │ put_entity, create_index                       │
├─────────────────┼───────────────────────────────────────────────┤
│ AI Orchestrator │ Agentic/Multi-Agent pipelines mit Tool-Use     │
├─────────────────┼───────────────────────────────────────────────┤
│ LLM API Handler │ database_query_with_llm, direkter DB-Zugriff  │
├─────────────────┼───────────────────────────────────────────────┤
│ AQL Engine      │ Queries via toolQuery() — inkl. REMOVE/DROP   │
├─────────────────┼───────────────────────────────────────────────┤
│ Admin API       │ Backup/Restore — KI-initiiert möglich          │
├─────────────────┼───────────────────────────────────────────────┤
│ Training APIs   │ LoRA Inline Training, Modellverwaltung         │
└─────────────────┴───────────────────────────────────────────────┘
```

---

## Risikobewertung auf einen Blick

| Angriffsvektor | Ohne Safety Layer | Mit Safety Layer |
|---|---|---|
| AI löscht Datensatz via `delete_entity` | ✅ Direkt möglich | 🔒 DOG + HILG blockiert |
| AI führt `FOR u IN users REMOVE u IN users` aus | ✅ Direkt möglich | 🔒 AQL Validator + IntentClassifier |
| AI droppt Index via `drop_index` | ✅ Direkt möglich | 🔒 DOG + HILG blockiert |
| Kein Checkpoint vor destruktiver Op | ✅ Kein Schutz | 🔒 Auto-Snapshot (POS) |
| Produktion = Entwicklung | ✅ Kein Unterschied | 🔒 Environment Guard |
| Keine forensische KI-Spur | ✅ Keine Logs | 🔒 AI Session Audit Trail |

---

## Verwandte Module

- **[Security Modul](../README.md)** — Übergeordnete Sicherheitsarchitektur
- **[LLM Modul](../../llm/README.md)** — AI Orchestrator, MCP Server, Inferenz
- **[Server Modul](../../server/README.md)** — HTTP API, MCP-Routen
- **[Query Modul](../../aql/README.md)** — AQL-Engine, Query-Validierung
- **[src/security/ROADMAP.md](../../../../src/security/ROADMAP.md)** — Implementierungsplan

---

## Schnellstart: AI Safety konfigurieren

```yaml
# config/security.yaml — Minimalkonfiguration für Produktion
environment:
  name: production
  ai_agent_restrictions:
    block_destructive: true
    require_approval: true
    denied_collections:
      - users
      - audit_log
      - billing

# config/ai_ml/llm/modes/default.yaml — Agentic Mode absichern
modes:
  - id: agentic
    safety:
      enabled: true
      require_approval_for: [DESTRUCTIVE, CRITICAL]
      approval_timeout_s: 60
      dry_run_preview: true
      auto_snapshot: true
      snapshot_dir: "/var/themis/ai-snapshots"
```

Vollständige Konfigurationsreferenz: [Betriebshandbuch](AI_SAFETY_RUNBOOK.md)
