# Linux-ThemisDB Integration Matching

Status: Draft (Denkpaper)
Datum: 2026-08-21
Scope: Konsolidierte technische Analyse und Integrationsstrategie Linux Core <-> ThemisDB fuer Production-Readiness

## Abstract

Dieses Dokument konsolidiert alle Erkenntnisse der vorliegenden Analyse zu einer production-first Integrationsstrategie fuer ThemisDB auf Linux. Kernthese: Produktionsreife entsteht nicht durch maximalen Featureumfang, sondern durch einen belastbaren Betriebsvertrag zwischen Linux-Runtime-Eigenschaften und ThemisDB-Capabilities. Besonders kritisch sind gemischte Lasten aus Core-DB, AdaLoRA, CDC/RAG, Repair/Rebalance und Multi-Region-Replikation. Das Dokument liefert ein methodisches Matching, capability-spezifische Wechselwirkungen, harte Go/No-Go Gates, ein 6-Phasen-Roadmapmodell sowie ein sofort startbares 30-Tage-Ausfuehrungsinkrement.

## 1. Problemstellung und Zielbild

Die Frage ist nicht, ob ThemisDB auf Linux betrieben werden kann, sondern wie ThemisDB so integriert wird, dass unter realistischen Mischlasten ein stabiler, sicherer und reproduzierbarer Produktionsbetrieb erreicht wird.

Zielbild:

- ThemisDB als Linux-first Userspace-Plattform (systemd/Container), nicht als Kernelmodul.
- Strikte Trennung von Datenebene (Core DB), KI-Ebene (AdaLoRA/Inference/RAG) und Control-Plane (Security/Observability/Operations).
- Betrieb nach messbaren Servicezielen statt nach isolierten Peak-Benchmarks.

## 2. Methodik des Integration Matching

Das Matching folgt einem vierstufigen Verfahren:

1. Linux-Faehigkeit identifizieren (z. B. cgroups v2, io_uring, eBPF, systemd, chrony).
2. ThemisDB-Capability zuordnen (z. B. AdaLoRA, Sharding, CDC, Multi-Region).
3. Kritische Wechselwirkung und Ausfallmodus bestimmen.
4. Verbindlichen Betriebsvertrag inkl. Abnahmegate definieren.

Damit wird aus Architekturannahmen eine umsetzbare Freigabelogik.

## 3. Architekturannahmen und Grenzen

- ThemisDB laeuft im Userspace, Linux-Kernelfunktionen werden ueber stabile APIs genutzt.
- Kein produktiver Kernelspace-Pfad fuer DB-Logik.
- Featureaktivierung erfolgt nur entlang von Betriebsvertraegen.
- Zielbranch fuer Umsetzungsarbeit ist develop.

## 4. Linux Core <-> ThemisDB Matching Matrix

| Linux Core Faehigkeit | ThemisDB Capability | Wechselwirkung / Nutzen | Risiko ohne Steuerung | Primare Gegenmassnahme |
|---|---|---|---|---|
| cgroups v2 (CPU, RAM, IO) | AdaLoRA, Inference, Query Engine | Harte Ressourcenisolation fuer gemischte Lasten | Tail-Latency-Spitzen, OOM, Query-Starvation | Service-Klassen mit festen Quoten je Workload |
| io_uring | WAL, Storage Engine, CDC IO-Pfade | Niedrigere I/O-Latenz und weniger Syscall-Overhead | Instabile Latenz unter Burst-Last | Feature-Gates, Fallback-Pfad, p99-Grenzwerte |
| eBPF Telemetrie | Observability, Hot-Path Profiling | Fruehe Erkennung von Bottlenecks/Queue-Druck | Blindflug bei Lastspitzen | Standardisierte eBPF-Metriken in SLO-Dashboards |
| systemd (Units/Slices) | Core Daemon, Model Services, Repair Jobs | Deterministischer Lifecycle und Restart-Semantik | Restart-Loops, inkonsistente Service-Zustaende | Unit-Hardening, Health-Gates, Restart-Policy |
| Filesystem + Block-Stack | WAL, Checkpoints, Adapter-Artefakte | Konsistente Recovery, kontrollierte Write-Amplification | Rebuild/Compaction kollidiert mit Foreground-Traffic | I/O-Priorisierung und Maintenance-Fenster |
| mdadm/RAID + Device Health | Sharding, Repair, Rebalance | Hoehere Node-Verfuegbarkeit | p99-Kollaps bei Rebuild + Repair Parallelitaet | Rebuild-aware Throttling im Repair-Scheduler |
| Net stack (TCP, TLS, buffers) | CDC Streams, WebSocket, API, Replication | Stabiler Datenfluss fuer Eventing/Replikation | Backlog-Eskalation, Slow-Consumer Collapse | End-to-End Backpressure und Queue-Limits |
| Time sync (chrony/NTP) | Multi-Region Active-Active | Robustere Konfliktaufloesung | Clock-Drift, inkonsistente Merge-Entscheidungen | Drift-SLO, Alarmierung, Admission-Gates |
| seccomp/capability drop | Security, Plugin Runtime | Reduzierter Attack Surface | Privilegienausweitung, hoehere Exploit-Wirkung | Least-Privilege Profile je Service |

## 5. Capability-spezifische Wechselwirkungen

### 5.1 AdaLoRA

- AdaLoRA-Checkpointing konkurriert mit WAL/Compaction um I/O und Page Cache.
- Training + Inference + Query in einem Lastprofil ist ein p99-Risiko ohne Isolation.
- Adapter-Versionierung benoetigt geordneten Rollback unter Last.

### 5.2 RAID, Sharding, Repair, Rebalance

- RAID adressiert Node-Resilienz, ersetzt aber kein verteiltes Sharding.
- Rebuild und Repair koennen gemeinsam Foreground-Latenzen destabilisieren.
- Der Shard-Router muss Storage-Health-Signale explizit verwerten.

### 5.3 CDC, RAG, Vector

- CDC-Bursts koennen Embedding- und Reindex-Pipelines ueberfahren.
- Ohne End-to-End Backpressure droht Queue-Aufschaukelung bis OOM.
- Reindex muss QoS-gesteuert und gegen Foreground entkoppelt laufen.

### 5.4 Multi-Region Active-Active

- Konfliktaufloesung ist nur mit robustem Zeit- und Versionsmodell stabil.
- Modell- und Adapter-Versionen muessen region-konsistent ausgerollt werden.
- Replikation darf nicht hinter Hintergrundjobs verdrangt werden.

## 6. Production-First Betriebsvertrag

Jede Capability ist erst dann produktionsreif, wenn der Linux-Betriebsvertrag nachweislich eingehalten wird.

| Capability | Linux Betriebsvertrag (MUSS) | Go-Live Nachweis |
|---|---|---|
| Core DB Query/Transaction | Dedizierte CPU/Memory-Mindestkontingente, Foreground-I/O Prioritaet | p95/p99 stabil in Normal- und Degraded-Modus |
| Sharding/Rebalance/Repair | Hintergrundjobs mit adaptiver I/O-Drosselung und Foreground-Priorisierung | Rebuild+Repair+Query ohne SLA-Bruch |
| AdaLoRA Training/Adapter | Separater Slice/Service mit harten CPU/RAM/IOPS/VRAM Quoten | Kein Query-Starvation, reproduzierbarer Rollback |
| CDC/WebSocket/Eventing | End-to-End Backpressure und Queue-Grenzen pro Kanal | Kein unbounded Backlog, kein Datenverlust unter Burst |
| RAG/Vector Indexing | Reindex-Fenster mit QoS und Ressourcen-Caps | Stabile ingest->index->retrieve Latenz |
| Multi-Region Active-Active | Drift-SLO via Chrony/NTP und konfliktfeste Versionsregeln | Deterministische Konfliktaufloesung bei Partition/Rejoin |
| Security Runtime | Least-Privilege (capability/seccomp/namespaces), fail-closed Secrets/Auth | Kein fail-open bei Policy/Secret/TLS Fehlern |

## 7. Harte Abhaengigkeiten (Integrationsreihenfolge)

1. Ohne Linux-Isolation keine belastbare AdaLoRA- oder CDC-Freigabe.
2. Ohne Rebuild-aware Scheduling keine produktionsreife Sharding/RAID-Freigabe.
3. Ohne Backpressure-Vertrag keine stabile RAG/CDC-Lastfaehigkeit.
4. Ohne Drift-Gates keine sichere Multi-Region Active-Active-Freigabe.

## 8. Go/No-Go Gates

- [ ] Gate G1 Ressourcenstabilitaet: 24h Mixed-Load ohne OOM-Kill, ohne Deadlock, ohne unkontrollierte Restart-Loops (Target: Q4 2026)
- [ ] Gate G2 Latenzstabilitaet: Query p99, Inference p95 und CDC-Ende-zu-Ende-Latenz innerhalb SLO-Budgets unter Mischlast (Target: Q1 2027)
- [ ] Gate G3 Degradationstoleranz: RAID-Rebuild + Shard-Repair + Foreground-Traffic ohne Datenverlust und ohne SLA-Break (Target: Q1 2027)
- [ ] Gate G4 Recovery: Crash-Recovery und geordneter AdaLoRA-Rollback innerhalb RTO/RPO-Profil (Target: Q1 2027)
- [ ] Gate G5 Security fail-closed: Policy/Secret/TLS Fehler fuehren zu deterministischem deny statt fail-open (Target: Q1 2027)
- [ ] Gate G6 Multi-Region Konsistenz: Partition/Rejoin/Clock-Skew liefern deterministische Konfliktaufloesung und konsistente Modellversionen (Target: Q1 2027)

## 9. Roadmap (ThemisDB-kompatibles Aufgabenformat)

### Phase 1: Design / API Vertrag

- [ ] Capability-Matrix als verbindliche Integrationsbasis in Architekturdoku referenzieren (Target: Q4 2026)
- [ ] Workload-Klassen finalisieren: core-db, adalora-train, inference, repair, cdc-index (Target: Q4 2026)
- [ ] SLOs definieren: p95/p99 fuer Query, Ingest, Inference, Replication, Repair (Target: Q4 2026)

### Phase 2: Core-Implementierung

- [ ] cgroups-v2 Profile je Workload-Klasse in Deployment-Assets einpflegen (Target: Q4 2026)
- [ ] io_uring Feature-Gate mit robustem Fallback und Telemetrie integrieren (Target: Q4 2026)
- [ ] systemd Unit-Hardening (limits, restart, sandbox) fuer Core + Nebenservices ausrollen (Target: Q4 2026)

### Phase 3: Fehlerbehandlung und Edge Cases

- [ ] Rebuild-aware Throttling aktivieren, wenn RAID degraded/rebuild aktiv ist (Target: Q1 2027)
- [ ] Repair/CDC/Indexing Backpressure mit Hard-Limits und Retry-Policy versehen (Target: Q1 2027)
- [ ] AdaLoRA-Job-Abbruch und Rollback bei Ressourcenverletzung deterministisch machen (Target: Q1 2027)

### Phase 4: Tests

- [ ] Mixed-Load Testprofil: Query + AdaLoRA + CDC + Reindex parallel (Target: Q1 2027)
- [ ] Chaos-Szenarien: node-loss, disk-latency, network-partition, clock-drift (Target: Q1 2027)
- [ ] Fail-closed Security-Tests fuer Capability/Namespace/Seccomp Profile (Target: Q1 2027)

### Phase 5: Performance/Hardening

- [ ] p99-Limits fuer degradierten Storage-Modus (RAID-Rebuild aktiv) verbindlich validieren (Target: Q1 2027)
- [ ] End-to-End Queue-Latency Budgets fuer CDC->RAG->Inference schliessen (Target: Q1 2027)
- [ ] Capacity-Model fuer GPU/CPU/RAM/IOPS je Workload-Klasse finalisieren (Target: Q1 2027)

### Phase 6: Dokumentation und Abnahme

- [ ] Runbooks fuer Rebuild+Repair, AdaLoRA-Degradation, CDC-Backlog und Multi-Region Konfliktbetrieb publizieren (Target: Q1 2027)
- [ ] Operative Release-Gates als Go/No-Go Checkliste in Governance-Doku verankern (Target: Q1 2027)
- [ ] Abschlussreview mit messbarer Soll-Ist-Evidence pro Capability durchfuehren (Target: Q1 2027)

## 10. Capability-Abnahmeprofil (erste produktionsreife Ausbaustufe)

### Core DB

- Lastprofil: OLTP + Read-Mix + WAL-Druck.
- Pflichtnachweis: Keine Inkonsistenz nach abruptem Prozessabbruch (Wiederholungstests).
- Abnahmesignal: p95/p99 stabil in Normalbetrieb und bei Hintergrundaktivitaet.

### RAID + Sharding + Repair

- Lastprofil: Device-Degrade Simulation, paralleler Rebuild, aktive Rebalance.
- Pflichtnachweis: Foreground bleibt priorisiert, Repair wird adaptiv gedrosselt.
- Abnahmesignal: Kein SLA-Kollaps waehrend Rebuild-Fenster.

### AdaLoRA

- Lastprofil: Training + Inference + Query parallel.
- Pflichtnachweis: Harte Quoten verhindern Verdrangung der Core-DB.
- Abnahmesignal: Rollback eines Adapter-Releases unter Last geordnet und reproduzierbar.

### CDC + RAG + Vector

- Lastprofil: Ingest-Burst + Reindex + Retrieval + Stream-Consumer.
- Pflichtnachweis: End-to-End Backpressure greift, Queue-Wachstum bleibt begrenzt.
- Abnahmesignal: Kein Datenverlust und kontrollierte Recovery nach Backlog.

### Multi-Region Active-Active

- Lastprofil: Regionale Netzpartition, Clock-Skew, Rejoin mit Konfliktlast.
- Pflichtnachweis: Konfliktregeln deterministisch und versionstreu.
- Abnahmesignal: Konsistente Daten- und Modellversionen nach Rejoin.

## 11. Ausfuehrungsinkrement 30 Tage (Startpaket)

### Woche 1

- [ ] Baseline-Messung fuer Query p95/p99, Inference p95, CDC-Latenz und Queue-Tiefen unter Normalbetrieb erfassen (Target: Q4 2026)
- [ ] cgroups-v2 Klassen aktivieren und auf core-db/adalora-train/cdc-index mappen (Target: Q4 2026)
- [ ] Restart- und Health-Policy fuer systemd Units vereinheitlichen (Target: Q4 2026)

### Woche 2

- [ ] Mixed-Load Harness aufsetzen (Query + AdaLoRA + CDC + Reindex parallel) (Target: Q4 2026)
- [ ] Hard-Limits fuer Queue-Groessen, Retry-Budgets und Circuit-Breaker je Stream definieren (Target: Q4 2026)
- [ ] Storage-Degrade Testfall vorbereiten (simulierter RAID-Rebuild + Foreground-Last) (Target: Q4 2026)

### Woche 3

- [ ] Gate G1 und G2 vorab validieren und erste Schwellenwerte fixieren (Target: Q4 2026)
- [ ] Crash-Recovery und Adapter-Rollback Drill automatisieren (Target: Q4 2026)
- [ ] Security fail-closed Testpfade fuer Policy/Secret/TLS Fehler in die Testmatrix aufnehmen (Target: Q4 2026)

### Woche 4

- [ ] Vorab-Gate-Review fuer G1-G4 mit Soll-Ist-Abgleich erstellen (Target: Q4 2026)
- [ ] Offene Grenzwertabweichungen priorisiert in Hardening-Backlog ueberfuehren (Target: Q4 2026)
- [ ] Go/No-Go Entscheidungsblatt fuer erste produktionsreife Freigabe vorbereiten (Target: Q4 2026)

## 12. Production Readiness Checklist

- [ ] Ressourcenisolation fuer alle Workload-Klassen ist aktiv und nachweisbar.
- [ ] AdaLoRA verletzt Core-DB-SLOs unter Last nicht (oder wird hart gedrosselt).
- [ ] RAID-Rebuild und Shard-Repair laufen parallel ohne unkontrollierten p99-Anstieg.
- [ ] CDC/RAG-Pipeline bleibt unter Burst-Last speicherstabil und backlog-kontrolliert.
- [ ] Multi-Region Konfliktfaelle sind mit stabiler Policy und Versionskonsistenz nachgewiesen.

## 13. Risiken und offene Punkte

- Zielkonflikt zwischen AdaLoRA-Durchsatz und stabiler DB-Latenz.
- Hardware-Varianz (NVMe/GPU/NUMA) verschiebt Profilgrenzen pro Umgebung.
- Multi-Region Modell-Synchronisierung (Adapter/Embeddings) braucht strikte Rollout-Governance.

## 14. Messbare Abnahmeindikatoren (Initialsatz)

- Query p99 unter Mischlast <= definierter Servicegrenze.
- Inference p95 stabil bei gleichzeitiger Replikations- und CDC-Last.
- Kein OOM/Watchdog-Kill im 24h Mixed-Load Soak.
- Erfolgreicher geordneter Rollback von AdaLoRA-Adapter-Versionen unter Last.
- Kein Datenverlust in CDC bei Backpressure-Ereignissen.

## 15. Weiterentwicklungspfad ab diesem Dokument

1. Dokument als Integrations-Referenz einfrieren und in Architektur-/Roadmap-Quellen verlinken.
2. Pro Gate G1-G6 numerische Schwellen je Zielumgebung definieren.
3. Gate-Evidence-Dateien pro Capability in den betroffenen Modulpfaden pflegen.
4. Erst nach Gate-Gruen: AdaLoRA-Tiefe, aggressivere Rebalance-Strategien und Multi-Region-Rollout erweitern.

Naechster verbindlicher Inkrementsatz:

- [ ] Numeric SLO Profile anlegen (p95/p99, RTO/RPO, max backlog, max restart/hour) (Target: Q4 2026)
- [ ] Mixed-Load Testmatrix je Capability verankern (Target: Q4 2026)
- [ ] Go/No-Go Checkliste in operative Runbooks uebernehmen (Target: Q4 2026)

## 16. Quellenbezug und Evidenzstatus

- Grundlage ist die Linux-ThemisDB Analyse aus dem Linux-Arbeitsverzeichnis sowie die in diesem Chat erarbeiteten Production-First Kriterien.
- Die in diesem Dokument genannten numerischen Gates sind bewusst als zu parametrisierende Rahmenwerte modelliert und muessen in der Zielumgebung verifiziert werden.
- Das Dokument versteht sich als Denkpaper mit Umsetzungsfokus: Hypothesen sind in pruefbare Gates und Roadmap-Aufgaben uebersetzt.

## 17. Operative Gate-Parametrisierung

Die ausfuellbare numerische Gate-Parametrisierung liegt in:

- LINUX_THEMISDB_GATE_DECISION_SHEET.md

Dieses Sheet ist das operative Entscheidungsartefakt fuer Go/No-Go pro Zielumgebung.
