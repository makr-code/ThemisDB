# AI Safety Architecture — Developer Reference

> **Für:** Backend-Entwickler, Security-Engineers
>
> Technische Kurzreferenz für die Implementierung des AI Safety Layers.
> Vollständige Nutzerdokumentation: `docs/de/security/ai_safety/`

## Motivation

KI-Agenten (MCP-Clients, AI Orchestrator, LLM-Pipeline) können an mehreren Stellen
in ThemisDB Datenbankoperationen ausführen. Ohne geeignete Schutzmaßnahmen
besteht das Risiko unbeabsichtigter oder böswilliger Datenzerstörung —
analog zum Cursor-KI-Vorfall (April 2026).

## Architekturentscheidungen

### Warum deterministisch (kein LLM-Guard)?

Der AI Safety Layer verwendet ausschließlich regelbasierte Klassifikation.
Ein KI-basierter Guard wäre selbst ein LLM-Aufruf — mit Latenz, Nondeterminismus
und potenziellem Prompt-Injection-Risiko. Regelbasiert: < 0.1ms, deterministisch,
kein Netzwerkzugriff.

LoRA-Adapter als Drop-In-Ersatz für den IntentClassifier ist in Phase 4 geplant
(IMPL-A2, Q4 2026) — aber nur für die Klassifikationsschicht, nicht für Guards.

### Warum synchroner Snapshot?

Async würde bedeuten, der Snapshot könnte nach Beginn der Operation fertiggestellt
werden. RocksDB-Checkpoints (Hardlinks) sind O(1) und typischerweise < 500ms.

## Zu implementierende Klassen

### Phase 1 (Q2 2026)

```
src/security/intent_classifier.cpp
  → IntentType::DATA_DESTRUCTION, IntentType::SCHEMA_MUTATION hinzufügen
  → kDataDestructionFeatures[], kSchemaMutationFeatures[] hinzufügen
  → classify() um zwei neue Kandidaten erweitern
  → riskDelta() + intentName() anpassen

src/query/aql_safety_validator.cpp  [NEU]
include/query/aql_safety_validator.h  [NEU]
  → AqlSafetyValidator::validate(aql_query) → ValidationResult
  → Tokenbasierte Mutation-Detection (REMOVE, INSERT, UPDATE, REPLACE, UPSERT, DROP, TRUNCATE)

src/server/mcp_server.cpp
  → toolQuery(): AqlSafetyValidator aufrufen wenn enforce_read_only aktiv
  → toolDeleteEntity(): dry_run-Flag auswerten
  → toolDropIndex(): dry_run-Flag auswerten
```

### Phase 2 (Q3 2026)

```
include/security/ai_operation_guard.h  [NEU]
src/security/ai_operation_guard.cpp  [NEU]
  → OperationClass enum (READ_ONLY, WRITE_SAFE, DESTRUCTIVE, CRITICAL)
  → AiOperationGuard::evaluate() → GuardDecision
  → AiOperationGuard::buildRequiresApprovalResponse()

src/server/mcp_server.cpp
  → pending_approvals_ map + mutex
  → toolQuery/toolDeleteEntity/toolDropIndex: Guard aufrufen
  → safety-Sektion aus Mode-Config laden

src/server/http_server.cpp
  → POST /v1/ai/approve/{id}
  → POST /v1/ai/deny/{id}
  → GET  /v1/ai/pending-approvals
```

### Phase 3 (Q3 2026)

```
src/server/mcp_server.cpp
  → Pre-Op-Snapshot vor Execution
  → environment-Config aus security.yaml lesen

src/server/http_server.cpp
  → POST /v1/ai/rollback/{snapshot_id}

config/security.yaml
  → environment: + ai_agent_restrictions: Block hinzufügen

config/ai_ml/llm/modes/default.yaml
  → safety: Block in agentic/multi_agent Modes
```

### Phase 4 (Q4 2026)

```
src/utils/audit_logger.cpp
  → Neue Event-Typen: AI_TOOL_CALL, AI_APPROVAL_REQUIRED, AI_OPERATION_EXECUTED, ...

src/security/intent_classifier.cpp
  → IMPL-A2: LoRA-Adapter als classify()-Implementierung

tests/security/ai_safety/
  → test_ai_operation_guard.cpp
  → test_ai_environment_guard.cpp
  → test_ai_snapshot.cpp
  → test_ai_audit_trail.cpp
  → test_intent_classifier_aql.cpp

src/query/test_aql_safety_validator.cpp [in tests/query/]
```

## Test-Strategie

```
Unit-Tests:
  - DOG-Klassifikation (alle Operationsklassen + AQL-Spezialfälle)
  - HILG-Approval-Flow (happy path, expiry, replay prevention)
  - AQL Validator (False-Positive-Prüfung auf echten Queries)
  - IntentClassifier (neue AQL-Features)
  - Environment Guard (production vs. staging vs. dev Matrix)

Integration-Tests (Chaos):
  - Simulierter destruktiver KI-Agent gegen alle Guards
  - Snapshot + Rollback End-to-End
  - Concurrent Approval (Race Condition)

Performance-Tests:
  - DOG-Overhead < 0.1ms (p99)
  - AQL-Validator < 0.1ms bei 1KB-Query
  - Snapshot-Erstellung p99 < 2s bei 100GB DB
```

## Abhängigkeiten

```
AiOperationGuard   → AqlSafetyValidator (für Query-Klassifikation)
McpServer          → AiOperationGuard (Guard vor jeder Tool-Exec)
McpServer          → RocksDBWrapper::createCheckpoint() (POS)
McpServer          → AuditLogger (neue AI-Events)
HttpServer         → McpServer::pendingApprovals (Approval-API)
IntentClassifier   → unabhängig (bestehendes Interface, neue Features)
```

## Verwandte Dokumente

- Vollständige Nutzerdoku: `docs/de/security/ai_safety/`
- Security ROADMAP: `src/security/ROADMAP.md` (Phase 5: AI Safety Layer)
- STUB_INVENTORY: `src/STUB_INVENTORY.md` (IntentClassifier STUB-Eintrag)
- FUTURE_ENHANCEMENTS: `src/security/FUTURE_ENHANCEMENTS.md`
