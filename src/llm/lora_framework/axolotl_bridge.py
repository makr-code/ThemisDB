# @file axolotl_bridge.py
# @brief Axolotl/PEFT integration for ThemisDB LoRA training.


"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            axolotl_bridge.py                                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     511                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Axolotl/PEFT Integration Bridge for ThemisDB QLoRA Training

This module provides integration between ThemisDB's QLoRA training system
and external tools like Axolotl and HuggingFace PEFT.

Features:
- Configuration conversion (ThemisDB ↔ Axolotl)
- Data export to Alpaca/JSONL format
- Adapter conversion (PEFT ↔ GGUF)
- Training orchestration
"""

import os
import json
import random
import sys
import yaml
import subprocess
import logging
from pathlib import Path
from typing import Dict, List, Optional, Any, Union
from dataclasses import dataclass, asdict

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


@dataclass
class QLoRAConfig:
    """QLoRA configuration from ThemisDB"""
    enabled: bool = False
    quantization_type: str = "nf4"  # "nf4", "int8", "int4"
    block_size: int = 64
    use_double_quantization: bool = False
    layer_by_layer: bool = True
    use_paged_optimizer: bool = False
    optimizer_offload: str = "none"


@dataclass
class LoRAHyperparameters:
    """LoRA hyperparameters from ThemisDB"""
    rank: int = 8
    alpha: float = 16.0
    dropout: float = 0.1
    learning_rate: float = 3e-4
    batch_size: int = 4
    num_epochs: int = 3
    max_seq_length: int = 512
    optimizer: str = "adamw"
    beta1: float = 0.9
    beta2: float = 0.999
    epsilon: float = 1e-8
    weight_decay: float = 0.01
    momentum: float = 0.0
    lr_scheduler: str = "constant"
    warmup_steps: int = 0
    lr_decay_gamma: float = 0.1
    lr_step_size: int = 100
    target_modules: List[str] = None
    
    def __post_init__(self):
        if self.target_modules is None:
            self.target_modules = ["q_proj", "v_proj"]


@dataclass
class ThemisDBTrainingConfig:
    """Complete training configuration from ThemisDB"""
    adapter_id: str
    base_model_path: str
    output_dir: str
    hyperparameters: LoRAHyperparameters
    qlora: QLoRAConfig
    mixed_precision: bool = True
    gradient_accumulation_steps: int = 4
    enable_checkpointing: bool = True
    checkpoint_interval_steps: int = 100
    checkpoint_dir: str = "data/lora_checkpoints"
    max_samples: int = 1000

    @classmethod
    def from_dict(cls, value: Dict[str, Any]) -> "ThemisDBTrainingConfig":
        config = dict(value)
        config["hyperparameters"] = LoRAHyperparameters(**config["hyperparameters"])
        config["qlora"] = QLoRAConfig(**config.get("qlora", {}))
        return cls(**config)


class AxolotlConfigGenerator:
    """Generate Axolotl configuration from ThemisDB config"""
    
    @staticmethod
    def generate(config: ThemisDBTrainingConfig, data_file: str) -> Dict[str, Any]:
        """
        Generate Axolotl configuration from ThemisDB config
        
        Args:
            config: ThemisDB training configuration
            data_file: Path to training data file
            
        Returns:
            Axolotl configuration dictionary
        """
        hp = config.hyperparameters
        qlora = config.qlora
        
        # Map quantization type
        quant_mapping = {
            "nf4": True,  # load_in_4bit
            "int8": False,  # load_in_8bit
            "int4": True,
        }
        
        axolotl_config = {
            # Base model
            "base_model": config.base_model_path,
            "model_type": "AutoModelForCausalLM",
            "tokenizer_type": "AutoTokenizer",
            
            # QLoRA/Quantization
            "adapter": "qlora" if qlora.enabled else "lora",
            "load_in_4bit": quant_mapping.get(qlora.quantization_type, True) if qlora.enabled else False,
            "load_in_8bit": (qlora.quantization_type == "int8") if qlora.enabled else False,
            
            # LoRA parameters
            "lora_r": hp.rank,
            "lora_alpha": int(hp.alpha),
            "lora_dropout": hp.dropout,
            "lora_target_modules": hp.target_modules,
            "lora_target_linear": True,
            
            # Training hyperparameters
            "sequence_len": hp.max_seq_length,
            "sample_packing": True,
            "pad_to_sequence_len": True,
            "gradient_accumulation_steps": config.gradient_accumulation_steps,
            "micro_batch_size": hp.batch_size,
            "num_epochs": hp.num_epochs,
            "learning_rate": hp.learning_rate,
            "lr_scheduler": hp.lr_scheduler,
            "warmup_steps": hp.warmup_steps,
            
            # Optimizer
            "optimizer": AxolotlConfigGenerator._map_optimizer(hp.optimizer, qlora.enabled),
            "weight_decay": hp.weight_decay,
            "adam_beta1": hp.beta1,
            "adam_beta2": hp.beta2,
            "adam_epsilon": hp.epsilon,
            
            # Precision and optimization
            "bf16": config.mixed_precision,
            "fp16": False,
            "tf32": True,
            "gradient_checkpointing": True,
            "flash_attention": True,
            
            # Data
            "datasets": [
                {
                    "path": data_file,
                    "type": "alpaca"
                }
            ],
            
            # Output
            "output_dir": config.output_dir,
            "saves_per_epoch": 1,
            
            # Logging
            "logging_steps": 1,
            "eval_steps": 10 if hp.num_epochs > 1 else None,
            "save_steps": config.checkpoint_interval_steps,
            "save_strategy": "steps",
            
            # Checkpointing
            "resume_from_checkpoint": True if config.enable_checkpointing else False,
        }
        
        # Add double quantization if enabled
        if qlora.enabled and qlora.use_double_quantization:
            axolotl_config["bnb_4bit_use_double_quant"] = True
        
        # Add paged optimizer if enabled
        if qlora.enabled and qlora.use_paged_optimizer:
            axolotl_config["bnb_4bit_compute_dtype"] = "bfloat16"
            axolotl_config["bnb_4bit_quant_type"] = "nf4"
        
        return axolotl_config
    
    @staticmethod
    def _map_optimizer(themis_opt: str, qlora_enabled: bool) -> str:
        """Map ThemisDB optimizer to Axolotl optimizer"""
        if qlora_enabled:
            # Use 8-bit optimizers for QLoRA
            opt_map = {
                "adamw": "adamw_bnb_8bit",
                "adam": "adam_bnb_8bit",
                "sgd": "sgd",
            }
        else:
            opt_map = {
                "adamw": "adamw_torch",
                "adam": "adam",
                "sgd": "sgd",
            }
        return opt_map.get(themis_opt.lower(), "adamw_bnb_8bit" if qlora_enabled else "adamw_torch")


class DataExporter:
    """Export ThemisDB training data to Alpaca/JSONL format"""

    @staticmethod
    def _is_excluded_source(path: str) -> bool:
        excluded_parts = {
            ".git", ".pytest_cache", ".mypy_cache", ".ruff_cache", ".cache",
            "__pycache__", ".venv", "venv", "node_modules", "build", "artifacts",
            "logs", "tmp", "temp", "downloads",
        }
        return any(part in excluded_parts for part in path.replace("\\", "/").lower().split("/"))

    @staticmethod
    def load_from_file(input_file: str, max_samples: int = 1000) -> List[Dict[str, Any]]:
        """Stream a docs artifact or JSON sample list without loading it all into memory."""
        if max_samples <= 0:
            raise ValueError("max_samples must be positive")
        try:
            import ijson
        except ImportError as exc:
            raise RuntimeError("Streaming JSON support is required; install ijson") from exc

        path = Path(input_file)
        with path.open("rb") as data_file:
            first_byte = data_file.read(1)
            data_file.seek(0)
            if first_byte == b"[":
                return list(ijson.items(data_file, "item"))[:max_samples]

        documents = {}
        with path.open("rb") as data_file:
            for document in ijson.items(data_file, "artifact_documents.item"):
                if isinstance(document, dict) and document.get("doc_id"):
                    documents[document["doc_id"]] = document

        samples = []
        with path.open("rb") as data_file:
            for chunk in ijson.items(data_file, "chunks.item"):
                if not isinstance(chunk, dict):
                    continue
                text = chunk.get("text")
                if not isinstance(text, str) or not text.strip():
                    continue
                document = documents.get(chunk.get("doc_id"), {})
                location = document.get("title") or document.get("file_path") or chunk.get("doc_id", "")
                if DataExporter._is_excluded_source(document.get("file_path", "")):
                    continue
                samples.append({
                    "instruction": "Continue the ThemisDB technical documentation accurately.",
                    "input": f"Documentation source: {location}",
                    "output": text.strip(),
                    "metadata": {
                        "doc_id": chunk.get("doc_id", ""),
                        "chunk_id": chunk.get("chunk_id", ""),
                    },
                })
                if len(samples) >= max_samples:
                    return samples
        if samples:
            return samples

        for prefix in ("samples.item", "documents.item"):
            with path.open("rb") as data_file:
                for item in ijson.items(data_file, prefix):
                    if prefix == "samples.item":
                        sample = item
                    else:
                        if not isinstance(item, dict):
                            continue
                        text = item.get("content")
                        if not isinstance(text, str) or not text.strip():
                            continue
                        metadata = item.get("metadata", {})
                        if not isinstance(metadata, dict):
                            metadata = {}
                        location = item.get("file_path") or metadata.get("file_name", "")
                        if DataExporter._is_excluded_source(location):
                            continue
                        sample = {
                            "instruction": "Continue the ThemisDB technical documentation accurately.",
                            "input": f"Documentation source: {location}",
                            "output": text.strip(),
                            "metadata": {"file_path": location},
                        }
                    if isinstance(sample, dict):
                        samples.append(sample)
                    if len(samples) >= max_samples:
                        return samples
        return samples

    @staticmethod
    def normalize_training_data(data: Union[List[Dict[str, Any]], Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Normalize standard samples or the generated documentation artifact."""
        if isinstance(data, list):
            return [sample for sample in data if isinstance(sample, dict)]
        if not isinstance(data, dict):
            raise ValueError("Training data must be a sample list or documentation artifact")

        document_list = data.get("artifact_documents", data.get("documents", []))
        if not isinstance(document_list, list):
            document_list = []
        documents = {
            doc.get("doc_id"): doc
            for doc in document_list
            if isinstance(doc, dict) and doc.get("doc_id")
        }
        chunks = data.get("chunks", [])
        samples = []
        if isinstance(chunks, list):
            for chunk in chunks:
                if not isinstance(chunk, dict):
                    continue
                text = chunk.get("text")
                if not isinstance(text, str) or not text.strip():
                    continue
                document = documents.get(chunk.get("doc_id"), {})
                location = document.get("title") or document.get("file_path") or chunk.get("doc_id", "")
                if DataExporter._is_excluded_source(document.get("file_path", "")):
                    continue
                samples.append({
                    "instruction": "Continue the ThemisDB technical documentation accurately.",
                    "input": f"Documentation source: {location}",
                    "output": text.strip(),
                    "metadata": {
                        "doc_id": chunk.get("doc_id", ""),
                        "chunk_id": chunk.get("chunk_id", ""),
                    },
                })
        if samples:
            return samples

        for document in document_list:
            if not isinstance(document, dict):
                continue
            text = document.get("content")
            if not isinstance(text, str) or not text.strip():
                continue
            metadata = document.get("metadata", {})
            if not isinstance(metadata, dict):
                metadata = {}
            location = document.get("file_path") or metadata.get("file_name", "")
            if DataExporter._is_excluded_source(location):
                continue
            samples.append({
                "instruction": "Continue the ThemisDB technical documentation accurately.",
                "input": f"Documentation source: {location}",
                "output": text.strip(),
                "metadata": {"file_path": location},
            })
        return samples
    
    @staticmethod
    def export_to_alpaca(samples: List[Dict[str, Any]], output_file: str) -> int:
        """
        Export training samples to Alpaca JSONL format
        
        Args:
            samples: List of training samples with 'input' and 'output' keys
            output_file: Output JSONL file path
            
        Returns:
            Number of samples exported
        """
        output_path = Path(output_file)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        count = 0
        with open(output_file, 'w', encoding='utf-8') as f:
            for sample in samples:
                alpaca_sample = {
                    "instruction": sample.get("instruction") or sample.get("prompt") or sample.get("input", ""),
                    "input": sample.get("context", "") if "instruction" not in sample else sample.get("input", ""),
                    "output": sample.get("output", ""),
                }
                # Add metadata if present
                if "metadata" in sample:
                    alpaca_sample["metadata"] = sample["metadata"]
                
                f.write(json.dumps(alpaca_sample, ensure_ascii=False) + '\n')
                count += 1
        
        logger.info(f"Exported {count} samples to {output_file}")
        return count
    
    @staticmethod
    def export_to_json(samples: List[Dict[str, str]], output_file: str) -> int:
        """
        Export training samples to JSON array format
        
        Args:
            samples: List of training samples
            output_file: Output JSON file path
            
        Returns:
            Number of samples exported
        """
        output_path = Path(output_file)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(samples, f, ensure_ascii=False, indent=2)
        
        logger.info(f"Exported {len(samples)} samples to {output_file}")
        return len(samples)


class AxolotlTrainer:
    """Orchestrate training with Axolotl"""
    
    def __init__(self, config: ThemisDBTrainingConfig):
        self.config = config
        self.work_dir = Path(config.output_dir)
        self.work_dir.mkdir(parents=True, exist_ok=True)
    
    def train(self, training_data: Union[List[Dict[str, Any]], Dict[str, Any]]) -> Dict[str, Any]:
        """
        Train LoRA adapter using Axolotl
        
        Args:
            training_data: List of training samples
            
        Returns:
            Training result dictionary
        """
        logger.info(f"Starting Axolotl training for adapter: {self.config.adapter_id}")
        
        # Step 1: Normalize and export training data
        samples = DataExporter.normalize_training_data(training_data)
        if self.config.max_samples <= 0:
            return {"success": False, "error": "max_samples must be positive", "num_samples": 0}
        samples = samples[:self.config.max_samples]
        data_file = self.work_dir / "training_data.jsonl"
        num_samples = DataExporter.export_to_alpaca(samples, str(data_file))
        if num_samples == 0:
            return {
                "success": False,
                "error": "No usable documentation or training samples were provided",
                "data_file": str(data_file),
                "num_samples": 0,
            }
        
        # Step 2: Generate Axolotl config
        axolotl_config = AxolotlConfigGenerator.generate(self.config, str(data_file))
        config_file = self.work_dir / "axolotl_config.yml"
        with open(config_file, 'w') as f:
            yaml.dump(axolotl_config, f, default_flow_style=False)
        logger.info(f"Generated Axolotl config: {config_file}")
        
        # Step 3: Use Axolotl when installed; otherwise train directly with PEFT.
        try:
            if self._check_axolotl_available():
                result = self._run_axolotl_training(config_file)
            else:
                result = self._run_peft_training(samples)
            result["config_file"] = str(config_file)
            result["data_file"] = str(data_file)
            result["num_samples"] = num_samples
            return result
        except Exception as e:
            logger.error(f"Training failed: {e}")
            return {
                "success": False,
                "error": str(e),
                "config_file": str(config_file),
                "data_file": str(data_file),
                "num_samples": num_samples
            }
    
    def _check_axolotl_available(self) -> bool:
        """Check if Axolotl is available"""
        try:
            subprocess.run([sys.executable, "-c", "import axolotl"],
                          check=True, capture_output=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False

    def _run_peft_training(self, samples: List[Dict[str, Any]]) -> Dict[str, Any]:
        """Fine-tune a Hugging Face causal LM with real PEFT LoRA gradients."""
        try:
            import torch
            from peft import LoraConfig, TaskType, get_peft_model
            from transformers import AutoModelForCausalLM, AutoTokenizer
        except ImportError as exc:
            return {
                "success": False,
                "error": f"Transformers PEFT training dependencies are missing: {exc}",
            }

        if self.config.qlora.enabled:
            return {
                "success": False,
                "error": "The PEFT backend currently supports LoRA, not QLoRA; set qlora.enabled to false",
            }
        if not Path(self.config.base_model_path).is_dir():
            return {
                "success": False,
                "error": "The PEFT backend requires a local Hugging Face model directory",
            }

        try:
            tokenizer = AutoTokenizer.from_pretrained(self.config.base_model_path, local_files_only=True)
            if tokenizer.pad_token_id is None:
                tokenizer.pad_token = tokenizer.eos_token

            device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
            model_dtype = torch.float16 if device.type == "cuda" else torch.float32
            model = AutoModelForCausalLM.from_pretrained(
                self.config.base_model_path,
                torch_dtype=model_dtype,
                local_files_only=True,
            ).to(device)
            model.config.use_cache = False
            model.gradient_checkpointing_enable(
                gradient_checkpointing_kwargs={"use_reentrant": False}
            )

            hp = self.config.hyperparameters
            if hp.rank <= 0 or hp.max_seq_length <= 1 or hp.num_epochs <= 0 or hp.batch_size <= 0:
                return {"success": False, "error": "LoRA rank, epochs, batch size, and sequence length must be positive"}
            target_modules = hp.target_modules or ["q_proj", "v_proj"]
            model = get_peft_model(model, LoraConfig(
                r=hp.rank,
                lora_alpha=hp.alpha,
                lora_dropout=hp.dropout,
                target_modules=target_modules,
                bias="none",
                task_type=TaskType.CAUSAL_LM,
            ))
            model.train()

            encoded_samples = []
            max_length = hp.max_seq_length
            for sample in samples:
                instruction = str(sample.get("instruction") or sample.get("prompt") or sample.get("input", "")).strip()
                context = str(sample.get("context", "") if "instruction" not in sample else sample.get("input", "")).strip()
                output = str(sample.get("output", "")).strip()
                if not instruction or not output:
                    continue

                prompt = instruction if not context else f"{instruction}\n\n{context}"
                prompt_ids = tokenizer(prompt, add_special_tokens=True)["input_ids"]
                target_ids = tokenizer(output, add_special_tokens=False)["input_ids"]
                if tokenizer.eos_token_id is not None:
                    target_ids.append(tokenizer.eos_token_id)
                target_ids = target_ids[:max_length]
                prompt_budget = max(0, max_length - len(target_ids))
                prompt_ids = prompt_ids[-prompt_budget:] if prompt_budget else []
                input_ids = prompt_ids + target_ids
                labels = ([-100] * len(prompt_ids)) + target_ids
                if target_ids:
                    encoded_samples.append((input_ids, labels))

            if not encoded_samples:
                return {"success": False, "error": "No samples contain both a prompt and completion"}

            optimizer = torch.optim.AdamW(
                (parameter for parameter in model.parameters() if parameter.requires_grad),
                lr=hp.learning_rate,
                weight_decay=hp.weight_decay,
            )
            final_loss = 0.0
            steps = 0
            for epoch in range(hp.num_epochs):
                    order = list(range(len(encoded_samples)))
                    random.Random(42 + epoch).shuffle(order)
                    epoch_samples = [encoded_samples[index] for index in order]
                    for start in range(0, len(epoch_samples), hp.batch_size):
                        batch = epoch_samples[start:start + hp.batch_size]
                        batch_length = max(len(item[0]) for item in batch)
                        input_ids = torch.full(
                            (len(batch), batch_length), tokenizer.pad_token_id,
                            dtype=torch.long, device=device,
                        )
                        labels = torch.full_like(input_ids, -100)
                        attention_mask = torch.zeros_like(input_ids)
                        for row, (tokens, targets) in enumerate(batch):
                            input_ids[row, :len(tokens)] = torch.tensor(tokens, dtype=torch.long, device=device)
                            labels[row, :len(targets)] = torch.tensor(targets, dtype=torch.long, device=device)
                            attention_mask[row, :len(tokens)] = 1

                        optimizer.zero_grad(set_to_none=True)
                        loss = model(input_ids=input_ids, attention_mask=attention_mask, labels=labels).loss
                        if not torch.isfinite(loss):
                            raise RuntimeError("Training produced a non-finite loss")
                        loss.backward()
                        torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
                        optimizer.step()
                        final_loss = float(loss.detach().cpu())
                        steps += 1

            output_dir = Path(self.config.output_dir)
            output_dir.mkdir(parents=True, exist_ok=True)
            model.save_pretrained(output_dir, safe_serialization=True)
            tokenizer.save_pretrained(output_dir)
            adapter_config = output_dir / "adapter_config.json"
            adapter_weights = output_dir / "adapter_model.safetensors"
            if not adapter_config.is_file() or not adapter_weights.is_file() or adapter_weights.stat().st_size == 0:
                raise RuntimeError("PEFT did not produce a non-empty adapter artifact")

            return {
                "success": True,
                "adapter_path": str(output_dir),
                "trainer_backend": "transformers-peft",
                "final_loss": final_loss,
                "epochs_completed": hp.num_epochs,
                "training_steps": steps,
                "num_samples": len(encoded_samples),
            }
        except Exception as exc:
            logger.exception("Transformers PEFT training failed")
            return {"success": False, "error": str(exc)}
    
    def _run_axolotl_training(self, config_file: Path) -> Dict[str, Any]:
        """Run Axolotl training subprocess"""
        cmd = [sys.executable, "-m", "axolotl.cli.train", str(config_file)]
        
        logger.info(f"Running: {' '.join(cmd)}")
        
        # Run training
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True
        )
        
        # Stream output
        output_lines = []
        for line in process.stdout:
            logger.info(line.rstrip())
            output_lines.append(line)
        
        # Wait for completion
        return_code = process.wait()
        
        if return_code == 0:
            logger.info("Training completed successfully")
            return {
                "success": True,
                "output": "".join(output_lines),
                "adapter_path": str(self.config.output_dir)
            }
        else:
            logger.error(f"Training failed with code {return_code}")
            return {
                "success": False,
                "error": f"Training process exited with code {return_code}",
                "output": "".join(output_lines)
            }


class AdapterConverter:
    """Convert between PEFT and GGUF adapter formats"""
    
    @staticmethod
    def peft_to_gguf(peft_dir: str, output_file: str, base_model: str,
                     convert_script: Optional[str] = None,
                     outtype: str = "f16") -> bool:
        """
        Convert a PEFT LoRA adapter directory to a standalone GGUF LoRA adapter.

        Uses llama.cpp's ``convert_lora_to_gguf.py`` (the former
        ``convert-lora-to-gguf.py`` name no longer exists upstream):
        ``python convert_lora_to_gguf.py <peft_dir> --base <hf_base_dir>
        --outfile <out.gguf> --outtype f16`` (``--base-model-id <hf_id>`` when
        ``base_model`` is not a local directory). The adapter is never merged
        into the base weights. AdaLoRA checkpoints (lora_E tensors) must first
        be materialised as plain LoRA, see ``scripts/lora/themis_lora_train.py``.

        Args:
            peft_dir: Directory containing adapter_config.json and adapter weights
            output_file: Output GGUF file path
            base_model: Local HF base model directory or HF model id
            convert_script: Path to convert_lora_to_gguf.py (default:
                ./llama.cpp/convert_lora_to_gguf.py)
            outtype: GGUF tensor type passed to --outtype (default f16)

        Returns:
            True if successful
        """
        try:
            script = (
                Path(convert_script)
                if convert_script
                else Path(__file__).resolve().parents[3] / "llama.cpp" / "convert_lora_to_gguf.py"
            )

            if not script.exists():
                logger.warning(f"Conversion script not found: {script}")
                logger.warning("Provide the path to llama.cpp convert_lora_to_gguf.py")
                return False

            cmd = [
                sys.executable, str(script), str(peft_dir),
                "--outfile", str(output_file), "--outtype", outtype,
            ]
            if Path(base_model).is_dir():
                cmd += ["--base", str(base_model)]
            else:
                cmd += ["--base-model-id", str(base_model)]

            logger.info(f"Converting PEFT to GGUF: {peft_dir} -> {output_file}")
            result = subprocess.run(cmd, check=True, capture_output=True, text=True)
            if not Path(output_file).is_file() or Path(output_file).stat().st_size == 0:
                logger.error("GGUF converter completed without producing a non-empty adapter")
                return False
            logger.info("Conversion successful")
            logger.debug(result.stdout)
            return Path(output_file).is_file()

        except subprocess.CalledProcessError as e:
            logger.error(f"Conversion failed: {e}")
            logger.error(e.stderr)
            return False
        except Exception as e:
            logger.error(f"Conversion error: {e}")
            return False


class AxolotlBridge:
    """Main bridge class for Axolotl/PEFT integration"""
    
    def __init__(self, config: ThemisDBTrainingConfig):
        self.config = config
        self.trainer = AxolotlTrainer(config)
    
    def train_and_convert(self, training_data: Union[List[Dict[str, Any]], Dict[str, Any]],
                         convert_to_gguf: bool = True) -> Dict[str, Any]:
        """
        Complete training pipeline: train with Axolotl and optionally convert to GGUF
        
        Args:
            training_data: Training samples
            convert_to_gguf: Whether to convert adapter to GGUF format
            
        Returns:
            Complete result dictionary
        """
        # Train with Axolotl
        train_result = self.trainer.train(training_data)
        
        if not train_result.get("success", False):
            return train_result
        
        # Convert to GGUF if requested
        if convert_to_gguf:
            gguf_file = Path(self.config.output_dir) / f"{self.config.adapter_id}.gguf"
            success = AdapterConverter.peft_to_gguf(
                train_result["adapter_path"],
                str(gguf_file),
                self.config.base_model_path
            )
            train_result["gguf_conversion"] = {
                "success": success,
                "output_file": str(gguf_file) if success else None
            }
            if not success:
                train_result["success"] = False
                train_result["error"] = "GGUF adapter conversion failed"
        
        return train_result


# CLI interface for testing
def main():
    """Test CLI interface"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Axolotl/PEFT Bridge for ThemisDB")
    parser.add_argument("--config", required=True, help="ThemisDB training config (JSON)")
    parser.add_argument("--data", required=True, help="Training data (JSON)")
    parser.add_argument("--no-convert", action="store_true", help="Skip GGUF conversion")
    
    args = parser.parse_args()
    
    # Load config
    with open(args.config) as f:
        config_dict = json.load(f)
    
    # Load data
    # Create bridge
    config = ThemisDBTrainingConfig.from_dict(config_dict)
    training_data = DataExporter.load_from_file(args.data, config.max_samples)
    bridge = AxolotlBridge(config)
    
    # Train
    result = bridge.train_and_convert(training_data, convert_to_gguf=not args.no_convert)
    
    # Print result
    print(json.dumps(result, indent=2))
    if not result.get("success", False):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
