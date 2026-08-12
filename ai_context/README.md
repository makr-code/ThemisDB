# ai_context

Datum: 2026-07-28
Status: Active
Bezug: Persistente Wissensbasis fuer AI-Agenten in ThemisDB
Primary (Quelle der Wahrheit): ai_context/COPILOT_INSTRUCTIONS.md, DOCUMENTATION_GOVERNANCE.md, AI_WIKI_INTEGRATION_PLAYBOOK.md

Persistente Wissensbasis für KI-Agenten im ThemisDB-Repository.

## Zweck

`ai_context/` enthält langlebige, kuratierte Wissensartefakte, die Agenten für konsistente Implementierungsentscheidungen benötigen.

## Ablage-Konventionen

- Nur stabile, modulübergreifend relevante Inhalte ablegen
- Keine temporären Entwürfe oder Session-Notizen
- Jede Datei mit klarer Quelle/Referenz pflegen
- Änderungen wie Produktcode reviewen (Korrektheit vor Umfang)

## Struktur

- [memory_management_policy.md](memory_management_policy.md) — Ownership-/Lifetime-Regeln (RAII)
- [api_contracts/README.md](api_contracts/README.md) — maschinenlesbare API-Vertragsübersichten
- [KNOWLEDGE_LINT_REPORT.md](KNOWLEDGE_LINT_REPORT.md) — aktueller AI-Context-Lintstatus (automatisch fortgeschrieben)
- [KNOWLEDGE_CONFLICTS.md](KNOWLEDGE_CONFLICTS.md) — Konfliktregister mit manuellem und auto-detektiertem Bereich
- [API_MODULE_STATUS_2026_07_18.md](API_MODULE_STATUS_2026_07_18.md) — referenzierte Status-Synchronisation fuer API

## Automatisierung (LLM-Wiki-Loop)

- Workflow: `.github/workflows/maintenance-docs.yml`
- Scripts:
	- `scripts/ai-context-lint.py` (Lint + Konflikt/Report-Updates fuer bestehenden AI-Context)
	- `scripts/ai-dev-llm-wiki-sync.py` (Developer-LLM-Wiki Sync in `ai_context/developer_llm_wiki/`)
- Framework-Spezifikation:
	- `ai_context/DEVELOPER_LLM_WIKI_FRAMEWORK.md`
- Betriebsart:
	- `push`/`pull_request`/`schedule`: pruefende Laeufe mit Reports als Artefakt
	- `workflow_dispatch`:
		- `apply_ai_updates=true` fuer AI-Context-Lint-Updates
		- `llm_wiki_apply_updates=true` fuer Developer-LLM-Wiki-Updates
		- `llm_wiki_full_sync=true` fuer initialen/globalen Migrationslauf
		- konkrete Erstlauf-Runbook-Sequenz: `ai_context/DEVELOPER_LLM_WIKI_FRAMEWORK.md` Abschnitt `8) Erster globaler Logic-Run`

## Initiale ADR-Referenzen

Hinweis: Die ADRs liegen aktuell unter `research/architecture_decisions/`.

- [ADR Übersicht](../research/architecture_decisions/README.md)
- [Decision Log](../research/architecture_decisions/decision_log.md)
- [ADR 001 — HNSW over FAISS](../research/architecture_decisions/adr_001_hnsw_over_faiss_vector_index.md)
- [ADR 002 — RocksDB backend](../research/architecture_decisions/adr_002_rocksdb_storage_backend.md)
- [ADR 003 — Boost Beast/ASIO HTTP](../research/architecture_decisions/adr_003_boost_beast_asio_http_server.md)
- [ADR 004 — Multi-model data model](../research/architecture_decisions/adr_004_multi_model_data_model.md)
- [ADR 005 — Argon2id](../research/architecture_decisions/adr_005_argon2id_over_scrypt_bcrypt.md)
- [ADR 006 — Chimera adapter architecture](../research/architecture_decisions/adr_006_plugin_chimera_adapter_architecture.md)
- [ADR 007 — gRPC internal RPC](../research/architecture_decisions/adr_007_grpc_for_internal_rpc.md)
- [ADR 008 — JWT/OAuth2 auth](../research/architecture_decisions/adr_008_jwt_oauth2_for_api_auth.md)
- [ADR 009 — Algorithm validation framework](../research/architecture_decisions/adr_009_algorithm_validation_framework.md)

## Installation

Keine Installation erforderlich; dieses Verzeichnis ist Teil des Repository-Inhalts.

## Usage

Dateien hier als Referenz lesen und bei inhaltlichen Änderungen im selben PR mit den betroffenen Code-/Dokumentationsänderungen synchron halten.

---
Zuletzt geprueft (AI-Context): 2026-07-28
