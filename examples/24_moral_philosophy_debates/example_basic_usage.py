"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_basic_usage.py                             ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     240                                            ║
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
Basic Usage Example - Ethical AI Framework

Demonstrates core functionality of the ThemisDB Ethical AI Framework.
"""

from argument_models import EthicalArgument, ArgumentChain, EthicalDecision, ArgumentType
from ethical_discourse_engine import create_discourse_engine
from rag_context_engine import create_rag_engine


def example_1_create_arguments():
    """Example 1: Create and manage ethical arguments."""
    print("\n" + "="*70)
    print("EXAMPLE 1: Creating Ethical Arguments")
    print("="*70)
    
    # Create a Kantian argument
    kant_arg = EthicalArgument(
        philosophy_school="kant",
        argument_type=ArgumentType.PRO,
        content="All persons have inherent dignity and must be treated as ends in themselves, never merely as means.",
        principle_basis=["kategorischer_imperativ", "selbstzweck"]
    )
    
    # Create a Utilitarian counter-argument
    util_arg = EthicalArgument(
        philosophy_school="utilitarianism",
        argument_type=ArgumentType.CONTRA,
        content="While individual dignity is important, we must consider the greatest good for the greatest number.",
        principle_basis=["greatest_happiness"]
    )
    
    # Link arguments
    util_arg.counterarguments.append(kant_arg.id)
    kant_arg.supports.append(util_arg.id)
    
    print(f"Kantian Argument ID: {kant_arg.id}")
    print(f"Content: {kant_arg.content[:80]}...")
    print(f"\nUtilitarian Counter ID: {util_arg.id}")
    print(f"Counters: {util_arg.counterarguments}")
    
    return kant_arg, util_arg


def example_2_initialize_debate():
    """Example 2: Initialize an ethical debate."""
    print("\n" + "="*70)
    print("EXAMPLE 2: Initializing Ethical Debate")
    print("="*70)
    
    # Create discourse engine
    engine = create_discourse_engine(philosophy_dir="philosophies")
    
    # Initialize debate
    debate = engine.initialize_debate(
        dilemma_description="Should an autonomous vehicle sacrifice its passenger to save five pedestrians?",
        philosophy_schools=['kant', 'utilitarianism', 'virtue_ethics'],
        context={'domain': 'autonomous_systems', 'urgency': 'high'}
    )
    
    print(f"Debate ID: {debate.dilemma_id}")
    print(f"Participating Schools: {', '.join(debate.participating_schools)}")
    print(f"Initial Arguments Loaded: {len(debate.initial_arguments)}")
    
    if debate.initial_arguments:
        arg = debate.initial_arguments[0]
        print(f"\nSample Argument:")
        print(f"  School: {arg.philosophy_school}")
        print(f"  Type: {arg.argument_type.value}")
        print(f"  Content: {arg.content[:100]}...")
    
    return debate


def example_3_rag_retrieval():
    """Example 3: Use RAG to retrieve relevant context."""
    print("\n" + "="*70)
    print("EXAMPLE 3: RAG Context Retrieval")
    print("="*70)
    
    # Create RAG engine
    rag = create_rag_engine()
    
    # Find similar dilemmas
    similar = rag.find_similar_dilemmas(
        "Should we prioritize individual privacy or public safety?",
        limit=5
    )
    
    print(f"Found {len(similar)} similar dilemmas:")
    for i, dilemma in enumerate(similar[:3], 1):
        print(f"\n{i}. Similarity: {dilemma['similarity_score']:.2f}")
        print(f"   Description: {dilemma['description'][:80]}...")
    
    # Retrieve philosophy-specific arguments
    kant_args = rag.retrieve_philosophy_arguments(
        philosophy_school='kant',
        argument_types=['pro'],
        limit=3
    )
    
    print(f"\n\nKantian Arguments Retrieved: {len(kant_args)}")
    if kant_args:
        print(f"Sample: {kant_args[0]['content'][:80]}...")
    
    # Build comprehensive context
    context = rag.build_rag_context(
        dilemma_description="AI decision-making in healthcare",
        philosophy_schools=['kant', 'utilitarianism'],
        dilemma_category='healthcare'
    )
    
    print(f"\n\nComprehensive Context Built:")
    print(f"  Similar dilemmas: {len(context['similar_dilemmas'])}")
    print(f"  Philosophy arguments: {len(context['philosophy_arguments'])}")
    print(f"  Best practices: {len(context['best_practices'])}")
    print(f"  Recent debates: {len(context['recent_debates'])}")
    
    return context


def example_4_create_decision():
    """Example 4: Create an ethical decision."""
    print("\n" + "="*70)
    print("EXAMPLE 4: Creating Ethical Decision")
    print("="*70)
    
    # Create decision
    decision = EthicalDecision(
        dilemma_id="dilemma_001",
        decision="Implement a transparent AI system that prioritizes patient consent while optimizing treatment outcomes.",
        primary_philosophy="kant",
        supporting_philosophies=["virtue_ethics", "care_ethics"],
        confidence=0.85,
        consensus_level=0.78
    )
    
    print(f"Decision ID: {decision.id}")
    print(f"Primary Philosophy: {decision.primary_philosophy}")
    print(f"Supporting Philosophies: {', '.join(decision.supporting_philosophies)}")
    print(f"Confidence: {decision.confidence:.2f}")
    print(f"Consensus: {decision.consensus_level:.2f}")
    print(f"\nDecision Text:")
    print(f"  {decision.decision}")
    
    return decision


def example_5_argument_chain():
    """Example 5: Create an argument chain."""
    print("\n" + "="*70)
    print("EXAMPLE 5: Building Argument Chain")
    print("="*70)
    
    # Create arguments
    thesis = EthicalArgument(
        philosophy_school="kant",
        argument_type=ArgumentType.PRO,
        content="We must respect human autonomy in all AI decisions."
    )
    
    antithesis = EthicalArgument(
        philosophy_school="utilitarianism",
        argument_type=ArgumentType.CONTRA,
        content="Sometimes restricting autonomy maximizes overall welfare."
    )
    
    synthesis = EthicalArgument(
        philosophy_school="virtue_ethics",
        argument_type=ArgumentType.SYNTHESIS,
        content="Balance autonomy with beneficence through wise judgment."
    )
    
    # Create chain
    chain = ArgumentChain(
        dilemma_id="dilemma_001",
        chain_type="dialectical"
    )
    
    chain.add_argument(thesis.id)
    chain.add_argument(antithesis.id)
    chain.add_argument(synthesis.id)
    
    chain.conclusion = "A nuanced approach respecting autonomy while considering welfare."
    chain.confidence_score = 0.82
    
    print(f"Chain ID: {chain.id}")
    print(f"Type: {chain.chain_type}")
    print(f"Arguments: {len(chain.arguments)}")
    print(f"Confidence: {chain.confidence_score:.2f}")
    print(f"\nConclusion: {chain.conclusion}")
    
    return chain


def main():
    """Run all examples."""
    print("\n" + "="*70)
    print("THEMISDB ETHICAL AI FRAMEWORK - BASIC USAGE EXAMPLES")
    print("="*70)
    
    # Run examples
    example_1_create_arguments()
    example_2_initialize_debate()
    example_3_rag_retrieval()
    example_4_create_decision()
    example_5_argument_chain()
    
    print("\n" + "="*70)
    print("ALL EXAMPLES COMPLETED")
    print("="*70 + "\n")


if __name__ == "__main__":
    main()
