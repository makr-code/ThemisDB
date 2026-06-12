# Paper Survey: Shannon, LLM-Unsicherheit, FLARE und Re-Retrieval RAG

## Ziel

Kurierte Forschungsquellen zu

- Shannon Information Theory als Fundament,
- Entropie/Unsicherheit bei LLM-Decoding,
- Retrieval-Augmented Generation (RAG),
- iterativen/adaptiven Re-Retrieval-Ansätzen (inkl. FLARE).

Alle Quellen sind mit direkter URL dokumentiert.

## A) Fundament: Shannon / Information Theory

1. Claude E. Shannon (1948)
- Titel: A Mathematical Theory of Communication
- Kernbeitrag: Entropie, Mutual Information, Channel Capacity als formales Fundament.
- Relevanz fuer ThemisDB: Basis fuer Unsicherheits- und Informationsmaße in Retrieval, Grounding und Optimierung.
- Quelle:
  - https://doi.org/10.1002/j.1538-7305.1948.tb01338.x
  - https://math.harvard.edu/~ctm/home/text/others/shannon/entropy/entropy.pdf

2. Wikipedia Uebersicht (sekundaer, gut fuer Einstieg)
- Titel: Information theory
- Kernbeitrag: kompakter Ueberblick inkl. Entropie, MI, KL, historische Einordnung.
- Quelle:
  - https://en.wikipedia.org/wiki/Information_theory

## B) LLM-Unsicherheit, Entropie, Decoding

1. Holtzman et al. (ICLR 2020)
- Titel: The Curious Case of Neural Text Degeneration
- Kernbeitrag: zeigt Degeneration bei Likelihood-basiertem Decoding; motiviert Nucleus Sampling (Top-p) und Tail-Trunkierung.
- Shannon-Bezug: praktische Kontrolle von Unsicherheit/Verteilungs-Tail im Tokenraum.
- Quelle:
  - https://arxiv.org/abs/1904.09751

2. Hewitt, Manning, Liang (EMNLP Findings)
- Titel: Truncation Sampling as Language Model Desmoothing
- Kernbeitrag: interpretiert Trunkierungs-Sampling als Desmoothing; fuehrt eta-sampling mit entropieabhaengiger Schwelle ein.
- Shannon-Bezug: explizite Entropie-basierte Decoding-Schwellen.
- Quelle:
  - https://arxiv.org/abs/2210.15191

3. Zhu et al. (AAAI 2024)
- Titel: Hot or Cold? Adaptive Temperature Sampling for Code Generation with Large Language Models
- Kernbeitrag: adaptive Temperature-Steuerung je Token-Schwierigkeit.
- Shannon-Bezug: indirekter Unsicherheitsbezug ueber dynamische Exploration/Exploitation.
- Quelle:
  - https://arxiv.org/abs/2309.02772

## C) RAG-Basis und skalierte Retrieval-Modelle

1. Lewis et al. (NeurIPS 2020)
- Titel: Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks
- Kernbeitrag: param. + non-param. Memory; klassischer Startpunkt fuer modernes RAG.
- Relevanz: Grundarchitektur fuer retrievalgestuetzte, auditierbare Antworten.
- Quelle:
  - https://arxiv.org/abs/2005.11401

2. Borgeaud et al. (RETRO)
- Titel: Improving language models by retrieving from trillions of tokens
- Kernbeitrag: Retrieval auf grossem externen Korpus fuer LM-Verbesserung.
- Relevanz: zeigt Skalierungshebel von externem Wissen statt nur Parametern.
- Quelle:
  - https://arxiv.org/abs/2112.04426

3. Izacard et al. (Atlas)
- Titel: Atlas: Few-shot Learning with Retrieval Augmented Language Models
- Kernbeitrag: retrieval-augmentierte Few-shot-LMs fuer wissensintensive Aufgaben.
- Relevanz: robuste Leistung bei kleinen Trainingssets + updatefaehigem Index.
- Quelle:
  - https://arxiv.org/abs/2208.03299

4. Gao et al. (Survey)
- Titel: Retrieval-Augmented Generation for Large Language Models: A Survey
- Kernbeitrag: Systematik von Naive/Advanced/Modular RAG, Evaluationsrahmen.
- Relevanz: gute Landkarte fuer Architekturentscheidungen.
- Quelle:
  - https://arxiv.org/abs/2312.10997

## D) Iteratives / Adaptives Re-Retrieval (inkl. FLARE)

1. Jiang et al. (EMNLP 2023)
- Titel: Active Retrieval Augmented Generation
- Kernbeitrag: FLARE (Forward-Looking Active REtrieval): waehrend Generation aktiv entscheiden, wann/was erneut retrieved wird; low-confidence-token Trigger fuer Regeneration.
- Relevanz: direkter Blueprint fuer Re-Retrieval in langen Antworten.
- Quelle:
  - https://arxiv.org/abs/2305.06983

2. Trivedi et al. (ACL 2023)
- Titel: Interleaving Retrieval with Chain-of-Thought Reasoning for Knowledge-Intensive Multi-Step Questions
- Kernbeitrag: IRCoT interleaved Retrieval + Reasoning pro Schritt statt One-shot-Retrieval.
- Relevanz: starker Multi-Hop-Ansatz gegen Halluzinationen und Retrieval-Fehlgriffe.
- Quelle:
  - https://arxiv.org/abs/2212.10509

3. Asai et al. (Self-RAG)
- Titel: Self-RAG: Learning to Retrieve, Generate, and Critique through Self-Reflection
- Kernbeitrag: on-demand Retrieval + Selbstkritik-Token; adaptives Verhalten waehrend Inferenz.
- Relevanz: kombinierbar mit Grounding-Scoring und Policy-Gates.
- Quelle:
  - https://arxiv.org/abs/2310.11511

4. Yan et al. (CRAG)
- Titel: Corrective Retrieval Augmented Generation
- Kernbeitrag: bewertet Retrieval-Qualitaet und startet corrective Aktionen (z. B. Web-Erweiterung, Recompose).
- Relevanz: robust gegen schlechte initiale Retrieval-Treffer.
- Quelle:
  - https://arxiv.org/abs/2401.15884

5. Jeong et al. (NAACL 2024)
- Titel: Adaptive-RAG: Learning to Adapt Retrieval-Augmented Large Language Models through Question Complexity
- Kernbeitrag: waehlt dynamisch zwischen no-retrieval, single-step, iterative retrieval basierend auf Query-Komplexitaet.
- Relevanz: reduziert Overhead bei einfachen Queries, erhoeht Robustheit bei komplexen.
- Quelle:
  - https://arxiv.org/abs/2403.14403

6. Shi et al. (REPLUG)
- Titel: REPLUG: Retrieval-Augmented Black-Box Language Models
- Kernbeitrag: Retrieval-Boost auch bei Black-Box-LMs (Prefixing statt Modellumbau).
- Relevanz: pragmatischer Weg fuer Systeme ohne tiefe LM-Integration.
- Quelle:
  - https://arxiv.org/abs/2301.12652

## E) Ergaenzung: Graph-RAG Uebersicht

1. Han et al. (Survey 2025)
- Titel: Retrieval-Augmented Generation with Graphs (GraphRAG)
- Kernbeitrag: Komponentenmodell fuer GraphRAG (query processor, retriever, organizer, generator, data source).
- Relevanz: hilfreich fuer ThemisDB, da Graph + RAG bereits ein Kernpfad ist.
- Quelle:
  - https://arxiv.org/abs/2501.00309

## F) Konkrete Hinweise fuer ThemisDB-Next-Steps aus den Papern

1. FLARE + CRAG kombinieren
- FLARE fuer aktive Re-Retrieval-Trigger bei low confidence.
- CRAG-Idee als Retrieval-Quality-Gate davor.

2. Entropie als Steuer-Signal standardisieren
- Nach Hewitt et al. entropieabhaengige Schwellen fuer Token/Passage-Entscheidungen einfuehren.
- Einheitliche Logging-Felder fuer entropy, confidence, retrieval_action.

3. Adaptive-RAG Routing bauen
- Query-Komplexitaet klassifizieren und Pfadwahl treffen:
  - einfach: single-step retrieval,
  - komplex: iterative retrieval (IRCoT/FLARE),
  - trivial: no-retrieval.

4. Self-RAG-artige Kritikspur
- Reflection-/Critique-Signale als auditierbare Intermediate-Events speichern.

## G) Kurzliste (wenn du nur 5 zuerst lesen willst)

1. Shannon 1948
- https://doi.org/10.1002/j.1538-7305.1948.tb01338.x

2. RAG (Lewis et al. 2020)
- https://arxiv.org/abs/2005.11401

3. FLARE (Jiang et al. 2023)
- https://arxiv.org/abs/2305.06983

4. Self-RAG (Asai et al. 2023)
- https://arxiv.org/abs/2310.11511

5. CRAG (Yan et al. 2024)
- https://arxiv.org/abs/2401.15884
