# ThemisDB LoRA Adapters

This directory stores trained LoRA adapter weights that are loaded by the
`llamacpp` plugin at inference time (configured in `config/config.yaml`
under `llm_plugins.llamacpp.lora.preload`).

## Adapters

| File | ID | Base model | Purpose |
|------|----|-----------|---------|
| `themis-docs-v1.bin` | `themis-docs-v1` | TinyLlama 1.1B Q4_0 | ThemisDB documentation assistant (RAG, config help, troubleshooting) |

## Generating `themis-docs-v1.bin`

The adapter must be trained before the server can load it. Follow these steps:

### Prerequisites

```bash
pip install transformers peft datasets tqdm
```

You also need the base model GGUF placed at `models/tinyllama-1.1b-q4_0.gguf`
and the SentencePiece tokenizer at `models/tinyllama-tokenizer.model`.

Download links:
- GGUF: <https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF>
- Tokenizer: <https://huggingface.co/TinyLlama/TinyLlama-1.1B-Chat-v1.0>

### Training

```bash
python scripts/train_lora.py \
  --config config/lora_training_config.yaml \
  --adapter themis_help_lora \
  --output loras/themis-docs-v1.bin
```

The training script reads:
- Training corpus: `data/docs_database.json` (ThemisDB documentation)
- User feedback: collection `help_feedback` (optional – requires a running ThemisDB instance)
- Hyperparameters: `config/lora_training_config.yaml` → `adapters.themis_help_lora`

Typical wall-clock time on a single RTX 3080: ~20 minutes for 3 epochs over
the bundled documentation corpus.

### Server-side loading

Once `themis-docs-v1.bin` is present in this directory, the adapter is
loaded automatically on server start via `config/config.yaml`:

```yaml
llm_plugins:
  llamacpp:
    lora:
      preload:
        - id: "themis-docs-v1"
          path: "loras/themis-docs-v1.bin"
          scale: 1.0
```

The server logs a startup message of the form:

```
[LLM] LoRA adapter 'themis-docs-v1' loaded (scale=1.0)
```

If the file is missing, the server emits a warning and continues without
the adapter (graceful degradation, `llm.required: false`).

## Adding new adapters

1. Define the adapter in `config/lora_training_config.yaml` under `adapters:`.
2. Train with `scripts/train_lora.py --adapter <adapter_id> --output loras/<file>.bin`.
3. Register the output file in `config/config.yaml` under
   `llm_plugins.llamacpp.lora.preload`.
4. Restart the server.

## Security

Adapter files are signed at export time by the LoRA framework
(`lora.security.enable_signatures: true` in `config/lora_training_config.yaml`).
The signature is verified by `LloraSecurityValidator` on load.
Do **not** place unsigned third-party adapter files in this directory.
