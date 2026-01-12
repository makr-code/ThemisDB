# GitHub Labels Guide / Leitfaden für GitHub-Labels

> **EN:** This guide explains the GitHub label system for ThemisDB issue and pull request categorization.
> 
> **DE:** Dieser Leitfaden erklärt das GitHub-Label-System zur Kategorisierung von Issues und Pull Requests für ThemisDB.

---

## 📋 Table of Contents / Inhaltsverzeichnis

- [English Version](#english-version)
  - [Label Categories](#label-categories)
  - [How to Use Labels](#how-to-use-labels)
  - [Label Automation](#label-automation)
- [Deutsche Version](#deutsche-version)
  - [Label-Kategorien](#label-kategorien)
  - [Verwendung der Labels](#verwendung-der-labels)
  - [Label-Automatisierung](#label-automatisierung)

---

## English Version

### Overview

ThemisDB uses a structured labeling system to categorize and organize issues and pull requests. The label system consists of 10 main categories, each with a specific purpose.

### Label Categories

#### 1. 🔴 Priority Labels (`priority:*`)

Indicate the urgency and importance of an issue.

| Label | Color | Description | When to Use |
|-------|-------|-------------|-------------|
| `priority:P0` | 🔴 Red | Critical - blocks release or causes system failure | Security vulnerabilities, data loss, server crashes |
| `priority:P1` | 🟠 Orange | High - important for next release | Important features, significant bugs |
| `priority:P2` | 🟡 Yellow | Medium - planned for future release | Nice-to-have features, minor bugs |
| `priority:P3` | 🟢 Green | Low - backlog | Enhancement ideas, cosmetic improvements |

**Usage Guidelines:**
- Every issue should have exactly ONE priority label
- P0 issues should be addressed immediately
- P1 issues should be addressed in the current sprint
- P2/P3 can be scheduled for future sprints

#### 2. 🏷️ Type Labels (`type:*`)

Categorize the nature of the issue.

| Label | Description | Examples |
|-------|-------------|----------|
| `type:bug` | Something doesn't work correctly | Crashes, incorrect output, errors |
| `type:feature` | New functionality request | New API endpoints, new features |
| `type:enhancement` | Improvement to existing functionality | Better error messages, UX improvements |
| `type:documentation` | Documentation improvements | README updates, API docs, guides |
| `type:security` | Security-related issues | CVEs, vulnerabilities, security hardening |
| `type:performance` | Performance optimization | Slow queries, memory leaks, optimizations |
| `type:refactoring` | Code improvement without behavior change | Code cleanup, reorganization |
| `type:test` | Test additions or improvements | New tests, test fixes |
| `type:question` | Question or support request | How-to questions, clarifications |
| `type:discussion` | Discussion or RFC | Design decisions, architecture proposals |

**Usage Guidelines:**
- Every issue should have exactly ONE type label
- Use `type:bug` for anything that doesn't work as documented
- Use `type:feature` for net-new functionality
- Use `type:enhancement` for improvements to existing features

#### 3. 🎯 Area Labels (`area:*`)

Identify which component or subsystem is affected.

| Label | Component | Examples |
|-------|-----------|----------|
| `area:llm` | LLM/AI features | llama.cpp integration, embeddings, inference |
| `area:storage` | Storage layer | RocksDB, persistence, durability |
| `area:aql` | AQL query language | Parser, query execution, functions |
| `area:api` | REST API | HTTP endpoints, API handlers |
| `area:networking` | Network protocols | gRPC, PostgreSQL protocol, connections |
| `area:sharding` | Distributed systems | RAID, sharding, clustering |
| `area:replication` | Data replication | WAL, replication, consistency |
| `area:security` | Security features | Auth, encryption, access control |
| `area:monitoring` | Observability | Metrics, logs, traces |
| `area:content-processing` | Content processing | PDF, Office, video, images |
| `area:geo` | Geospatial | GIS, spatial queries, GDAL |
| `area:voice` | Voice assistant | STT, TTS, Whisper integration |
| `area:performance` | Performance system | Benchmarks, profiling |
| `area:build` | Build system | CMake, vcpkg, dependencies |
| `area:docker` | Containers | Docker, docker-compose, Kubernetes |
| `area:ci-cd` | CI/CD | GitHub Actions, workflows |
| `area:docs` | Documentation | MkDocs, compendium, guides |
| `area:sdks` | Client SDKs | Python, Java, .NET clients |
| `area:observability` | Monitoring stack | Grafana, Prometheus, tracing |

**Usage Guidelines:**
- Issues can have MULTIPLE area labels if they affect multiple components
- Use area labels to help route issues to the right maintainers
- If unsure, skip area labels and maintainers will add them

#### 4. 📊 Status Labels (`status:*`)

Track the current state of an issue.

| Label | Description | Next Action |
|-------|-------------|-------------|
| `status:ready` | Ready to be worked on | Start implementation |
| `status:in-progress` | Currently being worked on | Continue work |
| `status:needs-review` | Needs review or feedback | Review and provide feedback |
| `status:needs-info` | Needs more information | Reporter should provide details |
| `status:blocked` | Blocked by dependency | Resolve blocker |
| `status:on-hold` | Waiting for decision | Await decision |

**Usage Guidelines:**
- Status labels are maintained by maintainers and contributors
- `status:needs-info` will be closed if no response in 14 days
- `status:blocked` should reference the blocking issue

#### 5. ⏱️ Effort Labels (`effort:*`)

Estimate the work required.

| Label | Time Estimate | Suitable For |
|-------|---------------|--------------|
| `effort:small` | < 1 day | Bug fixes, small improvements |
| `effort:medium` | 1-3 days | Feature additions, refactoring |
| `effort:large` | 1-2 weeks | Complex features, major refactoring |
| `effort:x-large` | > 2 weeks | Major features, architectural changes |

**Usage Guidelines:**
- Effort labels help with sprint planning
- Estimates are rough and may change during implementation
- Good for matching issues to available time

#### 6. 👥 Experience Labels

Help contributors find suitable issues.

| Label | Description | Ideal For |
|-------|-------------|-----------|
| `good first issue` | Good for newcomers | First-time contributors |
| `help wanted` | Looking for contributors | Any contributor |
| `mentor available` | Maintainer will mentor | Contributors needing guidance |

**Usage Guidelines:**
- `good first issue` should be well-defined with clear acceptance criteria
- `help wanted` indicates the team welcomes external contributions
- `mentor available` means a maintainer will provide 1-on-1 guidance

#### 7. 🎪 Special Labels

Special purpose labels for specific situations.

| Label | Description | When to Use |
|-------|-------------|-------------|
| `breaking-change` | Breaking API or behavior change | Requires major version bump |
| `regression` | Previously working, now broken | Bug introduced in recent release |
| `duplicate` | Duplicate of another issue | Reference original issue |
| `wontfix` | Will not be implemented | Not aligned with project goals |
| `invalid` | Invalid or not reproducible | Cannot reproduce or invalid report |
| `dependencies` | Dependency updates | vcpkg, submodules, library updates |
| `technical-debt` | Tech debt | Code that needs refactoring |

#### 8. 📦 Edition Labels (`edition:*`)

Specify which edition is affected.

| Label | Description |
|-------|-------------|
| `edition:minimal` | Minimal edition specific |
| `edition:standard` | Standard edition specific |
| `edition:enterprise` | Enterprise edition specific |

#### 9. 💻 Platform Labels (`platform:*`)

Identify platform-specific issues.

| Label | Platform |
|-------|----------|
| `platform:linux` | Linux-specific |
| `platform:windows` | Windows-specific |
| `platform:macos` | macOS-specific |

#### 10. 🌍 Language Labels (`lang:*`)

For language-specific documentation or localization.

| Label | Language |
|-------|----------|
| `lang:german` | German language |
| `lang:english` | English language |

### How to Use Labels

#### For Issue Reporters

When creating a new issue:

1. **Don't worry about labels** - Maintainers will add appropriate labels
2. **Focus on content** - Provide clear description, reproduction steps, expected vs actual behavior
3. **Use templates** - Follow the issue templates in `.github/ISSUE_TEMPLATE/`

#### For Contributors

When working on issues:

1. **Check labels** to understand priority and scope
2. **Look for `good first issue`** if you're new
3. **Check effort labels** to match your available time
4. **Update status** when starting work (add `status:in-progress`)

#### For Maintainers

When triaging issues:

1. **Add priority** (`priority:P0` to `priority:P3`)
2. **Add type** (e.g., `type:bug`, `type:feature`)
3. **Add area** (e.g., `area:llm`, `area:storage`)
4. **Add effort** if you can estimate it
5. **Add status** (`status:ready`, `status:needs-info`, etc.)
6. **Add special labels** as needed (`good first issue`, `help wanted`, etc.)

### Label Automation

Some labels are automatically applied:

- **CI/CD workflows** automatically add `area:ci-cd` to workflow-related PRs
- **Dependabot** adds `dependencies` label to dependency updates
- **Stale bot** may add labels to inactive issues

### Label Examples

#### Example 1: Critical Bug
```
priority:P0
type:bug
area:storage
area:replication
status:in-progress
regression
```

#### Example 2: Feature Request
```
priority:P2
type:feature
area:llm
area:api
effort:large
help wanted
```

#### Example 3: Good First Issue
```
priority:P3
type:enhancement
area:docs
effort:small
good first issue
mentor available
```

---

## Deutsche Version

### Überblick

ThemisDB verwendet ein strukturiertes Label-System zur Kategorisierung und Organisation von Issues und Pull Requests. Das Label-System besteht aus 10 Hauptkategorien mit jeweils spezifischem Zweck.

### Label-Kategorien

#### 1. 🔴 Prioritäts-Labels (`priority:*`)

Kennzeichnen die Dringlichkeit und Wichtigkeit eines Issues.

| Label | Farbe | Beschreibung | Wann verwenden |
|-------|-------|--------------|----------------|
| `priority:P0` | 🔴 Rot | Kritisch - blockiert Release oder verursacht Systemausfall | Sicherheitslücken, Datenverlust, Server-Abstürze |
| `priority:P1` | 🟠 Orange | Hoch - wichtig für nächstes Release | Wichtige Features, signifikante Bugs |
| `priority:P2` | 🟡 Gelb | Mittel - für zukünftiges Release geplant | Nice-to-have Features, kleinere Bugs |
| `priority:P3` | 🟢 Grün | Niedrig - Backlog | Verbesserungsideen, kosmetische Änderungen |

**Nutzungsrichtlinien:**
- Jedes Issue sollte GENAU EIN Prioritäts-Label haben
- P0-Issues müssen sofort bearbeitet werden
- P1-Issues sollten im aktuellen Sprint bearbeitet werden
- P2/P3 können für zukünftige Sprints eingeplant werden

#### 2. 🏷️ Typ-Labels (`type:*`)

Kategorisieren die Art des Issues.

| Label | Beschreibung | Beispiele |
|-------|--------------|-----------|
| `type:bug` | Etwas funktioniert nicht korrekt | Abstürze, falsche Ausgabe, Fehler |
| `type:feature` | Anfrage für neue Funktionalität | Neue API-Endpunkte, neue Features |
| `type:enhancement` | Verbesserung bestehender Funktionalität | Bessere Fehlermeldungen, UX-Verbesserungen |
| `type:documentation` | Dokumentationsverbesserungen | README-Updates, API-Docs, Anleitungen |
| `type:security` | Sicherheitsbezogene Issues | CVEs, Schwachstellen, Security-Härtung |
| `type:performance` | Performance-Optimierung | Langsame Queries, Memory-Leaks, Optimierungen |
| `type:refactoring` | Code-Verbesserung ohne Verhaltensänderung | Code-Aufräumung, Reorganisation |
| `type:test` | Test-Ergänzungen oder Verbesserungen | Neue Tests, Test-Fixes |
| `type:question` | Frage oder Support-Anfrage | How-to-Fragen, Klarstellungen |
| `type:discussion` | Diskussion oder RFC | Design-Entscheidungen, Architektur-Vorschläge |

**Nutzungsrichtlinien:**
- Jedes Issue sollte GENAU EIN Typ-Label haben
- Verwende `type:bug` für alles, was nicht wie dokumentiert funktioniert
- Verwende `type:feature` für komplett neue Funktionalität
- Verwende `type:enhancement` für Verbesserungen existierender Features

#### 3. 🎯 Bereichs-Labels (`area:*`)

Identifizieren, welche Komponente oder welches Subsystem betroffen ist.

| Label | Komponente | Beispiele |
|-------|------------|-----------|
| `area:llm` | LLM/KI-Features | llama.cpp Integration, Embeddings, Inferenz |
| `area:storage` | Storage-Schicht | RocksDB, Persistenz, Dauerhaftigkeit |
| `area:aql` | AQL Abfragesprache | Parser, Query-Ausführung, Funktionen |
| `area:api` | REST API | HTTP-Endpunkte, API-Handler |
| `area:networking` | Netzwerkprotokolle | gRPC, PostgreSQL-Protokoll, Verbindungen |
| `area:sharding` | Verteilte Systeme | RAID, Sharding, Clustering |
| `area:replication` | Datenreplikation | WAL, Replikation, Konsistenz |
| `area:security` | Sicherheitsfeatures | Auth, Verschlüsselung, Zugriffskontrolle |
| `area:monitoring` | Observability | Metriken, Logs, Traces |
| `area:content-processing` | Content-Verarbeitung | PDF, Office, Video, Bilder |
| `area:geo` | Geospatial | GIS, Spatial-Queries, GDAL |
| `area:voice` | Sprachassistent | STT, TTS, Whisper-Integration |
| `area:performance` | Performance-System | Benchmarks, Profiling |
| `area:build` | Build-System | CMake, vcpkg, Abhängigkeiten |
| `area:docker` | Container | Docker, docker-compose, Kubernetes |
| `area:ci-cd` | CI/CD | GitHub Actions, Workflows |
| `area:docs` | Dokumentation | MkDocs, Kompendium, Anleitungen |
| `area:sdks` | Client-SDKs | Python-, Java-, .NET-Clients |
| `area:observability` | Monitoring-Stack | Grafana, Prometheus, Tracing |

**Nutzungsrichtlinien:**
- Issues können MEHRERE Bereichs-Labels haben, wenn sie mehrere Komponenten betreffen
- Verwende Bereichs-Labels, um Issues an die richtigen Maintainer zu routen
- Bei Unsicherheit: Bereichs-Labels weglassen, Maintainer fügen sie hinzu

#### 4. 📊 Status-Labels (`status:*`)

Verfolgen den aktuellen Zustand eines Issues.

| Label | Beschreibung | Nächster Schritt |
|-------|--------------|------------------|
| `status:ready` | Bereit zur Bearbeitung | Implementation starten |
| `status:in-progress` | Wird gerade bearbeitet | Arbeit fortsetzen |
| `status:needs-review` | Benötigt Review oder Feedback | Review durchführen und Feedback geben |
| `status:needs-info` | Benötigt mehr Informationen | Reporter soll Details liefern |
| `status:blocked` | Blockiert durch Abhängigkeit | Blocker auflösen |
| `status:on-hold` | Wartet auf Entscheidung | Auf Entscheidung warten |

**Nutzungsrichtlinien:**
- Status-Labels werden von Maintainern und Contributors gepflegt
- `status:needs-info` wird nach 14 Tagen ohne Antwort geschlossen
- `status:blocked` sollte auf das blockierende Issue referenzieren

#### 5. ⏱️ Aufwands-Labels (`effort:*`)

Schätzen den erforderlichen Arbeitsaufwand.

| Label | Zeitschätzung | Geeignet für |
|-------|---------------|--------------|
| `effort:small` | < 1 Tag | Bug-Fixes, kleine Verbesserungen |
| `effort:medium` | 1-3 Tage | Feature-Ergänzungen, Refactoring |
| `effort:large` | 1-2 Wochen | Komplexe Features, größeres Refactoring |
| `effort:x-large` | > 2 Wochen | Major Features, architektonische Änderungen |

**Nutzungsrichtlinien:**
- Aufwands-Labels helfen bei der Sprint-Planung
- Schätzungen sind grob und können sich während der Implementation ändern
- Gut um Issues mit verfügbarer Zeit abzugleichen

#### 6. 👥 Erfahrungs-Labels

Helfen Contributors, passende Issues zu finden.

| Label | Beschreibung | Ideal für |
|-------|--------------|-----------|
| `good first issue` | Gut für Neueinsteiger | Erstmalige Contributors |
| `help wanted` | Sucht Contributors | Alle Contributors |
| `mentor available` | Maintainer bietet Mentoring | Contributors die Anleitung benötigen |

**Nutzungsrichtlinien:**
- `good first issue` sollte gut definiert sein mit klaren Akzeptanzkriterien
- `help wanted` zeigt an, dass das Team externe Beiträge begrüßt
- `mentor available` bedeutet, dass ein Maintainer 1-zu-1-Anleitung bietet

#### 7. 🎪 Spezial-Labels

Spezielle Labels für bestimmte Situationen.

| Label | Beschreibung | Wann verwenden |
|-------|--------------|----------------|
| `breaking-change` | Breaking API- oder Verhaltensänderung | Erfordert Major-Version-Bump |
| `regression` | Funktionierte vorher, jetzt kaputt | Bug in neuem Release eingeführt |
| `duplicate` | Duplikat eines anderen Issues | Auf Original-Issue referenzieren |
| `wontfix` | Wird nicht implementiert | Nicht mit Projekt-Zielen vereinbar |
| `invalid` | Ungültig oder nicht reproduzierbar | Nicht reproduzierbar oder ungültiger Report |
| `dependencies` | Dependency-Updates | vcpkg, Submodules, Library-Updates |
| `technical-debt` | Technische Schulden | Code der Refactoring benötigt |

#### 8. 📦 Editions-Labels (`edition:*`)

Spezifizieren, welche Edition betroffen ist.

| Label | Beschreibung |
|-------|--------------|
| `edition:minimal` | Minimal-Edition spezifisch |
| `edition:standard` | Standard-Edition spezifisch |
| `edition:enterprise` | Enterprise-Edition spezifisch |

#### 9. 💻 Plattform-Labels (`platform:*`)

Identifizieren plattformspezifische Issues.

| Label | Plattform |
|-------|-----------|
| `platform:linux` | Linux-spezifisch |
| `platform:windows` | Windows-spezifisch |
| `platform:macos` | macOS-spezifisch |

#### 10. 🌍 Sprach-Labels (`lang:*`)

Für sprachspezifische Dokumentation oder Lokalisierung.

| Label | Sprache |
|-------|---------|
| `lang:german` | Deutsche Sprache |
| `lang:english` | Englische Sprache |

### Verwendung der Labels

#### Für Issue-Reporter

Beim Erstellen eines neuen Issues:

1. **Keine Sorge um Labels** - Maintainer fügen passende Labels hinzu
2. **Fokus auf Inhalt** - Klare Beschreibung, Reproduktionsschritte, erwartetes vs. tatsächliches Verhalten
3. **Verwende Templates** - Folge den Issue-Templates in `.github/ISSUE_TEMPLATE/`

#### Für Contributors

Bei der Arbeit an Issues:

1. **Prüfe Labels** um Priorität und Umfang zu verstehen
2. **Suche nach `good first issue`** wenn du neu bist
3. **Prüfe Aufwands-Labels** um sie mit deiner verfügbaren Zeit abzugleichen
4. **Aktualisiere Status** beim Start der Arbeit (füge `status:in-progress` hinzu)

#### Für Maintainer

Beim Triagieren von Issues:

1. **Füge Priorität hinzu** (`priority:P0` bis `priority:P3`)
2. **Füge Typ hinzu** (z.B. `type:bug`, `type:feature`)
3. **Füge Bereich hinzu** (z.B. `area:llm`, `area:storage`)
4. **Füge Aufwand hinzu** wenn du ihn schätzen kannst
5. **Füge Status hinzu** (`status:ready`, `status:needs-info`, etc.)
6. **Füge Spezial-Labels hinzu** nach Bedarf (`good first issue`, `help wanted`, etc.)

### Label-Automatisierung

Einige Labels werden automatisch angewendet:

- **CI/CD-Workflows** fügen automatisch `area:ci-cd` zu Workflow-bezogenen PRs hinzu
- **Dependabot** fügt `dependencies`-Label zu Dependency-Updates hinzu
- **Stale Bot** kann Labels zu inaktiven Issues hinzufügen

### Label-Beispiele

#### Beispiel 1: Kritischer Bug
```
priority:P0
type:bug
area:storage
area:replication
status:in-progress
regression
```

#### Beispiel 2: Feature-Request
```
priority:P2
type:feature
area:llm
area:api
effort:large
help wanted
```

#### Beispiel 3: Good First Issue
```
priority:P3
type:enhancement
area:docs
effort:small
good first issue
mentor available
```

---

## 📚 Additional Resources / Zusätzliche Ressourcen

- **Label Configuration:** `.github/labels.yml`
- **Label Sync Script:** `.github/scripts/sync-labels.sh`
- **Contributing Guide:** `CONTRIBUTING.md`
- **Issue Templates:** `.github/ISSUE_TEMPLATE/`

---

## 🔄 Updating This Guide / Aktualisierung dieses Leitfadens

This guide should be updated whenever:
- New label categories are added
- Label meanings change
- New automation is introduced

Dieser Leitfaden sollte aktualisiert werden wenn:
- Neue Label-Kategorien hinzugefügt werden
- Label-Bedeutungen sich ändern
- Neue Automatisierung eingeführt wird

---

**Last Updated / Zuletzt aktualisiert:** 2026-01-11
