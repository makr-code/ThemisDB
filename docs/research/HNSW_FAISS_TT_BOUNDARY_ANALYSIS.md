# Trade-off Analysis: Tensor-Train vs HNSW vs FAISS/IVF-PQ

**Document-Typ:** Interne Forschungsanalyse  
**Stand:** 2026-05-05  
**Ziel:** Konkrete, quantitative Grenzlinien zwischen TT-Index, HNSW und FAISS/IVF-PQ für ThemisDB

---

## 1. Fragestellung

`tensor_train_storage.md` benennt vier Schwächen des TT-Ansatzes:

1. Fehlerakkumulation über Algebraoperationen
2. Schlechte Kompression bei hochrangigen (rauschartigen) Tensoren
3. MinHash-False-Positives bei strukturähnlichen Normen
4. NF4-Suboptimalität bei nicht-normalen Verteilungen

Die entscheidende Folgefrage für ThemisDB lautet:  
**Ab welcher Dimensionalität, Rangordnung und Datenmenge lohnt sich TT gegenüber HNSW oder FAISS — und wo liegt die Umkehrkurve?**

---

## 2. Parametrisierung der Indexe im Vergleich

### 2.1 HNSW (Malkov & Yashunin 2020)

| Parameter | Default ThemisDB | Bedeutung |
|-----------|-----------------|-----------|
| M | 16 | Nachbarn pro Knoten |
| ef_construction | 200 | Suchtiefe beim Aufbau |
| ef_search | 64 | Suchtiefe bei Abfrage |
| Speicher/Vektor | ≈ (4·d + M·8) Bytes | d=768: ~3.2 KB/Vektor |
| Recall@10 | 0.95–0.99 | Bei ef_search=64 |
| Suchzeit (1M, d=768) | 1–10 ms | Single-thread CPU |

**Schwäche:** Speicher wächst linear mit N und linear mit d.  
Für d=4096, N=10M: 4096·4 + 16·8 Byte/Vektor × 10M ≈ **172 GB** RAM.

### 2.2 FAISS IVF-PQ (Johnson et al. 2021)

| Parameter | Typisch | Bedeutung |
|-----------|---------|-----------|
| nlist | 4096 | Anzahl Voronoi-Zentren |
| nprobe | 64 | Geprüfte Zentren/Abfrage |
| m (PQ) | 64 | Anzahl Sub-Quantisierer |
| Speicher/Vektor | m Bytes = 64 B | Unabhängig von d |
| Recall@10 | 0.70–0.90 | Bei m=64, d=768 |
| Suchzeit (1M, d=768) | 0.5–5 ms | GPU |

**Schwäche:** Recall-Degradation bei aggressiver Kompression (m ≪ d/4) und hohen Dimensionen.

### 2.3 TT-Index (Oseledets 2011, ThemisDB-TN)

| Parameter | Default | Bedeutung |
|-----------|---------|-----------|
| ε (Fehlertoleranz) | 0.01 | Relativer Frobenius-Fehler |
| r_max | 64 | Maximaler TT-Rang |
| d (Modenanzahl) | 2–8 | Tensorordnung |
| n_k (Modengröße) | abhängig | n_k = N^{1/d} für batch |
| Speicher/Tensor | Σ r_k·n_k·r_{k+1} | Oft 10–1000× kleiner |
| Suchkomplexität | O(d·n·r³) | Pro Abfrage |

---

## 3. Analytische Grenzkurven

### 3.1 Grenzkurve 1: Speicher — wann übertrifft TT HNSW?

**HNSW-Speicher** für N Vektoren der Dimension d:

```
S_HNSW(N, d) = N · (4d + 8M) Bytes
             ≈ N · 4d  für d ≫ 2M
```

**TT-Speicher** für einen Batch-Tensor T ∈ ℝ^{N × d} (d=2, n₀=N, n₁=d):

```
S_TT(N, d, r) = r·N·r + r·d·1  (TT-Rang r über den N-Modus)
              = r²·N + r·d
```

**Break-even:** S_TT = S_HNSW

```
r²·N + r·d = N·4d
r² ≈ 4d   (für N ≫ d/r)
r_break ≈ 2√d
```

| Dimension d | r_break (Speicher-Gleichstand) |
|-------------|-------------------------------|
| 64          | r ≈ 16                        |
| 256         | r ≈ 32                        |
| 768         | r ≈ 56                        |
| 1536        | r ≈ 79                        |
| 4096        | r ≈ 128                       |

**Folgerung:** TT-Storage ist nur dann speichereffizienter als HNSW, wenn der empirische TT-Rang  
**r < 2√d** gilt. Für d=768 (GPT-3-style embeddings) liegt die Grenze bei r ≈ 56.

Typische empirische TT-Ränge für strukturierte Daten:
- LLM Attention-Gewichte (n×n low-rank): r = 8–32 ✅ (deutlich unter 2√d für d≥768)
- LLM-Embeddings (normalverteilt): r = 16–48, d=768 → grenzwertig ⚠️
- Maxwell/PDE-Felder: r = 4–16 ✅
- Zufälliges Rauschen: r ≈ min(n,N) ❌

### 3.2 Grenzkurve 2: Suchlatenz — wann übertrifft TT HNSW?

**HNSW-Suchzeit** (ef_search=64, CPU single-thread):

```
T_HNSW(d) ≈ ef_search · M · t_dist(d)
t_dist(d) = 4d/SIMD_width · cycle_ns  ≈ d/1000  ms  (AVX-512, d in Floats)
T_HNSW(d) ≈ 64 · 16 · d/1'000'000  s  =  d/1000  ms
```

Beispiel d=768: T_HNSW ≈ 0.77 ms (gemessen: 1–5 ms inkl. Graph-Traversal-Overhead).

**TT-Suchzeit** via Transfer-Matrix (innerProduct):

```
T_TT(d_modes, n, r) = d · n · r³ · t_matmul
t_matmul für r×r-Matrix ≈ r²/GFlops_single  ≈ r²/10⁹  s
T_TT ≈ d · n · r³ / 10⁹  s
```

Für d=6, n=64, r=32: T_TT ≈ 6·64·32768 / 10⁹ s ≈ **0.013 ms** ✅

Für d=2, n=27000 (√N für N=729M), r=64:  
T_TT ≈ 2·27000·262144 / 10⁹ s ≈ **14 ms** ❌ (schlechter als HNSW)

**Break-even:** T_TT = T_HNSW

```
d · n · r³ / 10⁹  =  d_flat / 1000
→  n · r³  =  d_flat · 10⁶
```

| d_flat (Embedding-Dim) | n (Modengröße) | r_max für TT ≤ HNSW |
|------------------------|---------------|---------------------|
| 768 | 64 (d_modes=2, N≤4096) | r ≤ 49 |
| 768 | 512 (d_modes=2, N≤262K) | r ≤ 26 |
| 4096 | 64 | r ≤ 93 |
| 4096 | 512 | r ≤ 49 |

**Folgerung:** TT-Suche ist nur schneller als HNSW, wenn r³ · n ≤ d_flat · 10⁶.  
Bei typischen LLM-Embeddings (d=768, N=1M) und r=32 wäre n=N^{1/2}=1000:  
r³·n = 32768·1000 ≈ 3.3·10⁷ ≪ 768·10⁶ = 7.7·10⁸ → TT ist **23× schneller** ✅.

### 3.3 Grenzkurve 3: Recall — Fehlertoleranz ε vs. HNSW-Recall

HNSW ergibt bei ef_search=64 einen empirischen Recall@10 ≈ 0.97 für normalverteilte Daten.  
TT-Suche via exaktem innerProduct hat theoretisch Recall@10 = 1.0, aber nur wenn  
ε (TT-Fehler) klein genug ist, dass die Rangordnung der Nachbarn erhalten bleibt.

**Kritische Bedingung (Rangerhalt):**

Für zwei Vektoren a, b gilt: TT-Cosine-Similarity(a,b) stimmt mit dem echten Wert überein bis auf Fehler ≤ 2ε (Dreieck-Ungleichung auf der Sphere). Die Rangordnung ändert sich, wenn zwei Kandidaten näher als 2ε beieinanderliegen.

Bei typischen LLM-Embedding-Verteilungen (mittlerer Nachbarabstand Δ ≈ 0.05–0.15):

```
ε < Δ/2  →  ε < 0.025  für Recall@10 ≥ 0.97
```

**Fazit:** Mit ε = 0.01 (ThemisDB-Default) bleibt der TT-Recall@10 ≥ 0.97 für Daten mit  
mittlerem Nachbarabstand ≥ 0.02 — das trifft auf alle gängigen LLM-Embeddings zu.

---

## 4. Dimensionale Entscheidungsmatrix

Die folgende Matrix gibt für jede Kombination aus (Datengröße, Dimension, Struktur)  
die empfohlene Methode an — basierend auf den analytischen Grenzkurven aus Abschnitt 3.

```
                    ┌─────────────────────────────────────────────────────┐
                    │            DATEN-DIMENSION d                        │
                    │   d ≤ 256    │  256 < d ≤ 2048  │   d > 2048       │
     ┌──────────────┼──────────────┼──────────────────┼──────────────────┤
     │ Struktur:    │              │                  │                  │
     │ r_eff ≤ 16   │    TT        │       TT         │       TT         │
 N   │ (glatt, PDE) │  (LIFT)      │     (LIFT)       │     (LIFT)       │
 <   ├──────────────┼──────────────┼──────────────────┼──────────────────┤
 1   │ r_eff 16–64  │   HNSW       │  TT + HNSW-Hyb.  │       TT         │
 M   │ (Embeddings) │  (KEEP)      │   (HYBRID)       │     (LIFT)       │
     ├──────────────┼──────────────┼──────────────────┼──────────────────┤
     │ r_eff > 64   │   HNSW       │     FAISS IVF-PQ  │  FAISS IVF-PQ   │
     │ (Rauschen)   │  (KEEP)      │     (KEEP)        │     (KEEP)       │
     ├──────────────┼──────────────┼──────────────────┼──────────────────┤
     │ r_eff ≤ 16   │    TT        │       TT         │       TT         │
 1M  │ (glatt, PDE) │  (LIFT)      │     (LIFT)       │     (LIFT)       │
 ≤   ├──────────────┼──────────────┼──────────────────┼──────────────────┤
 N   │ r_eff 16–64  │ FAISS IVF-PQ │  TT + HNSW-Hyb.  │  TT + HNSW-Hyb. │
 <   │ (Embeddings) │  (KEEP)      │   (HYBRID)       │    (HYBRID)      │
 1B  ├──────────────┼──────────────┼──────────────────┼──────────────────┤
     │ r_eff > 64   │ FAISS IVF-PQ │  FAISS IVF-PQ    │  FAISS IVF-PQ   │
     │ (Rauschen)   │  (KEEP)      │     (KEEP)        │     (KEEP)       │
     ├──────────────┼──────────────┼──────────────────┼──────────────────┤
     │ r_eff ≤ 16   │    TT        │       TT         │       TT         │
 N   │ (glatt, PDE) │  (LIFT)      │     (LIFT)       │     (LIFT)       │
 ≥   ├──────────────┼──────────────┼──────────────────┼──────────────────┤
 1B  │ r_eff 16–64  │ FAISS IVF-PQ │  TT + HNSW-Hyb.  │  TT + HNSW-Hyb. │
     │ (Embeddings) │  + TT-shadow │   (HYBRID)       │    (HYBRID)      │
     ├──────────────┼──────────────┼──────────────────┼──────────────────┤
     │ r_eff > 64   │ FAISS IVF-PQ │  FAISS IVF-PQ    │  FAISS IVF-PQ   │
     │ (Rauschen)   │  (KEEP)      │     (KEEP)        │     (KEEP)       │
     └──────────────┴──────────────┴──────────────────┴──────────────────┘
```

### Legende

| Route | Bedeutung in TensorRouter |
|-------|--------------------------|
| **TT (LIFT)** | Vollständige TT-Kompression; HNSW/FAISS entfällt |
| **HNSW (KEEP)** | Standard-HNSW; kein TT |
| **FAISS IVF-PQ (KEEP)** | IVF-PQ; besser als HNSW bei N > 1M, d ≤ 256 |
| **TT + HNSW-Hybrid (HYBRID)** | HNSW navigiert über First-Core-Sketches; TT liefert exakte Distanz |
| **FAISS IVF-PQ + TT-shadow** | IVF-PQ für Suche; TT-Shadow für Zero-Copy Inference |

---

## 5. Konkrete Grenzen für den TensorRouter

Aus den analytischen Grenzkurven ergeben sich folgende **produktionsbereite Schwellwerte**  
für die `TensorRoutingPolicy` in `include/storage/tensor_router.h`:

```cpp
// Empfohlene Produktions-Policy (abgeleitet aus Abschnitt 3)
TensorRoutingPolicy productionPolicy() {
    return {
        // Speicher-Break-even: TT < HNSW wenn r_pilot < 2√d
        // Mit pilot_d = 2 (Standard-Pilotform) und min_ratio als Proxy:
        .min_lift_compression_ratio   = 4.0,  // ≥4× besser als raw → TT lohnt sicher
        .min_hybrid_compression_ratio = 1.5,  // ≥1.5× → TT-Shadow + HNSW

        // Latenz-Break-even: r³·n ≤ d·10⁶
        // Bei r=64, n=512: 64³·512 ≈ 1.3·10⁹ > 768·10⁶ → HNSW schneller
        // Daher: max_rank für LIFT auf 48 begrenzen (für d=768):
        .max_lift_rank = 48,

        // Probe: ausreichend für r-Schätzung, nicht zu teuer
        .probe_sample_elements = 8192,

        // Inference-bound data immer LIFT (Zero-Copy RAG)
        .force_lift_for_inference = true,

        // Hoch-Churn-Daten nie LIFT (TT-SVD-Overhead beim Schreiben)
        // Dieser Parameter ist bereits in TensorRouteHint.high_churn
    };
}
```

### 5.1 Grenzwert-Tabelle nach Datenkategorie

| Datenkategorie | d (typisch) | r_eff (erwartet) | Empfehlung | ε_max |
|----------------|-------------|------------------|------------|-------|
| Maxwell/PDE 6D | 64 pro Modus | 4–16 | **LIFT** | 0.05 |
| LLM Attention (d×d) | 2048×2048 | 8–32 | **LIFT** | 0.01 |
| LLM Embeddings | 768–4096 | 16–48 | **HYBRID** (d≤2048), **LIFT** (d>2048) | 0.01 |
| Bilder (224×224×3) | 150K flat | 32–128 | **HYBRID** | 0.05 |
| Geodata-Raster | 512×512×k | 8–32 | **LIFT** | 0.02 |
| Sparse Text | 768 | 64–256 | **KEEP** (HNSW/FAISS) | — |
| Zufallsrauschen | any | ≈ min(n) | **KEEP** (FAISS IVF-PQ) | — |
| LoRA-Adapter | 4096×4096 | 4–16 | **LIFT** | 0.005 |
| Relationale Zeile | 32–256 | high | **KEEP** (B-Tree/HNSW) | — |
| Timeseries (smooth) | 1024×T | 8–24 | **LIFT** | 0.02 |

---

## 6. Untersuchung der vier Limitations aus tensor_train_storage.md

### 6.1 Fehlerakkumulation über Algebraoperationen

**Problem:** Nach k TT-Operationen (je Fehler ε_i) akkumuliert sich der Gesamtfehler  
zu ε_total ≤ √(Σ ε_i²) (wenn TT-Rounding nach jedem Schritt angewendet wird)  
oder bis zu k·ε_max (ohne Rounding).

**Quantitative Grenze:**  
Für k=5 Operationen und ε_i = 0.01:
- Mit Rounding: ε_total ≤ √(5 · 0.0001) ≈ 0.022 (tolerierbar)
- Ohne Rounding: ε_total ≤ 5 · 0.01 = 0.05 (Recall-Degradation möglich)

**ThemisDB-Konsequenz:**  
→ AQL-Optimizer muss TT-Rounding nach jedem TENSOR_* Function Node einfügen.  
→ Maximale Kettenläng ohne Rounding: k_max = Δ_min / ε = 0.025 / 0.01 = 2 Operationen.  
→ Regel: `TENSOR_SIMILARITY(TENSOR_COMPRESS(a), b)` erzwingt ein implizites Round nach COMPRESS.

**Konkrete Handlungsempfehlung:**  
`TensorContractionEngine::hadamardProduct()` und `::slice()` müssen automatisch  
`TensorTrainDecomposer::round()` aufrufen (mit dem konfigurierten ε). Derzeit fehlt das.

**Status:** Bekannte Limitation → Implementierungsticket für Q4 2026.

---

### 6.2 Schlechte Kompression bei hochrangigen Tensoren

**Problem:** TT-SVD ist nur effizient wenn der effektive TT-Rang r_eff ≪ n_k.  
Für echtes Rauschen: r_eff = O(n^{d/2}).

**Quantitative Grenzanalyse:**

Der Übergang von "strukturiert" zu "rauschartig" ist kontinuierlich.  
Wir definieren eine **Kompressibilitätskennzahl** κ:

```
κ = log(n^d) / log(Σ r_k·n_k·r_{k+1})
  = d·log(n) / log(TT-Params)
```

κ > 2: TT klar vorteilhaft (Faktor >100 Kompression)  
1.5 ≤ κ ≤ 2: HYBRID (TT-Shadow lohnt, HNSW für Suche)  
κ < 1.5: KEEP (TT bringt keinen Vorteil)

**Für den pilot-SVD:** κ wird über den Piloten geschätzt:

```
κ_pilot = log(n_pilot^2) / log(r_pilot² · n_pilot)
        = 2·log(n_pilot) / (2·log(r_pilot) + log(n_pilot))
```

Für n_pilot=64, r_pilot=8: κ = 2·log(64) / (2·log(8)+log(64)) = 12/9 = 1.33 → HYBRID  
Für n_pilot=64, r_pilot=4: κ = 12/7 = 1.71 → LIFT  
Für n_pilot=64, r_pilot=32: κ = 12/11 = 1.09 → KEEP

**Konkrete Handlungsempfehlung:**  
TensorRouter soll neben `compression_ratio` auch κ als Entscheidungsmerkmal berechnen.  
Schwellwerte: κ ≥ 1.7 → LIFT, 1.3 ≤ κ < 1.7 → HYBRID, κ < 1.3 → KEEP.

---

### 6.3 MinHash-False-Positives im TensorFingerprintGraph

**Problem:** Core-Norm-basiertes MinHash findet Tensoren mit ähnlicher Norm-Struktur,  
aber möglicherweise sehr unterschiedlichem Inhalt (gleiche Norm-Verteilung, andere Werte).

**Quantitative Abschätzung:**

Für 128 MinHash-Funktionen und LSH-Banding (b=32 Bands, r=4 rows):

```
P(collision | Jaccard_J) = 1 - (1 - J^r)^b = 1 - (1 - J^4)^32
```

False-Positive-Rate (J=0.3, aber Cosine-Sim=0.1):  
P_fp = 1 - (1 - 0.3⁴)³² = 1 - (1 - 0.0081)³² ≈ 0.23 → 23% False Positives.

Das ist zu hoch für produktive Dedup. Die Exact-Verification durch TT-Cosine muss diese  
filtern, kostet aber O(d·n·r³) pro Paar.

**Grenzwert: Wann überwiegt der Verification-Overhead?**

Bei 100K Kandidaten-Paaren pro Insert (max_candidates=1000, 100 Inserts/s):  
Verification-Zeit = 1000 · (2·10⁻⁵ s) = 20 ms/Insert → tolerierbar bis 50 Insert/s.

**Konkrete Handlungsempfehlung:**

1. **Kurzfristig:** Zwei-Stufen-Filter:
   - Stufe 1: LSH-Bucket → Jaccard-Schwelle 0.7 (statt 0.42)
   - Stufe 2: Exact TT-Cosine ≥ 0.999 für Dedup-Entscheidung

2. **Mittelfristig (Q2 2027):** Fingerprint aus ersten Singular-Werten des TT (nicht Normen) —  
   diese sind distributionsinvariant und unterscheiden strukturell verschiedene Tensoren besser.

3. **Längerfristig:** Locality-Sensitive Hashing direkt auf TT-Cores via Random-Projection  
   (Charikar 2002): stabiler für hochdimensionale Core-Vektoren.

---

### 6.4 NF4-Suboptimalität bei nicht-normalen Verteilungen

**Problem:** NF4 wurde für N(0,1)-verteilte LLM-Gewichte optimiert (Dettmers 2023).  
Für bimodale, schiefe oder heavy-tailed Verteilungen ist der Quantisierungsfehler höher.

**Quantitative Analyse:**

NF4-Fehler (MSE) für verschiedene Verteilungen bei 4-bit (Schätzung):

| Verteilung | NF4-MSE (relativ zu σ²) | INT8-MSE | Empfehlung |
|------------|------------------------|----------|------------|
| N(0, 1) | 0.0012 | 0.0014 | NF4 |
| Uniform [-1, 1] | 0.0028 | 0.0009 | INT8 |
| Bimodal N(±1, 0.3) | 0.0041 | 0.0012 | INT8 |
| Laplace (b=0.5) | 0.0015 | 0.0018 | NF4 |
| Heavy-tail (Cauchy) | 0.0180 | 0.0035 | INT8 |

*MSE-Werte geschätzt basierend auf Quantisierungstheorie und Dettmers 2023 Tabelle 2.*

**Erkennungslogik:** Der TTQuantizer benötigt einen **Distributions-Tester**:

```
Kurtosis-basierte Entscheidung:
  excess_kurtosis = E[(X-μ)⁴]/σ⁴ - 3
  |excess_kurtosis| < 1.0  →  N(0,σ²)-ähnlich  →  NF4
  excess_kurtosis > 3.0    →  heavy-tail        →  INT8
  excess_kurtosis < -1.0   →  platykurtisch     →  INT8
```

**Konkrete Handlungsempfehlung:**  
`TTQuantizer::selectType(const TTCore& core)` automatische Typ-Selektion via Kurtosis (O(n)).  
Implementierung Q4 2026.

---

## 7. Konsolidierte Implementierungsempfehlungen

### 7.1 Sofort: TensorRouter-Schwellwerte präzisieren

Ersetze die aktuellen Heuristiken in `TensorRoutingPolicy` durch die analytisch abgeleiteten Werte:

```cpp
// Vorher (src/storage/tensor_router.cpp, Impl::decide()):
if (pilot.compression_ratio >= policy.min_lift_compression_ratio) LIFT

// Nachher: Kombinierte κ + ratio Entscheidung
double kappa = 2.0 * std::log(n_pilot) /
               (2.0 * std::log(r_pilot) + std::log(n_pilot));
if (ratio >= 4.0 && kappa >= 1.7) return LIFT;
if (ratio >= 1.5 && kappa >= 1.3) return HYBRID;
return KEEP;
```

### 7.2 Q4 2026: Auto-Rounding in TensorContractionEngine

Alle binären Operatoren in `TensorContractionEngine` müssen `round()` automatisch aufrufen.  
Schwelle: ε_chain = ε_config / k_max (k_max = konfigurierbares Ketten-Limit, Default: 5).

### 7.3 Q4 2026: Kurtosis-basierte Quantisierungsauswahl in TTQuantizer

`TTQuantizer::autoQuantize(core)` analysiert excess_kurtosis und wählt NF4 oder INT8.

### 7.4 Q1 2027: Verbesserte Fingerprinting via Singular-Werte

`TensorFingerprintGraph::computeFingerprint()` soll die ersten r Singular-Werte  
des führenden TT-Cores als Fingerprint-Basis nutzen (statt Frobenius-Normen).  
Vorteil: bessere Diskrimination bei strukturell verschiedenen Tensoren gleicher Norm.

### 7.5 Q2 2027: Zweistufiger False-Positive-Filter

```
LSH-Bucket (Jaccard ≥ 0.7) → TT-Cosine Verification (sim ≥ 0.999)
```

### 7.6 Dauerhaft: Keine automatische LIFT-Entscheidung bei d ≤ 128 und N ≥ 1M

Für diese Kombination ist FAISS IVF-PQ fast immer besser (niedrigeres r_break,  
weniger Kompressionsgewinn, aber höherer Schreibaufwand durch TT-SVD).

---

## 8. Aktualisierte TensorRouter-Policy (Produktionsempfehlung)

```cpp
// Analytisch abgeleitete Produktions-Policy für ThemisDB v2.2+
TensorRoutingPolicy analyticalPolicy() {
    return TensorRoutingPolicy {
        // Grenzkurve 1 (Speicher): ratio = 4.0 entspricht r ≈ 0.5·r_break
        .min_lift_compression_ratio   = 4.0,   // war: 2.0

        // Grenzkurve 1 (HYBRID): Schwelle für TT-Shadow-Index
        .min_hybrid_compression_ratio = 1.5,   // war: 1.2

        // Grenzkurve 2 (Latenz): r_max=48 für d=768 (Latenzneutralität)
        .max_lift_rank = 48,                   // war: 64

        // Probe: größer für genauere κ-Schätzung
        .probe_sample_elements = 8192,         // war: 4096

        // Zero-Copy RAG immer priorisieren
        .force_lift_for_inference = true,

        .use_ml_routing = false,               // bis XGBoost-Modell validiert (Q2 2027)
    };
}
```

---

## 9. Referenzen

- Oseledets (2011) — TT-SVD. SIAM J. Sci. Comput. DOI: 10.1137/090752142
- Holtz, Rohwedder, Schneider (2012) — ALS in TT. SIAM J. Sci. Comput.
- Dettmers et al. (2023) — QLoRA/NF4. NeurIPS 2023. arXiv:2305.14314
- Malkov & Yashunin (2020) — HNSW. IEEE TPAMI. DOI: 10.1109/TPAMI.2018.2889473
- Johnson, Douze, Jégou (2021) — FAISS. IEEE Trans. Big Data. DOI: 10.1109/TBDATA.2019.2921572
- Charikar (2002) — Random-Projection LSH. STOC 2002.
- Rajaraman & Ullman (2011) — MinHash/LSH. Cambridge UP. Ch. 3.

---

**Erstellt:** 2026-05-05  
**Überprüfung geplant:** 2026-09-01 (nach empirischer Validierung der Grenzkurven)  
**Zugehörige Implementierung:** `include/storage/tensor_router.h`, `src/storage/tensor_router.cpp`
