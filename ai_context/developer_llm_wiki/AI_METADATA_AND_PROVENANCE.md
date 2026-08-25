# Developer LLM Wiki -- Canonical AI Metadata and Provenance

Datum: 2026-08-21
Status: Active
Bezug: ThemisDB-weites Provenienz- und Degradationsmodell fuer menschliche, hybride und LLM-synthetische Inhalte
Primary (Quelle der Wahrheit): include/llm_wiki/llm_wiki_plugin_interface.h, ai_working/AI_WIKI_INTEGRATION_PLAYBOOK.md, plugins/themisdb_llm_wiki/FUTURE_ENHANCEMENTS.md, plugins/themisdb_llm_wiki/ROADMAP.md

## Scope

- Kanonischer AI-Metadatensatz fuer die gesamte ThemisDB, nicht nur fuer die Wiki-LLM.
- Gilt fuer Dokumente, Wiki-Seiten, Claims, Graph-Kanten, Query-Antworten und Transformationsschritte.
- Ziel ist eine auditierbare Grenze zwischen menschlich verankerten, hybriden und vollsynthetischen Inhalten.

## Zielbild

- MVCC bleibt die technische Zeitachse.
- Graph-Verweise bleiben die Herkunftsachse.
- Der AI-Metadatensatz wird die semantische Herkunfts- und Qualitaetsachse.
- Jede relevante Entitaet tragt ihre Transformations- und Provenienzspur mit.

## Kanonische Felder

### Basisfelder

- `entity_id`
- `entity_type` (`document`, `page`, `claim`, `edge`, `chunk`, `transform`)
- `version_id`
- `parent_version_id`
- `origin_type` (`human`, `llm`, `hybrid`, `imported`)
- `source_refs[]`
- `created_at`
- `updated_at`

### Transformationsfelder

- `model_id`
- `pipeline_id`
- `prompt_hash`
- `transform_hash`
- `transform_step_count`
- `llm_edit_count`
- `actor_id`
- `session_id`
- `run_id`

### Provenienz- und Qualitaetsfelder

- `human_anchor_count`
- `synthetic_chain_length`
- `semantic_drift_score`
- `provenance_confidence`
- `trust_score`
- `reanchor_required`
- `policy_tags[]`

## Ebenenmodell

### Document

- Rohquelle, Importquelle oder externe Referenz.
- Speichert Ursprung, Integritaetsstatus und Referenzen auf die Erstquelle.

### Page

- Zusammengefasste Wissenseinheit mit Versionskette.
- Traegt die komplette Transformationshistorie sowie den aktuellen Provenienzstatus.

### Claim

- Einzelne Aussage mit eigener Herkunft und Drift-Historie.
- Massgeblich fuer Audit, Re-Anchor und Ranking.

### Edge

- Graph-Relation mit Quellenverweis, Ableitungsart und Vertrauensgrad.
- Dient als Relationenanker fuer semantische Navigation und Widerspruchsanalysen.

## Degradationsmodell

- Jede reine LLM-Weitergabe erhoht `synthetic_chain_length`.
- Jede menschliche Bestatigung erhoeht den Ankerwert und senkt die synthetische Unsicherheit.
- `semantic_drift_score` misst die Abweichung gegen die letzte menschlich verankerte oder Rohquellen-basierte Version.
- `provenance_confidence` kombiniert Ankerdichte, Quellennahe und Drift.

### Praktische Schwellen

- `synthetic_chain_length = 0`: direkt verankert.
- `synthetic_chain_length = 1..2`: beobachtbar, aber noch brauchbar.
- `synthetic_chain_length >= 3`: degradationskritisch.
- Hoher Drift oder fehlende Quellenreferenz: Re-Anchor erforderlich.

## ThemisDB-Einbaupunkte

- Wiki- und Plugin-Vertrag: `include/llm_wiki/llm_wiki_plugin_interface.h`
- Workspace- und Iterationslogik: `plugins/themisdb_llm_wiki/include/wikipedia/wiki_workspace_orchestrator.h`
- Roadmap und Qualitaetsziele: `plugins/themisdb_llm_wiki/ROADMAP.md`, `plugins/themisdb_llm_wiki/FUTURE_ENHANCEMENTS.md`
- Wiki-Betriebsrahmen: `ai_working/AI_WIKI_INTEGRATION_PLAYBOOK.md`
- Developer-Wiki-Sync: `scripts/ai-dev-llm-wiki-sync.py`
- Metadaten-Helfer: `scripts/add_doc_metadata.py`
- Drift-Pruefung: `scripts/drift-detector.py`

## Auswertungsregeln

- `stats()` soll Provenienz- und Degradationskennzahlen ausgeben.
- `wikiLint()` soll rein synthetische Ketten, fehlende Quellen und Drift markieren.
- Query- und Ranking-Pfade sollen menschlich verankerte Inhalte bevorzugen.
- Importer und Graph-Schichten sollen denselben Metadatenvertrag benutzen.

## Operationaler Workflow

1. Rohquelle ingestieren und als Ursprung markieren.
2. Transformationsschritt mit Modell- und Prompt-Metadaten speichern.
3. Claim- und Edge-Ebene mit denselben Provenienzreferenzen schreiben.
4. Degradationskennzahlen berechnen und persistieren.
5. Lint, Stats und Ranking gegen die Kennzahlen auswerten.
6. Bei kritischer Kette oder Drift Re-Anchor gegen die Rohquelle ausloesen.

## Teststrategie

- Human -> LLM -> LLM -> Query-Fall mit steigender synthetischer Kettenlaenge.
- Re-Anchor-Fall mit sinkendem Drift nach menschlicher Korrektur.
- Importfall mit unveraenderter Rohquelle und mehrfacher LLM-Weiterverarbeitung.
- Graph-Kante mit Ursprung und Ableitungsart.

## Ergebnis

- Dieser Metadatensatz ist die kanonische Basis fuer AI-nahe Inhalte in ThemisDB.
- Die Wiki-LLM ist nur ein erster grosser Verbraucher dieser Canonical-Layer.
- Alle weiteren AI-nahe Module sollen dieselbe Provenienzlogik verwenden.