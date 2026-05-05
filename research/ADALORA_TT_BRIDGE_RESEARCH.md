# AdaLoRA ↔ Tensor-Train Bridge: Research & Benefit Analysis

**Status:** Internal Research — Implementation Planned Q2 2027  
**Stand:** 2026-05-05  
**Korrespondierender Code:** `include/training/adalora_tt_bridge.h`

---

## 1. Kernthese

AdaLoRA (Zhang et al. 2023) und die Tensor-Train-Zerlegung (Oseledets 2011) lösen  
dasselbe mathematische Problem: **kompakte Darstellung und adaptive Rangsteuerung von  
Gewichtsmatrizen**. Für 2D-Matrizen (alle LoRA-Adapterebenen) sind beide Formalismen  
**exakt äquivalent**. Diese Äquivalenz ermöglicht einen verlustfreien, zero-copy  
Datenaustausch zwischen ThemisDB-TT-Storage und dem AdaLoRA-Trainingspfad.

---

## 2. Mathematische Äquivalenz

### 2.1 AdaLoRA-Parameterisierung (Zhang et al. 2023, §3)

AdaLoRA parametrisiert das Gewichtsdelta einer Schicht als:

```
ΔW = P · Λ · Q^T

P ∈ ℝ^{d × r}   (linke Singulärvektoren — orthogonal)
Λ ∈ ℝ^{r × r}   (Diagonale der Singulärwerte λ₁ ≥ λ₂ ≥ … ≥ λ_r)
Q ∈ ℝ^{k × r}   (rechte Singulärvektoren — orthogonal)
```

Das Pruning-Verfahren nullt die unwichtigsten λᵢ aus, wodurch der effektive  
Rang r_eff ≤ r schrittweise sinkt.

### 2.2 Tensor-Train-Zerlegung für 2D-Matrizen

Für eine Matrix ΔW ∈ ℝ^{d × k} lautet die TT-Darstellung (d=2):

```
ΔW(i, j) = G₀[1, i, :] · G₁[:, j, 1]

G₀ ∈ ℝ^{1 × d × r}   (TT-Core 0 — der "B"-Kern)
G₁ ∈ ℝ^{r × k × 1}   (TT-Core 1 — der "A"-Kern)
```

Damit gilt die **direkte Identifikation**:

```
G₀[1, :, i] = P[:, i] · √λᵢ      (i = 1, …, r)
G₁[i, :, 1] = Q[:, i] · √λᵢ      (i = 1, …, r)
```

Oder in Matrixform: **G₀ = (P · √Λ)^T** (umgeformt) und **G₁ = √Λ · Q^T**.

Dieses Mapping ist bijektiv für Matrizen positiven (Semi-)definiten Rangs.

### 2.3 Konsequenzen der Äquivalenz

| AdaLoRA-Operation | TT-Äquivalent |
|-------------------|--------------|
| Singular-Value-Pruning (λᵢ → 0) | TT-Rounding mit ε-Schwelle |
| Rank-Reallokation | Adaptive Wahl von r_max pro Core |
| Importance-Score ‖P·Λ‖² · ‖Q‖² | TT-Core-Norm ‖G₀‖_F · ‖G₁‖_F |
| Forward-Pass: P·Λ·Q^T · x | TT-Kontraktion G₀ ×₁ G₁ · x |
| Checkpoint-Serialisierung | TT-Core-Speicherung in RocksDB |
| QLoRA (NF4-Basisgewichte) | NF4-TTQuantizer auf G₀, G₁ |

---

## 3. Nutzenanalyse für ThemisDB

### 3.1 Zero-Copy Adapter-Serving (Inference)

**Problem heute:** Adapter laden = deserialize JSON/binary → reconstruct float32 matrices  
→ inject into llama.cpp via `llama_lora_apply()`.

**Mit TT-Bridge:**  
AdaLoRA-Checkpoints werden direkt als TT-Cores in `TensorNetworkStorageEngine` gespeichert.  
`GgmlTensorBridge::mapAdapter()` liefert einen mmap-Pointer. `llama_lora_apply()` erhält  
den Pointer ohne eine einzige Kopie.

**Quantitative Schätzung (Adapter-Hot-Load, 7B, Rank 64):**

| Schritt | Klassisch | TT-Bridge |
|---------|-----------|-----------|
| Disk-Read | 50–200 ms | < 1 ms (mmap, cached) |
| Deserialise | 20–80 ms | 0 ms |
| Matrix-Reconstruct | 5–30 ms | 0 ms |
| llama_lora_apply | 2–10 ms | 2–10 ms |
| **Gesamt Hot-Load** | **77–320 ms** | **2–11 ms** |

Laut `src/training/PERFORMANCE_EXPECTATIONS.md` (L-3) ist das Ziel: Adapter Hot-Load  
7B Rank 64 ≤ 50 ms. TT-Bridge liefert **2–11 ms** — Ziel 4× unterschritten.

### 3.2 Adapter-Deduplication via TensorFingerprintGraph

Viele AdaLoRA-Adapter für verwandte Aufgaben teilen Singulärvektoren  
(z. B. zwei Legal-Domänen-Adapter mit ähnlicher Semantik teilen P-Spalten).

**TensorDeduplicationManager** erkennt diese Überlappung:

```
Adapter A:  ΔW_A = G₀_A · G₁_A   (r=16)
Adapter B:  ΔW_B = G₀_B · G₁_B   (r=16)

Wenn ‖G₀_A - G₀_B‖_F / ‖G₀_A‖_F < δ_sim (=0.001):
→ Speichere G₀ einmal + Delta-Core G₀_Δ = G₀_B - G₀_A
```

**Erwartete Speicherreduktion** für eine Sammlung von 100 domänenverwandten Adaptern:  
40–60% (analog zu TIES-Merging-Befunden, Yadav et al. 2023).

### 3.3 Unified Rank-Pruning: AdaLoRA ↔ TT-Rounding

**Bisher:** AdaLoRA-Pruning (nullt λᵢ) und TT-Rounding (Oseledets Algorithmus 2)  
sind separate Codepfade mit identischer Semantik.

**Bridge:** `AdaLoraTTBridge::exportToTT()` konvertiert den pruned Adapter in einen  
TT-Train mit r_eff Cores (λᵢ > ε werden beibehalten). `TensorTrainDecomposer::round()`  
kann dann während des Trainings als Drop-in für `reallocateRanks()` verwendet werden.

```
ThemisDB TT-Rounding:  round(train, ε) = TT-SVD mit Schwelle ε
AdaLoRA Pruning:       mask(Λ, budget) = setze λᵢ=0 für unwichtige i

→ Beide minimieren ‖ΔW - ΔW_approx‖_F unter Rang-Budget-Constraint
→ TT-Rounding ist global-optimal (SVD-basiert), AdaLoRA-Pruning ist greedy
→ Vorteil TT: garantiert ε-Fehlerbound; AdaLoRA: garantiert Rang-Budget
```

**Potenzial:** Kombination beider Mechanismen in `AdaLoraTTBridge::roundAndReallocate()`:
1. TT-Rounding → findet den optimalen globalen Rang-Cut
2. Ergebnis-Rang → wird als neues Budget für `reallocateRanks()` übergeben

### 3.4 TT-RAG mit Live-Adapter-Wechsel (FLARE)

Im FLARE-Verfahren entscheidet das Modell mid-generation, ob es Wissen nachladen soll.  
Mit der TT-Bridge kann es gleichzeitig **den Adapter wechseln**:

```
Inferenz-Schritt 73:
  ThemisDB-Abfrage: "Finde ähnlichsten Adapter für Thema 'Steuerrecht'"
  → TensorFingerprintGraph.findSimilar(current_adapter_train, k=1)
  → GgmlTensorBridge.mapAdapter(ctx, "steuerrecht_v3")
  → llama_lora_apply(llama_ctx, adapter_tensor, scale=1.0)
  → Inferenz läuft weiter mit neuem Adapter — kein Neustart
```

**Latenz:** ~5–15 ms für Adapter-Switch (vs. 300–2000 ms für Modell-Reload).

---

## 4. Implementierungsplan

### Phase 1 (Q2 2027): Konvertierung AdaLoRA → TT

- `AdaLoraTTBridge::exportToTT(adapter, layer)` — G₀=P·√Λ, G₁=√Λ·Q^T
- `AdaLoraTTBridge::importFromTT(train, layer)` — inverse Transformation
- Verlustfreie Round-trip-Tests (TTD-äquivalent, Fehler < ε_machine)

### Phase 2 (Q2 2027): Storage-Integration

- `AdaLoraTTBridge::storeAdapter(engine, adapter, tenant, name)` — alle Layer als TT-Cores
- `AdaLoraTTBridge::loadAdapter(engine, tenant, name)` → `AdaLoRAAdapter`
- `LoRACheckpointManager`-Integration: neues Backend `CheckpointBackend::TT_STORAGE`

### Phase 3 (Q3 2027): Deduplication und Serving

- `TensorDeduplicationManager`-Integration: auto-dedup beim `storeAdapter()`
- `GgmlTensorBridge::mapAdapter()` direkt aus TT-Storage (Phase-3-Spezifikation)
- Live-Adapter-Switch im FLARE-Retrieval-Callback

### Phase 4 (Q4 2027): Unified Rank-Control

- `AdaLoraTTBridge::roundAndReallocate()` — gemeinsamer SVD-Schritt
- Training-Loop-Integration: TT-Rounding als optionaler Schritt nach jedem Epoch
- Vergleichsstudie: AdaLoRA-Pruning vs. TT-Rounding vs. kombiniert

---

## 5. Risiken & Einschränkungen

### 5.1 Orthogonalitätsverlust

AdaLoRA erzwingt Orthogonalität von P und Q via Regularisierungsverlust  
(Zhang et al. 2023, Gl. 5). Die TT-Darstellung ist nicht inhärent orthogonal.

**Mitigation:** `exportToTT()` wendet vor der Konvertierung eine QR-Orthogonalisierung an.  
`importFromTT()` validiert ‖P^T·P - I‖_F < ε_orth = 1e-4.

### 5.2 Vorzeichen-Ambiguität der Singular-Zerlegung

Die SVD ist nicht eindeutig (Vorzeichen der Vektoren). Dies kann zu  
Fingerprint-Diskordanz im `TensorFingerprintGraph` führen.

**Mitigation:** Normalisierung auf positive erste Komponente (P[:,0] > 0) in `exportToTT()`.

### 5.3 Höherrangige Adapter (r > 64)

TT-Kontraktion mit r > 64 und d_modes=2 liefert r³-skalierte Kosten.  
Für r=64: 64³ = 262,144 FLops/innerProduct → noch performant.  
Für r=256: 256³ ≈ 16M FLops → HNSW-Grenze überschritten → KEEP empfohlen.

**Regel:** `AdaLoraTTBridge` empfiehlt TT-Storage nur für r ≤ 64  
(entspricht `TensorRoutingPolicy::max_lift_rank = 48–64`).

### 5.4 Gradientenfluss durch TT-Representation

Während des Trainings (Backpropagation) muss der Gradient durch die TT-Kontraktion  
propagiert werden. Dies erfordert eine differenzierbare TT-Schicht in der  
Trainings-Engine (ggml-Autograd oder libtorch).

**Mitigation:** TT-Storage ist nur für Inference/Serving; Training läuft weiterhin  
im klassischen AdaLoRA-Format. `exportToTT()` wird nur post-training aufgerufen.

---

## 6. Zusammenfassung Nutzen

| Metrik | Ohne Bridge | Mit Bridge |
|--------|-------------|------------|
| Adapter Hot-Load (7B, r=64) | 77–320 ms | 2–11 ms |
| Adapter-Storage für 100 Varianten | 100% | ~50% (dedup) |
| FLARE Adapter-Switch Latenz | 300–2000 ms | 5–15 ms |
| TT-RAG TTFT (mit Adapter-Inject) | 150–400 ms | 40–90 ms |
| Rank-Pruning-Qualität | Greedy (AdaLoRA) | Global-Optimal (TT-SVD) |

---

## 7. Referenzen

- Zhang, Q. et al. (2023). **AdaLoRA: Adaptive Budget Allocation for Parameter-Efficient  
  Fine-Tuning**. ICLR 2023. arXiv:2303.10512.
- Oseledets, I. V. (2011). Tensor-Train Decomposition. SIAM J. Sci. Comput. 33(5).  
  DOI:10.1137/090752142
- Holtz, S. et al. (2012). ALS in TT format. SIAM J. Sci. Comput. 34(2). DOI:10.1137/100818893
- Hu, E. J. et al. (2022). LoRA: Low-Rank Adaptation of Large Language Models. ICLR 2022.
- Dettmers, T. et al. (2023). QLoRA. NeurIPS 2023. arXiv:2305.14314
- Yadav, P. et al. (2023). TIES-Merging. NeurIPS 2023. arXiv:2306.01708
