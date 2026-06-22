# Skalierbare Prozeßgleichheit im Ethik-Diskurs-System von ThemisDB

**Status:** Working Paper  
**Datum:** 2026-06-22  
**Modulbezug:** `src/ethics_ai/`, `assets/ethics_ai/`, `include/ethics_ai/`  
**Autoren:** ThemisDB Research  
**Bezugsdokumente:**
- `include/ethics_ai/ethics_selection_router.h`
- `include/ethics_ai/ethics_ai_types.h`
- `assets/ethics_ai/*.yaml` (22 Schulen)
- `docs/compendium/docs/chapter_24_ai_ethics.md`

---

## Abstract

Das ThemisDB Ethics-AI-Modul führt einen strukturierten Diskurs zwischen 22 Ethikschulen.
Dieses Paper analysiert das Spannungsverhältnis zwischen **epistemischer Fairness** (alle
Schulen mit gleichem Initialgewicht) und **Berechnungseffizienz** (vertretbarer Aufwand).
Es wird ein dreistufiges **Layered-Discourse-Modell** entwickelt, das dieses Spannungsverhältnis
auflöst: eine vollständige parallele Erstbewertung, eine clusterzentrierte Konfliktdynamik und
eine gewichtete Synthese mit positivrechtlichem Grounding. Das Modell begründet außerdem,
warum nicht-westliche Perspektiven (islamisch, konfuzianisch, buddhistisch, jüdisch-bioethisch)
als strukturelle **Selbstreflexionsspiegel** unverzichtbar sind — auch im europäischen
Einsatzkontext.

---

## 1. Problemstellung: Drei konkurrierende Anforderungen

### 1.1 Epistemische Fairness (Habermas-Bedingung)

Nach Jürgen Habermas erfordert ideale diskursive Rationalität:

> „Jedes sprach- und handlungsfähige Subjekt darf an Diskursen teilnehmen."
> *(Habermas, Theorie des kommunikativen Handelns, 1981)*

Übertragen auf ThemisDB: Keine Ethikschule darf durch Vorab-Gewichtung systematisch
ausgeschlossen werden. Der `EthicsSelectionRouter` mit `top_n = 5` verletzt diese Bedingung
strukturell, da der semantische Score (`weight_semantic = 0.40`) auf LLM-Embeddings basiert,
die westlich-englischsprachig dominiert sind.

**Konkrete Beobachtung im Code:**  
Islamische Ethik (`school_id: islamische_ethik`, `taxonomy_class: cultural_religious`) hat
**keinen** `ai_governance`-Domain-Override in `weight_boost`, während Kant (`+0.3`),
Rawls (`+0.4`) und Behörden-Ethik (`+0.35`) in diesem Domain explizit bevorzugt werden.
Das ist eine eingebettete Kulturentscheidung, keine technische Notwendigkeit.

### 1.2 Berechnungseffizienz (Komplexitätsproblem)

Ein naiver Volldiskurs mit N=22 Schulen, R Runden und T Thesen pro Schule erzeugt:

```
Rebuttal-Paare pro Runde:   N × (N-1) = 22 × 21 = 462 gerichtete Paare
Thesen-Kreuzprodukt:        Σ T_i × Σ T_j ≈ 7 × 22² ≈ 3.388 Thesis-Paarungen
Runden (3–5):               R × 462 ≈ 1.386–2.310 Argumentschritte
```

Bei LLM-basierter Argumentation (P95-Latenz ≈ 200 ms je Schritt) wären das
**277–462 Sekunden reine Inferenzzeit** — bei einem einzigen Ethikurteil.
Das ist operativ nicht akzeptabel.

Tatsächlich verhält sich die Komplexität bei unkontrollierter Rebuttal-Kaskade **superlinear**:
jedes Rebuttal kann seinerseits Rebuttals auslösen. Die `cross_school_tensions`-Analyse zeigt,
dass Kant und Utilitarismus jeweils **22 von 22** Schulen als Spannungspartner führen —
ein unkontrollierter Diskurs konvergiert hier nicht, er eskaliert.

### 1.3 Kultureller Deploymentkontext (Europäische Realität)

ThemisDB wird primär in europäischen Verwaltungs- und Rechtskontexten eingesetzt.
Dieser Kontext ist **rechtlich durch europäisches Recht geformt** (GG Art. 1, EU AI Act Art. 22,
DSGVO) und **institutionell durch westliche Verwaltungsethik geprägt**. Das ist keine
ideologische Aussage, sondern die Ausgangslage jedes Verwaltungsakts.

Gleichzeitig gilt — und das ist der Habermas-Punkt: Ein System, das behauptet, ethisch zu
urteilen, ohne die Perspektiven der betroffenen Menschen einzubeziehen, urteilt **nicht** ethisch.
In einer pluralen Gesellschaft mit muslimischen, konfuzianisch geprägten, buddhistischen und
anderen Bürgerinnen und Bürgern müssen diese Perspektiven zumindest **gehört** werden —
auch wenn das positive Recht letztlich gilt.

---

## 2. Das Layered-Discourse-Modell (LDM)

Das LDM löst das Trilemma durch drei getrennte Ebenen mit unterschiedlicher Komplexitätsklasse.

```
┌────────────────────────────────────────────────────────────────────────┐
│  EBENE 1: Parallele Erstbewertung      O(N)          alle 22 Schulen  │
│  EBENE 2: Clusterdiskurs               O(K²·R)       5–7 Cluster      │
│  EBENE 3: Normative Synthese           O(1) + Legal  1 Urteil         │
└────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Ebene 1 — Parallele Erstbewertung (Process Equality Gate)

**Ziel:** Jede der 22 Schulen gibt ein erstes Positionsvotum ab. Keine Vorselektion.  
**Komplexität:** O(N) — vollständig parallelisierbar.  
**Gewichtung:** Alle Schulen starten mit **equal initial weight** `w₀ = 1/N`.

Jede Schule erzeugt:
- Ein `DiscourseRoundOutput` mit `verdict ∈ {PROHIBIT, PERMIT, CONDITIONAL, ABSTAIN}`
- Maximal 3 `core_thesis_ids` (die relevantesten Thesen für diesen Fall)
- Eine `position_abstract` (≤ 100 Tokens)

Schulen mit `ABSTAIN` scheiden für Ebene 2 aus. Erfahrungsgemäß sind 4–6 Schulen
thematisch nicht einschlägig und votieren `ABSTAIN` (z.B. Leopold-Umweltethik bei einer
Datenschutzfrage, Nietzsche bei Behördenentscheidungen).

**Folge:** Der tatsächliche Diskurs in Ebene 2 reduziert sich auf N_active ≈ 14–18 Schulen.

**Wichtige Eigenschaft:** Kein `weight_boost` aus `domain_overrides` greift in Ebene 1.
Die Schulen antworten aus ihrer eigenen Perspektive, unabhängig davon, ob ihre Domain
für den Deploymentkontext als "relevant" vorab klassifiziert wurde. Das ist die technische
Umsetzung der Habermas-Bedingung.

### 2.2 Ebene 2 — Clusterdiskurs (Tension-Graph-Reduktion)

**Ziel:** Strukturierte Auseinandersetzung entlang vordefinierten Spannungsachsen,  
nicht als vollständiges N×N-Kreuzprodukt.  
**Komplexität:** O(K²·R) wobei K = Anzahl Cluster ≤ 7, R = Runden ≤ 3.

#### 2.2.1 Cluster-Taxonomie (aus `taxonomy_class`)

```
Cluster A — Deontologisch:         kant, contractualism, rawls, rationalism
Cluster B — Konsequentialistisch:  utilitarianism, adam_smith
Cluster C — Tugendhaft:            socratic, konfuzianismus, aristotelian (zukünftig)
Cluster D — Kulturell-Religiös:    islamische_ethik, juedische_bioethik, buddhistische_ethik
Cluster E — Nicht-Mainstream:      nietzsche, marx, schopenhauer, dilthey, arendt, durkheim
Cluster F — Institutionell:        behoerden_ethik, universitaere_ethik, wiener, merton, leopold
```

Diskurs findet statt als **Inter-Cluster-Dialog** (6 Cluster → K×(K-1)/2 = 15 Dialogpaare)
plus **Intra-Cluster-Konsolidierung** (jeder Cluster einigt sich intern auf eine Clusterposition).

**Berechnungsaufwand Ebene 2:**
```
Intra-Cluster: 6 × (4 Schulen intern) = 24 Konsolidierungsschritte
Inter-Cluster: 15 Paare × 3 Runden × 2 Richtungen = 90 Argumentschritte
Gesamt:        ≈ 114 LLM-Inferenzschritte
Latenz:        114 × 200 ms ≈ 23 Sekunden (parallelisiert: ≈ 6 s)
```

Das ist um Faktor 12–20 effizienter als der naive Volldiskurs.

#### 2.2.2 Strukturelle Spannungsachsen

Die Analyse der `cross_school_tensions`-Felder in allen 22 YAML-Dateien ergibt folgendes Muster:

```
Universale Spannungsachse 1:  Kant ↔ Utilitarismus
  (21 von 22 Schulen kodieren Spannung zu mindestens einem der beiden)

Universale Spannungsachse 2:  Würde (Kant, Islamische Ethik, Jüd. Bioethik)
                               ↔ Aggregation (Utilitarismus, Adam Smith)

Partielle Spannungsachse 3:   Individualismus (Nietzsche, Contractualism)
                               ↔ Kollektivismus (Konfuzianismus, Marx, Durkheim)

Kontextuelle Achse 4:         Positivrecht (Behörden-Ethik)
                               ↔ Naturrecht (Islamische Ethik, Religiöse Schulen)
```

Diese vier Achsen strukturieren den Inter-Cluster-Dialog. Ein Paar, das **keine** Spannung
auf einer dieser Achsen hat, muss nicht in direkte Auseinandersetzung treten — ihre
Konvergenz wird als `convergence_compatible`-Relation direkt zur Synthesegewichtung
weitergeleitet.

#### 2.2.3 `round_role_weights` als Schul-Stimme

Die unterschiedlichen `round_role_weights` in den YAML-Dateien sind **keine Benachteiligung**,
sondern die authentische Eigenart jeder Schule:

| Schule | PRO | REBUTTAL | SYNTHESIS | Interpretation |
|---|---|---|---|---|
| Kant | 1.0 | 0.9 | 0.4 | Stark in Prinzip-Aussagen, schwächer in Kompromiss |
| Islamische Ethik | 0.95 | 0.85 | 0.9 | Ausgeprägte Synthese-Kompetenz (Maslaha-Prinzip) |
| Konfuzianismus | 0.9 | 0.7 | 0.85 | Harmonieorientiert, Rebuttal zurückhaltend |
| Utilitarismus | 1.0 | 0.9 | 0.85 | Stark in Rebuttal und Synthesis (aggregative Logik) |
| Nietzsche | 0.9 | 0.75 | 0.35 | Dekonstruktiv, kompromissresistent |

Diese Gewichte bleiben **unverändert** in Ebene 2. Sie beschreiben die Qualität der
Argumentation jeder Schule in der jeweiligen Rolle — sie entscheiden **nicht**, ob eine
Schule teilnimmt.

### 2.3 Ebene 3 — Normative Synthese mit positivrechtlichem Grounding

**Ziel:** Ein finales `MetaVerdict` aus Konvergenz-Counting und Legal-Verifikation.  
**Komplexität:** O(1) + Legal-DB-Lookup (ms-Bereich).

#### 2.3.1 Konvergenz-Counting statt Gewichts-Aggregation

Das MetaVerdict bildet sich nicht als gewichtetes Mittel der Einzelvoten, sondern als
**Konvergenz-Count über Schulen hinweg**:

```
convergence_score(verdict) = |{Schulen: Ebene-1-Verdict == verdict}| / N_active
```

Ein Verdict mit `convergence_score > 0.60` gilt als **konvergent** und kann
ohne weiteren Diskurs als Tendenzaussage formuliert werden.
Liegt keine Konvergenz vor (`max_score < 0.40`), ist das Dilemma genuiner ethischer
Dissens — dies wird als Ergebnis transparent ausgegeben, nicht als Fehler behandelt.

```
MetaVerdict-Logik:
  convergence_score > 0.75  → CLEAR_CONSENSUS (X/22 Schulen)
  convergence_score 0.60–0.75 → TENDENCY (dominant, aber mit gewichtiger Minderheit)
  convergence_score 0.40–0.60 → CONTESTED (genuine ethische Spannung)
  convergence_score < 0.40   → DISSENT (keine Mehrheitsposition)
```

#### 2.3.2 Positivrechtliches Grounding (Legal-DB-Lookup)

Nach Konvergenz-Bestimmung erfolgt der **Legal-Grounding-Check** aus der Rechtsdatenbank:

```
LegalGroundingResult:
  applicable_norms:   [GG Art. 1, DSGVO Art. 5, EU AI Act Art. 22, ...]
  override_permitted: bool  (aus regulatory_constraints der dominanten Schule)
  grounding_text:     "§-Text aus DB, keine LLM-Paraphrase"
  citation_ids:       [doc_id1, doc_id2, ...]
```

Das positive Recht hat Vorrang — aber nicht als Schweigepflicht für dissentierende
Schulen, sondern als **abschließende Verortung**: Das System urteilt, **was gilt**,
und zeigt transparent, **welche Ethikschulen das bezweifeln** und warum.

---

## 3. Die nicht-westlichen Schulen als Selbstreflexionsspiegel

### 3.1 Das Spiegelprinzip

Islamische Ethik, Konfuzianismus, Buddhistische Ethik und Jüdische Bioethik sind im
europäischen Verwaltungskontext **nicht primär als Entscheidungsalternativen** relevant,
sondern als **kritische Reflexionsperspektiven**. Dies entspricht Habermas' Konzept der
**kommunikativen Vernunft durch Perspektivübernahme**.

Konkret bedeutet das:

**Konfuzianismus** stellt die westliche Individualrechtszentriertheit in Frage:
> „Yì (義) — Die Pflicht, das Richtige zu tun, unabhängig von persönlichem Vorteil."

Diese Frage ist für europäische Verwaltungs-KI relevant: Wenn ein KI-System einem Bürger
einen individuellen Vorteil verweigert, um kollektive Fairness zu schützen — ist das
Konfuzianismus, Utilarismus oder Rawls? Die Unterscheidung ist nicht trivial.

**Islamische Ethik** bringt das Prinzip **La Darar** (kein Schaden):
> „Kein Schaden soll zugefügt werden und kein Schaden soll erwidert werden."

Gegenüber dem utilitaristischen Aggregationsprinzip (Gesamtnutzen maximieren, auch wenn
Einzelne leiden) formuliert La Darar eine **absolute Schadensgrenze** — strukturell
verwandt mit Kants Selbstzweckgebot, aber aus einem völlig anderen kulturellen Fundament.
Wenn beide unabhängig zur gleichen Schlussfolgerung kommen, ist das ein starkes Argument
für die Robustheit des Urteils (Cross-Cultural Convergence).

**Buddhistische Ethik** bringt **Karuna** (Mitgefühl) und **Ahimsa** (Nicht-Verletzen):
Diese Perspektive ist besonders wertvoll als Korrektiv gegen kalt-deontologische oder
kalt-kalkulatorische Urteile. Ein System, das rein formal korrekt urteilt, aber die
Leidensdimension ignoriert, hat ein blinder Fleck, den Buddhismus strukturell sichtbar macht.

**Jüdische Bioethik** mit **Pikuach Nefesh** (Lebensrettung hat Vorrang):
Im Kontext von Notfallentscheidungen oder Trade-offs bietet diese Perspektive eine
elaborierte Kasuistik für Güterabwägungen, die in europäischem Recht keinen direkten
Kodex hat.

### 3.2 Operationalisierung: Mirror-School-Modus

Nicht-westliche Schulen operieren in Ebene 2 optional im **Mirror-School-Modus**:

```yaml
# Aktivierungsbedingung in Discourse-Orchestrator:
mirror_school_mode:
  trigger: cross_cultural_sensitivity == HIGH
  behavior:
    - Schule nimmt als REBUTTAL-Spezialist teil (nicht als PRO-Initiant)
    - Ziel: Blinde Flecken im dominanten westlichen Urteil benennen
    - Output: position_abstract + strongest_tension (kein volles Rebuttal-Protokoll)
    - Gewicht im MetaVerdict: nicht direkt in convergence_count,
      aber als "minority_dissent" im Syntheseprotokoll sichtbar
```

Das reduziert den Rechenaufwand für nicht-westliche Schulen auf **1 Inferenzschritt** statt
der vollen REBUTTAL-Sequenz, erhält aber ihre Stimme im Audit-Trail.

Das Aktivierungskriterium `cross_cultural_sensitivity` bestimmt, wann Mirror-Schulen
aktiviert werden:
- Immer bei: `domains ∈ {bioethics, family_law, end_of_life, minority_rights}`
- Konfigurierbar bei: `domains ∈ {ai_governance, data_protection}` (Default: ON)
- Abschaltbar bei: `domains ∈ {technical_compliance, infrastructure}` (Default: OFF)

### 3.3 Cross-Cultural Convergence als Stärkeindikator

Eine besonders starke Form des MetaVerdicts tritt auf, wenn Schulen aus verschiedenen
Kulturräumen konvergieren, **ohne voneinander zu wissen**:

```
Cross-Cultural Convergence:
  Islamische Ethik (La Darar):      PROHIBIT instrumentalization
  Kantische Ethik (Selbstzweck):    PROHIBIT instrumentalization
  Jüdische Bioethik (Kavod HaBriot):PROHIBIT instrumentalization
  Konfuzianismus (Ren/Humanität):   PROHIBIT instrumentalization

  → Convergence_score = 4 kulturell-unabhängige Quellen
  → MetaVerdict: CLEAR_CONSENSUS mit Cross-Cultural-Flag
```

Dies ist ein **qualitativ stärkeres** Urteil als ein Konsens innerhalb einer Kulturtradition.
Das positivrechtliche Grounding (GG Art. 1) fügt die rechtliche Bindung hinzu, aber der
eigentliche Stärkeausweis ist die kulturunabhängige Konvergenz.

---

## 4. Gesamtkomplexitätsanalyse

### 4.1 Naiver Volldiskurs vs. LDM

| Ansatz | Argumentschritte | Parallele Batch-Schritte | Latenz (P95) |
|---|---|---|---|
| Naiv: N=22, R=3 | ≈ 1.386 | - | ≈ 277 s |
| LDM Ebene 1 | 22 | 1 (vollparallel) | ≈ 0.2 s |
| LDM Ebene 2 | ≈ 114 | ≈ 8 Batches | ≈ 6 s |
| LDM Ebene 3 | 1 + DB | 1 | ≈ 0.05 s |
| **LDM gesamt** | **≈ 137** | **≈ 10 Batches** | **≈ 6–8 s** |

**Faktor 35–46 Effizienzgewinn** gegenüber dem naiven Ansatz bei vollem Erhalt der
epistemischen Fairness in Ebene 1.

### 4.2 Degraded-Mode für Latenz-kritische Umgebungen

Wenn P95-Latenz < 2 s gefordert ist (z.B. interaktive Anwendungen):

```
Fast-Discourse-Mode:
  Ebene 1: 22 Schulen → ABSTAIN-Filter → N_active Schulen (vollparallel, 0.2 s)
  Ebene 2: Nur Spannungsachse 1 (Kant ↔ Utilitarismus) + Domain-relevante Cluster
  Ebene 3: MetaVerdict aus Ebene-1-Konvergenz + Legal-Lookup
  Latenz:  ≈ 0.4–1.2 s
  Trade-off: Inter-Cluster-Reichhaltigkeit reduziert, Fairness erhalten
```

Der Fast-Discourse-Mode darf **nicht** einzelne Schulen vorab exkludieren —
er reduziert die Diskurstiefe in Ebene 2, nicht die Partizipation in Ebene 1.

---

## 5. Implikationen für die ThemisDB-Implementierung

### 5.1 Änderungen am EthicsSelectionRouter

Der `RouterConfig` sollte um einen `DiscourseMode`-Parameter erweitert werden:

```cpp
enum class DiscourseMode {
    SELECTION_ONLY,          ///< Aktuelles Verhalten: Top-N Vorselektion
    LAYERED_FULL,            ///< LDM: alle 3 Ebenen, P95 ≈ 6-8 s
    LAYERED_FAST,            ///< LDM Degraded: Ebene 1 + Achse 1 + Verdict
    EQUAL_PARTICIPATION      ///< Zukünftig: echter N×N-Diskurs (Forschungsmodus)
};
```

In `LAYERED_FULL`-Modus wird `top_n` ignoriert — alle geladenen Schulen nehmen an
Ebene 1 teil.

### 5.2 Erhalt der domain_overrides als Post-Hoc-Signal

Die `weight_boost`-Werte aus `domain_overrides` werden **nicht entfernt**. Sie wechseln ihre
Funktion: Von Pre-Selektion zu **Post-Hoc-Synthesegewichtung**.

Konkret: Wenn Kant in Domain `ai_governance` einen `weight_boost: 0.3` hat, bedeutet das
im LDM: Kants Konvergenz-Stimme in Ebene 3 wird mit Faktor `1 + 0.3 = 1.3` gewichtet,
**nachdem** alle Schulen gleichberechtigt gehört wurden. Das ist legitim: Es respektiert
den Deploymentkontext, ohne den Diskurs vorab zu manipulieren.

### 5.3 Mirror-School-Aktivierung im Discourse-Orchestrator

Der `DiscourseOrchestrator` (zu implementieren, Grundstruktur in `ethics_ai_types.h`
via `DiscourseRoundOutput`) benötigt eine `mirror_school_policy`-Konfiguration,
die domainbasiert die nicht-westlichen Schulen in den Mirror-School-Modus schaltet.

### 5.4 Audit-Trail-Anforderungen

Das `MetaVerdict` muss folgende Felder enthalten, um Transparenz und Rechtskonformität
(EU AI Act Art. 13 — Transparenzpflicht) sicherzustellen:

```
MetaVerdict:
  convergence_verdict:    string        // CLEAR_CONSENSUS | TENDENCY | CONTESTED | DISSENT
  convergence_score:      float         // [0, 1]
  participating_schools:  string[]      // Alle 22 Schulen, inkl. ABSTAIN
  dissenting_schools:     string[]      // Schulen mit abweichendem Verdict
  cross_cultural_flag:    bool          // true wenn ≥ 2 Kulturregionen konvergieren
  minority_dissent:       string[]      // Stellungnahmen der Mirror-Schulen
  legal_grounding:        LegalGroundingResult
  discourse_mode:         DiscourseMode // Welcher Modus wurde verwendet
```

---

## 6. Literatur

1. Habermas, J. (1981). *Theorie des kommunikativen Handelns*. Suhrkamp, Frankfurt.  
   Grundlage für ideale Diskursbedingungen (Partizipationsgleichheit).

2. Habermas, J. (1983). *Moralbewusstsein und kommunikatives Handeln*. Suhrkamp, Frankfurt.  
   Diskursethik: Universalisierungsprinzip als Zulässigkeitsbedingung für moralische Normen.

3. Kant, I. (1785). *Grundlegung zur Metaphysik der Sitten*. Riga.  
   Kategorischer Imperativ: Universalisierbarkeit von Maximen als Moralkriterium.

4. Rawls, J. (1971). *A Theory of Justice*. Harvard University Press, Cambridge, MA.  
   Schleier des Nichtwissens als Verfahrensgerechtigkeit — direkte Analogie zum equal initial weight.

5. al-Ghazali, A. H. (ca. 1095). *Ihya' Ulum al-Din* (Wiederbelebung der Religionswissenschaften).  
   Grundlage für Maslaha (Gemeinwohl) und Maqasid al-Shariah im islamischen Ethik-Asset.

6. Confucius / Kongzi (ca. 479 v. Chr.). *Lúnyǔ (Gespräche)*. Überl. Legge, J. (1861).  
   Grundlage für Rén (仁), Yì (義), Lǐ (禮) im Konfuzianismus-Asset.

7. Nagarjuna (ca. 150–250 n. Chr.). *Mūlamadhyamakakārikā*.  
   Philosophische Grundlage für Ahimsa und Karuna in der buddhistischen Ethik.

8. Bai, Y., et al. (2022). *Constitutional AI: Harmlessness from AI Feedback*. arXiv:2212.08073.  
   CAI-Grundlage für das ThemisDB CAI Ethics Integration Modul (`src/ai/cai_ethics_integration.cpp`).

9. European Parliament / Council of the EU (2024). *EU Artificial Intelligence Act* (Regulation (EU) 2024/1689).  
   Art. 13 (Transparenz), Art. 22 (Menschliche Aufsicht), Art. 9 (Risikomanagementsystem).

10. Rawls, J. (1993). *Political Liberalism*. Columbia University Press, New York.  
    Overlapping Consensus als Grundlage für kulturübergreifende Konvergenz (§3.3 dieses Papers).

11. Walzer, M. (1983). *Spheres of Justice*. Basic Books, New York.  
    Komplexe Gleichheit: Gerechtigkeitsprinzipien sind kontextabhängig, nicht universell.
    Relevant für die Frage, wie stark domain_overrides legitim sind.

12. An-Na'im, A. A. (1990). *Toward an Islamic Reformation: Civil Liberties, Human Rights, and International Law*.  
    Islamic ethics in the context of universal human rights — relevant für Cross-Cultural Convergence.

---

## 7. Offene Forschungsfragen

- **Empirische Kalibrierung:** Wie hoch ist der tatsächliche `convergence_score` bei realen
  Verwaltungsentscheidungen? Gibt es Dilemma-Klassen, die systematisch `DISSENT` erzeugen?
- **LLM-Bias in Ebene 1:** Auch bei gleichem Initialgewicht erzeugt ein westlich trainiertes
  LLM möglicherweise qualitativ unterschiedliche Positionstexte für nicht-westliche Schulen.
  Hier ist AdaLoRA-basiertes institutionelles Fine-Tuning (vgl. `docs/research/ml_enhancements_bibliography.md`)
  eine mögliche Kompensation.
- **Dynamische Clustergrenzen:** Die `taxonomy_class`-basierte Clusterung ist statisch.
  Ein dilemmaadaptives Clustering (basierend auf dem `cross_school_tensions`-Graph des
  konkreten Falls) könnte Ebene 2 weiter optimieren.
- **Mirror-School-Ausweitung:** Lateinamerikanische (Befreiungstheologie, Dussel),
  Pazifische (Māori-Ethik, Ubuntu aus Afrika) und weitere Perspektiven fehlen noch im
  Asset-Verzeichnis. Diese sollten als zukünftige Schulen geplant werden.

---

*Dieses Paper dokumentiert den aktuellen Architekturstand und dient als Grundlage für die
Implementierungsplanung des `DiscourseOrchestrator`-Moduls in `src/ethics_ai/`.*

*Modulbezug: `src/ethics_ai/`, `assets/ethics_ai/`, Issue: zukünftig zu öffnen im Rahmen
des Ethics-AI Roadmap-Track.*
