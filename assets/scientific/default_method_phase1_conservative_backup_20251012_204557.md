# default_method_phase1_conservative_backup_20251012_204557.yaml — Detailerklärung

## Zweck
Konservatives Fallback-Profil für frühe Prozessphasen, wenn Unsicherheit hoch ist oder wichtige Informationen/Abhängigkeiten fehlen.

## Wann verwenden?
Aktivierung bei:
- niedriger initialer Confidence,
- fehlendem kritischem Kontext,
- nicht verfügbaren externen Spezial-Agents.

## Charakteristik
- Supervisor deaktiviert (`supervisor_enabled: false`)
- Strengere, defensivere Ausführung in Phase 1/2
- Explizite Unknowns werden erzwungen (`require_explicit_unknowns: true`)
- Keine produktive Promotion erlaubt

## Prozesskern
1. **problem_intake_classification**  
   Vorsichtige Hypothesenbildung mit Fokus auf Lücken und Prüfbedingungen.
2. **conservative_evidence_synthesis**  
   Defensives Evidenzclustering mit Konfliktkennzeichnung statt aggressiver Schlussfolgerung.

## Entscheidungsgrenzen
- Erlaubt: `no_go`, `research_only`
- Verboten: `promotion_candidate`

## Governance-Nutzen
Dieses Profil reduziert Fehlpromotionsrisiko in frühen oder datenarmen Situationen und hält den Prozess vollständig auditierbar.
