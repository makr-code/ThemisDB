# FEM-Metadaten aus Dokumenteninhalten ableiten

**Version:** 1.0.0  
**Datum:** 7. Dezember 2025  
**Status:** Spezifikation & Implementierungsanleitung

---

## 1. Übersicht

Dieses Dokument beschreibt, wie FEM-Metadaten (Gewichte, Dämpfung, Steifigkeit) **automatisch aus Dokumenteninhalten** abgeleitet werden können, anstatt sie manuell zu setzen oder nur aus Edge-Typen zu inferieren.

**Kernprinzip:** Der **Inhalt** und die **Struktur** eines Dokuments bestimmen dessen FEM-Eigenschaften.

---

## 2. Content-Based Metadata Extraction

### 2.1 Übersicht der Analyseebenen

```
Dokumenteninhalt
    ↓
┌─────────────────────────────────────────────────┐
│ Level 1: Textanalyse                            │
│ - Keywords & Terminologie                       │
│ - Sentiment & Tonalität                         │
│ - Komplexität & Fachsprache                     │
└─────────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────────┐
│ Level 2: Strukturanalyse                        │
│ - Dokumenttyp-Erkennung                         │
│ - Abschnittsstruktur                            │
│ - Code/Daten-Verhältnis                         │
└─────────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────────┐
│ Level 3: Referenz-Analyse                       │
│ - Link-Dichte & -Qualität                       │
│ - Import/Include-Statements                     │
│ - Zitat-Analyse                                 │
└─────────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────────┐
│ Level 4: Semantische Analyse                    │
│ - Topic Modeling                                │
│ - Konzept-Extraktion                            │
│ - Domänen-Klassifikation                        │
└─────────────────────────────────────────────────┘
    ↓
FEM-Metadaten (weight, damping, stiffness, ...)
```

---

## 3. Document-to-FEM Mapping Strategien

### 3.1 Node-Metadaten aus Dokumenteninhalt

#### 3.1.1 Inertia (Trägheit) - Widerstand gegen Änderung

**Formel:**
```
inertia = f(stability_score, formality, complexity)

stability_score = 1 - (change_frequency / max_change_frequency)
formality = detect_formality(content)
complexity = calculate_complexity(content)

inertia = 0.3 × stability_score + 0.4 × formality + 0.3 × complexity
```

**Beispiel-Implementierung:**

```python
def calculate_inertia(document: Dict) -> float:
    """
    Berechne Trägheit aus Dokumenteninhalt
    
    Hohe Trägheit = Dokument ändert sich selten, formale Sprache, hohe Komplexität
    Niedrige Trägheit = Häufige Änderungen, informale Sprache, geringe Komplexität
    """
    content = document['content']
    
    # 1. Stabilität aus Änderungshistorie
    changes = get_change_history(document['_id'])
    days_since_creation = (datetime.now() - document['created_at']).days
    change_frequency = len(changes) / max(days_since_creation, 1)
    stability_score = 1.0 / (1.0 + change_frequency)  # Sigmoid-ähnlich
    
    # 2. Formalität aus Textanalyse
    formality_indicators = {
        'passive_voice_ratio': detect_passive_voice(content),
        'avg_sentence_length': calculate_avg_sentence_length(content),
        'technical_terms_ratio': count_technical_terms(content) / count_total_words(content),
        'citation_count': count_citations(content),
        'section_structure_score': analyze_section_structure(content)
    }
    
    formality = (
        0.2 * formality_indicators['passive_voice_ratio'] +
        0.2 * min(formality_indicators['avg_sentence_length'] / 30.0, 1.0) +
        0.3 * formality_indicators['technical_terms_ratio'] +
        0.1 * min(formality_indicators['citation_count'] / 10.0, 1.0) +
        0.2 * formality_indicators['section_structure_score']
    )
    
    # 3. Komplexität aus verschiedenen Metriken
    complexity_metrics = {
        'lexical_diversity': calculate_lexical_diversity(content),  # TTR
        'readability': 1.0 - (calculate_flesch_score(content) / 100.0),
        'code_complexity': calculate_code_complexity(content) if has_code(content) else 0.5,
        'concept_density': count_unique_concepts(content) / count_paragraphs(content)
    }
    
    complexity = sum(complexity_metrics.values()) / len(complexity_metrics)
    
    # Kombiniere zu Inertia
    inertia = (
        0.3 * stability_score +
        0.4 * formality +
        0.3 * complexity
    )
    
    return clip(inertia, 0.0, 1.0)


# Beispiel-Ergebnisse:
# - Legal Contract: inertia = 0.85 (selten geändert, formal, komplex)
# - README.md: inertia = 0.35 (oft geändert, informal, einfach)
# - API Spec: inertia = 0.70 (mittel stabil, formal, strukturiert)
# - Meeting Notes: inertia = 0.15 (häufig, informal, einfach)
```

#### 3.1.2 Change Amplification - Verstärkungsfaktor

**Formel:**
```
change_amplification = f(criticality_keywords, dependency_density, scope)

criticality_score = detect_critical_keywords(content)
dependency_density = count_dependencies / document_size
scope_score = detect_scope_indicators(content)

change_amplification = 0.5 + criticality_score × dependency_density × scope_score
```

**Implementierung:**

```python
def calculate_change_amplification(document: Dict) -> float:
    """
    Verstärkungsfaktor: Wie stark wirken sich Änderungen aus?
    
    > 1.0 = Änderungen werden verstärkt (kritische Dokumente)
    = 1.0 = Neutral
    < 1.0 = Änderungen werden gedämpft
    """
    content = document['content']
    
    # 1. Kritikalitäts-Keywords erkennen
    critical_keywords = [
        'MUST', 'SHALL', 'REQUIRED', 'CRITICAL', 'BREAKING',
        'deprecated', 'security', 'authentication', 'authorization',
        'payment', 'transaction', 'data loss', 'compliance'
    ]
    
    critical_keyword_count = sum(
        content.lower().count(kw.lower()) for kw in critical_keywords
    )
    total_words = count_words(content)
    criticality_score = min(critical_keyword_count / (total_words * 0.01), 1.0)
    
    # 2. Dependency Density aus Referenzen
    dependencies = extract_dependencies(content)  # Links, imports, references
    dependency_density = len(dependencies) / max(count_paragraphs(content), 1)
    dependency_density = min(dependency_density / 3.0, 1.0)  # Normalisieren
    
    # 3. Scope aus Dokumentenstruktur
    scope_indicators = {
        'affects_multiple_systems': detect_system_mentions(content) > 2,
        'global_configuration': 'global' in content.lower() or 'config' in document['_id'],
        'interface_definition': document['type'] in ['API', 'Interface', 'Contract'],
        'wide_audience': estimate_audience_size(document) > 100
    }
    scope_score = sum(scope_indicators.values()) / len(scope_indicators)
    
    # Kombiniere zu Amplification Factor
    base = 0.5  # Basis-Dämpfung
    amplification = base + (criticality_score * 0.3 + dependency_density * 0.4 + scope_score * 0.3)
    
    return clip(amplification, 0.1, 2.0)


# Beispiel-Ergebnisse:
# - Global Security Policy: 1.8 (hohe Verstärkung)
# - API Breaking Change Doc: 1.6
# - Feature Implementation: 1.0 (neutral)
# - Code Comment: 0.3 (starke Dämpfung)
```

#### 3.1.3 Impact Radius - Maximale Propagierungsdistanz

**Formel:**
```
impact_radius = f(document_centrality, reference_breadth, importance)

centrality = calculate_graph_centrality(document_id)
reference_breadth = unique_reference_targets / total_references
importance = detect_importance_markers(content)

impact_radius = base_radius × (centrality + reference_breadth + importance) / 3
```

**Implementierung:**

```python
def calculate_impact_radius(document: Dict, graph_context: Dict) -> int:
    """
    Berechne maximale Impact-Propagierungsdistanz
    
    Hoch = Zentrale Dokumente mit breiter Referenz-Basis
    Niedrig = Periphere Dokumente mit engen Referenzen
    """
    doc_id = document['_id']
    content = document['content']
    
    # 1. Graph Centrality (aus Graph-Analyse)
    # Betweenness, Closeness, PageRank
    centrality_metrics = calculate_centrality(doc_id, graph_context)
    centrality_score = (
        0.4 * centrality_metrics['betweenness'] +
        0.3 * centrality_metrics['closeness'] +
        0.3 * centrality_metrics['pagerank']
    )
    
    # 2. Reference Breadth (Diversität der Referenzen)
    references = extract_all_references(content)
    reference_domains = set(get_domain(ref) for ref in references)
    reference_breadth = len(reference_domains) / max(len(references), 1)
    
    # 3. Importance Markers aus Inhalt
    importance_markers = {
        'in_title_path': 'README' in doc_id or 'index' in doc_id,
        'specification_doc': document.get('type') == 'specification',
        'high_view_count': document.get('views', 0) > 1000,
        'many_backlinks': len(get_backlinks(doc_id)) > 20,
        'official_doc': 'official' in document.get('tags', [])
    }
    importance = sum(importance_markers.values()) / len(importance_markers)
    
    # Kombiniere zu Impact Radius
    base_radius = 5  # Default
    combined_score = (centrality_score + reference_breadth + importance) / 3
    
    impact_radius = int(base_radius * (0.5 + combined_score * 1.5))
    
    return clip(impact_radius, 1, 20)


# Beispiel-Ergebnisse:
# - Main README: radius = 15 (sehr zentral)
# - API Specification: radius = 12
# - Implementation File: radius = 5
# - Test File: radius = 2
```

---

### 3.2 Edge-Metadaten aus Dokumenteninhalten

#### 3.2.1 Edge Weight aus Referenz-Kontext

**Strategie:** Analysiere **wie** ein Dokument auf ein anderes referenziert

```python
def calculate_edge_weight_from_content(
    from_doc: Dict, 
    to_doc: Dict, 
    reference_context: str
) -> float:
    """
    Berechne Edge Weight basierend auf Referenz-Kontext
    
    Faktoren:
    - Wie wird referenziert? (MUST USE vs. "see also")
    - Wo wird referenziert? (Introduction vs. Appendix)
    - Wie oft wird referenziert?
    - In welchem Kontext? (Critical path vs. optional)
    """
    content = from_doc['content']
    to_doc_id = to_doc['_id']
    
    # 1. Analyse aller Referenzen auf to_doc
    references = find_all_references_to(content, to_doc_id)
    
    if not references:
        return 0.5  # Default wenn keine explizite Referenz
    
    weights = []
    
    for ref in references:
        ref_weight = 0.5  # Base weight
        
        # Kontext-Analyse um die Referenz herum
        context = get_surrounding_text(content, ref['position'], window=200)
        
        # 2. Imperativ-Level (MUST, SHOULD, MAY)
        if any(kw in context.upper() for kw in ['MUST', 'SHALL', 'REQUIRED']):
            ref_weight += 0.4
        elif any(kw in context.upper() for kw in ['SHOULD', 'RECOMMENDED']):
            ref_weight += 0.2
        elif any(kw in context.upper() for kw in ['MAY', 'OPTIONAL', 'CAN']):
            ref_weight -= 0.2
        
        # 3. Position im Dokument
        position_score = 1.0 - (ref['position'] / len(content))  # Früher = wichtiger
        ref_weight += 0.2 * position_score
        
        # 4. Kontext-Keywords
        critical_context = any(kw in context.lower() for kw in [
            'critical', 'important', 'essential', 'core', 'fundamental',
            'depends on', 'requires', 'prerequisite'
        ])
        if critical_context:
            ref_weight += 0.3
        
        optional_context = any(kw in context.lower() for kw in [
            'optional', 'alternative', 'see also', 'for reference',
            'if needed', 'advanced'
        ])
        if optional_context:
            ref_weight -= 0.3
        
        # 5. Link-Typ
        link_type = detect_link_type(ref)
        type_modifiers = {
            'code_import': 0.4,      # import statement = strong
            'inline_reference': 0.2,  # inline link = medium
            'see_also': -0.2,        # see also = weak
            'footnote': -0.1         # footnote = weak
        }
        ref_weight += type_modifiers.get(link_type, 0.0)
        
        weights.append(clip(ref_weight, 0.0, 1.0))
    
    # Durchschnitt aller Referenzen, aber höchste Referenz zählt mehr
    max_weight = max(weights)
    avg_weight = sum(weights) / len(weights)
    
    final_weight = 0.6 * max_weight + 0.4 * avg_weight
    
    return clip(final_weight, 0.0, 1.0)


# Beispiele:

# Example 1: Code Import
"""
import payment_processor  # CRITICAL: Required for all transactions
"""
# → weight = 0.9 (CRITICAL + code import + early position)

# Example 2: Optional Reference
"""
For advanced use cases, you may consult the detailed guide.
See also: [Advanced Configuration Guide](...)
"""
# → weight = 0.3 (MAY + "see also" + optional)

# Example 3: Specification Reference
"""
This implementation MUST comply with the Security Policy (see docs/security/policy.md).
All authentication requests SHALL follow the protocol defined in...
"""
# → weight = 0.95 (MUST + SHALL + critical context)
```

#### 3.2.2 Damping Coefficient aus Dokumenten-Ähnlichkeit

**Konzept:** Ähnliche Dokumente dämpfen weniger (enger gekoppelt)

```python
def calculate_damping_from_similarity(from_doc: Dict, to_doc: Dict) -> float:
    """
    Berechne Dämpfung basierend auf Dokument-Ähnlichkeit
    
    Hohe Ähnlichkeit → Niedrige Dämpfung → Starke Propagierung
    Niedrige Ähnlichkeit → Hohe Dämpfung → Schwache Propagierung
    """
    
    # 1. Textuelle Ähnlichkeit (TF-IDF, Embeddings)
    text_similarity = calculate_text_similarity(
        from_doc['content'], 
        to_doc['content']
    )
    
    # 2. Strukturelle Ähnlichkeit
    structural_similarity = compare_document_structure(from_doc, to_doc)
    
    # 3. Terminologie-Überlappung
    from_terms = extract_technical_terms(from_doc['content'])
    to_terms = extract_technical_terms(to_doc['content'])
    term_overlap = len(from_terms & to_terms) / len(from_terms | to_terms)
    
    # 4. Typ-Ähnlichkeit
    type_similarity = 1.0 if from_doc['type'] == to_doc['type'] else 0.5
    
    # Kombiniere zu Similarity Score
    similarity = (
        0.4 * text_similarity +
        0.2 * structural_similarity +
        0.3 * term_overlap +
        0.1 * type_similarity
    )
    
    # Invertiere für Dämpfung
    # Hohe Ähnlichkeit → Niedrige Dämpfung
    damping = 1.0 - similarity
    
    return clip(damping, 0.05, 0.95)


# Beispiele:
# - API Spec → Implementation: similarity=0.7 → damping=0.3 (niedrig)
# - Code → Tests: similarity=0.8 → damping=0.2 (sehr niedrig)
# - Spec → Marketing Doc: similarity=0.2 → damping=0.8 (hoch)
```

#### 3.2.3 Bidirectional Factor aus Referenz-Symmetrie

```python
def calculate_bidirectional_factor(doc_a: Dict, doc_b: Dict) -> float:
    """
    Wie stark propagiert Impact auch rückwärts?
    
    Basiert auf:
    - Gegenseitige Referenzen
    - Typ-Beziehung (Parent-Child vs. Peer)
    - Semantische Nähe
    """
    
    # 1. Gegenseitige Referenzen
    a_refs_b = references_exist(doc_a, doc_b)
    b_refs_a = references_exist(doc_b, doc_a)
    
    if a_refs_b and b_refs_a:
        mutual_reference_score = 0.8  # Stark bidirektional
    elif a_refs_b or b_refs_a:
        mutual_reference_score = 0.3  # Schwach bidirektional
    else:
        mutual_reference_score = 0.0  # Keine direkte Referenz
    
    # 2. Typ-Beziehung
    type_relationships = {
        ('specification', 'implementation'): 0.3,  # Spec → Impl stark, Impl → Spec schwach
        ('parent', 'child'): 0.6,                  # Bidirektional aber asymmetrisch
        ('peer', 'peer'): 0.9,                     # Gleichberechtigt
        ('test', 'code'): 0.2,                     # Tests → Code schwach
        ('documentation', 'code'): 0.1             # Docs → Code sehr schwach
    }
    
    type_a = doc_a.get('type', 'unknown')
    type_b = doc_b.get('type', 'unknown')
    type_score = type_relationships.get((type_a, type_b), 0.5)
    
    # 3. Semantische Symmetrie
    semantic_similarity = calculate_text_similarity(doc_a['content'], doc_b['content'])
    
    # Kombiniere
    bidirectional_factor = (
        0.4 * mutual_reference_score +
        0.3 * type_score +
        0.3 * semantic_similarity
    )
    
    return clip(bidirectional_factor, 0.0, 1.0)
```

---

## 4. Vollständige Beispiel-Pipeline

### 4.1 Content Ingestion mit FEM Metadata Extraction

```python
class ContentIngestionWithFEMMetadata:
    
    def ingest_document(self, file_path: str, metadata: Dict) -> str:
        """
        Vollständige Ingestion mit automatischer FEM-Metadaten-Extraktion
        """
        
        # 1. Dokument einlesen
        content = read_file(file_path)
        doc = {
            '_id': generate_id(file_path),
            'content': content,
            'type': detect_document_type(content, file_path),
            'created_at': datetime.now(),
            **metadata
        }
        
        # 2. FEM Node-Metadaten aus Inhalt extrahieren
        node_metadata = self.extract_node_metadata(doc)
        doc['fem_metadata'] = node_metadata
        
        # 3. Dokument speichern
        doc_id = self.db.insert(doc)
        
        # 4. Referenzen extrahieren und Edges mit FEM-Metadaten erstellen
        references = self.extract_references(content, doc_id)
        for ref in references:
            edge_metadata = self.extract_edge_metadata(doc, ref)
            self.create_edge(doc_id, ref['target_id'], ref['type'], edge_metadata)
        
        return doc_id
    
    def extract_node_metadata(self, doc: Dict) -> Dict:
        """Extrahiere alle FEM Node-Metadaten"""
        
        return {
            'inertia': calculate_inertia(doc),
            'elasticity': calculate_elasticity(doc),
            'stability': calculate_stability(doc),
            'change_amplification': calculate_change_amplification(doc),
            'impact_radius': calculate_impact_radius(doc, self.graph_context),
            'criticality': determine_criticality(doc),
            
            # Zusätzliche Content-basierte Metriken
            'content_metrics': {
                'word_count': count_words(doc['content']),
                'complexity': calculate_complexity(doc['content']),
                'formality': detect_formality(doc['content']),
                'technical_density': calculate_technical_density(doc['content']),
                'code_ratio': calculate_code_ratio(doc['content'])
            }
        }
    
    def extract_edge_metadata(self, from_doc: Dict, reference: Dict) -> Dict:
        """Extrahiere FEM Edge-Metadaten"""
        
        to_doc = self.db.get(reference['target_id'])
        
        return {
            '_weight': calculate_edge_weight_from_content(
                from_doc, 
                to_doc, 
                reference['context']
            ),
            'damping_coefficient': calculate_damping_from_similarity(from_doc, to_doc),
            'material_stiffness': calculate_material_stiffness(
                reference['type'],
                from_doc['type'],
                to_doc['type']
            ),
            'bidirectional_factor': calculate_bidirectional_factor(from_doc, to_doc),
            'change_sensitivity': estimate_change_sensitivity(to_doc),
            
            # Kontext-Informationen
            'reference_context': {
                'position_in_doc': reference['position'] / len(from_doc['content']),
                'reference_type': reference['type'],
                'imperative_level': detect_imperative_level(reference['context']),
                'keyword_indicators': extract_keywords(reference['context'])
            }
        }
```

### 4.2 Konkrete Code-Beispiele

#### Beispiel 1: Python Import Statement

```python
# Dokument A: services/payment/processor.py
"""
Payment processing service.

CRITICAL: This module depends on the fraud detection service.
All transactions MUST be validated before processing.
"""

from fraud_detection import validate_transaction  # REQUIRED
from utils.logging import log_transaction  # Optional helper


def process_payment(transaction):
    # MUST validate first
    if not validate_transaction(transaction):
        raise SecurityError("Fraud detected")
    
    # ... process payment
```

**Automatisch extrahierte FEM-Metadaten:**

```json
{
  "node_metadata": {
    "inertia": 0.65,
    "change_amplification": 1.4,
    "impact_radius": 8,
    "criticality": "high",
    "content_metrics": {
      "word_count": 87,
      "complexity": 0.72,
      "code_ratio": 0.65,
      "technical_density": 0.88
    }
  },
  
  "edges": [
    {
      "from": "services/payment/processor.py",
      "to": "fraud_detection/__init__.py",
      "type": "IMPORTS",
      "fem_metadata": {
        "_weight": 0.92,
        "damping_coefficient": 0.08,
        "material_stiffness": 0.90,
        "bidirectional_factor": 0.25,
        "criticality": "critical",
        "reference_context": {
          "imperative_level": "MUST",
          "keywords": ["CRITICAL", "REQUIRED", "validate"],
          "position_in_doc": 0.15
        }
      }
    },
    {
      "from": "services/payment/processor.py",
      "to": "utils/logging.py",
      "type": "IMPORTS",
      "fem_metadata": {
        "_weight": 0.35,
        "damping_coefficient": 0.55,
        "material_stiffness": 0.40,
        "bidirectional_factor": 0.05,
        "criticality": "low",
        "reference_context": {
          "imperative_level": "OPTIONAL",
          "keywords": ["helper"],
          "position_in_doc": 0.18
        }
      }
    }
  ]
}
```

#### Beispiel 2: Markdown Documentation mit Referenzen

```markdown
<!-- docs/api/authentication.md -->

# Authentication API

## Overview

This document specifies the authentication protocol for all API endpoints.

**IMPORTANT**: All implementations MUST comply with the Security Policy 
defined in [docs/security/policy.md](../security/policy.md).

## Implementation

See the reference implementation in `services/auth/oauth_handler.py`.

For testing, you may consult the test suite documentation.

## Related Documents

- [Security Best Practices](../security/best-practices.md) (recommended)
- [API Rate Limiting](./rate-limiting.md) (see also)
```

**Extrahierte FEM-Metadaten:**

```json
{
  "node_metadata": {
    "inertia": 0.75,
    "change_amplification": 1.6,
    "impact_radius": 12,
    "criticality": "critical",
    "content_metrics": {
      "formality": 0.82,
      "technical_density": 0.68,
      "section_structure_score": 0.90
    }
  },
  
  "edges": [
    {
      "to": "docs/security/policy.md",
      "weight": 0.95,
      "reason": "MUST comply, IMPORTANT marker, early position"
    },
    {
      "to": "services/auth/oauth_handler.py",
      "weight": 0.75,
      "reason": "Implementation reference, mid position"
    },
    {
      "to": "docs/security/best-practices.md",
      "weight": 0.45,
      "reason": "recommended (weaker than MUST)"
    },
    {
      "to": "./rate-limiting.md",
      "weight": 0.30,
      "reason": "see also (weakest indicator)"
    }
  ]
}
```

---

## 5. Machine Learning Ansätze

### 5.1 Supervised Learning für Weight Prediction

```python
class FEMWeightPredictor:
    """
    ML-Modell zur Vorhersage von Edge Weights aus Dokumenteninhalten
    """
    
    def __init__(self):
        self.model = GradientBoostingRegressor()
        self.vectorizer = TfidfVectorizer()
    
    def extract_features(self, from_doc: Dict, to_doc: Dict, ref_context: str):
        """Extrahiere Features für ML-Modell"""
        
        features = {}
        
        # Text-Features
        features['text_similarity'] = cosine_similarity(
            self.vectorizer.transform([from_doc['content']]),
            self.vectorizer.transform([to_doc['content']])
        )[0][0]
        
        # Kontext-Features
        features['imperative_must'] = 'MUST' in ref_context.upper()
        features['imperative_should'] = 'SHOULD' in ref_context.upper()
        features['imperative_may'] = 'MAY' in ref_context.upper()
        features['critical_keyword'] = any(kw in ref_context.lower() 
                                           for kw in ['critical', 'important'])
        features['optional_keyword'] = any(kw in ref_context.lower() 
                                          for kw in ['optional', 'see also'])
        
        # Positions-Features
        features['position_ratio'] = find_position(from_doc, to_doc) / len(from_doc['content'])
        
        # Typ-Features (One-Hot Encoding)
        from_type = from_doc.get('type', 'unknown')
        to_type = to_doc.get('type', 'unknown')
        features[f'from_type_{from_type}'] = 1
        features[f'to_type_{to_type}'] = 1
        
        # Strukturelle Features
        features['from_complexity'] = calculate_complexity(from_doc['content'])
        features['to_complexity'] = calculate_complexity(to_doc['content'])
        features['term_overlap'] = calculate_term_overlap(from_doc, to_doc)
        
        return pd.Series(features)
    
    def train(self, training_data: List[Tuple]):
        """
        Trainiere Modell mit historischen Daten
        
        training_data: [(from_doc, to_doc, ref_context, ground_truth_weight), ...]
        """
        
        X = []
        y = []
        
        for from_doc, to_doc, ref_context, true_weight in training_data:
            features = self.extract_features(from_doc, to_doc, ref_context)
            X.append(features)
            y.append(true_weight)
        
        X_df = pd.DataFrame(X)
        self.model.fit(X_df, y)
    
    def predict_weight(self, from_doc: Dict, to_doc: Dict, ref_context: str) -> float:
        """Vorhersage des Edge Weight"""
        
        features = self.extract_features(from_doc, to_doc, ref_context)
        weight = self.model.predict(pd.DataFrame([features]))[0]
        return clip(weight, 0.0, 1.0)
```

### 5.2 Training Data Collection

```python
def collect_training_data():
    """
    Sammle Trainings-Daten aus beobachteten Impact-Propagierungen
    """
    
    training_samples = []
    
    # Für jede historische Änderung
    for change in historical_changes:
        source_doc = get_document(change['document_id'])
        
        # Beobachte tatsächliche Impact-Propagierung
        actual_impacts = observe_actual_impacts(change, days=30)
        
        # Für jedes betroffene Dokument
        for target_id, observed_impact in actual_impacts.items():
            target_doc = get_document(target_id)
            edge = get_edge(change['document_id'], target_id)
            
            if edge:
                ref_context = extract_reference_context(source_doc, target_doc)
                
                # Ground Truth Weight aus beobachtetem Impact
                # Hoher Impact → Hoher Weight
                ground_truth_weight = observed_impact / change['magnitude']
                
                training_samples.append((
                    source_doc,
                    target_doc,
                    ref_context,
                    ground_truth_weight
                ))
    
    return training_samples
```

---

## 6. Regelbasierte Heuristiken (Fallback)

Wenn ML-Modelle nicht verfügbar sind, verwende regelbasierte Heuristiken:

```yaml
# config/fem_content_rules.yaml

content_rules:
  # Keywords → Weight Modifiers
  imperative_keywords:
    MUST:
      weight_modifier: +0.4
      damping_modifier: -0.3
    SHALL:
      weight_modifier: +0.4
      damping_modifier: -0.3
    REQUIRED:
      weight_modifier: +0.3
      damping_modifier: -0.2
    SHOULD:
      weight_modifier: +0.2
      damping_modifier: -0.1
    RECOMMENDED:
      weight_modifier: +0.1
      damping_modifier: 0.0
    MAY:
      weight_modifier: -0.2
      damping_modifier: +0.2
    OPTIONAL:
      weight_modifier: -0.3
      damping_modifier: +0.3
  
  # Kontext-Keywords
  critical_context:
    keywords: [critical, important, essential, core, fundamental]
    weight_modifier: +0.3
    criticality: high
  
  optional_context:
    keywords: [optional, see also, for reference, advanced, if needed]
    weight_modifier: -0.3
    criticality: low
  
  # Link-Typen
  link_types:
    code_import:
      base_weight: 0.8
      examples: [import, require, include, use]
    
    must_reference:
      base_weight: 0.9
      examples: [must comply, shall follow, required by]
    
    see_also:
      base_weight: 0.3
      examples: [see also, for more info, related]
    
    footnote:
      base_weight: 0.2
      examples: [^, footnote]
  
  # Position im Dokument
  position_weight:
    early: 1.2  # Erste 20%
    middle: 1.0  # 20-80%
    late: 0.8    # Letzte 20%
  
  # Dokumenttyp-spezifische Regeln
  document_types:
    specification:
      base_inertia: 0.7
      base_amplification: 1.5
      base_impact_radius: 10
    
    implementation:
      base_inertia: 0.5
      base_amplification: 1.0
      base_impact_radius: 5
    
    test:
      base_inertia: 0.8
      base_amplification: 0.5
      base_impact_radius: 2
    
    documentation:
      base_inertia: 0.6
      base_amplification: 0.8
      base_impact_radius: 6
```

---

## 7. Implementierungs-Roadmap

### Phase 1: Regelbasierte Extraktion (4 Wochen)
- [ ] Keyword-Detection implementieren
- [ ] Referenz-Kontext-Analyse
- [ ] Position-basierte Gewichtung
- [ ] Dokumenttyp-Erkennung
- [ ] Integration in Ingestion-Pipeline

### Phase 2: Content-Analyse (6 Wochen)
- [ ] NLP-Pipeline (spaCy, NLTK)
- [ ] Komplexitäts-Metriken
- [ ] Formalitäts-Detektion
- [ ] Terminologie-Extraktion
- [ ] Sentiment-Analyse

### Phase 3: ML-basierte Optimierung (8 Wochen)
- [ ] Training Data Collection
- [ ] Feature Engineering
- [ ] Model Training (Gradient Boosting)
- [ ] Model Evaluation
- [ ] A/B Testing gegen regelbasiert

### Phase 4: Continuous Learning (Ongoing)
- [ ] Feedback Loop implementieren
- [ ] Online Learning
- [ ] Model Re-training Pipeline
- [ ] Performance Monitoring

---

## 8. Validierung & Qualitätssicherung

```python
def validate_fem_metadata_quality(doc_id: str):
    """
    Validiere Qualität der extrahierten FEM-Metadaten
    """
    
    doc = get_document(doc_id)
    fem_metadata = doc['fem_metadata']
    edges = get_outgoing_edges(doc_id)
    
    checks = {
        # 1. Range Checks
        'inertia_in_range': 0.0 <= fem_metadata['inertia'] <= 1.0,
        'amplification_in_range': 0.1 <= fem_metadata['change_amplification'] <= 2.0,
        
        # 2. Consistency Checks
        'high_inertia_low_amplification': not (
            fem_metadata['inertia'] > 0.8 and 
            fem_metadata['change_amplification'] > 1.5
        ),
        
        # 3. Edge Consistency
        'critical_edges_high_weight': all(
            edge['fem_metadata']['_weight'] > 0.7 
            for edge in edges 
            if edge['fem_metadata'].get('criticality') == 'critical'
        ),
        
        # 4. Plausibility
        'radius_matches_centrality': (
            fem_metadata['impact_radius'] > 8 and 
            calculate_centrality(doc_id) > 0.7
        ) or (
            fem_metadata['impact_radius'] <= 8
        )
    }
    
    return all(checks.values()), checks
```

---

## 9. Zusammenfassung

### Kernprinzipien:

1. **Content is King**: Dokumenteninhalt bestimmt FEM-Eigenschaften
2. **Context Matters**: Wie referenziert wird ist wichtiger als dass referenziert wird
3. **Multi-Layer Approach**: Kombiniere Text, Struktur, Semantik
4. **Continuous Learning**: Verbessere Modelle aus Beobachtungen

### Vorteile:

- ✅ Automatische Metadaten-Generierung
- ✅ Keine manuelle Konfiguration pro Edge
- ✅ Selbst-korrigierend durch Feedback
- ✅ Skaliert mit Dokumentenwachstum

### Nächste Schritte:

1. Implementiere regelbasierte Extraktion
2. Sammle Training Data
3. Trainiere ML-Modelle
4. A/B Testing
5. Production Rollout

---

**Erstellt:** 7. Dezember 2025  
**Version:** 1.0.0  
**Autor:** ThemisDB Enterprise Team
