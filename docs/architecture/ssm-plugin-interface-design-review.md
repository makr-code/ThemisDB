# ISSMPlugin Interface Design Review (P1-D01)

Status: proposed / blocked pending human approval  
Target branch: `develop`  
Depends on: `/home/runner/work/ThemisDB/ThemisDB/docs/architecture/ssm-hybrid-rollout-plan.md`

## Zweck

Human-review Pflichtartefakt vor Implementierung von `ISSMPlugin` und `SSMStateStore`.

## Aktueller Repository-Status

- Implementierung von P1+ bleibt **blockiert**, bis ein menschlicher Reviewer dieses Dokument explizit freigibt.
- Dieses Dokument darf von AI vorbereitet und konkretisiert werden, aber **nicht** selbst auf `APPROVED` gesetzt werden.
- Phase-0-Evidenz liegt in den zugehörigen Analyseartefakten vor; sie reduziert technische Unsicherheit, ersetzt aber kein Human-Gate.

## AI-vorbereitete Empfehlung (nicht bindend)

Die folgende Empfehlung ist als Startpunkt für den menschlichen Review gedacht:

- **State-Distribution-Strategie:** `replication`
  - Begründung: Für den ersten produktionsnahen SSM-State-Pfad ist deterministisches Resume und geringeres Recovery-Risiko wichtiger als horizontale Maximalskalierung.
  - Konsequenz: Höherer Write-Overhead wird akzeptiert, bis Phase-3+/L5a-L5b stabil produktionsreif ist.
- **Fehlersemantik:** `fail-closed`
  - Begründung: Bei Cross-Shard-State-Verlust darf kein stiller Wechsel auf potenziell inkonsistenten oder tenant-fremden State erfolgen.
  - Konsequenz: Expliziter Fehler/Fallback auf den bestehenden Transformer/RAG-Pfad statt opportunistischer State-Wiederverwendung.
- **Konsistenzmodell:** `bounded-staleness`
  - Begründung: Stärker als eventual consistency, aber ohne die Betriebs-/Latenzkosten eines unnötig strengen Strong-Consistency-Zwangs für jedes Checkpoint-Ereignis.
  - Konsequenz: SSM-State-Resume muss an `LLMQueryContext::snapshot_ts` gebunden bleiben; ältere Snapshots außerhalb des erlaubten Fensters werden verworfen.
- **Ownership / Migration-Pfad:**
  - Phase 1: In-Memory `SSMStateStore` mit HLC-Snapshot-Bindung.
  - Phase 2: interne persistente Variante auf RocksDB-/KV-Prefix-Muster.
  - Phase 3+: verteilte Strategien erst nach L5a/L5b-Freigabe.
- **Governance-Fixierung:**
  - ThemisDB bleibt System-of-Record.
  - RocksDB dient nur als interner Persistenzbaustein für State-Snapshots.
  - Tenant-Isolation, Auditierbarkeit und explizite Zugriffskontrollen sind Freigabekriterien, keine spätere Option.

## Pflicht-Entscheidungen (Gate)

- [ ] **State-Distribution-Strategie (L5):**
  - [ ] `replication` (mehr Redundanz, höherer Write-Overhead)
  - [ ] `shard_partitioned` (bessere Skalierung, höheres Recovery-Risiko)
- [ ] **Fehlersemantik bei Cross-Shard-State-Ausfall:**
  - [ ] fail-closed
  - [ ] fail-open
- [ ] **Konsistenzmodell:**
  - [ ] eventually consistent
  - [ ] bounded-staleness
  - [ ] strong consistency (nur mit expliziter Begründung)
- [ ] **Ownership + Removal/Migration-Pfad** für Übergang von In-Memory zu persistenter Store-Variante dokumentiert.
- [ ] **Mamba Security/Governance-Klarstellung:**
  - ThemisDB bleibt Datenautorität (System-of-Record)
  - RocksDB nur interne Persistenz, kein externer Parallel-DB-Pfad
  - Zugriff/Audit für SSM-State-Snapshots verbindlich spezifiziert

## Review-Mindestkriterien

- [ ] Explizite Begründung dokumentiert, falls **nicht** `replication` gewählt wird
- [ ] Explizite Begründung dokumentiert, falls **nicht** `fail-closed` gewählt wird
- [ ] Konsistenzentscheidung mit `LLMQueryContext::snapshot_ts` und Resume-Verhalten abgeglichen
- [ ] Tenant-Isolation für Snapshot-Keying, Restore und Audit-Events beschrieben
- [ ] Fallback-Verhalten auf bestehenden Transformer/RAG-Pfad beschrieben
- [ ] Removal-/Migration-Target für spätere persistente und/oder verteilte Stores benannt

## L5-Ausrichtung (muss vorliegen)

- [ ] L5a-Abhängigkeit bestätigt: produktive P3+-Features erst nach Sharding Phase-C Gate.
- [ ] L5b-Abhängigkeit bestätigt: AdaLoRA-Rank-Awareness/Cross-Shard-Importance als Voraussetzung für konsistente Federation.

## Ergebnis

- [ ] **APPROVED** (Implementierung darf starten)
- [ ] **CHANGES REQUESTED** (Blocker offen)

> Solange kein menschlicher Reviewer `APPROVED` markiert, ist der effektive Status
> dieses Gates **nicht freigegeben / changes requested**.

Reviewer (human): ____________________  
Datum: ____________________
