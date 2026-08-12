/**
 * @file axolotl_bridge.py
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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
    enabled: bool = True
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
    def export_to_alpaca(samples: List[Dict[str, str]], output_file: str) -> int:
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
                    "instruction": sample.get("input", ""),
                    "input": "",
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
    
    def train(self, training_data: List[Dict[str, str]]) -> Dict[str, Any]:
        """
        Train LoRA adapter using Axolotl
        
        Args:
            training_data: List of training samples
            
        Returns:
            Training result dictionary
        """
        logger.info(f"Starting Axolotl training for adapter: {self.config.adapter_id}")
        
        # Step 1: Export training data
        data_file = self.work_dir / "training_data.jsonl"
        num_samples = DataExporter.export_to_alpaca(training_data, str(data_file))
        
        # Step 2: Generate Axolotl config
        axolotl_config = AxolotlConfigGenerator.generate(self.config, str(data_file))
        config_file = self.work_dir / "axolotl_config.yml"
        with open(config_file, 'w') as f:
            yaml.dump(axolotl_config, f, default_flow_style=False)
        logger.info(f"Generated Axolotl config: {config_file}")
        
        # Step 3: Check if Axolotl is available
        if not self._check_axolotl_available():
            logger.warning("Axolotl not found. Install with: pip install axolotl")
            return {
                "success": False,
                "error": "Axolotl not installed",
                "config_file": str(config_file),
                "data_file": str(data_file),
                "num_samples": num_samples
            }
        
        # Step 4: Run Axolotl training
        try:
            result = self._run_axolotl_training(config_file)
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
            subprocess.run(["python", "-c", "import axolotl"], 
                          check=True, capture_output=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
    
    def _run_axolotl_training(self, config_file: Path) -> Dict[str, Any]:
        """Run Axolotl training subprocess"""
        cmd = ["python", "-m", "axolotl.cli.train", str(config_file)]
        
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
    def peft_to_gguf(peft_dir: str, output_file: str, base_model: str) -> bool:
        """
        Convert PEFT adapter to GGUF format
        
        Args:
            peft_dir: Directory containing PEFT adapter
            output_file: Output GGUF file path
            base_model: Base model name or path
            
        Returns:
            True if successful
        """
        try:
            # Check if conversion script exists
            llama_cpp_dir = Path("llama.cpp")
            convert_script = llama_cpp_dir / "convert-lora-to-gguf.py"
            
            if not convert_script.exists():
                logger.warning(f"Conversion script not found: {convert_script}")
                logger.warning("Please clone llama.cpp repository")
                return False
            
            # Run conversion
            cmd = [
                "python", str(convert_script),
                "--input", peft_dir,
                "--output", output_file,
                "--base", base_model
            ]
            
            logger.info(f"Converting PEFT to GGUF: {peft_dir} -> {output_file}")
            result = subprocess.run(cmd, check=True, capture_output=True, text=True)
            logger.info("Conversion successful")
            logger.debug(result.stdout)
            return True
            
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
    
    def train_and_convert(self, training_data: List[Dict[str, str]], 
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
    with open(args.data) as f:
        training_data = json.load(f)
    
    # Create bridge
    config = ThemisDBTrainingConfig(**config_dict)
    bridge = AxolotlBridge(config)
    
    # Train
    result = bridge.train_and_convert(training_data, convert_to_gguf=not args.no_convert)
    
    # Print result
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
