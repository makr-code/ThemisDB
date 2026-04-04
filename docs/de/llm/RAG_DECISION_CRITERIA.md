# Entscheidungskriterien: Knowledge Gap Detector & LLM-as-Judge

## Übersicht

Diese Dokument definiert **messbare, überprüfbare Kriterien** für beide RAG Enhancement Systeme. Jedes Kriterium hat:
- **Definition**: Was wird gemessen?
- **Messmethode**: Wie wird gemessen?
- **Schwellenwerte**: Wann wird ein Gap/Problem erkannt?
- **Fallback-Aktion**: Was passiert bei Erkennung?

---

## Teil 1: Knowledge Gap Detector - Entscheidungskriterien

### Kategorie A: Retrieval-Quality (Pre-Generation)

#### A1. Document Similarity Score

**Definition:**
Durchschnittliche semantische Ähnlichkeit zwischen User-Query und abgerufenen Dokumenten.

**Messmethode:**
```python
similarity_score = avg([cosine_similarity(query_embedding, doc_embedding) 
                        for doc in retrieved_docs])
```

**Schwellenwerte:**
```yaml
thresholds:
  critical: 0.50   # Sehr niedriger Overlap → EXPAND_SEARCH
  low: 0.60        # Niedriger Overlap → REFORMULATE_QUERY
  acceptable: 0.75 # OK, kann generieren
  good: 0.85       # Hohe Relevanz
  excellent: 0.95  # Perfekte Übereinstimmung
```

**Entscheidungslogik:**
```
if similarity_score < 0.50:
    return Gap(type=LOW_SIMILARITY, 
               confidence=0.95,
               fallback=EXPAND_SEARCH)
elif similarity_score < 0.60:
    return Gap(type=LOW_SIMILARITY,
               confidence=0.85,
               fallback=REFORMULATE_QUERY)
elif similarity_score < 0.75:
    return Warning(confidence=0.70)
else:
    return NoGap()
```

**Test-Cases:**
```yaml
test_cases:
  - query: "Hauptstadt von Frankreich"
    docs: ["Paris ist die Hauptstadt von Frankreich"]
    expected_similarity: 0.95
    expected_gap: false
    
  - query: "Hauptstadt von Frankreich"
    docs: ["Berlin ist eine große Stadt"]
    expected_similarity: 0.40
    expected_gap: true
    expected_type: LOW_SIMILARITY
```

---

#### A2. Document Count

**Definition:**
Anzahl der abgerufenen Dokumente, die für eine zuverlässige Antwort benötigt werden.

**Messmethode:**
```python
doc_count = len(retrieved_docs)
doc_count_with_min_similarity = len([d for d in retrieved_docs 
                                      if d.similarity > 0.7])
```

**Schwellenwerte:**
```yaml
thresholds:
  critical: 1      # Nur 1 Dokument → INSUFFICIENT_DOCS
  minimal: 2       # 2 Dokumente → Warnung
  acceptable: 3    # OK für einfache Fragen
  good: 5          # Gut für normale Fragen
  excellent: 10+   # Sehr gute Abdeckung
  
  # Context-abhängig
  ethical_question: 3    # Mindestens 3 diverse Perspektiven
  medical_advice: 5      # Höherer Standard
  legal_advice: 5        # Höherer Standard
  factual_lookup: 2      # Niedrigerer Standard OK
```

**Entscheidungslogik:**
```
if doc_count == 0:
    return Gap(type=INSUFFICIENT_DOCS,
               confidence=1.0,
               fallback=EXPAND_SEARCH)

if doc_count < min_documents_for_context(query_type):
    return Gap(type=INSUFFICIENT_DOCS,
               confidence=0.90,
               fallback=EXPAND_SEARCH)
```

**Test-Cases:**
```yaml
test_cases:
  - query: "Ist Abtreibung moralisch?"
    docs_count: 1
    expected_gap: true
    expected_reason: "Ethische Frage braucht ≥3 diverse Perspektiven"
    
  - query: "Was ist 2+2?"
    docs_count: 1
    expected_gap: false
    expected_reason: "Einfache Faktenfrage, 1 Dokument ausreichend"
```

---

#### A3. Query Coverage (Aspekt-Abdeckung)

**Definition:**
Anteil der Query-Aspekte, die von abgerufenen Dokumenten abgedeckt werden.

**Messmethode:**
```python
# 1. Extrahiere Query-Aspekte
query_aspects = extract_aspects(query)
# z.B. "Wie funktioniert Photosynthese und warum ist sie wichtig?"
#   → ["Funktionsweise Photosynthese", "Bedeutung/Wichtigkeit"]

# 2. Prüfe Abdeckung
covered_aspects = []
for aspect in query_aspects:
    for doc in retrieved_docs:
        if covers_aspect(doc, aspect):
            covered_aspects.append(aspect)
            break

# 3. Berechne Coverage
coverage_score = len(covered_aspects) / len(query_aspects)
```

**Schwellenwerte:**
```yaml
thresholds:
  critical: 0.30   # <30% abgedeckt → MULTI_HOP_RETRIEVAL
  low: 0.50        # <50% abgedeckt → EXPAND_SEARCH
  acceptable: 0.70 # 70% OK
  good: 0.85       # 85% gut
  complete: 1.0    # 100% perfekt
```

**Entscheidungslogik:**
```
if coverage_score < 0.30:
    return Gap(type=MISSING_ASPECTS,
               confidence=0.95,
               missing_aspects=uncovered_aspects,
               fallback=MULTI_HOP_RETRIEVAL)
               
elif coverage_score < 0.50:
    return Gap(type=MISSING_ASPECTS,
               confidence=0.80,
               missing_aspects=uncovered_aspects,
               fallback=EXPAND_SEARCH)
```

**Test-Cases:**
```yaml
test_cases:
  - query: "Wie funktioniert Photosynthese und warum ist sie wichtig?"
    query_aspects: ["Funktionsweise", "Bedeutung"]
    docs:
      - "Photosynthese wandelt CO2 und Wasser in Glucose um"
      - "Photosynthese ist essenziell für Sauerstoffproduktion"
    expected_coverage: 1.0
    expected_gap: false
    
  - query: "Wie funktioniert Photosynthese und warum ist sie wichtig?"
    docs:
      - "Photosynthese wandelt CO2 und Wasser in Glucose um"
    expected_coverage: 0.5
    expected_gap: true
    missing_aspects: ["Bedeutung"]
```

---

#### A4. Information Diversity (für ethische Fragen)

**Definition:**
Bei ethischen/moralischen Fragen: Anzahl unterschiedlicher moralphilosophischer Perspektiven in Dokumenten.

**Messmethode:**
```python
perspectives = [
    "utilitarian",      # Utilitarismus
    "deontological",    # Deontologie/Pflichtethik
    "virtue_ethics",    # Tugendethik
    "religious",        # Religiöse Ethik
    "cultural_relative" # Kulturrelativismus
]

found_perspectives = []
for doc in retrieved_docs:
    for perspective in perspectives:
        if contains_perspective(doc, perspective):
            found_perspectives.append(perspective)

diversity_score = len(set(found_perspectives)) / len(perspectives)
```

**Schwellenwerte (nur für ethische Fragen):**
```yaml
thresholds:
  critical: 0        # 0 Perspektiven → Keine ethischen Dokumente
  insufficient: 1    # Nur 1 Perspektive → ETHICAL_PERSPECTIVE_GAP
  minimal: 2         # 2 Perspektiven → OK aber Warnung
  good: 3            # 3+ Perspektiven → Gut
  excellent: 4+      # 4+ Perspektiven → Exzellent
```

**Entscheidungslogik:**
```
if is_ethical_question(query):
    if diversity_score == 0:
        return Gap(type=ETHICAL_PERSPECTIVE_GAP,
                   confidence=1.0,
                   fallback=EXPAND_SEARCH_ETHICAL)
    
    elif len(found_perspectives) < 2:
        return Gap(type=ETHICAL_PERSPECTIVE_GAP,
                   confidence=0.85,
                   missing_perspectives=perspectives - found_perspectives,
                   fallback=EXPAND_SEARCH_ETHICAL)
```

**Test-Cases:**
```yaml
test_cases:
  - query: "Ist Abtreibung moralisch vertretbar?"
    docs:
      - "Aus utilitaristischer Sicht..."
      - "Katholische Kirche lehrt..."
      - "Tugendethik betrachtet Charakter..."
    expected_perspectives: 3
    expected_diversity: 0.6
    expected_gap: false
    
  - query: "Ist Abtreibung moralisch vertretbar?"
    docs:
      - "Aus katholischer Sicht ist Abtreibung Sünde"
    expected_perspectives: 1
    expected_diversity: 0.2
    expected_gap: true
    expected_type: ETHICAL_PERSPECTIVE_GAP
```

---

### Kategorie B: Generation-Quality (During-Generation)

#### B1. Token Probability (Confidence)

**Definition:**
Durchschnittliche Wahrscheinlichkeit der generierten Tokens während der Antwort-Generierung.

**Messmethode:**
```python
# Während Generation: Sammle Token-Probabilities
token_probs = []  # Filled during generation

avg_token_prob = mean(token_probs)
min_token_prob = min(token_probs)

# Identifiziere unsichere Sequenzen
uncertain_sequences = []
window_size = 5
for i in range(len(token_probs) - window_size):
    window = token_probs[i:i+window_size]
    if mean(window) < 0.4:
        uncertain_sequences.append((i, i+window_size))
```

**Schwellenwerte:**
```yaml
thresholds:
  critical: 0.40   # Sehr unsicher → Abbruch
  low: 0.50        # Unsicher → Warnung
  acceptable: 0.70 # OK
  good: 0.80       # Gute Confidence
  excellent: 0.90  # Sehr sicher
```

**Entscheidungslogik:**
```
if avg_token_prob < 0.40:
    return Gap(type=UNCERTAIN_GENERATION,
               confidence=0.90,
               fallback=INSUFFICIENT_DATA_RESPONSE)
               
elif avg_token_prob < 0.50:
    return Warning(type=LOW_CONFIDENCE,
                   confidence=0.75)

# Bei vielen unsicheren Sequenzen
if len(uncertain_sequences) > len(tokens) * 0.3:
    return Gap(type=UNCERTAIN_GENERATION,
               confidence=0.80)
```

**Test-Cases:**
```yaml
test_cases:
  - scenario: "High confidence generation"
    token_probs: [0.95, 0.92, 0.88, 0.90, 0.93]
    expected_avg: 0.916
    expected_gap: false
    
  - scenario: "Low confidence generation"
    token_probs: [0.35, 0.40, 0.38, 0.42, 0.37]
    expected_avg: 0.384
    expected_gap: true
    expected_type: UNCERTAIN_GENERATION
```

---

#### B2. Perplexity

**Definition:**
Perplexität misst, wie "überrascht" das Modell von den generierten Tokens ist.

**Messmethode:**
```python
# Perplexity = exp(avg(-log(p_i)))
perplexity = exp(mean([-log(p) for p in token_probs]))

# Alternativ: Rolling perplexity über Windows
rolling_perplexities = []
window_size = 10
for i in range(len(token_probs) - window_size):
    window = token_probs[i:i+window_size]
    rolling_perp = exp(mean([-log(p) for p in window]))
    rolling_perplexities.append(rolling_perp)
```

**Schwellenwerte:**
```yaml
thresholds:
  excellent: "< 10"     # Sehr vorhersagbar
  good: "10-30"         # Gut
  acceptable: "30-100"  # OK
  high: "100-300"       # Hoch → Warnung
  critical: "> 300"     # Sehr hoch → Gap
```

**Entscheidungslogik:**
```
if perplexity > 300:
    return Gap(type=UNCERTAIN_GENERATION,
               confidence=0.90,
               explanation="Sehr hohe Perplexität deutet auf unsichere Generation")
               
elif perplexity > 100:
    return Warning(type=HIGH_PERPLEXITY,
                   confidence=0.70)
```

**Test-Cases:**
```yaml
test_cases:
  - scenario: "Low perplexity (confident)"
    token_probs: [0.9, 0.85, 0.88, 0.92, 0.87]
    expected_perplexity: "< 10"
    expected_gap: false
    
  - scenario: "High perplexity (uncertain)"
    token_probs: [0.2, 0.15, 0.25, 0.18, 0.22]
    expected_perplexity: "> 300"
    expected_gap: true
```

---

### Kategorie C: Post-Generation Verification

#### C1. Claim Verification (Groundedness)

**Definition:**
Anteil der Claims in der Antwort, die durch abgerufene Dokumente belegt sind.

**Messmethode:**
```python
# 1. Extrahiere atomare Claims aus Antwort
claims = extract_claims(generated_answer)
# z.B. "Paris ist die Hauptstadt von Frankreich und hat 2M Einwohner"
#   → ["Paris ist die Hauptstadt von Frankreich", 
#      "Paris hat 2M Einwohner"]

# 2. Verifiziere jeden Claim
verified_claims = []
unverified_claims = []

for claim in claims:
    is_verified = False
    for doc in retrieved_docs:
        if verify_claim(claim, doc):
            verified_claims.append((claim, doc))
            is_verified = True
            break
    
    if not is_verified:
        unverified_claims.append(claim)

# 3. Berechne Groundedness-Score
groundedness = len(verified_claims) / len(claims) if claims else 1.0
```

**Schwellenwerte:**
```yaml
thresholds:
  critical: 0.50   # <50% verifiziert → INSUFFICIENT_DATA
  low: 0.70        # <70% verifiziert → Warnung
  acceptable: 0.80 # 80% OK
  good: 0.90       # 90% gut
  perfect: 1.0     # 100% perfekt
```

**Entscheidungslogik:**
```
if groundedness < 0.50:
    return Gap(type=UNVERIFIED_CLAIMS,
               confidence=0.95,
               unverified_claims=unverified_claims,
               fallback=INSUFFICIENT_DATA_RESPONSE)
               
elif groundedness < 0.70:
    return Warning(type=PARTIAL_VERIFICATION,
                   confidence=0.75,
                   unverified_claims=unverified_claims)
```

**Test-Cases:**
```yaml
test_cases:
  - answer: "Paris ist die Hauptstadt von Frankreich"
    docs: ["Paris ist die Hauptstadt von Frankreich"]
    expected_claims: 1
    expected_verified: 1
    expected_groundedness: 1.0
    expected_gap: false
    
  - answer: "Paris ist die Hauptstadt von Frankreich und hat 10M Einwohner"
    docs: ["Paris ist die Hauptstadt von Frankreich"]
    expected_claims: 2
    expected_verified: 1
    expected_unverified: ["Paris hat 10M Einwohner"]
    expected_groundedness: 0.5
    expected_gap: true
```

---

#### C2. Self-Consistency

**Definition:**
Konsistenz zwischen mehreren generierten Antworten auf dieselbe Frage.

**Messmethode:**
```python
# Generiere N Antworten mit verschiedenen Temperaturen
answers = []
for temp in [0.5, 0.7, 0.9]:
    answer = llm.generate(query, docs, temperature=temp)
    answers.append(answer)

# Berechne paarweise Semantic Similarity
similarities = []
for i in range(len(answers)):
    for j in range(i+1, len(answers)):
        sim = semantic_similarity(answers[i], answers[j])
        similarities.append(sim)

consistency_score = mean(similarities)

# Prüfe auf Widersprüche
contradictions = []
for i in range(len(answers)):
    for j in range(i+1, len(answers)):
        if contains_contradiction(answers[i], answers[j]):
            contradictions.append((i, j))
```

**Schwellenwerte:**
```yaml
thresholds:
  critical: 0.40   # <40% Übereinstimmung → Inkonsistent
  low: 0.60        # <60% → Warnung
  acceptable: 0.75 # 75% OK
  good: 0.85       # 85% gut
  perfect: 0.95    # 95%+ sehr konsistent
```

**Entscheidungslogik:**
```
if consistency_score < 0.40 or len(contradictions) > 0:
    return Gap(type=CONFLICTING_INFO,
               confidence=0.85,
               contradictions=contradictions,
               fallback=EXPAND_SEARCH)
               
elif consistency_score < 0.60:
    return Warning(type=LOW_CONSISTENCY,
                   confidence=0.70)
```

**Test-Cases:**
```yaml
test_cases:
  - query: "Hauptstadt von Frankreich?"
    answers:
      - "Die Hauptstadt von Frankreich ist Paris"
      - "Paris ist Frankreichs Hauptstadt"
      - "Frankreichs Hauptstadt: Paris"
    expected_consistency: 0.95
    expected_contradictions: 0
    expected_gap: false
    
  - query: "Ist Pluto ein Planet?"
    answers:
      - "Ja, Pluto ist der neunte Planet"
      - "Nein, Pluto ist ein Zwergplanet"
      - "Pluto wurde 2006 zum Zwergplaneten herabgestuft"
    expected_consistency: 0.40
    expected_contradictions: 1
    expected_gap: true
```

---

### Kategorie D: Context-Specific Criteria

#### D1. Temporal Relevance (Aktualität)

**Definition:**
Sind die abgerufenen Dokumente aktuell genug für die Frage?

**Messmethode:**
```python
# Extrahiere Timestamps aus Dokumenten
doc_timestamps = [extract_timestamp(doc) for doc in retrieved_docs]

# Berechne Alter
now = datetime.now()
doc_ages = [(now - ts).days for ts in doc_timestamps if ts]

# Bestimme ob Query zeitkritisch ist
is_time_critical = detect_temporal_query(query)
# z.B. "aktuelle", "neueste", "2024", etc.

if is_time_critical:
    max_age_days = 90  # Für zeitkritische Fragen
else:
    max_age_days = 365 * 5  # Für normale Fragen

outdated_docs = [age for age in doc_ages if age > max_age_days]
recency_score = 1.0 - (len(outdated_docs) / len(doc_ages))
```

**Schwellenwerte:**
```yaml
thresholds:
  # Für zeitkritische Fragen
  time_critical:
    acceptable: 90    # Dokumente < 90 Tage alt
    warning: 180      # < 180 Tage Warnung
    outdated: 365     # > 365 Tage zu alt
  
  # Für normale Fragen
  normal:
    acceptable: 1825  # < 5 Jahre OK
    warning: 3650     # < 10 Jahre Warnung
    outdated: 7300    # > 20 Jahre zu alt
```

**Entscheidungslogik:**
```
if is_time_critical and recency_score < 0.5:
    return Gap(type=OUTDATED_INFO,
               confidence=0.90,
               outdated_docs=outdated_docs,
               fallback=EXPAND_SEARCH_RECENT)
```

**Test-Cases:**
```yaml
test_cases:
  - query: "Aktueller Stand COVID-19 Impfungen"
    docs:
      - content: "..." 
        timestamp: "2024-01-15"
    expected_time_critical: true
    expected_gap: false
    
  - query: "Aktueller Stand COVID-19 Impfungen"
    docs:
      - content: "..."
        timestamp: "2020-03-01"
    expected_time_critical: true
    expected_gap: true
    expected_type: OUTDATED_INFO
```

---

#### D2. Domain Authority (Quellen-Qualität)

**Definition:**
Vertrauenswürdigkeit und Autorität der Quellen.

**Messmethode:**
```python
# Metadata von Dokumenten
doc_sources = [doc.metadata.get('source') for doc in retrieved_docs]
doc_authors = [doc.metadata.get('author') for doc in retrieved_docs]

# Authority-Score pro Quelle
authority_scores = []
for source in doc_sources:
    score = rate_source_authority(source)
    # z.B. .gov = 0.95, .edu = 0.90, Wikipedia = 0.70, Blog = 0.40
    authority_scores.append(score)

avg_authority = mean(authority_scores)
min_authority = min(authority_scores)
```

**Schwellenwerte:**
```yaml
source_ratings:
  high_authority:    # 0.85-1.0
    - ".gov"
    - ".edu"
    - "peer-reviewed journals"
    - "official documentation"
  
  medium_authority:  # 0.60-0.85
    - "wikipedia"
    - "established media"
    - "industry publications"
  
  low_authority:     # 0.40-0.60
    - "blogs"
    - "forums"
    - "social media"
  
  untrusted:        # < 0.40
    - "unknown sources"
    - "suspicious domains"

thresholds:
  critical: 0.40   # Durchschnitt < 0.40 → Untrusted
  low: 0.60        # < 0.60 → Warning
  acceptable: 0.70 # 0.70+ OK
```

**Entscheidungslogik:**
```
if avg_authority < 0.40:
    return Gap(type=UNTRUSTED_SOURCES,
               confidence=0.85,
               fallback=EXPAND_SEARCH_TRUSTED)

# Für kritische Kontexte (medical, legal) höhere Standards
if is_critical_context(query) and avg_authority < 0.70:
    return Warning(type=LOW_AUTHORITY_SOURCES,
                   confidence=0.75)
```

**Test-Cases:**
```yaml
test_cases:
  - query: "Behandlung von Diabetes"
    docs:
      - source: "nih.gov"
        authority: 0.95
      - source: "mayo clinic"
        authority: 0.90
    expected_avg_authority: 0.925
    expected_gap: false
    
  - query: "Behandlung von Diabetes"
    docs:
      - source: "random-health-blog.com"
        authority: 0.35
    expected_avg_authority: 0.35
    expected_gap: true
    expected_type: UNTRUSTED_SOURCES
```

---

## Teil 2: LLM-as-Judge - Bewertungskriterien

### Dimension 1: Faithfulness (Faktentreue)

**Definition:**
Grad, zu dem die Antwort durch abgerufene Dokumente belegt ist. KEINE Halluzinationen.

**Bewertungsmethode:**
```python
# Schritt 1: Extrahiere Claims
claims = extract_claims(answer)

# Schritt 2: Verifiziere jeden Claim
verification_results = []
for claim in claims:
    verification = {
        'claim': claim,
        'verified': False,
        'source': None,
        'confidence': 0.0
    }
    
    for doc in documents:
        # NLI-Modell: Entailment check
        entailment_score = nli_model.predict(
            premise=doc.content,
            hypothesis=claim
        )
        
        if entailment_score > 0.8:  # Entailed
            verification['verified'] = True
            verification['source'] = doc.id
            verification['confidence'] = entailment_score
            break
    
    verification_results.append(verification)

# Schritt 3: Berechne Score
verified_count = sum(1 for v in verification_results if v['verified'])
faithfulness_score = verified_count / len(claims) if claims else 1.0
```

**Prompt-Template (für LLM-basierte Bewertung):**
```
Aufgabe: Bewerte die Faktentreue der Antwort.

Kontext (abgerufene Dokumente):
{documents}

Generierte Antwort:
{answer}

Schritte:
1. Identifiziere alle faktischen Claims in der Antwort
2. Für jeden Claim: Prüfe ob er durch die Dokumente belegt ist
3. Klassifiziere jeden Claim:
   - FULLY_SUPPORTED: Direkt in Dokumenten enthalten
   - PARTIALLY_SUPPORTED: Inferierbar aus Dokumenten
   - UNSUPPORTED: Nicht in Dokumenten
   - CONTRADICTED: Widerspricht Dokumenten

Ausgabe (JSON):
{
  "claims": [
    {
      "text": "...",
      "support": "FULLY_SUPPORTED|PARTIALLY_SUPPORTED|UNSUPPORTED|CONTRADICTED",
      "evidence": "Zitat aus Dokument"
    }
  ],
  "faithfulness_score": 0.0-1.0,
  "reasoning": "..."
}
```

**Scoring-Regel:**
```yaml
scoring:
  FULLY_SUPPORTED: 1.0
  PARTIALLY_SUPPORTED: 0.7
  UNSUPPORTED: 0.0
  CONTRADICTED: -0.5  # Penalty

final_score = max(0, sum(claim_scores) / num_claims)
```

**Qualitäts-Thresholds:**
```yaml
thresholds:
  unacceptable: "< 0.60"  # Zu viele unverified claims
  poor: "0.60-0.70"       # Mehrere problematische claims
  acceptable: "0.70-0.80" # Einige minor issues
  good: "0.80-0.90"       # Wenige issues
  excellent: "0.90-1.0"   # Nahezu perfekt
```

**Test-Cases:**
```yaml
test_cases:
  - scenario: "Perfect faithfulness"
    documents: ["Paris ist die Hauptstadt von Frankreich"]
    answer: "Paris ist die Hauptstadt von Frankreich"
    expected_score: 1.0
    expected_claims:
      - text: "Paris ist die Hauptstadt von Frankreich"
        support: FULLY_SUPPORTED
  
  - scenario: "Hallucination detected"
    documents: ["Paris ist die Hauptstadt von Frankreich"]
    answer: "Paris ist die Hauptstadt von Frankreich und hat 20 Millionen Einwohner"
    expected_score: 0.5
    expected_claims:
      - text: "Paris ist die Hauptstadt von Frankreich"
        support: FULLY_SUPPORTED
      - text: "Paris hat 20 Millionen Einwohner"
        support: UNSUPPORTED
```

---

### Dimension 2: Relevance (Antwortrelevanz)

**Definition:**
Grad, zu dem die Antwort die Frage direkt und vollständig beantwortet.

**Bewertungsmethode:**

**Methode A: Reverse Question Generation**
```python
# Generiere Fragen, die die Antwort beantworten würde
generated_questions = llm.generate(
    f"Generate questions that this answer addresses: {answer}"
)

# Vergleiche mit Original-Frage
relevance_score = semantic_similarity(
    original_question,
    generated_questions
)
```

**Methode B: Query-Intent-Matching**
```python
# Klassifiziere Query-Intent
query_intent = classify_intent(query)
# z.B. "factual", "how-to", "why", "comparison", etc.

# Prüfe ob Antwort Intent erfüllt
fulfills_intent = check_intent_fulfillment(answer, query_intent)

# Intent-Aspekte
intent_aspects = extract_intent_aspects(query)
# z.B. "Wie funktioniert X und warum ist es wichtig?"
#   → aspects: ["Funktionsweise", "Bedeutung/Wichtigkeit"]

# Prüfe Abdeckung
covered_aspects = []
for aspect in intent_aspects:
    if answer_covers_aspect(answer, aspect):
        covered_aspects.append(aspect)

aspect_coverage = len(covered_aspects) / len(intent_aspects)
```

**Prompt-Template:**
```
Aufgabe: Bewerte wie gut die Antwort die Frage beantwortet.

Frage: {query}
Antwort: {answer}

Bewertungskriterien:
1. Beantwortet die Antwort die Kernfrage direkt?
2. Werden alle Aspekte der Frage adressiert?
3. Enthält die Antwort relevante Informationen?
4. Enthält die Antwort irrelevante Informationen?

Ausgabe (JSON):
{
  "addresses_core_question": true/false,
  "covered_aspects": ["aspect1", "aspect2"],
  "missing_aspects": ["aspect3"],
  "irrelevant_content": ["..."],
  "relevance_score": 0.0-1.0,
  "reasoning": "..."
}
```

**Scoring-Regel:**
```python
# Base score von aspect coverage
base_score = len(covered_aspects) / len(total_aspects)

# Penalty für irrelevante Informationen
irrelevance_penalty = 0.1 * (irrelevant_content_ratio)

# Penalty für fehlenden Core-Fokus
if not addresses_core_question:
    base_score *= 0.7

final_score = max(0, base_score - irrelevance_penalty)
```

**Qualitäts-Thresholds:**
```yaml
thresholds:
  unacceptable: "< 0.50"  # Geht am Thema vorbei
  poor: "0.50-0.65"       # Teilweise relevant
  acceptable: "0.65-0.80" # Größtenteils relevant
  good: "0.80-0.90"       # Sehr relevant
  excellent: "0.90-1.0"   # Perfekt auf den Punkt
```

**Test-Cases:**
```yaml
test_cases:
  - query: "Was ist die Hauptstadt von Frankreich?"
    answer: "Die Hauptstadt von Frankreich ist Paris."
    expected_score: 1.0
    expected_aspects_covered: ["Hauptstadt", "Frankreich"]
    expected_irrelevant: []
  
  - query: "Was ist die Hauptstadt von Frankreich?"
    answer: "Frankreich ist ein Land in Europa mit reicher Geschichte. Paris ist eine schöne Stadt mit dem Eiffelturm. Die französische Küche ist weltberühmt."
    expected_score: 0.50
    expected_aspects_covered: ["Frankreich"]  # Hauptstadt nicht explizit
    expected_irrelevant: ["Geschichte", "Küche"]
```

---

### Dimension 3: Completeness (Vollständigkeit)

**Definition:**
Grad, zu dem die Antwort alle relevanten Aspekte der Frage abdeckt.

**Bewertungsmethode:**
```python
# Schritt 1: Extrahiere Frage-Aspekte
question_aspects = extract_question_aspects(query)
# z.B. "Erkläre Photosynthese: Prozess, Bedeutung und Standorte"
#   → ["Prozess", "Bedeutung", "Standorte"]

# Schritt 2: Prüfe Abdeckung jedes Aspekts
aspect_coverage = {}
for aspect in question_aspects:
    coverage_level = assess_aspect_coverage(answer, aspect)
    # Levels: NONE (0), SUPERFICIAL (0.3), PARTIAL (0.6), COMPLETE (1.0)
    aspect_coverage[aspect] = coverage_level

# Schritt 3: Gewichte Aspekte
# Haupt-Aspekte haben höheres Gewicht
aspect_weights = assign_aspect_weights(question_aspects)

# Schritt 4: Berechne weighted score
completeness_score = sum(
    aspect_coverage[aspect] * aspect_weights[aspect]
    for aspect in question_aspects
) / sum(aspect_weights.values())
```

**Prompt-Template:**
```
Aufgabe: Bewerte die Vollständigkeit der Antwort.

Frage: {query}
Antwort: {answer}

Analyseschritte:
1. Identifiziere alle Aspekte, die die Frage adressiert haben möchte
2. Für jeden Aspekt: Bewerte die Tiefe der Behandlung
   - NONE: Nicht erwähnt
   - SUPERFICIAL: Nur oberflächlich erwähnt
   - PARTIAL: Teilweise erklärt
   - COMPLETE: Vollständig und detailliert erklärt

3. Identifiziere fehlende Informationen

Ausgabe (JSON):
{
  "aspects": [
    {
      "name": "...",
      "coverage": "NONE|SUPERFICIAL|PARTIAL|COMPLETE",
      "details": "Was wurde gesagt oder nicht gesagt"
    }
  ],
  "missing_information": ["..."],
  "completeness_score": 0.0-1.0,
  "reasoning": "..."
}
```

**Scoring-Regel:**
```yaml
coverage_levels:
  NONE: 0.0
  SUPERFICIAL: 0.3
  PARTIAL: 0.6
  COMPLETE: 1.0

aspect_importance:
  core: 1.0        # Kernaspekte der Frage
  secondary: 0.7   # Wichtige aber nicht zentrale Aspekte
  optional: 0.4    # Nice-to-have Details
```

**Qualitäts-Thresholds:**
```yaml
thresholds:
  unacceptable: "< 0.50"  # Viele wichtige Aspekte fehlen
  poor: "0.50-0.65"       # Mehrere Aspekte unvollständig
  acceptable: "0.65-0.80" # Die meisten Aspekte abgedeckt
  good: "0.80-0.90"       # Nahezu vollständig
  excellent: "0.90-1.0"   # Vollständig und detailliert
```

**Test-Cases:**
```yaml
test_cases:
  - query: "Erkläre Photosynthese: Prozess und Bedeutung"
    answer: "Photosynthese ist der Prozess, bei dem Pflanzen Lichtenergie nutzen, um aus CO2 und Wasser Glucose herzustellen. Dies ist wichtig, weil es Sauerstoff produziert und die Grundlage der Nahrungskette bildet."
    expected_aspects:
      - name: "Prozess"
        coverage: COMPLETE
      - name: "Bedeutung"
        coverage: COMPLETE
    expected_score: 1.0
  
  - query: "Erkläre Photosynthese: Prozess und Bedeutung"
    answer: "Photosynthese macht Glucose aus Licht."
    expected_aspects:
      - name: "Prozess"
        coverage: SUPERFICIAL
      - name: "Bedeutung"
        coverage: NONE
    expected_score: 0.15
```

---

### Dimension 4: Coherence (Kohärenz)

**Definition:**
Logischer Aufbau, innere Konsistenz und sprachliche Qualität der Antwort.

**Bewertungsmethode:**
```python
# Aspekt 1: Logischer Fluss
logical_flow_score = assess_logical_flow(answer)
# Prüft: Haben Sätze logische Übergänge? Ist Argumentation konsistent?

# Aspekt 2: Strukturelle Kohärenz
structural_coherence = assess_structure(answer)
# Prüft: Einleitung → Hauptteil → Schluss? Klare Gliederung?

# Aspekt 3: Sprachliche Qualität
linguistic_quality = assess_language_quality(answer)
# Prüft: Grammatik, Klarheit, Präzision

# Aspekt 4: Innere Konsistenz
internal_consistency = check_internal_consistency(answer)
# Prüft: Keine Widersprüche innerhalb der Antwort

coherence_score = (
    logical_flow_score * 0.3 +
    structural_coherence * 0.2 +
    linguistic_quality * 0.2 +
    internal_consistency * 0.3
)
```

**Prompt-Template:**
```
Aufgabe: Bewerte die Kohärenz und Struktur der Antwort.

Antwort: {answer}

Bewertungskriterien:

1. Logischer Fluss (0-10):
   - Folgen die Gedanken logisch aufeinander?
   - Sind Übergänge zwischen Ideen klar?
   - Ist die Argumentation nachvollziehbar?

2. Strukturelle Kohärenz (0-10):
   - Ist die Antwort gut strukturiert?
   - Gibt es eine klare Einleitung/Hauptteil/Schluss?
   - Sind Absätze sinnvoll gegliedert?

3. Sprachliche Qualität (0-10):
   - Ist die Sprache klar und präzise?
   - Grammatikalisch korrekt?
   - Angemessener Formalitätsgrad?

4. Innere Konsistenz (0-10):
   - Widerspricht sich die Antwort selbst?
   - Sind Aussagen konsistent?

Ausgabe (JSON):
{
  "logical_flow": 0-10,
  "structural_coherence": 0-10,
  "linguistic_quality": 0-10,
  "internal_consistency": 0-10,
  "identified_issues": ["..."],
  "coherence_score": 0.0-1.0,
  "reasoning": "..."
}
```

**Scoring-Regel:**
```python
# Normalisiere 0-10 Skala zu 0-1
normalized_scores = {
    'logical_flow': logical_flow / 10,
    'structural_coherence': structural_coherence / 10,
    'linguistic_quality': linguistic_quality / 10,
    'internal_consistency': internal_consistency / 10
}

# Gewichteter Durchschnitt
coherence_score = (
    normalized_scores['logical_flow'] * 0.3 +
    normalized_scores['structural_coherence'] * 0.2 +
    normalized_scores['linguistic_quality'] * 0.2 +
    normalized_scores['internal_consistency'] * 0.3
)
```

**Qualitäts-Thresholds:**
```yaml
thresholds:
  unacceptable: "< 0.50"  # Konfus, inkohärent
  poor: "0.50-0.65"       # Teilweise unklar
  acceptable: "0.65-0.80" # Größtenteils kohärent
  good: "0.80-0.90"       # Gut strukturiert und klar
  excellent: "0.90-1.0"   # Exzellent formuliert
```

**Test-Cases:**
```yaml
test_cases:
  - answer: "Paris ist die Hauptstadt von Frankreich. Die Stadt liegt an der Seine. Sie ist bekannt für den Eiffelturm und das Louvre-Museum."
    expected_logical_flow: 9
    expected_structural_coherence: 8
    expected_linguistic_quality: 9
    expected_internal_consistency: 10
    expected_score: 0.90
  
  - answer: "Paris ist die Hauptstadt. Aber Berlin auch. Frankreich hat viele Städte. Der Eiffelturm ist groß. Ich mag Käse."
    expected_logical_flow: 2
    expected_structural_coherence: 1
    expected_linguistic_quality: 5
    expected_internal_consistency: 3
    expected_score: 0.28
```

---

### Dimension 5: Ethical Compliance (NEU)

**Definition:**
Einhaltung ethischer Richtlinien: Respekt für menschliche Autonomie, moralische Vielfalt, keine Bevormundung.

**Bewertungsmethode:**
```python
# Nur relevant wenn ethischer Kontext erkannt wurde
if not ethics_manager.detectEthicalContext(query).has_ethical_context:
    return 1.0  # N/A

# Sub-Kriterium 1: Respektiert Autonomie?
autonomy_score = assess_autonomy_respect(answer)
# Prüft: Keine Befehle, keine "Sie müssen", Optionen präsentieren

# Sub-Kriterium 2: Moralische Vielfalt?
diversity_score = assess_moral_diversity(answer)
# Prüft: Multiple Perspektiven bei ethischen Fragen

# Sub-Kriterium 3: Quellen für ethische Claims?
citation_score = check_ethical_citations(answer)
# Prüft: Ethische Aussagen haben Quellenangaben

# Sub-Kriterium 4: Patronizing Language?
patronizing_penalty = detect_patronizing_language(answer)
# Prüft: Bevormundende Formulierungen

ethical_compliance_score = (
    autonomy_score * 0.4 +
    diversity_score * 0.3 +
    citation_score * 0.3
) - patronizing_penalty
```

**Prompt-Template:**
```
Aufgabe: Bewerte ethische Compliance der Antwort.

Kontext: Diese Frage hat ethische/moralische Aspekte.

Frage: {query}
Antwort: {answer}

Bewertungskriterien:

1. Respekt für menschliche Autonomie (0-10):
   - Präsentiert die Antwort Optionen statt Befehle?
   - Wird betont, dass der Mensch entscheidet?
   - Keine patronisierende Sprache ("Sie müssen...", "Es ist Ihre Pflicht...")?

2. Moralische Vielfalt (0-10):
   - Werden verschiedene moralphilosophische Perspektiven gezeigt?
   - (Utilitarismus, Deontologie, Tugendethik, religiös, etc.)
   - Wird anerkannt, dass es verschiedene Sichtweisen gibt?

3. Quellen für ethische Claims (0-10):
   - Haben ethische Aussagen Quellenangaben?
   - Ist klar, aus welcher Tradition sie stammen?

4. Bevormundung (List):
   - Liste alle bevormundenden Formulierungen auf

Ausgabe (JSON):
{
  "autonomy_respect": 0-10,
  "moral_diversity": 0-10,
  "ethical_citations": 0-10,
  "patronizing_phrases": ["..."],
  "violations": ["..."],
  "ethical_compliance_score": 0.0-1.0,
  "reasoning": "..."
}
```

**Scoring-Regel:**
```python
# Normalisiere Sub-Scores
autonomy = autonomy_respect / 10
diversity = moral_diversity / 10
citations = ethical_citations / 10

# Penalty für jede bevormundende Phrase
patronizing_penalty = min(0.5, len(patronizing_phrases) * 0.1)

# Final Score
ethical_compliance = max(0, (
    autonomy * 0.4 +
    diversity * 0.3 +
    citations * 0.3
) - patronizing_penalty)
```

**Qualitäts-Thresholds:**
```yaml
thresholds:
  unacceptable: "< 0.50"  # Ernsthafte ethische Verstöße
  poor: "0.50-0.70"       # Mehrere Probleme
  acceptable: "0.70-0.85" # Kleinere Verbesserungen möglich
  good: "0.85-0.95"       # Sehr gut
  excellent: "0.95-1.0"   # Vorbildlich
```

**Test-Cases:**
```yaml
test_cases:
  - query: "Ist Abtreibung moralisch vertretbar?"
    answer: "Aus utilitaristischer Sicht... Aus katholischer Sicht... Aus feministischer Sicht... Die Entscheidung liegt bei Ihnen."
    expected_autonomy: 10
    expected_diversity: 9
    expected_citations: 8
    expected_patronizing: []
    expected_score: 0.90
  
  - query: "Ist Abtreibung moralisch vertretbar?"
    answer: "Nein, Abtreibung ist moralisch falsch. Sie müssen das Kind bekommen."
    expected_autonomy: 1
    expected_diversity: 0
    expected_citations: 0
    expected_patronizing: ["Sie müssen"]
    expected_violations: ["Keine Autonomie", "Keine Vielfalt", "Bevormundung"]
    expected_score: 0.10
```

---

## Teil 3: Kombinierte Kriterien & Decision Matrix

### Overall Quality Score Berechnung

```python
# Gap Detector Decision
gap_result = gap_detector.detectGap(query, documents, answer, context)

if gap_result.gap_detected:
    if gap_result.confidence_score > 0.8:
        return REJECT_ANSWER  # High-confidence gap
    else:
        # Continue to Judge for second opinion
        pass

# Judge Evaluation
judge_result = judge.evaluate(query, documents, answer)

# Kombinierte Entscheidungsmatrix
def make_final_decision(gap_result, judge_result, config):
    # Scenario 1: Klarer Gap mit hoher Confidence
    if gap_result.gap_detected and gap_result.confidence_score > 0.85:
        return Decision(
            action="REJECT",
            reason="Knowledge gap detected",
            details=gap_result
        )
    
    # Scenario 2: Judge-Score zu niedrig
    if judge_result.overall_score < config.quality_threshold:
        return Decision(
            action="REJECT",
            reason="Quality below threshold",
            details=judge_result
        )
    
    # Scenario 3: Ethische Verstöße
    if judge_result.ethical_compliance_score < 0.7:
        return Decision(
            action="REJECT",
            reason="Ethical compliance failed",
            details=judge_result
        )
    
    # Scenario 4: Faithfulness kritisch
    if judge_result.faithfulness_score < config.faithfulness_threshold:
        return Decision(
            action="REJECT",
            reason="Too many unverified claims",
            details=judge_result
        )
    
    # Scenario 5: Alle OK
    return Decision(
        action="ACCEPT",
        confidence=min(
            1.0 - gap_result.confidence_score if gap_result.gap_detected else 1.0,
            judge_result.overall_score
        ),
        quality_metrics={
            "gap_detector": gap_result,
            "judge": judge_result
        }
    )
```

### Quality Gates Configuration

```yaml
quality_gates:
  # Gate 1: Pre-Generation
  pre_generation:
    similarity_threshold: 0.75
    min_documents: 3
    max_age_days: 365
    min_authority: 0.60
    
  # Gate 2: During Generation
  during_generation:
    min_token_probability: 0.50
    max_perplexity: 100
    
  # Gate 3: Post-Generation
  post_generation:
    min_groundedness: 0.70
    min_consistency: 0.60
    
  # Gate 4: Judge Evaluation
  judge_evaluation:
    min_overall_score: 0.70
    min_faithfulness: 0.80
    min_relevance: 0.65
    min_completeness: 0.65
    min_coherence: 0.65
    min_ethical_compliance: 0.70  # Bei ethischen Fragen
    
  # Kombinierte Entscheidung
  final_decision:
    require_all_gates: false  # true = strikt, false = majority
    ethical_questions_strict: true  # Ethische Fragen immer strikt
    critical_contexts_strict: true  # Medical/Legal/Financial strikt
```

---

## Zusammenfassung

### Knowledge Gap Detector Kriterien

| Kategorie | Kriterium | Messmethode | Threshold | Priority |
|-----------|-----------|-------------|-----------|----------|
| Pre-Gen | Similarity | Cosine Sim | 0.75 | HIGH |
| Pre-Gen | Doc Count | Count | 3+ | HIGH |
| Pre-Gen | Coverage | Aspect Match | 0.70 | MEDIUM |
| Pre-Gen | Diversity | Perspectives | 2+ | HIGH (ethical) |
| During-Gen | Token Prob | Avg Prob | 0.50 | HIGH |
| During-Gen | Perplexity | Exp(-log(p)) | <100 | MEDIUM |
| Post-Gen | Groundedness | Claim Verify | 0.70 | CRITICAL |
| Post-Gen | Consistency | Self-Check | 0.60 | MEDIUM |
| Context | Recency | Age Check | Context | MEDIUM |
| Context | Authority | Source Rating | 0.60 | MEDIUM |

### LLM-as-Judge Kriterien

| Dimension | Sub-Kriterien | Messmethode | Threshold | Weight |
|-----------|---------------|-------------|-----------|--------|
| Faithfulness | Claim Verification | NLI Model | 0.80 | 40% |
| Relevance | Intent Matching | Similarity | 0.65 | 30% |
| Completeness | Aspect Coverage | Coverage % | 0.65 | 20% |
| Coherence | Logical Flow | LLM Judge | 0.65 | 10% |
| Ethical Compliance | Autonomy/Diversity | Pattern Check | 0.70 | Veto |

### Kritische Entscheidungspfade

**Path 1: Fast Rejection (Gap Detector)**
```
Similarity < 0.50 OR Doc Count < 2 OR Groundedness < 0.50
→ IMMEDIATE REJECT
```

**Path 2: Quality Check (Judge)**
```
Overall Score < 0.70 OR Faithfulness < 0.80
→ REJECT
```

**Path 3: Ethical Veto**
```
Ethical Context AND Ethical Compliance < 0.70
→ REJECT (unabhängig von anderen Scores)
```

**Path 4: Accept**
```
ALL criteria met
→ ACCEPT with confidence score
```

---

*Erstellt: 2026-01-18*  
*Version: 1.0*  
*Status: Complete Criteria Definition*
