# Parameter-Efficient Fine-Tuning (PEFT) Framework Integration für ThemisDB

**Stand:** 19. Dezember 2025  
**Version:** 1.1.0  
**Kategorie:** LLM Training

---

## Zusammenfassung

Dieses Dokument analysiert verfügbare **Parameter-Efficient Fine-Tuning (PEFT)** Methoden und deren Integration in ThemisDB. PEFT-Methoden ermöglichen das Trainieren von LLMs mit minimalen Ressourcen durch Anpassung nur eines kleinen Teils der Parameter.

**Wichtigste PEFT-Methoden:**
- **LoRA** (Low-Rank Adaptation) - Hauptfokus, am weitesten verbreitet
- **QLoRA** - LoRA mit 4-bit Quantisierung
- **AdaLoRA** - Adaptive LoRA mit dynamischem Rank
- **IA³** (Infused Adapter by Inhibiting and Amplifying Inner Activations)
- **Prompt Tuning** - Lernt nur Prompt Embeddings
- **P-Tuning** - Continuous Prompt Tuning
- **Prefix Tuning** - Trainierbare Prefixes pro Layer

Dieses Dokument ergänzt die geplante Llama.cpp Integration für Inferenz (v1.3.0).

**Status Quo (v1.2.0):**
- 🚧 **Inferenz**: Llama.cpp Integration geplant für v1.3.0 (siehe Roadmap)
- ✅ **vLLM Support**: Multi-LoRA Serving Dokumentation vorhanden (siehe `VLLM_MULTI_LORA_INTEGRATION.md`)
- ✅ **Datenexport**: JSONL Exporter mit Adapter-Metadata vollständig implementiert
- ✅ **Sharding**: Horizontales Sharding mit Raft-Konsensus, WAL-Replikation, Auto-Rebalancing
- ❌ **Training**: Noch keine direkte Framework-Integration

**Ziel:** Integration eines PEFT Training Frameworks (LoRA/QLoRA/etc.) für:
1. **Inline Training** direkt aus ThemisDB Multi-Model Daten (Graph, Vector, Relational)
2. **Distributed Training** über ThemisDB Shards
3. **Horizontale Adapter-Bereitstellung** mit Load Balancing und Failover

---

## 1. Was haben wir bereits?

### 1.1 Bestehende Infrastruktur (v1.2.0)

**Hinweis:** Die Llama.cpp Integration für native Inferenz ist für v1.3.0 geplant (siehe ROADMAP.md). Die hier beschriebene Training-Integration kann parallel entwickelt werden und ist unabhängig von Llama.cpp.

#### ✅ JSONL LLM Exporter (`include/exporters/jsonl_llm_exporter.h`) - IMPLEMENTIERT
```cpp
// Vollständig implementiert:
- Instruction Tuning, Chat Completion, Text Completion Formate
- Weighting-Strategien (freshness, length-based)
- Quality Filtering (min/max length, duplicates)
- Schema Validation (Outlines-kompatibel)
- LoRA Adapter Metadata Tracking
- vLLM-spezifische Konfiguration
```

**Capabilities:**
- Export von ThemisDB → JSONL für Training
- Automatische Gewichtung nach Aktualität
- Metadata für LoRAExchange.ai Standard
- Schema-validierte Samples (100% JSON Schema konform)

#### ✅ Streaming API (`docs/api/STREAMING_JSONL_TRAINING.md`)
```cpp
// Bereits implementiert (Commit 6b4129b):
POST /api/export/jsonl_llm/stream
- Chunked Transfer Encoding
- On-demand Streaming (kein vollständiger Export)
- Backpressure Support
- Batch-wise DB Zugriff
```

**Use Case:**
```python
# PyTorch/HuggingFace IterableDataset
dataset = ThemisDBStreamDataset(
    base_url='http://themisdb:8765',
    query_params={'theme': 'Rechtssprechung', 'from_date': '2020-01-01'}
)
trainer.train(dataset)  # Direkt aus DB, kein lokaler Export
```

#### ✅ Adapter Registry (`docs/exporters/LORA_ADAPTER_METADATA.md`)
```cpp
struct AdapterMetadata {
    string adapter_id;           // "legal-qa-v1"
    string adapter_version;      // "1.2.0"
    string base_model_name;      // "mistralai/Mistral-7B-v0.1"
    string task_type;            // "question-answering"
    string domain;               // "legal"
    
    struct TrainingConfig {
        int lora_rank;           // 8, 16, 32
        double lora_alpha;       // 16.0
        double lora_dropout;     // 0.1
        vector<string> target_modules;  // ["q_proj", "v_proj", ...]
    } training_config;
}
```

**Integration Points:**
- Metadata wird während Export gespeichert
- Training-Framework liest Metadata
- vLLM nutzt Metadata für Serving

### 1.2 Was fehlt noch?

❌ **Training Framework Integration** - Kein direkter Adapter/Wrapper  
❌ **Python Training Library** - Kein `themisdb-trainer` Package  
❌ **C++ Training Adapter** - Keine DLL/SO für natives Training  
❌ **Orchestration** - Kein automatisierter Train→Deploy Workflow  

---

## 2. PEFT-Methoden im Detail

### 2.1 Übersicht Parameter-Efficient Fine-Tuning Methoden

| Methode | Parameter % | VRAM | Training Speed | Inference Speed | Use Case |
|---------|-------------|------|----------------|-----------------|----------|
| **Full Fine-Tuning** | 100% | Hoch (48GB+) | Langsam | Standard | Maximum Quality |
| **LoRA** | 0.1-1% | Niedrig (12GB) | Mittel | Standard | **Best Balance** ⭐ |
| **QLoRA** | 0.1-1% | **Sehr Niedrig (8GB)** | Langsam | Standard | Consumer GPUs |
| **AdaLoRA** | 0.1-1% | Niedrig (12GB) | Mittel | Standard | Automatic Tuning |
| **IA³** | 0.01% | **Minimal (6GB)** | **Schnell** | **Schneller** | Lightweight Tasks |
| **Prompt Tuning** | 0.001% | Minimal (4GB) | Sehr Schnell | Standard | Few-Shot Learning |
| **Prefix Tuning** | 0.01% | Minimal (6GB) | Schnell | Standard | Task-Specific |
| **P-Tuning v2** | 0.1% | Niedrig (8GB) | Mittel | Standard | NLU Tasks |

### 2.2 LoRA (Low-Rank Adaptation) ⭐ **Empfohlen**

**Paper:** LoRA: Low-Rank Adaptation of Large Language Models (Microsoft, 2021)  
**Idee:** Fügt trainierbare Low-Rank Matrizen zu frozen Modell-Gewichten hinzu

**Mathematik:**
```
W' = W₀ + ΔW = W₀ + BA
wobei B ∈ ℝᵈˣʳ, A ∈ ℝʳˣᵏ, r ≪ min(d,k)
```

**Modell-Kompatibilität:** ⚠️ **NICHT Modellübergreifend**

**Wichtig:** LoRA-Adapter sind **spezifisch für ein Base-Model** und **NICHT** zwischen verschiedenen LLMs transferierbar:

```
❌ NICHT Kompatibel:
- LoRA trainiert auf Llama-2-7B → funktioniert NICHT mit Mistral-7B
- LoRA trainiert auf GPT-2 → funktioniert NICHT mit Llama
- LoRA trainiert auf Llama-7B → funktioniert NICHT mit Llama-13B

✅ Kompatibel:
- LoRA trainiert auf Llama-2-7B → funktioniert mit Llama-2-7B ✓
- Verschiedene LoRAs auf GLEICHEM Base-Model austauschbar
```

**Gründe für Inkompatibilität:**
1. **Dimensionen:** Jedes Model hat unterschiedliche Layer-Größen (d, k)
2. **Architektur:** Unterschiedliche Layer-Namen (q_proj vs. query, etc.)
3. **Tokenizer:** Verschiedene Vocabulary-Sizes
4. **Semantik:** Weight-Space ist nicht aligned zwischen Models

**❓ Wie macht vLLM das, dass Adapter mit "allen Modellen" funktionieren?**

**Klärung:** vLLM macht Adapter **NICHT** modellübergreifend kompatibel. vLLM erlaubt:

1. **Multi-LoRA Serving auf EINEM Base-Model:**
   ```
   vLLM Server
   ├─ Base Model: Mistral-7B (geladen in VRAM)
   └─ Adapter Pool:
      ├─ legal-qa-v1 → NUR für Mistral-7B ✓
      ├─ medical-v1 → NUR für Mistral-7B ✓
      └─ code-gen-v1 → NUR für Mistral-7B ✓
   ```

2. **Dynamische Adapter-Auswahl pro Request:**
   ```python
   # Request 1: Legal query
   client.completions.create(
       model="mistralai/Mistral-7B-v0.1",
       prompt="Legal question...",
       extra_body={"lora_name": "legal-qa-v1"}  # Wählt Adapter aus Pool
   )
   
   # Request 2: Medical query (GLEICHES Base-Model!)
   client.completions.create(
       model="mistralai/Mistral-7B-v0.1",
       prompt="Medical question...",
       extra_body={"lora_name": "medical-v1"}  # Anderer Adapter, GLEICHES Model
   )
   ```

3. **Effiziente Batching:**
   - vLLM kann Requests mit verschiedenen Adaptern im gleichen Batch verarbeiten
   - PagedAttention ermöglicht Sharing des Base-Model KV-Cache
   - Adapter-spezifische Gewichte werden nur für betroffene Tokens geladen

**vLLM's Strategie für "Universalität":**

| Aspekt | vLLM Ansatz | Limitation |
|--------|-------------|------------|
| **Multi-Base-Model Support** | Kann verschiedene Base-Models hosten (Llama, Mistral, GPT-J) | Jedes Base-Model braucht eigene Adapter |
| **Multi-Adapter auf 1 Base-Model** | ✅ Ja, unbegrenzt viele Adapter pro Base-Model | Adapter sind an Base-Model gebunden |
| **Cross-Model Adapter Sharing** | ❌ Nicht möglich | Dimensionen inkompatibel |
| **Dynamic Adapter Loading** | ✅ Ja, Adapter können zur Laufzeit ge-/entladen werden | Nur für kompatibles Base-Model |

**Was vLLM NICHT kann:**
```python
# ❌ FEHLER: Llama LoRA auf Mistral Base-Model
client.completions.create(
    model="mistralai/Mistral-7B",
    extra_body={"lora_name": "llama-legal-adapter"}  # ← Inkompatibel!
)
# → Dimension mismatch: Llama LoRA (4096) ≠ Mistral (4096 aber andere Architektur)
```

**Verbesserungsvorschläge für ThemisDB Strategie:**

1. **Multi-Base-Model Registry:** 
   ```cpp
   struct AdapterRegistry {
       map<string, vector<AdapterInfo>> adapters_by_base_model;
       // Gruppierung: "mistral-7b" → [legal-v1, medical-v1]
       //              "llama-3-8b" → [code-v1, chat-v1]
   };
   ```

2. **Automatische Base-Model Erkennung:**
   ```cpp
   // Verhindert falsche Adapter-Zuordnung
   bool validateAdapterCompatibility(
       const string& adapter_id,
       const string& base_model_id
   ) {
       auto adapter_meta = registry.getAdapter(adapter_id);
       if (adapter_meta.base_model_name != base_model_id) {
           throw IncompatibleAdapterException(
               "Adapter " + adapter_id + " requires " + 
               adapter_meta.base_model_name + " but got " + base_model_id
           );
       }
       return true;
   }
   ```

3. **Fallback-Strategie für Model-Migration:**
   ```cpp
   // Wenn Base-Model gewechselt wird
   struct ModelMigrationPlan {
       string old_base_model;     // "mistral-7b"
       string new_base_model;     // "llama-3-8b"
       
       // Adapter müssen re-trainiert werden
       vector<AdapterRetrainingTask> adapter_tasks;
       
       // Aber: Training-Daten können wiederverwendet werden
       bool reuse_training_data = true;
       bool reuse_hyperparameters = true;  // LoRA rank, alpha, etc.
   };
   ```

**Best Practice (korrigiert):**
- ✅ Ein Base-Model wählen und dabei bleiben
- ✅ Mehrere LoRAs für verschiedene Domänen auf GLEICHEM Base-Model
- ✅ Bei Base-Model Wechsel: **Alle Adapter re-trainieren** (aber Training-Daten wiederverwenden)
- ✅ vLLM für **Multi-Adapter auf 1 Base-Model**, **NICHT** für Cross-Model Adapter
- ✅ Separate vLLM-Instanzen für verschiedene Base-Models (z.B. eine für Mistral, eine für Llama)

**Eigenschaften:**
- ✅ Trainiert nur ~0.1-1% der Parameter (z.B. 4M statt 7B)
- ✅ Memory: 3-4x weniger als Full Fine-Tuning
- ✅ Inference: Keine Latenz-Overhead (A und B können mit W₀ gemerged werden)
- ✅ Multi-Adapter: Verschiedene LoRAs für einen Base-Model
- ✅ Swappable: Adapter können zur Laufzeit gewechselt werden (auf GLEICHEM Base-Model)

**Hyperparameter:**
```python
lora_config = {
    'r': 8,              # Rank (4, 8, 16, 32, 64)
    'lora_alpha': 16,    # Scaling factor (oft 2*r)
    'lora_dropout': 0.1, # Dropout rate
    'target_modules': ['q_proj', 'v_proj', 'k_proj', 'o_proj']  # Welche Layer
}
```

**Wann nutzen:**
- Production Use Cases (am ausgereiftesten)
- Multi-Domain Adapters (legal, medical, etc.) auf GLEICHEM Base-Model
- Wenn Inference-Speed wichtig ist
- Standard für die meisten Anwendungen

### 2.3 QLoRA (Quantized LoRA)

**Paper:** QLoRA: Efficient Finetuning of Quantized LLMs (Dettmers et al., 2023)  
**Idee:** LoRA + 4-bit Quantisierung des Base Models

**Eigenschaften:**
- ✅ **Extrem Memory-Efficient:** 7B Model in 8GB VRAM trainierbar
- ✅ Nutzt NF4 (Normal Float 4-bit) Quantisierung
- ✅ Double Quantization für weitere Savings
- ✅ Paged Optimizers (nutzt CPU RAM als Backup)
- ❌ ~30% langsamer als LoRA (durch Dequantization)
- ❌ Numerische Instabilitäten möglich

**Memory Comparison:**
```
Model Size: Mistral-7B

Full FP16:  14 GB VRAM (7B * 2 bytes)
LoRA FP16:  12 GB VRAM (frozen model + gradients)
QLoRA 4bit: 6-8 GB VRAM (quantized + adapters)
```

**Wann nutzen:**
- Consumer GPUs (RTX 3090, 4090 mit 24GB)
- Cloud-Kosten minimieren
- Proof-of-Concepts / Experimente
- Wenn Memory kritischer als Speed

### 2.4 AdaLoRA (Adaptive LoRA)

**Paper:** Adaptive Budget Allocation for Parameter-Efficient Fine-Tuning (Zhang et al., 2023)  
**Idee:** Rank r wird dynamisch per Layer angepasst

**Eigenschaften:**
- ✅ Automatisches Rank-Tuning (kein manuelles r-Tuning nötig)
- ✅ Prunes unwichtige Singular Values
- ✅ Bessere Accuracy bei gleichem Parameter-Budget
- ❌ Komplexer Training Loop
- ❌ Weniger verbreitet als LoRA

**Rank Allocation:**
```python
# AdaLoRA lernt automatisch:
# - Attention Layers: r=16 (wichtig)
# - FFN Layers: r=4 (weniger wichtig)
# statt manuell r=8 überall
```

**Wann nutzen:**
- Maximale Accuracy bei fixem Parameter-Budget
- Wenn Hyperparameter-Tuning Zeit/Ressourcen kostet
- Research / Experimente

### 2.5 IA³ (Infused Adapter)

**Paper:** Few-Shot Parameter-Efficient Fine-Tuning (Liu et al., 2022)  
**Idee:** Multiplicative scaling vectors statt additive Low-Rank Matrizen

**Eigenschaften:**
- ✅ **Minimal Parameters:** 0.01% (10x weniger als LoRA)
- ✅ **Fast:** Keine Matrix-Multiplikation, nur Element-wise scaling
- ✅ Inference-Overhead: ~0% (pure multiplication)
- ❌ Weniger expressive als LoRA
- ❌ Nur für simple Tasks geeignet

**Mathematik:**
```
y = W₀x ⊙ lᵥ   (für Values in Attention)
y = W₀x ⊙ lₖ   (für Keys in Attention)
y = W₀x ⊙ lₓ   (für FFN)
```

**Parameter Count:**
```
LoRA:  d × r + r × k  (z.B. 4096*8 + 8*4096 = 65k pro Layer)
IA³:   d              (z.B. 4096 pro Layer)
→ IA³ hat ~16x weniger Parameter
```

**Wann nutzen:**
- Sehr einfache Tasks (Classification, NER)
- Extreme Resource Constraints (Edge Devices)
- Wenn Speed > Accuracy

### 2.6 Prompt Tuning

**Paper:** The Power of Scale for Parameter-Efficient Prompt Tuning (Lester et al., 2021)  
**Idee:** Trainiert nur Prompt-Embeddings, Model bleibt komplett frozen

**Eigenschaften:**
- ✅ **Minimalste Parameter:** 0.001% (z.B. 20 tokens * 4096 dim = 80k)
- ✅ Sehr schnelles Training
- ✅ Perfekt für Multi-Task Learning (1 Prompt pro Task)
- ❌ Nur effektiv bei sehr großen Models (>10B)
- ❌ Bei kleinen Models (<1B) fast nutzlos

**Beispiel:**
```
Original Input:  "Translate to German: Hello"
Prompt Tuning:   [P1][P2][P3]...[P20] "Translate to German: Hello"
                 └─────trainierbar─────┘ └────────frozen──────────┘
```

**Wann nutzen:**
- Sehr große Models (>11B Parameter)
- Multi-Task Scenarios
- Wenn Model-Weights nicht verändert werden dürfen

### 2.7 Prefix Tuning

**Paper:** Prefix-Tuning (Li & Liang, 2021)  
**Idee:** Fügt trainierbare Prefixes zu jedem Transformer-Layer hinzu

**Eigenschaften:**
- ✅ Mehr expressive als Prompt Tuning (prefix per layer)
- ✅ ~0.01% Parameter
- ✅ Gut für Generation Tasks
- ❌ Inference Overhead (längere Sequences)

**Architektur:**
```
Layer 1: [prefix₁] + Input → Output₁
Layer 2: [prefix₂] + Output₁ → Output₂
...
Layer N: [prefixₙ] + Outputₙ₋₁ → Final Output
```

**Wann nutzen:**
- Generation Tasks (Text, Code)
- Wenn LoRA zu viel Memory braucht
- Multi-Task mit shared backbone

### 2.8 P-Tuning v2

**Paper:** P-Tuning v2 (Liu et al., 2022)  
**Idee:** Deep Prompt Tuning + Reparameterization

**Eigenschaften:**
- ✅ Bridging gap zwischen Prompt Tuning und Fine-Tuning
- ✅ Funktioniert auch bei kleineren Models
- ✅ ~0.1% Parameter
- ❌ Komplexere Implementation

**Wann nutzen:**
- NLU Tasks (Classification, NER, QA)
- Wenn Prompt Tuning nicht funktioniert (kleines Model)

---

## 3. Verfügbare Training Frameworks (Open-Source)

### Framework-Übersicht

| Framework | License | PEFT Support | Speed | Memory | Use Case |
|-----------|---------|--------------|-------|--------|----------|
| **Axolotl** | Apache 2.0 | LoRA, QLoRA, IA³ | Standard | Standard | Production ⭐ |
| **Unsloth** | Apache 2.0 | LoRA, QLoRA | **2x faster** | **50% less** | Performance |
| **PEFT (HuggingFace)** | Apache 2.0 | **Alle Methoden** | Standard | Standard | Research |
| **LLaMA Factory** | Apache 2.0 | LoRA, QLoRA, Full | Standard | Standard | Multi-Backend |
| **TRL** | Apache 2.0 | LoRA + RLHF | Standard | Standard | RLHF, DPO |

### PEFT-Methoden Support Matrix

| Framework | LoRA | QLoRA | AdaLoRA | IA³ | Prompt/Prefix | P-Tuning |
|-----------|------|-------|---------|-----|---------------|----------|
| **Axolotl** | ✅ | ✅ | ❌ | ✅ | ❌ | ❌ |
| **Unsloth** | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| **PEFT** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **LLaMA Factory** | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |

### 2.1 Axolotl (⭐ Empfehlung für Production)

**Repository:** https://github.com/OpenAccess-AI-Collective/axolotl  
**License:** Apache 2.0  
**Maintainer:** OpenAccess AI Collective (aktive Community)

**Vorteile:**
- ✅ **Production-Ready** - Verwendet von vielen Startups/Companies
- ✅ **YAML Config** - Deklarative Konfiguration, keine Python-Änderungen
- ✅ **Multi-Format Support** - JSONL, Parquet, HuggingFace Datasets
- ✅ **LoRA/QLoRA/FSDP** - Alle wichtigen Parameter-Efficient Methods
- ✅ **Multi-GPU Support** - DeepSpeed, FSDP
- ✅ **Wandb/MLflow Integration** - Experiment Tracking
- ✅ **Streaming Support** - Große Datasets via IterableDataset
- ✅ **Best Practices** - Flash Attention 2, Gradient Checkpointing

**Integration mit ThemisDB:**
```yaml
# axolotl_config.yaml
base_model: mistralai/Mistral-7B-v0.1
model_type: MistralForCausalLM

# ThemisDB Datenquelle
datasets:
  - path: http://themisdb:8765/api/export/jsonl_llm/stream
    type: custom  # Custom Dataset Loader
    streaming: true
    data_files:
      train: legal_qa_2024.jsonl

# LoRA Configuration (aus ThemisDB Metadata)
adapter: lora
lora_r: 8
lora_alpha: 16
lora_dropout: 0.1
lora_target_modules:
  - q_proj
  - v_proj
  - k_proj
  - o_proj

# Training
output_dir: ./adapters/legal-qa-v1
num_epochs: 3
learning_rate: 2e-4
```

**Aufwand für Integration:** 🟢 **Niedrig** (1-2 Tage)
- Custom Dataset Loader für ThemisDB Streaming API
- Config Template Generator basierend auf Adapter Metadata
- Optional: CLI Wrapper `themisdb train --adapter legal-qa-v1`

### 2.2 Unsloth (⭐ Empfehlung für Performance)

**Repository:** https://github.com/unslothai/unsloth  
**License:** Apache 2.0  
**Besonderheit:** Custom CUDA Kernels für 2x Speedup

**Vorteile:**
- ✅ **2x schneller** als Standard PEFT (custom kernels)
- ✅ **50% weniger VRAM** - Optimierte Memory Management
- ✅ **Einfache API** - `FastLanguageModel` wrapper
- ✅ **LoRA/QLoRA Support** - 4-bit, 8-bit quantization
- ✅ **Free Tier auf Google Colab** - Auch für Testzwecke

**Integration mit ThemisDB:**
```python
from unsloth import FastLanguageModel
from datasets import IterableDataset

# ThemisDB Streaming Dataset
def themisdb_generator():
    response = requests.post(
        'http://themisdb:8765/api/export/jsonl_llm/stream',
        json={'theme': 'Rechtssprechung'},
        stream=True
    )
    for line in response.iter_lines():
        yield json.loads(line)

dataset = IterableDataset.from_generator(themisdb_generator)

# Unsloth Training
model, tokenizer = FastLanguageModel.from_pretrained(
    model_name="mistralai/Mistral-7B-v0.1",
    max_seq_length=2048,
    load_in_4bit=True,  # QLoRA
)

model = FastLanguageModel.get_peft_model(
    model,
    r=8,
    lora_alpha=16,
    target_modules=["q_proj", "v_proj", "k_proj", "o_proj"],
)

# Training
trainer = SFTTrainer(
    model=model,
    train_dataset=dataset,
    max_seq_length=2048,
)
trainer.train()
```

**Aufwand für Integration:** 🟢 **Niedrig** (1-2 Tage)
- Wrapper für ThemisDB IterableDataset
- Metadata-basierte Model/LoRA Config
- Performance Benchmarks vs. Axolotl

### 2.3 PEFT (HuggingFace)

**Repository:** https://github.com/huggingface/peft  
**License:** Apache 2.0  
**Besonderheit:** Low-Level Library, maximale Flexibilität

**Vorteile:**
- ✅ **Standard Library** - Von HuggingFace maintained
- ✅ **Maximale Kontrolle** - Low-level API
- ✅ **Viele Adapter Types** - LoRA, AdaLoRA, IA3, Prompt Tuning
- ✅ **Model Hub Integration** - Direktes Pushen zu HuggingFace

**Nachteile:**
- ❌ Mehr Boilerplate Code als Axolotl
- ❌ Keine YAML Config (nur Python)
- ❌ Weniger "Batteries Included" Features

**Use Case:** Wenn spezielle Anpassungen nötig sind, die Axolotl/Unsloth nicht abdecken.

**Aufwand für Integration:** 🟡 **Mittel** (3-5 Tage)
- Vollständiger Training Loop selbst implementieren
- Eigenes Experiment Tracking
- Eigenes Checkpoint Management

### 2.4 LLaMA Factory

**Repository:** https://github.com/hiyouga/LLaMA-Factory  
**License:** Apache 2.0  
**Besonderheit:** Web UI + CLI

**Vorteile:**
- ✅ **Web UI** - Graphische Oberfläche für Training
- ✅ **Multi-Backend** - PEFT, QLoRA, FSDP, DeepSpeed
- ✅ **Data Management** - Built-in Dataset Browser
- ✅ **Model Zoo** - Viele vortrainierte Modelle

**Nachteile:**
- ❌ Zusätzliche Complexity (Web UI)
- ❌ Weniger programmierbar als Axolotl

**Use Case:** Wenn GUI für nicht-technische User wichtig ist.

---

---

## 4. C++ Native Training-Integration (für ThemisDB)

### 4.1 Warum C++ statt Python?

**ThemisDB ist in C++ geschrieben** und benötigt native Integration für:
1. ✅ **Zero-Copy** - Direkter Zugriff auf RocksDB Memory
2. ✅ **Performance** - Keine Python-Overhead
3. ✅ **Deployment** - Single Binary ohne Python-Dependencies
4. ✅ **Integration** - Natives Training direkt aus SQL/AQL

**Problem:** Alle modernen Training-Frameworks sind Python-basiert (PyTorch, HuggingFace)

### 4.2 C++ Integration Optionen

#### Option A: llama.cpp Training Support 🟢 **EMPFOHLEN**

**Status:** llama.cpp v1.3.0 (geplant) hat experimentellen LoRA Training Support

**Repository:** https://github.com/ggerganov/llama.cpp  
**License:** MIT

**Vorteile:**
- ✅ **Pure C++** - Keine Python-Abhängigkeit
- ✅ **Bereits geplant** - llama.cpp Integration für v1.3.0
- ✅ **Same Stack** - Training + Inference in gleichem Framework
- ✅ **Lightweight** - Keine PyTorch/CUDA-Toolkit Dependencies
- ✅ **CPU + GPU** - Metal (Mac), CUDA, ROCm, Vulkan, SYCL
- ✅ **Quantization** - GGUF Format nativ

**llama.cpp LoRA Training Features:**
```cpp
// llama.cpp/examples/finetune/finetune.cpp (experimentell)
struct train_params {
    int n_ctx = 512;
    int n_batch = 8;
    int n_epochs = 1;
    
    // LoRA config
    int lora_r = 8;
    float lora_alpha = 16.0f;
    float lora_dropout = 0.1f;
    
    // Optimizer
    enum optimizer_type optimizer = ADAM;
    float learning_rate = 1e-3f;
};

// Training Loop
llama_train(model, train_params, dataset);
```

**Aktueller Status (Dezember 2024):**
- ✅ Basic LoRA Training implementiert
- ✅ GGUF Adapter Export/Import
- ⚠️ Experimentell, nicht Production-Ready
- ⚠️ Weniger Features als Python Frameworks

**Integration mit ThemisDB:**
```cpp
// ThemisDB native LoRA training
#include "llm/llama_trainer.h"
#include "exporters/jsonl_llm_exporter.h"

namespace themis {
namespace llm {

class LlamaLoRATrainer {
public:
    LlamaLoRATrainer(const std::string& model_path, 
                     const LoRAConfig& config);
    
    // Train from ThemisDB data (zero-copy)
    void train(const std::vector<BaseEntity>& entities);
    
    // Export adapter in GGUF format
    void saveAdapter(const std::string& output_path);
    
    // Direct integration with Llama.cpp inference
    void deployToInference(LlamaCppContext& ctx);
};

} // namespace llm
} // namespace themis
```

**Aufwand:** 🟡 **Mittel** (2-3 Wochen)
- llama.cpp Training API integrieren
- ThemisDB → llama.cpp Datenformat Konverter
- Training Loop mit RocksDB Integration
- Tests + Benchmarks

#### Option B: LibTorch (PyTorch C++ API) 🟡

**Repository:** https://pytorch.org/cppdocs/  
**License:** BSD-3-Clause

**Vorteile:**
- ✅ Vollständige PyTorch Features in C++
- ✅ Production-Ready
- ✅ PEFT-Methoden selbst implementierbar
- ❌ **Huge Dependency** - 2GB+ LibTorch + CUDA Toolkit
- ❌ Keine fertigen PEFT Implementations
- ❌ Manuelles Implementieren von LoRA/QLoRA nötig

**LibTorch LoRA Implementation:**
```cpp
#include <torch/torch.h>

// LoRA Layer Implementation
struct LoRALinear : torch::nn::Module {
    LoRALinear(int in_features, int out_features, int rank, float alpha)
        : lora_A(register_module("lora_A", 
                 torch::nn::Linear(in_features, rank))),
          lora_B(register_module("lora_B", 
                 torch::nn::Linear(rank, out_features))),
          scaling(alpha / rank) {
        // Initialize
        torch::nn::init::kaiming_uniform_(lora_A->weight);
        torch::nn::init::zeros_(lora_B->weight);
    }
    
    torch::Tensor forward(torch::Tensor x, torch::Tensor base_output) {
        auto lora_output = lora_B(lora_A(x));
        return base_output + lora_output * scaling;
    }
    
    torch::nn::Linear lora_A, lora_B;
    float scaling;
};

// Training Loop
auto optimizer = torch::optim::Adam(model->parameters(), /*lr=*/1e-3);
for (auto& batch : data_loader) {
    optimizer.zero_grad();
    auto output = model->forward(batch.data);
    auto loss = torch::nn::functional::cross_entropy(output, batch.targets);
    loss.backward();
    optimizer.step();
}
```

**Aufwand:** 🔴 **Hoch** (1-2 Monate)
- LoRA/QLoRA von Grund auf implementieren
- Optimizer (Adam, AdamW) konfigurieren
- Gradient Checkpointing
- Multi-GPU Support (DDP)
- Integration mit ThemisDB

**Empfehlung:** ❌ **NICHT Empfohlen** - Zu viel Aufwand, große Dependencies

#### Option C: ONNX Runtime Training 🟡

**Repository:** https://onnxruntime.ai/  
**License:** MIT

**Vorteile:**
- ✅ C++ API
- ✅ Cross-Platform (CPU, CUDA, DirectML, TensorRT)
- ❌ Training API weniger ausgereift als Inferenz
- ❌ Keine fertigen PEFT Implementations

**Aufwand:** 🔴 **Hoch** (1-2 Monate)

**Empfehlung:** ❌ **NICHT Empfohlen**

#### Option D: Hybrid Python→C++ Export 🟢 **Praktische Alternative**

**Idee:** Training in Python, dann Export zu C++ für Inferenz

**Workflow:**
```
1. Training (Python):
   ThemisDB → Python Streaming → Axolotl/Unsloth → LoRA Adapter (safetensors)
   
2. Konvertierung:
   LoRA Adapter (.safetensors) → GGUF Format (llama.cpp-kompatibel)
   
3. Inferenz (C++ ThemisDB):
   llama.cpp lädt GGUF + LoRA → Native C++ Inference
```

**Konvertierung Tools:**
```bash
# Python LoRA → GGUF Konvertierung
python llama.cpp/convert-lora-to-gguf.py \
  --input ./adapters/legal-qa-v1/adapter_model.safetensors \
  --output ./adapters/legal-qa-v1.gguf \
  --base mistralai/Mistral-7B-v0.1
```

**ThemisDB Integration:**
```cpp
// C++ Inference mit LoRA (llama.cpp)
#include "llm/llama_cpp_inference.h"

auto model = llama_load_model("models/mistral-7b.gguf");
auto lora = llama_load_lora("adapters/legal-qa-v1.gguf");

llama_apply_lora(model, lora);

auto response = llama_generate(model, "Was ist Immissionsschutz?");
```

**Vorteile:**
- ✅ **Best of Both Worlds** - Python Training (ausgereift) + C++ Inference (performant)
- ✅ **Zero Python Runtime Dependency** - Nur für Training-Phase
- ✅ **Production-Ready** - llama.cpp ist battle-tested
- ✅ **Geringer Aufwand** - Nur Konvertierung implementieren

**Aufwand:** 🟢 **Niedrig** (1 Woche)
- Python Training Setup (bereits dokumentiert)
- Konvertierungs-Script/Tool
- C++ Inference Integration

#### Option E: Custom C++ Training Engine 🔴

**Von Grund auf eigenes Training Framework implementieren**

**Aufwand:** 🔴 **Sehr Hoch** (3-6 Monate, 3-5 Entwickler)

**Komponenten:**
- Autograd Engine (Backpropagation)
- Tensor Operations (Matrix Multiplication, etc.)
- CUDA Kernels (für GPU)
- Optimizers (Adam, AdamW, SGD)
- LoRA Layer Implementation
- Gradient Checkpointing
- Mixed Precision Training (FP16/BF16)
- Distributed Training (Multi-GPU)

**Empfehlung:** ❌❌❌ **ABSOLUT NICHT Empfohlen**
- Reinventing the wheel
- 1000x mehr Aufwand als Nutzen
- Maintenance-Alptraum

### 4.3 Empfohlene C++ Strategie für ThemisDB

#### 🎯 **2-Phasen Ansatz:**

**Phase 1 (v1.2.0-v1.3.0): Hybrid Python/C++** ⭐ **Sofort umsetzbar**

```
┌──────────────────────────────────────────────────┐
│ Training Phase (Python - optional)               │
│ ┌────────────┐      ┌──────────┐      ┌──────┐ │
│ │ ThemisDB   │─HTTP→│ Axolotl/ │─save→│ LoRA │ │
│ │ (C++)      │      │ Unsloth  │      │ .st  │ │
│ └────────────┘      └──────────┘      └───┬──┘ │
└────────────────────────────────────────────┼────┘
                                             │
                    ┌────────────────────────▼────┐
                    │ Konvertierung (Tool)        │
                    │ safetensors → GGUF          │
                    └────────────┬────────────────┘
                                 │
┌────────────────────────────────▼─────────────────┐
│ Inference Phase (C++ - native)                   │
│ ┌────────────┐      ┌──────────┐      ┌──────┐  │
│ │ ThemisDB   │──────│ llama.cpp│◄─────│ LoRA │  │
│ │ (C++)      │      │ (C++)    │      │ .gguf│  │
│ └────────────┘      └──────────┘      └──────┘  │
└──────────────────────────────────────────────────┘
```

**Vorteile:**
- ✅ Sofort produktiv (Python Ecosystem)
- ✅ Kein C++ Training Code nötig
- ✅ Production C++ Inference (llama.cpp)
- ✅ Geringer Aufwand (1-2 Wochen)

**Phase 2 (v1.4.0+): Native C++ Training** - Optional

Falls llama.cpp Training API ausgereift ist:

```cpp
// Vollständig in C++
#include "llm/llama_trainer.h"

auto trainer = themis::llm::LlamaLoRATrainer(
    "models/mistral-7b.gguf",
    config
);

// Zero-copy Training aus ThemisDB
trainer.trainFromThemisDB(query);

// Direktes Deployment
trainer.saveAdapter("adapters/legal-v2.gguf");
```

### 4.4 Implementierungs-Empfehlung

#### ✅ **Phase 1 Implementation (1-2 Wochen)**

**Komponenten:**

1. **Python Training Connector** (besteht bereits aus Dokumentation)
   - ThemisDB HTTP Streaming API nutzen
   - Axolotl/Unsloth Integration
   - Adapter Metadata Tracking

2. **LoRA→GGUF Konverter** (NEU - 2-3 Tage)
   ```cpp
   // src/llm/lora_converter.cpp
   class LoRAConverter {
   public:
       // Convert safetensors → GGUF
       static bool convertToGGUF(
           const std::string& input_safetensors,
           const std::string& output_gguf,
           const std::string& base_model_name
       );
       
       // Validate GGUF LoRA
       static bool validateGGUF(const std::string& gguf_path);
   };
   ```

3. **ThemisDB LoRA Manager** (NEU - 3-4 Tage)
   ```cpp
   // src/llm/lora_manager.cpp
   class LoRAManager {
   public:
       // Register trained adapter
       void registerAdapter(const AdapterMetadata& metadata,
                           const std::string& gguf_path);
       
       // List available adapters
       std::vector<AdapterInfo> listAdapters() const;
       
       // Deploy to llama.cpp inference engine
       void deployAdapter(const std::string& adapter_id,
                         LlamaCppContext& inference_ctx);
   };
   ```

4. **AQL Integration** (NEU - 2-3 Tage)
   ```sql
   -- Deploy trained LoRA adapter
   EXECUTE llm_deploy_adapter 'legal-qa-v1';
   
   -- Query with specific adapter
   SELECT llm_generate(
       'Was ist Immissionsschutz?',
       adapter: 'legal-qa-v1'
   );
   
   -- List adapters
   SELECT * FROM llm_adapters;
   ```

### 4.4 Implementierungs-Empfehlung

#### ✅ **Inline Native Training Integration (C++)** ⭐ **Empfohlen für ThemisDB**

**Ziel:** Vollständig integriertes Training direkt aus ThemisDB's Multi-Model Storage ohne Export oder externe Tools.

**Architektur:**

```
┌─────────────────────────────────────────────────────────────┐
│              ThemisDB Inline Training Engine                │
└─────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│ Layer 4: AQL Training Interface (SQL-like)                   │
│                                                              │
│  TRAIN ADAPTER legal_qa_v1                                   │
│    FROM documents                                            │
│    WHERE category = 'Rechtssprechung'                        │
│    WITH base_model = 'mistral-7b',                          │
│         lora_rank = 8,                                       │
│         epochs = 3;                                          │
│                                                              │
│  -- Multi-Model Query Training                              │
│  TRAIN ADAPTER medical_v1                                    │
│    FROM (                                                    │
│      SELECT d.text, r.diagnosis, g.context                  │
│      FROM documents d                                        │
│      JOIN relations r ON d.id = r.doc_id                   │
│      JOIN GRAPH_TRAVERSE(g, 'medical_context') g            │
│      WHERE VECTOR_SIMILARITY(d.embedding, @query) > 0.8     │
│    );                                                        │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│ Layer 3: Training Orchestrator (C++)                         │
│  - Query Optimizer für Training Data                         │
│  - Batch Generator (streaming from storage)                  │
│  - Memory-Mapped Training Data                               │
│  - Zero-Copy Data Pipeline                                   │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│ Layer 2: Training Backend (Choose One)                       │
│                                                              │
│  Option A: llama.cpp (C++)          ┌─────────────────┐     │
│  ├─ Native C++, MIT License         │ ✅ Recommended   │     │
│  ├─ GGUF Format                     │ ✅ Lightweight   │     │
│  └─ CPU/GPU Support                 └─────────────────┘     │
│                                                              │
│  Option B: LibTorch (C++)           ┌─────────────────┐     │
│  ├─ Full PyTorch Features           │ ⚠️ Large Deps   │     │
│  ├─ Custom LoRA Implementation      │ ⚠️ Complex      │     │
│  └─ Production-Ready                └─────────────────┘     │
│                                                              │
│  Option C: Custom C++ Engine        ┌─────────────────┐     │
│  ├─ Full Control                    │ ❌ High Effort   │     │
│  └─ No External Dependencies        │ ❌ Maintenance   │     │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│ Layer 1: Multi-Model Storage (ThemisDB)                      │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  Relational  │  │    Graph     │  │   Vector     │      │
│  │   (RocksDB)  │  │  (RocksDB)   │  │   (FAISS)    │      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
│         │                  │                  │              │
│         └──────────────────┼──────────────────┘              │
│                            ▼                                 │
│              Zero-Copy Memory Access                         │
│              Direct RocksDB Iterator                         │
│              SIMD-Optimized Batching                         │
└──────────────────────────────────────────────────────────────┘
```

#### 🎯 **Phase 1: Inline Training mit llama.cpp Backend**

**Komponenten zu implementieren:**

**1. Training Query Optimizer** (NEU - 1 Woche)
```cpp
// include/llm/training_query_optimizer.h
namespace themis::llm {

class TrainingQueryOptimizer {
public:
    // Parse AQL training query
    TrainingPlan parseTrainingQuery(const std::string& aql_query);
    
    // Optimize data access pattern
    struct TrainingPlan {
        std::string adapter_id;
        query::QueryPlan data_query;      // Query for training data
        LoRAConfig lora_config;           // LoRA hyperparameters
        TrainingConfig training_config;   // Epochs, LR, etc.
        
        // Multi-model data sources
        bool uses_relational = false;
        bool uses_graph = false;
        bool uses_vector = false;
    };
    
    // Estimate memory requirements
    size_t estimateMemoryUsage(const TrainingPlan& plan);
};

} // namespace themis::llm
```

**2. Zero-Copy Batch Generator** (NEU - 1 Woche)
```cpp
// include/llm/batch_generator.h
namespace themis::llm {

class BatchGenerator {
public:
    BatchGenerator(const TrainingPlan& plan, 
                   storage::RocksDBBackend& storage);
    
    // Iterator-based batch generation (zero-copy)
    class Iterator {
    public:
        struct Batch {
            const char* input_text;      // Direct pointer to RocksDB memory
            const char* target_text;
            size_t batch_size;
            float* weights;               // Optional sample weights
            
            // Multi-model context
            graph::GraphContext* graph_ctx = nullptr;
            vector::VectorContext* vector_ctx = nullptr;
        };
        
        bool hasNext() const;
        Batch next();
        
    private:
        storage::RocksDBIterator rocks_iterator_;
        std::vector<char*> memory_mapped_regions_;  // Zero-copy
    };
    
    Iterator begin();
    Iterator end();
    
private:
    storage::RocksDBBackend& storage_;
    TrainingPlan plan_;
};

} // namespace themis::llm
```

**3. Inline Training Engine** (NEU - 2 Wochen)
```cpp
// include/llm/inline_training_engine.h
namespace themis::llm {

class InlineTrainingEngine {
public:
    InlineTrainingEngine(storage::RocksDBBackend& storage);
    
    // Execute training from AQL query
    AdapterInfo trainFromAQL(const std::string& aql_query);
    
    // Training backends
    enum class Backend {
        LLAMA_CPP,    // llama.cpp (recommended)
        LIBTORCH,     // LibTorch C++ API
        CUSTOM        // Custom implementation
    };
    
    void setBackend(Backend backend);
    
    // Training execution
    struct TrainingResult {
        std::string adapter_id;
        std::string adapter_path;       // GGUF file
        
        // Metrics
        float final_loss;
        float training_time_seconds;
        size_t samples_processed;
        
        // Storage stats
        size_t bytes_read_from_rocksdb;
        size_t batches_generated;
    };
    
    TrainingResult train(const TrainingPlan& plan);
    
    // Multi-model data integration
    void enableGraphContext(bool enable);
    void enableVectorContext(bool enable);
    
private:
    storage::RocksDBBackend& storage_;
    Backend backend_ = Backend::LLAMA_CPP;
    
    // Backend adapters
    std::unique_ptr<LlamaCppTrainer> llama_trainer_;
    std::unique_ptr<LibTorchTrainer> torch_trainer_;
};

} // namespace themis::llm
```

**4. llama.cpp Training Adapter** (NEU - 1 Woche)
```cpp
// src/llm/llamacpp_trainer.cpp
namespace themis::llm {

class LlamaCppTrainer {
public:
    LlamaCppTrainer(const std::string& base_model_path);
    
    // Initialize LoRA layers
    void initializeLoRA(const LoRAConfig& config);
    
    // Training from ThemisDB batches (zero-copy)
    TrainingResult trainFromBatches(BatchGenerator::Iterator begin,
                                    BatchGenerator::Iterator end,
                                    const TrainingConfig& config);
    
    // Export trained adapter
    void saveAdapter(const std::string& output_path);
    
private:
    // llama.cpp context
    llama_model* model_ = nullptr;
    llama_context* ctx_ = nullptr;
    
    // LoRA weights (A and B matrices)
    std::vector<LoRALayer> lora_layers_;
    
    struct LoRALayer {
        std::string layer_name;
        std::vector<float> A;  // rank x in_features
        std::vector<float> B;  // out_features x rank
        float scaling;
    };
    
    // Training loop
    void trainingStep(const BatchGenerator::Batch& batch);
    void backward(float loss);
    void optimizerStep();  // Adam optimizer
    
    // Adam optimizer state
    struct AdamState {
        std::vector<float> m;  // First moment
        std::vector<float> v;  // Second moment
        float beta1 = 0.9f;
        float beta2 = 0.999f;
        float epsilon = 1e-8f;
    } adam_state_;
};

} // namespace themis::llm
```

**5. AQL Training Syntax** (NEU - 3-4 Tage)
```cpp
// src/query/aql_training_parser.cpp

// Parse AQL TRAIN statement
class AQLTrainingParser : public AQLParser {
public:
    /*
    Grammar:
    
    TRAIN ADAPTER adapter_name
      FROM table | query
      [WHERE condition]
      [WITH options]
      [USING multi_model_features];
    
    Options:
      base_model = 'model_name'
      lora_rank = integer
      lora_alpha = float
      lora_dropout = float
      epochs = integer
      learning_rate = float
      batch_size = integer
      
    Multi-model features:
      GRAPH_CONTEXT(node_types, relationship_types)
      VECTOR_SIMILARITY(embedding_field, threshold)
      RELATIONAL_JOIN(tables...)
    */
    
    TrainingPlan parseTrainStatement(const std::string& aql);
};
```

**6. Multi-Model Training Data Integration** (NEU - 1 Woche)
```cpp
// include/llm/multimodel_training_data.h
namespace themis::llm {

class MultiModelTrainingData {
public:
    // Combine data from multiple models
    struct TrainingSample {
        // Primary text data
        std::string instruction;
        std::string input_context;
        std::string output;
        
        // Graph enrichment
        struct GraphContext {
            std::vector<std::string> connected_entities;
            std::vector<std::string> relationship_types;
            std::map<std::string, std::string> node_properties;
        } graph_context;
        
        // Vector enrichment  
        struct VectorContext {
            std::vector<float> embedding;
            std::vector<std::pair<std::string, float>> similar_docs;  // id, score
        } vector_context;
        
        // Relational metadata
        std::map<std::string, std::string> metadata;
        
        // Sample weight (for importance sampling)
        float weight = 1.0f;
    };
    
    // Generate enriched training samples
    std::vector<TrainingSample> generateSamples(
        const query::QueryResult& base_query,
        bool include_graph = false,
        bool include_vector = false
    );
    
private:
    storage::RocksDBBackend& storage_;
    graph::GraphEngine& graph_;
    vector::VectorIndex& vector_index_;
};

} // namespace themis::llm
```

#### 📝 **Beispiele: AQL Training Queries**

**Beispiel 1: Einfaches Training**
```sql
-- Basic LoRA training from relational data
TRAIN ADAPTER legal_qa_v1
  FROM documents
  WHERE category = 'Rechtssprechung' 
    AND created_at > '2020-01-01'
  WITH 
    base_model = 'mistral-7b',
    lora_rank = 8,
    lora_alpha = 16,
    epochs = 3,
    learning_rate = 0.0002;
```

**Beispiel 2: Multi-Model Training (Graph + Vector + Relational)**
```sql
-- Advanced training with graph context
TRAIN ADAPTER medical_diagnosis_v1
  FROM (
    -- Base documents (relational)
    SELECT 
      d.patient_description AS instruction,
      d.doctor_notes AS input,
      d.diagnosis AS output,
      d.embedding
    FROM medical_documents d
    WHERE d.verified = true
  )
  USING GRAPH_CONTEXT(
    -- Add graph relationships
    node_types: ['Patient', 'Symptom', 'Disease', 'Treatment'],
    relationships: ['HAS_SYMPTOM', 'DIAGNOSED_WITH', 'TREATED_BY']
  )
  USING VECTOR_SIMILARITY(
    -- Add similar cases as context
    field: embedding,
    threshold: 0.85,
    top_k: 5
  )
  WITH
    base_model = 'llama-3-8b',
    lora_rank = 16,
    epochs = 5,
    batch_size = 8;
```

**Beispiel 3: RAG-Enhanced Training**
```sql
-- Training with vector similarity for context
TRAIN ADAPTER environmental_law_v1
  FROM documents d
  WHERE d.theme = 'Immissionsschutz'
  USING VECTOR_SIMILARITY(
    field: d.embedding,
    query_embedding: EMBED('Lärmschutz Grenzwerte'),
    threshold: 0.75
  )
  WITH
    base_model = 'mistral-7b',
    lora_rank = 8,
    epochs = 3,
    -- Auto-weight by document freshness and similarity
    sample_weights = AUTO_WEIGHT(freshness: 0.5, similarity: 0.5);
```

**Beispiel 4: Cross-Domain Training**
```sql
-- Train adapter on multiple related domains
TRAIN ADAPTER multi_domain_v1
  FROM (
    SELECT text, category FROM documents 
    WHERE category IN ('Legal', 'Medical', 'Technical')
  )
  USING GRAPH_CONTEXT(
    -- Link related concepts across domains
    relationships: ['RELATED_TO', 'REFERENCES', 'SIMILAR_CONCEPT']
  )
  WITH
    base_model = 'mistral-7b',
    lora_rank = 32,  -- Higher rank for multi-domain
    epochs = 5,
    -- Domain-specific target modules
    target_modules = ['q_proj', 'v_proj', 'k_proj', 'o_proj', 
                     'gate_proj', 'up_proj', 'down_proj'];
```

**Beispiel 5: Incremental Training**
```sql
-- Continue training existing adapter
TRAIN ADAPTER legal_qa_v2
  FROM documents
  WHERE created_at > '2024-01-01'  -- New data only
  WITH
    base_model = 'mistral-7b',
    parent_adapter = 'legal_qa_v1',  -- Start from existing adapter
    lora_rank = 8,
    epochs = 1,  -- Just 1 epoch for incremental
    learning_rate = 0.0001;  -- Lower LR for fine-tuning
```

#### 🔧 **Implementation Roadmap**

**Woche 1-2: Foundation**
- [ ] AQL TRAIN syntax parser
- [ ] TrainingQueryOptimizer implementation
- [ ] Basic BatchGenerator (relational only)

**Woche 3-4: Training Backend**
- [ ] llama.cpp integration wrapper
- [ ] LoRA layer initialization
- [ ] Basic training loop (Adam optimizer)
- [ ] GGUF adapter export

**Woche 5-6: Multi-Model Integration**
- [ ] Graph context enrichment
- [ ] Vector similarity context
- [ ] Multi-model BatchGenerator
- [ ] Sample weighting strategies

**Woche 7-8: Optimization & Testing**
- [ ] Zero-copy memory optimization
- [ ] SIMD batch processing
- [ ] GPU acceleration (CUDA/Metal)
- [ ] Integration tests
- [ ] Performance benchmarks

**Gesamt-Aufwand:** 🟡 **6-8 Wochen (1-2 Entwickler)**

#### ⚡ **Performance-Optimierungen**

**Zero-Copy Data Access:**
```cpp
// Direct memory mapping from RocksDB
auto batch = batch_generator.next();
const char* text = batch.input_text;  // Points directly to RocksDB memory
// No memcpy, no allocation
```

**SIMD Batching:**
```cpp
// Vectorized batch processing
#include <immintrin.h>

void processBatchSIMD(const float* embeddings, size_t batch_size) {
    for (size_t i = 0; i < batch_size; i += 8) {
        __m256 vec = _mm256_load_ps(&embeddings[i]);
        // SIMD operations
        _mm256_store_ps(&result[i], vec);
    }
}
```

**Async I/O:**
```cpp
// Prefetch next batch while training current batch
std::future<Batch> next_batch = std::async([&]() {
    return batch_generator.next();
});

trainer.trainOnBatch(current_batch);
current_batch = next_batch.get();  // Overlap I/O with compute
```

#### 📊 **Expected Performance**

**Training Speed:**
```
Dataset: 100k samples, Mistral-7B, LoRA rank=8
Hardware: A100 40GB

Traditional Approach (Export → Python):
- Export JSONL: 5 minutes
- Load to memory: 2 minutes  
- Training: 45 minutes
Total: 52 minutes

Inline Training (ThemisDB):
- No export: 0 minutes ✓
- Zero-copy loading: 0 minutes ✓
- Training: 40 minutes (5min faster, SIMD batching)
Total: 40 minutes (23% faster)

Memory:
- Traditional: 24GB (model + data + gradients)
- Inline: 18GB (zero-copy, no data duplication)
→ 25% less memory
```

---

## 6. Sharding-Integration: Distributed LoRA Training & Deployment

### 6.1 ThemisDB Sharding Architektur - Überblick

ThemisDB nutzt **horizontales Sharding** mit Consistent Hashing für Datenverteilung über mehrere Shards. Die bestehende Sharding-Infrastruktur kann für **verteiltes LoRA-Training** und **horizontale Adapter-Bereitstellung** genutzt werden.

**Bestehende Sharding-Komponenten:**
```cpp
// Existierende ThemisDB Sharding-Architektur
namespace themis::sharding {
    class ShardRouter          // Query Routing (SCATTER_GATHER, SINGLE_SHARD)
    class URNResolver          // Shard Location Resolution
    class RemoteExecutor       // Cross-Shard RPC
    class ShardTopology        // Cluster Membership
    class DataMigrator         // Shard Rebalancing
    class WALShipper          // Replica Sync
    class RaftState           // Consensus & Leader Election
}
```

### 6.2 Distributed LoRA Training über Shards

#### Strategie A: Data-Parallel Training (Empfohlen) 🟢

**Idee:** Jeder Shard trainiert auf seinen lokalen Daten, Gradienten werden aggregiert.

```
┌──────────────────────────────────────────────────────────────┐
│                    Coordinator Shard (Leader)                │
│  - Model Synchronization                                     │
│  - Gradient Aggregation (AllReduce)                          │
│  - Checkpoint Management                                     │
└────────────┬─────────────────────────────────────────────────┘
             │
             ├─────────────────┬──────────────────┬─────────────┐
             ▼                 ▼                  ▼             ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│  Shard 1         │ │  Shard 2         │ │  Shard N         │
│  ┌────────────┐  │ │  ┌────────────┐  │ │  ┌────────────┐  │
│  │ Local Data │  │ │  │ Local Data │  │ │  │ Local Data │  │
│  │ (Legal)    │  │ │  │ (Medical)  │  │ │  │ (Technical)│  │
│  └─────┬──────┘  │ │  └─────┬──────┘  │ │  └─────┬──────┘  │
│        ▼         │ │        ▼         │ │        ▼         │
│  ┌────────────┐  │ │  ┌────────────┐  │ │  ┌────────────┐  │
│  │  Training  │  │ │  │  Training  │  │ │  │  Training  │  │
│  │   Loop     │  │ │  │   Loop     │  │ │  │   Loop     │  │
│  └─────┬──────┘  │ │  └─────┬──────┘  │ │  └─────┬──────┘  │
│        ▼         │ │        ▼         │ │        ▼         │
│  [Gradients]────┼─┼────────┼─────────┼─┼────────┼─────────┼──> AllReduce
└──────────────────┘ └──────────────────┘ └──────────────────┘
```

**Implementation:**
```cpp
// include/llm/distributed_training_coordinator.h
namespace themis::llm {

class DistributedTrainingCoordinator {
public:
    DistributedTrainingCoordinator(
        sharding::ShardRouter& router,
        sharding::ShardTopology& topology,
        const TrainingConfig& config
    );
    
    // Start distributed training across shards
    TrainingResult trainDistributed(const TrainingPlan& plan);
    
    // Gradient synchronization strategies
    enum class SyncStrategy {
        ALL_REDUCE,          // Ring-AllReduce (Ring-based)
        PARAMETER_SERVER,    // Central gradient aggregation
        FEDERATED           // Privacy-preserving (no raw data sharing)
    };
    
private:
    // Coordination methods
    void broadcastModel(const LoRAWeights& weights);
    LoRAWeights aggregateGradients(
        const std::vector<LoRAWeights>& shard_gradients
    );
    void syncCheckpoint(int epoch);
    
    // Fault tolerance
    void handleShardFailure(const std::string& shard_id);
    
    sharding::ShardRouter& router_;
    sharding::ShardTopology& topology_;
    SyncStrategy sync_strategy_ = SyncStrategy::ALL_REDUCE;
};

} // namespace themis::llm
```

**AQL Syntax für Distributed Training:**
```sql
-- Distributed training across all shards
TRAIN ADAPTER legal_qa_v1 DISTRIBUTED
  FROM documents
  WHERE category = 'Rechtssprechung'
  WITH
    base_model = 'mistral-7b',
    lora_rank = 8,
    epochs = 3,
    -- Distributed training params
    sync_strategy = 'ALL_REDUCE',
    sync_frequency = 100,  -- Sync every 100 batches
    coordinator_shard = 'shard_0';  -- Leader shard
```

**Gradient Synchronization:**
```cpp
// Per Shard: Local Training Step
void shardTrainingStep(const Batch& batch) {
    // 1. Forward pass (local data)
    auto output = model.forward(batch.input);
    auto loss = compute_loss(output, batch.target);
    
    // 2. Backward pass (compute gradients)
    auto gradients = model.backward(loss);
    
    // 3. Send gradients to coordinator (non-blocking)
    if (step % sync_frequency == 0) {
        coordinator.asyncSendGradients(shard_id, gradients);
    }
}

// Coordinator: Gradient Aggregation
LoRAWeights aggregateGradients(
    const std::map<std::string, LoRAWeights>& shard_gradients
) {
    LoRAWeights aggregated;
    
    // AllReduce: Average gradients from all shards
    for (const auto& [shard_id, grads] : shard_gradients) {
        for (size_t i = 0; i < grads.size(); ++i) {
            aggregated[i] += grads[i] / shard_gradients.size();
        }
    }
    
    // Broadcast aggregated gradients back to shards
    for (const auto& [shard_id, _] : shard_gradients) {
        executor_.send(shard_id, "update_gradients", aggregated);
    }
    
    return aggregated;
}
```

**Vorteile:**
- ✅ **Skalierbar**: Linear scaling mit Anzahl Shards
- ✅ **Fault-Tolerant**: Ein Shard-Ausfall stoppt nicht das gesamte Training
- ✅ **Data Locality**: Keine Daten müssen zwischen Shards bewegt werden
- ✅ **Privacy**: Shards tauschen nur Gradienten aus, nicht Rohdaten

**Nachteile:**
- ⚠️ **Network Overhead**: Gradient-Synchronization benötigt Bandbreite
- ⚠️ **Consistency**: Gradients müssen synchron aggregiert werden

#### Strategie B: Federated Learning (Privacy-Focused)

**Für hochsensitive Daten** (z.B. Medizin, Legal):

```cpp
// Federated Averaging (FedAvg) Implementation
class FederatedTrainingCoordinator {
public:
    TrainingResult trainFederated(const TrainingPlan& plan) {
        LoRAWeights global_model = initializeModel();
        
        for (int round = 0; round < num_rounds; ++round) {
            // 1. Broadcast global model to all shards
            broadcastModel(global_model);
            
            // 2. Each shard trains locally (multiple epochs)
            std::vector<LoRAWeights> shard_models;
            for (const auto& shard_id : topology_.getShards()) {
                auto result = executor_.execute(shard_id, {
                    {"command", "train_local"},
                    {"epochs", local_epochs},
                    {"data_filter", plan.data_query}
                });
                shard_models.push_back(result.model_weights);
            }
            
            // 3. Aggregate shard models (weighted averaging)
            global_model = aggregateModels(shard_models);
            
            // 4. Checkpoint
            saveCheckpoint(global_model, round);
        }
        
        return {global_model, metrics};
    }
    
private:
    LoRAWeights aggregateModels(
        const std::vector<LoRAWeights>& shard_models
    ) {
        // FedAvg: Weighted average by number of samples
        LoRAWeights aggregated;
        size_t total_samples = 0;
        
        for (const auto& model : shard_models) {
            total_samples += model.num_samples;
        }
        
        for (const auto& model : shard_models) {
            float weight = (float)model.num_samples / total_samples;
            for (size_t i = 0; i < model.weights.size(); ++i) {
                aggregated.weights[i] += model.weights[i] * weight;
            }
        }
        
        return aggregated;
    }
};
```

### 6.3 Horizontale LoRA Adapter Bereitstellung

**Ziel:** LoRA-Adapter über mehrere Shards verteilen für Load Balancing und Verfügbarkeit.

#### Deployment-Strategien

**Strategie 1: Adapter Co-Location mit Daten** 🟢 **Empfohlen**

```
┌─────────────────────────────────────────────────────────┐
│                  Shard Distribution                      │
└─────────────────────────────────────────────────────────┘

Shard 1 (Legal Domain)          Shard 2 (Medical Domain)
┌────────────────────┐          ┌────────────────────┐
│ Data:              │          │ Data:              │
│ - Legal Docs       │          │ - Medical Docs     │
│ - Case Law         │          │ - Patient Records  │
│                    │          │                    │
│ LoRA Adapters:     │          │ LoRA Adapters:     │
│ ├─ legal-qa-v1.gguf│          │ ├─ medical-v1.gguf │
│ ├─ legal-qa-v2.gguf│          │ ├─ diagnosis-v1    │
│ └─ contract-v1     │          │ └─ treatment-v1    │
│                    │          │                    │
│ Base Model:        │          │ Base Model:        │
│ └─ mistral-7b.gguf │          │ └─ mistral-7b.gguf │
└────────────────────┘          └────────────────────┘

Shard 3 (Technical Domain)      Shard 4 (General)
┌────────────────────┐          ┌────────────────────┐
│ Data:              │          │ Data:              │
│ - Tech Docs        │          │ - General Docs     │
│ - Code             │          │ - News, etc.       │
│                    │          │                    │
│ LoRA Adapters:     │          │ LoRA Adapters:     │
│ ├─ code-gen-v1     │          │ ├─ general-v1      │
│ └─ tech-qa-v1      │          │ └─ summary-v1      │
│                    │          │                    │
│ Base Model:        │          │ Base Model:        │
│ └─ mistral-7b.gguf │          │ └─ mistral-7b.gguf │
└────────────────────┘          └────────────────────┘
```

**Vorteile:**
- ✅ **Data Locality**: Adapter läuft auf Shard mit relevanten Daten
- ✅ **Zero Data Movement**: Keine Cross-Shard Datenübertragung
- ✅ **Domain Specialization**: Jeder Shard spezialisiert auf seine Domäne

**Implementation:**
```cpp
// include/llm/adapter_deployment_manager.h
namespace themis::llm {

class AdapterDeploymentManager {
public:
    // Deploy adapter to shard(s)
    void deployAdapter(
        const std::string& adapter_id,
        const std::string& adapter_path,
        DeploymentStrategy strategy
    );
    
    enum class DeploymentStrategy {
        CO_LOCATED,      // Deploy to shard with matching data
        REPLICATED,      // Deploy to all shards (redundancy)
        BALANCED         // Load-balanced distribution
    };
    
    // Adapter routing
    std::string routeToAdapter(
        const std::string& query,
        const std::string& adapter_id
    );
    
private:
    // Determine best shard for adapter
    std::string selectShardForAdapter(
        const std::string& adapter_id,
        const AdapterMetadata& metadata
    );
    
    sharding::ShardRouter& router_;
    sharding::ShardTopology& topology_;
};

} // namespace themis::llm
```

**AQL Deployment:**
```sql
-- Deploy adapter to specific shard
DEPLOY ADAPTER legal_qa_v1
  TO SHARD 'shard_legal'
  WITH strategy = 'CO_LOCATED';

-- Deploy to all shards (redundancy)
DEPLOY ADAPTER general_v1
  TO ALL SHARDS
  WITH strategy = 'REPLICATED';

-- Query with adapter routing
SELECT llm_generate(
    'Was ist Immissionsschutz?',
    adapter: 'legal_qa_v1'
)
-- Automatically routed to shard_legal
```

**Strategie 2: Adapter Replication für High Availability**

```cpp
// Replicate adapter to multiple shards for redundancy
void deployAdapterReplicated(
    const std::string& adapter_id,
    const std::string& adapter_path,
    int replication_factor = 3
) {
    // Get all healthy shards
    auto shards = topology_.getHealthyShards();
    
    // Select replication_factor shards
    std::vector<std::string> target_shards;
    for (size_t i = 0; i < std::min((size_t)replication_factor, shards.size()); ++i) {
        target_shards.push_back(shards[i]);
    }
    
    // Deploy to each shard in parallel
    std::vector<std::future<void>> deployments;
    for (const auto& shard_id : target_shards) {
        deployments.push_back(std::async([&]() {
            executor_.execute(shard_id, {
                {"command", "load_adapter"},
                {"adapter_id", adapter_id},
                {"adapter_path", adapter_path}
            });
        }));
    }
    
    // Wait for all deployments
    for (auto& fut : deployments) {
        fut.get();
    }
    
    // Register in adapter registry
    adapter_registry_.registerDeployment(
        adapter_id,
        target_shards,
        replication_factor
    );
}

// Query routing with failover
std::string queryWithAdapter(
    const std::string& query,
    const std::string& adapter_id
) {
    // Get shards hosting this adapter
    auto shards = adapter_registry_.getShardsForAdapter(adapter_id);
    
    // Try shards in order (load balancing + failover)
    for (const auto& shard_id : shards) {
        try {
            auto result = executor_.execute(shard_id, {
                {"command", "llm_generate"},
                {"query", query},
                {"adapter_id", adapter_id}
            });
            return result.text;
        } catch (const ShardUnreachableException& e) {
            // Failover to next shard
            continue;
        }
    }
    
    throw std::runtime_error("All adapter replicas unavailable");
}
```

### 6.4 Load Balancing & Query Routing

**Intelligent Routing basierend auf:**
1. **Data Affinity**: Query zu Shard mit relevanten Daten routen
2. **Adapter Location**: Shard mit geladenem Adapter bevorzugen
3. **Load**: Shards mit niedriger Last bevorzugen
4. **Latency**: Geografisch nähesten Shard wählen

```cpp
// Adapter-Aware Query Router
class LLMQueryRouter {
public:
    std::string route(
        const std::string& query,
        const std::string& adapter_id,
        const RoutingHint& hint = {}
    ) {
        // 1. Get shards with adapter
        auto candidate_shards = adapter_registry_.getShardsForAdapter(adapter_id);
        
        // 2. Filter by data affinity
        if (hint.has_data_filter) {
            candidate_shards = filterByDataAffinity(
                candidate_shards, hint.data_filter
            );
        }
        
        // 3. Select based on load + latency
        auto selected_shard = selectBestShard(candidate_shards, {
            .prefer_low_load = true,
            .prefer_low_latency = true,
            .weights = {0.6, 0.4}  // 60% load, 40% latency
        });
        
        // 4. Execute on selected shard
        return executor_.execute(selected_shard, {
            {"command", "llm_generate"},
            {"query", query},
            {"adapter_id", adapter_id}
        }).text;
    }
    
private:
    std::vector<std::string> filterByDataAffinity(
        const std::vector<std::string>& shards,
        const std::string& data_filter
    ) {
        // Use ShardRouter to determine which shards have matching data
        std::vector<std::string> filtered;
        for (const auto& shard_id : shards) {
            if (router_.hasMatchingData(shard_id, data_filter)) {
                filtered.push_back(shard_id);
            }
        }
        return filtered.empty() ? shards : filtered;
    }
};
```

### 6.5 Cross-Shard Adapter Ensembles

**Idee:** Kombiniere Antworten von mehreren Adaptern auf verschiedenen Shards.

```cpp
// Multi-Adapter Ensemble Query
std::string ensembleQuery(
    const std::string& query,
    const std::vector<std::string>& adapter_ids
) {
    // Scatter query to shards with different adapters
    std::vector<std::future<std::string>> responses;
    
    for (const auto& adapter_id : adapter_ids) {
        auto shard = router_.routeToAdapter(adapter_id);
        responses.push_back(std::async([=]() {
            return executor_.execute(shard, {
                {"command", "llm_generate"},
                {"query", query},
                {"adapter_id", adapter_id}
            }).text;
        }));
    }
    
    // Gather all responses
    std::vector<std::string> all_responses;
    for (auto& fut : responses) {
        all_responses.push_back(fut.get());
    }
    
    // Merge/Vote on best response
    return mergeResponses(all_responses);
}
```

**AQL Ensemble Query:**
```sql
-- Query multiple adapters and merge results
SELECT llm_ensemble_generate(
    'Welche rechtlichen und medizinischen Aspekte gibt es?',
    adapters: ['legal_qa_v1', 'medical_v1'],
    merge_strategy: 'VOTE'  -- or 'CONCAT', 'BEST_SCORE'
);
```

### 6.6 Performance & Scalability

**Expected Performance:**

| Metric | Single Shard | 4 Shards (Distributed) | 16 Shards |
|--------|--------------|------------------------|-----------|
| **Training Throughput** | 100 samples/s | 380 samples/s (3.8x) | 1400 samples/s (14x) |
| **Inference Latency** | 50ms | 52ms (+2ms network) | 55ms (+5ms network) |
| **Adapter Load Time** | 2s | 2s (parallel) | 2s (parallel) |
| **Failover Time** | N/A | <100ms (replica) | <100ms (replica) |
| **Max Concurrent Queries** | 100 QPS | 400 QPS | 1600 QPS |

**Network Overhead:**
```
Gradient Sync per Step:
- LoRA rank=8, 32 layers: ~4MB per sync
- Sync frequency: every 100 steps
- Network usage: 40KB/step (acceptable)

Adapter Replication:
- Adapter size: 16MB (rank=8)
- Replication to 4 shards: 64MB total
- One-time cost, amortized over queries
```

### 6.7 Fault Tolerance & Recovery

**Shard Failure Handling:**
```cpp
// Automatic failover for adapter queries
class ResilientLLMService {
public:
    std::string query(
        const std::string& text,
        const std::string& adapter_id
    ) {
        auto shards = adapter_registry_.getShardsForAdapter(adapter_id);
        
        for (size_t attempt = 0; attempt < shards.size(); ++attempt) {
            try {
                auto shard = shards[attempt];
                
                // Circuit breaker check
                if (circuit_breaker_.isOpen(shard)) {
                    continue;  // Skip unhealthy shard
                }
                
                auto result = executor_.execute(shard, {...});
                circuit_breaker_.recordSuccess(shard);
                return result.text;
                
            } catch (const ShardException& e) {
                circuit_breaker_.recordFailure(shard);
                // Try next replica
            }
        }
        
        throw std::runtime_error("All adapter replicas failed");
    }
    
private:
    sharding::CircuitBreaker circuit_breaker_;
};
```

**Training Checkpoint Recovery:**
```cpp
// Coordinator handles shard failures during training
void handleShardFailure(const std::string& failed_shard) {
    // 1. Mark shard as unhealthy
    topology_.markUnhealthy(failed_shard);
    
    // 2. Redistribute data from failed shard
    auto data_range = topology_.getDataRange(failed_shard);
    auto backup_shards = topology_.getReplicasFor(failed_shard);
    
    // 3. Resume training without failed shard
    active_shards_.erase(failed_shard);
    
    // 4. Adjust gradient aggregation (exclude failed shard)
    sync_strategy_.excludeShard(failed_shard);
    
    // 5. Log & alert
    logger_.warn("Shard {} failed, training continues with {} shards",
                 failed_shard, active_shards_.size());
}
```

---

## 6.8 Adapter Compatibility Validation & Error Prevention

**Problem:** Verhindern, dass inkompatible Adapter auf falschen Base-Models geladen werden.

### 6.8.1 Compatibility Checking Strategy

```cpp
// include/llm/adapter_compatibility_validator.h
namespace themis::llm {

class AdapterCompatibilityValidator {
public:
    // Validation result
    struct ValidationResult {
        bool is_compatible;
        std::string error_message;
        std::vector<std::string> warnings;
        
        // Detailed mismatch info
        struct Mismatch {
            std::string field;
            std::string expected;
            std::string actual;
        };
        std::vector<Mismatch> mismatches;
    };
    
    // Validate adapter against base model
    ValidationResult validate(
        const AdapterMetadata& adapter,
        const ModelMetadata& base_model
    ) {
        ValidationResult result;
        result.is_compatible = true;
        
        // 1. Check model name match
        if (adapter.base_model_name != base_model.model_name) {
            result.is_compatible = false;
            result.mismatches.push_back({
                "base_model_name",
                adapter.base_model_name,
                base_model.model_name
            });
            result.error_message = fmt::format(
                "Adapter '{}' requires base model '{}' but got '{}'",
                adapter.adapter_id,
                adapter.base_model_name,
                base_model.model_name
            );
        }
        
        // 2. Check architecture compatibility
        if (!checkArchitectureCompatibility(adapter, base_model)) {
            result.is_compatible = false;
            result.error_message += "\nArchitecture mismatch detected.";
        }
        
        // 3. Check dimension compatibility
        if (!checkDimensionCompatibility(adapter, base_model)) {
            result.is_compatible = false;
            result.error_message += "\nDimension mismatch detected.";
        }
        
        // 4. Check tokenizer compatibility
        if (adapter.tokenizer_hash != base_model.tokenizer_hash) {
            result.warnings.push_back(
                "Tokenizer mismatch - inference may produce unexpected results"
            );
        }
        
        return result;
    }
    
private:
    bool checkArchitectureCompatibility(
        const AdapterMetadata& adapter,
        const ModelMetadata& base_model
    ) {
        // Verify target modules exist in base model
        for (const auto& module : adapter.training_config.target_modules) {
            if (!base_model.hasModule(module)) {
                return false;
            }
        }
        return true;
    }
    
    bool checkDimensionCompatibility(
        const AdapterMetadata& adapter,
        const ModelMetadata& base_model
    ) {
        // Check LoRA dimensions match model dimensions
        for (const auto& [layer_name, dimensions] : adapter.layer_dimensions) {
            auto model_dims = base_model.getLayerDimensions(layer_name);
            if (dimensions.d != model_dims.d || dimensions.k != model_dims.k) {
                return false;
            }
        }
        return true;
    }
};

} // namespace themis::llm
```

### 6.8.2 Runtime Validation in Deployment

```cpp
// Automatic validation before adapter deployment
class SafeAdapterDeploymentManager : public AdapterDeploymentManager {
public:
    void deployAdapter(
        const std::string& adapter_id,
        const std::string& adapter_path,
        const std::string& target_shard
    ) override {
        // 1. Load adapter metadata
        auto adapter_meta = loadAdapterMetadata(adapter_path);
        
        // 2. Get base model info from target shard
        auto base_model_info = executor_.execute(target_shard, {
            {"command", "get_model_info"}
        });
        
        // 3. Validate compatibility
        AdapterCompatibilityValidator validator;
        auto validation = validator.validate(adapter_meta, base_model_info);
        
        if (!validation.is_compatible) {
            throw IncompatibleAdapterException(
                validation.error_message
            );
        }
        
        // 4. Log warnings
        for (const auto& warning : validation.warnings) {
            logger_.warn("Adapter deployment warning: {}", warning);
        }
        
        // 5. Proceed with deployment
        AdapterDeploymentManager::deployAdapter(
            adapter_id, adapter_path, target_shard
        );
        
        // 6. Register deployment with validation info
        registry_.registerValidatedDeployment(
            adapter_id,
            target_shard,
            validation
        );
    }
};
```

### 6.8.3 AQL-Level Validation

```sql
-- Automatic validation in AQL DEPLOY statement
DEPLOY ADAPTER legal_qa_v1
  TO SHARD 'shard_legal'
  WITH strategy = 'CO_LOCATED',
       validate_compatibility = TRUE;  -- Default: TRUE

-- Output bei Fehler:
-- ERROR: Adapter 'legal_qa_v1' incompatible with base model
-- Expected: mistralai/Mistral-7B-v0.1
-- Found: meta-llama/Llama-2-7b-hf
-- Suggestion: Re-train adapter on Llama-2-7b or deploy to Mistral shard
```

### 6.8.4 Adapter Registry with Base Model Grouping

```cpp
// Gruppierung von Adapters nach Base-Model
class BaseModelAwareAdapterRegistry {
public:
    struct BaseModelGroup {
        std::string base_model_name;
        std::string base_model_version;
        std::vector<AdapterInfo> adapters;
        std::vector<std::string> deployed_shards;
    };
    
    // Get all adapters for a specific base model
    std::vector<AdapterInfo> getAdaptersForBaseModel(
        const std::string& base_model_name
    ) {
        return base_model_groups_[base_model_name].adapters;
    }
    
    // List all base models with their adapter counts
    std::map<std::string, size_t> listBaseModels() {
        std::map<std::string, size_t> result;
        for (const auto& [model_name, group] : base_model_groups_) {
            result[model_name] = group.adapters.size();
        }
        return result;
    }
    
    // Register adapter with automatic grouping
    void registerAdapter(const AdapterMetadata& metadata) {
        auto& group = base_model_groups_[metadata.base_model_name];
        group.base_model_name = metadata.base_model_name;
        group.adapters.push_back(AdapterInfo::from(metadata));
    }
    
private:
    std::map<std::string, BaseModelGroup> base_model_groups_;
};
```

### 6.8.5 Query-Time Validation

```cpp
// Validate adapter before query execution
std::string queryWithValidation(
    const std::string& query,
    const std::string& adapter_id
) {
    // 1. Get adapter info
    auto adapter_info = registry_.getAdapter(adapter_id);
    
    // 2. Find shard with adapter
    auto candidate_shards = registry_.getShardsForAdapter(adapter_id);
    
    for (const auto& shard_id : candidate_shards) {
        // 3. Verify base model compatibility
        auto shard_model = topology_.getBaseModel(shard_id);
        
        if (shard_model != adapter_info.base_model_name) {
            logger_.error(
                "Shard {} has wrong base model for adapter {}. "
                "Expected: {}, Got: {}",
                shard_id,
                adapter_id,
                adapter_info.base_model_name,
                shard_model
            );
            continue;  // Try next shard
        }
        
        // 4. Execute query on validated shard
        try {
            return executor_.execute(shard_id, {
                {"command", "llm_generate"},
                {"query", query},
                {"adapter_id", adapter_id}
            }).text;
        } catch (const ShardException& e) {
            continue;  // Failover to next shard
        }
    }
    
    throw NoCompatibleShardException(
        "No shard with compatible base model found for adapter " + adapter_id
    );
}
```

### 6.8.6 Migration Assistant für Base-Model Wechsel

```cpp
// Tool für Base-Model Migration
class BaseModelMigrationAssistant {
public:
    struct MigrationPlan {
        std::string old_base_model;
        std::string new_base_model;
        
        struct AdapterMigration {
            std::string adapter_id;
            std::string training_data_path;  // Kann wiederverwendet werden
            LoRAConfig lora_config;          // Kann wiederverwendet werden
            bool requires_retraining = true;
        };
        std::vector<AdapterMigration> adapters;
        
        // Estimated effort
        size_t total_samples_to_retrain;
        double estimated_training_hours;
    };
    
    // Create migration plan
    MigrationPlan planMigration(
        const std::string& from_model,
        const std::string& to_model
    ) {
        MigrationPlan plan;
        plan.old_base_model = from_model;
        plan.new_base_model = to_model;
        
        // Get all adapters for old model
        auto adapters = registry_.getAdaptersForBaseModel(from_model);
        
        for (const auto& adapter : adapters) {
            AdapterMigration migration;
            migration.adapter_id = adapter.adapter_id;
            migration.training_data_path = adapter.data_source_uri;
            migration.lora_config = adapter.training_config;
            migration.requires_retraining = true;
            
            plan.adapters.push_back(migration);
            plan.total_samples_to_retrain += adapter.num_training_samples;
        }
        
        // Estimate training time
        plan.estimated_training_hours = 
            estimateTrainingTime(plan.total_samples_to_retrain, to_model);
        
        return plan;
    }
    
    // Execute migration (re-train all adapters)
    void executeMigration(const MigrationPlan& plan) {
        for (const auto& adapter : plan.adapters) {
            logger_.info("Re-training adapter {} on new base model {}",
                        adapter.adapter_id, plan.new_base_model);
            
            // Create new adapter ID
            std::string new_adapter_id = adapter.adapter_id + "_" + 
                                        sanitize(plan.new_base_model);
            
            // Re-train using existing training data
            TrainingPlan training_plan;
            training_plan.adapter_id = new_adapter_id;
            training_plan.base_model = plan.new_base_model;
            training_plan.lora_config = adapter.lora_config;
            training_plan.training_data_source = adapter.training_data_path;
            
            trainer_.train(training_plan);
        }
    }
};
```

### 6.8.7 Best Practices für Fehlerprävention

**1. Strikte Naming Convention:**
```cpp
// Adapter ID enthält Base-Model Info
std::string generateAdapterID(
    const std::string& domain,
    const std::string& base_model,
    const std::string& version
) {
    // Format: {domain}_{base_model_short}_{version}
    // Beispiel: legal_mistral7b_v1
    std::string model_short = shortenModelName(base_model);
    return fmt::format("{}_{}_v{}", domain, model_short, version);
}
```

**2. Metadata Checksums:**
```cpp
// Verify adapter integrity
struct AdapterChecksum {
    std::string base_model_hash;  // SHA256 of model architecture
    std::string weights_hash;     // SHA256 of adapter weights
    std::string config_hash;      // SHA256 of LoRA config
};

bool verifyAdapterIntegrity(
    const std::string& adapter_path,
    const AdapterChecksum& expected
) {
    auto actual = computeAdapterChecksum(adapter_path);
    return actual.base_model_hash == expected.base_model_hash &&
           actual.weights_hash == expected.weights_hash;
}
```

**3. Automatic Testing:**
```cpp
// Test adapter compatibility during CI/CD
void testAdapterCompatibility(const std::string& adapter_path) {
    auto adapter_meta = loadAdapterMetadata(adapter_path);
    auto base_model = loadBaseModel(adapter_meta.base_model_name);
    
    // Try to load adapter
    auto model_with_adapter = base_model.loadAdapter(adapter_path);
    
    // Test inference
    auto test_input = "Test query";
    auto output = model_with_adapter.generate(test_input);
    
    // Verify output shape
    ASSERT_EQ(output.shape(), expected_shape);
}
```

---

## 7. Recommended Final Architecture (with Sharding)

```
┌─────────────────────────────────────────────────────────┐
│                  ThemisDB Training Stack                │
│              (Unabhängig von Llama.cpp)                 │
└─────────────────────────────────────────────────────────┘

Layer 4: User Interface
┌─────────────────────┐  ┌──────────────────┐
│  CLI: themisdb train│  │  Python API      │
│  --adapter legal-v1 │  │  ThemisTrainer() │
└──────────┬──────────┘  └─────────┬────────┘
           │                       │
           └───────────┬───────────┘
                       ▼
Layer 3: Training Orchestration (Python)
┌─────────────────────────────────────────┐
│  ThemisDB Training Library (Python)     │
│  - Config Generator (YAML/Python)       │
│  - Metadata Integration                 │
│  - Experiment Tracking (Wandb/MLflow)   │
│  - Checkpoint→vLLM Deployment           │
└──────────────────┬──────────────────────┘
                   ▼
Layer 2: Training Framework (Choose One)
┌──────────────┐  ┌──────────────┐  ┌──────────┐
│   Axolotl    │  │   Unsloth    │  │   PEFT   │
│  (Standard)  │  │ (Fast/Memory)│  │ (Custom) │
└──────┬───────┘  └──────┬───────┘  └─────┬────┘
       │                  │                 │
       └──────────────────┼─────────────────┘
                          ▼
Layer 1: Data Source (ThemisDB)
┌─────────────────────────────────────────┐
│  ThemisDB HTTP API (v1.2.0)             │
│  - /api/export/jsonl_llm/stream         │
│  - /api/adapters/{id}/metadata          │
│  - Streaming IterableDataset Support   │
└─────────────────────────────────────────┘

                          │
                          ▼ (v1.3.0+)
┌─────────────────────────────────────────┐
│  Optional: Llama.cpp Inferenz           │
│  - Native LLM Execution                 │
│  - LoRA Adapter Loading                 │
│  - GPU/CPU Inference                    │
└─────────────────────────────────────────┘
```

### 3.2 Komponenten

#### A) Python Training Library: `themisdb-trainer` (PyPI Package)

**Funktionen:**
- ThemisDB Dataset Loader (IterableDataset)
- Config Generator aus Adapter Metadata
- Framework Abstraction Layer (Axolotl/Unsloth/PEFT)
- Wandb/MLflow Integration
- Automatic Model→vLLM Deployment

**Installation:**
```bash
pip install themisdb-trainer
```

**Usage:**
```python
from themisdb_trainer import ThemisTrainer, ThemisConfig

# Config von ThemisDB Metadata
config = ThemisConfig.from_themis_adapter(
    themis_url='http://themisdb:8765',
    adapter_id='legal-qa-v1',
    framework='axolotl'  # oder 'unsloth', 'peft'
)

# Training
trainer = ThemisTrainer(config)
trainer.train()
trainer.deploy_to_vllm()  # Automatisches Deployment
```

#### B) CLI Tool: `themisdb train`

```bash
# Einfachstes Training
themisdb train --adapter legal-qa-v1

# Mit Framework-Wahl
themisdb train --adapter legal-qa-v1 --framework unsloth

# Custom Config
themisdb train --config custom_config.yaml

# Deployment nach Training
themisdb train --adapter legal-qa-v1 --deploy-vllm
```

#### C) Optional: C++ Training Adapter (DLL/SO) 🟡

**Nur wenn absolut nötig** (z.B. für native DB Integration ohne Python).

**Use Case:**
- Training direkt aus C++ Server (ohne Python)
- Custom Gradient Updates basierend auf DB Queries
- Zero-Copy Training mit RocksDB Memory

**Aufwand:** 🔴 **Hoch** (2-4 Wochen)
- Implementierung kompletter Backpropagation in C++
- CUDA Kernel Development
- Optimizer Implementation (Adam, SGD)
- Kein ROI außer für sehr spezielle Use Cases

**Empfehlung:** ❌ **NICHT empfohlen**
- Python Training Libraries (Axolotl/Unsloth) sind ausgereift
- C++ Training hat keinen Performance-Vorteil (GPU-bound, nicht CPU)
- Python→C++ Interop (pybind11) ist einfacher als vollständige C++ Reimplementierung

---

## 4. Empfohlene Implementierung (OOP + Best Practices)

### 4.1 Python Package Structure

```
themisdb-trainer/
├── themisdb_trainer/
│   ├── __init__.py
│   ├── config.py          # Configuration Management
│   ├── datasets.py        # ThemisDB IterableDataset
│   ├── trainers/
│   │   ├── __init__.py
│   │   ├── base.py        # Abstract Trainer Interface
│   │   ├── axolotl.py     # Axolotl Wrapper
│   │   ├── unsloth.py     # Unsloth Wrapper
│   │   └── peft.py        # PEFT Wrapper
│   ├── deployers/
│   │   ├── __init__.py
│   │   ├── vllm.py        # vLLM Deployment
│   │   └── huggingface.py # HF Hub Upload
│   └── utils/
│       ├── metadata.py    # Metadata Parsing
│       └── tracking.py    # Wandb/MLflow
├── tests/
├── examples/
└── setup.py
```

### 4.2 OOP Design: Abstract Trainer Interface

```python
# themisdb_trainer/trainers/base.py
from abc import ABC, abstractmethod
from typing import Optional
from ..config import ThemisConfig

class BaseTrainer(ABC):
    """Abstract base class for all training framework adapters."""
    
    def __init__(self, config: ThemisConfig):
        self.config = config
        self.model = None
        self.dataset = None
    
    @abstractmethod
    def load_model(self) -> None:
        """Load base model with LoRA configuration."""
        pass
    
    @abstractmethod
    def load_dataset(self) -> None:
        """Load training data from ThemisDB."""
        pass
    
    @abstractmethod
    def train(self) -> dict:
        """Execute training loop. Returns metrics."""
        pass
    
    @abstractmethod
    def save_adapter(self, output_path: str) -> None:
        """Save LoRA adapter to disk."""
        pass
    
    def validate(self) -> dict:
        """Validate trained adapter. Optional override."""
        return {}
    
    def get_metadata(self) -> dict:
        """Generate adapter metadata for ThemisDB registry."""
        return {
            'adapter_id': self.config.adapter_id,
            'adapter_version': self.config.adapter_version,
            'base_model': self.config.base_model,
            'training_config': {
                'lora_rank': self.config.lora_rank,
                'lora_alpha': self.config.lora_alpha,
                'lora_dropout': self.config.lora_dropout,
                'learning_rate': self.config.learning_rate,
                'num_epochs': self.config.num_epochs,
            }
        }
```

### 4.3 Concrete Implementation: Axolotl Trainer

```python
# themisdb_trainer/trainers/axolotl.py
from .base import BaseTrainer
import yaml
import subprocess

class AxolotlTrainer(BaseTrainer):
    """Axolotl framework wrapper following OOP principles."""
    
    def load_model(self) -> None:
        # Generate Axolotl YAML config from ThemisDB metadata
        config_yaml = self._generate_axolotl_config()
        with open('/tmp/axolotl_config.yaml', 'w') as f:
            yaml.dump(config_yaml, f)
    
    def load_dataset(self) -> None:
        # ThemisDB streaming dataset
        from ..datasets import ThemisDBStreamingDataset
        self.dataset = ThemisDBStreamingDataset(
            base_url=self.config.themis_url,
            adapter_id=self.config.adapter_id
        )
    
    def train(self) -> dict:
        # Call Axolotl CLI
        result = subprocess.run([
            'accelerate', 'launch', '-m', 'axolotl.cli.train',
            '/tmp/axolotl_config.yaml'
        ], capture_output=True)
        
        # Parse training metrics
        return self._parse_metrics(result.stdout)
    
    def save_adapter(self, output_path: str) -> None:
        # Axolotl saves automatically, just move/verify
        import shutil
        shutil.move(f'{self.config.output_dir}/adapter_model', output_path)
    
    def _generate_axolotl_config(self) -> dict:
        """Generate Axolotl YAML from ThemisDB metadata."""
        return {
            'base_model': self.config.base_model,
            'model_type': 'MistralForCausalLM',
            'datasets': [{
                'path': f'{self.config.themis_url}/api/export/jsonl_llm/stream',
                'type': 'custom',
                'streaming': True,
            }],
            'adapter': 'lora',
            'lora_r': self.config.lora_rank,
            'lora_alpha': self.config.lora_alpha,
            'lora_dropout': self.config.lora_dropout,
            'lora_target_modules': self.config.target_modules,
            'output_dir': self.config.output_dir,
            'num_epochs': self.config.num_epochs,
            'learning_rate': self.config.learning_rate,
        }
```

### 4.4 Factory Pattern für Framework-Auswahl

```python
# themisdb_trainer/trainers/__init__.py
from .base import BaseTrainer
from .axolotl import AxolotlTrainer
from .unsloth import UnslothTrainer
from .peft import PEFTTrainer

class TrainerFactory:
    """Factory for creating trainer instances based on framework choice."""
    
    _trainers = {
        'axolotl': AxolotlTrainer,
        'unsloth': UnslothTrainer,
        'peft': PEFTTrainer,
    }
    
    @classmethod
    def create(cls, framework: str, config) -> BaseTrainer:
        """Create trainer instance for specified framework."""
        if framework not in cls._trainers:
            raise ValueError(f"Unknown framework: {framework}. "
                           f"Available: {list(cls._trainers.keys())}")
        return cls._trainers[framework](config)
```

### 4.5 High-Level API

```python
# themisdb_trainer/__init__.py
from .config import ThemisConfig
from .trainers import TrainerFactory

class ThemisTrainer:
    """High-level API for ThemisDB LoRA/QLoRA training."""
    
    def __init__(self, config: ThemisConfig):
        self.config = config
        self.trainer = TrainerFactory.create(config.framework, config)
    
    def train(self) -> dict:
        """Execute complete training workflow."""
        print(f"[ThemisDB] Loading model: {self.config.base_model}")
        self.trainer.load_model()
        
        print(f"[ThemisDB] Loading dataset from: {self.config.themis_url}")
        self.trainer.load_dataset()
        
        print(f"[ThemisDB] Starting training (framework: {self.config.framework})")
        metrics = self.trainer.train()
        
        print(f"[ThemisDB] Saving adapter: {self.config.adapter_id}")
        self.trainer.save_adapter(f'./adapters/{self.config.adapter_id}')
        
        # Update ThemisDB registry
        self._register_adapter(self.trainer.get_metadata())
        
        return metrics
    
    def deploy_to_vllm(self, vllm_server: str = 'http://vllm:8000'):
        """Deploy trained adapter to vLLM server."""
        from .deployers.vllm import VLLMDeployer
        deployer = VLLMDeployer(vllm_server)
        deployer.deploy(f'./adapters/{self.config.adapter_id}')
    
    def _register_adapter(self, metadata: dict):
        """Register adapter in ThemisDB adapter registry."""
        import requests
        requests.post(
            f'{self.config.themis_url}/api/adapters/register',
            json={'adapter_metadata': metadata}
        )
```

---

## 5. Empfohlener Workflow

### 5.1 End-to-End Beispiel

```python
from themisdb_trainer import ThemisTrainer, ThemisConfig

# 1. Lade Config von ThemisDB (basierend auf Metadata)
config = ThemisConfig.from_themis_adapter(
    themis_url='http://themisdb:8765',
    adapter_id='legal-qa-v1',
    framework='axolotl',  # Oder 'unsloth' für Performance
    num_epochs=3,
    learning_rate=2e-4,
)

# 2. Training
trainer = ThemisTrainer(config)
metrics = trainer.train()
print(f"Training complete: {metrics}")

# 3. Deployment zu vLLM
trainer.deploy_to_vllm('http://vllm-server:8000')

# 4. Testen
from openai import OpenAI
client = OpenAI(base_url='http://vllm-server:8000/v1')
response = client.completions.create(
    model='mistralai/Mistral-7B-v0.1',
    prompt='Was sind die Voraussetzungen für eine Baugenehmigung?',
    extra_body={'lora_name': 'legal-qa-v1'}
)
print(response.choices[0].text)
```

### 5.2 CLI Workflow

```bash
# 1. Training starten
themisdb train \
  --adapter legal-qa-v1 \
  --framework axolotl \
  --themis-url http://themisdb:8765 \
  --epochs 3

# 2. Deploy zu vLLM
themisdb deploy \
  --adapter legal-qa-v1 \
  --vllm-server http://vllm:8000

# 3. Testen
curl -X POST http://vllm:8000/v1/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "mistralai/Mistral-7B-v0.1",
    "prompt": "Legal question...",
    "extra_body": {"lora_name": "legal-qa-v1"}
  }'
```

---

## 6. Aufwands-Schätzung

### Option A: Python Training Library + Axolotl Integration 🟢 **Empfohlen**

**Aufwand:** 1-2 Wochen (1 Entwickler)

- **Woche 1:**
  - ThemisDB IterableDataset Loader (1 Tag)
  - Config Generator aus Metadata (1 Tag)
  - Axolotl Wrapper Implementation (2 Tage)
  - CLI Tool + Tests (1 Tag)

- **Woche 2:**
  - vLLM Deployment Integration (2 Tage)
  - Wandb/MLflow Tracking (1 Tag)
  - Dokumentation + Beispiele (1 Tag)
  - Integration Tests (1 Tag)

**Deliverables:**
- ✅ PyPI Package: `themisdb-trainer`
- ✅ CLI: `themisdb train`
- ✅ Dokumentation + Tutorials
- ✅ Beispiel Workflows

### Option B: Python Library + Multi-Framework Support (Axolotl + Unsloth + PEFT)

**Aufwand:** 2-3 Wochen (1 Entwickler)

- Option A (1-2 Wochen)
- + Unsloth Wrapper (2 Tage)
- + PEFT Wrapper (2 Tage)
- + Factory Pattern + Tests (1 Tag)

### Option C: C++ Training Adapter (DLL/SO) 🔴 **NICHT Empfohlen**

**Aufwand:** 1-2 Monate (2-3 Entwickler)

- Gradient Computation in C++ (1 Woche)
- CUDA Kernel Development (2 Wochen)
- Optimizer Implementation (Adam, SGD) (1 Woche)
- Bindings (pybind11) (1 Woche)
- Testing + Benchmarking (1 Woche)

**ROI:** ❌ Negativ
- Python Frameworks (Axolotl/Unsloth) sind ausgereift
- Training ist GPU-bound, nicht CPU-bound
- Maintenance-Aufwand deutlich höher als Python

---

## 7. Empfehlung

### 🎯 **Empfohlene Lösung: Option A (Python + Axolotl)**

**Begründung:**
1. ✅ **Niedrigster Aufwand** (1-2 Wochen)
2. ✅ **Production-Ready** (Axolotl wird von vielen Firmen genutzt)
3. ✅ **Best Practices** (Flash Attention 2, Gradient Checkpointing)
4. ✅ **OOP Design** (Abstract Trainer, Factory Pattern)
5. ✅ **Erweiterbar** (Unsloth/PEFT später hinzufügbar)
6. ✅ **Zero Vendor Lock-In** (Apache 2.0 License)

**Migration Path:**
- Phase 1: Axolotl Integration (1-2 Wochen)
- Phase 2: Unsloth für Performance-Critical Use Cases (optional, +2 Tage)
- Phase 3: PEFT für Custom Workflows (optional, +2 Tage)

### ❌ **NICHT Empfohlen: C++ Adapter**

**Begründung:**
- Aufwand zu hoch (1-2 Monate vs. 1-2 Wochen)
- Kein Performance-Vorteil (Training ist GPU-bound)
- Hoher Maintenance-Aufwand
- Python Frameworks sind state-of-the-art

---

## 8. Nächste Schritte

### Phase 1: Prototyp (1 Woche)
- [ ] ThemisDB IterableDataset implementieren
- [ ] Axolotl Config Generator aus Metadata
- [ ] Basis CLI Tool (`themisdb train`)
- [ ] Proof-of-Concept Training mit echten ThemisDB Daten

### Phase 2: Production-Ready (1 Woche)
- [ ] PyPI Package Publishing
- [ ] vLLM Deployment Automation
- [ ] Wandb/MLflow Integration
- [ ] Comprehensive Tests + CI/CD

### Phase 3: Erweiterungen (optional)
- [ ] Unsloth Integration (Performance)
- [ ] PEFT Integration (Flexibilität)
- [ ] Web UI für Training Monitoring
- [ ] Automatisches Hyperparameter Tuning

---

## 9. Referenzen

### Training Frameworks
- [Axolotl](https://github.com/OpenAccess-AI-Collective/axolotl) - Production LoRA Training
- [Unsloth](https://github.com/unslothai/unsloth) - Fast + Memory-Efficient Training
- [PEFT](https://github.com/huggingface/peft) - HuggingFace Parameter-Efficient Fine-Tuning
- [LLaMA Factory](https://github.com/hiyouga/LLaMA-Factory) - Multi-Backend Training Platform

### ThemisDB Dokumentation
- [vLLM Multi-LoRA Integration](../exporters/VLLM_MULTI_LORA_INTEGRATION.md)
- [JSONL LLM Exporter](../exporters/JSONL_LLM_EXPORTER.md)
- [Streaming JSONL Training API](../api/STREAMING_JSONL_TRAINING.md)
- [LoRA Adapter Metadata](../exporters/LORA_ADAPTER_METADATA.md)

### Best Practices
- [HuggingFace Fine-Tuning Guide](https://huggingface.co/docs/transformers/training)
- [LoRAExchange.ai Metadata Standard](https://loraexchange.ai/)
- [PEFT Documentation](https://huggingface.co/docs/peft/)

---

**Status:** Ready for Implementation  
**Recommended Timeline:** 1-2 Weeks for Option A (Python + Axolotl)  
**Next Action:** Create Prototype ThemisDB Training Library
