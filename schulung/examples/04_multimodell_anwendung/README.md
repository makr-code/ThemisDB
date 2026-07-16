# Multi-Model-Anwendung — Lernplattform

![Schwierigkeit](https://img.shields.io/badge/schwierigkeit-experte-red)
![Dauer](https://img.shields.io/badge/dauer-45--60%20min-blue)

## Übersicht

Vollständige Multi-Model-Anwendung: **Lernplattform** mit allen ThemisDB-Datenmodellen.

### Datenmodell

```
Collections:
  mm_courses      (Dokument/Vertex) — Kurse mit Embeddings
  mm_students     (Vertex)          — Teilnehmer
  mm_instructors  (Vertex)          — Dozenten
  mm_enrollments  (Edge)            — Einschreibungen (students → courses)
  mm_taught_by    (Edge)            — Dozentenzuweisung (courses → instructors)
  mm_progress     (Dokument)        — Lernfortschritt-Ereignisse

Modelle im Einsatz:
  Dokument   → Kurse mit flexiblen Attributen
  Graph      → Einschreibungen, Dozentenzuweisungen, Traversierungen
  Vektor     → Kurs-Embeddings für Ähnlichkeitsempfehlungen
  Relational → Einschreibungs-Status, Noten (strukturiert)
```

## Ausführen

```bash
cd schulung/examples/04_multimodell_anwendung
python main.py
```

## Features

### 1. Kursübersicht (Graph + Aggregation)
- Teilnehmerzahlen via INBOUND-Traversierung
- Dozenten via OUTBOUND-Traversierung
- Abschlussquoten

### 2. Studenten-Dashboard (Graph + Dokument)
- Belegte Kurse mit Status
- Abschluss-Scores
- Fortschrittsanzeige

### 3. Vektor-Empfehlungen (Vektormodell)
- Kurs-Embeddings (8-dimensional)
- Cosinus-Ähnlichkeit
- Personalisierte Empfehlungen basierend auf bisherigen Kursen

### 4. Dozenten-Statistiken (Graph + Aggregation)
- Kurse pro Dozent
- Gesamt-Einschreibungen

### 5. Fortschrittsanalyse (Zeitreihen-ähnlich)
- Lernaktivität pro Tag
- Aktivste Lernende

## Empfehlungsalgorithmus

```python
# Interesse-Vektor = Durchschnitt der belegten Kurs-Embeddings
interest_vector = average([embedding(course) for course in enrolled_courses])

# Empfehlungen: Cosinus-Ähnlichkeit mit nicht belegten Kursen
recommendations = sorted(
    [
        (course, cosine_similarity(course.embedding, interest_vector))
        for course in all_courses
        if course not in enrolled_courses
    ],
    key=lambda x: x[1],
    reverse=True
)[:3]
```

## Weiterführend

- [Alle Beispiele aus dem Hauptverzeichnis](../../../examples/)
- [Best Practices Guide](../../dokumente/05_best_practices_guide.md)
- [Präsentation: Anwendungsbeispiele](../../praesentation/05_anwendungsbeispiele.md)
