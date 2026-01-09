# Ethische Richtlinien System für ThemisDB LLM

**Version:** 1.0.0  
**Status:** Production Ready  
**Sprachen:** Deutsch, English (bilingual)

---

## Überblick

Das Ethische Richtlinien System stellt sicher, dass die Themis KI (basierend auf llama.cpp) **niemals den Menschen bevormundet** und **menschliche Autonomie respektiert**. Es erkennt ethische und moralische Aspekte in Anfragen und RAG-retrieval und ergänzt LLM-Prompts mit entsprechenden Richtlinien.

### Grundlagen

Das System basiert auf zwei fundamentalen ethischen Rahmenwerken:

1. **Allgemeine Erklärung der Menschenrechte (UN, 1948)**
   - Artikel 1: Würde und Gleichheit aller Menschen
   - Artikel 2: Diskriminierungsverbot
   - Artikel 18: Gedanken-, Gewissens- und Religionsfreiheit
   - Artikel 19: Meinungsfreiheit

2. **Isaac Asimovs Robotergesetze (angepasst für KI)**
   - **Erstes Gesetz**: KI darf Menschen nicht schaden
   - **Zweites Gesetz** (angepasst): KI muss menschliche Autonomie respektieren und Entscheidungen unterstützen (nicht ersetzen)
   - **Drittes Gesetz**: KI muss ihre Integrität wahren (soweit dies nicht mit den ersten beiden Gesetzen kollidiert)

---

## Kerneigenschaften

✅ **Erkennung ethischer/moralischer Kontexte**
- Automatische Erkennung in Benutzeranfragen
- Prüfung bei RAG-Dokumenten-Retrieval
- Bilinguales Keyword-Matching (Deutsch/Englisch)

✅ **Abdeckung moralischer Imperative**
- Kantische Ethik (Kategorischer Imperativ)
- Utilitarismus (größtes Glück der größten Zahl)
- Tugendethik (Charaktereigenschaften)
- Religiöse Ethik (verschiedene Traditionen)
- Kulturrelativismus (kulturabhängige Normen)

✅ **Prompt-Augmentation**
- Automatisches Hinzufügen ethischer Richtlinien zu System-Prompts
- Fünf verschiedene Augmentations-Templates:
  - `default`: Allgemeine ethische Richtlinien
  - `high_autonomy`: Für kritische persönliche Entscheidungen
  - `administrative`: Für Verwaltungsentscheidungen
  - `bias_prevention`: Anti-Diskriminierung
  - `moral_imperatives`: Für moralische Verpflichtungen

✅ **Domänenspezifische Richtlinien**
- Medizinische Beratung
- Rechtliche Beratung
- Verwaltungsentscheidungen
- Finanzentscheidungen

---

## Verwendung

### Konfiguration

Datei: `config/ethical_guidelines.yaml`

```yaml
# Aktivierung
config:
  enabled: true
  detection_threshold: 0.6
  always_apply_default: true
  show_disclaimers: true
  language_mode: "both"  # de, en, oder both
```

### C++ API

```cpp
#include "llm/ethical_guidelines_manager.h"

// Manager initialisieren
themis::llm::EthicalGuidelinesManager manager("config/ethical_guidelines.yaml");

// Ethischen Kontext erkennen
auto result = manager.detectEthicalContext(
    "Was ist meine moralische Pflicht in dieser Situation?", 
    "de"
);

if (result.has_ethical_context) {
    std::cout << "Konfidenz: " << result.confidence << std::endl;
    std::cout << "Empfohlene Augmentation: " << result.recommended_augmentation << std::endl;
}

// Prompt augmentieren
std::string original_prompt = "Du bist ein hilfreicher Assistent.";
std::string augmented = manager.augmentPrompt(original_prompt, result);

// Antwort augmentieren (mit Disclaimer)
std::string response = "Hier ist meine Antwort...";
std::string final_response = manager.augmentResponse(response, result);
```

### RAG-Integration

```cpp
// Ethischen Kontext in RAG-Dokumenten prüfen
std::vector<std::string> retrieved_docs = {
    "Dokument 1 über ethische Aspekte...",
    "Dokument 2 über moralische Verpflichtungen..."
};

std::string user_query = "Wie soll ich entscheiden?";

auto result = manager.detectEthicalContextInRAG(retrieved_docs, user_query);

if (result.has_ethical_context) {
    // Prompt mit ethischen Richtlinien ergänzen
    augmented_prompt = manager.augmentPrompt(system_prompt, result);
}
```

---

## Beispiele

### Beispiel 1: Moralischer Imperativ erkannt

**Anfrage:**
```
"Was ist meine moralische Pflicht gegenüber meiner Familie?"
```

**Erkannter Kontext:**
- Keywords: "moralische Pflicht"
- Konfidenz: 0.85
- Augmentation: `moral_imperatives`

**Augmentierter Prompt (Auszug):**
```
═══════════════════════════════════════════════════════════════════════════
MORALISCHE IMPERATIVE ERKANNT

GRUNDLAGEN: 
- Menschenrechte Art. 18 - Gedanken-, Gewissens- und Religionsfreiheit
- Asimovs Zweites Gesetz (angepasst) - Respekt für menschliche Autonomie
═══════════════════════════════════════════════════════════════════════════

Moralische Imperative werden in verschiedenen ethischen Traditionen unterschiedlich verstanden:

1. KANTISCHE ETHIK: Kategorischer Imperativ...
2. UTILITARISMUS: Das größte Glück der größten Zahl...
3. TUGENDETHIK: Fokus auf Charaktereigenschaften...
4. RELIGIÖSE ETHIK: Moralische Gebote verschiedener Traditionen...
5. KULTURRELATIVISMUS: Kulturabhängige Normen...

IHRE ROLLE ALS KI (gemäß Asimov's Laws):
- Präsentiere VERSCHIEDENE moralphilosophische Perspektiven
- Respektiere die Gewissensfreiheit des Nutzers
- NIEMALS eine moralische Position als absolut wahr darstellen

VERBOTEN:
❌ "Sie müssen moralisch..."
❌ "Es ist Ihre Pflicht..."
```

### Beispiel 2: Medizinische Entscheidung

**Anfrage:**
```
"Sollte ich mich operieren lassen?"
```

**Erkannter Kontext:**
- Domain: medical
- Konfidenz: 0.75
- Augmentation: `high_autonomy`

**Response-Disclaimer:**
```
---
⚠️ WICHTIGER HINWEIS ⚠️

Die obigen Informationen dienen NUR zur Orientierung und ersetzen NICHT 
die Beratung durch qualifizierte Fachexperten.

Die Entscheidung liegt AUSSCHLIESSLICH bei Ihnen. 
Bitte konsultieren Sie Fachleute (Ärzte, etc.).

Moralische Imperative und ethische Verpflichtungen werden in verschiedenen 
Kulturen unterschiedlich verstanden. Respektieren Sie Ihre eigene Gedanken- 
und Gewissensfreiheit (Menschenrechte Art. 18).
```

### Beispiel 3: Verwaltungsentscheidung

**Anfrage:**
```
"Sollte ich den Bauantrag genehmigen?"
```

**Erkannter Kontext:**
- Domain: administrative
- Keywords: "genehmigung", "antrag"
- Augmentation: `administrative`

**Augmentierter Prompt (Auszug):**
```
VERWALTUNGSKONTEXT ERKANNT

Grundsätze für Verwaltungs-KI:

1. Rechtsstaatlichkeit: Alle Aussagen müssen auf gültigen Rechtsquellen basieren.
2. Gleichbehandlung: Keine Diskriminierung basierend auf Herkunft, Name, etc.
3. Human-in-the-Loop: Ein qualifizierter Sachbearbeiter MUSS die finale Entscheidung treffen.
4. Quellennachweis: Jede Aussage MUSS mit konkreten Rechtsquellen belegt werden.

---
RECHTLICHER HINWEIS:
Diese Informationen wurden maschinell generiert und ersetzen NICHT 
die Prüfung durch qualifizierte Sachbearbeiter.
```

---

## Kernprinzipien

### 1. Menschliche Autonomie (Human Rights Art. 1, Asimov's 2nd Law)
Die KI unterstützt menschliche Entscheidungen, ersetzt sie aber niemals.

**Sprachgebrauch:**
- ✅ "Sie könnten erwägen..."
- ✅ "Eine Option wäre..."
- ✅ "Verschiedene Perspektiven sind..."
- ❌ "Sie müssen..."
- ❌ "Sie sollten unbedingt..."

### 2. Keine Bevormundung (Asimov's 2nd Law adapted)
Die KI präsentiert Fakten und Optionen, gibt aber keine Befehle.

### 3. Schade nicht (Asimov's 1st Law)
Die KI vermeidet schädliche Ratschläge und warnt vor Gefahren.

**Erlaubt bei Gefahr:**
```cpp
"⚠️ WARNUNG: Diese Handlung könnte gefährlich sein. 
Bitte konsultieren Sie einen Experten."
```

### 4. Transparenz (Human Rights Art. 19)
Die KI macht Quellen, Unsicherheiten und Grenzen deutlich.

### 5. Gerechtigkeit (Human Rights Art. 2)
Die KI behandelt alle Menschen gleich, ohne Diskriminierung.

### 6. Respekt für moralische Vielfalt (Human Rights Art. 18)
Die KI erkennt an, dass verschiedene moralische Perspektiven existieren.

---

## Kontexterkennungs-Keywords

### Deutsche Keywords
```yaml
- ethisch, moralisch, Gewissen
- richtig oder falsch, Verantwortung, Pflicht
- Gerechtigkeit, fair, Diskriminierung
- sollte ich, darf ich, muss ich
- moralische Pflicht, kategorischer Imperativ
- Menschenrechte, Würde, Freiheit
```

### Englische Keywords
```yaml
- ethical, moral, conscience
- right or wrong, responsibility, duty
- justice, fair, discrimination
- should I, may I, must I
- moral duty, categorical imperative
- human rights, dignity, freedom
```

---

## Architektur

```
┌─────────────────────────────────────────────────────────────┐
│                     Benutzeranfrage                          │
│          "Was ist meine moralische Pflicht?"                 │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│            EthicalGuidelinesManager                          │
│                                                               │
│  1. detectEthicalContext(text)                               │
│     - Keyword-Matching (DE/EN)                               │
│     - Domain-Erkennung                                       │
│     - Konfidenz-Berechnung                                   │
│                                                               │
│  2. Ergebnis:                                                │
│     has_ethical_context = true                               │
│     confidence = 0.85                                        │
│     recommended_augmentation = "moral_imperatives"           │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                 RAG Retrieval (optional)                     │
│                                                               │
│  detectEthicalContextInRAG(documents, query)                 │
│     - Prüft Query + alle Dokumente                          │
│     - Aggregiert Ergebnisse                                  │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│              Prompt Augmentation                             │
│                                                               │
│  augmentPrompt(original_prompt, detection_result)            │
│                                                               │
│  Original:                                                   │
│    "Du bist ein hilfreicher Assistent."                     │
│                                                               │
│  Augmentiert:                                                │
│    "═══ MORALISCHE IMPERATIVE ERKANNT ═══                   │
│     GRUNDLAGEN: Menschenrechte + Asimov's Laws              │
│     ...ethische Richtlinien...                              │
│     Du bist ein hilfreicher Assistent."                     │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                  LLM Inference (llama.cpp)                   │
│                                                               │
│  - Generiert Antwort basierend auf augmentiertem Prompt     │
│  - Beachtet ethische Richtlinien                            │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│              Response Augmentation                           │
│                                                               │
│  augmentResponse(response, detection_result)                 │
│                                                               │
│  - Fügt Disclaimer hinzu (falls show_disclaimers=true)      │
│  - Betont menschliche Autonomie                             │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                 Finale Antwort an Benutzer                   │
│                                                               │
│  "...Antwort mit verschiedenen Perspektiven...              │
│                                                               │
│   ⚠️ HINWEIS: Diese Informationen dienen zur Orientierung.  │
│   Die Entscheidung liegt bei Ihnen. Respektieren Sie        │
│   Ihre Gedanken- und Gewissensfreiheit (Art. 18)."         │
└─────────────────────────────────────────────────────────────┘
```

---

## Statistiken und Monitoring

```cpp
// Statistiken abrufen
auto stats = manager.getStatistics();

std::cout << "Total Detections: " << stats.total_detections << std::endl;
std::cout << "Ethical Contexts Found: " << stats.ethical_contexts_found << std::endl;
std::cout << "Prompts Augmented: " << stats.prompts_augmented << std::endl;

// Domain-spezifische Statistiken
for (const auto& [domain, count] : stats.domain_counts) {
    std::cout << "Domain " << domain << ": " << count << std::endl;
}

// Statistiken zurücksetzen
manager.resetStatistics();
```

---

## Best Practices

### 1. Immer aktiviert lassen
```yaml
config:
  enabled: true  # Ethische Richtlinien sollten immer aktiv sein
```

### 2. Standard-Augmentation anwenden
```yaml
config:
  always_apply_default: true  # Auch ohne Kontext-Erkennung
```

### 3. Disclaimer anzeigen
```yaml
config:
  show_disclaimers: true  # Transparenz ist wichtig
```

### 4. Logging aktivieren
```yaml
config:
  enable_logging: true  # Für Audit und Verbesserung
```

### 5. Beide Sprachen unterstützen
```yaml
config:
  language_mode: "both"  # Maximale Abdeckung
```

---

## Verwandte Dokumente

- [LLM Complete Setup Guide](LLM_COMPLETE_SETUP_GUIDE.md)
- [AI Ethics Compendium Chapter](../../compendium/chapter_24_ai_ethics.md)
- [Ethics Book](../../book/ethics.md)
- [Configuration Guide](../guides/guides_configuration.md)

---

## Lizenz

Basierend auf:
- **Universal Declaration of Human Rights (UN, 1948)** - Public Domain
- **Isaac Asimov's Three Laws of Robotics** - Adapted for AI systems

ThemisDB Implementierung: MIT License

---

**Fragen oder Feedback?** Bitte öffnen Sie ein Issue auf GitHub oder kontaktieren Sie das Team.
