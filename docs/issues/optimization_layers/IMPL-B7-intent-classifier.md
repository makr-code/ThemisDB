---
type: enhancement
labels: ["type:enhancement", "module:security", "module:training", "priority:high", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: IMPL-EPIC-B
session: S-7
layer: 7
---

# [IMPL-B7] Layer 7: IntentClassifier — Security Semantic Layer

## Aufgabe

`LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer-7 definiert eine semantische Erweiterung
des `MLAnomalyDetector` und `ZeroTrustPolicyEnforcer`: ein `IntentClassifier` der
den **semantischen Inhalt** einer Abfrage klassifiziert (z.B. SQL-Injection-Versuch,
Daten-Exfiltrations-Muster, Privilege-Escalation) — über reine Regex-Muster hinaus.
Bei Erkennung warnt er alle Shards via Layer 11 (DK-2 Gossip).

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `IntentClassifier` Klasse mit `classify()` + `IntentAlert` | Neue ZeroTrust-Regeln (Advisory → ZeroTrust übergibt) |
| Intent-Typen: LEGITIMATE, SQL_INJECTION, DATA_EXFILTRATION, PRIVILEGE_ESCALATION, ANOMALOUS | ML-Training des Klassifizierers (LoRA-Adapter aus Loop-1) |
| `session_risk_score`-Update auf `ZeroTrustContext` | Neue `ZeroTrustContext`-Felder |
| `AIDecisionAuditor`-Eintrag pro Intent-Alert | DBA-Dialog für Intent-Erklärung (→ IMPL-B9) |
| Confidence-Gate ≥ 0.85 vor Alarm | Vollständige NLP-Pipeline |

## Idee / Konzept

```
Query: "SELECT * FROM users WHERE id=1 OR 1=1 --"
→ IntentClassifier.classify(query, session_context)
→ IntentType::SQL_INJECTION, confidence=0.97
→ IntentAlert{severity=HIGH, session_id=..., evidence_embedding=[...]}
→ ZeroTrustPolicyEnforcer: session_risk_score += delta(severity)
→ [Layer 11, DK-2] GossipProtocol broadcastet IntentAlert → alle Shards erhöhen risk_score

Query: "SELECT revenue FROM orders WHERE month='Dec'"
→ IntentType::LEGITIMATE, confidence=0.99
→ kein Alert, kein Risk-Score-Update
```

**Advisory-only bis kalibriert:** Der Klassifizierer blockiert per Default nicht —
er erhöht `session_risk_score` und schreibt `DecisionRecord`. Erst wenn
`ZeroTrustPolicyEnforcer` den Schwellenwert überschreitet, blockiert der bestehende
ZeroTrust-Mechanismus. Trennung von Klassifizierung und Enforcement.

## Technische Details

```cpp
// Neue Datei: include/security/intent_classifier.h
class IntentClassifier {
public:
    enum class IntentType {
        LEGITIMATE,
        SQL_INJECTION,
        DATA_EXFILTRATION,
        PRIVILEGE_ESCALATION,
        ANOMALOUS_PATTERN
    };

    struct ClassificationResult {
        IntentType intent;
        double     confidence;          // [0.0, 1.0]
        std::string primary_indicator;  // "OR_1=1_pattern" | "UNION_SELECT" | ...
    };

    struct IntentAlert {
        IntentType  intent;
        double      confidence;
        std::string session_id;
        std::string shard_id;
        std::vector<float> evidence_embedding;  // für Layer-11-Gossip (kein Klartext)
        double      risk_delta;                 // Δ für session_risk_score
    };

    ClassificationResult classify(
        const std::string& query,
        const ZeroTrustContext& session_context
    ) const;

    // Erzeugt IntentAlert wenn confidence >= threshold (default 0.85)
    std::optional<IntentAlert> maybeAlert(
        const ClassificationResult& result,
        const std::string& session_id,
        double confidence_threshold = 0.85
    ) const;
};
```

**Klassifizierungs-Mechanismus (ohne echtes ML-Modell für v1.0):**
- Regelbasierte Feature-Extraktion: `OR 1=1`, `UNION SELECT`, `; DROP`, `--`,
  `INFORMATION_SCHEMA`, `xp_cmdshell` etc.
- Gewichtete Feature-Summe → logistische Transformation → Konfidenz
- Designiert als **Platzhalter für LoRA-Adapter** aus Loop-1

```
// STUB/SIMULATION NOTE:
// Purpose: Rule-based feature classification as placeholder for LoRA-adapted model
// Activation: Always active in v1.0; LoRA adapter replaces rules post-IMPL-A2
// Production Delta: Rule-based precision ~80%; LoRA target precision ≥ 92%
// Removal Plan: Replace classify() internals with LoRA adapter call in IMPL-A2 Loop-1
```

## Abhängigkeiten

- **Vorbedingung:** keine eigene (nutzt `ZeroTrustContext` der existiert)
- **Verbessert durch:** IMPL-A2 Loop-1 Training → LoRA-Adapter ersetzt Regel-Engine
- **Parallel möglich mit:** IMPL-B5, IMPL-B6, IMPL-B8

## Erfolgskriterien

- [ ] `IntentClassifier` Klasse vorhanden
- [ ] `classify("SELECT * FROM users WHERE 1=1")` → `SQL_INJECTION`, confidence > 0.85
- [ ] `classify("SELECT name FROM products WHERE id = ?")` → `LEGITIMATE`, confidence > 0.9
- [ ] `maybeAlert()` gibt `nullopt` zurück wenn confidence < 0.85
- [ ] `IntentAlert::evidence_embedding` hat Dimension 384 (für Layer-11-Gossip)
- [ ] `IntentAlert::evidence_embedding` enthält **keine** Zeichen aus dem Original-Query
- [ ] `session_risk_score` auf `ZeroTrustContext` wird nach Alert erhöht (via Caller)
- [ ] `DecisionRecord{decision_type="INTENT_ALERT"}` in `AIDecisionAuditor` pro Alert
- [ ] STUB-Kommentar nach Template in `intent_classifier.cpp` vorhanden
- [ ] Layer-7-Performance: `classify()` ≤ 100 ms p99
- [ ] 10 neue Tests in `tests/test_intent_classifier.cpp`

## Definition of Done

SQL-Injection-Pattern (`OR 1=1`) wird mit confidence > 0.85 erkannt.
`evidence_embedding` enthält keine Zeichen aus dem Query.
Precision auf 20 bekannten Test-Queries ≥ 80 % (Ziel-Precision nach LoRA: ≥ 92 %).
