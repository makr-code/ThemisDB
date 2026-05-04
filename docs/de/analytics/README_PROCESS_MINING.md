# Process Mining in AQL - Project Documentation

**Version**: 1.0  
**Date**: 2025-12-24  
**Status**: Research & Design Phase Complete ✅

---

## Overview

This documentation suite covers the complete design and implementation plan for integrating **Process Mining** capabilities into ThemisDB's **Advanced Query Language (AQL)**.

### Original Requirement

> "Ich möchte das Process-mining auch in AQL möglich machen. eine Idee ist eine Ideal Prozessstruktur zu übergeben und die process-miner suchen nach graphen, vectoren, relationalen Zusammenhängen und zeigen entsprechende gefundene 'ähnliche' Prozesse an."

**Translation**: Enable process mining in AQL by passing an ideal process structure and having process miners search for similar processes using graph, vector, and relational patterns.

### New Requirement

> "Modelle für Verwaltung bevorzugt"

**Translation**: Prefer models for public administration/governance.

---

## 📁 Documentation Structure

### 1. [PROCESS_MINING_SUMMARY.md](./PROCESS_MINING_SUMMARY.md) ⭐ **START HERE**
**Executive Summary** - High-level overview of the entire project

**Contents**:
- Project objectives and achievements
- Deliverables summary (documentation, code, config)
- Architecture overview
- Quick reference to all other documents
- Next steps

**Audience**: Project managers, stakeholders, new team members

---

### 2. [PROCESS_MINING_RESEARCH_AND_ROADMAP.md](./PROCESS_MINING_RESEARCH_AND_ROADMAP.md)
**Comprehensive Research** - 50+ pages of scientific foundation and implementation plan

**Contents**:
- **Section 1**: Scientific foundations (11 publications)
- **Section 2**: Existing ThemisDB features
- **Section 3**: Requirements analysis
- **Section 4**: Solution architecture
- **Section 5**: Implementation roadmap (6 phases)
- **Section 6**: Example use cases
- **Section 7**: Scientific publications reference
- **Section 8**: Success criteria
- **Section 9**: Risk analysis

**Key Topics**:
- Alpha Miner, Heuristic Miner, Inductive Miner algorithms
- Graph Edit Distance, Jaccard Similarity, LCS, Cosine Similarity
- Administrative process best practices
- Compliance frameworks (GWB, BauO, DSGVO, AGG)

**Audience**: Developers, researchers, architects

---

### 3. [PROCESS_MINING_AQL_EXAMPLES.md](./PROCESS_MINING_AQL_EXAMPLES.md)
**Practical Examples** - 10 complete real-world scenarios with AQL queries

**Contents**:
- **Example 1**: Finding similar building permit processes
- **Example 2**: Conformance checking against ideal process
- **Example 3**: Pattern-based filtering
- **Example 4**: Procurement process discovery
- **Example 5**: Process variant analysis (HR)
- **Example 6**: Bottleneck detection (Budget planning)
- **Example 7**: Predictive analytics (End time prediction)
- **Example 8**: List available administrative models
- **Example 9**: Export discovered process as BPMN
- **Example 10**: Complex analysis - Cross-department comparison

**Also Includes**:
- Tips & best practices
- Troubleshooting guide
- Performance optimization tips

**Audience**: Database users, analysts, administrators

---

### 4. [process_mining_guide.md](./process_mining_guide.md)
**Feature Guide** - Overview of process mining capabilities (existing document)

**Contents**:
- Process discovery algorithms
- Analysis capabilities (DFG, variants, bottlenecks)
- Conformance checking
- Export formats (BPMN, Petri Net)
- Usage examples

**Audience**: End users, tutorial readers

---

### 5. [BPMN_FUTURE_ENHANCEMENTS.md](./BPMN_FUTURE_ENHANCEMENTS.md)
**Future Enhancements** - Planned improvements for BPMN/process engine

**Contents**:
- Wire protocol migration to binary protobuf
- Individual node visit timestamp tracking
- ProcessGraphManager initialization improvements

**Audience**: Developers, architects, project managers

---

## 💻 Code Documentation

### 1. [process_pattern_matcher.h](../../include/analytics/process_pattern_matcher.h)
**Core Pattern Matching Class**

```cpp
class ProcessPatternMatcher {
    // Find processes similar to a pattern
    std::pair<Status, std::vector<SimilarityResult>> findSimilar(
        const ProcessPattern& pattern,
        const PatternMatchConfig& config
    );
    
    // Compare process with ideal model
    std::pair<Status, ConformanceResult> compareWithIdeal(
        const std::string& case_id,
        const ProcessPattern& ideal_pattern
    );
    
    // Check if process matches pattern
    std::pair<Status, bool> hasPattern(
        const std::string& case_id,
        const ProcessPattern& pattern,
        double threshold
    );
    
    // Load administrative models
    std::pair<Status, std::map<std::string, ProcessPattern>> 
        loadAdministrativeModels();
};
```

**Features**:
- 4 similarity methods: GRAPH, VECTOR, BEHAVIORAL, HYBRID
- Pattern caching for performance
- Integration with VectorIndex and GraphIndex
- Batch operations support

---

### 2. [process_mining_functions.h](../../include/query/functions/process_mining_functions.h)
**AQL Function Definitions**

**15 New AQL Functions**:

**Pattern Matching** (NEW):
- `PM_FIND_SIMILAR(pattern, config)` → Array
- `PM_COMPARE_IDEAL(case_id, ideal)` → Object
- `PM_HAS_PATTERN(case_id, pattern, threshold)` → Boolean

**Event Log**:
- `PM_EXTRACT_LOG(collection, config)` → Object
- `PM_EXTRACT_TRACE(case_id)` → Object

**Discovery**:
- `PM_DISCOVER_PROCESS(log, config)` → Object
- `PM_VARIANTS(log, top_n)` → Array

**Administrative Models** (NEW):
- `PM_LOAD_ADMIN_MODEL(model_id)` → Object
- `PM_LIST_ADMIN_MODELS()` → Array

**Conformance**:
- `PM_CONFORMANCE(case_id, model)` → Object
- `PM_DEVIATIONS(case_id, model)` → Array

**Performance**:
- `PM_BOTTLENECKS(log, threshold)` → Array
- `PM_PREDICT_END(case_id)` → Object

**Export**:
- `PM_EXPORT_BPMN(model)` → String

---

## ⚙️ Configuration

### [administrative_process_models.yaml](../../config/process_models/administrative_process_models.yaml)
**Predefined Administrative Process Models**

**5 Models Included**:

1. **bauantrag_standard** - Building Permit Process
   - §34 BauO compliant
   - 3-month SLA
   - Vier-Augen-Prinzip

2. **beschaffung_vergaberecht** - Procurement Process
   - GWB and VOB/A compliant
   - EU threshold rules (€214,000)
   - Documentation requirements

3. **personal_einstellung** - HR Recruitment
   - AGG compliant (anti-discrimination)
   - DSGVO compliant (data protection)
   - Betriebsrat integration

4. **haushaltsplanung_jaehrlich** - Budget Planning
   - Annual cycle
   - Fixed milestones (30.09., 15.10., 30.11.)
   - Multi-level approval

5. **dokumenten_freigabe** - Document Approval
   - Multi-stage workflow
   - Version control
   - Vier-Augen-Prinzip

---

## 🎯 Quick Start Guide

### For Users: Run Your First Query

```aql
-- 1. List available administrative models
LET models = PM_LIST_ADMIN_MODELS()
RETURN models

-- 2. Load a specific model
LET ideal = PM_LOAD_ADMIN_MODEL("bauantrag_standard")
RETURN ideal

-- 3. Find similar processes
LET similar = PM_FIND_SIMILAR(ideal, {
  method: "hybrid",
  threshold: 0.75,
  limit: 10
})
RETURN similar
```

**See**: [PROCESS_MINING_AQL_EXAMPLES.md](./PROCESS_MINING_AQL_EXAMPLES.md) for 10 complete examples.

---

### For Developers: Implementation Guide

1. **Read Research Document**
   - [PROCESS_MINING_RESEARCH_AND_ROADMAP.md](./PROCESS_MINING_RESEARCH_AND_ROADMAP.md)
   - Understand algorithms and metrics

2. **Review API Design**
   - [process_pattern_matcher.h](../../include/analytics/process_pattern_matcher.h)
   - [process_mining_functions.h](../../include/query/functions/process_mining_functions.h)

3. **Follow Implementation Roadmap**
   - Phase 2: Core Implementation
   - Phase 3: AQL Integration
   - Phase 4: Administrative Models
   - Phase 5: Testing
   - Phase 6: Optimization

4. **Run Tests**
   - Unit tests (target: 85%+ coverage)
   - Integration tests
   - Performance benchmarks

---

## 📊 Project Status

### Phase 1: Research & Design ✅ COMPLETED
**Duration**: Week 1-2  
**Status**: 100% Complete

**Deliverables**:
- [x] 50+ pages research document
- [x] 11 scientific publications referenced
- [x] API design complete
- [x] 15 AQL functions specified
- [x] 5 administrative models defined
- [x] 10 usage examples created
- [x] Architecture diagrams

### Phase 2: Core Implementation ⏳ NEXT
**Duration**: Week 3-4  
**Status**: Not Started

**Tasks**:
- [ ] Implement ProcessPatternMatcher class
- [ ] Graph similarity algorithms
- [ ] Vector similarity with embeddings
- [ ] Behavioral similarity metrics
- [ ] Unit tests

### Phase 3-6: See Roadmap
**Status**: Planned

**See**: [PROCESS_MINING_RESEARCH_AND_ROADMAP.md](./PROCESS_MINING_RESEARCH_AND_ROADMAP.md) Section 5 for full roadmap.

---

## 🔬 Scientific Foundation

### Key Publications

1. **van der Aalst, W.M.P. (2016)**  
   *Process Mining: Data Science in Action*

2. **Dijkman et al. (2011)**  
   *Similarity of Business Process Models: Metrics and Evaluation*

3. **Weidlich et al. (2011)**  
   *Behavioural Profiles for Business Process Models*

4. **Evermann et al. (2017)**  
   *Predicting Process Behaviour Using Deep Learning*

**Complete List**: See [PROCESS_MINING_RESEARCH_AND_ROADMAP.md](./PROCESS_MINING_RESEARCH_AND_ROADMAP.md) Section 7.

---

## 🏛️ Administrative Focus

### Compliance Frameworks Supported

**Building & Construction**:
- §34 BauO (Building regulations)
- Dokumentationspflicht

**Procurement**:
- GWB §119 (Public procurement)
- VOB/A (Construction contracts)

**HR & Privacy**:
- AGG (Anti-discrimination)
- DSGVO/GDPR (Data protection)

**General Administration**:
- Vier-Augen-Prinzip (Four-eyes principle)
- Haushaltsrecht (Budget law)

---

## 🎓 Use Cases

### Public Administration
- Building permit conformance checking
- Procurement process compliance (GWB)
- HR recruitment (AGG/DSGVO compliant)
- Budget planning optimization
- Document approval workflows

### Industry
- Production process optimization
- Quality assurance workflows
- Supply chain process mining

### Healthcare
- Patient pathway analysis
- Treatment process discovery
- Clinical compliance checking

---

## 📈 Performance & Scalability

### Design Targets
- **100k processes**: Similarity search < 1 second
- **1M processes**: Incremental indexing < 5 seconds
- **Cache hit rate**: > 80% for frequent patterns

### Optimization Strategies
- HNSW indexing for vector search
- Pattern caching
- Parallel processing (OpenMP)
- GPU acceleration (optional, CUDA)

---

## 🛠️ Technology Stack

### Existing Components (Leverage)
- **VectorIndex**: HNSW, GNN embeddings
- **GraphIndex**: Graph analytics
- **ProcessMining**: Alpha/Heuristic/Inductive miners

### New Components (To Be Implemented)
- **ProcessPatternMatcher**: Pattern matching engine
- **AdministrativeModelLoader**: YAML model loader
- **SimilarityComputer**: Graph/Vector/Behavioral metrics

---

## 📚 Additional Resources

### Internal Documentation
- [AQL Grammar](../../aql/AQL_GRAMMAR_EXTENDED_v1.3.1.ebnf)
- [Function Registry](../../include/query/functions/function_registry.h)
- [Graph Index](../../include/index/graph_index.h)
- [Vector Index](../../include/index/vector_index.h)

### External Resources
- [Process Mining Book (van der Aalst)](https://www.springer.com/gp/book/9783662498507)
- [ProM Tools](https://www.promtools.org/)
- [BPMN Specification](https://www.omg.org/spec/BPMN/2.0/)

---

## 🤝 Contributing

### For Questions
- See [PROCESS_MINING_AQL_EXAMPLES.md](./PROCESS_MINING_AQL_EXAMPLES.md) Troubleshooting section
- Review [PROCESS_MINING_RESEARCH_AND_ROADMAP.md](./PROCESS_MINING_RESEARCH_AND_ROADMAP.md) for technical details

### For Implementation
1. Review API design in header files
2. Follow coding standards from existing codebase
3. Write unit tests (target: 85%+ coverage)
4. Update documentation as needed

---

## 📝 Changelog

### Version 1.0 (2025-12-24)
- ✅ Complete research & design phase
- ✅ 4 documentation files created (100+ pages)
- ✅ 2 API header files designed
- ✅ 1 configuration file with 5 models
- ✅ 15 AQL functions specified
- ✅ 11 scientific publications referenced
- ✅ 10 usage examples provided

---

## 📞 Contact & Support

**Project**: ThemisDB Process Mining AQL Integration  
**Phase**: Research & Design Complete  
**Version**: 1.0  
**Date**: 2025-12-24

**Team**: ThemisDB Development Team  
**Repository**: [makr-code/ThemisDB](https://github.com/makr-code/ThemisDB)

---

## 🎯 Summary

This project delivers a **comprehensive design** for integrating process mining into AQL with:
- ✅ Scientific foundation (11 publications)
- ✅ Practical examples (10 scenarios)
- ✅ Administrative focus (5 governance models)
- ✅ Multi-model similarity (graph + vector + behavioral)
- ✅ Production-ready architecture
- ✅ Clear implementation roadmap

**Status**: Ready for implementation phase 🚀

---

**Last Updated**: 2026-04-06  
**Next Review**: Start of Implementation Phase 2
