# LoRA Training für RESPO

Anleitung zum Fine-Tuning von Code-LLMs mit LoRA (Low-Rank Adaptation).

## Voraussetzungen

- NVIDIA GPU mit mindestens 16 GB VRAM (24+ GB empfohlen)
- CUDA 11.8+ und cuDNN
- Python 3.10+

## Installation

```bash
cd training
pip install -r requirements.txt
```

## Trainings-Daten

### Datenformat

Training Data im JSONL Format:

```jsonl
{"instruction": "Implementiere einen LRU Cache", "input": "", "output": "class LRUCache:\n    ..."}
{"instruction": "Erkläre diesen Code", "input": "def foo(): pass", "output": "Diese Funktion..."}
{"instruction": "Finde den Bug", "input": "def div(a, b): return a/b", "output": "Division by zero..."}
```

### Datenquellen

1. **Eigener Code**
   - Interne Repositories
   - Code Reviews
   - Dokumentation

2. **Öffentliche Datasets**
   - The Stack (HuggingFace)
   - CodeSearchNet
   - MBPP (Benchmarks)

## Training starten

```bash
# Python-spezifisches Training
python train_lora.py --config configs/python.yaml

# TypeScript Training
python train_lora.py --config configs/typescript.yaml

# Mit DeepSpeed (Multi-GPU)
deepspeed train_lora.py --config configs/python.yaml --deepspeed configs/ds_config.json
```

## Konfiguration

Siehe `configs/` für Beispiel-Konfigurationen:

- `base.yaml` - Basis-Konfiguration
- `python.yaml` - Python-spezifisch
- `typescript.yaml` - TypeScript-spezifisch

### Wichtige Hyperparameter

| Parameter | Empfehlung | Beschreibung |
|-----------|------------|--------------|
| `r` | 64 | LoRA Rank (höher = mehr Parameter) |
| `lora_alpha` | 128 | Alpha Scaling Factor |
| `learning_rate` | 2e-4 | Lernrate |
| `num_epochs` | 3 | Anzahl Epochen |
| `batch_size` | 4 | Batch-Größe pro GPU |

## Evaluation

```bash
# HumanEval Benchmark
python evaluate.py --model ./output/respo-python --benchmark humaneval

# MBPP Benchmark
python evaluate.py --model ./output/respo-python --benchmark mbpp

# Custom Tests
python evaluate.py --model ./output/respo-python --test-file tests/custom.jsonl
```

## vLLM Deployment

Nach dem Training kann der LoRA Adapter in vLLM geladen werden:

```bash
python -m vllm.entrypoints.openai.api_server \
    --model codellama/CodeLlama-13b-Instruct-hf \
    --enable-lora \
    --lora-modules respo-python=./output/respo-python
```

## Tipps

1. **Starte klein**: Erst mit wenigen Daten testen
2. **Überwache Loss**: Früh stoppen bei Overfitting
3. **Validierung**: Regelmäßig auf Testset evaluieren
4. **Speicher**: Gradient Checkpointing bei VRAM-Mangel

## Bekannte Probleme

- **OOM**: Batch-Größe reduzieren oder Gradient Checkpointing aktivieren
- **Slow Training**: Prüfen ob GPU genutzt wird (`nvidia-smi`)
- **Bad Results**: Mehr Daten, anderes LR, längeres Training
