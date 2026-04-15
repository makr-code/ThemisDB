"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            demo_ethics_evaluation.py                          ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     386                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Demonstration of Ethics Evaluation Metrics
in the Moral Philosophy Debates System

This script shows how to integrate the ethics evaluation framework
with the existing moral philosophy debate system.
"""

from ethics_evaluation_metrics import (
    EthicsEvaluator,
    quick_evaluate,
    batch_evaluate
)


def demo_single_evaluation():
    """Demonstrate evaluation of a single ethical decision."""
    print("\n" + "=" * 70)
    print("DEMO 1: Single Ethical Decision Evaluation")
    print("=" * 70 + "\n")
    
    # Simulated ethical decision from a moral philosophy debate
    decision = {
        'id': 'trolley-problem-001',
        'decision': (
            "After careful consideration of all stakeholders and consequences, "
            "we conclude that pulling the lever is ethically justified. This "
            "principle follows from utilitarian reasoning because it minimizes "
            "overall harm, while also respecting the categorical imperative by "
            "treating all lives with equal consideration. Therefore, the decision "
            "to act achieves both practical feasibility and ethical alignment."
        ),
        'primary_philosophy': 'utilitarianism',
        'supporting_philosophies': ['kant', 'virtue_ethics'],
        'principle_basis': [
            'utility_maximization',
            'harm_minimization',
            'equal_consideration'
        ],
        'confidence': 0.78,
        'consensus_level': 0.65,
        'dissenting_views': {
            'deontology': 'Acting directly causes harm, which is impermissible'
        },
        'argument_chain_ids': ['arg-001', 'arg-002', 'arg-003']
    }
    
    # Evaluate the decision
    result = quick_evaluate(decision)
    
    # Display results
    print(f"Decision: {decision['decision'][:100]}...")
    print(f"\nPrimary Philosophy: {decision['primary_philosophy']}")
    print(f"Supporting: {', '.join(decision['supporting_philosophies'])}")
    print(f"\n{'─' * 70}")
    print("EVALUATION SCORES")
    print('─' * 70)
    print(f"Overall Ethics Score:     {result.overall_score:.3f} / 1.000")
    print(f"\nDimension Breakdown:")
    print(f"  • Decision Quality:     {result.decision_quality.overall_score:.3f}")
    print(f"  • Consistency:          {result.consistency.overall_score:.3f}")
    print(f"  • Fairness:             {result.fairness.overall_score:.3f}")
    print(f"  • Alignment:            {result.alignment.overall_score:.3f}")
    print(f"  • Transparency:         {result.transparency.overall_score:.3f}")
    
    print(f"\n{'─' * 70}")
    print("DECISION QUALITY DETAILS")
    print('─' * 70)
    print(f"  Outcome Satisfaction:   {result.decision_quality.outcome_satisfaction:.3f}")
    print(f"  Ethical Alignment:      {result.decision_quality.ethical_alignment:.3f}")
    print(f"  Feasibility:            {result.decision_quality.feasibility:.3f}")
    print(f"  Long-term Impact:       {result.decision_quality.long_term_impact:.3f}")
    
    print(f"\n{'─' * 70}")
    print("TRANSPARENCY ANALYSIS")
    print('─' * 70)
    print(f"  Completeness:           {result.transparency.explanation_completeness:.3f}")
    print(f"  Clarity:                {result.transparency.reasoning_clarity:.3f}")
    print(f"  Robustness:             {result.transparency.justification_robustness:.3f}")
    print(f"  Traceability:           {result.transparency.traceability:.3f}")
    
    if result.transparency.missing_elements:
        print(f"  Missing Elements:       {', '.join(result.transparency.missing_elements)}")
    else:
        print(f"  Missing Elements:       None ✓")


def demo_comparative_evaluation():
    """Demonstrate comparative evaluation across different philosophies."""
    print("\n" + "=" * 70)
    print("DEMO 2: Comparative Evaluation Across Philosophies")
    print("=" * 70 + "\n")
    
    # Same dilemma, different philosophical approaches
    decisions = [
        {
            'id': 'approach-utilitarian',
            'decision': (
                "Maximize overall welfare by considering consequences for all "
                "stakeholders. The principle of utility therefore guides our action."
            ),
            'primary_philosophy': 'utilitarianism',
            'supporting_philosophies': ['consequentialism'],
            'principle_basis': ['utility_maximization', 'welfare'],
            'confidence': 0.85,
            'consensus_level': 0.70
        },
        {
            'id': 'approach-kantian',
            'decision': (
                "Act according to the categorical imperative, treating all persons "
                "as ends in themselves. Therefore, we must respect human dignity "
                "and consider universal principles."
            ),
            'primary_philosophy': 'kant',
            'supporting_philosophies': ['deontology'],
            'principle_basis': ['categorical_imperative', 'dignity', 'universalizability'],
            'confidence': 0.80,
            'consensus_level': 0.75
        },
        {
            'id': 'approach-virtue',
            'decision': (
                "Cultivate virtues such as wisdom, courage, and justice. A virtuous "
                "agent would therefore act with practical wisdom considering character."
            ),
            'primary_philosophy': 'virtue_ethics',
            'supporting_philosophies': ['aristotle'],
            'principle_basis': ['virtue', 'practical_wisdom', 'character'],
            'confidence': 0.75,
            'consensus_level': 0.65
        }
    ]
    
    # Evaluate all approaches
    aggregate = batch_evaluate(decisions)
    
    print("Dilemma: Autonomous vehicle must choose between two harmful outcomes")
    print(f"\nEvaluated {aggregate.num_decisions} different philosophical approaches\n")
    
    print('─' * 70)
    print("AGGREGATE STATISTICS")
    print('─' * 70)
    print(f"Mean Overall Score:       {aggregate.mean_scores['overall_score']:.3f}")
    print(f"Median Overall Score:     {aggregate.median_scores['overall_score']:.3f}")
    print(f"Standard Deviation:       {aggregate.std_scores['overall_score']:.3f}")
    print(f"Range:                    {aggregate.min_scores['overall_score']:.3f} - {aggregate.max_scores['overall_score']:.3f}")
    
    print(f"\n{'─' * 70}")
    print("DIMENSION MEANS")
    print('─' * 70)
    for dim in ['decision_quality', 'consistency', 'fairness', 'alignment', 'transparency']:
        score = aggregate.mean_scores[dim]
        bar = '█' * int(score * 30)
        print(f"  {dim:20s}  {score:.3f}  {bar}")


def demo_fairness_evaluation():
    """Demonstrate fairness evaluation with demographic data."""
    print("\n" + "=" * 70)
    print("DEMO 3: Fairness Evaluation with Demographic Analysis")
    print("=" * 70 + "\n")
    
    decision = {
        'id': 'resource-allocation-001',
        'decision': (
            "Allocate healthcare resources based on clinical need and likelihood "
            "of benefit, ensuring fair consideration of all demographic groups "
            "and stakeholders. Therefore, we apply principles of justice and equity."
        ),
        'primary_philosophy': 'rawls',
        'supporting_philosophies': ['utilitarianism'],
        'principle_basis': ['justice', 'fairness', 'equity'],
        'confidence': 0.82,
        'consensus_level': 0.78
    }
    
    # Context with demographic fairness data
    context = {
        'group_outcomes': {
            'group_a': {
                'outcome_rate': 0.76,
                'true_positive_rate': 0.82,
                'false_positive_rate': 0.14,
                'fairness_score': 0.87
            },
            'group_b': {
                'outcome_rate': 0.74,
                'true_positive_rate': 0.80,
                'false_positive_rate': 0.15,
                'fairness_score': 0.85
            },
            'group_c': {
                'outcome_rate': 0.75,
                'true_positive_rate': 0.81,
                'false_positive_rate': 0.14,
                'fairness_score': 0.86
            }
        }
    }
    
    evaluator = EthicsEvaluator()
    result = evaluator.evaluate_decision(decision, context)
    
    print("Scenario: Healthcare Resource Allocation")
    print(f"Decision: {decision['decision'][:80]}...\n")
    
    print('─' * 70)
    print("FAIRNESS METRICS")
    print('─' * 70)
    print(f"  Demographic Parity:     {result.fairness.demographic_parity:.3f}")
    print(f"    (outcome rate disparity across groups)")
    print(f"\n  Equalized Odds:         {result.fairness.equalized_odds:.3f}")
    print(f"    (TPR/FPR disparity across groups)")
    print(f"\n  Individual Fairness:    {result.fairness.individual_fairness:.3f}")
    print(f"    (similar cases treated similarly)")
    print(f"\n  Group Fairness:         {result.fairness.group_fairness_score:.3f}")
    print(f"    (average across all groups)")
    
    print(f"\n{'─' * 70}")
    print("FAIRNESS ANALYSIS")
    print('─' * 70)
    if result.fairness.demographic_parity > 0.95:
        print("  ✓ Excellent demographic parity - minimal disparity across groups")
    elif result.fairness.demographic_parity > 0.90:
        print("  ✓ Good demographic parity - low disparity across groups")
    else:
        print("  ⚠ Moderate disparity detected - review needed")


def demo_alignment_with_constraints():
    """Demonstrate alignment evaluation with constitutional constraints."""
    print("\n" + "=" * 70)
    print("DEMO 4: Alignment Evaluation with Constitutional Constraints")
    print("=" * 70 + "\n")
    
    decision = {
        'id': 'content-moderation-001',
        'decision': (
            "Implement content moderation that respects free expression while "
            "preventing harm. We must consider all stakeholders and ensure "
            "transparency in our decision-making process."
        ),
        'primary_philosophy': 'mill',
        'supporting_philosophies': ['liberalism'],
        'principle_basis': ['harm_principle', 'liberty', 'transparency'],
        'confidence': 0.80,
        'consensus_level': 0.72
    }
    
    context = {
        'expected_principles': [
            'harm_principle',
            'liberty',
            'transparency',
            'accountability'
        ],
        'constitutional_rules': [
            {'type': 'forbidden_keyword', 'keyword': 'censorship'},
            {'type': 'forbidden_keyword', 'keyword': 'suppress'}
        ],
        'constraints': [
            {'type': 'min_confidence', 'value': 0.70},
            {'type': 'required_keyword', 'keyword': 'transparency'},
            {'type': 'required_philosophy', 'philosophy': 'mill'}
        ]
    }
    
    evaluator = EthicsEvaluator()
    result = evaluator.evaluate_decision(decision, context)
    
    print("Scenario: Content Moderation Policy")
    print(f"Decision: {decision['decision']}\n")
    
    print('─' * 70)
    print("ALIGNMENT METRICS")
    print('─' * 70)
    print(f"  Principle Adherence:    {result.alignment.principle_adherence:.3f}")
    print(f"    (matches {len([p for p in decision['principle_basis'] if p in context['expected_principles']])}/{len(context['expected_principles'])} expected principles)")
    print(f"\n  Constitutional Comp.:   {result.alignment.constitutional_compliance:.3f}")
    print(f"    ({len(result.alignment.violated_principles)} constitutional violations)")
    print(f"\n  Value Alignment:        {result.alignment.value_alignment:.3f}")
    print(f"    (alignment with human values)")
    print(f"\n  Constraint Satisfaction: {result.alignment.constraint_satisfaction:.3f}")
    print(f"    ({int(result.alignment.constraint_satisfaction * len(context['constraints']))}/{len(context['constraints'])} constraints satisfied)")
    
    if result.alignment.violated_principles:
        print(f"\n  ⚠ Violations: {', '.join(result.alignment.violated_principles)}")
    else:
        print(f"\n  ✓ No constitutional violations detected")


def demo_export_formats():
    """Demonstrate export to different formats."""
    print("\n" + "=" * 70)
    print("DEMO 5: Export to JSON and Prometheus Formats")
    print("=" * 70 + "\n")
    
    decisions = [
        {
            'id': f'decision-{i}',
            'decision': f'Ethical decision {i} with principle-based reasoning.',
            'primary_philosophy': 'kant',
            'confidence': 0.75 + i * 0.05,
            'consensus_level': 0.70 + i * 0.03
        }
        for i in range(3)
    ]
    
    evaluator = EthicsEvaluator()
    for decision in decisions:
        evaluator.evaluate_decision(decision)
    
    print(f"Evaluated {len(evaluator.evaluation_history)} decisions\n")
    
    # Show sample Prometheus metrics
    print('─' * 70)
    print("SAMPLE PROMETHEUS METRICS")
    print('─' * 70)
    sample_metrics = evaluator.evaluation_history[0].to_prometheus_metrics()
    for metric in sample_metrics[:8]:
        print(f"  {metric}")
    print("  ...")
    
    # Show summary statistics
    print(f"\n{'─' * 70}")
    print("SUMMARY STATISTICS")
    print('─' * 70)
    summary = evaluator.get_summary_statistics()
    print(f"  Total Evaluations:      {summary['num_decisions']}")
    print(f"  Mean Overall Score:     {summary['mean_scores']['overall_score']:.3f}")
    print(f"  Score Range:            {summary['min_scores']['overall_score']:.3f} - {summary['max_scores']['overall_score']:.3f}")
    
    print("\n  ✓ Metrics can be exported to:")
    print("    - JSON: evaluator.export_metrics_json('metrics.json')")
    print("    - Prometheus: evaluator.export_prometheus_metrics('metrics.prom')")


def main():
    """Run all demonstrations."""
    print("\n" + "=" * 70)
    print(" " * 10 + "ETHICS EVALUATION METRICS DEMONSTRATION")
    print(" " * 15 + "Moral Philosophy Debates System")
    print("=" * 70)
    
    demo_single_evaluation()
    demo_comparative_evaluation()
    demo_fairness_evaluation()
    demo_alignment_with_constraints()
    demo_export_formats()
    
    print("\n" + "=" * 70)
    print(" " * 20 + "DEMONSTRATION COMPLETE")
    print("=" * 70)
    print("\nFor more information, see:")
    print("  - ETHICS_EVALUATION_METRICS_README.md")
    print("  - test_ethics_evaluation_metrics.py")
    print("  - ethics_evaluation_metrics.py (module source)")
    print()


if __name__ == "__main__":
    main()
