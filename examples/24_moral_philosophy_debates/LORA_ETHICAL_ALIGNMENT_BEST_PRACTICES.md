> **Hinweis:** Inhalt ist konzeptuell/referenziell. Code-Bezüge mit `<!-- TODO: verify against source -->` markiert.

# LoRa Ethical Alignment: Best Practices for ThemisDB

## Executive Summary

This document provides comprehensive best practices for using Low-Rank Adaptation (LoRa) fine-tuning to create ethically aligned AI models within the ThemisDB ecosystem. It combines cutting-edge research with practical implementation guidance for training models that make transparent, fair, and accountable ethical decisions.

## Table of Contents

1. [Introduction to LoRa for Ethics](#introduction-to-lora-for-ethics)
2. [Dataset Preparation](#dataset-preparation)
3. [Training Configuration](#training-configuration)
4. [Multi-Philosophy Adapter Strategy](#multi-philosophy-adapter-strategy)
5. [Graph-Augmented Training](#graph-augmented-training)
6. [Quality Assurance & Validation](#quality-assurance--validation)
7. [Deployment & Monitoring](#deployment--monitoring)
8. [Troubleshooting Guide](#troubleshooting-guide)

---

## 1. Introduction to LoRa for Ethics

### 1.1 Why LoRa for Ethical AI?

**Benefits**:
- **Parameter Efficiency**: Train only 0.1-1% of parameters
- **Preserve General Knowledge**: Base model capabilities remain intact
- **Modular Philosophy Adapters**: Separate adapters for each ethical framework
- **Fast Iteration**: Quick training and testing cycles
- **Minimal Infrastructure**: Can train on single GPU

**Key Research**:
- LoRa (Hu et al., 2021): Original LoRa paper
- QLoRA (Dettmers et al., 2023): 4-bit quantization for efficiency
- Constitutional AI (Anthropic, 2022): Principle-based alignment

### 1.2 Architecture Overview

```
┌─────────────────────────────────────────────┐
│         Base Language Model                 │
│         (Llama 3, Mistral, etc.)           │
└──────────────────┬──────────────────────────┘
                   │
      ┌────────────┴────────────┐
      │                         │
┌─────▼──────┐          ┌──────▼─────┐
│   LoRa     │          │   LoRa     │
│  Adapter   │   ...    │  Adapter   │
│  (Kant)    │          │ (Utilit.)  │
└────────────┘          └────────────┘
      │                         │
      └────────────┬────────────┘
                   │
      ┌────────────▼─────────────┐
      │   Ethical Decision       │
      │   Output                 │
      └──────────────────────────┘
```

### 1.3 Key Concepts

**Rank (r)**: Dimensionality of low-rank matrices
- Higher rank = more capacity but more parameters
- For ethics: Recommended r=16 to r=32

**Alpha (α)**: Scaling factor
- α = 2r is a good default
- Controls magnitude of adapter contribution

**Target Modules**: Which layers to adapt
- Attention layers: q_proj, k_proj, v_proj, o_proj
- FFN layers: gate_proj, up_proj, down_proj
- For ethics: Adapt all for maximum flexibility

---

## 2. Dataset Preparation

### 2.1 Data Sources

**Primary Sources**:
1. **ETHICS Dataset** (Hendrycks et al., 2021)
   - 100,000+ scenarios across 5 ethical dimensions
   - Justice, Deontology, Virtue, Utilitarianism, Commonsense

2. **Moral Machine Dataset** (MIT)
   - 40 million decisions from 233 countries
   - Trolley problem variants
   - Cross-cultural perspectives

3. **Social Chemistry** (Forbes et al., 2020)
   - Social norms and rules-of-thumb
   - Context-dependent judgments

4. **ThemisDB Debate History**
   - High-quality debates from your own system
   - Multi-philosophy perspectives
   - Real-world scenarios

**Supplementary Sources**:
- Stanford Encyclopedia of Philosophy extracts
- Philosophy textbook Q&A pairs
- Case studies from medical/legal ethics
- Synthetic data from Constitutional AI approach

### 2.2 Data Format

**Training Example Structure**:
```json
{
  "id": "ethics_train_001",
  "scenario": {
    "description": "A doctor has five patients who will die without organ transplants. A healthy patient comes in for a routine checkup. Should the doctor harvest the healthy patient's organs to save the five?",
    "domain": "medical_ethics",
    "stakeholders": ["healthy_patient", "five_patients"],
    "urgency": "high",
    "complexity": "high"
  },
  "philosophy": "kant",
  "reasoning_chain": [
    "According to Kantian ethics, we must treat people as ends in themselves, never merely as means.",
    "Harvesting the healthy patient's organs treats them as a mere means to save others.",
    "This violates the Categorical Imperative.",
    "Even though five lives could be saved, the action is morally impermissible."
  ],
  "decision": {
    "action": "do_not_harvest",
    "reasoning": "The doctor must not harvest the healthy patient's organs. Kantian ethics prohibits using one person as a means to save others, regardless of the numbers involved. Each person has inherent dignity that must be respected.",
    "confidence": 0.95,
    "principles": [
      "categorical_imperative",
      "respect_for_persons",
      "human_dignity"
    ]
  },
  "alternative_philosophies": {
    "utilitarian": {
      "decision": "harvest",
      "reasoning": "Utilitarianism would support harvesting to maximize overall utility (5 lives vs 1).",
      "confidence": 0.85
    },
    "virtue": {
      "decision": "do_not_harvest",
      "reasoning": "A virtuous doctor displays integrity and respect for life, not instrumentalization.",
      "confidence": 0.80
    }
  },
  "metadata": {
    "difficulty": "advanced",
    "controversy_level": "high",
    "quality_score": 0.92,
    "human_verified": true,
    "source": "ETHICS_dataset"
  }
}
```

### 2.3 Dataset Balance

**Philosophy Balance**:
```python
def balance_dataset_by_philosophy(dataset):
    """Ensure equal representation of each philosophy"""
    philosophies = ['kant', 'utilitarian', 'virtue', 'care', 'discourse', 
                    'social_contract', 'deontology', 'consequentialist']
    
    balanced_data = []
    min_count = min(len(dataset[p]) for p in philosophies)
    
    for philosophy in philosophies:
        # Sample equal number from each
        sampled = random.sample(dataset[philosophy], min_count)
        balanced_data.extend(sampled)
    
    random.shuffle(balanced_data)
    return balanced_data
```

**Domain Balance**:
```python
def balance_dataset_by_domain(dataset):
    """Ensure representation across ethical domains"""
    domains = ['medical', 'legal', 'business', 'technology', 
               'environmental', 'social', 'political']
    
    # Ensure minimum representation per domain
    min_per_domain = 100
    
    balanced = []
    for domain in domains:
        domain_data = [d for d in dataset if d['scenario']['domain'] == domain]
        if len(domain_data) < min_per_domain:
            # Augment with synthetic data if needed
            domain_data = augment_domain_data(domain_data, min_per_domain)
        balanced.extend(domain_data)
    
    return balanced
```

**Difficulty Distribution**:
```python
def ensure_difficulty_distribution(dataset):
    """Ensure mix of easy, medium, hard scenarios"""
    # Target distribution: 30% easy, 40% medium, 30% hard
    target_dist = {'easy': 0.3, 'medium': 0.4, 'hard': 0.3}
    
    categorized = {
        'easy': [d for d in dataset if d['metadata']['difficulty'] == 'easy'],
        'medium': [d for d in dataset if d['metadata']['difficulty'] == 'medium'],
        'hard': [d for d in dataset if d['metadata']['difficulty'] == 'hard']
    }
    
    total_size = len(dataset)
    balanced = []
    
    for difficulty, ratio in target_dist.items():
        target_count = int(total_size * ratio)
        sampled = random.sample(categorized[difficulty], 
                               min(target_count, len(categorized[difficulty])))
        balanced.extend(sampled)
    
    return balanced
```

### 2.4 Data Quality Filtering

```python
class EthicsDataQualityFilter:
    def __init__(self):
        self.min_reasoning_length = 100  # characters
        self.max_reasoning_length = 2000
        self.min_quality_score = 0.7
    
    def filter_dataset(self, dataset):
        """Filter dataset for quality"""
        filtered = []
        
        for example in dataset:
            # Check quality score
            if example['metadata']['quality_score'] < self.min_quality_score:
                continue
            
            # Check reasoning length
            reasoning_len = len(example['decision']['reasoning'])
            if not (self.min_reasoning_length <= reasoning_len <= self.max_reasoning_length):
                continue
            
            # Check for required fields
            if not self.has_required_fields(example):
                continue
            
            # Check for ethical consistency
            if not self.is_ethically_consistent(example):
                continue
            
            filtered.append(example)
        
        return filtered
    
    def has_required_fields(self, example):
        """Verify all required fields are present"""
        required = [
            'scenario.description',
            'philosophy',
            'decision.action',
            'decision.reasoning',
            'decision.principles'
        ]
        
        for field in required:
            if not self.get_nested_field(example, field):
                return False
        
        return True
    
    def is_ethically_consistent(self, example):
        """Check if reasoning is consistent with stated philosophy"""
        philosophy = example['philosophy']
        reasoning = example['decision']['reasoning']
        principles = example['decision']['principles']
        
        # Verify philosophy-specific keywords present
        philosophy_keywords = self.get_philosophy_keywords(philosophy)
        
        reasoning_lower = reasoning.lower()
        keyword_count = sum(1 for kw in philosophy_keywords if kw in reasoning_lower)
        
        # Should have at least 2 philosophy-specific keywords
        if keyword_count < 2:
            return False
        
        # Verify principles match philosophy
        valid_principles = self.get_philosophy_principles(philosophy)
        for principle in principles:
            if principle not in valid_principles:
                return False
        
        return True
```

### 2.5 Data Augmentation

**Synthetic Data Generation**:
```python
class EthicsDataAugmenter:
    def __init__(self, llm_backend):
        self.llm = llm_backend
    
    def augment_scenario(self, base_scenario):
        """Create variations of existing scenarios"""
        augmented = []
        
        # Variation 1: Change stakeholders
        stakeholders_variant = self.vary_stakeholders(base_scenario)
        augmented.append(stakeholders_variant)
        
        # Variation 2: Change context
        context_variant = self.vary_context(base_scenario)
        augmented.append(context_variant)
        
        # Variation 3: Change urgency
        urgency_variant = self.vary_urgency(base_scenario)
        augmented.append(urgency_variant)
        
        # Variation 4: Add constraints
        constraint_variant = self.add_constraints(base_scenario)
        augmented.append(constraint_variant)
        
        return augmented
    
    def generate_counterfactual(self, scenario):
        """Generate counterfactual scenarios"""
        prompt = f"""
        Given this ethical scenario:
        {scenario['description']}
        
        Generate a counterfactual scenario that changes one key variable
        while maintaining the core ethical dilemma.
        """
        
        counterfactual = self.llm.generate(prompt)
        return counterfactual
    
    def cross_philosophy_augmentation(self, scenario, base_philosophy):
        """Generate reasoning from multiple philosophies"""
        philosophies = ['kant', 'utilitarian', 'virtue', 'care', 'discourse']
        
        augmented = []
        for philosophy in philosophies:
            if philosophy == base_philosophy:
                continue
            
            prompt = f"""
            Analyze this ethical scenario from the perspective of {philosophy} ethics:
            {scenario['description']}
            
            Provide:
            1. Reasoning chain
            2. Decision
            3. Key principles applied
            """
            
            response = self.llm.generate(prompt)
            augmented_example = self.parse_response(response, philosophy)
            augmented.append(augmented_example)
        
        return augmented
```

---

## 3. Training Configuration

### 3.1 Optimal LoRa Hyperparameters

**Configuration Template**:
```yaml
# lora_ethics_optimal_config.yaml

model:
  base_model: "meta-llama/Llama-3-8b"
  quantization:
    enabled: true
    bits: 4  # QLoRa for efficiency
    type: "nf4"  # NormalFloat4
    double_quantization: true
    
lora:
  # Core LoRa settings
  rank: 16  # r=16 for balanced capacity/efficiency
  alpha: 32  # α=2r
  dropout: 0.1  # Prevents overfitting
  
  # Target modules for full ethical reasoning capacity
  target_modules:
    - "q_proj"    # Query projection (attention)
    - "k_proj"    # Key projection (attention)
    - "v_proj"    # Value projection (attention)
    - "o_proj"    # Output projection (attention)
    - "gate_proj" # Gate projection (FFN)
    - "up_proj"   # Up projection (FFN)
    - "down_proj" # Down projection (FFN)
  
  # Ethics-specific settings
  bias: "none"  # Don't adapt biases
  task_type: "CAUSAL_LM"
  
training:
  # Training hyperparameters
  learning_rate: 2e-4  # Standard for LoRa
  batch_size: 4  # Adjust based on GPU memory
  gradient_accumulation_steps: 4  # Effective batch size = 16
  epochs: 3  # 3-5 epochs typical for LoRa
  
  # Learning rate schedule
  lr_scheduler: "cosine"
  warmup_steps: 100
  warmup_ratio: 0.03
  
  # Optimization
  optimizer: "adamw"
  weight_decay: 0.01
  max_grad_norm: 0.3  # Gradient clipping
  
  # Logging and checkpointing
  logging_steps: 10
  save_steps: 100
  eval_steps: 50
  save_total_limit: 3  # Keep only best 3 checkpoints
  
  # Ethics-specific training
  philosophy_balanced_sampling: true
  domain_balanced_sampling: true
  difficulty_curriculum: true  # Start easy, progress to hard
  
evaluation:
  # Validation strategy
  strategy: "steps"
  eval_steps: 50
  
  # Ethics-specific metrics
  metrics:
    - "perplexity"
    - "ethics_consistency"
    - "philosophy_alignment"
    - "fairness_score"
    - "transparency_score"
  
  # Early stopping
  early_stopping:
    enabled: true
    patience: 3
    metric: "ethics_consistency"
    mode: "max"

dataset:
  # Data configuration
  train_split: 0.8
  val_split: 0.1
  test_split: 0.1
  
  # Preprocessing
  max_length: 2048  # Token limit
  padding: "max_length"
  truncation: true
  
  # Ethics-specific preprocessing
  include_reasoning_chain: true
  include_alternative_philosophies: true
  include_principles: true

inference:
  # Generation parameters
  temperature: 0.7  # Balanced creativity/consistency
  top_p: 0.9
  top_k: 50
  repetition_penalty: 1.1
  max_new_tokens: 512
  
  # Ethics-specific inference
  force_principle_citation: true
  require_reasoning_chain: true
  confidence_threshold: 0.7
```

### 3.2 Philosophy-Specific Configurations

**Kant (Deontological)**:
```yaml
# lora_kant_config.yaml
inherit_from: "lora_ethics_optimal_config.yaml"

lora:
  rank: 20  # Slightly higher for complex rule systems
  
training:
  epochs: 4  # More epochs for rule learning
  focus_principles:
    - "categorical_imperative"
    - "respect_for_persons"
    - "universalizability"
  
dataset:
  filter_by_philosophy: "kant"
  augment_with_rules: true
  include_maxim_analysis: true
```

**Utilitarian (Consequentialist)**:
```yaml
# lora_utilitarian_config.yaml
inherit_from: "lora_ethics_optimal_config.yaml"

training:
  focus_principles:
    - "maximize_utility"
    - "greatest_good"
    - "impartial_consideration"
  
dataset:
  filter_by_philosophy: "utilitarian"
  include_outcome_analysis: true
  include_probability_estimates: true
  augment_with_calculations: true
```

**Virtue Ethics**:
```yaml
# lora_virtue_config.yaml
inherit_from: "lora_ethics_optimal_config.yaml"

lora:
  rank: 18  # Moderate complexity
  
training:
  focus_virtues:
    - "wisdom"
    - "courage"
    - "justice"
    - "temperance"
    - "compassion"
  
dataset:
  filter_by_philosophy: "virtue"
  include_character_analysis: true
  augment_with_virtue_examples: true
```

### 3.3 Training Loop Implementation

```python
class EthicsLoRaTrainer:
    def __init__(self, config_path: str):
        self.config = self.load_config(config_path)
        self.model = self.setup_model()
        self.tokenizer = self.setup_tokenizer()
        self.evaluator = EthicsEvaluator()
    
    def setup_model(self):
        """Setup base model with LoRa"""
        from peft import LoraConfig, get_peft_model
        from transformers import AutoModelForCausalLM
        
        # Load base model with quantization
        model = AutoModelForCausalLM.from_pretrained(
            self.config['model']['base_model'],
            load_in_4bit=True,
            quantization_config=BitsAndBytesConfig(
                load_in_4bit=True,
                bnb_4bit_quant_type="nf4",
                bnb_4bit_use_double_quant=True,
                bnb_4bit_compute_dtype=torch.float16
            ),
            device_map="auto"
        )
        
        # Configure LoRa
        lora_config = LoraConfig(
            r=self.config['lora']['rank'],
            lora_alpha=self.config['lora']['alpha'],
            target_modules=self.config['lora']['target_modules'],
            lora_dropout=self.config['lora']['dropout'],
            bias="none",
            task_type="CAUSAL_LM"
        )
        
        # Apply LoRa
        model = get_peft_model(model, lora_config)
        model.print_trainable_parameters()
        
        return model
    
    def train(self, train_dataset, val_dataset):
        """Main training loop with ethics validation"""
        from transformers import Trainer, TrainingArguments
        
        # Training arguments
        training_args = TrainingArguments(
            output_dir="./ethics_lora_output",
            learning_rate=self.config['training']['learning_rate'],
            per_device_train_batch_size=self.config['training']['batch_size'],
            gradient_accumulation_steps=self.config['training']['gradient_accumulation_steps'],
            num_train_epochs=self.config['training']['epochs'],
            lr_scheduler_type=self.config['training']['lr_scheduler'],
            warmup_steps=self.config['training']['warmup_steps'],
            logging_steps=self.config['training']['logging_steps'],
            save_steps=self.config['training']['save_steps'],
            eval_steps=self.config['training']['eval_steps'],
            evaluation_strategy="steps",
            save_total_limit=self.config['training']['save_total_limit'],
            load_best_model_at_end=True,
            metric_for_best_model="ethics_score",
            greater_is_better=True,
            report_to=["tensorboard", "wandb"]
        )
        
        # Custom trainer with ethics evaluation
        trainer = EthicsAwareTrainer(
            model=self.model,
            args=training_args,
            train_dataset=train_dataset,
            eval_dataset=val_dataset,
            tokenizer=self.tokenizer,
            compute_metrics=self.compute_ethics_metrics,
            callbacks=[
                EthicsValidationCallback(self.evaluator),
                BiasDetectionCallback(),
                EarlyStoppingCallback(patience=3)
            ]
        )
        
        # Train
        train_result = trainer.train()
        
        # Save final adapter
        self.model.save_pretrained("./ethics_lora_final")
        
        return train_result
    
    def compute_ethics_metrics(self, eval_pred):
        """Compute ethics-specific metrics during training"""
        predictions, labels = eval_pred
        
        # Decode predictions
        decoded_preds = self.tokenizer.batch_decode(
            predictions,
            skip_special_tokens=True
        )
        decoded_labels = self.tokenizer.batch_decode(
            labels,
            skip_special_tokens=True
        )
        
        # Evaluate ethics metrics
        ethics_scores = []
        for pred, label in zip(decoded_preds, decoded_labels):
            score = self.evaluator.evaluate_decision(
                decision=pred,
                reference=label
            )
            ethics_scores.append(score.overall_score)
        
        return {
            'ethics_score': np.mean(ethics_scores),
            'ethics_std': np.std(ethics_scores),
            'perplexity': self.calculate_perplexity(eval_pred)
        }
```

### 3.4 Curriculum Learning for Ethics

```python
class EthicsCurriculumLearning:
    def __init__(self, dataset):
        self.dataset = dataset
        self.difficulty_levels = ['easy', 'medium', 'hard', 'expert']
    
    def create_curriculum(self):
        """Create difficulty-based curriculum"""
        curriculum = {
            'stage_1': self.get_by_difficulty('easy', ratio=1.0),
            'stage_2': self.get_by_difficulty(['easy', 'medium'], ratio=0.7),
            'stage_3': self.get_by_difficulty(['medium', 'hard'], ratio=0.5),
            'stage_4': self.get_by_difficulty('all', ratio=0.3)
        }
        
        return curriculum
    
    def train_with_curriculum(self, model, curriculum):
        """Train model through curriculum stages"""
        for stage, data in curriculum.items():
            print(f"Training {stage} with {len(data)} examples")
            
            # Train on this stage
            trainer = EthicsLoRaTrainer(model)
            trainer.train(data)
            
            # Validate before proceeding
            validation_score = trainer.validate()
            
            # Must achieve minimum score to proceed
            if validation_score < 0.75:
                print(f"Warning: Low score at {stage}, repeating")
                trainer.train(data)  # Train again
```

---

## 4. Multi-Philosophy Adapter Strategy

### 4.1 Architecture

**Concept**: Train separate LoRa adapters for each philosophical framework

```
Base Model (Llama-3-8b)
    ├── Kant Adapter (r=20)
    ├── Utilitarian Adapter (r=16)
    ├── Virtue Adapter (r=18)
    ├── Care Adapter (r=16)
    ├── Discourse Adapter (r=18)
    └── Contract Adapter (r=16)
```

**Benefits**:
1. **Specialized Reasoning**: Each adapter optimized for its philosophy
2. **Modularity**: Easy to add/remove/update philosophies
3. **Ensemble Decisions**: Combine multiple perspectives
4. **Interpretability**: Clear attribution to philosophy

### 4.2 Implementation

```python
class MultiPhilosophyAdapterSystem:
    def __init__(self, base_model_name: str):
        self.base_model_name = base_model_name
        self.base_model = None
        self.adapters = {}
        self.adapter_configs = self.load_adapter_configs()
    
    def train_all_adapters(self, dataset):
        """Train adapter for each philosophy"""
        philosophies = ['kant', 'utilitarian', 'virtue', 'care', 
                       'discourse', 'social_contract']
        
        for philosophy in philosophies:
            print(f"Training {philosophy} adapter...")
            
            # Filter dataset for this philosophy
            philosophy_data = self.filter_dataset(dataset, philosophy)
            
            # Load configuration for this philosophy
            config = self.adapter_configs[philosophy]
            
            # Train adapter
            trainer = EthicsLoRaTrainer(config)
            adapter = trainer.train(philosophy_data)
            
            # Store adapter
            self.adapters[philosophy] = adapter
            self.save_adapter(adapter, philosophy)
            
            # Evaluate adapter
            eval_results = self.evaluate_adapter(adapter, philosophy)
            print(f"{philosophy} adapter evaluation: {eval_results}")
    
    def reason_with_philosophy(self, scenario: str, philosophy: str):
        """Reason using specific philosophy adapter"""
        # Load base model if not already loaded
        if self.base_model is None:
            self.base_model = self.load_base_model()
        
        # Load adapter
        adapter = self.load_adapter(philosophy)
        model = self.apply_adapter(self.base_model, adapter)
        
        # Generate reasoning
        prompt = self.create_philosophy_prompt(scenario, philosophy)
        reasoning = model.generate(prompt)
        
        return reasoning
    
    def ensemble_reasoning(self, scenario: str, philosophies: List[str] = None):
        """Combine reasoning from multiple philosophies"""
        if philosophies is None:
            philosophies = list(self.adapters.keys())
        
        # Get reasoning from each philosophy
        reasonings = {}
        for philosophy in philosophies:
            reasonings[philosophy] = self.reason_with_philosophy(
                scenario,
                philosophy
            )
        
        # Synthesize final decision
        synthesis = self.synthesize_multi_philosophy(reasonings)
        
        return {
            'individual_reasonings': reasonings,
            'synthesis': synthesis,
            'philosophies_used': philosophies
        }
    
    def synthesize_multi_philosophy(self, reasonings: Dict[str, str]):
        """Synthesize decision from multiple philosophical perspectives"""
        # Use base model to synthesize
        prompt = f"""
        Multiple philosophical perspectives on an ethical scenario:
        
        {self.format_reasonings(reasonings)}
        
        Synthesize these perspectives into a balanced decision that:
        1. Acknowledges each philosophy's insights
        2. Identifies common ground and conflicts
        3. Provides a coherent recommendation
        4. Explains trade-offs made
        """
        
        synthesis = self.base_model.generate(prompt)
        return synthesis
```

### 4.3 Adapter Merging Strategies

```python
class AdapterMerging:
    @staticmethod
    def average_merge(adapters: List):
        """Simple averaging of adapter weights"""
        merged = {}
        
        for key in adapters[0].keys():
            weights = [adapter[key] for adapter in adapters]
            merged[key] = torch.mean(torch.stack(weights), dim=0)
        
        return merged
    
    @staticmethod
    def weighted_merge(adapters: List, weights: List[float]):
        """Weighted merging based on philosophy importance"""
        assert len(adapters) == len(weights)
        assert sum(weights) == 1.0
        
        merged = {}
        
        for key in adapters[0].keys():
            adapter_weights = [adapter[key] for adapter in adapters]
            merged[key] = sum(
                w * aw for w, aw in zip(weights, adapter_weights)
            )
        
        return merged
    
    @staticmethod
    def task_arithmetic_merge(base_adapter, specialized_adapters, scales):
        """Task arithmetic for adapter combination"""
        # Δ = α₁(θ₁ - θ₀) + α₂(θ₂ - θ₀) + ...
        merged = base_adapter.copy()
        
        for adapter, scale in zip(specialized_adapters, scales):
            for key in merged.keys():
                delta = adapter[key] - base_adapter[key]
                merged[key] += scale * delta
        
        return merged
```

### 4.4 Dynamic Adapter Selection

```python
class DynamicAdapterSelector:
    def __init__(self, adapters: Dict):
        self.adapters = adapters
        self.selector_model = self.train_selector()
    
    def select_best_adapter(self, scenario: str):
        """Select most appropriate philosophy for scenario"""
        # Extract scenario features
        features = self.extract_scenario_features(scenario)
        
        # Predict best philosophy
        philosophy_scores = self.selector_model.predict(features)
        best_philosophy = max(philosophy_scores, key=philosophy_scores.get)
        
        return best_philosophy, philosophy_scores
    
    def extract_scenario_features(self, scenario: str):
        """Extract features for adapter selection"""
        return {
            'domain': self.detect_domain(scenario),
            'stakeholder_count': self.count_stakeholders(scenario),
            'urgency': self.assess_urgency(scenario),
            'complexity': self.assess_complexity(scenario),
            'values_involved': self.identify_values(scenario),
            'keywords': self.extract_keywords(scenario)
        }
    
    def train_selector(self):
        """Train model to select appropriate philosophy"""
        # Use historical data to train classifier
        # Features: scenario characteristics
        # Label: which philosophy performed best
        pass
```

---

## 5. Graph-Augmented Training

### 5.1 Concept

Augment LoRa training with graph-based representations of ethical reasoning to improve structured thinking.

```
Scenario → [Graph Builder] → Ethical Decision Graph
                                    ↓
                          [Graph Encoder (GNN)]
                                    ↓
                            Graph Embeddings
                                    ↓
                    [Concat with Text Embeddings]
                                    ↓
                              [LoRa Model]
                                    ↓
                            Ethical Decision
```

### 5.2 Implementation

```python
class GraphAugmentedLoRaTraining:
    def __init__(self, base_model, graph_encoder):
        self.base_model = base_model
        self.graph_encoder = graph_encoder
        self.lora_config = self.create_lora_config()
    
    def build_ethical_graph(self, scenario):
        """Build graph representation of ethical scenario"""
        graph = EthicalDecisionGraph()
        
        # Add scenario node
        scenario_node = graph.add_node('scenario', {
            'description': scenario['description'],
            'domain': scenario['domain']
        })
        
        # Add stakeholder nodes
        for stakeholder in scenario['stakeholders']:
            stake_node = graph.add_node('stakeholder', stakeholder)
            graph.add_edge(scenario_node, stake_node, 'involves')
        
        # Add principle nodes
        for principle in scenario['relevant_principles']:
            prin_node = graph.add_node('principle', principle)
            graph.add_edge(prin_node, scenario_node, 'applies_to')
        
        # Add action nodes
        for action in scenario['possible_actions']:
            action_node = graph.add_node('action', action)
            graph.add_edge(scenario_node, action_node, 'considers')
            
            # Add outcome nodes
            for outcome in action['predicted_outcomes']:
                outcome_node = graph.add_node('outcome', outcome)
                graph.add_edge(action_node, outcome_node, 'leads_to')
        
        return graph
    
    def encode_graph(self, graph):
        """Encode graph using GNN"""
        # Convert to PyTorch Geometric format
        data = self.graph_to_pyg(graph)
        
        # Encode with GNN
        graph_embedding = self.graph_encoder(data)
        
        return graph_embedding
    
    def prepare_training_example(self, scenario, decision):
        """Prepare example with graph augmentation"""
        # Build graph
        graph = self.build_ethical_graph(scenario)
        
        # Encode graph
        graph_emb = self.encode_graph(graph)
        
        # Tokenize text
        text_input = self.tokenizer(
            scenario['description'],
            return_tensors="pt"
        )
        
        # Augment with graph embedding
        augmented_input = {
            'input_ids': text_input['input_ids'],
            'attention_mask': text_input['attention_mask'],
            'graph_embedding': graph_emb,
            'labels': self.tokenizer(decision['reasoning'])['input_ids']
        }
        
        return augmented_input
    
    def train_with_graphs(self, dataset):
        """Train LoRa with graph-augmented data"""
        augmented_dataset = []
        
        for example in dataset:
            augmented = self.prepare_training_example(
                example['scenario'],
                example['decision']
            )
            augmented_dataset.append(augmented)
        
        # Train with custom collator that handles graph embeddings
        trainer = EthicsLoRaTrainer(
            model=self.base_model,
            data_collator=GraphAugmentedCollator(),
            train_dataset=augmented_dataset
        )
        
        adapter = trainer.train()
        return adapter
```

### 5.3 Graph Neural Network for Ethics

```python
import torch
import torch.nn as nn
from torch_geometric.nn import GCNConv, global_mean_pool

class EthicalReasoningGNN(nn.Module):
    def __init__(self, node_feature_dim, hidden_dim, output_dim):
        super().__init__()
        
        # Graph convolutional layers
        self.conv1 = GCNConv(node_feature_dim, hidden_dim)
        self.conv2 = GCNConv(hidden_dim, hidden_dim)
        self.conv3 = GCNConv(hidden_dim, output_dim)
        
        # Node type embeddings
        self.node_type_embedding = nn.Embedding(10, node_feature_dim)
        
        # Edge type embeddings
        self.edge_type_embedding = nn.Embedding(15, hidden_dim)
        
        # Attention for principle nodes
        self.principle_attention = nn.MultiheadAttention(
            embed_dim=output_dim,
            num_heads=4
        )
    
    def forward(self, data):
        x, edge_index, batch = data.x, data.edge_index, data.batch
        edge_type = data.edge_attr
        
        # Encode node types
        x = self.node_type_embedding(x)
        
        # Graph convolutions
        x = self.conv1(x, edge_index)
        x = torch.relu(x)
        x = self.conv2(x, edge_index)
        x = torch.relu(x)
        x = self.conv3(x, edge_index)
        
        # Apply attention to principle nodes
        principle_mask = (data.node_type == 'principle')
        if principle_mask.any():
            principle_features = x[principle_mask]
            attended, _ = self.principle_attention(
                principle_features,
                principle_features,
                principle_features
            )
            x[principle_mask] = attended
        
        # Global pooling
        graph_embedding = global_mean_pool(x, batch)
        
        return graph_embedding
```

---

## 6. Quality Assurance & Validation

### 6.1 Continuous Evaluation During Training

```python
class EthicsValidationCallback:
    def __init__(self, evaluator, validation_frequency=50):
        self.evaluator = evaluator
        self.validation_frequency = validation_frequency
        self.validation_scenarios = self.load_validation_scenarios()
    
    def on_step_end(self, args, state, control, **kwargs):
        """Validate ethics after every N steps"""
        if state.global_step % self.validation_frequency != 0:
            return
        
        model = kwargs['model']
        
        # Run ethics validation
        results = self.run_ethics_validation(model)
        
        # Log results
        self.log_validation_results(results, state.global_step)
        
        # Check for ethics degradation
        if results['overall_ethics_score'] < 0.7:
            print("⚠️  WARNING: Ethics score degraded!")
            # Optionally stop training
            control.should_training_stop = True
        
        return control
    
    def run_ethics_validation(self, model):
        """Run comprehensive ethics validation"""
        results = {
            'consistency': [],
            'fairness': [],
            'transparency': [],
            'alignment': []
        }
        
        for scenario in self.validation_scenarios:
            # Generate decision
            decision = model.generate(scenario['prompt'])
            
            # Evaluate
            eval_result = self.evaluator.evaluate_decision(
                decision=decision,
                context=scenario
            )
            
            results['consistency'].append(eval_result.consistency)
            results['fairness'].append(eval_result.fairness)
            results['transparency'].append(eval_result.transparency)
            results['alignment'].append(eval_result.alignment)
        
        # Aggregate
        aggregated = {
            metric: np.mean(values)
            for metric, values in results.items()
        }
        
        aggregated['overall_ethics_score'] = np.mean(list(aggregated.values()))
        
        return aggregated
```

### 6.2 Bias Detection During Training

```python
class BiasDetectionCallback:
    def __init__(self, protected_attributes):
        self.protected_attributes = protected_attributes
        self.bias_detector = BiasDetector()
    
    def on_epoch_end(self, args, state, control, **kwargs):
        """Detect bias at end of each epoch"""
        model = kwargs['model']
        
        # Test for bias
        bias_results = self.test_for_bias(model)
        
        # Log bias metrics
        self.log_bias_metrics(bias_results, state.epoch)
        
        # Alert if significant bias detected
        if bias_results['max_bias'] > 0.2:
            print(f"⚠️  BIAS ALERT: {bias_results['biased_attributes']}")
            
            # Optionally: Apply debiasing techniques
            self.apply_debiasing(model, bias_results)
        
        return control
    
    def test_for_bias(self, model):
        """Test model for demographic biases"""
        bias_scores = {}
        
        for attribute in self.protected_attributes:
            # Generate scenarios with different attribute values
            scenarios = self.generate_attribute_variants(attribute)
            
            # Get decisions
            decisions = [model.generate(s) for s in scenarios]
            
            # Measure disparity
            bias_score = self.measure_decision_disparity(decisions)
            bias_scores[attribute] = bias_score
        
        return {
            'bias_scores': bias_scores,
            'max_bias': max(bias_scores.values()),
            'biased_attributes': [
                attr for attr, score in bias_scores.items()
                if score > 0.1
            ]
        }
```

### 6.3 Red Team Testing

```python
class EthicsRedTeamTester:
    def __init__(self, model):
        self.model = model
        self.adversarial_scenarios = self.load_adversarial_scenarios()
    
    def run_red_team_test(self):
        """Test model with adversarial scenarios"""
        results = {
            'failures': [],
            'edge_cases': [],
            'inconsistencies': [],
            'bias_exploitation': []
        }
        
        for scenario in self.adversarial_scenarios:
            decision = self.model.generate(scenario['prompt'])
            
            # Check for failures
            if self.is_ethical_failure(decision, scenario):
                results['failures'].append({
                    'scenario': scenario,
                    'decision': decision,
                    'reason': 'ethical_violation'
                })
            
            # Check for inconsistency
            if self.check_inconsistency(decision, scenario):
                results['inconsistencies'].append({
                    'scenario': scenario,
                    'decision': decision
                })
            
            # Check for bias exploitation
            if self.exploits_bias(decision, scenario):
                results['bias_exploitation'].append({
                    'scenario': scenario,
                    'decision': decision,
                    'bias_type': scenario['bias_type']
                })
        
        return results
    
    def generate_adversarial_scenarios(self):
        """Generate adversarial test cases"""
        adversarial = []
        
        # Type 1: Conflicting principles
        adversarial.extend(self.generate_principle_conflicts())
        
        # Type 2: Edge cases
        adversarial.extend(self.generate_edge_cases())
        
        # Type 3: Bias traps
        adversarial.extend(self.generate_bias_traps())
        
        # Type 4: Ambiguous scenarios
        adversarial.extend(self.generate_ambiguous_scenarios())
        
        return adversarial
```

---

## 7. Deployment & Monitoring

### 7.1 Production Deployment

```python
class EthicsLoRaDeployment:
    def __init__(self, adapter_registry):
        self.adapter_registry = adapter_registry
        self.base_model = None
        self.current_adapter = None
        self.monitoring = EthicsMonitoring()
    
    def deploy_adapter(self, philosophy: str, version: str):
        """Deploy specific philosophy adapter to production"""
        # Load adapter from registry
        adapter_info = self.adapter_registry.get_adapter(philosophy, version)
        
        # Validate adapter
        validation_result = self.validate_adapter(adapter_info)
        if not validation_result['passed']:
            raise ValueError(f"Adapter validation failed: {validation_result}")
        
        # Load base model if needed
        if self.base_model is None:
            self.base_model = self.load_base_model()
        
        # Apply adapter
        self.current_adapter = self.apply_adapter(
            self.base_model,
            adapter_info['weights']
        )
        
        # Initialize monitoring
        self.monitoring.start_monitoring(philosophy, version)
        
        print(f"✓ Deployed {philosophy} adapter version {version}")
    
    def validate_adapter(self, adapter_info):
        """Validate adapter before deployment"""
        checks = {
            'version_compatibility': self.check_version_compatibility(adapter_info),
            'ethics_benchmarks': self.run_ethics_benchmarks(adapter_info),
            'bias_tests': self.run_bias_tests(adapter_info),
            'performance_tests': self.run_performance_tests(adapter_info)
        }
        
        passed = all(check['passed'] for check in checks.values())
        
        return {
            'passed': passed,
            'checks': checks
        }
    
    def inference_with_monitoring(self, scenario: str):
        """Run inference with production monitoring"""
        # Generate decision
        start_time = time.time()
        decision = self.current_adapter.generate(scenario)
        latency = time.time() - start_time
        
        # Evaluate decision
        ethics_score = self.evaluate_decision(decision)
        
        # Log metrics
        self.monitoring.log_inference(
            scenario=scenario,
            decision=decision,
            ethics_score=ethics_score,
            latency=latency
        )
        
        # Check for anomalies
        if ethics_score < 0.7:
            self.monitoring.alert_low_ethics_score(scenario, decision, ethics_score)
        
        return decision
```

### 7.2 A/B Testing Ethics Adapters

```python
class EthicsAdapterABTest:
    def __init__(self, adapter_a, adapter_b, traffic_split=0.5):
        self.adapter_a = adapter_a
        self.adapter_b = adapter_b
        self.traffic_split = traffic_split
        self.results = {'a': [], 'b': []}
    
    def route_request(self, scenario: str):
        """Route request to A or B based on split"""
        if random.random() < self.traffic_split:
            variant = 'a'
            adapter = self.adapter_a
        else:
            variant = 'b'
            adapter = self.adapter_b
        
        # Generate decision
        decision = adapter.generate(scenario)
        
        # Evaluate
        ethics_score = self.evaluate_decision(decision, scenario)
        
        # Record result
        self.results[variant].append({
            'scenario': scenario,
            'decision': decision,
            'ethics_score': ethics_score,
            'timestamp': datetime.now()
        })
        
        return decision, variant
    
    def analyze_results(self):
        """Analyze A/B test results"""
        a_scores = [r['ethics_score'] for r in self.results['a']]
        b_scores = [r['ethics_score'] for r in self.results['b']]
        
        # Statistical comparison
        from scipy import stats
        t_stat, p_value = stats.ttest_ind(a_scores, b_scores)
        
        analysis = {
            'adapter_a': {
                'mean_score': np.mean(a_scores),
                'std_score': np.std(a_scores),
                'count': len(a_scores)
            },
            'adapter_b': {
                'mean_score': np.mean(b_scores),
                'std_score': np.std(b_scores),
                'count': len(b_scores)
            },
            'statistical_test': {
                't_statistic': t_stat,
                'p_value': p_value,
                'significant': p_value < 0.05
            }
        }
        
        # Determine winner
        if analysis['statistical_test']['significant']:
            if analysis['adapter_a']['mean_score'] > analysis['adapter_b']['mean_score']:
                analysis['winner'] = 'a'
            else:
                analysis['winner'] = 'b'
        else:
            analysis['winner'] = 'inconclusive'
        
        return analysis
```

### 7.3 Production Monitoring Dashboard

```python
class EthicsProductionMonitoring:
    def __init__(self, themis_client):
        self.themis_client = themis_client
        self.metrics_buffer = []
    
    def log_inference(self, scenario, decision, ethics_score, latency):
        """Log production inference"""
        metric = {
            'timestamp': datetime.now(),
            'scenario_hash': hashlib.sha256(scenario.encode()).hexdigest(),
            'decision_hash': hashlib.sha256(decision.encode()).hexdigest(),
            'ethics_score': ethics_score,
            'latency_ms': latency * 1000,
            'adapter_version': self.get_current_adapter_version()
        }
        
        self.metrics_buffer.append(metric)
        
        # Flush to ThemisDB every 100 metrics
        if len(self.metrics_buffer) >= 100:
            self.flush_metrics()
    
    def flush_metrics(self):
        """Flush metrics to ThemisDB"""
        # Store in timeline for time-series analysis
        self.themis_client.timeline.batch_insert(self.metrics_buffer)
        
        # Store aggregates in relational
        hourly_agg = self.aggregate_hourly(self.metrics_buffer)
        self.themis_client.relational.insert(
            'ethics_metrics_hourly',
            hourly_agg
        )
        
        self.metrics_buffer = []
    
    def get_real_time_dashboard(self):
        """Get real-time dashboard data"""
        # Query last hour of metrics
        recent_metrics = self.themis_client.timeline.query(
            start_time=datetime.now() - timedelta(hours=1),
            metric_type='ethics_inference'
        )
        
        dashboard = {
            'current_qps': self.calculate_qps(recent_metrics),
            'avg_ethics_score': np.mean([m['ethics_score'] for m in recent_metrics]),
            'p50_latency': np.percentile([m['latency_ms'] for m in recent_metrics], 50),
            'p95_latency': np.percentile([m['latency_ms'] for m in recent_metrics], 95),
            'p99_latency': np.percentile([m['latency_ms'] for m in recent_metrics], 99),
            'low_score_count': sum(1 for m in recent_metrics if m['ethics_score'] < 0.7),
            'error_rate': self.calculate_error_rate(recent_metrics)
        }
        
        return dashboard
    
    def detect_anomalies(self):
        """Detect anomalies in production metrics"""
        # Query recent metrics
        metrics = self.themis_client.timeline.query(
            start_time=datetime.now() - timedelta(hours=24)
        )
        
        anomalies = []
        
        # Check for ethics score degradation
        if self.detect_score_degradation(metrics):
            anomalies.append({
                'type': 'ethics_degradation',
                'severity': 'high',
                'description': 'Ethics scores declining over time'
            })
        
        # Check for bias drift
        if self.detect_bias_drift(metrics):
            anomalies.append({
                'type': 'bias_drift',
                'severity': 'high',
                'description': 'Detected drift in fairness metrics'
            })
        
        # Check for latency spikes
        if self.detect_latency_spike(metrics):
            anomalies.append({
                'type': 'latency_spike',
                'severity': 'medium',
                'description': 'Latency increased significantly'
            })
        
        return anomalies
```

---

## 8. Troubleshooting Guide

### 8.1 Common Issues

**Issue 1: Low Ethics Scores During Training**

**Symptoms**:
- Ethics evaluation scores < 0.7
- Model produces inconsistent reasoning
- Fairness metrics poor

**Diagnosis**:
```python
def diagnose_low_ethics_scores(model, validation_data):
    results = {
        'consistency_issues': [],
        'fairness_issues': [],
        'reasoning_quality_issues': []
    }
    
    for example in validation_data:
        decision = model.generate(example['scenario'])
        
        # Check consistency
        if not is_consistent(decision, example['philosophy']):
            results['consistency_issues'].append(example)
        
        # Check fairness
        if has_bias(decision):
            results['fairness_issues'].append(example)
        
        # Check reasoning quality
        if not has_quality_reasoning(decision):
            results['reasoning_quality_issues'].append(example)
    
    return results
```

**Solutions**:
1. **Increase LoRa rank**: Try r=24 or r=32 for more capacity
2. **Extend training**: Add 1-2 more epochs
3. **Improve dataset quality**: Filter low-quality examples
4. **Add curriculum learning**: Start with easier examples
5. **Adjust learning rate**: Try 1e-4 instead of 2e-4

**Issue 2: Overfitting to Specific Philosophy**

**Symptoms**:
- High training scores, low validation scores
- Model too rigid in applying philosophy
- Poor generalization to edge cases

**Solutions**:
1. **Increase dropout**: Try 0.15 or 0.2
2. **Data augmentation**: Add more diverse scenarios
3. **Regularization**: Add weight decay
4. **Cross-philosophy training**: Include alternative perspectives

**Issue 3: Bias in Ethical Decisions**

**Symptoms**:
- Demographic parity < 0.8
- Different outcomes for protected groups
- Inconsistent treatment of similar cases

**Diagnosis and Solutions**:
```python
def mitigate_bias_in_adapter(model, adapter, biased_attribute):
    # Step 1: Identify biased examples
    biased_examples = detect_biased_decisions(model, biased_attribute)
    
    # Step 2: Create debiasing dataset
    debiasing_data = create_counterfactual_dataset(biased_examples)
    
    # Step 3: Fine-tune with fairness constraints
    fairness_trainer = FairnessAwareTrainer(
        model=model,
        adapter=adapter,
        fairness_constraints=[
            DemographicParityConstraint(threshold=0.9),
            EqualizedOddsConstraint(threshold=0.9)
        ]
    )
    
    debiased_adapter = fairness_trainer.train(debiasing_data)
    
    # Step 4: Validate debiasing
    validation_result = validate_fairness(debiased_adapter)
    
    return debiased_adapter, validation_result
```

**Issue 4: Slow Inference**

**Symptoms**:
- Inference latency > 2 seconds
- High GPU memory usage
- Can't scale to production load

**Solutions**:
1. **Use QLoRa**: 4-bit quantization reduces memory and improves speed
2. **Reduce rank**: Lower r reduces computation
3. **Optimize generation**: Reduce max_new_tokens
4. **Batch inference**: Process multiple requests together
5. **Use vLLM**: Fast inference engine

### 8.2 Performance Optimization

```python
class EthicsLoRaOptimizer:
    @staticmethod
    def optimize_for_inference(adapter, base_model):
        """Optimize adapter for fast inference"""
        # Merge adapter into base model
        merged_model = merge_adapter_weights(base_model, adapter)
        
        # Quantize to int8
        quantized_model = quantize_model(merged_model, bits=8)
        
        # Compile with torch.compile (PyTorch 2.0+)
        optimized_model = torch.compile(quantized_model, mode="reduce-overhead")
        
        return optimized_model
    
    @staticmethod
    def enable_flash_attention(model):
        """Enable Flash Attention 2 for speed"""
        model.config.use_flash_attention_2 = True
        return model
    
    @staticmethod
    def setup_vllm_serving(adapter_path):
        """Setup vLLM for production serving"""
        from vllm import LLM, SamplingParams
        
        llm = LLM(
            model=adapter_path,
            tensor_parallel_size=1,
            dtype="float16",
            max_model_len=2048
        )
        
        sampling_params = SamplingParams(
            temperature=0.7,
            top_p=0.9,
            max_tokens=512
        )
        
        return llm, sampling_params
```

---

## Conclusion

This comprehensive guide provides best practices for using LoRa fine-tuning to create ethically aligned AI models in ThemisDB. Key takeaways:

1. **Dataset Quality is Critical**: Balance philosophies, domains, and difficulties
2. **Multi-Adapter Strategy**: Train separate adapters per philosophy
3. **Continuous Evaluation**: Monitor ethics metrics throughout training
4. **Graph Augmentation**: Enhance reasoning with structured representations
5. **Production Monitoring**: Track ethics scores in production
6. **Iterative Improvement**: Use feedback to continuously refine adapters

By following these practices, you can create ethical AI systems that are:
- **Transparent**: Clear reasoning chains
- **Fair**: Unbiased across demographics
- **Accountable**: Traceable to principles
- **Consistent**: Reliable decisions
- **Adaptable**: Modular philosophy components

**Next Steps**:
1. Prepare your ethics training dataset
2. Train your first philosophy adapter
3. Validate with comprehensive benchmarks
4. Deploy with monitoring
5. Iterate based on production feedback

**Resources**:
- Example configs: `lora_ethics_optimal_config.yaml`
- Training scripts: `themis_ethics_lora_training.py`
- Evaluation: `ethics_evaluation_metrics.py`
- Monitoring: `ethics_monitoring_dashboard.py`

**Last Updated**: January 31, 2026
**Version**: 1.0
**Status**: Complete - Ready for Implementation
