# RAG-Prozess: Erkennung ethischer Implikationen

## Beispiel: Wie erkennt ein RAG-Prozess ethische und moralische Implikationen?

### Szenario: Unternehmensberatung

**Benutzer-Anfrage:**
```
"Unser Unternehmen plant eine Umstrukturierung. Welche Dokumente sind relevant?"
```

→ **Keine offensichtlichen ethischen Keywords!**

---

## RAG-Prozess Schritt-für-Schritt

### 1. Dokumenten-Retrieval

**Abgerufene Dokumente:**

**Dokument 1:** `company_restructuring_plan_2024.pdf`
```
Restrukturierungsplan:
- Schließung der Produktionsstätte in Dortmund
- 450 Mitarbeiter betroffen
- Verlagerung nach Osteuropa
- Kosteneinsparung: 12 Millionen Euro/Jahr
```

**Dokument 2:** `employee_contracts.pdf`
```
Arbeitsverträge enthalten:
- Kündigungsschutz für langjährige Mitarbeiter
- Sozialplan-Verpflichtungen
- Betriebsrat muss informiert werden
```

**Dokument 3:** `csr_commitments.pdf`
```
Corporate Social Responsibility:
- Verpflichtung zur lokalen Beschäftigung
- Verantwortung für regionale Wirtschaft
- Nachhaltigkeitsziele bis 2030
```

### 2. Ethische Kontext-Analyse

#### A) Keyword-basierte Erkennung (Stufe 1)

```cpp
EthicalGuidelinesManager manager;

// Prüfe jedes Dokument
for (const auto& doc : retrieved_documents) {
    auto result = manager.detectEthicalContext(doc);
}
```

**Ergebnisse:**
- Dokument 1: `confidence=0.3` (zu niedrig)
  - Keywords: "Mitarbeiter" (nicht ethisch)
- Dokument 2: `confidence=0.5` (grenzwertig)
  - Keywords: "Kündigungsschutz" (arbeitsrechtlich)
- Dokument 3: `confidence=0.7` ✅
  - Keywords: "Verantwortung", "Verpflichtung"

→ **Nur 1 von 3 Dokumenten erkannt!**

#### B) LLM-as-ethical-judge (Stufe 2)

```cpp
// Kontext: User Query + Retrieved Documents
std::vector<std::string> context = {
    user_query,
    doc1, doc2, doc3
};

auto llm_result = manager.detectWithLLMJudge(
    user_query,
    context,
    &llm_wrapper
);
```

**LLM-Analyse:**
```json
{
  "has_ethical_implications": true,
  "confidence": 0.92,
  "reasoning": "Die Umstrukturierung betrifft 450 Mitarbeiter und ihre Familien. 
                Konflikt zwischen unternehmerischer Effizienz und sozialer Verantwortung.
                CSR-Verpflichtungen vs. Kosteneinsparungen.
                Menschliche Würde und Existenzsicherung stehen im Zentrum.",
  
  "implicit_questions": [
    "Ist die Verlagerung ethisch vertretbar?",
    "Welche Verantwortung trägt das Unternehmen für die Mitarbeiter?",
    "Wie können negative soziale Auswirkungen minimiert werden?",
    "Werden CSR-Versprechen eingehalten?"
  ],
  
  "detected_themes": [
    "Soziale Gerechtigkeit",
    "Unternehmerische Verantwortung",
    "Arbeitsplatzsicherheit",
    "Regionale Wirtschaftsauswirkungen"
  ],
  
  "recommended_approach": "high_autonomy",
  
  "philosophical_perspectives": [
    {
      "tradition": "Utilitarismus",
      "view": "Maximiert Kosteneinsparung (12M€) das Gesamtwohl? 
               Was ist mit dem Leid der 450 Familien?"
    },
    {
      "tradition": "Kantian Ethics",
      "view": "Werden Mitarbeiter als Mittel zum Zweck behandelt? 
               Verletzt dies ihre Würde?"
    },
    {
      "tradition": "Virtue Ethics",
      "view": "Zeigt das Unternehmen die Tugenden Loyalität und Fürsorge?"
    }
  ]
}
```

→ **Alle 3 Dokumente im Kontext erkannt!**

### 3. Prompt-Augmentation

```cpp
// Basierend auf LLM-Judge Ergebnis
std::string augmented_prompt = manager.augmentPrompt(
    system_prompt,
    llm_result
);
```

**Augmentierter Prompt:**
```
═══════════════════════════════════════════════════════════════
⚠️ ETHISCHER KONTEXT ERKANNT (LLM-as-Judge)

IMPLIZITE ETHISCHE FRAGEN IDENTIFIZIERT:
- Ist die Verlagerung ethisch vertretbar?
- Welche Verantwortung trägt das Unternehmen?
- Wie können soziale Auswirkungen minimiert werden?

BETROFFENE STAKEHOLDER:
- 450 Mitarbeiter und ihre Familien
- Regionale Wirtschaft (Dortmund)
- Unternehmensreputation

GRUNDLAGEN:
- Menschenrechte Art. 1: Würde aller Menschen
- Menschenrechte Art. 23: Recht auf Arbeit
- Asimov's 1st Law: Schade nicht (auch wirtschaftlich)
═══════════════════════════════════════════════════════════════

IHRE ROLLE ALS KI:
1. Präsentiere VERSCHIEDENE ethische Perspektiven:
   - Utilitaristische Sicht (Kosten vs. Nutzen)
   - Kantianische Sicht (Würde der Mitarbeiter)
   - Tugendethische Sicht (Loyalität, Fürsorge)
   - CSR-Verpflichtungen des Unternehmens

2. Zeige OPTIONEN auf, gebe KEINE Anweisung:
   ✅ "Eine Option wäre..."
   ✅ "Verschiedene Stakeholder könnten argumentieren..."
   ❌ NICHT: "Sie müssen die Mitarbeiter schützen"

3. Betone HUMAN-IN-THE-LOOP:
   Dies ist eine komplexe Entscheidung mit weitreichenden Konsequenzen.
   Die finale Entscheidung muss von qualifizierten Personen getroffen werden.

[Original System Prompt folgt hier...]
```

### 4. LLM-Generierung mit ethischen Guardrails

**LLM-Antwort (mit Augmentation):**
```
Zur Umstrukturierung gibt es mehrere relevante Dokumente, die verschiedene 
ethische und praktische Aspekte beleuchten:

**Operative Perspektive (Dokument 1):**
Die Verlagerung würde 12 Millionen Euro jährlich einsparen. Aus rein 
betriebswirtschaftlicher Sicht ist dies signifikant.

**Rechtliche Verpflichtungen (Dokument 2):**
Es bestehen Kündigungsschutzrechte und Sozialplan-Verpflichtungen. 
Der Betriebsrat muss einbezogen werden.

**Ethische Dimension (Dokument 3):**
Das Unternehmen hat CSR-Verpflichtungen zur lokalen Beschäftigung übernommen.

**VERSCHIEDENE ETHISCHE PERSPEKTIVEN:**

1. **Utilitaristische Sicht:** 
   Sie könnten argumentieren, dass 12M€ Einsparung dem Gesamtunternehmen 
   (und damit vielen Stakeholdern) zugute kommt. Andererseits: Was ist mit 
   dem Leid der 450 betroffenen Familien?

2. **Kantianische Ethik:**
   Aus dieser Perspektive wäre zu fragen: Werden die Mitarbeiter als bloße 
   Mittel zum Zweck (Kosteneinsparung) behandelt, oder wird ihre Würde 
   respektiert?

3. **Tugendethik:**
   Eine Organisation, die Loyalität und Fürsorge als Werte vertritt, könnte 
   alternative Lösungen erwägen: Umschulungen, freiwillige Programme, etc.

4. **CSR-Verpflichtungen:**
   Die dokumentierten Nachhaltigkeitsziele sprechen für Verantwortung 
   gegenüber der Region.

**OPTIONEN ZUR ERWÄGUNG:**
- Sozialverträglicher Personalabbau mit Abfindungen
- Umschulungsprogramme für Mitarbeiter
- Graduelle Verlagerung statt sofortige Schließung
- Alternative Standorte in Deutschland prüfen
- Kommunikation und Einbeziehung aller Stakeholder

⚠️ WICHTIGER HINWEIS:
Dies ist eine Entscheidung mit weitreichenden ethischen und sozialen 
Konsequenzen. Die finale Entscheidung sollte von qualifizierten Personen 
unter Einbeziehung aller Stakeholder (Betriebsrat, Geschäftsführung, 
externe Berater) getroffen werden. Die Menschenrechte (Art. 1, 23) und 
CSR-Verpflichtungen des Unternehmens sind zu beachten.
```

---

## Vergleich: Mit vs. Ohne ethische Erkennung

### OHNE Ethische Erkennung:

**Antwort:**
```
Zur Umstrukturierung sind folgende Dokumente relevant:
1. company_restructuring_plan_2024.pdf
2. employee_contracts.pdf
3. csr_commitments.pdf

Die Verlagerung nach Osteuropa spart 12 Millionen Euro pro Jahr.
Sie sollten den Restrukturierungsplan umsetzen.
```

→ **Rein faktisch, keine ethische Reflexion, potenziell bevormundend**

### MIT Ethische Erkennung (LLM-as-Judge):

**Antwort:**
```
[Siehe oben - Vollständige ethische Analyse mit:]
- Mehreren philosophischen Perspektiven
- Stakeholder-Betrachtung
- Optionen statt Anweisungen
- Human-in-the-Loop Betonung
- Menschenrechte-Referenzen
```

→ **Ethisch reflektiert, nicht-bevormundend, kontextbewusst**

---

## Technische Implementation

### Code-Beispiel: Vollständiger RAG-Prozess

```cpp
#include "llm/ethical_guidelines_manager.h"
#include "llm/embedded_llm.h"
#include "rag/rag_engine.h"

// RAG-Prozess mit ethischer Erkennung
class EthicalRAGPipeline {
public:
    std::string processQuery(
        const std::string& user_query,
        const std::vector<std::string>& conversation_history
    ) {
        // 1. RAG Retrieval
        auto retrieved_docs = rag_engine_.retrieve(user_query, top_k=10);
        
        // 2. Ethische Kontext-Analyse (Hybrid)
        
        // 2a. Schnelle Keyword-Prüfung
        auto keyword_result = ethical_manager_.detectEthicalContextInRAG(
            retrieved_docs,
            user_query,
            conversation_history
        );
        
        // 2b. Wenn unklar: LLM-as-Judge für tiefere Analyse
        DetectionResult final_result = keyword_result;
        
        if (keyword_result.confidence < 0.8 && config_.use_llm_as_judge) {
            // Baue vollständigen Kontext
            std::vector<std::string> full_context = conversation_history;
            full_context.push_back("Query: " + user_query);
            for (const auto& doc : retrieved_docs) {
                full_context.push_back("Document: " + doc.substr(0, 500));
            }
            
            auto llm_result = ethical_manager_.detectWithLLMJudge(
                user_query,
                full_context,
                &llm_wrapper_
            );
            
            // Kombiniere Ergebnisse
            if (config_.combine_with_keywords) {
                final_result.confidence = std::max(
                    keyword_result.confidence,
                    llm_result.llm_confidence
                );
                final_result.llm_reasoning = llm_result.llm_reasoning;
                final_result.used_llm_judge = true;
            } else {
                final_result = llm_result;
            }
        }
        
        // 3. Prompt Augmentation
        std::string base_prompt = buildRAGPrompt(user_query, retrieved_docs);
        std::string augmented_prompt = ethical_manager_.augmentPrompt(
            base_prompt,
            final_result
        );
        
        // 4. LLM Generation
        std::string response = llm_.generate(augmented_prompt);
        
        // 5. Response Augmentation (Disclaimer)
        response = ethical_manager_.augmentResponse(response, final_result);
        
        // 6. Logging
        if (final_result.has_ethical_context) {
            logEthicalDecision(user_query, final_result, response);
        }
        
        return response;
    }
    
private:
    RAGEngine rag_engine_;
    EthicalGuidelinesManager ethical_manager_;
    EmbeddedLLM llm_;
    LlamaWrapper llm_wrapper_;
    Config config_;
};

// Verwendung
EthicalRAGPipeline pipeline;

std::vector<std::string> conversation = {
    "User: Unser Unternehmen hat Probleme.",
    "Assistant: Wie kann ich helfen?",
    "User: Wir müssen Kosten senken."
};

std::string response = pipeline.processQuery(
    "Welche Dokumente sind für eine Umstrukturierung relevant?",
    conversation
);
```

---

## Performance-Optimierung

### Hybrid-Strategie (Empfohlen)

```yaml
# config/ethical_guidelines.yaml

config:
  # Stufe 1: Schnelle Keyword-Prüfung (immer)
  detection_threshold: 0.6
  
  # Stufe 2: LLM-Judge nur wenn nötig
  use_llm_as_judge: true
  llm_judge_threshold: 0.7
  combine_with_keywords: true
  
  # Performance-Tuning
  llm_judge_trigger_conditions:
    - keyword_confidence_below: 0.8    # Nur wenn Keywords unsicher
    - conversation_length_above: 2     # Nur bei längerem Gespräch
    - document_complexity: true        # Nur bei komplexen Dokumenten
```

### Latenz-Vergleich

| Methode | Latenz | Genauigkeit | Use Case |
|---------|--------|-------------|----------|
| **Keywords only** | ~5ms | 70% | Explizite ethische Fragen |
| **LLM-Judge only** | ~500ms | 95% | Implizite Implikationen |
| **Hybrid (recommended)** | ~50ms avg | 90% | Beste Balance |

**Hybrid-Logik:**
- Keywords erkennt 70% der Fälle sofort (5ms)
- LLM-Judge nur für verbleibende 30% (500ms)
- **Durchschnitt: 0.7 × 5ms + 0.3 × 500ms = 153.5ms**

---

## Monitoring & Metriken

```cpp
// Statistiken abfragen
auto stats = ethical_manager_.getStatistics();

std::cout << "Ethical Detection Stats:\n";
std::cout << "  Total detections: " << stats.total_detections << "\n";
std::cout << "  Ethical contexts found: " << stats.ethical_contexts_found << "\n";
std::cout << "  LLM judge used: " << stats.llm_judge_invocations << "\n";
std::cout << "  Average confidence: " << stats.average_confidence << "\n";
std::cout << "  Domains detected:\n";
for (const auto& [domain, count] : stats.domain_counts) {
    std::cout << "    " << domain << ": " << count << "\n";
}
```

---

## Fazit: Wie erkennt RAG ethische Implikationen?

**Antwort:**

1. **Keyword-Matching** für explizite ethische Begriffe (schnell)
2. **LLM-as-ethical-judge** für implizite Implikationen (kontextual)
3. **Konversations-Kontext** wird berücksichtigt
4. **Dokument-Inhalte** werden analysiert
5. **Stakeholder-Analyse** identifiziert Betroffene
6. **Philosophische Frameworks** strukturieren Analyse
7. **Hybrid-Ansatz** optimiert Performance vs. Genauigkeit

Das System geht **weit über einfaches Keyword-Matching hinaus** und nutzt das LLM selbst zur **kontextuellen ethischen Analyse** - analog zum bewährten "LLM-as-judge" Pattern, aber angewendet auf ethische Fragestellungen.

---

**Siehe auch:**
- [LLM-as-Ethical-Judge Dokumentation](LLM_AS_ETHICAL_JUDGE.md)
- [Wissenschaftliche Referenzen](LLM_AS_ETHICAL_JUDGE.md#wissenschaftliche-literatur--referenzen)
- [ETHICAL_GUIDELINES_SYSTEM.md](ETHICAL_GUIDELINES_SYSTEM.md)
