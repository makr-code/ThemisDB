# default_method.yaml — Detailerklärung

## Zweck
`default_method.yaml` ist das primäre, allgemeine Prozessmodell für wissenschaftliches Arbeiten im ThemisDB-Dream-Mode.  
Es deckt die vollständige Kette von Problemklassifikation über Strategieerzeugung bis zur kontrollierten Promotion ab.

## Einsatzkontext
- Modus: `dream_research` (verpflichtend)
- Ziel: evidenzbasierte, reproduzierbare Problembearbeitung mit BPMN- und YAML-gestütztem Prozessgedächtnis
- Sicherheitsprinzip: fail-closed + Human Approval vor Promotion

## Phasenlogik (8 Phasen)
1. **Problem Intake and Classification**  
   Klassifiziert Problemraum (Domäne, Risiko, Unsicherheit, Evidenzbedarf).
2. **BPMN Strategy Mapping**  
   Übersetzt Klassifikation in BPMN-Strategiebausteine (`diagnose -> hypothesis -> test -> decision -> rollback`).
3. **YAML Epistemic Model Derivation**  
   Leitet run-spezifisches Erkenntnismodell ab (Annahmen, Falsifikationskriterien, Metriken, Stop-Regeln).
4. **Candidate Strategy Dreaming**  
   Erzeugt mehrere synthetische Kandidatstrategien (nicht produktiv).
5. **Syntax and Schema Validation**  
   Prüft BPMN-/YAML-Konformität.
6. **Semantic / Governance Validation**  
   Prüft Prozesskonsistenz, Auditierbarkeit und Gate-Vollständigkeit.
7. **Decision and Versioned Promotion**  
   Entscheidet `no_go|research_only|promotion_candidate`.
8. **Strategy Library Evolution**  
   Bewertet Strategien, verwirft schwache Muster, übernimmt starke Muster versioniert in die Bibliothek.

## Gate- und Governance-Anbindung
- Pflicht-Gates: `BRAIN-DREAM-*` + `BRAIN-BPMN-PROCEDURAL-INTEGRITY`
- Ohne Gate-Closure bleibt Ergebnis `research_only`
- Promotion nur mit Approval-Referenz + Rollback-Checkpoint

## Ergebnisartefakte
- `gate_report`
- `decision` + `approval_reference`
- `failure_mode_profile`
- `library_update_patch`
