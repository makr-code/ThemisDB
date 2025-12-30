# Kapitel 17: LLM-Integration und Prompt Engineering

## Überblick

ThemisDB bietet eine nahtlose Integration von Large Language Models (LLMs) direkt in die Datenbankebene. Diese Integration ermöglicht es, LLM-Funktionalitäten wie Text-Generierung, Embedding-Erstellung, semantische Suche und RAG (Retrieval Augmented Generation) Patterns direkt in AQL-Queries zu verwenden.

**Hauptvorteile:**
- **Native LLM-Funktionen** - PROMPT(), GENERATE(), EMBED() direkt in AQL
- **Text-to-AQL Generierung** - Natürlichsprachliche Query-Erstellung
- **RAG Patterns** - Semantische Suche mit Kontext-Anreicherung
- **Vector Search Integration** - Kombiniert mit Chapter 8 für Semantic Search
- **Multi-Model LLM** - Unterstützung für OpenAI, Anthropic, Ollama, lokale Models

## 17.1 LLM-Funktionen in AQL

### 17.1.1 PROMPT() - Text-Generierung

Die `PROMPT()`-Funktion sendet Anfragen an konfigurierte LLM-Provider:

```aql
// Einfache Text-Generierung
FOR doc IN articles
  FILTER doc.category == 'technology'
  LIMIT 5
  RETURN {
    title: doc.title,
    summary: PROMPT('gpt-4', 
      CONCAT('Fasse folgenden Artikel zusammen: ', doc.content),
      {max_tokens: 150, temperature: 0.7}
    )
  }
```

**Erweiterte Optionen:**

```aql
// Mit System-Prompt und Strukturierung
FOR product IN products
  FILTER product.reviews_count > 100
  RETURN {
    product_name: product.name,
    analysis: PROMPT('gpt-4', 
      {
        system: 'Du bist ein Produkt-Analyst. Analysiere Kundenrezensionen.',
        user: CONCAT('Produkt: ', product.name, '\n\nBewertungen:\n', 
                     product.reviews_text),
        response_format: 'json'
      },
      {
        temperature: 0.3,
        max_tokens: 500,
        response_schema: {
          sentiment: 'string',
          key_features: 'array',
          recommendations: 'string'
        }
      }
    )
  }
```

### 17.1.2 EMBED() - Embedding-Generierung

Erstellt Vektor-Embeddings für Text:

```aql
// Dokument mit Embedding speichern
INSERT {
  title: 'Einführung in ThemisDB',
  content: 'ThemisDB ist eine Multi-Model-Datenbank...',
  embedding: EMBED('text-embedding-3-small', 
    'ThemisDB ist eine Multi-Model-Datenbank...')
} INTO documents

// Batch-Embedding für Performance
FOR doc IN documents
  FILTER doc.embedding == null
  LIMIT 100
  UPDATE doc WITH {
    embedding: EMBED('text-embedding-3-small', doc.content),
    embedded_at: DATE_NOW()
  } IN documents
```

### 17.1.3 GENERATE() - Strukturierte Generierung

Generiert strukturierte Daten basierend auf Schema:

```aql
// Strukturierte Produkt-Kategorisierung
FOR product IN products
  FILTER product.auto_categorized == false
  LIMIT 50
  LET categories = GENERATE('gpt-4',
    {
      prompt: CONCAT('Kategorisiere folgendes Produkt:\n', 
                     'Name: ', product.name, '\n',
                     'Beschreibung: ', product.description),
      schema: {
        primary_category: {type: 'string', enum: ['Electronics', 'Clothing', 'Food', 'Books']},
        subcategories: {type: 'array', items: {type: 'string'}},
        tags: {type: 'array', items: {type: 'string'}, maxItems: 5},
        confidence: {type: 'number', minimum: 0, maximum: 1}
      }
    }
  )
  UPDATE product WITH {
    categories: categories,
    auto_categorized: true,
    categorized_at: DATE_NOW()
  } IN products
```

## 17.2 Text-to-AQL Generierung

### 17.2.1 Natural Language Query Interface

ThemisDB kann natürlichsprachliche Anfragen in AQL übersetzen:

```aql
// Text-to-AQL Helper Function
LET user_question = 'Zeige mir die 10 teuersten Produkte aus der Elektronik-Kategorie'

LET generated_query = PROMPT('gpt-4',
  {
    system: `Du bist ein AQL-Experte. Konvertiere Benutzeranfragen in gültige AQL-Queries.
    
    Schema:
    - products (name, price, category, stock)
    - orders (order_id, customer_id, product_id, quantity, order_date)
    - customers (customer_id, name, email, country)
    
    Gebe nur die AQL-Query zurück, keine Erklärungen.`,
    user: user_question
  },
  {temperature: 0.1}
)

RETURN {
  question: user_question,
  generated_aql: generated_query,
  // Query wird in separatem Schritt ausgeführt für Sicherheit
}
```

### 17.2.2 Query-Validierung und -Ausführung

**Sicherheitsvalidierung:**

```aql
// Query-Validierung vor Ausführung
LET user_input = @user_question

LET llm_response = PROMPT('gpt-4',
  {
    system: `Generiere sichere AQL-Queries. Verwende immer @parameter für Benutzereingaben.
    Keine destructive Operationen (DELETE, DROP, etc.) ohne explizite Bestätigung.`,
    user: user_input
  }
)

// Validiere Query
LET is_safe = (
  llm_response.query NOT LIKE '%DELETE%' AND
  llm_response.query NOT LIKE '%DROP%' AND
  llm_response.query NOT LIKE '%REMOVE%' AND
  llm_response.includes_parameters == true
)

RETURN is_safe ? llm_response : {error: 'Unsafe query detected'}
```

## 17.3 RAG (Retrieval Augmented Generation)

### 17.3.1 Semantische Suche mit Kontext

Kombiniert Vector Search mit LLM-Generierung:

```aql
// RAG Pattern: Suche + Generierung
LET user_query = 'Wie funktioniert MVCC in ThemisDB?'

// Schritt 1: Embedding der Anfrage
LET query_embedding = EMBED('text-embedding-3-small', user_query)

// Schritt 2: Semantische Suche
LET relevant_docs = (
  FOR doc IN documentation
    LET similarity = COSINE_SIMILARITY(doc.embedding, query_embedding)
    FILTER similarity > 0.7
    SORT similarity DESC
    LIMIT 5
    RETURN {
      content: doc.content,
      title: doc.title,
      similarity: similarity
    }
)

// Schritt 3: Kontext zusammenführen
LET context = (
  FOR doc IN relevant_docs
    RETURN CONCAT('## ', doc.title, '\n\n', doc.content)
)

// Schritt 4: LLM-Generierung mit Kontext
LET answer = PROMPT('gpt-4',
  {
    system: `Du bist ein ThemisDB-Experte. Beantworte Fragen basierend auf der bereitgestellten Dokumentation.
    Zitiere relevante Abschnitte wenn möglich.`,
    user: CONCAT(
      'Dokumentation:\n\n',
      CONCAT_SEPARATOR('\n\n---\n\n', context),
      '\n\n---\n\n',
      'Frage: ', user_query
    )
  },
  {temperature: 0.3, max_tokens: 1000}
)

RETURN {
  question: user_query,
  answer: answer,
  sources: relevant_docs[*].title,
  source_count: LENGTH(relevant_docs)
}
```

### 17.3.2 Erweiterte RAG mit Re-Ranking

```aql
// Hybrid Search: BM25 + Vector + Re-Ranking
LET user_query = 'Beste Performance-Optimierungen für Graphen-Queries'

// Stage 1: Keyword Search (BM25)
LET bm25_results = (
  FOR doc IN documentation
    SEARCH ANALYZER(doc.content IN TOKENS(user_query, 'text_en'), 'text_en')
    SORT BM25(doc) DESC
    LIMIT 20
    RETURN doc
)

// Stage 2: Vector Search
LET query_embedding = EMBED('text-embedding-3-small', user_query)
LET vector_results = (
  FOR doc IN documentation
    LET similarity = COSINE_SIMILARITY(doc.embedding, query_embedding)
    FILTER similarity > 0.6
    SORT similarity DESC
    LIMIT 20
    RETURN doc
)

// Stage 3: Combine and Re-Rank with LLM
LET combined = UNION_DISTINCT(bm25_results, vector_results)

LET reranked = (
  FOR doc IN combined
    LET relevance_score = PROMPT('gpt-4',
      {
        system: 'Rate die Relevanz von 0.0 bis 1.0. Gebe nur die Zahl zurück.',
        user: CONCAT(
          'Query: ', user_query, '\n\n',
          'Dokument: ', SUBSTRING(doc.content, 0, 500)
        )
      },
      {temperature: 0.0, max_tokens: 5}
    )
    RETURN {
      doc: doc,
      score: TO_NUMBER(relevance_score)
    }
)

LET top_docs = (
  FOR item IN reranked
    SORT item.score DESC
    LIMIT 5
    RETURN item.doc
)

// Generate comprehensive answer
LET answer = PROMPT('gpt-4',
  {
    system: 'Erstelle eine umfassende Antwort basierend auf den relevantesten Dokumenten.',
    user: CONCAT(
      'Top Dokumente:\n\n',
      (FOR doc IN top_docs
        RETURN CONCAT('---\n', doc.content)
      ),
      '\n\nFrage: ', user_query
    )
  },
  {temperature: 0.3, max_tokens: 1500}
)

RETURN {
  query: user_query,
  answer: answer,
  sources: top_docs[*].title
}
```

### 17.3.3 ThemisDB's Pre-Filtering-Vorteil für RAG

**Das Post-Filtering-Problem in Polyglot-Systemen:**

In traditionellen RAG-Systemen [29], die auf Polyglot Persistence basieren (separate Vektor-DB, Graph-DB, Relational-DB), müssen Sie typischerweise "Post-Filtering" verwenden [2], [5]:

```python
# Traditioneller Ansatz (ineffizient):
# 1. Vektor-DB: Hole 1000 ähnliche Dokumente
vector_results = vector_db.search(query_embedding, k=1000)

# 2. Graph-DB: Hole erlaubte Dokumente basierend auf Graph-Kontext
allowed_docs = graph_db.query("MATCH (user)-[:CAN_ACCESS]->(doc)")

# 3. Relational-DB: Hole Metadaten-Filter (z.B. Jahr=2024)
filtered_docs = relational_db.query("SELECT id WHERE year=2024")

# 4. Manuelle Schnittmengenbildung im Application-Code
final_results = intersect(vector_results, allowed_docs, filtered_docs)
# → Problem: 990 von 1000 Ergebnissen werden verworfen!
```

**Warum ist das ineffizient?**
- Die Vektorsuche liefert initial 1000 Ergebnisse, von denen 990 irrelevant sind [5]
- Verschwendet Rechenleistung für Vektor-Ähnlichkeitsberechnungen [2]
- Erhöht Latenz signifikant
- Skaliert schlecht bei großen Datenmengen

**ThemisDB's Pre-Filtering-Architektur:**

Dank der nativen Multi-Model-Architektur [3], [11] (siehe Kapitel 3.2) kann ThemisDB die Query-Ausführung umkehren [2]:

```aql
-- Pre-Filtering in ThemisDB (hochperformant):
FOR doc IN documents
  -- Phase 1: ZUERST relationale Filter (schneller Index-Zugriff)
  FILTER doc.year == 2024
  FILTER doc.department == "IT"
  FILTER doc.confidentiality <= @user_clearance_level
  
  -- Phase 2: Graph-Kontext-Filter
  FILTER doc._id IN (
    FOR v, e IN 1..2 OUTBOUND @user_id access_rights
      RETURN v._id
  )
  
  -- Phase 3: Vektorsuche NUR auf erlaubter Teilmenge
  LET similarity = COSINE_SIMILARITY(doc.embedding, @query_embedding)
  FILTER similarity > 0.7
  
  SORT similarity DESC
  LIMIT 10
  RETURN doc
```

**Wie funktioniert Pre-Filtering intern?**

1. **Phase 1 (Relationaler Filter):** 
   - Query-Engine nutzt schnelle Sekundärindizes (z.B. Index für `year=2024`) [2], [3]
   - Erstellt eine hochselektive Kandidatenliste (z.B. ein Bitset mit 50 erlaubten IDs)

2. **Phase 2 (Graph-Filter):**
   - Graph-Traversierung [22] auf dieser reduzierten Menge
   - Weitere Einschränkung basierend auf Zugriffsrechten

3. **Phase 3 (Vektorsuche):**
   - Die rechenintensive HNSW-Vektorsuche [25] läuft NUR auf den 50 erlaubten Dokumenten
   - Statt 1000 Vektor-Vergleiche nur 50 notwendig [2]

**Performance-Vergleich:**

| Ansatz | Vektor-Operationen | Latenz | Skalierung |
|--------|-------------------|--------|------------|
| **Post-Filtering (Polyglot)** | 1000 (dann 990 verwerfen) | 500-1000ms | Schlecht |
| **Pre-Filtering (ThemisDB)** | 50 (nur relevante) | 50-100ms | Gut |
| **Performance-Gewinn** | 20x weniger | 10x schneller | Linear |

**Praktisches Beispiel - Verwaltungs-RAG:**

```aql
-- Finde relevante BImSchG-Gutachten für Bürgeranfrage
LET user_query = "Lärmschutz Windkraftanlage Havelland"
LET query_embedding = EMBED('text-embedding-3-small', user_query)

FOR doc IN legal_documents
  -- Pre-Filter 1: Nur relevante Rechtsgebiete (Relational)
  FILTER doc.legal_area == "BImSchG"
  FILTER doc.topic IN ["Lärmschutz", "Windenergie"]
  
  -- Pre-Filter 2: Nur Dokumente für Region (Graph)
  FILTER doc.region IN (
    FOR r IN regions
      FILTER r.name == "Havelland" OR r.parent_region == "Havelland"
      RETURN r._id
  )
  
  -- Pre-Filter 3: Nur aktuelle, gültige Gutachten (Relational)
  FILTER doc.status == "valid"
  FILTER doc.valid_until > DATE_NOW()
  
  -- ERST JETZT: Vektorsuche auf stark reduzierter Menge
  LET similarity = COSINE_SIMILARITY(doc.embedding, query_embedding)
  FILTER similarity > 0.75
  
  SORT similarity DESC
  LIMIT 5
  
  RETURN {
    title: doc.title,
    similarity: similarity,
    metadata: {
      case_number: doc.case_number,
      date: doc.issue_date,
      region: doc.region
    },
    excerpt: SUBSTRING(doc.content, 0, 300)
  }
```

**Architektonischer Vorteil:**

Die Query-Engine von ThemisDB hat Zugriff auf alle Index-Projektionen (relational, graph, vector) im selben RocksDB-Backend [3], [13]. Sie kann einen kostenbasierten Optimizer verwenden, um den effizientesten Ausführungsplan zu wählen:
- Welcher Filter ist am selektivsten?
- In welcher Reihenfolge sollten Filter angewendet werden?
- Wann lohnt sich der Übergang von Index-Scan zu Vektor-Suche?

Dies ist in Polyglot-Systemen [33] unmöglich, da jede Datenbank isoliert operiert.

## 17.4 Prompt Engineering Best Practices

### 17.4.1 Chain-of-Thought Prompting

```aql
// Multi-Step Reasoning mit CoT
FOR customer IN customers
  FILTER customer.churn_risk == null
  LIMIT 10
  
  // Schritt 1: Daten sammeln
  LET customer_data = {
    orders_count: LENGTH(
      FOR o IN orders
        FILTER o.customer_id == customer._key
        RETURN 1
    ),
    last_order_date: (
      FOR o IN orders
        FILTER o.customer_id == customer._key
        SORT o.order_date DESC
        LIMIT 1
        RETURN o.order_date
    )[0],
    total_spent: SUM(
      FOR o IN orders
        FILTER o.customer_id == customer._key
        RETURN o.total_amount
    ),
    support_tickets: LENGTH(
      FOR t IN support_tickets
        FILTER t.customer_id == customer._key
        RETURN 1
    )
  }
  
  // Schritt 2: LLM-Analyse mit Chain-of-Thought
  LET analysis = PROMPT('gpt-4',
    {
      system: `Du bist ein Churn-Prediction-Experte. Analysiere Schritt für Schritt:
      1. Bewerte die Aktivität des Kunden
      2. Analysiere Kaufverhalten
      3. Berücksichtige Support-Interaktionen
      4. Gebe Churn-Risiko (low/medium/high) und Begründung`,
      user: CONCAT(
        'Kunde-Daten:\n',
        'Bestellungen: ', customer_data.orders_count, '\n',
        'Letzte Bestellung: ', customer_data.last_order_date, '\n',
        'Gesamtumsatz: €', customer_data.total_spent, '\n',
        'Support-Tickets: ', customer_data.support_tickets, '\n\n',
        'Analysiere Schritt für Schritt das Churn-Risiko.'
      )
    },
    {temperature: 0.2, max_tokens: 500}
  )
  
  UPDATE customer WITH {
    churn_risk: analysis.risk_level,
    churn_reasoning: analysis.reasoning,
    analyzed_at: DATE_NOW()
  } IN customers
```

### 17.4.2 Few-Shot Learning

```aql
// Classification mit Few-Shot Examples
LET examples = [
  {text: 'Produkt kam defekt an', category: 'quality_issue'},
  {text: 'Lieferung zwei Wochen zu spät', category: 'delivery_issue'},
  {text: 'Falscher Artikel geliefert', category: 'order_error'},
  {text: 'Sehr zufrieden, gerne wieder!', category: 'positive_feedback'}
]

FOR ticket IN support_tickets
  FILTER ticket.category == null
  LIMIT 100
  
  LET category = PROMPT('gpt-4',
    {
      system: 'Kategorisiere Support-Tickets basierend auf den Beispielen.',
      user: CONCAT(
        'Beispiele:\n',
        (FOR ex IN examples
          RETURN CONCAT(ex.text, ' -> ', ex.category)
        ),
        '\n\nNeues Ticket: ', ticket.message, '\n\nKategorie:'
      )
    },
    {temperature: 0.1, max_tokens: 20}
  )
  
  UPDATE ticket WITH {
    category: category,
    auto_categorized: true
  } IN support_tickets
```

## 17.5 Multi-Model LLM Patterns

### 17.5.1 Graph + LLM

```aql
// Knowledge Graph Reasoning mit LLM
FOR person IN persons
  FILTER person._key == @person_id
  
  // Graphtraversierung für Kontext
  LET connections = (
    FOR v, e, p IN 1..3 OUTBOUND person GRAPH 'social_network'
      RETURN {
        name: v.name,
        relationship: e.type,
        depth: LENGTH(p.edges)
      }
  )
  
  // LLM analysiert das soziale Netzwerk
  LET network_analysis = PROMPT('gpt-4',
    {
      system: 'Analysiere das soziale Netzwerk und gebe Empfehlungen.',
      user: CONCAT(
        'Person: ', person.name, '\n',
        'Verbindungen:\n',
        (FOR c IN connections
          RETURN CONCAT('- ', c.name, ' (', c.relationship, ', Distanz: ', c.depth, ')')
        ),
        '\n\nWelche Personen sollten miteinander vernetzt werden?'
      )
    }
  )
  
  RETURN {
    person: person.name,
    connections_count: LENGTH(connections),
    recommendations: network_analysis
  }
```

### 17.5.2 Temporal + LLM

```aql
// Zeit-basierte Trend-Analyse mit LLM
LET trend_data = (
  FOR metric IN metrics
    FILTER metric.type == 'revenue'
    FOR SYSTEM_TIME AS OF DATEADD(DATE_NOW(), -30, 'day') TO DATE_NOW()
    COLLECT date = DATE_TRUNC(metric.timestamp, 'day')
    AGGREGATE daily_revenue = SUM(metric.value)
    SORT date
    RETURN {date: date, revenue: daily_revenue}
)

LET trend_analysis = PROMPT('gpt-4',
  {
    system: 'Analysiere Umsatz-Trends und gebe Vorhersagen.',
    user: CONCAT(
      'Tägliche Umsätze der letzten 30 Tage:\n',
      (FOR d IN trend_data
        RETURN CONCAT(DATE_FORMAT(d.date, '%Y-%m-%d'), ': €', d.revenue)
      ),
      '\n\nAnalysiere Trends und gebe eine 7-Tage-Vorhersage.'
    )
  },
  {temperature: 0.3}
)

RETURN {
  data: trend_data,
  analysis: trend_analysis
}
```

## 17.6 LLM-Konfiguration und Provider

### 17.6.1 Provider-Konfiguration

```aql
// Multi-Provider Setup
LET providers = {
  openai: {
    api_key: @openai_key,
    models: ['gpt-4', 'gpt-3.5-turbo', 'text-embedding-3-small'],
    default_model: 'gpt-4'
  },
  anthropic: {
    api_key: @anthropic_key,
    models: ['claude-3-opus', 'claude-3-sonnet'],
    default_model: 'claude-3-sonnet'
  },
  ollama: {
    endpoint: 'http://localhost:11434',
    models: ['llama3', 'mistral'],
    default_model: 'llama3'
  }
}

// Provider-Auswahl basierend auf Anforderungen
LET select_provider = (task_type) => (
  task_type == 'embedding' ? 'openai' :
  task_type == 'long_context' ? 'anthropic' :
  task_type == 'local' ? 'ollama' :
  'openai'
)
```

### 17.6.2 Kosten-Optimierung

```aql
// Smart Model Selection basierend auf Komplexität
FOR task IN tasks
  FILTER task.processed == false
  
  // Einfache Aufgaben -> Günstigeres Modell
  LET complexity = (
    LENGTH(task.input) > 1000 ? 'high' :
    CONTAINS(task.input, 'analysiere') ? 'medium' :
    'low'
  )
  
  LET model = (
    complexity == 'high' ? 'gpt-4' :
    complexity == 'medium' ? 'gpt-3.5-turbo' :
    'gpt-3.5-turbo'
  )
  
  LET result = PROMPT(model, task.input, 
    {
      temperature: complexity == 'high' ? 0.7 : 0.3,
      max_tokens: complexity == 'high' ? 2000 : 500
    }
  )
  
  UPDATE task WITH {
    result: result,
    model_used: model,
    cost_tier: complexity,
    processed: true
  } IN tasks
```

## 17.7 Prompt Template Library

### 17.7.1 Vordefinierte Templates

```aql
// Template-System für wiederverwendbare Prompts
LET templates = {
  summarize: {
    system: 'Du bist ein Zusammenfassungs-Experte. Erstelle prägnante Zusammenfassungen.',
    user_template: 'Fasse folgenden Text zusammen:\n\n{{content}}\n\nZusammenfassung (max {{max_words}} Wörter):'
  },
  translate: {
    system: 'Du bist ein professioneller Übersetzer.',
    user_template: 'Übersetze von {{from_lang}} nach {{to_lang}}:\n\n{{text}}'
  },
  sentiment: {
    system: 'Analysiere das Sentiment. Antworte nur mit: positive, negative, oder neutral',
    user_template: '{{text}}'
  },
  extract_entities: {
    system: 'Extrahiere benannte Entitäten (Personen, Orte, Organisationen).',
    user_template: '{{text}}\n\nFormat: JSON {persons: [], locations: [], organizations: []}'
  }
}

// Template verwenden
LET apply_template = (template_name, variables) => {
  LET template = templates[template_name]
  LET user_prompt = REGEX_REPLACE(
    template.user_template,
    '\\{\\{(\\w+)\\}\\}',
    variables
  )
  RETURN {system: template.system, user: user_prompt}
}

FOR article IN articles
  LIMIT 10
  LET summary = PROMPT('gpt-3.5-turbo',
    apply_template('summarize', {
      content: article.content,
      max_words: '100'
    })
  )
  RETURN {title: article.title, summary: summary}
```

## 17.8 Monitoring und Logging

### 17.8.1 LLM-Usage Tracking

```aql
// Track LLM-Nutzung für Kostenanalyse
INSERT {
  timestamp: DATE_NOW(),
  model: @model,
  provider: @provider,
  input_tokens: @input_tokens,
  output_tokens: @output_tokens,
  cost: @cost,
  latency_ms: @latency,
  task_type: @task_type,
  user_id: @user_id
} INTO llm_usage_log

// Kosten-Analyse
FOR log IN llm_usage_log
  FILTER log.timestamp > DATE_SUBTRACT(DATE_NOW(), 7, 'day')
  COLLECT 
    model = log.model,
    task_type = log.task_type
  AGGREGATE 
    total_calls = COUNT(1),
    total_cost = SUM(log.cost),
    avg_latency = AVG(log.latency_ms),
    total_input_tokens = SUM(log.input_tokens),
    total_output_tokens = SUM(log.output_tokens)
  SORT total_cost DESC
  RETURN {
    model: model,
    task_type: task_type,
    calls: total_calls,
    cost: ROUND(total_cost, 2),
    avg_latency_ms: ROUND(avg_latency, 0),
    tokens: {
      input: total_input_tokens,
      output: total_output_tokens
    }
  }
```

## 17.9 Sicherheit und Compliance

### 17.9.1 Input Sanitization

```aql
// Sichere LLM-Anfragen durch Sanitization
LET sanitize_input = (user_input) => {
  // Entferne potenziell schädliche Prompts
  LET cleaned = REGEX_REPLACE(user_input, 'ignore previous instructions', '', 'i')
  LET cleaned2 = REGEX_REPLACE(cleaned, 'disregard', '', 'i')
  LET cleaned3 = SUBSTITUTE(cleaned2, ['system:', 'assistant:'], '')
  
  // Längen-Begrenzung
  RETURN LENGTH(cleaned3) > 5000 ? SUBSTRING(cleaned3, 0, 5000) : cleaned3
}

FOR request IN user_requests
  LET safe_input = sanitize_input(request.query)
  LET response = PROMPT('gpt-4', safe_input, {temperature: 0.3})
  
  // Log für Audit
  INSERT {
    timestamp: DATE_NOW(),
    user_id: request.user_id,
    original_input: request.query,
    sanitized_input: safe_input,
    response: response,
    flagged: request.query != safe_input
  } INTO llm_audit_log
  
  RETURN response
```

### 17.9.2 PII-Erkennung und Redaktion

```aql
// Automatische PII-Erkennung vor LLM-Verarbeitung
LET detect_pii = (text) => {
  RETURN PROMPT('gpt-4',
    {
      system: 'Erkenne und markiere personenbezogene Daten (PII). Gebe JSON zurück: {has_pii: boolean, types: []}',
      user: text
    },
    {temperature: 0.0, response_format: 'json'}
  )
}

FOR document IN documents
  FILTER document.pii_checked == false
  
  LET pii_check = detect_pii(document.content)
  
  // Redaktiere PII falls notwendig
  LET safe_content = pii_check.has_pii ? 
    PROMPT('gpt-4',
      {
        system: 'Ersetze alle PII durch Platzhalter wie [NAME], [EMAIL], [PHONE].',
        user: document.content
      }
    ) : document.content
  
  UPDATE document WITH {
    content_safe: safe_content,
    has_pii: pii_check.has_pii,
    pii_types: pii_check.types,
    pii_checked: true
  } IN documents
```

## 17.10 Performance-Optimierung

### 17.10.1 Caching von LLM-Responses

```aql
// Cache für häufige Anfragen
LET query_hash = SHA256(CONCAT(user_query, model_name))

// Prüfe Cache
LET cached = (
  FOR c IN llm_cache
    FILTER c.query_hash == query_hash
    FILTER c.timestamp > DATE_SUBTRACT(DATE_NOW(), 24, 'hour')
    LIMIT 1
    RETURN c.response
)[0]

// Verwende Cache oder rufe LLM auf
LET response = cached != null ? cached :
  LET fresh_response = PROMPT(model_name, user_query)
  LET _ = INSERT {
    query_hash: query_hash,
    query: user_query,
    model: model_name,
    response: fresh_response,
    timestamp: DATE_NOW()
  } INTO llm_cache
  RETURN fresh_response

RETURN response
```

### 17.10.2 Batch-Verarbeitung

```aql
// Batch-LLM-Calls für bessere Performance
LET batch_size = 20

FOR batch IN (
  FOR doc IN documents
    FILTER doc.summary == null
    LIMIT 100
    COLLECT batch_id = FLOOR(TO_NUMBER(doc._key) / batch_size)
    INTO group
    RETURN group
)
  
  // Einzelner LLM-Call für ganzen Batch
  LET batch_summaries = PROMPT('gpt-4',
    {
      system: 'Erstelle Zusammenfassungen für mehrere Dokumente. Gebe JSON-Array zurück.',
      user: CONCAT(
        'Erstelle für jedes Dokument eine Zusammenfassung:\n\n',
        (FOR doc IN batch
          RETURN CONCAT('DOC_', doc._key, ':\n', doc.content, '\n\n')
        )
      )
    },
    {temperature: 0.3, response_format: 'json'}
  )
  
  // Verteile Ergebnisse
  FOR doc IN batch
    LET summary = batch_summaries['DOC_' + doc._key]
    UPDATE doc WITH {
      summary: summary,
      summarized_at: DATE_NOW()
    } IN documents
```

## 17.11 Praktische Anwendungsfälle

### 17.11.1 Intelligente Suchvorschläge

```aql
// Query-Expansion mit LLM
LET user_search = @search_term

LET expanded_queries = PROMPT('gpt-4',
  {
    system: 'Generiere 5 verwandte Suchbegriffe für bessere Suchergebnisse.',
    user: CONCAT('Ursprüngliche Suche: ', user_search)
  }
)

// Suche mit erweiterten Begriffen
LET all_terms = APPEND([user_search], expanded_queries.terms)

FOR doc IN documents
  FILTER (
    FOR term IN all_terms
      FILTER CONTAINS(LOWER(doc.content), LOWER(term))
      RETURN 1
  ) ANY
  RETURN doc
```

### 17.11.2 Automatische Daten-Anreicherung

```aql
// Produkt-Metadaten durch LLM anreichern
FOR product IN products
  FILTER product.enriched == false
  LIMIT 50
  
  LET enrichment = PROMPT('gpt-4',
    {
      system: 'Analysiere das Produkt und gebe strukturierte Metadaten zurück.',
      user: CONCAT(
        'Produkt: ', product.name, '\n',
        'Beschreibung: ', product.description, '\n\n',
        'Gebe zurück: {target_audience: [], use_cases: [], features: [], competitors: []}'
      )
    },
    {temperature: 0.3, response_format: 'json'}
  )
  
  UPDATE product WITH {
    metadata: enrichment,
    enriched: true,
    enriched_at: DATE_NOW()
  } IN products
```

### 17.11.3 Sentiment-basierte Alerting

```aql
// Echtzeit-Sentiment-Monitoring mit Alerts
FOR review IN reviews
  FILTER review.sentiment == null
  FILTER review.created_at > DATE_SUBTRACT(DATE_NOW(), 1, 'hour')
  
  LET sentiment_analysis = PROMPT('gpt-4',
    {
      system: 'Analysiere das Sentiment und gebe Dringlichkeit (critical/high/medium/low) zurück.',
      user: review.text
    },
    {temperature: 0.1, response_format: 'json'}
  )
  
  // Alert bei negativem Sentiment
  LET alert = sentiment_analysis.sentiment == 'negative' AND 
              sentiment_analysis.urgency IN ['critical', 'high'] ?
    INSERT {
      type: 'negative_review',
      review_id: review._key,
      product_id: review.product_id,
      urgency: sentiment_analysis.urgency,
      reason: sentiment_analysis.reason,
      timestamp: DATE_NOW()
    } INTO alerts : null
  
  UPDATE review WITH {
    sentiment: sentiment_analysis.sentiment,
    sentiment_score: sentiment_analysis.score,
    urgency: sentiment_analysis.urgency,
    analyzed_at: DATE_NOW()
  } IN reviews
```

## 17.12 Best Practices Zusammenfassung

### DO ✅

1. **Verwende @parameter binding** für alle Benutzereingaben
2. **Cache häufige Anfragen** um Kosten zu sparen
3. **Validiere LLM-Outputs** vor der Speicherung
4. **Batch-Verarbeitung** für große Datenmengen
5. **Monitor Kosten** und Performance kontinuierlich
6. **Sanitize Inputs** vor LLM-Calls
7. **Verwende strukturierte Outputs** (JSON) wenn möglich
8. **Implementiere Fallbacks** bei LLM-Fehlern

### DON'T ❌

1. **Keine sensiblen Daten** ungefiltert an LLMs senden
2. **Keine unvalidierten LLM-Queries** ausführen
3. **Keine unbegrenzten LLM-Calls** ohne Rate-Limiting
4. **Keine Hardcoded API-Keys** im Code
5. **Keine synchronen LLM-Calls** für zeitkritische Operationen
6. **Keine Abhängigkeit** von einem einzelnen Provider

## Zusammenfassung

ThemisDB's LLM-Integration ermöglicht:

- **Native AQL-Funktionen** für Text-Generierung, Embeddings und strukturierte Ausgaben
- **Text-to-AQL** für natürlichsprachliche Query-Erstellung
- **RAG Patterns** mit semantischer Suche und Kontext-Anreicherung
- **Multi-Model Synergien** mit Graph, Temporal und Vector Search
- **Kosten-Optimierung** durch intelligentes Caching und Model-Selection
- **Enterprise-Grade Sicherheit** mit Input-Sanitization und PII-Schutz

Die Integration von LLMs direkt in die Datenbankebene reduziert Latenz, vereinfacht Architektur und ermöglicht völlig neue Anwendungsfälle von intelligenter Datenanalyse bis zu automatisierter Content-Generierung.

---

**Nächstes Kapitel:** [Kapitel 18: Machine Learning Integration](chapter_18_ml.md)
**Vorheriges Kapitel:** [Kapitel 16: Machine Learning](chapter_16_ml.md)
