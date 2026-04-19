# Query Engine Unification: AQL + GraphQL Execution Draft

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-19  

---

## I. Introduction

ThemisDB's query engine currently supports two distinct paradigms:
- **AQL** (Adaptive Query Language): Relational + document-oriented, optimized for transactional workloads
- **GraphQL**: Schema-driven graph traversal, optimized for API consumption

This draft explores a unified execution model that handles both query families through a common cost-based optimizer and execution engine.

## II. Current State

### AQL Execution Path
- Parser: Yacc-based grammar (src/aql/parser/)
- Optimizer: Rule-based + heuristic costing (src/aql/optimizer/)
- Executor: Iterator-model execution (src/aql/executor/)
- Storage: Multi-index coordinated reads

### GraphQL Execution Path  
- Parser: GraphQL standard grammar (src/graphql/parser/)
- Executor: Field resolver-based execution
- N+1 problem mitigation: Batching directives

## III. Unification Opportunities

### 1. Common IR (Intermediate Representation)
Both AQL and GraphQL can be compiled to:
- Operator tree (SELECT, PROJECT, JOIN, TRAVERSE)
- Cost model: Cardinality estimation + selectivity
- Execution plan: Physical operator assignment

### 2. Query Normalization
- GraphQL field traversal → JOIN + PROJECT sequence
- AQL SELECT → GraphQL root query fragment
- Schema mapping: AQL table schema ↔ GraphQL type schema

### 3. Shared Optimizer Rules
- Predicate pushdown
- Early projection
- Join reordering
- Index selection

## IV. Implementation Phases

### Phase 1: IR Design (Pending)
- [ ] Define unified operator set
- [ ] Implement GraphQL → IR compiler
- [ ] Implement IR → execution plan mapper

### Phase 2: Cost Model Unification (Pending)
- [ ] Merge AQL statistics + GraphQL schema stats
- [ ] Unified cardinality estimator
- [ ] Cost calibration on mixed workloads

### Phase 3: Shared Optimization (Pending)
- [ ] Rule-based optimizer for unified IR
- [ ] Incremental plan refinement

### Phase 4: Execution Engine Consolidation (Pending)
- [ ] Iterator model for all operators
- [ ] Unified context/state management

## V. Challenges

1. **Schema Mismatch**: AQL tables ↔ GraphQL types may not 1:1 map
2. **Traversal Semantics**: Graph edges are explicit in GraphQL, implicit joins in AQL
3. **Cost Model Calibration**: Different workload profiles
4. **Backward Compatibility**: Existing AQL/GraphQL deployments

## VI. Next Steps

- Review existing AQL cost model (src/aql/optimizer/cost_estimator.h)
- Map GraphQL resolver chains to AQL join sequences
- Design IR operator set
- Prototype IR compiler for subset of GraphQL queries
- Measure query plan quality on reference benchmark set

---

*Placeholder for research paper draft. Pending concrete implementation evidence.*
