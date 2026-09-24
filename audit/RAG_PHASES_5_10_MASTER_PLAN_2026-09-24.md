# RAG Readiness Phases 5-10 Implementation Roadmap
**Master Plan for Enterprise Production Hardening (Q1-Q3 2027)**

**Document Version:** 1.0  
**Status:** Ready for Batch 4 Implementation  
**Date:** 2026-09-24  
**Author:** ThemisDB Contributors

---

## Executive Summary

Phases 5-10 deliver enterprise-grade production hardening across adaptive routing, cost control, freshness SLA, observability, evaluation quality, and cost optimization. Total effort: ~24,000 LOC across 3 parallel execution batches over 44 weeks.

**Key Deliverables:**
- ✅ Phase 5: Adaptive Routing (QueryIntentClassifier, AdaptiveRouter, PolicyStore)
- ✅ Phase 6: Reranking Budget Gate (BudgetGate, Orchestrator, CostAnalyzer)
- ✅ Phase 7: Freshness SLA (LatencyMonitor, StalenessRouter, Scheduler)
- ✅ Phase 8: Observability SLO (SLOTracker, Telemetry, Alerting)
- ✅ Phase 9: Research Eval Harness (EvalRunner, DriftDetector, Adversarial)
- ✅ Phase 10: Cost Optimizer (ParetoOptimizer, SLOEnforcer, Analyzer)

**Specifications:** 6 detailed markdown documents (58KB) with API contracts, acceptance criteria, testing strategy, and rollout plans.

---

## Batch Structure

### Batch 4: Phases 5-6 (Adaptive Retrieval + Cost Control)
**Timeline:** Q1 2027 (10 weeks)  
**Effort:** 4,000 LOC production + 1,000 LOC tests  
**Dependencies:** Phases 1-4 complete ✅

**Phase 5: Adaptive Routing**
- Components: QueryIntentClassifier (NLP), AdaptiveHybridRouter, RouterPolicyStore
- Features: Intent classification (4 categories), dynamic weight adjustment, offline feedback loop
- Testing: Unit (3 test files, 20+ tests), integration (E2E, A/B testing)
- Acceptance: 5% nDCG lift, ≤10% latency overhead, ≥95% classification accuracy

**Phase 6: Reranking Budget Gate**
- Components: RerankerBudgetGate (ROI estimation), CrossEncoderOrchestrator, RerankerCostAnalyzer
- Features: ROI-based routing, batch efficiency, tenant budget enforcement
- Testing: Unit (3 test files, 18+ tests), integration (budget enforcement, multi-tenant)
- Acceptance: ≥80% beneficial rerank rate, cost accuracy ±5%, 100% budget enforcement

**Files to Create:**
```
include/rag/query_intent_classifier.h/cpp
include/rag/adaptive_hybrid_router.h/cpp
include/rag/router_policy_store.h/cpp
include/rag/reranker_budget_gate.h/cpp
include/rag/cross_encoder_orchestrator.h/cpp
include/rag/reranker_cost_analyzer.h/cpp

tests/rag/test_query_intent_classifier.cpp
tests/rag/test_adaptive_hybrid_router.cpp
tests/rag/test_router_policy_store.cpp
tests/rag/test_reranker_budget_gate.cpp
tests/rag/test_cross_encoder_orchestrator.cpp
tests/rag/test_reranker_cost_analyzer.cpp

Specifications: (already created)
src/rag/ADAPTIVE_ROUTING_SPECIFICATION.md
src/rag/RERANKING_BUDGET_GATE_SPECIFICATION.md
```

**Deliverables:**
- 6 headers (1,300 LOC)
- 6 implementation files (1,500 LOC)
- 6 test files (1,200 LOC)
- CI gate workflow: `gate-pr-rag-phase5-6.yml`

---

### Batch 5: Phase 7 (Data Freshness SLA)
**Timeline:** Q1-Q2 2027 (7 weeks, parallel to Batch 4 Phase 6)  
**Effort:** 3,300 LOC production + 800 LOC tests  
**Dependencies:** Phase 4 security (KeyRotationManager for version tracking)

**Phase 7: Freshness SLA**
- Components: IngestionLatencyMonitor (T-Digest aggregation), StalenessAwareRouter, IndexRefreshScheduler, FreshnessSLAEnforcer
- Features: p95 staleness tracking, SLA compliance, emergency refresh, staleness metadata in response
- Testing: Unit (4 test files, 22+ tests), integration (multi-shard, emergency scenarios)
- Acceptance: p95 < 5min, 99%+ SLA compliance, <10s refresh latency, 99.9% fallback availability

**Files to Create:**
```
include/rag/ingestion_latency_monitor.h/cpp
include/rag/staleness_aware_router.h/cpp
include/rag/index_refresh_scheduler.h/cpp
include/rag/freshness_sla_enforcer.h/cpp

tests/rag/test_ingestion_latency_monitor.cpp
tests/rag/test_staleness_aware_router.cpp
tests/rag/test_index_refresh_scheduler.cpp
tests/rag/test_freshness_sla_enforcer.cpp

Specifications: (already created)
src/rag/FRESHNESS_SLA_SPECIFICATION.md
```

**Deliverables:**
- 4 headers (1,000 LOC)
- 4 implementation files (1,200 LOC)
- 4 test files (1,100 LOC)
- CI gate workflow: `gate-pr-rag-phase7.yml`

---

### Batch 6: Phases 8-10 (Observability + Optimization)
**Timeline:** Q2-Q3 2027 (16 weeks, after Batch 4)  
**Effort:** 7,500 LOC production + 1,600 LOC tests  
**Dependencies:** Batch 4 complete (routing + budgeting), Phase 7 complete (freshness)

**Phase 8: Observability SLO**
- Components: SLODefinition (spec versioning), SLOTracker (windowed percentiles), TelemetryGate (OTLP)
- Features: Real-time compliance tracking, burn rate calculation, multi-window voting, OTLP integration
- Testing: Unit (3 test files, 16+ tests), integration (alert accuracy, OTLP delivery)
- Acceptance: ±2% accuracy, <60s alert latency, ≥95% alert accuracy, ≥99.9% telemetry delivery

**Phase 9: Research Eval Harness**
- Components: EvaluationDataset (5+ domains), EvaluationRunner (metrics), DriftDetector (statistical), AdversarialEval
- Features: Multi-domain evaluation, drift detection, RCA recommendations, adversarial robustness
- Testing: Unit (4 test files, 20+ tests), integration (pipeline, trend analysis)
- Acceptance: Drift detection <2 cycles latency, ≥95% F1 accuracy, ≥5 domains, 80% adversarial recovery

**Phase 10: Cost Optimizer**
- Components: ParetoOptimizer (frontier exploration), TenantSLOEnforcer (policy application), TradeoffAnalyzer, CostAttributor
- Features: Multi-objective optimization (quality-latency-cost), tenant utility functions, trade-off visualization
- Testing: Unit (4 test files, 18+ tests), integration (multi-tenant, enforcement, SLA compliance)
- Acceptance: ≥20% cost reduction, ≥90% frontier accuracy, 99%+ SLA enforcement, <2h optimization time

**Files to Create:**
```
# Phase 8
include/rag/slo_definition.h/cpp
include/rag/slo_tracker.h/cpp
include/rag/telemetry_gate.h/cpp

tests/rag/test_slo_definition.cpp
tests/rag/test_slo_tracker.cpp
tests/rag/test_telemetry_gate.cpp

# Phase 9
include/rag/evaluation_dataset.h/cpp
include/rag/evaluation_runner.h/cpp
include/rag/drift_detector.h/cpp
include/rag/adversarial_eval.h/cpp

tests/rag/test_evaluation_dataset.cpp
tests/rag/test_evaluation_runner.cpp
tests/rag/test_drift_detector.cpp
tests/rag/test_adversarial_eval.cpp

# Phase 10
include/rag/pareto_optimizer.h/cpp
include/rag/tenant_slo_enforcer.h/cpp
include/rag/tradeoff_analyzer.h/cpp
include/rag/cost_attributor.h/cpp

tests/rag/test_pareto_optimizer.cpp
tests/rag/test_tenant_slo_enforcer.cpp
tests/rag/test_tradeoff_analyzer.cpp
tests/rag/test_cost_attributor.cpp

Specifications: (already created)
src/rag/OBSERVABILITY_SLO_SPECIFICATION.md
src/rag/RESEARCH_EVAL_HARNESS_SPECIFICATION.md
src/rag/COST_OPTIMIZER_SPECIFICATION.md
```

**Deliverables:**
- 11 headers (3,100 LOC)
- 11 implementation files (3,800 LOC)
- 11 test files (2,400 LOC)
- CI gate workflows: `gate-pr-rag-phase8.yml`, `gate-pr-rag-phase9.yml`, `gate-pr-rag-phase10.yml`

---

## Governance Tasks (Ongoing)

### G1: Module ROADMAP Updates
- Update 8 module ROADMAPs with Phase 5-10 entries
- Link to CI gate workflows
- Timing: Sequential with each batch (Batch 4 → update phases 5-6, etc.)

### G2: AI Wiki Synchronization
- Refresh GOVERNANCE_AND_ROADMAP.md with Phase 5-10 architecture
- Update WIKI_STATUS.json (generated_at, source_count)
- Timing: End of each batch

### G3: CI Gate Workflows
- Create 4 new gate workflows (phases 5-8, phases 9-10 combined)
- Timing: Before each batch implementation

### G4: Test Quarantine Integration
- Move adversarial/eval tests from quarantine → normal CI
- Integrate optimizer benchmarks into perf suite
- Timing: Phase 9-10 implementations

---

## Critical Path & Dependencies

```
Phase 1-4: ✅ COMPLETE

Phase 5 (Intent Classification):
  ├→ Phase 6 (RerankerBudgetGate) [dependency: Phase 5 optional]
  └→ Phase 8 (Telemetry) [dependency: Phase 5-6 for metrics]

Phase 7 (Freshness):
  ├→ Independent of 5-6
  └→ Phase 10 (SLO Enforcement) [dependency: Phase 7 for config constraints]

Phase 8 (Observability):
  ├→ Requires: Phase 5-7 complete (metrics sources)
  └→ Phase 9-10: Both require Phase 8

Phase 9 (Evaluation):
  ├→ Requires: Phase 5-8 complete (baseline metrics)
  └→ Phase 10: Requires Phase 9 (drift detection input)

Phase 10 (Optimizer):
  ├→ Requires: Phases 5-9 complete
  ├→ Critical path node (last phase)
  └→ Post-Phase 10: GA promotion (requires Phase 10 sign-off)
```

**Parallel Tracks:**
- Batch 4 (Phases 5-6) can run in parallel with Batch 5 (Phase 7)
- Phase 8 can start as soon as Phase 6 ships
- Phases 9-10 must wait for Phase 8

**Timeline:**
```
Q1 2027:
  Week 1-10: Batch 4 (Phases 5-6)
  Week 1-7: Batch 5 (Phase 7) [parallel]
  Week 6+: Phase 8 design & planning

Q2 2027:
  Week 1-8: Phase 8 (Observability)
  Week 5+: Phase 9 (Evaluation) [after Phase 8 framework]

Q3 2027:
  Week 1-8: Phase 9 (Evaluation)
  Week 5+: Phase 10 (Optimizer) [after Phase 9 drift detection]
  Week 13-16: Phase 10 completion & sign-off
```

---

## Specification Documents Created

| Document | Path | Status |
|----------|------|--------|
| Phase 5: Adaptive Routing | `src/rag/ADAPTIVE_ROUTING_SPECIFICATION.md` | ✅ Created |
| Phase 6: Reranking Budget Gate | `src/rag/RERANKING_BUDGET_GATE_SPECIFICATION.md` | ✅ Created |
| Phase 7: Freshness SLA | `src/rag/FRESHNESS_SLA_SPECIFICATION.md` | ✅ Created |
| Phase 8: Observability SLO | `src/rag/OBSERVABILITY_SLO_SPECIFICATION.md` | ✅ Created |
| Phase 9: Research Eval Harness | `src/rag/RESEARCH_EVAL_HARNESS_SPECIFICATION.md` | ✅ Created |
| Phase 10: Cost Optimizer | `src/rag/COST_OPTIMIZER_SPECIFICATION.md` | ✅ Created |

**Total Specification Size:** ~58 KB (comprehensive design contracts)

---

## Acceptance Gates

### Batch 4 Gate (Phases 5-6)
- [ ] All 6 header + implementation files compile without errors
- [ ] All 6 test files (18+ tests) pass with 100% success rate
- [ ] Code review: API contracts match specifications
- [ ] Performance benchmarks: meet p99 latency targets
- [ ] CI gates: `gate-pr-rag-phase5-6.yml` passes

### Batch 5 Gate (Phase 7)
- [ ] All 4 header + implementation files compile
- [ ] All 4 test files (22+ tests) pass
- [ ] Code review: staleness tracking logic validated
- [ ] Performance: refresh latency < 10s, staleness computation < 1ms
- [ ] CI gates: `gate-pr-rag-phase7.yml` passes

### Batch 6 Gate (Phases 8-10)
- [ ] All 11 header + implementation files compile
- [ ] All 11 test files (54+ tests) pass
- [ ] Code review: optimizer logic verified, SLO tracking validated
- [ ] Performance: optimization <2h, SLA check <10ms
- [ ] Integration tests: end-to-end pipeline working
- [ ] CI gates: all Phase 8-10 gate workflows pass
- [ ] Dashboard validation: Pareto frontier rendering correctly

---

## Risk & Mitigation

| Risk | Severity | Mitigation |
|------|----------|-----------|
| OTLP dependency unavailable | High | Graceful fallback (log telemetry locally) |
| Optimization computation too slow | High | Reduce candidate count, use Bayesian pruning |
| Drift detection false positives | Medium | Statistical rigor, multi-window voting |
| Adversarial eval generation failures | Medium | Pre-generated adversarial queries, manual review |
| Tenant SLO enforcement conflicts | Medium | Manual override, fallback to previous config |

---

## Success Metrics

**Technical:**
- 100% phase acceptance gate passage
- All specifications reflected in code
- 95%+ test pass rate
- <2% regression vs Phase 1-4

**Business:**
- 20-30% cost reduction opportunity identified
- 5%+ nDCG improvement in adaptive routing
- 99%+ SLA compliance maintained
- Sub-60s issue detection via observability

**Operational:**
- Zero production incidents from new code
- ≤10% alert false positive rate
- <48h mean time to resolution (MTTR)
- ≥95% automated SLO enforcement success

---

## Next Steps

**Immediate (before Batch 4):**
1. Review all 6 specifications for completeness
2. Create CI gate workflows for each phase
3. Update module ROADMAPs with Phase 5-10 entries
4. Brief engineering team on critical path and parallel tracks

**Batch 4 Kickoff (Q1 2027 Week 1):**
1. Begin Phase 5 implementation (QueryIntentClassifier)
2. Begin Phase 6 implementation (RerankerBudgetGate)
3. Parallel: Begin Phase 7 implementation (IngestionLatencyMonitor)
4. Daily standups: track progress against specifications

**Batch 4 Completion (Week 10):**
1. All Phase 5-6 tests passing
2. Code review completed
3. Performance benchmarks validated
4. PR merged and tagged for release

**Batch 5 → Batch 6 Transition:**
1. Phase 8 planning & design review
2. Phase 9 dataset preparation (1000 eval queries)
3. Phase 10 optimization strategy selection

---

## Questions for Team

1. **Search Strategy (Phase 10):** Bayesian or genetic algorithm?
   - Recommendation: Bayesian (faster, more predictable)
   
2. **Telemetry Sampling (Phase 8):** 100% or 10% sampling?
   - Recommendation: 100% initially, reduce post-launch
   
3. **Adversarial Eval Types (Phase 9):** OOD/Typo/Ambiguous/Misleading or more?
   - Recommendation: Start with 4 types, expand post-launch

4. **Cost Attribution Method (Phase 10):** RocksDB CF or separate files?
   - Recommendation: Column families (cleaner, better composability)

---

**End of Master Implementation Roadmap**

Next Signal: User "weiter" to begin Batch 4 implementation
