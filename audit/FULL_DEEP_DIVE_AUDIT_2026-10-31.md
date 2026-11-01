# ThemisDB – Vollständiger Deep-Dive-Audit (Sourcecode & Dokumentation)

**Author:** ThemisDB Team
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Repository-Stand:** Branch `develop` @ `13449dbe6a` (2026-10-31), `VERSION=2.4.0`, `RELEASE_TYPE=alpha`
**Audit-Typ:** Statische Source-/Doku-Analyse + quantitative Metriken + Stichproben-Verifikation (kein neuer Build-/CTest-Lauf)
**Folgedokument:** [ROADMAP.md](ROADMAP.md) (audit-abgeleitete Production-Readiness-Roadmap)

> **AI-Provenienz:** AI-assisted generation; model attribution is approximate and not guaranteed.
> Exact model backend not verifiable from final document text alone. Menschliche Prüfung steht aus (`Human_Review: false`).
>
> **Normativer Rang:** Dieser Bericht ist eine Audit-Bewertung. Release-/GA-Status bleibt laut
> [../ROADMAP.md](../ROADMAP.md) („Canonical Status Authority“) dort verbindlich. Abweichungen zwischen
> diesem Bericht und Root-SOT sind als Findings markiert und müssen dort aufgelöst werden.

---

## 0. Management Summary

### Gesamturteil

**ThemisDB ist in der aktuellen Form (2.4.0-alpha) nicht produktionsreif.** Die Codebasis ist
außergewöhnlich breit (73 Module, ~1,42 Mio. nicht-leere C++-Zeilen in `src/` + `include/`,
~89.000 GTest-Makros) und in mehreren Kernbereichen (Auth, Security, Governance, Plugins,
Netzwerk-Framing, Transaktions-Testinventar) solide. Produktionsreife scheitert aber an
**wenigen, klar benennbaren Klassen von Problemen**:

1. **Durability/Recovery fail-open** – der eigene WAL (`WALStorage`) für Cross-Shard-Transaktionen und
   Replikation bricht Recovery nach 5 Minuten oder bei Magic-Mismatch still ab und der Start läuft weiter;
   ein Test zementiert dieses Verhalten, die Storage-README behauptet das Gegenteil ([STO-01](#sto-01)).
2. **Distributed/HA mit simulierten Erfolgspfaden** – DR-Restore/Traffic-Shift/Metadata melden
   Erfolg ohne Aktion ([DIS-08](#dis-08)); Stream-Shard-Transfer simuliert `PREPARE` ohne Transport
   und es gibt **keinen** Produktions-Caller für den echten Transport ([DIS-03](#dis-03));
   zwei konkurrierende 2PC-Koordinatoren und fragmentierte Recovery ([DIS-05](#dis-05), [DIS-06](#dis-06)).
3. **Unsichere optionale Modi ohne Production-Guard** – unsignierte Update-Manifeste werden
   akzeptiert ([SEC-01](#sec-01)), gRPC-Admin-Port immer Plaintext ([DIS-02](#dis-02)),
   QUIC `SSL_VERIFY_NONE` per Config ([DIS-01](#dis-01)), Shard-RPC fällt bei mTLS-Zertifikatsfehlern auf
   Plaintext zurück und der Produktionsmodus wird inkonsistent erkannt ([SEC-14](#sec-14)),
   unsichere Beispielkonfigurationen.
4. **Fehlende belastbare Test-Evidenz** – der letzte lokale Volllauf (`ctest_full.log`, 2026-10-24)
   ist wegen Infrastrukturfehler ungültig (1.777/1.781 „Failed“ durch `ninja: Permission denied`),
   2.965 `GTEST_SKIP`, 1.020 triviale Asserts, keine Sanitizer-/Fuzz-PR-Gates ([EVD-01](#evd-01), [TST-01](#tst-01), [OPS-06](#ops-06)).
5. **Widersprüchliche Status-Dokumentation** – `CHANGELOG.md` und `SECURITY.md` deklarieren
   2.4.0 als „GA“/„production-ready“, während `VERSION`/`ROADMAP.md` Alpha mit offenen Gates
   ausweisen ([DOC-02](#doc-02)).
6. **Betrieb/K8s nicht HA-fähig** – Helm `replicaCount: 1`, kein PodDisruptionBudget,
   NetworkPolicy deaktiviert, Chart-`appVersion` `1.0.0` ≠ 2.4.0, Runbooks Stand April 2026 ohne
   Übungsnachweis ([OPS-09](#ops-09), [OPS-12](#ops-12), [RUN-01](#run-01)).

### Kennzahlen auf einen Blick

| Dimension | Bewertung (0–5) | Kernaussage |
|---|---:|---|
| Durability & Recovery | 2 | RocksDB-Primärspeicher solide; eigener Tx-/Replikations-WAL mit CRC, aber fail-open Recovery (per Test zementiert); Crash-Tests nur In-Process |
| Verteilte Konsistenz & HA | 2 | Breite 2PC/3PC-Modelltests, aber simulierte Pfade, Doppel-Koordinator, DR-No-ops |
| Security (AuthN/AuthZ/Krypto) | 3,5 | Auth/Security/Governance reif (4); Supply-Chain- und Default-Config-Lücken |
| Query/Search/Ressourcenlimits | 3 | Timeouts/Watchdog vorhanden; LIMIT nach Materialisierung, stille Fallbacks |
| AI/LLM | 2 | Breite Implementierung, aber Real-Model-Tests überwiegend `GTEST_SKIP`, Stub-Backends |
| Betrieb/Deployment (Container/Helm/Runbooks) | 2,5 | Gute Container-/Pod-Security-Defaults; keine HA-Defaults, Versionen inkonsistent |
| CI/Test-Evidenz | 2 | Sehr großes Testinventar, aber keine aktuelle valide Volllauf-Evidenz, tolerante Gates |
| Dokumentation & Governance | 2,5 | 72/73 Module mit Pflicht-Docs; Root-SOT-Widersprüche, Doku-Sprawl |
| **Gesamt (ungewichtet)** | **≈ 2,4** | **Alpha. Single-Node-Core nach P0-Fixes RC-fähig; Multi-Node/HA und AI nicht produktionsreif.** |

### Findings nach Schweregrad (nach Kalibrierung)

| Schweregrad | Anzahl | Davon direkt im Code/Datei verifiziert |
|---|---:|---:|
| 🔴 Critical | 2 | 2 |
| 🟠 High | 22 | 19 |
| 🟡 Medium | 40 | 22 |
| ⚪ Low / Info | 15 | 7 |
| ❌ Verworfen (False Positive) | 5 | 5 |

Nicht als ✅ markierte Findings (🔎) stammen aus Agent-Analysen mit Dateibeleg, wurden aber nicht einzeln
nachgelesen; Details und Umstufungen in [Kapitel 7](#7-kalibrierung--verworfene-und-herabgestufte-befunde).

### Empfohlener Produktions-Zuschnitt

- **Stufe 1 – Single-Node Core RC (Ziel Q1 2027):** `storage`, `transaction` (lokal), `query`/`aql`,
  `index`, `auth`, `security`, `governance`, `server`, `api`, `config`, `observability` – nach Abschluss
  aller P0-Items der [ROADMAP.md](ROADMAP.md).
- **Stufe 2 – Multi-Node Beta (Ziel Q2 2027):** `sharding`, `replication`, `failover`, `network`,
  `rpc_grpc`, `cdc` – erst nach kanonischem Koordinator, Recovery-Vertrag und Multi-Prozess-Chaos-Evidenz.
- **Stufe 3 – AI/LLM als „Preview“ hinter Feature-Flags:** `llm`, `rag`, `llama_cpp`, `whisper`,
  `stable_diffusion`, `onnx_clip`, `training`, `prompt_engineering` u. a. – kein GA-Versprechen,
  bis Real-Model-CI ohne Skips läuft.

---

## 1. Scope, Methodik und Grenzen

### 1.1 Scope

- **Source:** alle 73 Modulverzeichnisse unter [../src/](../src/), zugehörige Header unter
  [../include/](../include/), Tests unter [../tests/](../tests/), Benchmarks unter [../benchmarks/](../benchmarks/).
- **Build/CI/Deploy:** [../CMakeLists.txt](../CMakeLists.txt), [../vcpkg.json](../vcpkg.json), 99 Workflows unter
  [../.github/workflows/](../.github/workflows/), Docker-Dateien, [../helm/themisdb/](../helm/themisdb/), `config/`.
- **Dokumentation:** Root-SOT ([../ROADMAP.md](../ROADMAP.md), [../CHANGELOG.md](../CHANGELOG.md),
  [../SECURITY.md](../SECURITY.md), [../RELEASE_STRATEGY.md](../RELEASE_STRATEGY.md),
  [../VERSIONING.md](../VERSIONING.md), [../DOCUMENTATION_GOVERNANCE.md](../DOCUMENTATION_GOVERNANCE.md)),
  Modul-Docs (`README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`), `docs/`, `audit/`,
  Developer-LLM-Wiki (`ai_context/developer_llm_wiki/`, Stand `WIKI_STATUS.json` 2026-10-31, 9.419 Quellen).

### 1.2 Methodik

1. **Kontextaufnahme** über Developer-LLM-Wiki (`INDEX.md`, `SOLL_IST_GAP_SUMMARY.md`) und bestehenden Audit-Stack
   (letzter Implementierungsaudit: [IMPLEMENTATION_AUDIT_2026-09-30.md](IMPLEMENTATION_AUDIT_2026-09-30.md)).
2. **Sieben parallele Cluster-Deep-Dives** (read-only): Storage/Transaktionen, Distributed/HA, Server/Security,
   Query/Search, AI/LLM, Build/CI/Ops, Dokumentation.
3. **Quantitative Vollerhebung** per Skript über `src/`, `include/`, `tests/`, `benchmarks/`
   (Regex-Marker; siehe [Anhang A](#anhang-a--reproduzierbarkeit)).
4. **Stichproben-Verifikation** aller Critical- und der meisten High-Findings durch direktes Lesen der Fundstellen.
5. **Kalibrierung:** Agent-Findings wurden gegen Policy-Dateien und Code gegengeprüft; False Positives
   verworfen, Schweregrade angepasst ([Kapitel 7](#7-kalibrierung--verworfene-und-herabgestufte-befunde)).

**Schweregrad-Definition**

| Grad | Kriterium |
|---|---|
| 🔴 Critical | In Default-/Produktionskonfiguration erreichbar **und** führt zu stillem Datenverlust oder stillem Falsch-Erfolg einer sicherheits-/verfügbarkeitskritischen Operation |
| 🟠 High | Erreichbar unter realistischer Konfiguration; Datenintegritäts-, Sicherheits- oder Verfügbarkeitsrisiko; oder Release-Gate nicht nachweisbar |
| 🟡 Medium | Begrenzter Blast-Radius, Defense-in-Depth-Lücke, Test-/Doku-Lücke mit Produktionsrelevanz |
| ⚪ Low/Info | Hygiene, Konsistenz, toter Code, Empfehlungen |

**Reifeskala (Module)**

| Stufe | Bedeutung |
|---:|---|
| 0 | Nicht vorhanden / leer |
| 1 | Prototyp oder Docs-only |
| 2 | Funktional, aber erreichbare Stubs/Simulationen oder kritische Lücken |
| 3 | Funktional und getestet; Produktionsnachweise (Last, Recovery, Chaos) fehlen |
| 4 | Gehärtet, breite Negativtests, nur kleinere Lücken |
| 5 | Produktionsbewährt mit reproduzierbarer Evidenz |

### 1.3 Grenzen

- **Kein Build/CTest in diesem Audit.** Aussagen zur Testlage basieren auf Inventar und dem vorhandenen
  Log `ctest_full.log` (2026-10-24), das sich als **ungültig** herausstellte ([EVD-01](#evd-01)).
- **Keine dynamischen Sicherheitstests** (Pentest/Fuzzing nicht ausgeführt).
- **Regex-Marker sind Heuristiken:** Zählungen (`stub`, `simulat`, `catch(...)` …) enthalten Kommentare und
  dokumentierte, bewusst zulässige Fallbacks; sie dienen der Priorisierung, nicht als Defektanzahl.
- **Testdatei-Zählung je Modul** erfasst nur `tests/<modul>/`; Tests in `tests/` (Root) oder anderen
  Ordnern sind nur in den Gesamtzahlen enthalten.
- Findings mit Verifikationsstatus 🔎 sind von den Cluster-Analysen mit Fundstelle/Zitat belegt, aber nicht
  zusätzlich durch den Hauptauditor nachgelesen worden.

---

## 2. Quantitative Basis

### 2.1 Gesamtumfang

| Kennzahl | Wert |
|---|---:|
| Getrackte Dateien im Repository | 65.540 |
| davon unter `ai_context/` | 41.611 (63 %) |
| Modulverzeichnisse unter `src/` | 73 |
| `src/` C/C++-Dateien / nicht-leere LOC | 1.862 / 966.803 |
| `include/` C/C++-Dateien / nicht-leere LOC | 2.140 / 452.567 |
| `tests/` C/C++-Dateien (gesamt) / `TEST*`-Makros | 4.319 / 89.291 |
| davon `tests/legacy/` (per CMake-Filter ausgeschlossen) | 1.018 Dateien / 21.644 `TEST*` |
| Modul-zugeordnete Tests (`tests/<modul>/`) | 1.530 Dateien / 497.890 LOC / 28.969 `TEST*` |
| `GTEST_SKIP` (gesamt `tests/`) | 2.965 |
| Triviale Asserts (`EXPECT_TRUE(true)`, `SUCCEED()`) (gesamt) | 1.020 |
| Deaktivierte Testdateien (`*.skip`, `*.old`) | 5 |
| Benchmark-Dateien (Modul-zugeordnet) | 339 |
| In CTest registrierte Tests (Build vom 2026-10-24) | 1.781 |
| GitHub-Workflows | 99 |
| Modul-Pflichtdokumente vollständig (README/ARCHITECTURE/ROADMAP/FUTURE_ENHANCEMENTS) | 72/73 |
| Markdown-Dateien unter `docs/` | 5.438 (Großteil `docs/ARCHIVED/`) |

### 2.2 Risiko-Marker in `src/` + `include/` (heuristisch)

| Marker | Treffer | Einordnung |
|---|---:|---|
| `stub` | 791 | Großteil dokumentierte Hardware-/Optional-Fallbacks; Hotspots: `security` 145, `llama_cpp` 79, `llm` 61, `whisper` 61, `acceleration` 47, `tensor` 40 |
| `simulat*` | 485 | Hotspots: `llm` 67, `rag` 63, `security` 45, `sharding` 43 |
| `placeholder` | 333 | Hotspot: `prompt_engineering` 42 (meist Template-Platzhalter, fachlich legitim) |
| `not (yet) implemented` | 190 | Hotspot: `chimera` 95 |
| `catch (...)` | 1.435 | Hotspots: `server` 240, `llm` 146, `index` 113, `query` 100 |
| roher `new` / `delete` | 152 / 340 | Policy: nur Review-Exception (siehe [../.github/copilot-instructions.md](../.github/copilot-instructions.md) §8) |
| `.detach()` | 43 | Lebensdauer-Risiko (vgl. [STO-04](#sto-04)) |
| `sleep_for`/`Sleep` | 354 | Teilweise Retry-Backoff in Worker-Threads (vgl. [DIS-14](#dis-14)) |
| `reinterpret_cast` | 1.200 | Überwiegend Serialisierung/I/O; kein Einzelbefund |
| TLS-„insecure“-Muster | 165 | Überwiegend Warn-/Guard-Code; reale Befunde: [DIS-01](#dis-01), [DIS-02](#dis-02) |
| `system()`/`popen()` | 12 | Kein verifizierter Injektionspfad in diesem Audit |
| `rand()` | 3 | Nicht sicherheitsrelevant verifiziert (vgl. [SEC-08](#sec-08)) |
| `TODO/FIXME/XXX/HACK` | 21 | Sehr niedrig – Lücken sind als „stub/simulation“ markiert statt als TODO |

### 2.3 Test-Dichte-Ausreißer (src-LOC vs. Testumfang)

| Modul | src-LOC | Testdateien | `TEST*` | Auffälligkeit |
|---|---:|---:|---:|---|
| `server` | 90.590 | 24 | 509 | Größtes Risiko-Verhältnis bei zweitgrößtem Modul; 240 `catch(...)` |
| `distributed_tensor` | 9.798 | 4 | 29 | Kaum Tests für verteilte Zustände; Manifest in-memory |
| `prompt_engineering` | 14.091 | 7 | 81 | Großer Code, dünne Verhaltensprüfung |
| `timeseries` | 8.437 | 7 | 142 | Retention/Compaction ungetestet |
| `scheduler` | 7.744 | 6 | 60 | Verteilte Fehlerfälle ungetestet |
| `whisper` | 2.465 | 2 | 6 | 61 Stub-Marker |
| `llama_cpp` | 2.801 | 5 | 23 | 79 Stub-Marker |
| `analytics` | 30.901 | 42 | 1.189 | 90 triviale Asserts |
| `gpu` | 13.011 | 63 | 1.398 | 90 `GTEST_SKIP` |
| `llm` | 108.388 | 130 | 2.005 | 258 `GTEST_SKIP` |

---

## 3. Reife-Matrix aller 73 Module

Legende: LOC = nicht-leere Zeilen in `src/<modul>`; T-Dat. = Dateien in `tests/<modul>`; Skips = `GTEST_SKIP`;
Reife nach Skala aus Kapitel 1.2. Findings siehe Kapitel 5.

### 3.1 Core Storage & Transaktionen

| Modul | LOC | T-Dat. | `TEST*` | Skips | Reife | Hauptrisiko / Findings |
|---|---:|---:|---:|---:|---:|---|
| `storage` | 39.186 | 21 | 214 | 9 | 3 | Fail-open WAL-Recovery ([STO-01](#sto-01)); keine E2E-Crash-/Restore-Tests ([STO-07](#sto-07)) |
| `transaction` | 12.582 | 33 | 596 | 3 | 3 | Starkes Isolation-/2PC-Testinventar; verteilter Pfad hängt an `sharding` ([DIS-05](#dis-05)) |
| `index` | 37.895 | 31 | 502 | 27 | 3 | 113 `catch(...)`; Rebuild-/Konsistenz-Evidenz fehlt |
| `cache` | 9.287 | 15 | 392 | 3 | 3 | Redis-Koordinator ohne hiredis = No-op ([STO-03](#sto-03)) |
| `core` | 3.297 | 8 | 63 | 0 | 3 | Abhängig von optionalem Backend-Wiring |
| `base` | 6.862 | 8 | 199 | 0 | 3 | Wenig Stress-Evidenz |
| `document` | 140 | 6 | 151 | 0 | 2 | Sehr kleine Implementierung vs. API-Umfang (2.219 Header-LOC) |
| `metadata` | 5.668 | 34 | 503 | 0 | 3 | Verteilte Persistenzgarantien unbelegt |
| `cdc` | 5.580 | 35 | 686 | 20 | 2 | Kafka nur mit `THEMIS_ENABLE_KAFKA` ([STO-02](#sto-02)); Outbox-Tests auf Windows übersprungen ([STO-06](#sto-06)) |
| `temporal` | 8.064 | 20 | 564 | 0 | 3 | Crash-/Recovery-Garantien unbelegt |
| `timeseries` | 8.437 | 7 | 142 | 0 | 2 | Dünne Tests ([STO-08](#sto-08)) |
| `maintenance` | 2.076 | 7 | 36 | 0 | 2 | Detached Thread mit `this` ([STO-04](#sto-04)); AuthZ extern ([STO-09](#sto-09)) |

### 3.2 Distributed / HA / Netzwerk

| Modul | LOC | T-Dat. | `TEST*` | Skips | Reife | Hauptrisiko / Findings |
|---|---:|---:|---:|---:|---:|---|
| `sharding` | 57.215 | 30 | 600 | 0 | 2 | Stream-`PREPARE` simuliert ([DIS-03](#dis-03)); Doppel-Koordinator ([DIS-05](#dis-05)); In-Process-RPC ([DIS-04](#dis-04)) |
| `replication` | 12.802 | 17 | 510 | 4 | 3 | Multi-Node-/Crash-Evidenz fehlt; Recovery-Vertrag fragmentiert ([DIS-06](#dis-06)) |
| `failover` | 1.759 | 12 | 111 | 0 | 2 | DR-Handler melden Erfolg ohne Aktion ([DIS-08](#dis-08)); Spare-Rückgabe ignoriert ([DIS-07](#dis-07)) |
| `network` | 17.414 | 75 | 1.320 | 10 | 3 | Gute Framing-/Größenvalidierung; QUIC-Verify abschaltbar ([DIS-01](#dis-01)) |
| `rpc_grpc` | 1.374 | 4 | 41 | 1 | 2 | Admin-Port immer Plaintext ([DIS-02](#dis-02)) |
| `scheduler` | 7.744 | 6 | 60 | 2 | 2 | Keine verteilten Fehlerfall-Tests |
| `distributed_knowledge` | 1.841 | 9 | 107 | 0 | 2 | Prototyp; keine Partitionstests |
| `distributed_tensor` | 9.798 | 4 | 29 | 0 | 2 | Manifest-Store in-memory ([DIS-13](#dis-13)) |
| `chaos` | 325 | 8 | 118 | 1 | 2 | Kein Multi-Prozess-Chaos ([DIS-11](#dis-11)) |

### 3.3 Server, API, Security, Governance, Plattform

| Modul | LOC | T-Dat. | `TEST*` | Skips | Reife | Hauptrisiko / Findings |
|---|---:|---:|---:|---:|---:|---|
| `server` | 90.590 | 24 | 509 | 4 | 3 | Niedrige Testdichte; 240 `catch(...)`; routenweiter AuthZ-Nachweis fehlt ([SEC-13](#sec-13)) |
| `api` | 6.507 | 16 | 346 | 5 | 3 | Routenweite AuthZ-Tests fehlen |
| `auth` | 18.911 | 26 | 508 | 38 | 4 | JWT-Security-Testdatei deaktiviert ([SEC-12](#sec-12)); PRNG in SAML ([SEC-09](#sec-09)) |
| `security` | 24.151 | 92 | 1.738 | 67 | 4 | 145 Stub-Marker (HSM/PKI-Fallbacks) – Produktions-Guard prüfen |
| `user_storage_encrypted` | 2.854 | 8 | 89 | 8 | 3 | Key-Lifecycle-Evidenz begrenzt |
| `config` | 4.354 | 10 | 402 | 3 | 3 | Unsichere Beispiel-Configs ([SEC-05](#sec-05)–[SEC-07](#sec-07)) |
| `governance` | 20.659 | 16 | 290 | 0 | 4 | Stark; Deploy-Garantien offen |
| `observability` | 13.631 | 17 | 236 | 0 | 3 | Helm-ServiceMonitor default aus ([OPS-16](#ops-16)) |
| `utils` | 24.433 | 32 | 252 | 6 | 3 | Audit-Logger-Pfade in Fehlermeldungen ([SEC-04](#sec-04)) |
| `access_model` | 1.634 | 7 | 94 | 1 | 3 | Querschnitts-Enforcement nicht hier belegt |
| `plugins` | 5.773 | 25 | 440 | 9 | 4 | Signatur-/Hash-Prüfung vorhanden |
| `updates` | 12.555 | 12 | 257 | 0 | 2 | Unsignierte Manifeste akzeptiert ([SEC-01](#sec-01)) |
| `themis` | 8.190 | 5 | 142 | 1 | 3 | Secure-Default-/Shutdown-Evidenz unvollständig |

### 3.4 Query, Search, Analytics, Beschleunigung

| Modul | LOC | T-Dat. | `TEST*` | Skips | Reife | Hauptrisiko / Findings |
|---|---:|---:|---:|---:|---:|---|
| `query` | 43.929 | 61 | 1.010 | 4 | 3 | LIMIT nach Vollmaterialisierung ([QRY-01](#qry-01)); Heuristik-Kardinalitäten ([QRY-08](#qry-08)) |
| `aql` | 11.373 | 72 | 1.630 | 19 | 3 | `std::stoi` auf User-Input ([QRY-02](#qry-02)); Feature-Skips ([QRY-05](#qry-05)) |
| `search` | 5.569 | 16 | 211 | 0 | 3 | Timeout/Cancellation laut eigener Roadmap unvollständig ([QRY-07](#qry-07)) |
| `retrieval` | 1.473 | 5 | 30 | 0 | 2 | Prototyp |
| `graph` | 13.294 | 36 | 1.045 | 13 | 3 | Legacy-Pfadtests übersprungen ([QRY-06](#qry-06)) |
| `geo` | 9.036 | 41 | 854 | 43 | 3 | Viele Skips; Scale-Evidenz unklar |
| `analytics` | 30.901 | 42 | 1.189 | 16 | 3 | 90 triviale Asserts ([TST-01](#tst-01)) |
| `execution` | 419 | 4 | 28 | 0 | 2 | Kritische Schicht mit minimalem Code/Test |
| `vector_search` | 0 | 1 | 3 | 0 | 1 | Docs-only Modulpfad (Implementierung in `index`/`rag`) |
| `tensor` | 9.839 | 23 | 516 | 0 | 2 | Lexikalischer Embedding-Fallback ([QRY-03](#qry-03)) |
| `chimera` | 6.235 | 6 | 152 | 0 | 2 | 95 „not implemented“-Marker |
| `process` | 14.399 | 26 | 410 | 0 | 3 | Scale-Evidenz begrenzt |
| `projects` | 1.691 | 3 | 56 | 0 | 2 | Dünne Tests |
| `toolbox` | 1.792 | 6 | 117 | 1 | 2 | Edge-Case-Tests begrenzt |
| `scraper` | 3.339 | 8 | 93 | 4 | 2 | Ressourcen-/SSRF-Grenzen unbelegt |
| `acceleration` | 26.485 | 16 | 401 | 17 | 3 | 47 Stub-Marker; Fallback-Korrektheit belegen |
| `gpu` | 13.011 | 63 | 1.398 | 90 | 3 | CPU-Fallback meldet `true` ([QRY-04](#qry-04)) |
| `performance` | 10.952 | 26 | 392 | 20 | 3 | Hardware-Evidenz uneinheitlich |

### 3.5 AI / LLM / Content

| Modul | LOC | T-Dat. | `TEST*` | Skips | Reife | Hauptrisiko / Findings |
|---|---:|---:|---:|---:|---:|---|
| `llm` | 108.388 | 130 | 2.005 | 258 | 3 | Real-Model-Tests geskippt ([AI-03](#ai-03)); globale Token-/Admission-Limits ([AI-07](#ai-07), [AI-09](#ai-09)) |
| `rag` | 38.184 | 70 | 1.299 | 16 | 3 | Embedding-Integration umgebungsabhängig |
| `llama_cpp` | 2.801 | 5 | 23 | 0 | 2 | 79 Stub-Marker; keine Model-Load-Evidenz |
| `llm_wiki` | 2.460 | 4 | 25 | 4 | 2 | RocksDB-abhängige Tests geskippt |
| `llm_l_wiki` | 0 | 0 | 0 | 0 | 0 | Leere Datei, nur `ARCHITECTURE.md` ([DOC-10](#doc-10)) |
| `llm_streaming` | 0 | 1 | 3 | 0 | 1 | Docs-only Modulpfad |
| `training` | 10.988 | 23 | 375 | 0 | 2 | Ressourcen-/Safety-Grenzen unbelegt |
| `prompt_engineering` | 14.091 | 7 | 81 | 0 | 2 | Dünne Verhaltensprüfung |
| `evaluation` | 4.437 | 5 | 50 | 0 | 2 | Infrastruktur-/Modellabhängig |
| `ethics_ai` | 7.194 | 10 | 69 | 4 | 2 | Enforcement am Inferenz-Rand unbelegt |
| `ai` | 1.154 | 4 | 54 | 0 | 2 | Integrationsschicht, wenig Produktionspfad-Tests |
| `ai_working` | 0 | 0 | 0 | 0 | 1 | Docs-only, nicht normativ |
| `voice` | 12.590 | 32 | 879 | 4 | 3 | Externe Backends konditional |
| `whisper` | 2.465 | 2 | 6 | 0 | 2 | Stub-Modus ohne Prod-Guard ([SEC-10](#sec-10)) |
| `stable_diffusion` | 1.971 | 5 | 25 | 0 | 2 | Ressourcen-/Security-Validierung begrenzt |
| `onnx_clip` | 998 | 4 | 39 | 13 | 2 | Real-Model-Tests begrenzt |
| `image_analysis` | 975 | 2 | 19 | 0 | 2 | Prototyp |
| `content` | 21.196 | 28 | 673 | 14 | 3 | Untrusted-Format-Härtung belegen |
| `ingestion` | 18.810 | 35 | 832 | 6 | 3 | Parser-/HTTP-Missbrauchsfälle |
| `importers` | 22.590 | 12 | 319 | 0 | 2 | Parser-/Remote-Input-Security dünn getestet |
| `exporters` | 8.186 | 16 | 411 | 12 | 3 | Large-Output-Tests fehlen |

### 3.6 Verteilung der Reifegrade

| Reife | Anzahl Module | Module |
|---:|---:|---|
| 0 | 1 | `llm_l_wiki` |
| 1 | 3 | `vector_search`, `llm_streaming`, `ai_working` (alle docs-only) |
| 2 | 32 | u. a. `sharding`, `failover`, `rpc_grpc`, `updates`, `cdc`, `timeseries`, `tensor`, `llama_cpp`, `whisper`, `importers` |
| 3 | 33 | u. a. `storage`, `transaction`, `query`, `aql`, `server`, `network`, `replication`, `llm`, `rag`, `gpu` |
| 4 | 4 | `auth`, `security`, `governance`, `plugins` |
| 5 | 0 | – |

---

## 4. Querschnittsbewertung nach Dimensionen

### 4.1 Durability & Recovery (2/5)

- **Positiv:** WAL-Replay prüft Magic, Längen und CRC ([../src/storage/wal_storage.cpp](../src/storage/wal_storage.cpp));
  Produktions-Fail-closed-Guards sind in `src/storage/README.md`/`ROADMAP.md` dokumentiert.
- **Negativ:** Replay bricht nach `RECOVERY_TIMEOUT = 5 min` mit **WARN** ab und der Start läuft weiter
  (`wal_storage.cpp:494-504`); ein Magic-Mismatch mitten im Segment wird wie ein Tail-Truncate behandelt
  (`wal_storage.cpp:514`, stilles `break`). Betroffen sind der Cross-Shard-Tx-WAL und der Replikations-WAL
  ([STO-01](#sto-01)). Crash-/Restore-Tests existieren unter `tests/` (Root) und `tests/integration/`, laufen aber
  In-Process; `tests/test_wal_storage.cpp:290-316` erwartet das stille Abbrechen ausdrücklich ([STO-07](#sto-07)).

### 4.2 Verteilte Konsistenz & HA (2/5)

- **Positiv:** umfangreiche modellierte 2PC/3PC-, Partition- und Coordinator-Crash-Szenarien; Fail-fast-Validierung bei
  verteilten Transaktionen; Shard-RPC verweigert In-Process-Simulation für Nicht-`inproc://`-Endpunkte
  (`shard_rpc_client.cpp:1050-1055`).
- **Negativ:** `StreamSession::initialize()` liefert ohne injizierten Transport `true` (`stream_protocol.cpp:693-710`) und
  `setPrepareTransferCallback()` wird in `src/` **nirgends** aufgerufen → stream-basierter Shard-Transfer ist in Produktion
  faktisch simuliert. DR-Handler (Restore, Traffic-Shift, Metadata) melden Erfolg ohne Adapteraufruf
  ([../src/failover/PRODUCTION_REQUIREMENTS.md](../src/failover/PRODUCTION_REQUIREMENTS.md) Z. 43-50). Root-Roadmap
  dokumentiert Doppel-Koordinator (`CrossShardTransactionCoordinator` vs. `TwoPhaseCommitCoordinator`) und fragmentierte
  Recovery (Issue #5373). Tests simulieren Partitionen per Bool-Flag statt realer Netzfehler.

### 4.3 Security (3,5/5)

- **Positiv:** `auth`, `security`, `governance`, `plugins` auf Reife 4; Constant-Time-Vergleiche (`CRYPTO_memcmp`),
  Auth-Middleware fail-closed (`auth_middleware.cpp:505-518` → `Denied`), Plugin-Hash-/Signaturprüfung,
  Audit-Logger mit Hash-Chain, QUIC-Default `verify_tls = true` (`include/network/quic_server.h:395`).
- **Negativ:** Update-Manifeste nur geprüft, wenn Verifier konfiguriert **und** Signatur nicht leer
  (`manifest_database.cpp:272`); gRPC-Admin-Port bindet immer `InsecureServerCredentials()`
  (`grpc_plugin.cpp:216-228`); Shard-RPC-Server fällt bei mTLS-Zertifikatsfehler auf Plaintext zurück und
  `THEMIS_PRODUCTION_MODE` wird an ≥ 7 Stellen unterschiedlich geparst ([SEC-14](#sec-14)); TSA-Zertifikatsprüfung in
  mitgelieferter Config aus (`config/timestamp_authority.yaml:64`); Beispiel-Configs binden `0.0.0.0` ohne TLS.

### 4.4 Query/Search & Ressourcenlimits (3/5)

- **Positiv:** Watchdog-/Timeout-Arbeit mit Abschlussnachweis (`src/query/WAVE_3B_CLOSURE_EVIDENCE.md`),
  Degradations-Metriken in Search/GPU, Tiefenvalidierung in AQL.
- **Negativ:** `LIMIT` erst nach allen Operatoren (`vectorized_execution.cpp:274-277`), stille semantische Fallbacks
  (`tensor/utr_converter.cpp:294-306`), Kardinalitäts-Defaults (`query_optimizer.cpp:490-552`), Search-Timeouts laut
  `src/search/ROADMAP.md` nur `[~]`.

### 4.5 AI/LLM (2/5)

- **Positiv:** breite Implementierung (108k LOC `llm`, 38k `rag`), dokumentierte Cancellation/Lifecycle,
  explizite Unterscheidung „Modell nicht verfügbar“ statt Scheinerfolg in Tests.
- **Negativ:** 258 `GTEST_SKIP` allein in `tests/llm` (u. a. TinyLlama, Real-Embeddings, LoRA); Stub-Backends in
  `llama_cpp`/`whisper`/`stable_diffusion` ohne Produktions-Guard; Behauptung „1024 concurrent requests“
  (`src/llm/THREADING.md:493`) ohne reproduzierbares Artefakt.

### 4.6 Betrieb & Deployment (2,5/5)

- **Positiv:** Non-root-Container mit Healthchecks; Helm mit `securityContext`, read-only RootFS, Limits, Probes;
  HPA- und NetworkPolicy-Templates vorhanden; Runbooks unter `docs/production/RUNBOOKS/`.
- **Negativ:** `replicaCount: 1`, kein PDB-Template, NetworkPolicy `enabled: false`, ServiceMonitor default aus
  (Metrik-Endpoint selbst aktiv),
  Chart `version: 0.2.0`/`appVersion: "1.0.0"`, `debian/changelog` fehlt, Runbooks/DRP Stand April 2026 ohne Übungsnachweis.

### 4.7 CI & Test-Evidenz (2/5)

- **Positiv:** 99 Workflows mit Top-Level-`permissions` in allen Dateien, keine `@main`/`@master`-Actions,
  Release-Evidence-Chain, Sanitizer-Presets (ASan/UBSan/TSan), Fuzz-Targets.
- **Negativ:** Kein gültiger aktueller Volllauf ([EVD-01](#evd-01)); kein Sanitizer- oder Fuzz-Lauf auf PRs
  (`build-clang-fast.yml:5` „without sanitizer overhead“; `security-fuzzing.yml` nur `schedule`/`workflow_dispatch`);
  26× `continue-on-error: true` in 15 Workflows, 149× `|| true`; einzelne Gate-Workflows tolerieren Fehler.

### 4.8 Dokumentation & Governance (2,5/5)

- **Positiv:** 72/73 Module mit vollständigen Pflicht-Docs; Modul-Roadmaps strukturkonform (Root-Sync 71/71);
  Developer-LLM-Wiki aktuell (2026-10-31); Audit-Hub mit klarer Kanonik.
- **Negativ:** Root-SOT widersprüchlich (GA vs. Alpha), Roadmap 233 KB mit zwei konkurrierenden Prozentwerten
  (~72 % vs. ~82–84 %, Stand 2026-09-09), `CHANGELOG.md` nicht chronologisch, Runbooks nicht aus dem Onboarding verlinkt,
  `docs/` von Archivmaterial dominiert.

---

## 5. Findings-Register

Spalten: **Verif.** ✅ = vom Hauptauditor direkt an der Fundstelle nachgelesen; 🔎 = von der Cluster-Analyse mit
Fundstelle/Zitat belegt, nicht zusätzlich nachgelesen. **Aufwand:** S ≤ 3 PT, M ≤ 2 PW, L > 2 PW.
Zusammengeführte Agent-IDs sind in Klammern angegeben.

### 5.1 🔴 Critical

#### <a id="sto-01"></a>STO-01 – WAL-Recovery ist fail-open (Timeout und Mid-Log-Korruption) ✅

- **Fundstelle:** `src/storage/wal_storage.cpp:490-514` (`replaySegment`)
- **Betroffene Nutzer:** `src/sharding/wal_manager.cpp:147` (Cross-Shard-Transaktions-WAL) und
  `src/replication/replication_manager.cpp:685` (Replikations-Durability-WAL; überspringt zusätzlich nicht
  deserialisierbare Einträge mit WARN, `:670-674`). Der RocksDB-basierte Single-Node-Primärspeicher nutzt den
  RocksDB-eigenen WAL und ist **nicht** betroffen.
- **Befund:** `RECOVERY_TIMEOUT = 5 min`; bei Überschreitung wird nur `THEMIS_WARN(... "Incomplete recovery is better than
  hung startup")` geloggt und die Schleife verlassen – der Prozess startet mit unvollständigem Zustand. Ein Magic-Mismatch
  (`hdr.magic != WAL_MAGIC`) beendet die Replay-Schleife ebenfalls still, d. h. Korruption **mitten im Log** wird wie ein
  harmloser Tail-Truncate behandelt; alle nachfolgenden, bereits bestätigten Records gehen verloren.
- **Zementiert durch Test:** `tests/test_wal_storage.cpp:290-316` (`CorruptedEntry_StopsReplay`) erwartet ausdrücklich
  „Recovery should silently stop at the corrupted entry – not crash“.
- **Widerspruch zur Modul-Doku:** `src/storage/README.md:67` („✅ Fail-closed behavior is implemented and chaos-tested for
  crash recovery“) und `:122` („On recovery failure: Startup fails fast … no silent corruption“).
- **Produktionswirkung:** Nach Crash/Neustart können committete Koordinator-Entscheidungen bzw. Replikations-Log-Einträge
  still fehlen → In-Doubt-Transaktionen werden nicht aufgelöst (Atomizitätsverletzung über Shards), Replikate
  divergieren; kein Operator-Signal außer einer WARN-Zeile.
- **Empfehlung:** (1) Recovery-Ergebnis als Status (`Complete | TruncatedTail | Corrupted | TimedOut`) zurückgeben;
  (2) in Produktionsprofil bei `Corrupted`/`TimedOut` **Start verweigern** (Readiness = false) bzw. nur mit explizitem
  `--wal-recovery-mode=skip-corrupted` und Quarantäne des Segments; (3) Tail-Truncate nur am Segmentende akzeptieren;
  (4) Timeout konfigurierbar, Default „unbegrenzt mit Fortschrittsmetrik“; (5) Prozess-Crash-Tests (siehe [STO-07](#sto-07)).
- **Aufwand:** M

#### <a id="dis-08"></a>DIS-08 – Failover/DR-Handler melden Erfolg ohne Aktion (DIS-08, DIS-09) ✅

- **Fundstelle:** `src/failover/PRODUCTION_REQUIREMENTS.md:43-50` (Selbstauskunft des Moduls), Handler in `src/failover/`
- **Befund:** Restore- und Traffic-Shift-Handler geben Erfolg zurück, ohne Storage- oder Traffic-Adapter aufzurufen; das
  Metadata-Update loggt und gibt `true` zurück, ohne zu schreiben; Catch-up prüft Quorum, nicht die Replikationsposition.
- **Produktionswirkung:** Ein DR-Ablauf kann „abgeschlossen“ melden, obwohl weder Daten wiederhergestellt noch Traffic
  umgeschwenkt wurde – Split-Brain- bzw. Datenverlustrisiko bei echtem Ausfall.
- **Empfehlung:** Handler ohne konfigurierten Adapter **fail-closed** (`NotConfigured`-Fehler statt Erfolg); Erfolg nur bei
  Adapter-ACK plus Verifikation (Checksumme/Positionsvergleich); Catch-up an LSN/Term koppeln; negative Tests für
  fehlende Adapter, fehlendes Quorum und fehlendes Fencing ([DIS-15](#dis-15)).
- **Aufwand:** L

### 5.2 🟠 High

| ID | Bereich | Fundstelle | Befund | Empfehlung | Aufw. | Verif. |
|---|---|---|---|---|---|---|
| <a id="dis-03"></a>DIS-03 | Distributed / Stub | `src/sharding/stream_protocol.cpp:663-710`, `:912` | `StreamSession::initialize()` liefert ohne `prepare_transfer_cb_` per In-Process-Simulation `true`. `setPrepareTransferCallback()` hat in `src/` **keinen** Aufrufer → Stream-Shard-Migration ist in Produktion simuliert. | Transport-Callback bei Rebalancer-Konstruktion verpflichtend verdrahten; ohne Callback `false` + Fehlerstatus; Simulation nur hinter Test-Build-Flag. | M | ✅ |
| <a id="dis-05"></a>DIS-05 | Distributed / Konsistenz | `ROADMAP.md:636-684` (Issue #5373) | Zwei 2PC-Koordinatoren (`CrossShardTransactionCoordinator` vs. `TwoPhaseCommitCoordinator`) mit unterschiedlicher Semantik; Issue als erledigt markiert, Risiko offen dokumentiert. | Einen kanonischen Koordinator festlegen, zweiten entfernen oder als Adapter darauf abbilden; Contract-Tests für beide Aufrufpfade. | L | ✅ |
| <a id="dis-06"></a>DIS-06 | Distributed / Recovery | `ROADMAP.md:636-684` | Recovery-Vertrag fragmentiert (Koordinator-Log, Participant-WAL, Replikation); WAL-Policy-Drift dokumentiert. | Einheitliches Recovery-Protokoll (Coordinator-Decision-Log persistent, Participant-In-Doubt-Resolution, Timeouts) spezifizieren und mit Multi-Prozess-Crash-Tests belegen. | L | ✅ |
| <a id="dis-07"></a>DIS-07 | Failover | `src/failover/auto_failover_manager.cpp:613-614` | Rückgabewert von `activateSpareIfNeeded()` wird ignoriert; Failover läuft ohne Kapazitätsersatz weiter. | Rückgabe auswerten, Failover-Status `Degraded` setzen, Alarm/Metrik emittieren. | S | ✅ |
| <a id="dis-01"></a>DIS-01 | Netzwerk / TLS | `src/network/quic_server.cpp:809-814`; Default `include/network/quic_server.h:395` | `verify_tls=false` setzt `SSL_VERIFY_NONE` (Default ist `true`); kein Produktions-Guard. | Im Produktionsprofil `verify_tls=false` beim Start ablehnen; nur mit Dev-Flag erlauben; Test für Ablehnung. | S | ✅ |
| <a id="dis-02"></a>DIS-02 | gRPC / TLS | `src/rpc_grpc/grpc_plugin.cpp:216-228`, `:743-747`; `src/rpc_grpc/TRUST.md:32-58` | Admin-Listener nutzt **immer** `grpc::InsecureServerCredentials()`, auch wenn der Hauptlistener TLS nutzt. Hauptlistener ohne TLS nur mit Warnung. | Admin-Listener mit denselben (m)TLS-Credentials; Plaintext nur bei Bind auf Loopback; Produktionsprofil verweigert Plaintext. | M | ✅ |
| <a id="sec-01"></a>SEC-01 | Supply Chain | `src/updates/manifest_database.cpp:272` | Signaturprüfung nur, wenn `verifier_` gesetzt **und** `manifest.signature` nicht leer → unsignierte Manifeste werden akzeptiert (nur Hash). | Fail-closed: fehlender Verifier oder leere Signatur = Ablehnung; gepinnte Trust-Anchors; Negativtests (unsigniert, unbekannter Signer, abgelaufen). | M | ✅ |
| <a id="sec-14"></a>SEC-14 | Transport-Security / Produktionsmodus *(neu)* | `src/sharding/shard_rpc_server.cpp:320-326`, `:330`; `src/sharding/shard_rpc_client.cpp:224-226`; `include/core/production_mode.h:28-50`; `include/security/transport_security_checker.h:37-65`; `src/storage/storage_engine.cpp:39-50`; `src/security/timestamp_authority.cpp:52`; `include/security/hsm_security_checker.h:38` | (1) Shard-RPC-Server fällt bei **aktiviertem** mTLS und fehlerhaftem Zertifikats-/Key-Laden auf `grpc::InsecureServerCredentials()` zurück – auch im Produktionsmodus, nur mit ERROR-Log (CWE-295). (2) `THEMIS_PRODUCTION_MODE` wird an ≥ 7 Stellen mit unterschiedlicher Semantik geparst: Shard-RPC akzeptiert nur exakt `1`, `ProductionMode` akzeptiert `true/yes/on` (nicht `production`), `TransportSecurityChecker` akzeptiert `production` (nicht `yes/on`) → bei `THEMIS_PRODUCTION_MODE=true` oder `THEMIS_ENVIRONMENT=production` greift der Shard-mTLS-Zwang **nicht**, andere Guards schon. | Eine kanonische Funktion (`themis::core::ProductionMode::isEnabled()`) mit dokumentierter, case-insensitiver Werteliste; alle Direktparser ersetzen; Zertifikatsfehler bei aktiviertem mTLS = Startfehler (fail-closed); Unit-Tests je Werteform; Startup-Log mit aufgelöstem Modus. | S | ✅ |
| <a id="sto-03"></a>STO-03 | Cache / Konsistenz | `src/cache/redis_cache_coordinator.cpp:105-111` | Ohne hiredis-Build ist der Koordinator ein No-op mit WARN – keine Cross-Node-Invalidierung, Caller sieht Erfolg. | Konfiguration mit Redis-Koordinator ohne hiredis = Startfehler/Readiness false; Statusmetrik. | S | ✅ |
| <a id="sto-04"></a>STO-04 | Lifecycle | `src/maintenance/database_maintenance_orchestrator.cpp:608-613` | `std::thread(...).detach()` mit Capture von `this` → Use-after-free bei Shutdown während laufender Wartung. | `std::jthread`/Thread-Pool mit `stop_token`, Join im Destruktor; TSan-Test für Shutdown während Wartung. | S | ✅ |
| <a id="qry-01"></a>QRY-01 | Query / Ressourcen | `src/query/vectorized_execution.cpp:274-277` | `LIMIT` wird erst nach vollständiger Materialisierung aller Operatoren angewendet → Speicher/Latenz proportional zur Gesamtmenge. | LIMIT-/Top-K-Pushdown in Scan/Sort, Early-Termination, Speicherbudget pro Query; Benchmark „LIMIT 10 auf 10⁷ Zeilen“. | M | ✅ |
| <a id="qry-03"></a>QRY-03 | Query / Semantik | `src/tensor/utr_converter.cpp:294-306` | Stiller Fallback auf lexikalische Pseudo-Embeddings, wenn kein Embedding-Provider verfügbar ist. | Fallback nur explizit; Ergebnis-Metadaten `degraded=true`; im Produktionsprofil Fehler statt Fallback. | S | 🔎 |
| <a id="sec-13"></a>SEC-13 | AuthZ / Test-Gap | `src/server/` (90.590 LOC, 24 Testdateien), `src/api/` | Kein automatisierter Nachweis, dass **jede** HTTP/gRPC/WebSocket/GraphQL-Route unauthentifizierte, mandantenfremde und Scope-unzureichende Aufrufe ablehnt. | Routen-Registry-getriebener AuthZ-Matrix-Test (alle Routen × {anon, falscher Tenant, fehlender Scope}); neue Route ohne Policy = Build-/Testfehler. | L | 🔎 |
| <a id="ops-01"></a>OPS-01 | CI-Gates (OPS-01/02/03) | `.github/workflows/wiki-pr-gate.yml:83`, `compliance-governance-gates.yml:157`, `gate-release-benchmark-validation.yml:111` | Gate-Workflows tolerieren Fehler (`continue-on-error: true` bzw. `\|\| true`); repo-weit 26× `continue-on-error: true` in 15 Workflows, 149× `\|\| true`. | Gates fail-closed; Toleranz nur mit Begründungskommentar + Ablaufdatum; Lint-Regel für neue Vorkommen ([CI-03](#ci-03)). | M | ✅ |
| <a id="ops-06"></a>OPS-06 | CI / Sanitizer | `.github/workflows/build-clang-fast.yml:5`, `build-sanitizer-nightly.yml` | ASan/UBSan nur nightly; PR-Lane explizit „without sanitizer overhead“. Speicherfehler werden erst nach Merge entdeckt. | Schlanke ASan+UBSan-PR-Lane für Core-Module (`storage`, `transaction`, `network`, `query`, `auth`), TSan nightly verpflichtend. | M | ✅ |
| <a id="ops-07"></a>OPS-07 | CI / Fuzzing | `.github/workflows/security-fuzzing.yml:5-9` | Fuzzing nur `schedule`/`workflow_dispatch`; Parser-Änderungen werden auf PRs nicht gefuzzt. | 60–120 s Fuzz-Smoke je betroffenem Parser-Target auf PRs (Pfadfilter), Korpus-Cache; Crash = Fail. | M | ✅ |
| <a id="ops-09"></a>OPS-09 | Kubernetes / HA | `helm/themisdb/values.yaml:4`, `:241`; `helm/themisdb/templates/` | `replicaCount: 1`, **kein** PodDisruptionBudget-Template, NetworkPolicy `enabled: false`. | PDB-Template, HA-Values-Profil (≥ 3 Replikas, Anti-Affinity, TopologySpread), NetworkPolicy im Produktionsprofil aktiv. | M | ✅ |
| <a id="ops-12"></a>OPS-12 | Versionierung | `helm/themisdb/Chart.yaml:5-6` | `version: 0.2.0`, `appVersion: "1.0.0"` vs. `VERSION` 2.4.0. | `appVersion` aus `VERSION` generieren; Versions-Konsistenz-Gate (VERSION, CMake, vcpkg, Helm, Debian, SDKs). | S | ✅ |
| <a id="doc-02"></a>DOC-02 | Status-Drift (DOC-02/03/04/05/20, SEC-11) | `CHANGELOG.md:66-72`, `:382-386`; `SECURITY.md:26-27`; `RELEASE_STRATEGY.md:79`; vs. `VERSION`/`RELEASE_TYPE` alpha und `ROADMAP.md` | `CHANGELOG` deklariert „[2.4.0] – 2026-08-13 – GA Release … production-ready“ und „GA Release Quality Gates“ mit offenem D-11; `SECURITY.md` nennt „2.4.0 (current GA lane) … Technical GA gates PASS“; Root-Roadmap/VERSION: Alpha, Gates offen. | Einheitliche Statusquelle (Root-ROADMAP „Canonical Status Authority“); CHANGELOG-Eintrag in „2.4.0-alpha“ umdeklarieren bzw. GA-Claims als „geplant/nicht erreicht“ markieren; SECURITY-Supporttabelle an `RELEASE_TYPE` koppeln; CI-Check auf „GA“/„production-ready“ bei `RELEASE_TYPE != ga`. | S | ✅ |
| <a id="evd-01"></a>EVD-01 | Test-Evidenz | `ctest_full.log` (2026-10-24, untracked); `tests/cmake/RunRegisteredTest.cmake:148` | Letzter lokaler Volllauf: „0% tests passed, 1777 tests failed out of 1781“ – Ursache Infrastruktur (`ninja: error: opening build log: Permission denied` beim On-demand-Build). Übrige Logs vom 2026-10-05/08. Es existiert **keine gültige aktuelle Volllauf-Evidenz**; Validierungsartefakt laut Root-Roadmap seit 2026-10-30 abgelaufen. | On-demand-Build in CTest im Evidenzlauf deaktivieren (Build-Schritt getrennt, Fehler = Abbruch); signiertes, datiertes CTest-JUnit-Artefakt pro `develop`-Commit in CI; Evidenz-Ablaufüberwachung. | M | ✅ |
| <a id="ai-03"></a>AI-03 | AI / Test-Evidenz (AI-03..06) | `tests/llm/test_real_embeddings.cpp:100`, `test_lora_adapters.cpp:92`, `tests/llm/test_wiki_index_store_phase_b.cpp:101`, `tests/llm/test_llm_tinyllama_inference.cpp:105` | Real-Model-/Integrationstests werden ohne Modell-Fixtures übersprungen; 258 `GTEST_SKIP` in `tests/llm`. Kein Nachweis echter Inferenz in CI. | Gepinntes Tiny-Modell (Hash) im CI-Cache ([../.github/workflows/reusable-llm-model-cache.yml](../.github/workflows/reusable-llm-model-cache.yml)); Skips in dieser Lane = Fail; Skip-Budget je Modul. | M | 🔎 |
| <a id="run-01"></a>RUN-01 | Betrieb / DR (OPS-15, DOC-11/12) | `docs/production/RUNBOOKS/*.md`, `docs/production/DISASTER_RECOVERY_PLAN.md` | Restore-, Upgrade-, Failover-, Scaling- und Core-Runbooks existieren, sind aber „Version 1.0, April 2026“; DRP fordert quartalsweisen Review (überfällig); kein Übungs-/Restore-Nachweis, RPO/RTO unbelegt; nicht aus Onboarding verlinkt. | Quartals-Review nachholen; Restore-/Failover-Drill mit Protokoll (Zeitstempel, RPO/RTO gemessen) als Release-Evidenz; Verlinkung aus `QUICKSTART.md`/Ops-Index. | M | ✅ |

### 5.3 🟡 Medium

| ID | Bereich | Fundstelle | Befund | Empfehlung | Aufw. | Verif. |
|---|---|---|---|---|---|---|
| <a id="dis-04"></a>DIS-04 | Distributed / Build-Gate | `src/sharding/shard_rpc_client.cpp:1042-1055` | Ohne `THEMIS_HAS_SHARD_GRPC=1` wirft der Client für Nicht-`inproc://`/`loopback://`-Endpunkte (korrekt fail-closed); feste Antworten nur für Test-Endpunkte. Es fehlt ein Release-Gate, das Multi-Node-Pakete mit echtem Transport erzwingt. *(von High herabgestuft)* | Release-Build-Gate `THEMIS_HAS_SHARD_GRPC=1` für Multi-Node-Artefakte; Produktionsprofil lehnt `inproc://`-Endpunkte beim Start ab. | S | ✅ |
| <a id="dis-10"></a>DIS-10 | Distributed / Test-Gap | `tests/sharding/test_sharding_contract_hardening_focused.cpp:99-149` | 2PC-Tests nutzen Stub-Teilnehmerzustand; kein echter RPC, kein WAL, kein Prozess-Crash. | Multi-Prozess-Tests mit echtem Transport, Koordinator-Kill, WAL-Replay und Partition-Heilung. | L | 🔎 |
| <a id="dis-11"></a>DIS-11 | Distributed / Test-Gap | `tests/sharding/test_sharding_phase6_hardening.cpp:139-191` | Partition per Bool-Flag („every request fails“); keine Verzögerung, kein Reordering, keine Duplikate, keine asymmetrischen Partitionen. | Netzwerk-Fault-Injection (z. B. `tc netem`/Toxiproxy) in einer Multi-Prozess-Chaos-Suite. | L | 🔎 |
| <a id="dis-13"></a>DIS-13 | Distributed Tensor / Persistenz | `src/distributed_tensor/src/manifest_store.cpp:6` | Manifest-Store ist eine In-Memory-Map; Placement-/Recovery-Metadaten gehen beim Neustart verloren. | Transaktional über DB/WAL persistieren; Restart-Test. | M | 🔎 |
| <a id="dis-14"></a>DIS-14 | Verfügbarkeit | `src/sharding/stream_protocol.cpp:135` | Retry-Backoff per `sleep_for` in Worker-Threads → Thread-Erschöpfung bei längeren Ausfällen. | Asynchrone Timer, begrenzte Retries, Deadline-Propagation. | M | 🔎 |
| <a id="dis-15"></a>DIS-15 | Failover / Vorbedingungen | `src/failover/PRODUCTION_REQUIREMENTS.md:39-57` | Quorum/Fencing sind „deployment recommendations, not validations performed by the module“. | Als erzwungene Vorbedingungen implementieren; Negativtests für fehlendes Quorum/Fencing. | M | 🔎 |
| <a id="sto-02"></a>STO-02 | CDC / Konfiguration | `src/cdc/kafka_cdc_producer.cpp:15-29` | Ohne librdkafka: No-op-Producer, `start()`/`publish()` liefern `false` (kein stiller Erfolg) – aber konfigurierte Kafka-Auslieferung führt nicht zu Startfehler/Readiness-Fehler. *(von High herabgestuft)* | „Kafka konfiguriert, aber nicht verfügbar“ = Startfehler; CDC-Lieferstatus im Health-Endpoint. | S | ✅ |
| <a id="sto-05"></a>STO-05 | CDC / Fehlerbehandlung | `src/cdc/changefeed.cpp:220-223` | Leere `catch (const std::exception&) {} catch (...) {}` beim Parsen der Legacy-Sequenznummer → Korruption wird zum Fallback-Wert. | Strukturiert loggen (Key, Rohwert), Korruption von „nicht vorhanden“ unterscheiden, Fehlermetrik. | S | ✅ |
| <a id="sto-06"></a>STO-06 | CDC / Test-Gap | `tests/cdc/test_cdc_outbox.cpp:26` | Outbox-Tests unter Windows per `GTEST_SKIP` wegen Fixture-Crash deaktiviert. | Fixture-Crash beheben; Outbox-/Retention-/Delivery-Tests plattformübergreifend verpflichtend. | M | 🔎 |
| <a id="sto-07"></a>STO-07 | Storage / Test-Gap *(rekalibriert)* | `tests/test_wal_storage.cpp:290-316`; `tests/test_crash_recovery_direct.cpp`; `tests/integration/storage/backup_recovery_integration_test.cpp` | Crash-/WAL-/Restore-Tests **existieren** (Root- und Integrationsbaum, nicht unter `tests/storage/`), laufen aber In-Process. `CorruptedEntry_StopsReplay` **erwartet ausdrücklich** das stille Abbrechen („Recovery should silently stop at the corrupted entry – not crash“) und zementiert damit [STO-01](#sto-01). Kein Prozess-Kill-Test (Kill während Write/Fsync), kein Mid-Log-Korruptionstest mit nachfolgenden gültigen Records, kein Timeout-Test. | Test auf den neuen Fail-closed-Vertrag umstellen; Prozess-Kill-Harness mit Ack-Zählung; Mid-Log-Korruption, Torn Write, Fsync-Fehler; Backup→Restore→Verify-Roundtrip. | L | ✅ |
| <a id="sto-08"></a>STO-08 | Timeseries / Test-Gap | `src/timeseries/` (8.437 LOC) vs. `tests/timeseries/` (7 Dateien) | Retention, Compaction, große Aggregationsfenster und nebenläufige Ingestion schwach getestet. | Concurrency-, Restart-, Retention-Grenz- und Compaction-Fehlertests. | L | 🔎 |
| <a id="sto-10"></a>STO-10 | Storage / Fehlerbehandlung | `src/storage/streaming_ingest_manager.cpp:320-326` | Batch-Write-Status nur an der Batch-Grenze; Teilbatch-Semantik und Fehler-Propagation nicht erkennbar definiert. | Batch-Fehler explizit propagieren, fehlgeschlagene Offsets melden, Retry/Idempotenz definieren. | M | 🔎 |
| <a id="sec-03"></a>SEC-03 | AuthN / Observability | `src/server/auth_middleware.cpp:505-518` | `catch (...)` → `Denied("Invalid token")` (fail-closed ✔), aber Keystore-/Validator-Ausfälle sind von ungültigen Tokens nicht unterscheidbar; nur DEBUG-Log. | Bekannte Exceptions separat behandeln; Security-Event + Metrik `auth_internal_error`; weiterhin fail-closed. | S | 🔎 |
| <a id="sec-04"></a>SEC-04 | Information Disclosure | `src/utils/audit_logger.cpp:231-238` | Vollständiger Audit-Log-Pfad in Fehlermeldungen. | Pfade in extern sichtbaren Meldungen redigieren. | S | 🔎 |
| <a id="sec-05"></a>SEC-05 | TLS / Konfiguration | `config/timestamp_authority.yaml:64` | `verify_tsa_cert: false` in mitgelieferter Konfiguration → MITM auf TSA möglich. | Default `true`; `false` nur im Dev-Profil, im Produktionsprofil Startfehler. | S | ✅ |
| <a id="sec-06"></a>SEC-06 | Exposure / Konfiguration | `config/raid_entity_config.example.yaml:172`, `:185` | Beispiel bindet `0.0.0.0` mit `enable_tls: false`. | Beispiele auf `127.0.0.1` + TLS; Produktionsprofil verweigert extern erreichbares Plaintext. | S | ✅ |
| <a id="sec-07"></a>SEC-07 | Exposure / Konfiguration | `config/replication.example.yaml:6` | Replikations-Beispiel bindet `0.0.0.0`. | `127.0.0.1` als Beispiel-Default; externe Bindung nur mit mTLS. | S | ✅ |
| <a id="sec-09"></a>SEC-09 | Krypto / AuthN | `src/auth/saml_authenticator.cpp:122` | SAML-Request-IDs (`generateRequestId`) aus `std::mt19937_64` – nicht kryptographisch, vorhersagbar. | OpenSSL `RAND_bytes`; Lint-Regel gegen `mt19937`/`rand()` in `auth/` und `security/`. | S | ✅ |
| <a id="sec-12"></a>SEC-12 | AuthN / Test-Gap | `tests/security/test_jwt_security.cpp.skip` | JWT-Sicherheitstests per Dateiendung deaktiviert. *(von Low heraufgestuft: kritischer AuthN-Pfad)* | Reaktivieren; `alg=none`, Algorithm-Confusion, Expiry, Issuer, Key-Rotation als Pflichtfälle. | M | ✅ |
| <a id="qry-02"></a>QRY-02 | AQL / Eingabevalidierung | `src/aql/aql_query_validator.cpp:261-262` | `std::stoi` auf Regex-Treffer `\d+` → `std::out_of_range` bei überlangen Zahlen; Upstream-Catch nicht verifiziert. *(von High herabgestuft)* | `std::from_chars` + harte Max-Tiefe; Fuzz-Seed. | S | ✅ |
| <a id="qry-04"></a>QRY-04 | GPU / Ergebnisstatus | `src/gpu/gpu_module.cpp:137-145` | CPU-Fallback liefert `true` (mit Fallback-Metrik); Aufrufer erkennt degradierte Ausführung nicht. *(von High herabgestuft)* | Status `{Ok, OkDegraded, Failed}`; in API-Antwort/Trace sichtbar. | M | ✅ |
| <a id="qry-05"></a>QRY-05 | AQL / Test-Gap | `tests/aql/test_aql_with_clause.cpp:272` | WITH-Klausel-Tests konfigurationsabhängig übersprungen. | Capability-basierte Pflicht-CI. | M | 🔎 |
| <a id="qry-06"></a>QRY-06 | AQL / Test-Gap | `tests/aql/test_aql_shortest_path.cpp:30` | Legacy-SHORTEST_PATH-Tests übersprungen. | Portieren; Tiefe, Zyklen, Timeout, Cancellation. | M | 🔎 |
| <a id="qry-07"></a>QRY-07 | Search / Timeouts | `src/search/ROADMAP.md:266-284` | Erzwungene Timeout-/Cancellation-Semantik nur `[~]`. | Abschließen + Integrationstests; bis dahin kein Produktions-Claim. | L | 🔎 |
| <a id="qry-08"></a>QRY-08 | Optimizer | `src/query/query_optimizer.cpp:490-552` | Kardinalitäts-Defaults (z. B. `100000`) bei fehlenden Statistiken. | Herkunft der Schätzung tracken, Statistiken sammeln, Plan-Regressionstests mit Daten-Skew. | M | 🔎 |
| <a id="ai-07"></a>AI-07 | LLM / Ressourcen | `src/llm/task_decomposer.cpp:982` | Fallback `max_tokens = 512`, aber kein zentrales hartes Cap an der Request-Grenze. | Globale Token-/Kontext-Obergrenze, unabhängig von Task-Konfiguration. | M | 🔎 |
| <a id="ai-08"></a>AI-08 | LLM / Doku-Drift | `src/llm/THREADING.md:493` | „Tested up to 1024 concurrent requests on single GPU“ ohne reproduzierbares Artefakt. | Benchmark-Artefakt + Hardware veröffentlichen oder Aussage entfernen. | S | 🔎 |
| <a id="ai-09"></a>AI-09 | LLM / Admission Control | `src/llm/vision_config.cpp:149` | Concurrency-Limit nur für Vision belegt; keine einheitliche Admission-Control/GPU-Speicherbuchhaltung. | Gemeinsamer Admission-Controller für alle Inferenz-Backends. | L | 🔎 |
| <a id="ai-10"></a>AI-10 | LLM / Test-Gap (AI-10, AI-11) | `tests/llm/test_llm_vision_encoder.cpp:129`; `tests/llm/test_lora_adapters.cpp:319` | Vision-Konstruktor-Ambiguität und LoRA-Concurrency per Skip ausgeklammert. | Ambiguität beheben; LoRA-Lifecycle/Concurrency unter TSan testen. | M | 🔎 |
| <a id="ops-08"></a>OPS-08 | K8s / Netzwerk | `helm/themisdb/values.yaml:241` | NetworkPolicy-Template vorhanden, aber `enabled: false`. | Default aktivieren, dokumentierte Allowlist. | S | ✅ |
| <a id="ops-11"></a>OPS-11 | Container / Supply Chain | `Dockerfile:18`, `:181` | `FROM ubuntu:24.04` ohne Digest-Pin. | Digest-Pinning + automatisierte Updates; Image-Signatur vor Publish. | S | ✅ |
| <a id="ops-18"></a>OPS-18 | Packaging | `debian/` | `debian/changelog` fehlt → Debian-Paketversion nicht aus Quelle ableitbar. | Aus `VERSION` generieren; Teil des Versions-Gates ([OPS-12](#ops-12)). | S | ✅ |
| <a id="hyg-01"></a>HYG-01 | Repo-Hygiene (OPS-13, OPS-14) | `git ls-files` | 65.540 getrackte Dateien, 63 % unter `ai_context/` (inkl. generiertem Doxygen-XML); 24 Dateien > 5 MB (z. B. `docs/ARCHIVED/ai-working-history/scan_tests.json` 80 MB, `archive/moved_files.txt` 29 MB); Bench-Artefakte im Root (`bench_access_coordinator_gates.exe.json`). | Generierte Artefakte aus VCS (Release-Assets/Artefakt-Store); `.gitignore`; Hygiene-Gate mit Max-Dateigröße. | M | ✅ |
| <a id="tst-01"></a>TST-01 | Testqualität (DIS-12) | `tests/` | 2.965 `GTEST_SKIP`, 1.020 triviale Asserts (z. B. `tests/sharding/test_sharding_phase2_hardening.cpp:423`), 1.018 tote Legacy-Testdateien, 5 deaktivierte Dateien. | Skip-Budget je Modul mit Pflichtbegründung; triviale Asserts ersetzen; Legacy-Tests portieren oder löschen. | L | ✅ |
| <a id="tst-02"></a>TST-02 | Testdichte | [Abschnitt 2.3](#23-test-dichte-ausreißer-src-loc-vs-testumfang) | `server` 90.590 LOC / 24 Testdateien; `distributed_tensor`, `prompt_engineering`, `timeseries`, `scheduler`, `llama_cpp`, `whisper` dünn getestet. | Coverage-Ziel für Stufe-1-Module (≥ 70 % Lines, ≥ 60 % Branches) mit Messung in CI. | L | ✅ |
| <a id="tst-03"></a>TST-03 | Test-Registrierung *(neu)* | `tests/CMakeLists.txt:315`; `tests/CMakeLists.txt:21001-21007` | `tests/test_auto_failover_recovery.cpp` ist aus `themis_tests` ausgeschlossen und **nirgends** sonst registriert (Failover-Recovery ungetestet). Focused-Target `test_wal_storage_focused` verweist auf nicht existierendes `tests/wal/test_wal_storage.cpp` → stiller `if(EXISTS)`-No-op. | Registrierungs-Lint: jede `test_*.cpp` landet in genau einem Target oder steht auf einer begründeten Ausschlussliste; `if(EXISTS)`-Guards auf tote Pfade prüfen. | S | ✅ |
| <a id="ci-03"></a>CI-03 | CI-Toleranz | `.github/workflows/` | 26× `continue-on-error: true` in 15 Workflows, 149× `\|\| true` – ohne Klassifikation (Diagnose vs. Gate). | Inventar mit Begründung, Owner und Ablaufdatum; Guard-Regel gegen neue unbegründete Vorkommen. | M | ✅ |
| <a id="doc-08"></a>DOC-08 | Status-Metrik (DOC-08, DOC-09) | `ROADMAP.md:47-53` | Zwei Fortschrittswerte (~72 % source-verified vs. ~82–84 % optimistisch), Stand 2026-09-09. | Eine datierte, reproduzierbare Metrik; Altwerte in Historie. | S | ✅ |
| <a id="doc-10"></a>DOC-10 | Modul-Docs | `src/llm_l_wiki/` | Nur `ARCHITECTURE.md` + eine leere Quelldatei; README/ROADMAP/FUTURE_ENHANCEMENTS fehlen. | Modul entfernen oder Pflicht-Docs ergänzen. | S | ✅ |
| <a id="doc-19"></a>DOC-19 | Doku-Sprawl | `docs/` (5.438 `.md`), `docs/ARCHIVED/` | Archivmaterial dominiert; aktuelle Betriebsdoku schwer auffindbar. | Archiv aus der Default-Navigation, Archiv-Banner, Index auf kanonische Seiten. | M | ✅ |

### 5.4 ⚪ Low / Info

| ID | Fundstelle | Befund | Empfehlung | Verif. |
|---|---|---|---|---|
| <a id="ai-01"></a>AI-01 (AI-01, AI-02) | `src/llm/ssm_stub_plugin.cpp:16-189` | SSM-Stub vollständig in `#if 0` → toter, unerreichbarer Code. *(von High herabgestuft)* | Datei entfernen. | ✅ |
| <a id="ai-12"></a>AI-12 | `src/llm/TODO_CRITICAL_GAPS.md:107` | Timeout-Mechanismen als Lücke dokumentiert, andere Docs behaupten Produktionsreife. | Docs abgleichen, Timeout-Integrationstest. | 🔎 |
| <a id="sec-02"></a>SEC-02 | `src/plugins/plugin_manager.cpp:1545-1560` | Plugin-Load fail-closed bei ungültiger Signatur ✔; Pinning der Signer-Identitäten nicht nachgewiesen. | Gepinnte Signer; Negativtests (unbekannter Signer, Verifikation deaktiviert). | 🔎 |
| <a id="sec-08"></a>SEC-08 | `src/auth/ldap_connection_pool.cpp:213` | `std::rand()` nur für Retry-Jitter – nicht sicherheitsrelevant. | Jitter-Helper mit `<random>`, Kommentar „non-security“. | ✅ |
| <a id="sec-10"></a>SEC-10 | `src/whisper/whisper_plugin.cpp:18-29` | Stub-Transcriber-Modus ohne Produktions-Guard. | Nur mit Dev-Flag; Produktionsprofil verweigert Start ohne Modell (→ WS5). | 🔎 |
| <a id="sto-09"></a>STO-09 | `src/maintenance/API.md:19-23` | AuthZ nicht im Handler, nur in der Routenregistrierung. | Durch AuthZ-Matrix-Test [SEC-13](#sec-13) abdecken. | 🔎 |
| <a id="ops-04"></a>OPS-04 (OPS-04, OPS-05) | `.github/workflows/release-mainline.yml:130` u. a. | `actions/checkout@v5` ist laut `.github/ACTION_PIN_POLICY.md:40-52` Tier 2 und policy-konform. *(von High herabgestuft)* | Optional SHA-Pins im Release-Pfad. | ✅ |
| <a id="ops-16"></a>OPS-16 | `helm/themisdb/values.yaml:210-213` | Metrik-Endpoint aktiv (`metrics.enabled: true`), `serviceMonitor.enabled: false`; Alert-Regeln nicht im Chart. *(rekalibriert: Agent zitierte Z. 106 = Autoscaling)* | Produktions-Values-Profil mit ServiceMonitor + PrometheusRule. | ✅ |
| <a id="ops-17"></a>OPS-17 | `VERSION:1`, `vcpkg.json:3` | Kernversionen konsistent; Gate über alle Paket-Metadaten fehlt. | In Versions-Gate ([OPS-12](#ops-12)) aufnehmen. | 🔎 |
| <a id="doc-07"></a>DOC-07 | `audit/README.md:4` | Audit-Hub-Stand 2026-10-01. | Mit diesem Audit aktualisiert. | ✅ |
| <a id="doc-16"></a>DOC-16 | `BRANCHING_STRATEGY.md:19-20` | Legacy-Branchnamen nur im Migrationshinweis – zulässig; als historisch kennzeichnen. | Historien-Abschnitt. | 🔎 |
| <a id="doc-17"></a>DOC-17 | `VERSIONING.md:4` | Uneinheitliche Edition-Schreibweise. | Kanonische Schreibweise festlegen. | 🔎 |
| <a id="doc-18"></a>DOC-18 | `QUICKSTART.md:145-155` | Deutsche Fragmente im englischen Onboarding. | Sprachlich vereinheitlichen. | 🔎 |
| <a id="doc-21"></a>DOC-21 | `CHANGELOG.md:398`, `:453`, `:910`, `:1006`, `:1034`, `:1081` | Versionsreihenfolge nicht chronologisch (1.9.0 → 1.8.1-rc1 → 1.5.0 → 1.4.2 → 1.4.0 → 1.8.0). | Sortieren; Changelog-Lint. | ✅ |
| <a id="ci-04"></a>CI-04 (Info) | `crda.yml`, `gate-workflow-guidelines.yml`, `maintenance-housekeeping.yml`, `maintenance-workflow-guardrails-observe.yml` | `pull_request_target` in 4 Workflows; `crda.yml` checkt Base-Ref mit `contents: read` aus – kein Pwn-Request-Muster gefunden (begrenzte Prüfung). | Guard-Regel: kein Checkout von PR-Head unter `pull_request_target`. | ✅ |

---

## 6. Doku-vs-Code-Drift (Matrix)

| # | Dokumentierte Aussage | Fundstelle Doku | Realität (Code/Evidenz) | Finding |
|---:|---|---|---|---|
| 1 | „[2.4.0] – GA Release“, „production-ready“ | `CHANGELOG.md:66-72` | `VERSION` 2.4.0 + `RELEASE_TYPE` alpha; Root-Roadmap: GA durch Wave-A/B-Evidenz und Human-Sign-off blockiert | [DOC-02](#doc-02) |
| 2 | „2.4.0 (current GA lane) … Technical GA gates PASS“ | `SECURITY.md:26-27` | Kein gültiger aktueller Volllauf; Sign-off offen | [DOC-02](#doc-02), [EVD-01](#evd-01) |
| 3 | „GA Release Quality Gates“ | `CHANGELOG.md:382-386` | Im selben Abschnitt Gate D-11 **OPEN** | [DOC-02](#doc-02) |
| 4 | „Fail-closed … chaos-tested for crash recovery“; „Startup fails fast … no silent corruption“ | `src/storage/README.md:67`, `:76`, `:122` | `WALStorage`-Replay bricht still ab; Test erwartet stilles Abbrechen | [STO-01](#sto-01), [STO-07](#sto-07) |
| 5 | Issue #5373 als erledigt markiert | `ROADMAP.md:636-684` | Doppel-Koordinator und fragmentierte Recovery im selben Abschnitt als offenes Risiko | [DIS-05](#dis-05), [DIS-06](#dis-06) |
| 6 | „Tested up to 1024 concurrent requests on single GPU“ | `src/llm/THREADING.md:493` | Kein Artefakt; Real-Model-Tests überwiegend `GTEST_SKIP` | [AI-08](#ai-08), [AI-03](#ai-03) |
| 7 | Timeout-Enforcement Search | `src/search/ROADMAP.md:266-284` | Nur `[~]`; anderswo Produktions-Claims | [QRY-07](#qry-07) |
| 8 | Fortschritt ~72 % **und** ~82–84 % | `ROADMAP.md:47-53` | Zwei Metriken, Stand 2026-09-09 | [DOC-08](#doc-08) |
| 9 | Chart `appVersion: "1.0.0"` | `helm/themisdb/Chart.yaml:5-6` | Produkt 2.4.0 | [OPS-12](#ops-12) |
| 10 | Runbooks „Version 1.0, April 2026“, DRP mit Quartals-Review | `docs/production/RUNBOOKS/*.md`, `docs/production/DISASTER_RECOVERY_PLAN.md` | Review überfällig; kein Übungsnachweis; nicht aus `QUICKSTART.md` verlinkt | [RUN-01](#run-01) |
| 11 | Compliance-Snapshot „ISO 27001 ✅ 95 %“, „SOC 2 Type II ✅ 90 %“ | `audit/README.md` (Compliance Snapshot) | Checklisten-Selbsteinschätzung, keine Zertifizierung; offene High-Findings in Supply Chain/Evidenz | [SEC-01](#sec-01), [EVD-01](#evd-01) |

**Positiv (ehrliche Selbstauskunft):** `src/failover/PRODUCTION_REQUIREMENTS.md`, `src/rpc_grpc/TRUST.md` und die
`STUB/SIMULATION NOTE`-Kommentare dokumentieren Lücken offen. Das Problem ist dort nicht Drift, sondern fehlende
Durchsetzung im Code (Produktionsprofil-Guards).

---

## 7. Kalibrierung – verworfene und herabgestufte Befunde

### 7.1 Verworfen (False Positives)

| Ursprüngliche Agent-ID | Behauptung | Grund der Verwerfung |
|---|---|---|
| DOC-06 | `DOCUMENTATION_GOVERNANCE.md` > 60 Tage alt | Tatsächlich 41 Tage seit letzter Aktualisierung |
| DOC-13 | `ROADMAP.md` verletzt Metadata-Gate | `.github/doc-metadata-gate.json` schließt `ROADMAP.md`, `**/ROADMAP.md` aus |
| DOC-14 | `CHANGELOG.md` verletzt Metadata-Gate | Ebenfalls per Gate-Konfiguration ausgeschlossen |
| DOC-15 | `QUICKSTART.md` verletzt Metadata-Gate | Root-Dateien stehen nicht in der Include-Liste des Gates |
| Docs-Agent (Tabelle) | „Metadata compliance: Failing“ pauschal | Nicht belegt; folgt aus DOC-13..15 |
| OPS-15, DOC-11, DOC-12 (als „fehlt“) | Keine Backup-/Restore-/Upgrade-Runbooks | Runbooks existieren (`docs/production/RUNBOOKS/RESTORE_RUNBOOK.md`, `UPGRADE_RUNBOOK.md` u. a.) → Befund umformuliert zu „veraltet/ungeübt/unverlinkt“ ([RUN-01](#run-01)) |

### 7.2 Umgestuft bzw. rekalibriert

| ID | Agent-Einstufung | Final | Begründung |
|---|---|---|---|
| AI-01/AI-02 | High | Low | `ssm_stub_plugin.cpp` vollständig in `#if 0` – unerreichbar |
| DIS-04 | High | Medium | Fail-closed für reale Endpunkte; nur Release-Gate fehlt |
| STO-02 | High | Medium | `start()`/`publish()` liefern `false`, kein stiller Erfolg |
| QRY-02 | High | Medium | Eingabe-Exception, Upstream-Catch unklar; kein Datenrisiko |
| QRY-04 | High | Medium | Fallback mit Metrik, Ergebnis korrekt; nur Status-Semantik |
| OPS-04/OPS-05 | High | Low | `@v5` ist laut `.github/ACTION_PIN_POLICY.md:40-52` Tier 2 und zulässig |
| DOC-05 | Critical | High (in DOC-02) | Doku-Widerspruch, kein Laufzeitrisiko |
| SEC-12 | Low | Medium | Deaktivierte JWT-Tests im kritischen AuthN-Pfad |
| STO-01 | Critical | Critical (Scope präzisiert) | Betrifft `WALStorage` (Cross-Shard-Tx- und Replikations-WAL), nicht den RocksDB-Primärspeicher |
| STO-07 | „nur 6 Testdateien, keine Crash-Tests“ | Medium (rekalibriert) | `tests/storage/` hat 21 Testdateien; Crash-/Restore-Tests liegen in `tests/` und `tests/integration/` |
| OPS-16 | „Monitoring aus“ (`values.yaml:106`) | Low | Z. 106 ist Autoscaling; Metrik-Endpoint ist aktiv, nur ServiceMonitor aus |
| OPS-10 | Medium, separat | in OPS-09 | Gleiche Ursache (HA-Defaults) |
| Modulreife `sharding`, `updates` | 3 | 2 | Erreichbare Simulation ([DIS-03](#dis-03)) bzw. unsignierte Manifeste ([SEC-01](#sec-01)) |

---

## 8. Stärken (belegt)

- **Auth/Security/Governance/Plugins (Reife 4):** Constant-Time-Vergleiche (`CRYPTO_memcmp`), Auth-Middleware
  fail-closed, Plugin-Hash- und Signaturprüfung mit `ERR_PLUGIN_INVALID_SIGNATURE`, Audit-Logger mit Hash-Chain,
  Queue-Guard, Rotation und Fsync.
- **Transaktions-Testinventar:** breite Isolation-, 2PC/3PC-, Partition- und Coordinator-Crash-Modelltests
  (`tests/transaction/test_coordinator_crash_recovery.cpp`, `test_cross_coordinator_wal_recovery.cpp`).
- **Fail-fast in verteilten Pfaden:** Shard-RPC verweigert In-Process-Antworten für reale Endpunkte
  (`shard_rpc_client.cpp:1050-1055`); Storage-Abhängigkeiten im Produktionsmodus fail-closed
  (`src/storage/ROADMAP.md:17`, `:35-36`).
- **Container/K8s-Grundlagen:** Non-root (`Dockerfile:223 USER themis`), Healthchecks, Helm-`securityContext`,
  read-only RootFS, Ressourcenlimits, Probes, HPA-/NetworkPolicy-Templates.
- **CI-Grundhygiene:** Top-Level-`permissions` in allen 99 Workflows, keine `@main`/`@master`-Actions,
  Release-Evidence-Chain (`.github/workflows/release-evidence-chain.yml`), SBOM-/Supply-Chain-Workflows,
  Sanitizer- und Fuzz-Infrastruktur vorhanden (nur nicht PR-gating).
- **Dokumentationsdisziplin:** 72/73 Module mit vollständigen Pflicht-Docs; Stubs/Simulationen überwiegend
  mit `STUB/SIMULATION NOTE` gekennzeichnet; nur 21 `TODO/FIXME`-Marker in ~1,4 Mio. LOC.

---

## 9. Abgleich mit früheren Audits und Root-Roadmap

| Quelle | Aussage | Bewertung durch diesen Audit |
|---|---|---|
| [IMPLEMENTATION_AUDIT_2026-09-30.md](IMPLEMENTATION_AUDIT_2026-09-30.md) | Implementierungs-Sync, Drift-Handling | Weiterhin gültig als Implementierungsstand; ergänzt um Laufzeit-/Fail-open-Befunde |
| [MATURITY_REPORT_2026-08.md](MATURITY_REPORT_2026-08.md) | Monatliche Reife-/Gate-Lage | Durch die Modul-Reifematrix (Kap. 3) überholt |
| [EXECUTIVE_SUMMARY_TEST_DENSITY_2026-10-11.md](EXECUTIVE_SUMMARY_TEST_DENSITY_2026-10-11.md), [PHASE1_COVERAGE_BASELINE_2026-10-11.md](PHASE1_COVERAGE_BASELINE_2026-10-11.md) | Testdichte/Coverage-Baseline | Bestätigt Dichteproblem ([TST-02](#tst-02)); Coverage-Messung muss in CI verankert werden |
| [WAVE_C_AUDIT_EVIDENCE.md](WAVE_C_AUDIT_EVIDENCE.md) | Wave-C-Audit-Evidenz | Root-Roadmap: Validierungsartefakt am 2026-10-30 abgelaufen → erneuern |
| [../ROADMAP.md](../ROADMAP.md) | Alpha; GA-Gate über Wave A/B + Human-Sign-off; ~72 % source-verified | **Bestätigt.** Dieser Audit liefert die technische Begründung, warum GA blockiert bleibt |
| [../docs/governance/GA_PROMOTION_SIGN_OFF.md](../docs/governance/GA_PROMOTION_SIGN_OFF.md) | Verbindlicher Sign-off | Unverändert maßgeblich; dieser Audit ersetzt keinen Sign-off |
| `audit/README.md` Compliance-Snapshot | ISO 27001 95 %, SOC 2 90 % | Als Checklisten-Selbsteinschätzung kennzeichnen (Drift-Matrix Nr. 11) |

**Neu gegenüber bisherigen Audits:** STO-01 (fail-open Recovery inkl. Test-Zementierung), DIS-03 (fehlender
Produktions-Caller für den Stream-Transport), DIS-02 (gRPC-Admin immer Plaintext), SEC-01 (unsignierte Manifeste),
SEC-14 (mTLS-Fallback auf Plaintext, inkonsistente Produktionsmodus-Erkennung), EVD-01 (ungültiger Volllauf),
TST-03 (unregistrierte Failover-/WAL-Tests), HYG-01 (Repo-Größe).

---

## 10. Übergang zur Roadmap

Die audit-abgeleitete Production-Readiness-Roadmap steht in [ROADMAP.md](ROADMAP.md). Zuordnung der Findings zu
Workstreams:

| Workstream | Findings | Meilenstein |
|---|---|---|
| WS1 Durability & Recovery | STO-01, STO-07, STO-04, STO-05, STO-06, STO-10, STO-08 | M1 |
| WS2 Distributed Consistency & HA | DIS-03, DIS-05, DIS-06, DIS-07, DIS-08, DIS-04, DIS-10, DIS-11, DIS-13, DIS-14, DIS-15, STO-03, STO-02 | M2 |
| WS3 Security Defaults & Supply Chain | SEC-01, SEC-14, DIS-01, DIS-02, SEC-13, STO-09, SEC-03..07, SEC-09, SEC-12, SEC-02, SEC-08, OPS-11 | M1 |
| WS4 Query Resource Limits | QRY-01..08 | M1 |
| WS5 AI/LLM Preview Hardening | AI-03, AI-07..10, SEC-10, AI-01, AI-12 | M3 (Preview) |
| WS6 CI Fail-Closed & Test-Evidenz | EVD-01, OPS-01, OPS-06, OPS-07, CI-03, TST-01..03, CI-04 | M0/M1 |
| WS7 Kubernetes & Packaging | OPS-09, OPS-08, OPS-12, OPS-16..18 | M0–M2 |
| WS8 Backup/DR & Runbooks | RUN-01 | M1 |
| WS9 Documentation Truth | DOC-02, DOC-08, DOC-10, DOC-19, DOC-21, DOC-07, DOC-16..18 | M0 |
| WS10 Repo Hygiene | HYG-01 | M1 |
| WS11 Evidence & GA Gates | alle P0/P1 | M3 |

OPS-04 ist policy-konform und keinem Workstream zugeordnet. Die vollständige Zuordnung mit Priorität und Meilenstein je
Finding steht in der [Traceability-Matrix der Roadmap](ROADMAP.md#traceability-matrix).

---

## Anhang A – Reproduzierbarkeit

**Repository-Stand:** `git rev-parse --short HEAD` → `13449dbe6a` (Branch `develop`, 2026-10-31).

**Quantitative Erhebung** (Python, UTF-8, Fehler ignoriert; Dateiendungen `.cpp .cc .cxx .c .h .hpp .hh .hxx .cu .cuh .ipp .tpp`):

| Kennzahl | Regex / Methode |
|---|---|
| LOC | nicht-leere Zeilen |
| `TEST*` | `^\s*TEST(_F\|_P)?\s*\(` (multiline) |
| `GTEST_SKIP` | `GTEST_SKIP` |
| Triviale Asserts | `(EXPECT\|ASSERT)_TRUE\(\s*true\s*\)\|SUCCEED\(\)` |
| `stub` | `\b(stub\|STUB\|Stub)\b` |
| `simulat` | `simulat` (case-insensitive) |
| `catch (...)` | `catch\s*\(\s*\.\.\.\s*\)` |
| `.detach()` | `\.detach\(\)` |
| TLS-insecure | `SSL_VERIFY_NONE\|verify_none\|CURLOPT_SSL_VERIFYPEER,\s*0\|insecure` (case-insensitive) |

Modul-Zuordnung: `src/<m>/`, `include/<m>/`, `tests/<m>/`, `benchmarks/<m>/`. Gesamtzahlen für `tests/` inkl.
Root- und `legacy/`-Dateien.

**Weitere Kommandos (PowerShell, Repo-Root):**

```powershell
git ls-files | Measure-Object                       # getrackte Dateien
git ls-files ai_context | Measure-Object            # Anteil ai_context
git ls-files | ForEach-Object { Get-Item $_ -ErrorAction SilentlyContinue } |
  Where-Object Length -gt 5MB | Sort-Object Length -Descending   # große Dateien
Select-String -Path .github\workflows\*.yml -Pattern 'continue-on-error:\s*true' | Measure-Object
Select-String -Path .github\workflows\*.yml -Pattern '\|\|\s*true' | Measure-Object
Select-String -Path ctest_full.log -Pattern 'tests passed|Permission denied' | Select-Object -First 5
```

**Validierung der Audit-Dokumente:**

```powershell
python scripts\validate_doc_metadata.py --repo-root . --files-from <liste.txt> --report-json <report.json>
python -m pytest tests\test_doc_metadata_gate.py -q
```

**Nicht ausgeführt:** Build, CTest, Sanitizer, Fuzzing, Pentest, Lasttests. Erste Roadmap-Aktion ist daher die
Herstellung einer gültigen Test-Baseline ([EVD-01](#evd-01)).


