# AI Safety Architecture — Developer Reference

> **Für:** Backend-Entwickler, Security Engineers, Reviewer
>
> Diese Seite beschreibt den **aktuellen** AI-Safety-Stand im Security-Modul.
> Vollständige Betriebs-/Nutzerdokumentation: `docs/de/security/ai_safety/`.

## 1) Zweck und Status

Der AI Safety Layer schützt KI-getriebene Datenbankoperationen (MCP/agentic Tool-Use)
vor destruktiven oder unkontrollierten Aktionen durch mehrschichtige, deterministische
Kontrollen.

- **Status:** produktiv integriert; Implementierungsfortschritt wird in
  `src/security/ROADMAP.md` (Phase 5: AI Safety Layer) geführt.
- **Nicht mehr gültig:** Die frühere Annahme „Planungs-/Stub-Stand Phase 1–4“ ist veraltet.

## 2) Aktuelles Bedrohungsmodell (AI-spezifisch)

| Bedrohung | Beispiel | Primäre Controls |
|---|---|---|
| Unbeabsichtigte Datenlöschung | `delete_entity`, `DROP INDEX`, unbeschränkte `REMOVE`-Queries | `AiOperationGuard`, Human-in-the-Loop Approval, Dry-Run Preview |
| Prompt-/Tool-Missbrauch | Agent führt mutierende AQL im read-only Kontext aus | `AqlSafetyValidator`, Modus-Flags (`enforce_read_only`) |
| Falsche Umgebungsannahme | Agent behandelt Produktion wie Dev/Test | Environment Restrictions aus `config/security.yaml` |
| Fehlende Wiederherstellbarkeit | Destruktive Aktion ohne Rollback-Punkt | Pre-Operation Snapshot + Rollback-API |
| Fehlende Forensik | Keine nachvollziehbare KI-Aktionsspur | AI Session Audit Trail Events |

## 3) Control-Architektur (aktuell)

1. **AQL Read-Only Enforcer**
   `include/query/aql_safety_validator.h`, `src/query/aql_safety_validator.cpp`
   Blockiert mutierende AQL-Operationen in read-only Tool-Kontexten.

2. **Destructive Operation Guard (DOG)**
   `include/security/ai_operation_guard.h`, `src/security/ai_operation_guard.cpp`
   Klassifiziert Operationen (`READ_ONLY`, `WRITE_SAFE`, `DESTRUCTIVE`, `CRITICAL`).

3. **Human-in-the-Loop Gate (HILG)**
   `src/server/mcp_server.cpp`, `src/server/http_server.cpp`
   Approval/Denial-Flow für riskante Operationen inkl. Pending-Approval-Verwaltung.

4. **Environment Isolation Guard**
   `config/security.yaml` (`environment`, `ai_agent_restrictions`)
   Erzwingt Umgebungsgrenzen (z. B. produktionsspezifische Restriktionen).

5. **Pre-Operation Snapshot + Rollback**
   `McpServer` + Storage Checkpoint/Restore-Hooks
   Sicherung vor Ausführung destruktiver/critical Operationen mit Rollback-Pfad.

6. **AI Session Audit Trail**
   `src/utils/audit_logger.cpp` (AI-Eventtypen)
   Manipulationssichere Nachvollziehbarkeit von Tool-Aufrufen, Approvals und Ausführung.

## 4) Betriebsgrenzen / Operating Limits

- Der AI Safety Layer schützt **KI-initiierte Tool-Pfade** (MCP/Agentic-Workflows), nicht
  beliebige externe Non-AI Admin- oder Direktzugriffe.
- Schutzwirkung hängt von korrekter Modus-/Umgebungskonfiguration ab
  (`config/ai_ml/llm/modes/default.yaml`, `config/security.yaml`).
- Der Layer ergänzt, ersetzt aber nicht:
  - Authentifizierung (`src/auth/**`)
  - klassische Autorisierung/RBAC/RLS (`src/security/rbac.cpp`, `row_level_security.cpp`)
  - Krypto-/Key-Management (Vault/HSM/PKI).
- Performance-/Chaos-/Erweiterungsziele werden in `ROADMAP.md` und
  `FUTURE_ENHANCEMENTS.md` weitergeführt; diese Datei beschreibt den Architekturvertrag.

## 5) Abgrenzung zu HSM-/Auth-/Policy-Dokumenten

| Thema | Führendes Dokument |
|---|---|
| Modulweite Sicherheitsarchitektur (Krypto, RBAC, HSM, PKI) | `src/security/ARCHITECTURE.md` |
| Security Threat Model & Controls (gesamt) | `src/security/SECURITY.md` |
| Security Audit-Findings / Remediation-Status | `src/security/AUDIT.md` |
| AuthN/Auth-Flow (JWT/OIDC/MFA/Session) | `src/auth/ARCHITECTURE.md` |
| Governance/Policy/Compliance-Layer | `src/governance/ARCHITECTURE.md` |
| AI-Safety Betriebsdoku (Runbook, Validator, Snapshot, Audit Trail) | `docs/de/security/ai_safety/README.md` |

## 6) Review- & Audit-Nachweis (Dokument-Update)

**Review-Referenzen (Pflichtquellen):**
- `docs/DOCUMENTATION_REVIEW_GUIDELINES.md`
- `docs/SYSTEMATISCHER_REVIEWPLAN.md`
- `docs/de/development/SOURCE_CODE_AUDIT.md`
- `docs/audit-framework/AUDIT_RUNBOOK.md`

**Durchgeführte Checks für dieses Update (2026-05-13):**
- ✅ Fachreview gegen Security-Kerndokumente (`SECURITY.md`, `ARCHITECTURE.md`, `AUDIT.md`, `ROADMAP.md`)
- ✅ Sourcecode-/Dokumentationsaudit der AI-Safety-Pfade und Referenzdateien
  (`include/security/ai_operation_guard.h`, `src/security/ai_operation_guard.cpp`,
  `include/query/aql_safety_validator.h`, `src/query/aql_safety_validator.cpp`,
  `src/security/intent_classifier.cpp`, `src/server/mcp_server.cpp`, `src/server/http_server.cpp`)
- ✅ Ergebnis verlinkt über die oben genannten Kern-/Audit-Dokumente und
  `docs/de/security/ai_safety/AI_SAFETY_ARCHITECTURE.md`
- ✅ Betroffene Datei im Review festgehalten: `src/security/AI_SAFETY_ARCHITECTURE.md`

## 7) Verwandte Dokumente

- `src/security/README.md`
- `src/security/ROADMAP.md`
- `src/security/FUTURE_ENHANCEMENTS.md`
- `docs/de/security/ai_safety/README.md`
