# FUTURE_PLAN.md

# ThemisDB Future Plan
## Hybrid Knowledge Retrieval Architecture
### ANN Frontdoor · Tensor Mid-Layer · Graph Truth Layer · LLM/LoRA Final Layer

**Status:** Draft  
**Stand:** 2026-06-01  
**Zweck:** Langfristige Zielarchitektur und Evolutionspfad für ThemisDB im Bereich RAG, Graph, Tensor, Vektor, LLM und LoRA

---

## 1. Executive Summary

ThemisDB entwickelt sich von einer modularen Hybrid-RAG- und Multi-Model-Architektur zu einer **mehrschichtigen Knowledge Retrieval Architecture** weiter.

Das Ziel ist nicht nur, Dokumente oder Chunks semantisch ähnlich zu finden, sondern Wissen über mehrere Ebenen strukturiert, komprimiert, verteilbar und auditierbar zugänglich zu machen.

### Zielschichtung

1. **HNSW / DiskANN als ANN-Frontdoor**  
   Semantische Kandidatenfindung mit niedriger Latenz und hoher Skalierbarkeit.

2. **Tensorlayer als Mid-Layer für Compression / Routing / Summary**  
   Strukturverdichtung, Similarity, Routing, Candidate Compression und Wissenssummaries.

3. **Graphlayer für Constraints / Evidence / Provenance**  
   Exakte relationale Wahrheitsschicht für Evidenz, Berechtigungen, Herkunft und Verifikation.

4. **LLM / LoRA als Final-Layer**  
   Wissensgebundene Antwortgenerierung und Domänenadaptation über strukturierte, auditierbare Kontexte.

---

## 2. Motivation

Klassisches RAG folgt oft dem Schema:

`query -> embedding -> top-k retrieval -> prompt -> answer`

Dieses Muster ist für einfache semantische Suche ausreichend, stößt jedoch an Grenzen bei:

- Multi-Hop-Wissensräumen
- Provenance- und Vertrauenslogik
- verteilten Instanzen
- domänenspezifischen Adaptern
- redundanten Kontexten
- wachsender Datenmenge
- regulatorischer Nachvollziehbarkeit

ThemisDB besitzt bereits starke Bausteine in den Bereichen:

- Graph
- Vektor-/Embedding-Suche
- Tensor-nahe Datenstrukturen
- LLM/LoRA
- Training/Provenance
- Sharding/Distribution

Diese Fähigkeiten sollen in einer langfristig konsistenten Zielarchitektur zusammengeführt werden.

---

## 3. Zielbild

### 3.1 Leitidee

ThemisDB soll langfristig eine **wissenszentrierte, mehrschichtige Retrieval- und Reasoning-Plattform** werden.

Nicht mehr nur:
- Dokumente finden,
- Chunks ranken,
- Prompts füllen,

sondern:
- relevante Wissensräume identifizieren,
- strukturierte Wissensschnitte verdichten,
- Evidenz und Provenance absichern,
- domänenspezifische Generierung auf belastbarer Grundlage ausführen.

---

## 4. Ist-Zustand in Kurzform

Der aktuelle Systemzustand lässt sich wie folgt beschreiben:

### Vorhanden
- Vektororientierte Retrieval-Logik
- Graphbasierte Beziehungen und Constraints
- LLM-/LoRA-/Training-Subsysteme
- Tensornahe Komponenten und Adapter-/Speicherlogik
- Sharding- und Distributionsfähigkeiten

### Teilweise vorhanden
- RAG-Qualitäts- und Gap-Detection
- LoRA-/Adapter-Provenance
- modell- und adapterbezogene Registry-Strukturen
- tensorische Spezialkomponenten mit Beschleunigungspotential

### Fehlend oder nicht vollständig integriert
- klare ANN-Frontdoor-Schicht
- definierter Tensor-Mid-Layer
- packagebasierter Adapterlebenszyklus
- systematischer Modellwechsel-Workflow
- federierte tensorische Shard-Summaries
- geschichtete Orchestrierung über ANN -> Tensor -> Graph -> LLM

---

## 5. Langfristige Architektur

---

## 5.1 ANN Frontdoor
### HNSW / DiskANN als erste Retrieval-Schicht

### Ziel
HNSW und DiskANN bilden die erste operative Schicht zur semantischen Kandidatenfindung.

### Aufgaben
- schnelle nearest-neighbor Suche
- top-k Kandidatenbildung
- Hot-/Cold-Daten-Trennung
- shard-lokale und globale Kandidatenfindung
- Retrieval über:
  - Dokumente
  - Chunks
  - Entitäten
  - Adapter
  - Modelle
  - Shard-Summaries

### Leitprinzip
ANN beantwortet nicht die endgültige Wissensfrage, sondern liefert:
> „Welche Kandidaten sind relevant genug für weitere strukturierte Verarbeitung?“

### Zielvorteile
- geringe Latenz
- gute Skalierbarkeit
- klare Frontdoor für RAG und Knowledge Retrieval
- Wiederverwendung der ANN-Logik auch für Artefakte wie Adapter und Packages

---

## 5.2 Tensor Mid-Layer
### Compression · Routing · Summary

### Ziel
Tensoren werden als strukturierte Verdichtungs- und Beschleunigungsschicht zwischen ANN und Graph genutzt.

### Aufgaben
- Kandidatenkompression
- Redundanzreduktion
- Routing
- Similarity Fingerprinting
- Wissenssummaries
- relationale Approximation
- Shard-Relevanzbewertung
- Adapter-/Package-Ähnlichkeit
- Speicherkompression großer Wissensräume

### Prinzip
Tensoren ersetzen weder ANN noch Graph.  
Sie fungieren als:
> „Mid-Layer zur Strukturverdichtung und Relevanzsteuerung.“

### Zielvorteile
- kleinere Kandidatenmengen
- weniger Tokenbudget-Verbrauch
- effizientere Kontextbildung
- weniger unnötige Cross-shard-Fan-outs
- bessere strukturelle Wissensabdeckung

---

## 5.3 Graph Truth Layer
### Constraints · Evidence · Provenance

### Ziel
Der Graphlayer bleibt die exakte Wahrheitsschicht für Semantik, Evidenz und Herkunft.

### Aufgaben
- relationale Verifikation
- Provenance
- Evidence Chains
- ACL / Berechtigungen
- Multi-Hop-Zusammenhänge
- semantische Strukturprüfung
- policy-relevante Einschränkungen

### Prinzip
Approximation darf unterstützen, aber nicht finale Korrektheit ersetzen.

> Finale semantische, sicherheitsrelevante oder regulatorische Entscheidungen müssen auf exakten Graph-/Policy-Prüfungen beruhen.

### Zielvorteile
- Nachvollziehbarkeit
- Auditierbarkeit
- regulatorische Belastbarkeit
- starke Evidenzorientierung

---

## 5.4 LLM / LoRA Final Layer
### Generierung auf wissensgebundener Grundlage

### Ziel
LLM- und LoRA-basierte Antwortsysteme arbeiten nicht auf rohen top-k Chunks, sondern auf geschichteten, geprüften und verdichteten Wissenskontexten.

### Aufgaben
- Antwortgenerierung
- Domänenadaptation
- LoRA-/AdaLoRA-basierte Spezialfähigkeiten
- Final prompt assembly aus:
  - vector candidates
  - tensor summaries
  - graph evidence
  - provenance / trust signals

### Prinzip
LLM/LoRA sind der Final-Layer, nicht die Retrieval-Wahrheit.

### Zielvorteile
- bessere Faithfulness
- geringere Halluzination
- höhere Domänengenauigkeit
- reproduzierbare Adapterlebenszyklen
- robustere Modellwechsel

---

## 6. Langfristige Zielmarken

---

## 6.1 Unified Knowledge Tensor Layer

### Vision
Ein übergreifender tensorischer Wissenslayer verbindet:

- Graph
- Embeddings
- Prozesswissen
- Dokumentwissen
- Adapterwissen
- Provenance

### Bedeutung
RAG ist dann nicht mehr nur:

`retrieve top-k chunks`

sondern:

`retrieve best tensor slices + graph evidence + vector candidates`

### Nutzen
- Wissensrepräsentationen werden quer über mehrere Schichten strukturierbar
- Gemeinsamkeiten und Relevanzen können über mehrere Dimensionen gleichzeitig betrachtet werden
- Provenance und Trust werden retrievalfähig

---

## 6.2 Tensor-native Graph Reasoning

### Vision
Bestimmte graphische Reasoning-Aufgaben werden nicht nur traversal- oder heuristikbasiert gelöst, sondern durch tensornahe Verfahren unterstützt:

- factorized relation propagation
- tensor contraction
- approximate relational inference

### Bedeutung
Tensorische Verfahren unterstützen:
- Kandidatenbildung
- Relevanzpropagation
- Multi-Hop-Priorisierung
- Approximation relationaler Suchräume

### Grenzen
Finale semantische / regulatorische Entscheidungen bleiben exakten Graph-/Policy-Prüfungen vorbehalten.

---

## 6.3 Federated / Cross-shard Tensor Summaries

### Vision
Verteilte Themis-Instanzen tauschen zunächst tensorische Shard-Summaries aus, statt komplette Wissensräume oder große Teilgraphen zu verschieben.

### Bedeutung
Statt kompletter Replikation oder breitem Fan-out:
- kompakte strukturverdichtete Wissenssignaturen
- gezielte exakte Nachladung nur bei Bedarf

### Nutzen
- geringere Netzlast
- bessere Skalierbarkeit
- effizienteres verteiltes RAG
- geeignet für föderierte und souveräne Setups

---

## 7. Zielzustand von RAG

---

## 7.1 Heutiges Standardmodell
`query -> embedding -> vector retrieval -> graph filter -> prompt -> answer`

---

## 7.2 Zielmodell
`query -> ANN frontdoor -> tensor compression/routing -> graph evidence/provenance validation -> LLM/LoRA generation`

### Bedeutet konkret
RAG soll langfristig:
- nicht nur Chunks,
- sondern **Wissensräume**
- nicht nur Ähnlichkeit,
- sondern **strukturierte Relevanz**
- nicht nur Kontext,
- sondern **belegte Evidenz**
liefern.

---

## 8. Evolutionspfad

---

## 8.1 Kurzfristig
### Fokus: Quick Wins

- HNSW/DiskANN als explizite ANN-Frontdoor etablieren
- Tensor-Fingerprints für:
  - Adapter
  - Modelle
  - Shard-Summaries
  - Kandidatenräume
- zero-copy / mmap / lazy loading für Adapter-/Tensorartefakte
- Candidate Compression vor Graph-/LLM-Stufen
- ANN auch für Artefaktsuche nutzbar machen

### Erwarteter Nutzen
- geringere Latenz
- weniger RAM-Kopien
- schnellere Adapter-/Package-Suche
- reduzierte Promptgrößen

---

## 8.2 Mittelfristig
### Fokus: Strukturintegration

- tensorische Summary-Layer für:
  - Teilgraphen
  - Entitätsräume
  - Themencluster
  - Shards
- bessere Verzahnung von:
  - vector candidates
  - tensor summaries
  - graph evidence
- LoRAPackage / PortableAdapterProduct als vollwertige Artefaktklassen
- Modellwechsel-Workflow mit Kompatibilitätsmatrix
- provenance-sensitive retrieval weighting

### Erwarteter Nutzen
- bessere Ergebnisqualität
- bessere Domänenadaptation
- klarere Lebenszyklen
- weniger unnötige Cross-shard-Abfragen

---

## 8.3 Langfristig
### Fokus: Knowledge Fabric

- Unified Knowledge Tensor Layer
- Tensor-native Graph Reasoning
- Federated / Cross-shard Tensor Summaries
- adaptive Query Planner über alle vier Schichten
- package- und modellübergreifende Wissens- und Adaptertopologie

### Erwarteter Nutzen
- skalierbares wissenszentriertes RAG
- deutlich bessere relationale Kontextbildung
- effizientere verteilte Wissensabfragen
- neue Klasse strukturierter KI-Datenbankfunktionalität

---

## 9. Nicht-Ziele

Dieses Future Plan Dokument bedeutet **nicht**:

- Graph vollständig durch Tensoren zu ersetzen
- ANN durch Tensorik zu ersetzen
- exakte Policy-/ACL-/Compliance-Logik durch Approximation zu ersetzen
- jedes Problem in einen Tensor zu pressen

### Stattdessen
Jede Schicht behält ihre klare Verantwortung:
- ANN findet
- Tensor verdichtet
- Graph verifiziert
- LLM antwortet

---

## 10. Risiken

### 10.1 Komplexität
Mehr Schichten bedeuten höhere Systemkomplexität und mehr Orchestrierungsaufwand.

### 10.2 Debuggability
Approximation und Verdichtung erschweren Fehleranalyse und Erklärbarkeit.

### 10.3 Governance-Risiken
Unscharfe Nutzung von Tensor-Approximation an regulatorisch sensiblen Stellen ist zu vermeiden.

### 10.4 Pflegeaufwand
Unified Knowledge Layer erfordert disziplinierte Versionierung, klare APIs und stabile Datenmodelle.

---

## 11. Erfolgskriterien

Langfristige Architekturentscheidungen sollen an messbaren Ergebnissen bewertet werden.

### Retrieval
- geringere Query-Latenz
- geringere Kandidatenmengen
- stabiler oder besserer Recall@k

### Tensorlayer
- Speicherersparnis
- geringere Cross-shard-Last
- weniger redundante Kandidaten

### Graphlayer
- höhere Evidence-Abdeckung
- bessere Provenance-Nachvollziehbarkeit
- geringere Policy-Verletzungen

### LLM / LoRA
- geringere Promptgröße
- bessere Faithfulness
- robustere Adapter-Rebuild- und Wechselpfade

---

## 12. Architektur-Leitsatz

**ThemisDB soll langfristig eine mehrschichtige Knowledge Retrieval Architecture werden, in der ANN schnell findet, Tensoren strukturiert verdichten, Graphen exakte Evidenz und Provenance sichern und LLM/LoRA auf dieser belastbaren Wissensbasis antworten.**

---

## 13. Nächste Schritte

1. Ist-Zustand je Schicht präzise inventarisieren
2. Gap-Analyse ableiten
3. Kurzfristige Quick Wins identifizieren
4. Zielzustand in Architektur- und Issue-Planung überführen
5. Tensor-/ANN-/Graph-/LoRA-Roadmap aufeinander abstimmen

---

## 14. Statushinweis

Dieses Dokument beschreibt die langfristige Zielarchitektur und dient als strategischer Referenzrahmen.  
Es ersetzt keine technische Spezifikation einzelner Komponenten und keine Umsetzungs-Roadmap im Detail.

Konkrete Implementierung, Zuständigkeiten und Reihenfolgen werden über Issue-Planung, ADRs und Modul-Roadmaps definiert.
