[docs](../../index.md) > [de](../index.md) > [core](./index.md) > [architecture](./architecture.md)
**Datum:** 2026-03-11
**Status:** draft
**Primary (Quelle der Wahrheit):**
- `src/core/ARCHITECTURE.md`
- `src/core/README.md`
- `include/core/`

**Bezug / Reference:**
- Issue: [META] Dokumentationssystem: Primary → Secondary → Compendium
- Kontext: Beispiel-Modul-Pipeline, die das dreistufige Dokumentationsmodell demonstriert.

---

## TL;DR

Das Core-Modul ist die Infrastruktur-Basis von ThemisDB: Dependency Injection, zentrales Logging, Konfigurationsverwaltung und Kontext-Propagation. Es stellt keine eigene Datenbankfunktionalität bereit, sondern verbindet alle anderen Module über ein konsistentes Rahmenwerk.

---

## Kontext

- **Problem:** Jedes Modul benötigt Logging, DI-Container, Konfiguration und Laufzeitkontext — ohne gemeinsame Basis entstehen Redundanzen und Inkonsistenzen.
- **Ziel:** Ein schlanker, typsicherer Core, der von allen anderen Modulen verwendet werden kann, ohne zirkuläre Abhängigkeiten einzuführen.
- **Nicht-Ziele:** Geschäftslogik, Datenbankoperationen, Protokoll-Implementierungen.

---

## Architekturübersicht

```
┌─────────────────────────────────────────────────────────────────┐
│                        Core Module                              │
│                                                                 │
│  ┌─────────────────┐   ┌─────────────────┐   ┌──────────────┐  │
│  │  DI Container   │   │  Logger (DI)    │   │  Config      │  │
│  │  (ServiceLocator│   │  ILogger        │   │  Manager     │  │
│  │   + Registry)   │   │  LoggerFactory  │   │              │  │
│  └────────┬────────┘   └────────┬────────┘   └──────┬───────┘  │
│           │                     │                   │          │
│  ┌────────▼─────────────────────▼───────────────────▼───────┐  │
│  │             Context Propagation (thread_local)            │  │
│  │             IContextPtr · ContextPropagation              │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │   Module Dependency Resolver                              │  │
│  │   ModuleDependencyResolver · topologicalSort              │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Kernkomponenten

### Dependency Injection (DI)

Das Core-Modul stellt einen einfachen Service-Container bereit. Module registrieren ihre Abhängigkeiten beim Start und lösen sie über typsichere Getter auf.

**Primärquelle:** `src/core/README.md`, `include/core/`

```cpp
// Registrierung
container.register<ILogger>(std::make_shared<ConsoleLogger>());

// Auflösung
auto logger = container.resolve<ILogger>();
```

### Logging

Der Logger ist selbst als DI-Service registriert (`ILogger`). Die Fabrik `LoggerFactory` ermöglicht modulspezifische Logger mit automatischem Kontext-Prefix.

**Primärquelle:** `src/core/ARCHITECTURE.md`

### Konfigurationsverwaltung

Der `ConfigManager` lädt YAML/JSON-Konfiguration und stellt typsicheren Zugriff via `get<T>(key)` bereit. Zur Laufzeit können Werte überschrieben werden (z. B. für Tests).

### Kontext-Propagation

`ContextPropagation` nutzt `thread_local` Speicher, um Anfrage-Kontexte (Trace-ID, Tenant-ID, Correlation-ID) ohne explizite Parameterübergabe durch den Call-Stack zu tragen.

**Primärquelle:** `src/core/ARCHITECTURE.md`

### Modul-Dependency-Resolver

`ModuleDependencyResolver` implementiert topologische Sortierung der Modul-Abhängigkeiten. Module deklarieren ihre Abhängigkeiten; der Resolver berechnet die Initialisierungsreihenfolge.

---

## Schichten-Modell

```
┌──────────────────────────────────────────────┐
│  Application Layer (Server, HTTP, gRPC, …)   │
├──────────────────────────────────────────────┤
│  Feature Layer (Query, Auth, Cache, …)        │
├──────────────────────────────────────────────┤
│  Storage Layer (Transactions, CDC, …)         │
├──────────────────────────────────────────────┤
│  Core (DI · Logger · Config · Context)  ◄─── │  ← Dieses Modul
└──────────────────────────────────────────────┘
```

Jede Schicht darf Core verwenden. Core darf keine andere Schicht verwenden (kein Zirkel).

---

## Entscheidungen / Trade-offs

| Entscheidung | Begründung |
|---|---|
| `thread_local` für Kontext | Kein explizites Durchreichen durch alle Funktionsaufrufe; kein Performance-Overhead durch Mutex. |
| Kein globales Singleton für DI | Erlaubt Test-Isolation: Jeder Test kann seinen eigenen Container instanziieren. |
| Topologische Sortierung bei Modulstart | Verhindert undefiniertes Verhalten durch falsche Initialisierungsreihenfolge. |
| YAML als primäres Konfigurationsformat | Menschenlesbar, weit verbreitet; JSON als Fallback für maschinenerzeugte Configs. |

---

## Links

- [Primary: src/core/README.md](../../../../src/core/README.md)
- [Primary: src/core/ARCHITECTURE.md](../../../../src/core/ARCHITECTURE.md)
- [Primary: src/core/ROADMAP.md](../../../../src/core/ROADMAP.md)
- [Inhaltsmodell](../../architecture/CONTENT_MODEL.md)
- [Compendium: Kapitel 2 — Architektur-Überblick](../../../../compendium/docs/chapter_02_architecture.md)
