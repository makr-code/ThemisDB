# agentic_bpmn_coding_loop.yaml — Detailerklärung

## Zweck
Spezialisiertes Prozessmodell für coding-zentrierte Agentenarbeit mit klaren Verifikations- und Rollback-Pfaden.

## Leitidee
Der Prozess verbindet typische Entwicklungszyklen (Analyse, Patch, Test, Review) mit wissenschaftlicher Evidenzführung und Governance-Gates.

## Hauptablauf
1. **Scope and Constraints Capture**  
   Definiert Änderungsgrenzen, Nicht-Ziele und Sicherheitsannahmen.
2. **Patch Strategy Planning**  
   Erstellt mehrere Patch-Strategien mit erwarteten Risiken/Nutzen.
3. **Candidate Patching**  
   Erzeugt Kandidatänderungen inkrementell.
4. **Build/Test Validation**  
   Führt Build-/Test- und Qualitätsprüfungen aus.
5. **Governance & Safety Check**  
   Prüft Policy-Compliance, Nachvollziehbarkeit und Fail-Closed-Bedingungen.
6. **Promotion / Rollback Decision**  
   Trifft kontrollierte Entscheidung mit Approval-Referenz.

## Typische Failure Modes
- grüne Tests bei unvollständiger Abdeckung,
- regressionsanfällige Randfälle,
- Policy-Verletzung trotz funktionalem Patch.

## Kontrollmaßnahmen
- Gate-Pflicht vor Promotion,
- explizite Risiko-/Rollback-Dokumentation,
- versionierte Übergabe nur bei validierter Evidenzlage.
