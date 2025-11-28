"""
LoRA Trainer

Fine-tune code LLMs using LoRA (Low-Rank Adaptation).
"""

import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Optional

import structlog

logger = structlog.get_logger(__name__)


@dataclass
class DatasetConfig:
    """Dataset configuration."""
    
    # Data format
    format: str = "jsonl"  # jsonl, parquet, csv
    
    # Column names
    instruction_column: str = "instruction"
    input_column: str = "input"
    output_column: str = "output"
    
    # Preprocessing
    max_length: int = 2048
    truncation: bool = True
    padding: str = "max_length"
    
    # Data split
    train_split: float = 0.9
    eval_split: float = 0.1
    
    # Filtering
    min_length: int = 10
    max_examples: Optional[int] = None


@dataclass
class TrainingConfig:
    """LoRA training configuration."""
    
    # Model
    base_model: str = "codellama/CodeLlama-7b-Instruct-hf"
    
    # Output
    output_dir: str = "./output/lora"
    
    # LoRA parameters
    lora_r: int = 16
    lora_alpha: int = 32
    lora_dropout: float = 0.05
    lora_target_modules: list[str] = field(default_factory=lambda: [
        "q_proj", "k_proj", "v_proj", "o_proj",
        "gate_proj", "up_proj", "down_proj",
    ])
    
    # Training parameters
    num_epochs: int = 3
    batch_size: int = 4
    gradient_accumulation_steps: int = 4
    learning_rate: float = 2e-4
    weight_decay: float = 0.01
    warmup_ratio: float = 0.03
    lr_scheduler_type: str = "cosine"
    
    # Optimization
    optim: str = "paged_adamw_8bit"
    fp16: bool = True
    bf16: bool = False
    gradient_checkpointing: bool = True
    
    # Evaluation
    eval_steps: int = 100
    save_steps: int = 500
    logging_steps: int = 10
    
    # Hardware
    device_map: str = "auto"
    max_memory: Optional[dict] = None
    
    # Quantization
    load_in_4bit: bool = True
    bnb_4bit_compute_dtype: str = "float16"
    bnb_4bit_quant_type: str = "nf4"
    use_nested_quant: bool = False
    
    # Dataset
    dataset: DatasetConfig = field(default_factory=DatasetConfig)
    
    @classmethod
    def from_yaml(cls, filepath: str) -> "TrainingConfig":
        """Load config from YAML file."""
        import yaml
        
        with open(filepath) as f:
            data = yaml.safe_load(f)
        
        # Handle nested dataset config
        if "dataset" in data:
            data["dataset"] = DatasetConfig(**data["dataset"])
        
        return cls(**data)
    
    def to_yaml(self, filepath: str) -> None:
        """Save config to YAML file."""
        import yaml
        from dataclasses import asdict
        
        data = asdict(self)
        
        with open(filepath, "w") as f:
            yaml.dump(data, f, default_flow_style=False)


@dataclass
class TrainingResult:
    """Training result."""
    
    # Status
    success: bool
    
    # Metrics
    final_loss: float
    best_loss: float
    training_time_seconds: float
    
    # Output
    output_path: str
    adapter_path: str
    
    # Training history
    history: list[dict[str, float]] = field(default_factory=list)
    
    # Evaluation metrics
    eval_metrics: dict[str, float] = field(default_factory=dict)
    
    def to_dict(self) -> dict[str, Any]:
        return {
            "success": self.success,
            "final_loss": self.final_loss,
            "best_loss": self.best_loss,
            "training_time_seconds": self.training_time_seconds,
            "output_path": self.output_path,
            "adapter_path": self.adapter_path,
            "history": self.history,
            "eval_metrics": self.eval_metrics,
        }
    
    def save(self, filepath: str) -> None:
        """Save results to JSON."""
        Path(filepath).write_text(json.dumps(self.to_dict(), indent=2))


class LoRATrainer:
    """
    LoRA trainer for code LLMs.
    
    Uses PEFT (Parameter-Efficient Fine-Tuning) with LoRA for efficient
    fine-tuning of large language models on code datasets.
    """
    
    def __init__(self, config: TrainingConfig):
        """Initialize the trainer."""
        self.config = config
        self._model = None
        self._tokenizer = None
        self._peft_config = None
    
    def _setup_quantization(self):
        """Setup quantization config."""
        try:
            from transformers import BitsAndBytesConfig
            import torch
            
            compute_dtype = getattr(torch, self.config.bnb_4bit_compute_dtype)
            
            return BitsAndBytesConfig(
                load_in_4bit=self.config.load_in_4bit,
                bnb_4bit_quant_type=self.config.bnb_4bit_quant_type,
                bnb_4bit_compute_dtype=compute_dtype,
                bnb_4bit_use_double_quant=self.config.use_nested_quant,
            )
        except ImportError:
            logger.warning("bitsandbytes not available, using fp16")
            return None
    
    def _load_model(self):
        """Load the base model."""
        try:
            from transformers import AutoModelForCausalLM, AutoTokenizer
            import torch
        except ImportError:
            raise ImportError("transformers not installed. Install with: pip install transformers")
        
        logger.info("Loading base model", model=self.config.base_model)
        
        # Quantization config
        bnb_config = self._setup_quantization()
        
        # Load tokenizer
        self._tokenizer = AutoTokenizer.from_pretrained(
            self.config.base_model,
            trust_remote_code=True,
        )
        
        # Set padding token if not set
        if self._tokenizer.pad_token is None:
            self._tokenizer.pad_token = self._tokenizer.eos_token
        
        # Load model
        model_kwargs = {
            "device_map": self.config.device_map,
            "trust_remote_code": True,
        }
        
        if bnb_config:
            model_kwargs["quantization_config"] = bnb_config
        elif self.config.fp16:
            model_kwargs["torch_dtype"] = torch.float16
        
        if self.config.max_memory:
            model_kwargs["max_memory"] = self.config.max_memory
        
        self._model = AutoModelForCausalLM.from_pretrained(
            self.config.base_model,
            **model_kwargs,
        )
        
        # Enable gradient checkpointing
        if self.config.gradient_checkpointing:
            self._model.gradient_checkpointing_enable()
        
        logger.info("Model loaded", params=sum(p.numel() for p in self._model.parameters()))
    
    def _setup_lora(self):
        """Setup LoRA configuration."""
        try:
            from peft import LoraConfig, get_peft_model, prepare_model_for_kbit_training
        except ImportError:
            raise ImportError("peft not installed. Install with: pip install peft")
        
        logger.info(
            "Setting up LoRA",
            r=self.config.lora_r,
            alpha=self.config.lora_alpha,
            target_modules=self.config.lora_target_modules,
        )
        
        # Prepare model for training
        if self.config.load_in_4bit:
            self._model = prepare_model_for_kbit_training(self._model)
        
        # LoRA config
        self._peft_config = LoraConfig(
            r=self.config.lora_r,
            lora_alpha=self.config.lora_alpha,
            lora_dropout=self.config.lora_dropout,
            target_modules=self.config.lora_target_modules,
            bias="none",
            task_type="CAUSAL_LM",
        )
        
        # Apply LoRA
        self._model = get_peft_model(self._model, self._peft_config)
        
        # Print trainable params
        trainable_params = sum(p.numel() for p in self._model.parameters() if p.requires_grad)
        total_params = sum(p.numel() for p in self._model.parameters())
        
        logger.info(
            "LoRA applied",
            trainable_params=trainable_params,
            total_params=total_params,
            trainable_pct=f"{100 * trainable_params / total_params:.2f}%",
        )
    
    def _load_dataset(self, data_path: str):
        """Load and prepare the training dataset."""
        try:
            from datasets import load_dataset, Dataset
        except ImportError:
            raise ImportError("datasets not installed. Install with: pip install datasets")
        
        logger.info("Loading dataset", path=data_path)
        
        data_path = Path(data_path)
        dataset_config = self.config.dataset
        
        # Load based on format
        if data_path.is_dir():
            # Directory with files
            dataset = load_dataset("json", data_dir=str(data_path))
        elif data_path.suffix == ".jsonl" or data_path.suffix == ".json":
            dataset = load_dataset("json", data_files=str(data_path))
        elif data_path.suffix == ".parquet":
            dataset = load_dataset("parquet", data_files=str(data_path))
        elif data_path.suffix == ".csv":
            dataset = load_dataset("csv", data_files=str(data_path))
        else:
            raise ValueError(f"Unsupported data format: {data_path.suffix}")
        
        # Get train split
        if "train" in dataset:
            dataset = dataset["train"]
        
        # Apply max examples limit
        if dataset_config.max_examples:
            dataset = dataset.select(range(min(len(dataset), dataset_config.max_examples)))
        
        # Format for training
        def format_prompt(example):
            """Format example as instruction prompt."""
            instruction = example.get(dataset_config.instruction_column, "")
            input_text = example.get(dataset_config.input_column, "")
            output = example.get(dataset_config.output_column, "")
            
            if input_text:
                prompt = f"### Instruction:\n{instruction}\n\n### Input:\n{input_text}\n\n### Response:\n{output}"
            else:
                prompt = f"### Instruction:\n{instruction}\n\n### Response:\n{output}"
            
            return {"text": prompt}
        
        dataset = dataset.map(format_prompt)
        
        # Filter by length
        def filter_length(example):
            tokens = self._tokenizer(example["text"], truncation=False)
            length = len(tokens["input_ids"])
            return length >= dataset_config.min_length and length <= dataset_config.max_length
        
        dataset = dataset.filter(filter_length)
        
        # Split
        dataset = dataset.train_test_split(test_size=dataset_config.eval_split)
        
        logger.info(
            "Dataset prepared",
            train_examples=len(dataset["train"]),
            eval_examples=len(dataset["test"]),
        )
        
        return dataset
    
    def train(self, data_path: str) -> TrainingResult:
        """
        Train the model with LoRA.
        
        Args:
            data_path: Path to training data (JSONL, Parquet, or directory)
        
        Returns:
            TrainingResult with training metrics and output paths
        """
        try:
            from transformers import TrainingArguments, Trainer, DataCollatorForLanguageModeling
            from trl import SFTTrainer
        except ImportError:
            raise ImportError("trl not installed. Install with: pip install trl")
        
        start_time = time.time()
        
        # Create output directory
        output_path = Path(self.config.output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        # Load model and tokenizer
        self._load_model()
        
        # Setup LoRA
        self._setup_lora()
        
        # Load dataset
        dataset = self._load_dataset(data_path)
        
        # Training arguments
        training_args = TrainingArguments(
            output_dir=str(output_path),
            num_train_epochs=self.config.num_epochs,
            per_device_train_batch_size=self.config.batch_size,
            per_device_eval_batch_size=self.config.batch_size,
            gradient_accumulation_steps=self.config.gradient_accumulation_steps,
            learning_rate=self.config.learning_rate,
            weight_decay=self.config.weight_decay,
            warmup_ratio=self.config.warmup_ratio,
            lr_scheduler_type=self.config.lr_scheduler_type,
            optim=self.config.optim,
            fp16=self.config.fp16,
            bf16=self.config.bf16,
            evaluation_strategy="steps",
            eval_steps=self.config.eval_steps,
            save_strategy="steps",
            save_steps=self.config.save_steps,
            logging_steps=self.config.logging_steps,
            load_best_model_at_end=True,
            report_to="none",
            save_total_limit=3,
        )
        
        # Create trainer
        trainer = SFTTrainer(
            model=self._model,
            train_dataset=dataset["train"],
            eval_dataset=dataset["test"],
            peft_config=self._peft_config,
            dataset_text_field="text",
            max_seq_length=self.config.dataset.max_length,
            tokenizer=self._tokenizer,
            args=training_args,
        )
        
        logger.info("Starting training")
        
        # Train
        train_result = trainer.train()
        
        # Save final model
        adapter_path = output_path / "adapter"
        self._model.save_pretrained(str(adapter_path))
        self._tokenizer.save_pretrained(str(adapter_path))
        
        # Get metrics
        metrics = train_result.metrics
        final_loss = metrics.get("train_loss", 0)
        
        # Evaluate
        eval_metrics = {}
        try:
            eval_result = trainer.evaluate()
            eval_metrics = eval_result
        except Exception as e:
            logger.warning("Evaluation failed", error=str(e))
        
        training_time = time.time() - start_time
        
        # Build result
        result = TrainingResult(
            success=True,
            final_loss=final_loss,
            best_loss=final_loss,
            training_time_seconds=training_time,
            output_path=str(output_path),
            adapter_path=str(adapter_path),
            history=trainer.state.log_history,
            eval_metrics=eval_metrics,
        )
        
        # Save results
        result.save(str(output_path / "training_result.json"))
        
        # Save config
        self.config.to_yaml(str(output_path / "training_config.yaml"))
        
        logger.info(
            "Training complete",
            final_loss=final_loss,
            time_seconds=training_time,
            adapter_path=str(adapter_path),
        )
        
        return result
    
    def merge_and_export(self, adapter_path: str, output_path: str) -> str:
        """
        Merge LoRA weights with base model and export.
        
        Args:
            adapter_path: Path to LoRA adapter
            output_path: Path to save merged model
        
        Returns:
            Path to merged model
        """
        try:
            from peft import PeftModel
            from transformers import AutoModelForCausalLM, AutoTokenizer
        except ImportError:
            raise ImportError("peft/transformers not installed")
        
        logger.info("Merging adapter with base model")
        
        # Load base model
        base_model = AutoModelForCausalLM.from_pretrained(
            self.config.base_model,
            device_map="cpu",
            trust_remote_code=True,
        )
        
        # Load adapter
        model = PeftModel.from_pretrained(base_model, adapter_path)
        
        # Merge
        merged_model = model.merge_and_unload()
        
        # Save
        output_path = Path(output_path)
        output_path.mkdir(parents=True, exist_ok=True)
        
        merged_model.save_pretrained(str(output_path))
        
        # Save tokenizer
        tokenizer = AutoTokenizer.from_pretrained(adapter_path)
        tokenizer.save_pretrained(str(output_path))
        
        logger.info("Merged model saved", path=str(output_path))
        
        return str(output_path)


class DatasetBuilder:
    """Build training datasets from various sources."""
    
    def __init__(self, output_path: str):
        """Initialize the builder."""
        self.output_path = Path(output_path)
        self.output_path.mkdir(parents=True, exist_ok=True)
        self.examples = []
    
    def add_instruction(
        self,
        instruction: str,
        output: str,
        input_text: str = "",
    ) -> None:
        """Add an instruction example."""
        self.examples.append({
            "instruction": instruction,
            "input": input_text,
            "output": output,
        })
    
    def add_code_completion(
        self,
        prefix: str,
        completion: str,
        language: str = "python",
    ) -> None:
        """Add a code completion example."""
        self.add_instruction(
            instruction=f"Complete the following {language} code:",
            input_text=prefix,
            output=completion,
        )
    
    def add_code_explanation(
        self,
        code: str,
        explanation: str,
        language: str = "python",
    ) -> None:
        """Add a code explanation example."""
        self.add_instruction(
            instruction=f"Explain what this {language} code does:",
            input_text=code,
            output=explanation,
        )
    
    def add_code_review(
        self,
        code: str,
        review: str,
        language: str = "python",
    ) -> None:
        """Add a code review example."""
        self.add_instruction(
            instruction=f"Review this {language} code and provide feedback:",
            input_text=code,
            output=review,
        )
    
    def add_debug(
        self,
        buggy_code: str,
        fixed_code: str,
        language: str = "python",
    ) -> None:
        """Add a debugging example."""
        self.add_instruction(
            instruction=f"Fix the bug in this {language} code:",
            input_text=buggy_code,
            output=fixed_code,
        )
    
    def from_github_issues(self, repo: str, token: Optional[str] = None) -> int:
        """
        Import examples from GitHub issues.
        
        Note: This is a placeholder. Full implementation would fetch closed 
        issues with code solutions from the repository.
        """
        raise NotImplementedError(
            "GitHub issues import not yet implemented. "
            "Use add_instruction() to manually add training examples."
        )
    
    def from_stack_overflow(self, tags: list[str], limit: int = 1000) -> int:
        """
        Import examples from Stack Overflow.
        
        Note: This is a placeholder. Full implementation would fetch Q&A 
        with code from Stack Overflow API.
        """
        raise NotImplementedError(
            "Stack Overflow import not yet implemented. "
            "Use add_instruction() to manually add training examples."
        )
    
    def save(self) -> str:
        """Save dataset to JSONL file."""
        output_file = self.output_path / "training_data.jsonl"
        
        with open(output_file, "w") as f:
            for example in self.examples:
                f.write(json.dumps(example) + "\n")
        
        logger.info(
            "Dataset saved",
            path=str(output_file),
            examples=len(self.examples),
        )
        
        return str(output_file)
    
    def get_stats(self) -> dict:
        """Get dataset statistics."""
        if not self.examples:
            return {"total": 0}
        
        instruction_lens = [len(e["instruction"]) for e in self.examples]
        output_lens = [len(e["output"]) for e in self.examples]
        
        return {
            "total": len(self.examples),
            "avg_instruction_len": sum(instruction_lens) / len(instruction_lens),
            "avg_output_len": sum(output_lens) / len(output_lens),
            "max_instruction_len": max(instruction_lens),
            "max_output_len": max(output_lens),
        }
