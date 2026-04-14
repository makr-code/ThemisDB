"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            complete_self_improving_ethics_loop.py             ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     650                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Complete Self-Improving Ethics Loop

Integrates all components into a complete self-improving ethical AI system:
- Prompt optimization
- Ethical decision-making with RAG context
- Outcome tracking
- Conditional LoRa retraining

Author: ThemisDB Ethics AI Framework
License: MIT
"""

from dataclasses import dataclass, field
from typing import List, Dict, Any, Optional
from datetime import datetime
import json


@dataclass
class EthicsLoopConfig:
    """
    Configuration for the self-improving ethics loop.
    
    Attributes:
        prompt_optimization_enabled: Enable prompt optimization phase
        rag_retrieval_enabled: Enable RAG context retrieval
        outcome_tracking_enabled: Enable outcome quality tracking
        lora_retraining_enabled: Enable conditional LoRa retraining
        retraining_quality_threshold: Minimum quality for retraining
        retraining_data_threshold: Minimum data size for retraining
        optimization_interval_decisions: Optimize prompts every N decisions
        evaluation_interval_decisions: Evaluate metrics every N decisions
    """
    
    prompt_optimization_enabled: bool = True
    rag_retrieval_enabled: bool = True
    outcome_tracking_enabled: bool = True
    lora_retraining_enabled: bool = True
    retraining_quality_threshold: float = 0.80
    retraining_data_threshold: int = 100
    optimization_interval_decisions: int = 50
    evaluation_interval_decisions: int = 25


@dataclass
class DecisionOutcome:
    """
    Tracks the outcome of an ethical decision.
    
    Attributes:
        decision_id: ID of the decision
        dilemma_id: ID of the dilemma
        satisfaction_score: User/stakeholder satisfaction (0-1)
        ethical_alignment_score: Alignment with ethical principles (0-1)
        feasibility_score: Practical feasibility (0-1)
        long_term_impact_score: Long-term impact assessment (0-1)
        feedback_text: Qualitative feedback
        timestamp: When outcome was recorded
    """
    
    decision_id: str = ""
    dilemma_id: str = ""
    satisfaction_score: float = 0.0
    ethical_alignment_score: float = 0.0
    feasibility_score: float = 0.0
    long_term_impact_score: float = 0.0
    feedback_text: str = ""
    timestamp: datetime = field(default_factory=datetime.now)
    
    def overall_quality(self) -> float:
        """Calculate overall quality score."""
        return (
            self.satisfaction_score * 0.3 +
            self.ethical_alignment_score * 0.3 +
            self.feasibility_score * 0.2 +
            self.long_term_impact_score * 0.2
        )
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'decision_id': self.decision_id,
            'dilemma_id': self.dilemma_id,
            'satisfaction_score': self.satisfaction_score,
            'ethical_alignment_score': self.ethical_alignment_score,
            'feasibility_score': self.feasibility_score,
            'long_term_impact_score': self.long_term_impact_score,
            'overall_quality': self.overall_quality(),
            'feedback_text': self.feedback_text,
            'timestamp': self.timestamp.isoformat()
        }


class CompleteEthicsImprovementLoop:
    """
    Complete 4-phase self-improving ethical AI workflow.
    
    Phases:
    1. Prompt Optimization: Iteratively improve ethics prompts
    2. Decision with Optimized Prompt: Make decisions using best prompt + RAG
    3. Outcome Tracking: Track decision outcomes and quality
    4. Self-Improvement: Conditionally retrain LoRa adapters
    """
    
    def __init__(
        self,
        config: Optional[EthicsLoopConfig] = None,
        prompt_optimizer=None,
        rag_engine=None,
        discourse_engine=None,
        lora_trainer=None,
        llm_backend=None
    ):
        """
        Initialize the complete ethics loop.
        
        Args:
            config: Loop configuration
            prompt_optimizer: EthicsPromptOptimizer instance
            rag_engine: RagContextEngine instance
            discourse_engine: EthicalDiscourseEngine instance
            lora_trainer: LoRATrainerWithOptimizedPrompts instance
            llm_backend: LLM backend for generation
        """
        self.config = config or EthicsLoopConfig()
        self.prompt_optimizer = prompt_optimizer
        self.rag_engine = rag_engine
        self.discourse_engine = discourse_engine
        self.lora_trainer = lora_trainer
        self.llm_backend = llm_backend
        
        # State tracking
        self.decision_count = 0
        self.outcomes: List[DecisionOutcome] = []
        self.pending_retraining_data: List[Dict[str, Any]] = []
        self.loop_history: List[Dict[str, Any]] = []
    
    # ========================================================================
    # Phase 1: Prompt Optimization
    # ========================================================================
    
    def phase1_optimize_prompts(
        self,
        initial_prompt: Optional[str] = None,
        max_iterations: int = 5
    ) -> Dict[str, Any]:
        """
        Phase 1: Optimize ethics prompts using test cases.
        
        Args:
            initial_prompt: Initial prompt template (uses default if None)
            max_iterations: Maximum optimization iterations
        
        Returns:
            Optimization results
        """
        if not self.config.prompt_optimization_enabled:
            print("Prompt optimization disabled")
            return {'status': 'skipped'}
        
        if not self.prompt_optimizer:
            print("No prompt optimizer configured")
            return {'status': 'error', 'message': 'No optimizer'}
        
        print("\n" + "="*70)
        print("PHASE 1: PROMPT OPTIMIZATION")
        print("="*70)
        
        # Create initial prompt if not provided
        if initial_prompt is None:
            initial_prompt = self._default_ethics_prompt()
        
        # Create initial version
        initial_version = self.prompt_optimizer.create_initial_prompt(
            initial_prompt,
            ['dilemma']
        )
        
        # Run iterative optimization
        final_version = self.prompt_optimizer.iterative_optimization(
            initial_version.version_id,
            self.llm_backend,
            max_iterations=max_iterations
        )
        
        results = {
            'status': 'success',
            'initial_version_id': initial_version.version_id,
            'final_version_id': final_version.version_id,
            'iterations': len(self.prompt_optimizer.prompt_versions) - 1,
            'final_avg_score': (
                sum(final_version.performance_scores.values()) / 
                len(final_version.performance_scores)
                if final_version.performance_scores else 0.0
            ),
            'timestamp': datetime.now().isoformat()
        }
        
        print(f"\nOptimization complete. Final avg score: {results['final_avg_score']:.3f}")
        
        self.loop_history.append({
            'phase': 'prompt_optimization',
            'results': results
        })
        
        return results
    
    # ========================================================================
    # Phase 2: Decision with Optimized Prompt
    # ========================================================================
    
    def phase2_make_decision(
        self,
        dilemma_description: str,
        philosophy_schools: List[str],
        dilemma_category: str = "general"
    ) -> Dict[str, Any]:
        """
        Phase 2: Make ethical decision using optimized prompt + RAG context.
        
        Args:
            dilemma_description: Description of the ethical dilemma
            philosophy_schools: Philosophy schools to consult
            dilemma_category: Category of the dilemma
        
        Returns:
            Decision results
        """
        print("\n" + "="*70)
        print("PHASE 2: ETHICAL DECISION-MAKING")
        print("="*70)
        
        self.decision_count += 1
        
        # Build RAG context if enabled
        rag_context_text = ""
        if self.config.rag_retrieval_enabled and self.rag_engine:
            print("Building RAG context...")
            rag_context = self.rag_engine.build_rag_context(
                dilemma_description,
                philosophy_schools,
                dilemma_category
            )
            rag_context_text = self.rag_engine.format_context_for_prompt(rag_context)
        
        # Get best prompt version
        best_prompt = self._get_best_prompt()
        
        # Construct full prompt
        full_prompt = f"""{best_prompt}

Ethical Dilemma:
{dilemma_description}

{rag_context_text}

Provide a comprehensive ethical analysis and decision.
"""
        
        # Generate decision
        if self.llm_backend:
            decision_text = self.llm_backend.generate(full_prompt)
        else:
            decision_text = self._mock_decision(dilemma_description, philosophy_schools)
        
        # Store decision
        decision_id = f"decision_{self.decision_count}_{int(datetime.now().timestamp())}"
        
        decision_results = {
            'decision_id': decision_id,
            'dilemma_description': dilemma_description,
            'philosophy_schools': philosophy_schools,
            'decision_text': decision_text,
            'rag_context_used': bool(rag_context_text),
            'prompt_version_used': self.prompt_optimizer.current_version_id if self.prompt_optimizer else None,
            'timestamp': datetime.now().isoformat()
        }
        
        print(f"\nDecision {decision_id} completed")
        print(f"Philosophies consulted: {', '.join(philosophy_schools)}")
        print(f"RAG context used: {decision_results['rag_context_used']}")
        
        self.loop_history.append({
            'phase': 'decision_making',
            'results': decision_results
        })
        
        return decision_results
    
    # ========================================================================
    # Phase 3: Outcome Tracking
    # ========================================================================
    
    def phase3_track_outcome(
        self,
        decision_id: str,
        dilemma_id: str,
        satisfaction_score: float,
        ethical_alignment_score: float,
        feasibility_score: float,
        long_term_impact_score: float,
        feedback_text: str = ""
    ) -> DecisionOutcome:
        """
        Phase 3: Track outcome of an ethical decision.
        
        Args:
            decision_id: ID of the decision
            dilemma_id: ID of the dilemma
            satisfaction_score: User satisfaction (0-1)
            ethical_alignment_score: Ethical alignment (0-1)
            feasibility_score: Practical feasibility (0-1)
            long_term_impact_score: Long-term impact (0-1)
            feedback_text: Qualitative feedback
        
        Returns:
            DecisionOutcome record
        """
        if not self.config.outcome_tracking_enabled:
            print("Outcome tracking disabled")
            return DecisionOutcome()
        
        print("\n" + "="*70)
        print("PHASE 3: OUTCOME TRACKING")
        print("="*70)
        
        outcome = DecisionOutcome(
            decision_id=decision_id,
            dilemma_id=dilemma_id,
            satisfaction_score=satisfaction_score,
            ethical_alignment_score=ethical_alignment_score,
            feasibility_score=feasibility_score,
            long_term_impact_score=long_term_impact_score,
            feedback_text=feedback_text
        )
        
        self.outcomes.append(outcome)
        
        # Add to pending retraining data if quality is high
        if outcome.overall_quality() >= self.config.retraining_quality_threshold:
            self.pending_retraining_data.append({
                'decision_id': decision_id,
                'quality_score': outcome.overall_quality(),
                'outcome_data': outcome.to_dict()
            })
        
        print(f"Outcome tracked for decision {decision_id}")
        print(f"Overall quality: {outcome.overall_quality():.3f}")
        print(f"Pending retraining data: {len(self.pending_retraining_data)} examples")
        
        self.loop_history.append({
            'phase': 'outcome_tracking',
            'results': outcome.to_dict()
        })
        
        return outcome
    
    # ========================================================================
    # Phase 4: Self-Improvement (Conditional LoRa Retraining)
    # ========================================================================
    
    def phase4_self_improvement(self) -> Optional[Dict[str, Any]]:
        """
        Phase 4: Conditionally retrain LoRa adapter based on outcomes.
        
        Returns:
            Retraining results if performed, None otherwise
        """
        if not self.config.lora_retraining_enabled:
            print("LoRa retraining disabled")
            return None
        
        if not self.lora_trainer:
            print("No LoRa trainer configured")
            return None
        
        print("\n" + "="*70)
        print("PHASE 4: SELF-IMPROVEMENT (LoRa Retraining)")
        print("="*70)
        
        # Check if we have enough data
        if len(self.pending_retraining_data) < self.config.retraining_data_threshold:
            print(f"Insufficient data: {len(self.pending_retraining_data)} < {self.config.retraining_data_threshold}")
            return None
        
        # Prepare training data
        from lora_training_with_optimized_prompts import OptimizedPromptDataset
        
        dataset = OptimizedPromptDataset(
            prompt_optimizer=self.prompt_optimizer,
            rag_engine=self.rag_engine
        )
        
        # Generate examples from successful decisions
        decision_ids = [d['decision_id'] for d in self.pending_retraining_data]
        dataset.generate_from_successful_decisions(
            decision_ids,
            min_satisfaction=self.config.retraining_quality_threshold
        )
        
        # Balance dataset
        balanced_examples = dataset.balance_by_philosophy(target_per_philosophy=50)
        
        # Prepare for training
        train_data, test_data = self.lora_trainer.prepare_training(dataset)
        
        # Conditional retrain
        retrain_results = self.lora_trainer.conditional_retrain(
            train_data,
            quality_threshold=self.config.retraining_quality_threshold,
            min_data_size=self.config.retraining_data_threshold
        )
        
        if retrain_results:
            print("\nRetraining successful!")
            print(f"New adapter: {retrain_results['adapter_path']}")
            print(f"Training loss: {retrain_results['loss']:.3f}")
            
            # Clear pending data
            self.pending_retraining_data.clear()
            
            self.loop_history.append({
                'phase': 'self_improvement',
                'results': retrain_results
            })
        
        return retrain_results
    
    # ========================================================================
    # Complete Loop Execution
    # ========================================================================
    
    def run_complete_loop(
        self,
        dilemma_description: str,
        philosophy_schools: List[str],
        dilemma_category: str = "general",
        with_optimization: bool = False
    ) -> Dict[str, Any]:
        """
        Run complete 4-phase ethics improvement loop.
        
        Args:
            dilemma_description: Ethical dilemma to analyze
            philosophy_schools: Philosophy schools to consult
            dilemma_category: Category of dilemma
            with_optimization: Whether to run prompt optimization first
        
        Returns:
            Complete loop results
        """
        loop_start = datetime.now()
        
        print("\n" + "="*70)
        print("COMPLETE ETHICS IMPROVEMENT LOOP")
        print("="*70)
        
        results = {
            'loop_id': f"loop_{int(loop_start.timestamp())}",
            'start_time': loop_start.isoformat()
        }
        
        # Phase 1: Prompt Optimization (optional, periodic)
        if with_optimization or (self.decision_count % self.config.optimization_interval_decisions == 0):
            optimization_results = self.phase1_optimize_prompts()
            results['optimization'] = optimization_results
        
        # Phase 2: Decision-Making
        decision_results = self.phase2_make_decision(
            dilemma_description,
            philosophy_schools,
            dilemma_category
        )
        results['decision'] = decision_results
        
        # Phase 3: Outcome Tracking (simulated for demo)
        # In production, this would be called after real-world implementation
        outcome = self.phase3_track_outcome(
            decision_id=decision_results['decision_id'],
            dilemma_id=f"dilemma_{self.decision_count}",
            satisfaction_score=0.85,  # Simulated
            ethical_alignment_score=0.88,
            feasibility_score=0.82,
            long_term_impact_score=0.80,
            feedback_text="Simulated positive outcome"
        )
        results['outcome'] = outcome.to_dict()
        
        # Phase 4: Self-Improvement (conditional)
        if self.decision_count % self.config.evaluation_interval_decisions == 0:
            improvement_results = self.phase4_self_improvement()
            if improvement_results:
                results['improvement'] = improvement_results
        
        results['end_time'] = datetime.now().isoformat()
        results['duration_seconds'] = (datetime.now() - loop_start).total_seconds()
        
        print("\n" + "="*70)
        print("LOOP COMPLETE")
        print(f"Duration: {results['duration_seconds']:.2f} seconds")
        print("="*70 + "\n")
        
        return results
    
    def save_loop_history(self, output_file: str) -> None:
        """Save complete loop history to file."""
        history_data = {
            'config': {
                'prompt_optimization_enabled': self.config.prompt_optimization_enabled,
                'rag_retrieval_enabled': self.config.rag_retrieval_enabled,
                'outcome_tracking_enabled': self.config.outcome_tracking_enabled,
                'lora_retraining_enabled': self.config.lora_retraining_enabled
            },
            'decision_count': self.decision_count,
            'outcomes_count': len(self.outcomes),
            'average_quality': (
                sum(o.overall_quality() for o in self.outcomes) / len(self.outcomes)
                if self.outcomes else 0.0
            ),
            'loop_history': self.loop_history,
            'exported_at': datetime.now().isoformat()
        }
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(history_data, f, indent=2, ensure_ascii=False)
        
        print(f"Saved loop history to {output_file}")
    
    # ========================================================================
    # Helper Methods
    # ========================================================================
    
    def _default_ethics_prompt(self) -> str:
        """Default ethics prompt template."""
        return """You are an ethical AI assistant specialized in moral philosophy.

Analyze the following ethical dilemma:

{dilemma}

Provide a comprehensive analysis considering:
1. Multiple philosophical perspectives (Kantian, Utilitarian, Virtue Ethics, etc.)
2. Key ethical considerations and principles
3. Potential consequences and stakeholder impacts
4. A balanced, well-justified recommendation

Structure your response with clear sections for each perspective and a final synthesis.
"""
    
    def _get_best_prompt(self) -> str:
        """Get the best performing prompt version."""
        if not self.prompt_optimizer or not self.prompt_optimizer.current_version_id:
            return self._default_ethics_prompt()
        
        current_version = self.prompt_optimizer.prompt_versions[
            self.prompt_optimizer.current_version_id
        ]
        return current_version.prompt_text
    
    def _mock_decision(self, dilemma: str, schools: List[str]) -> str:
        """Mock decision for testing."""
        return f"""Ethical Analysis of: {dilemma[:100]}...

Philosophical Perspectives:

{chr(10).join(f'### {school.title()}: Analysis from {school} perspective...' for school in schools)}

Synthesis:
After considering multiple ethical frameworks, a balanced decision that respects
core principles while maximizing positive outcomes is recommended.

Decision: [Specific recommendation based on the analysis]
"""


# Convenience function
def create_complete_loop(
    config: Optional[EthicsLoopConfig] = None,
    themis_host: str = "localhost",
    themis_port: int = 8080
) -> CompleteEthicsImprovementLoop:
    """
    Create complete ethics improvement loop with all components.
    
    Args:
        config: Loop configuration
        themis_host: ThemisDB host
        themis_port: ThemisDB port
    
    Returns:
        Configured CompleteEthicsImprovementLoop
    """
    # Import components
    from ethics_prompt_optimization_framework import (
        EthicsPromptOptimizer, create_standard_test_suite
    )
    from rag_context_engine import RagContextEngine
    from ethical_discourse_engine import EthicalDiscourseEngine
    from lora_training_with_optimized_prompts import LoRATrainerWithOptimizedPrompts
    
    # Create components
    try:
        from themis_client import MoralDebateClient
        client = MoralDebateClient(host=themis_host, port=themis_port)
    except ImportError:
        client = None
    
    prompt_optimizer = EthicsPromptOptimizer(themis_client=client)
    
    # Add standard test cases
    for test_case in create_standard_test_suite():
        prompt_optimizer.add_test_case(test_case)
    
    rag_engine = RagContextEngine(themis_client=client)
    discourse_engine = EthicalDiscourseEngine(themis_client=client)
    lora_trainer = LoRATrainerWithOptimizedPrompts(
        base_model_path="meta-llama/Llama-2-7b-hf"
    )
    
    return CompleteEthicsImprovementLoop(
        config=config or EthicsLoopConfig(),
        prompt_optimizer=prompt_optimizer,
        rag_engine=rag_engine,
        discourse_engine=discourse_engine,
        lora_trainer=lora_trainer
    )
