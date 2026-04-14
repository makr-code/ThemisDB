"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_ethics_evaluation_metrics.py                  ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     313                                            ║
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
Test script for ethics_evaluation_metrics module.
"""

from ethics_evaluation_metrics import (
    EthicsEvaluator,
    quick_evaluate,
    batch_evaluate,
    DecisionQualityMetrics,
    ConsistencyMetrics,
    FairnessMetrics,
    AlignmentMetrics,
    TransparencyMetrics
)


def test_single_evaluation():
    """Test evaluation of a single decision."""
    print("Test 1: Single Decision Evaluation")
    print("-" * 50)
    
    decision = {
        'id': 'test-001',
        'decision': 'We must prioritize fairness and transparency because '
                   'stakeholders deserve consideration. Therefore, we consider '
                   'consequences and apply the principle of justice.',
        'primary_philosophy': 'rawls',
        'supporting_philosophies': ['kant', 'utilitarianism'],
        'principle_basis': ['justice', 'fairness', 'transparency'],
        'confidence': 0.90,
        'consensus_level': 0.85,
        'dissenting_views': {},
        'argument_chain_ids': ['arg-1', 'arg-2', 'arg-3']
    }
    
    result = quick_evaluate(decision)
    
    print(f"Decision ID: {result.decision_id}")
    print(f"Overall Score: {result.overall_score:.3f}")
    print(f"Decision Quality: {result.decision_quality.overall_score:.3f}")
    print(f"Consistency: {result.consistency.overall_score:.3f}")
    print(f"Fairness: {result.fairness.overall_score:.3f}")
    print(f"Alignment: {result.alignment.overall_score:.3f}")
    print(f"Transparency: {result.transparency.overall_score:.3f}")
    
    assert result.overall_score > 0.0 and result.overall_score <= 1.0
    print("✓ Single evaluation passed\n")


def test_batch_evaluation():
    """Test batch evaluation of multiple decisions."""
    print("Test 2: Batch Evaluation")
    print("-" * 50)
    
    decisions = [
        {
            'id': f'batch-{i}',
            'decision': f'Decision {i} considers ethics and stakeholders with principle-based reasoning.',
            'primary_philosophy': 'kant',
            'supporting_philosophies': ['utilitarianism'],
            'principle_basis': ['categorical_imperative'],
            'confidence': 0.7 + i * 0.05,
            'consensus_level': 0.6 + i * 0.05,
            'dissenting_views': {},
            'argument_chain_ids': [f'arg-{i}']
        }
        for i in range(5)
    ]
    
    aggregate = batch_evaluate(decisions)
    
    print(f"Evaluated {aggregate.num_decisions} decisions")
    print(f"Mean overall score: {aggregate.mean_scores['overall_score']:.3f}")
    print(f"Median overall score: {aggregate.median_scores['overall_score']:.3f}")
    print(f"Std deviation: {aggregate.std_scores['overall_score']:.3f}")
    print(f"Min score: {aggregate.min_scores['overall_score']:.3f}")
    print(f"Max score: {aggregate.max_scores['overall_score']:.3f}")
    
    assert aggregate.num_decisions == 5
    print("✓ Batch evaluation passed\n")


def test_fairness_evaluation():
    """Test fairness metrics with group outcomes."""
    print("Test 3: Fairness Evaluation with Group Data")
    print("-" * 50)
    
    decision = {
        'id': 'fairness-test',
        'decision': 'Apply fair treatment across all groups',
        'primary_philosophy': 'rawls',
        'supporting_philosophies': [],
        'principle_basis': ['fairness'],
        'confidence': 0.8,
        'consensus_level': 0.7,
    }
    
    context = {
        'group_outcomes': {
            'group_a': {
                'outcome_rate': 0.75,
                'true_positive_rate': 0.80,
                'false_positive_rate': 0.15,
                'fairness_score': 0.85
            },
            'group_b': {
                'outcome_rate': 0.72,
                'true_positive_rate': 0.78,
                'false_positive_rate': 0.17,
                'fairness_score': 0.82
            }
        }
    }
    
    evaluator = EthicsEvaluator()
    result = evaluator.evaluate_decision(decision, context)
    
    print(f"Demographic Parity: {result.fairness.demographic_parity:.3f}")
    print(f"Equalized Odds: {result.fairness.equalized_odds:.3f}")
    print(f"Individual Fairness: {result.fairness.individual_fairness:.3f}")
    print(f"Group Fairness: {result.fairness.group_fairness_score:.3f}")
    
    assert result.fairness.demographic_parity > 0.9  # Small disparity
    print("✓ Fairness evaluation passed\n")


def test_alignment_evaluation():
    """Test alignment with constraints."""
    print("Test 4: Alignment Evaluation with Constraints")
    print("-" * 50)
    
    decision = {
        'id': 'alignment-test',
        'decision': 'We must ensure transparency and accountability',
        'primary_philosophy': 'kant',
        'supporting_philosophies': ['virtue_ethics'],
        'principle_basis': ['transparency', 'accountability'],
        'confidence': 0.85,
        'consensus_level': 0.8,
    }
    
    context = {
        'expected_principles': ['transparency', 'accountability', 'fairness'],
        'constitutional_rules': [
            {'type': 'required_keyword', 'keyword': 'transparency'}
        ],
        'constraints': [
            {'type': 'min_confidence', 'value': 0.7},
            {'type': 'required_keyword', 'keyword': 'accountability'}
        ]
    }
    
    evaluator = EthicsEvaluator()
    result = evaluator.evaluate_decision(decision, context)
    
    print(f"Principle Adherence: {result.alignment.principle_adherence:.3f}")
    print(f"Constitutional Compliance: {result.alignment.constitutional_compliance:.3f}")
    print(f"Constraint Satisfaction: {result.alignment.constraint_satisfaction:.3f}")
    
    assert result.alignment.constraint_satisfaction == 1.0  # All constraints satisfied
    print("✓ Alignment evaluation passed\n")


def test_export_functionality():
    """Test export to JSON and Prometheus."""
    print("Test 5: Export Functionality")
    print("-" * 50)
    
    decision = {
        'id': 'export-test',
        'decision': 'Test decision for export',
        'primary_philosophy': 'utilitarianism',
        'confidence': 0.8,
        'consensus_level': 0.7,
    }
    
    evaluator = EthicsEvaluator()
    evaluator.evaluate_decision(decision)
    
    # Test JSON export
    import tempfile
    import os
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
        json_path = f.name
    
    evaluator.export_metrics_json(json_path)
    assert os.path.exists(json_path)
    print(f"✓ JSON export successful: {json_path}")
    os.unlink(json_path)
    
    # Test Prometheus export
    with tempfile.NamedTemporaryFile(mode='w', suffix='.prom', delete=False) as f:
        prom_path = f.name
    
    evaluator.export_prometheus_metrics(prom_path)
    assert os.path.exists(prom_path)
    print(f"✓ Prometheus export successful: {prom_path}")
    
    with open(prom_path, 'r') as f:
        content = f.read()
        assert 'ethics_overall_score' in content
    
    os.unlink(prom_path)
    print("✓ Export functionality passed\n")


def test_component_metrics():
    """Test individual component metric classes."""
    print("Test 6: Component Metrics")
    print("-" * 50)
    
    # Test DecisionQualityMetrics
    quality = DecisionQualityMetrics(
        outcome_satisfaction=0.8,
        ethical_alignment=0.9,
        feasibility=0.7,
        long_term_impact=0.75
    )
    quality.calculate_overall_score()
    assert quality.overall_score > 0.0
    print(f"✓ Decision Quality: {quality.overall_score:.3f}")
    
    # Test ConsistencyMetrics
    consistency = ConsistencyMetrics(
        intra_case_consistency=0.9,
        inter_case_consistency=0.85,
        philosophy_consistency=0.88,
        temporal_consistency=0.8
    )
    consistency.calculate_overall_score()
    assert consistency.overall_score > 0.0
    print(f"✓ Consistency: {consistency.overall_score:.3f}")
    
    # Test FairnessMetrics
    fairness = FairnessMetrics(
        demographic_parity=0.9,
        equalized_odds=0.88,
        individual_fairness=0.92,
        group_fairness_score=0.85
    )
    fairness.calculate_overall_score()
    assert fairness.overall_score > 0.0
    print(f"✓ Fairness: {fairness.overall_score:.3f}")
    
    # Test AlignmentMetrics
    alignment = AlignmentMetrics(
        principle_adherence=0.95,
        constitutional_compliance=0.92,
        value_alignment=0.88,
        constraint_satisfaction=1.0
    )
    alignment.calculate_overall_score()
    assert alignment.overall_score > 0.0
    print(f"✓ Alignment: {alignment.overall_score:.3f}")
    
    # Test TransparencyMetrics
    transparency = TransparencyMetrics(
        explanation_completeness=0.9,
        reasoning_clarity=0.85,
        justification_robustness=0.88,
        traceability=0.9
    )
    transparency.calculate_overall_score()
    assert transparency.overall_score > 0.0
    print(f"✓ Transparency: {transparency.overall_score:.3f}")
    
    print("✓ All component metrics passed\n")


def main():
    """Run all tests."""
    print("\n" + "=" * 50)
    print("ETHICS EVALUATION METRICS TEST SUITE")
    print("=" * 50 + "\n")
    
    test_single_evaluation()
    test_batch_evaluation()
    test_fairness_evaluation()
    test_alignment_evaluation()
    test_export_functionality()
    test_component_metrics()
    
    print("=" * 50)
    print("ALL TESTS PASSED ✓")
    print("=" * 50)


if __name__ == "__main__":
    main()
