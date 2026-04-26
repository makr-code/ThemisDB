# RETE / Forward-Chaining Rule Engine für Expert-System-Integration

**Metadaten:**
- Source: Forgy (1982) — RETE Algorithm; Doorenbos (1995) — Rete++; CLIPS Expert System Shell (NASA); Drools Business Rules Engine (Red Hat); OWL 2 RL Forward-Chaining
- URL: [Forgy 1982](https://doi.org/10.1016/0004-3702(82)90024-9) · [CLIPS](https://clipsrules.net/) · [Drools](https://www.drools.org/) · [OWL 2 RL](https://www.w3.org/TR/owl2-profiles/#OWL_2_RL)
- Tags: `expert-system`, `rule-engine`, `rete-algorithm`, `forward-chaining`, `backward-chaining`, `horn-clauses`, `working-memory`, `agenda`, `production-rules`
- ThemisDB-Versionen: v2.1.0+ (planned — `ExpertSystemEngine`, `KnowledgeBase`)
- Status: [ ] Identified | [x] Partially Adopted | [ ] Fully Adopted  
  *(CEP-NFA als Partial-Adoption; vollständig Q2 2027 — `src/analytics/ROADMAP.md` Phase 4)*

## 📋 Summary

RETE-basierte Rule-Engines sind der etablierte Best-Practice für Expertensysteme, die viele Regeln gegen viele Objekte evaluieren müssen. Kernprinzip: **Zustandsspeicherung statt Neuauswertung** — Änderungen am Working Memory propagieren inkrementell durch ein vorberechnetes Netzwerk aus Bedingungsknoten. Das CEP-NFA-Pattern-Matching in ThemisDB (`cep_engine.cpp`) ist eine Event-Stream-Spezialisierung dieses Musters; `ExpertSystemEngine` erweitert es um persistente Fakten-Wissensbasis, Vorwärts-/Rückwärtsverkettung und Erklärungskomponente.

## 🎯 Core Principles

1. **Working Memory (WM) = Single Source of Truth**: Alle bekannten Fakten (`(subject, predicate, object)`) leben im WM; Regeln lesen nur aus WM; externe Datenbankzugriffe sind Antipattern.
2. **Inkrementelle Propagation**: `add-wme` (assert) und `remove-wme` (retract) propagieren *nur die Änderung*; keine Voll-Neuauswertung bei jedem Schritt.
3. **Trennung Alpha / Beta**: Alpha-Knoten prüfen einzelne WM-Elemente (einfach, O(1)); Beta-Knoten joinen partielle Matches (teuer, aber gecacht). Design-Regel: Alpha-Bedingungen so restriktiv wie möglich.
4. **Salience-gesteuerte Agenda**: Alle aktivierten Regelinstanzen kommen in die Agenda; Priorität (`salience`) steuert Feuerstrategie. Gleiche Salience → LEX oder Zeitstempel-Ordnung.
5. **Horn-Klausel-Beschränkung**: Nur Regeln der Form `IF (B₁ AND B₂ AND ... AND Bₙ) THEN H` (keine Disjunktionen im Kopf; keine Negation im Rumpf in v2.1.0).
6. **Vorwärtsverkettung bis Fixpunkt**: `forwardChain(max_cycles)` iteriert bis keine Regelinstanz mehr feuert oder `max_cycles` überschritten ist; kein manuelles Trigger-Management.
7. **Erklärbarkeit (Explainability)**: Jede abgeleitete Konsequenz speichert den Beweis-Trace (welche Regeln mit welchen Fakten haben H erzeugt); `explain(fact_id)` gibt strukturierten Proof-Tree zurück.
8. **Rückwärtsverkettung (Goal-Driven)**: `queryGoal(goal)` sucht rückwärts von Ziel zu Prämissen; Depth-Limited-Search (max 10); Zirkeldetektion via Visited-Set.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/analytics/` — `ExpertSystemEngine`: Vorwärts-/Rückwärtsverkettung; Agenda-Management; `explain()`
- `src/analytics/` — `KnowledgeBase`: Working Memory (`unordered_multimap<predicate, Fact>`); YAML-Regelladung
- `src/analytics/cep_engine.cpp` — Existierende NFA-Engine dient als Alpha-Netz-Äquivalent für Event-Stream-Bedingungen
- `src/graph/` — `KnowledgeGraphReasoner`: Horn-Klausel-Forward-Chaining für Tripel-Inferenz

### What Was Adopted?

**1. Working Memory + Faktenlayout**
```cpp
// KnowledgeBase — Working Memory
struct Fact {
    std::string subject;
    std::string predicate;
    std::string object;
    std::string source_rule_id;  // leer wenn direkt assertiert
    uint64_t timestamp_ms;
};

class KnowledgeBase {
    // Prädikat-Index für O(log N) Lookup
    std::unordered_multimap<std::string, Fact> facts_;
    std::mutex mu_;

public:
    void assertFact(Fact f);            // add-wme
    void retractFact(const std::string& fact_id);  // remove-wme
    std::vector<Fact> getFacts(const std::string& predicate) const;
};
```

**2. Horn-Klausel-Regelformat (YAML)**
```yaml
rules:
  # Transitives Vorgesetzten-Verhältnis — Forward-Chaining Beispiel
  - id: indirect_reports_to
    priority: 10
    description: "Transitives reports_to via Kettenregel"
    if:
      - subject: "?A"
        predicate: "reports_to"
        object: "?B"
      - subject: "?B"
        predicate: "reports_to"
        object: "?C"
    then:
      - subject: "?A"
        predicate: "indirectly_reports_to"
        object: "?C"
    ml_confidence_threshold: 0.0   # 0.0 = rein deterministisch

  # ML-augmentierte Regel mit LoRA-Konfidenz
  - id: expert_domain_soft
    priority: 5
    if:
      - subject: "?P"
        predicate: "authored"
        object: "?D"
      - subject: "?D"
        predicate: "hasKeyword"
        object: "?K"
    then:
      - subject: "?P"
        predicate: "expertIn"
        object: "?K"
    ml_confidence_threshold: 0.80   # LoRA-Adapter muss ≥ 0.80 bestätigen
    lora_adapter: "domain_expertise_v1"
```

**3. ExpertSystemEngine — Forward-Chaining-Loop**
```cpp
// ExpertSystemEngine::forwardChain() — Fixpunkt-Iteration
int ExpertSystemEngine::forwardChain(int max_cycles) {
    int rules_fired = 0;
    for (int cycle = 0; cycle < max_cycles; ++cycle) {
        // 1. Matchphase: Alle Regeln gegen WM evaluieren
        auto activations = matchAllRules(knowledge_base_);

        // 2. Konfliktmenge leer → Fixpunkt erreicht
        if (activations.empty()) break;

        // 3. Salience-Sortierung + Top-Aktivierung wählen
        std::sort(activations.begin(), activations.end(),
                  [](const auto& a, const auto& b) {
                      return a.rule.priority > b.rule.priority;
                  });
        const auto& selected = activations.front();

        // 4. ML-Confidence-Check (optional)
        if (selected.rule.ml_confidence_threshold > 0.0f) {
            float score = ml_scorer_->predict(selected.bindings);
            if (score < selected.rule.ml_confidence_threshold) continue;
        }

        // 5. Konsequenz assertieren + Proof-Trace speichern
        Fact derived = applyBindings(selected.rule.then, selected.bindings);
        derived.source_rule_id = selected.rule.id;
        knowledge_base_.assertFact(derived);
        proof_store_.record(derived, selected);
        ++rules_fired;
    }
    return rules_fired;
}
```

**4. Rückwärtsverkettung — Depth-Limited-Search**
```cpp
// ExpertSystemEngine::queryGoal() — Backward-Chaining
GoalResult ExpertSystemEngine::queryGoal(
    const Fact& goal, int depth, std::unordered_set<std::string>& visited) {

    // Zirkeldetektion
    auto key = goal.subject + ":" + goal.predicate + ":" + goal.object;
    if (visited.count(key)) return GoalResult::Cycle;
    if (depth > max_backward_depth_) return GoalResult::DepthExceeded;
    visited.insert(key);

    // Direkt im WM? → sofort erfüllt
    if (knowledge_base_.hasFact(goal)) return GoalResult::Satisfied;

    // Regeln suchen, deren Konsequenz `goal` unifiziert
    for (const auto& rule : rules_) {
        auto bindings = unify(rule.then, goal);
        if (!bindings) continue;

        // Alle Prämissen rekursiv beweisen
        bool all_satisfied = true;
        std::vector<Fact> sub_proofs;
        for (const auto& premise : rule.if_clauses) {
            auto concrete = applyBindings(premise, *bindings);
            auto sub = queryGoal(concrete, depth + 1, visited);
            if (!sub.satisfied) { all_satisfied = false; break; }
            sub_proofs.push_back(concrete);
        }
        if (all_satisfied) return GoalResult::Satisfied(sub_proofs, rule.id);
    }
    return GoalResult::Unsatisfied;
}
```

**5. CEP-Integration: Event-Strom als WM-Update-Quelle**
```cpp
// CEPEngine-Callback → ExpertSystemEngine::assertFact()
cep_engine.setOnMatchCallback([&](const CEPMatch& match) {
    // CEP-Match erzeugt Fakten im ExpertSystem-WM
    Fact f;
    f.subject = match.getField("entity_id");
    f.predicate = "triggered_event";
    f.object = match.rule_id;
    f.timestamp_ms = match.timestamp_ms;
    expert_system_.assertFact(f);
    expert_system_.forwardChain(10);  // inkrementell
});
```

### Deviations & Rationale

| RETE-Standard | ThemisDB Abweichung | Begründung |
|---|---|---|
| Vollständiges Alpha/Beta-Netz | Horn-Klausel-Join-Sequenz | Kein volles Rete-Netz für ≤100 Regeln nötig; einfacher zu debuggen |
| OPS5-Produktionsformat | YAML Horn-Klauseln | YAML zugänglicher; Schema-validierbar; Domain-Experten-friendly |
| Rete++ Memory-Sharing | Predicate-Index `unordered_multimap` | Ausreichend für ≤10k Fakten; geringere Komplexität |
| NAF (Negation-as-Failure) | v2.2.0 geplant | Vereinfacht initiale Implementierung erheblich |
| Trigger-basiertes Feuern | `forwardChain(max_cycles)` Batch | Deterministischer Ablauf; einfacher zu testen |

## ⚠️ Trade-offs & Limitations

- **Kein NAF in v2.1.0**: Regeln mit Negations-Prämissen (`IF NOT exists(X, ...)`).
  - Mitigation: Absenz-Check via `KnowledgeBase::hasFact()` in v2.2.0.
- **Beta-Memory-Größe**: Ohne vollständes Rete-Netz kein strukturelles Memory-Sharing für ähnliche Regelpräfixe.
  - Mitigation: Prädikat-Index statt naive Vollsuche; max 100 Regeln + 10k Fakten.
- **Nicht-monotone Rücknahme**: `retractFact()` invalidiert abgeleitete Fakten nicht automatisch (Truth Maintenance).
  - Mitigation: v2.2.0 geplant; interim: manuelle `retractFact()` für abgeleitete Tripel.
- **ML-Scorer-Latenz**: ML-Konfidenz-Check kann Forward-Chaining verlangsamen wenn Scorer langsam.
  - Mitigation: Scorer-Aufruf async; Timeout 50 ms; bei Timeout = deterministisch weiter.

## 🔬 Validation

- [ ] Forward-Chaining Fixpunkt-Korrektheit: Transitives Beispiel (3 Klauseln)
- [ ] Rückwärtsverkettung: 3-Hop-Ziel beweisen; Zirkeldetektion verifizieren
- [ ] CEP-Integration: CEP-Match → WM-Fakt → Forward-Chaining-Triggerung end-to-end
- [ ] Concurrency: 8 Threads assertieren Fakten gleichzeitig; keine Race Conditions
- [ ] Performance: 10k Fakten + 100 Regeln, Forward-Chaining ≤ 50 ms
- [ ] Module README verlinkt (`src/analytics/README.md`)
- [ ] implementation_influence index aktualisiert

## 📚 Related

- [Paper: RETE Algorithm — Forgy (1982)](../papers/forgy_rete_algorithm_1982.md)
- [Paper: OWL 2 / Description Logic Handbook](../papers/owl2_description_logics_2012.md)
- [Best Practice: OWL-lite Ontology Constraints](owl_lite_ontology_constraints.md)
- [Best Practice: Multi-LoRA Adapter Routing](multi_lora_adapter_routing.md)
- [`src/analytics/FUTURE_ENHANCEMENTS.md`](../../../src/analytics/FUTURE_ENHANCEMENTS.md) — Expert System Engine
- [`src/analytics/ROADMAP.md`](../../../src/analytics/ROADMAP.md) — Phase 4

---
**Last Updated:** 2026-04-22
