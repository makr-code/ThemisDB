> **Hinweis:** Inhalt ist konzeptuell/referenziell. Code-Bezüge mit `<!-- TODO: verify against source -->` markiert.

# Wissensquellen für LLM-Kontext

Diese Dokumentation beschreibt die verfügbaren Wissensquellen und deren Integration in das Debate-System.

## Übersicht

Das `KnowledgeResearcher`-Modul recherchiert automatisch Hintergrundwissen aus verschiedenen Quellen, um dem LLM fundierten Kontext für philosophische Debatten bereitzustellen.

## Verfügbare Quellen

### 1. Wikipedia (Primärquelle)

**Zugriff:** Wikipedia MediaWiki API  
**Sprachen:** Deutsch (de), Englisch (en)  
**API Endpoint:** `https://de.wikipedia.org/w/api.php`

**Vorteile:**
- Keine API-Keys erforderlich
- Umfassende Artikel zu philosophischen Themen
- Mehrsprachig verfügbar
- Strukturierte Daten über API

**Verwendung:**
```python
from knowledge_researcher import KnowledgeResearcher

researcher = KnowledgeResearcher(wikipedia_lang="de")
context = researcher.research_topic("Kategorischer Imperativ")
```

**Abgedeckte Bereiche:**
- Philosophische Konzepte und Theorien
- Biographien von Philosophen
- Ethische Schulen und Strömungen
- Historischer Kontext

### 2. Stanford Encyclopedia of Philosophy (SEP)

**Zugriff:** Web (URL-Mapping)  
**URL:** `https://plato.stanford.edu/entries/`  
**Lizenz:** Frei zugänglich

**Vorteile:**
- Höchste akademische Qualität
- Peer-reviewed Artikel
- Umfassende Bibliographien
- Regelmäßig aktualisiert

**Abgedeckte Themen:**
- Kantian Ethics: `kant-moral`
- Utilitarianism: `utilitarianism-history`
- Virtue Ethics: `ethics-virtue`
- Contractualism: `contractualism`
- Meta-Ethics: `metaethics`
- Moral Realism: `moral-realism`
- Deontology: `ethics-deontological`

**Limitierungen:**
- Kein öffentliches API (nur Web-Scraping möglich)
- Nur englischsprachig
- Rate-Limiting beachten

### 3. Internet Encyclopedia of Philosophy (IEP)

**Zugriff:** Web  
**URL:** `https://iep.utm.edu/`  
**Lizenz:** Frei zugänglich

**Vorteile:**
- Akademisch geprüft
- Zugänglich geschrieben
- Breite Themenabdeckung

**Verwendung:** Ähnlich wie SEP, über URL-Mapping

### 4. Semantic Scholar

**Zugriff:** REST API  
**API:** `https://api.semanticscholar.org/graph/v1`  
**Lizenz:** Frei (Rate-Limits beachten)

**Vorteile:**
- Wissenschaftliche Paper aus allen Disziplinen
- Citation Counts
- Abstracts verfügbar
- Keine API-Keys erforderlich (für Basis-Zugriff)

**Rate Limits:**
- 100 Anfragen pro 5 Minuten (ohne Key)
- 1000 Anfragen pro 5 Minuten (mit API Key)

**Verwendung:**
```python
researcher = KnowledgeResearcher(enable_semantic_scholar=True)
context = researcher.research_topic("utilitarian ethics", depth="deep")
```

**Suchfelder:**
- `title`: Titel des Papers
- `abstract`: Zusammenfassung
- `authors`: Autoren
- `year`: Erscheinungsjahr
- `citationCount`: Anzahl Zitationen
- `url`: Link zum Paper

### 5. arXiv

**Zugriff:** Atom/RSS Feed API  
**API:** `http://export.arxiv.org/api/query`  
**Lizenz:** Frei zugänglich

**Vorteile:**
- Preprints und aktuelle Forschung
- Volltext verfügbar (PDF)
- Keine API-Keys erforderlich
- Umfassende Metadaten

**Relevante Kategorien für Ethik:**
- `cs.CY` - Computers and Society (Tech-Ethik, AI Ethics)
- `cs.AI` - Artificial Intelligence (AI Ethics)
- `q-bio` - Quantitative Biology (Bioethik)
- `econ` - Economics (Wirtschaftsethik)

**Rate Limits:**
- Max 1 Request pro 3 Sekunden
- Max 5 Anfragen ohne Pause

**Beispielsuche:**
```python
researcher = KnowledgeResearcher(enable_arxiv=True)
context = researcher.research_topic("AI ethics", depth="deep")
```

### 6. PubMed (Optional)

**Zugriff:** E-utilities API  
**API:** `https://eutils.ncbi.nlm.nih.gov/entrez/eutils/`  
**API-Key:** Empfohlen (erhöht Rate-Limits)

**Vorteile:**
- Medizinische und bioethische Literatur
- Abstracts verfügbar
- MeSH-Terme für präzise Suche

**Verwendung:**
```python
researcher = KnowledgeResearcher(enable_pubmed=True)
# Benötigt NCBI API Key in Umgebungsvariable
```

**Relevante Bereiche:**
- Medizinethik
- Bioethik
- Forschungsethik
- Klinische Ethik

## Integration in das Debate-System

### Workflow

1. **News-Artikel wird ausgewählt**
2. **Automatische Wissensrecherche:**
   ```python
   knowledge_researcher = KnowledgeResearcher()
   
   # Thema aus News extrahieren
   topic = extract_main_topic(news_article)
   keywords = extract_keywords(news_article)
   
   # Wissen recherchieren
   knowledge_context = knowledge_researcher.research_topic(
       topic=topic,
       keywords=keywords,
       depth="moderate"  # light, moderate, deep
   )
   ```

3. **LLM-Kontext generieren:**
   ```python
   llm_context = knowledge_context.to_llm_prompt_context()
   ```

4. **In Debate-Prompts einbinden:**
   ```python
   prompt = f"""
   {llm_context}
   
   Als {philosopher_name} analysiere ich die folgende Nachricht:
   {news_article.content}
   
   Meine Position aus {philosophy_school}-Perspektive:
   ...
   """
   ```

### Recherche-Tiefen

**Light (schnell, 2-5 Sekunden):**
- Nur Wikipedia (Intro)
- 1-2 Quellen
- Grundlegende Konzepte

**Moderate (Standard, 5-15 Sekunden):**
- Wikipedia (vollständig)
- SEP Entries (URLs)
- Key Concepts
- Historischer Kontext

**Deep (umfassend, 15-30 Sekunden):**
- Alle Wikipedia-Artikel
- SEP + IEP
- Semantic Scholar (Top 5 Papers)
- arXiv (Top 5 Papers)
- Detaillierte Konzeptanalyse

### Caching

Das System cached Recherchen für 1 Stunde:

```python
researcher = KnowledgeResearcher(cache_duration=3600)

# Erste Anfrage: Recherche durchgeführt (15s)
context1 = researcher.research_topic("Kant")

# Zweite Anfrage: Aus Cache (< 1ms)
context2 = researcher.research_topic("Kant")
```

## API-Keys und Konfiguration

### Empfohlene API-Keys (Optional)

Alle Basis-Features funktionieren ohne API-Keys. Für erweiterte Nutzung:

```bash
# Semantic Scholar (optional, erhöht Rate-Limits)
export SEMANTIC_SCHOLAR_API_KEY="your_key_here"

# PubMed (optional, für medizinische Ethik)
export NCBI_API_KEY="your_key_here"
```

### Konfiguration in Code

```python
researcher = KnowledgeResearcher(
    wikipedia_lang="de",           # Sprache
    enable_arxiv=True,              # arXiv aktivieren
    enable_pubmed=False,            # PubMed (benötigt Key)
    enable_semantic_scholar=True,   # Semantic Scholar
    cache_duration=3600             # Cache 1 Stunde
)
```

## Verwendungsbeispiele

### Beispiel 1: Forschung zu einem Philosophen

```python
from knowledge_researcher import KnowledgeResearcher

researcher = KnowledgeResearcher()

# Recherche zu Kant
context = researcher.search_specific_philosopher("Immanuel Kant")

print(context.to_llm_prompt_context())
# Ausgabe:
# **Wissenskontext zum Thema: Immanuel Kant**
#
# **Schlüsselkonzepte:**
# - Kategorischer Imperativ: Handle nur nach derjenigen Maxime...
# - Autonomie: Die Fähigkeit des Menschen, sich selbst Gesetze zu geben...
# ...
```

### Beispiel 2: Ethisches Konzept

```python
context = researcher.search_ethical_concept("Gerechtigkeit")

print(f"Gefunden: {len(context.sources)} Quellen")
print(f"Philosophische Positionen: {list(context.philosophical_positions.keys())}")
```

### Beispiel 3: Integration in Debate

```python
from debate_chat import DebateChatManager
from knowledge_researcher import KnowledgeResearcher

# Setup
chat_manager = DebateChatManager()
knowledge_researcher = KnowledgeResearcher()

# News-basierte Recherche
news_article = get_current_news()
topic = extract_topic(news_article.title)

# Wissen recherchieren
knowledge = knowledge_researcher.research_topic(
    topic=topic,
    keywords=news_article.ethical_topics,
    depth="moderate"
)

# In LLM-Prompts verwenden
for philosopher in philosophers:
    prompt = generate_prompt_with_knowledge(
        philosopher=philosopher,
        news=news_article,
        knowledge=knowledge
    )
    response = llm_backend.generate(prompt)
```

## Datenstruktur

### KnowledgeSource

```python
@dataclass
class KnowledgeSource:
    id: str                    # Eindeutige ID
    title: str                 # Titel
    content: str               # Hauptinhalt
    source_type: str           # wikipedia, arxiv, sep, etc.
    url: str                   # Link zur Quelle
    author: Optional[str]      # Autor(en)
    published_date: Optional[datetime]  # Erscheinungsdatum
    relevance_score: float     # 0.0 - 1.0
    keywords: List[str]        # Schlagwörter
    citations: int             # Anzahl Zitationen
    summary: str               # Kurzzusammenfassung
```

### KnowledgeContext

```python
@dataclass
class KnowledgeContext:
    topic: str                              # Hauptthema
    sources: List[KnowledgeSource]          # Alle Quellen
    key_concepts: Dict[str, str]            # Konzept -> Definition
    historical_context: str                 # Historischer Hintergrund
    philosophical_positions: Dict[str, str] # Schule -> Position
    relevant_theories: List[str]            # Relevante Theorien
    created_at: datetime                    # Erstellungszeitpunkt
```

## Fehlerbehebung

### Problem: Wikipedia API langsam

**Lösung:** Reduziere Anzahl der Suchergebnisse oder verwende Caching:
```python
researcher = KnowledgeResearcher(cache_duration=7200)  # 2 Stunden
```

### Problem: Semantic Scholar Rate-Limit

**Lösung:** 
1. Verwende API-Key (erhöht Limit auf 1000/5min)
2. Reduziere Recherche-Tiefe auf "moderate"
3. Erhöhe Sleep-Zeit zwischen Requests

### Problem: Keine arXiv Ergebnisse

**Ursache:** arXiv hat wenig klassische Philosophie  
**Lösung:** Fokus auf Tech-Ethik, AI-Ethik, Bioethik für arXiv

### Problem: SEP/IEP nicht verfügbar

**Lösung:** System fällt automatisch auf Wikipedia + Semantic Scholar zurück

## Erweiterungsmöglichkeiten

### Weitere Quellen

1. **JSTOR** (API verfügbar mit institutionellem Zugang)
2. **PhilPapers** (Philosophie-Datenbank)
3. **Google Scholar** (via SerpAPI)
4. **Project MUSE** (Geisteswissenschaften)
5. **SpringerLink** (via API)

### NLP-Integration

Für bessere Konzeptextraktion:
```python
# Zukünftige Erweiterung
from transformers import pipeline

nlp = pipeline("question-answering")
concepts = nlp.extract_concepts(source.content)
```

### Mehrsprachigkeit

```python
# Parallele Recherche in mehreren Sprachen
contexts = []
for lang in ["de", "en", "fr"]:
    researcher = KnowledgeResearcher(wikipedia_lang=lang)
    context = researcher.research_topic(topic)
    contexts.append(context)

# Merge contexts
merged = merge_knowledge_contexts(contexts)
```

## Best Practices

1. **Cache nutzen** - Wiederholte Anfragen vermeiden
2. **Rate-Limits respektieren** - Sleeps zwischen Requests
3. **Tiefe anpassen** - "light" für Tests, "deep" für Produktion
4. **Fehlerbehandlung** - Fallback auf andere Quellen
5. **Relevanz prüfen** - Quellen nach Relevance-Score sortieren
6. **Context begrenzen** - Nur Top 3-5 Quellen für LLM-Kontext

## Lizenz und Nutzungsbedingungen

- **Wikipedia:** CC BY-SA 3.0 - Attribution erforderlich
- **SEP:** Frei zugänglich, Zitation erforderlich
- **Semantic Scholar:** Kostenlos, Rate-Limits beachten
- **arXiv:** Frei, CC-Lizenzen der einzelnen Papers beachten
- **PubMed:** Public Domain (US Gov), kostenlos

## Support und Dokumentation

- Wikipedia API: https://www.mediawiki.org/wiki/API:Main_page
- Semantic Scholar: https://www.semanticscholar.org/product/api
- arXiv API: https://arxiv.org/help/api/
- SEP: https://plato.stanford.edu/
- PubMed: https://www.ncbi.nlm.nih.gov/home/develop/api/
