"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_complete_workflow.py                       ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     143                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Complete Workflow Example - Self-Improving Ethics Loop

Demonstrates the full 4-phase self-improving ethical AI workflow.
"""

from complete_self_improving_ethics_loop import (
    create_complete_loop,
    EthicsLoopConfig
)


def main():
    """Run complete self-improving ethics workflow."""
    print("\n" + "="*70)
    print("COMPLETE SELF-IMPROVING ETHICS WORKFLOW")
    print("="*70)
    
    # Configure the loop
    config = EthicsLoopConfig(
        prompt_optimization_enabled=True,
        rag_retrieval_enabled=True,
        outcome_tracking_enabled=True,
        lora_retraining_enabled=True,
        retraining_quality_threshold=0.80,
        retraining_data_threshold=100,
        optimization_interval_decisions=50,
        evaluation_interval_decisions=25
    )
    
    print("\nConfiguration:")
    print(f"  Prompt Optimization: {config.prompt_optimization_enabled}")
    print(f"  RAG Retrieval: {config.rag_retrieval_enabled}")
    print(f"  Outcome Tracking: {config.outcome_tracking_enabled}")
    print(f"  LoRa Retraining: {config.lora_retraining_enabled}")
    
    # Create complete loop
    print("\nInitializing complete ethics loop...")
    loop = create_complete_loop(config=config)
    
    # Test dilemmas
    dilemmas = [
        {
            'description': "Should an autonomous vehicle prioritize passenger safety over pedestrian safety?",
            'philosophies': ['kant', 'utilitarianism', 'virtue_ethics'],
            'category': 'autonomous_systems'
        },
        {
            'description': "Should AI be allowed to make hiring decisions without human oversight?",
            'philosophies': ['kant', 'utilitarianism', 'care_ethics'],
            'category': 'employment'
        },
        {
            'description': "Should patient data be shared for AI research without explicit consent?",
            'philosophies': ['kant', 'utilitarianism', 'virtue_ethics'],
            'category': 'healthcare'
        }
    ]
    
    # Run workflow for each dilemma
    results = []
    for i, dilemma in enumerate(dilemmas, 1):
        print(f"\n{'='*70}")
        print(f"DILEMMA {i}/{len(dilemmas)}")
        print(f"{'='*70}")
        print(f"Description: {dilemma['description'][:80]}...")
        
        # Run complete loop (with optimization on first iteration)
        result = loop.run_complete_loop(
            dilemma_description=dilemma['description'],
            philosophy_schools=dilemma['philosophies'],
            dilemma_category=dilemma['category'],
            with_optimization=(i == 1)  # Optimize on first run
        )
        
        results.append(result)
        
        # Print summary
        print(f"\nResult Summary:")
        print(f"  Loop ID: {result['loop_id']}")
        print(f"  Duration: {result['duration_seconds']:.2f}s")
        
        if 'optimization' in result:
            opt = result['optimization']
            print(f"  Optimization Score: {opt.get('final_avg_score', 0):.3f}")
        
        decision = result['decision']
        print(f"  Decision ID: {decision['decision_id']}")
        print(f"  RAG Context Used: {decision['rag_context_used']}")
        
        outcome = result['outcome']
        print(f"  Overall Quality: {outcome['overall_quality']:.3f}")
        print(f"  Satisfaction: {outcome['satisfaction_score']:.3f}")
        print(f"  Ethical Alignment: {outcome['ethical_alignment_score']:.3f}")
    
    # Save history
    history_file = "/tmp/ethics_loop_history.json"
    loop.save_loop_history(history_file)
    print(f"\nWorkflow history saved to: {history_file}")
    
    # Print overall statistics
    print(f"\n{'='*70}")
    print("WORKFLOW STATISTICS")
    print(f"{'='*70}")
    print(f"Total Decisions: {loop.decision_count}")
    print(f"Total Outcomes Tracked: {len(loop.outcomes)}")
    print(f"Pending Retraining Data: {len(loop.pending_retraining_data)}")
    
    if loop.outcomes:
        avg_quality = sum(o.overall_quality() for o in loop.outcomes) / len(loop.outcomes)
        print(f"Average Decision Quality: {avg_quality:.3f}")
    
    print(f"\n{'='*70}")
    print("WORKFLOW COMPLETE")
    print(f"{'='*70}\n")


if __name__ == "__main__":
    main()
