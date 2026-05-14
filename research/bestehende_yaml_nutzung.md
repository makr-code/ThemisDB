# Bestehende YAML-Nutzung in ThemisDB

**Stand:** 14. Mai 2026
**Version:** 1.1
**Kategorie:** 🔍 Research
**Status:** Research Complete

---

## Abstract

Dieser Artikel überprüft die bestehende YAML-Nutzung im Repository `makr-code/ThemisDB` gegen den aktuellen Code- und Dokumentationsstand. Das Ergebnis ist eindeutig: YAML ist in ThemisDB bereits breit etabliert, aber nicht jede YAML-Datei hat denselben Status. Ein Teil der Artefakte ist produktiv bzw. laufzeitrelevant (z. B. PII-, Retention-, Ethical- und Core-Konfigurationen), ein weiterer Teil dient API-/Deployment-Beschreibung (OpenAPI, Kubernetes-CRD), und ein dritter Teil ist klar als Beispiel, Profil oder Zukunftskonzept zu lesen. Die frühere Fassung dieses Dokuments vermischte diese Ebenen teilweise. Die vorliegende Überarbeitung trennt deshalb strikt zwischen **verifiziertem Ist-Zustand**, **belegbaren Codepfaden** und **nicht als implementiert bestätigten Zukunftsideen**.

---

## 1. Einleitung

ThemisDB beschreibt sich selbst als **Multi-Model-Datenbank** mit nativer AI/LLM-Integration und modularer Architektur. In diesem Umfeld ist YAML im Repository an vielen Stellen sichtbar: als Konfigurationsformat, als Austauschformat, als API-/Deployment-Beschreibung und als Testartefakt.

Für eine review-fähige Aussage reicht reine Dateiexistenz jedoch nicht aus. Entscheidend ist, ob ein YAML-Artefakt

1. im aktuellen Repository vorhanden ist,
2. über einen konkreten Codepfad geladen, validiert oder referenziert wird, und
3. nicht bloß als Beispiel oder Zukunftskonzept dokumentiert ist.

Ziel dieses Dokuments ist daher **nicht**, YAML pauschal als „überall produktiv“ darzustellen, sondern den aktuellen Stand belastbar einzuordnen.

---

## 2. Methodik / Ansatz

Die Analyse basiert auf einem Repository-Snapshot des lokalen Arbeitsstands (Code- und Dateireferenzen: ThemisDB-Branch `copilot/review-bestehende-yaml-nutzung`, Snapshot `3af441821703119a9c24107b8d666dcbae9b4956`). Dieser Snapshot wurde während der Analyse lokal per `git rev-parse HEAD` verifiziert; die Quellenlisten nutzen commit-gepinnte GitHub-URLs zur Reproduzierbarkeit.

Anschließend kombiniert die Untersuchung vier Prüfschritte:

1. **Artefaktprüfung:** Sichtung der YAML-Dateien unter `config/`, `deploy/`, `docs/` und `openapi/`.
2. **Codeabgleich:** Prüfung, welche Komponenten YAML aktiv laden, validieren, mappen oder exportieren.
3. **Testabgleich:** Sichtung vorhandener Unit-/Integrationstests, die YAML-Verarbeitung direkt prüfen.
4. **Abgrenzung von Konzepten:** Abgleich mit Forschungs- und Readme-Dateien, um Zukunftsideen nicht als Ist-Funktion zu missverstehen.

Verwendete Leitfrage: **„Ist dies ein reales YAML-Artefakt mit nachweisbarem Bezug zum aktuellen ThemisDB-Stand?“**

Wichtig für die Einordnung:

- Diese Untersuchung ist **repository-basiert**, nicht benchmark-basiert.
- Es wurden **keine dedizierten Performance-Benchmarks zur YAML-Nutzung** gefunden, die für diesen Artikel belastbar zitierbar wären.
- Performance-Benchmarks sind für diese Bestandsaufnahme **methodisch out of scope**; bewertet wird die Existenz, Einbindung und Testbarkeit von YAML-Artefakten, nicht deren Parse- oder Laufzeitkosten.
- Aussagen zu Laufzeitverhalten werden deshalb nur dort getroffen, wo der Codepfad oder Tests dies direkt stützen.

---

## 3. Verifizierter Ist-Stand

## 3.1 Laufzeit- und Betriebskonfigurationen

Die stärkste, direkt belegbare YAML-Nutzung liegt im Konfigurationsbereich.

| Bereich | Primäres Artefakt | Code-/Systembezug | Einordnung |
|---|---|---|---|
| PII / Security | `config/security/pii_patterns.yaml` | `PIIRedactionPolicy` nutzt standardmäßig `config/pii_patterns.yaml`; `ConfigPathResolver` mappt Altpfad auf neuen Pfad | **Produktionsnah / laufzeitrelevant** |
| Retention / Data Management | `config/data_management/retention_policies.yaml` | `HttpServer` und `main_server` referenzieren `config/retention_policies.yaml` und nutzen den Resolver | **Produktionsnah / laufzeitrelevant** |
| Ethics / LLM | `config/compliance/ethical_guidelines.yaml` | `EthicalGuidelinesManager` verwendet standardmäßig `config/ethical_guidelines.yaml`; Altpfad wird gemappt | **Produktionsnah / laufzeitrelevant** |
| Core-Server-Konfiguration | `config/core/config.yaml` | `main_server` sucht explizit nach `./config/core/config.yaml` und Legacy-Pfaden | **Produktionsnah / laufzeitrelevant** |
| NLP | `config/nlp/nlp_config.yaml` | Reales Artefakt im Konfigurationsbaum; Teil der NLP-Konfigurationslandschaft | **Vorhanden; konkrete Aktivierung im Artikel nur vorsichtig behauptbar** |
| LLM-Modelle | `config/llm-models.yaml` | Reales Modellregister im Repo; dient als deklaratives Modell-/Profilartefakt | **Vorhanden; nicht pauschal als einziger Laufzeitpfad darstellen** |
| Sharding | `config/sharding/shard-router-example.yaml` | Beispielhafte Cluster-/Router-Topologie | **Beispiel-/Referenzartefakt, nicht automatisch Default-Runtime** |

### Kernaussage

YAML ist in ThemisDB bereits **kanonisches Konfigurationsmedium** für mehrere Sicherheits-, Compliance- und Betriebsbereiche. Diese Aussage ist belegbar. **Nicht belegbar** ist dagegen die frühere pauschale Behauptung, praktisch alle relevanten Konfigurationen seien gleichförmig versioniert, hot-reload-fähig oder sicherheitsmäßig identisch abgesichert.

---

## 3.2 YAML im Code: Laden, Validieren, Migrieren, Exportieren

Neben reinen Konfigurationsdateien existieren mehrere konkrete YAML-Codepfade:

### 1. Allgemeine YAML/JSON-Validierung

`ConfigSchemaValidator` lädt `.yaml`- und `.yml`-Dateien über `yaml-cpp`, wandelt sie in JSON um und validiert sie anschließend gegen JSON Schema. Dazu gehören sowohl dateibasierte als auch stringbasierte Eingänge.

**Bedeutung:** YAML ist nicht bloß Ablageformat, sondern Teil einer generischen Validierungsstrecke.

### 2. Pfadmigration zwischen Legacy- und Zielstruktur

`ConfigPathResolver` mappt mehrere historische Pfade auf die aktuelle Ordnerstruktur, u. a.:

- `config/pii_patterns.yaml` → `config/security/pii_patterns.yaml`
- `config/ethical_guidelines.yaml` → `config/compliance/ethical_guidelines.yaml`
- `config/retention_policies.yaml` → `config/data_management/retention_policies.yaml`
- `config/config.yaml` → `config/core/config.yaml`

**Bedeutung:** Die YAML-Nutzung ist gewachsen und wird aktiv migrationskompatibel gehalten. Das spricht für reale Nutzung über mehrere Entwicklungsstände hinweg.

### 3. Sicherheits- und Compliance-Pfade

`PIIRedactionPolicy` dokumentiert explizit einen Zero-Configuration-Default für `config/pii_patterns.yaml`. `HttpServer` und `main_server` initialisieren Retention-Komponenten über den aufgelösten Pfad zu `retention_policies.yaml`. `EthicalGuidelinesManager` besitzt ebenfalls einen Standardpfad auf YAML-Basis.

**Bedeutung:** Für diese Teilbereiche ist YAML nicht nur Dokumentation, sondern Teil des erwarteten Betriebsmodells.

### 4. YAML als Import-/Exportformat

`PromptLibraryIO` exportiert Prompt-Bibliotheken nach YAML und importiert sie wieder zurück. Das ist ein anderer Nutzungstyp als klassische Serverkonfiguration: YAML dient hier als **portable Austauschrepräsentation** für semantische Inhalte.

### 5. Testbare YAML-Verarbeitung

Die Tests `test_config_schema_validator.cpp`, `test_index_analyzer.cpp` und `test_prompt_library_io.cpp` prüfen explizit das Laden, Validieren bzw. Rundtrippen von YAML-Inhalten.

**Bedeutung:** YAML-Verarbeitung ist in mehreren Bereichen nicht nur implementiert, sondern auch regressionsgesichert.

---

## 3.3 YAML für API- und Deployment-Beschreibung

Nicht jede YAML-Datei ist Konfiguration im engeren Sinn. Zwei weitere Klassen sind relevant:

### 1. Kubernetes-CRD

`deploy/kubernetes/crds/themisdb.vcc.io_themisdbs.yaml` beschreibt ein deklaratives Deployment-Modell für ThemisDB als Kubernetes Custom Resource. Das ist **kein DB-Schema**, aber ein starkes Indiz dafür, dass ThemisDB deklarative Betriebsbeschreibungen auf YAML-Basis systematisch nutzt.

### 2. OpenAPI-Spezifikationen

Im Repository existieren zwei OpenAPI-YAML-Dateien:

- `docs/openapi.yaml`
- `openapi/openapi.yaml`

Dabei weist `docs/openapi.yaml` selbst per `x-api-governance.source_of_truth` auf sich als Source of Truth aus. `openapi/openapi.yaml` ist damit nicht automatisch obsolet, aber die frühere Darstellung als gleichrangige, einheitliche REST-Beschreibung war zu ungenau.

### Einordnung

Diese Artefakte zeigen: YAML wird in ThemisDB **nicht nur für interne Settings**, sondern auch für **Schnittstellen- und Infrastrukturverträge** verwendet.

---

## 3.4 Was im aktuellen Stand **nicht** als implementiert gelten sollte

Die frühere Fassung enthielt mehrere Aussagen, die im aktuellen Repository-Stand nicht belastbar genug sind. Für Review-Zwecke sollten diese Punkte **nicht** als bestehende Produktfunktion formuliert werden:

1. **Dokumenten-Metadaten-Schema unter `projects/Themis.DocumentManager/Config/metadata_dokument.yaml`**
   Dieses konkrete Artefakt konnte im aktuellen Repository weder unter `projects/` noch per gezielter Pfadsuche verifiziert werden; für den aktuellen Stand ist es daher als fehlendes bzw. nicht mehr vorhandenes Artefakt zu behandeln.

2. **YAML-basierte Schema-Definition für ThemisDB-Core**
   In `research/schema/README.md` ist die YAML-Schema-Definition weiterhin ausdrücklich als **Konzept** markiert, nicht als implementiertes Feature.

3. **Pauschale Behauptungen wie „alle Konfigurationen haben eine Version“**
   In mehreren YAML-Dateien existieren Versionsfelder, aber nicht konsistent über sämtliche Artefaktklassen hinweg.

4. **Generische Aussagen wie „Hot-Reload-fähig“ oder „Security-by-Default“ für alle YAML-Nutzungen**
   Solche Eigenschaften sind nur dann belastbar, wenn der jeweilige konkrete Codepfad oder die jeweilige Komponente das explizit belegt.

---

## 4. Evaluation / Experimente

Da für die YAML-Nutzung selbst keine dedizierten Benchmark-Artefakte gefunden wurden und Performance-Messungen für diesen Dokumenttyp bewusst nicht Ziel der Untersuchung sind, erfolgt die Evaluation als **Repository-basierte Bestandsaufnahme** statt als Performance-Experiment.

### 4.1 Quantitativer Überblick des gesichteten Bestands

Für den geprüften Snapshot ergab die Sichtung mindestens:

- **181 YAML-Dateien** unter `config/`
- **13 YAML-Dateien** unter `deploy/`
- **2 OpenAPI-YAML-Dateien** unter `docs/` bzw. `openapi/`

Diese Zahlen beweisen noch keine Laufzeitrelevanz im Einzelfall, zeigen aber klar: YAML ist im Repository kein Randphänomen.

### 4.2 Qualitative Bewertung

| Prüffrage | Ergebnis | Begründung |
|---|---|---|
| Ist YAML im Produktkern vorhanden? | **Ja** | Core-, Security-, Ethics- und Retention-Pfade sind direkt im Code referenziert. |
| Wird YAML generisch verarbeitet? | **Ja** | `ConfigSchemaValidator` und `PromptLibraryIO` laden bzw. emittieren YAML. |
| Ist YAML testseitig abgesichert? | **Ja** | Mehrere Tests prüfen YAML-Laden, Parsing und Rundtrips. |
| Ist YAML bereits das deklarative Datenbankschema von ThemisDB? | **Nein, nicht belegt** | Das Schema-README bezeichnet dies weiterhin als Konzept. |
| Liegen belastbare YAML-Performance-Benchmarks vor? | **Nein** | Für diesen Artikel wurden keine direkt zitierbaren Benchmark-Artefakte gefunden. |

### 4.3 Schluss aus der Evaluation

Die belastbare Aussage lautet daher:

> **ThemisDB nutzt YAML heute bereits breit und in mehreren produktionsnahen Codepfaden, aber die YAML-basierte Definition des eigentlichen Datenbankschemas ist nach aktuellem Repository-Stand weiterhin eine Zukunftsidee und kein eingeführter Kernmechanismus.**

---

## 5. Limitations / Known Issues

1. **Repository-Sicht statt Laufzeitbeobachtung:** Diese Analyse beruht auf Code, Konfigurationsdateien und Tests, nicht auf einem vollständigen End-to-End-Betrieb aller Module.
2. **Nicht jede vorhandene YAML-Datei ist automatisch aktiv:** Einige Artefakte sind Beispiele, Profile, Referenzdateien oder Deployment-Beschreibungen.
3. **Kein globaler Hot-Reload-Nachweis:** Einzelne Komponenten sprechen von Reload oder Default-Pfaden; daraus darf kein universelles Verhalten für alle YAML-Artefakte abgeleitet werden.
4. **Keine eigene YAML-Performance-Evaluation:** Aussagen zu Performance, Skalierung oder Parse-Kosten wären ohne dedizierte Messungen spekulativ.
5. **Historische Pfade bleiben sichtbar:** Durch `ConfigPathResolver` existieren Legacy- und Zielpfade nebeneinander. Das erhöht Kompatibilität, erschwert aber eine rein dateibasierte Interpretation des Ist-Stands.

---

## 6. Fazit

Die überarbeitete Bewertung fällt differenziert aus:

- **Ja:** YAML ist in ThemisDB bereits fest verankert — insbesondere für Security-, Compliance-, Core- und Betriebs-Konfigurationen sowie für API-/Deployment-Beschreibungen und einzelne Import-/Exportpfade.
- **Ja:** Diese Nutzung ist durch konkrete Codepfade und Tests belegbar.
- **Nein:** Daraus folgt **nicht**, dass YAML bereits das allgemeine ThemisDB-Schema-, Migrations- oder Index-Management des Datenbankkerns steuert.

Für Folgearbeiten wie `git_gitops_themis_vergleich.md` bedeutet das: YAML-basierte Schema- oder GitOps-Ideen können plausibel an bestehende Muster anschließen, müssen aber weiterhin sauber als **Erweiterung** und nicht als bereits vorhandene Kernfunktion beschrieben werden.

---

## 7. References / Quellen

### Externe Standards und Bibliotheken

1. YAML Language Development Team: **YAML Ain’t Markup Language (YAML) Version 1.2.2**.
   URL: https://yaml.org/spec/1.2.2/
2. jbeder et al.: **yaml-cpp** (C++ YAML parser/emitter).
   URL: https://github.com/jbeder/yaml-cpp
3. OpenAPI Initiative: **OpenAPI Specification**.
   URL: https://spec.openapis.org/oas/latest.html
4. Kubernetes Documentation: **CustomResourceDefinitions**.
   URL: https://kubernetes.io/docs/tasks/extend-kubernetes/custom-resources/custom-resource-definitions/
5. JSON Schema: **Understanding JSON Schema**.
   URL: https://json-schema.org/understanding-json-schema/

### Repository-Artefakte (prüfbarer Snapshot)

6. ThemisDB README — Projektpositionierung als Multi-Model-Datenbank.
   URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/README.md
7. ThemisDB ARCHITECTURE — Modulübersicht inkl. `config/`, `query/`, `server/`, `security/`.
   URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/ARCHITECTURE.md
8. `config/security/pii_patterns.yaml` — PII-Detektionskonfiguration.
   URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/config/security/pii_patterns.yaml
9. `config/data_management/retention_policies.yaml` — Retention-/Compliance-Konfiguration.
   URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/config/data_management/retention_policies.yaml
10. `config/compliance/ethical_guidelines.yaml` — Ethische Leitlinien auf YAML-Basis.
    URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/config/compliance/ethical_guidelines.yaml
11. `src/config/config_schema_validator.cpp` — generisches Laden von YAML/JSON und Schema-Validierung.
    URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/src/config/config_schema_validator.cpp
12. `src/config/config_path_resolver.cpp` — Legacy-/Zielpfad-Mapping für YAML-Konfigurationen.
    URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/src/config/config_path_resolver.cpp
13. `include/security/pii_redaction_policy.h` — Default-Pfad und Reload-Kontext für PII-YAML.
    URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/include/security/pii_redaction_policy.h
14. `include/llm/ethical_guidelines_manager.h` — Default-Pfad für `ethical_guidelines.yaml`.
    URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/include/llm/ethical_guidelines_manager.h
15. `deploy/kubernetes/crds/themisdb.vcc.io_themisdbs.yaml` — deklarativer Kubernetes-Vertrag.
    URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/deploy/kubernetes/crds/themisdb.vcc.io_themisdbs.yaml
16. `docs/openapi.yaml` — OpenAPI Source of Truth.
    URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/docs/openapi.yaml
17. `openapi/openapi.yaml` — weitere OpenAPI-Spezifikation im Repository.
    URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/openapi/openapi.yaml
18. `research/schema/README.md` — Abgrenzung: YAML-Schema-Definition. Derzeit nur Konzept.
    URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/research/schema/README.md
19. `tests/test_config_schema_validator.cpp` — Tests für YAML-Parsing/Schema-Validierung.
    URL: https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/tests/test_config_schema_validator.cpp
20. `tests/test_index_analyzer.cpp` und `tests/test_prompt_library_io.cpp` — Tests für YAML-Konfigurationsladung bzw. YAML-Roundtrip.
    URLs:
    - https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/tests/test_index_analyzer.cpp
    - https://github.com/makr-code/ThemisDB/blob/3af441821703119a9c24107b8d666dcbae9b4956/tests/test_prompt_library_io.cpp
