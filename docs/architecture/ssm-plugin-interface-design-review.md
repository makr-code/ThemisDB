# ISSMPlugin Interface Design Review (P1-D01)

Status: proposed  
Target branch: `develop`  
Depends on: `/home/runner/work/ThemisDB/ThemisDB/docs/architecture/ssm-hybrid-rollout-plan.md`

## Zweck

Human-review Pflichtartefakt vor Implementierung von `ISSMPlugin` und `SSMStateStore`.

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

## L5-Ausrichtung (muss vorliegen)

- [ ] L5a-Abhängigkeit bestätigt: produktive P3+-Features erst nach Sharding Phase-C Gate.
- [ ] L5b-Abhängigkeit bestätigt: AdaLoRA-Rank-Awareness/Cross-Shard-Importance als Voraussetzung für konsistente Federation.

## Ergebnis

- [ ] **APPROVED** (Implementierung darf starten)
- [ ] **CHANGES REQUESTED** (Blocker offen)

Reviewer (human): ____________________  
Datum: ____________________

