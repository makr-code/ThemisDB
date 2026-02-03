# AI Review Template Selection Guide

## Overview

ThemisDB has multiple AI review templates optimized for different review types. This guide helps you select the right template(s) for your review needs.

---

## Quick Selection

| Review Type | Template | When to Use |
|------------|----------|-------------|
| **General Component** | `ai-review-component-template.md` | Any component needing comprehensive review |
| **Testing & QA** | `ai-review-testing-quality.md` | Test coverage, quality assurance, flaky tests |
| **LLM/AI Components** | `ai-review-llm-components.md` | LLM engine, embeddings, RAG, vector search |
| **Distributed Systems** | `ai-review-distributed-systems.md` | Sharding, replication, consensus, CDC |
| **Migration Planning** | `ai-review-migration-planning.md` | Major refactorings, library upgrades, breaking changes |
| **Cost Optimization** | `ai-review-cost-optimization.md` | Infrastructure costs, resource efficiency |

---

## Detailed Template Descriptions

### 1. Component Template (Base Template)
**File:** `ai-review-component-template.md`
**Best For:** General-purpose component reviews

**Use When:**
- Reviewing any component (storage, query, API, etc.)
- Need comprehensive analysis across multiple dimensions
- First-time review of a component
- Quarterly deep-dive reviews

**Covers:**
- ✅ Code quality and best practices
- ✅ Research paper comparison
- ✅ Documentation verification
- ✅ Security and compliance
- ✅ Performance analysis
- ✅ Roadmap derivation

**Output:**
- 5-10 research papers cited
- Competitive system comparison
- Prioritized roadmap with action items
- Detailed findings with evidence

**Example Use Cases:**
- "Review the storage layer"
- "Analyze query engine performance and design"
- "Audit API layer security and documentation"

---

### 2. Testing & Quality Assurance Template
**File:** `ai-review-testing-quality.md`
**Best For:** Test coverage and quality reviews

**Use When:**
- Need to improve test coverage
- Investigating flaky tests
- Pre-release quality gate
- Quarterly QA audits

**Covers:**
- ✅ Coverage metrics (line, branch, function)
- ✅ Test type inventory (unit, integration, e2e)
- ✅ Test quality assessment
- ✅ Flaky test identification
- ✅ Performance test analysis
- ✅ Security/fuzz testing coverage

**Output:**
- Exact coverage percentages per component
- List of untested functions with file:line
- Flaky test root causes
- Prioritized test gap action items
- Coverage improvement targets

**Example Use Cases:**
- "Why is test coverage low for query engine?"
- "Identify and fix flaky tests"
- "Pre-release QA review for v1.5"

---

### 3. LLM/AI Components Template
**File:** `ai-review-llm-components.md`
**Best For:** AI/ML component reviews

**Use When:**
- Reviewing LLM engine integration
- Evaluating vector search performance
- Auditing RAG pipeline
- Assessing AI ethics/safety

**Covers:**
- ✅ Model evaluation (tokens/sec, perplexity)
- ✅ Vector search benchmarks (recall@k)
- ✅ RAG quality metrics
- ✅ OWASP LLM Top 10 security checks
- ✅ Bias and fairness assessment
- ✅ Recent AI research (last 3 years)

**Output:**
- Performance benchmarks across model sizes
- Vector search recall at scale
- Prompt injection vulnerabilities
- Research paper comparison (Flash Attention, GPTQ, etc.)
- Ethics and safety gaps

**Example Use Cases:**
- "Evaluate llama.cpp integration performance"
- "Audit vector search for production readiness"
- "Check RAG pipeline for hallucination issues"

---

### 4. Distributed Systems Template
**File:** `ai-review-distributed-systems.md`
**Best For:** Distributed system component reviews

**Use When:**
- Reviewing consensus implementation
- Auditing replication strategy
- Evaluating sharding design
- Testing failure modes

**Covers:**
- ✅ Raft/Paxos correctness
- ✅ CAP theorem positioning
- ✅ Replication lag measurement
- ✅ Failure mode testing (Jepsen-style)
- ✅ Cross-shard operations
- ✅ Comparison with Spanner, CockroachDB, etc.

**Output:**
- Consensus implementation gaps vs papers
- CAP trade-off analysis
- Failure scenario test results
- Performance at scale (1 node vs 5 nodes)
- Research paper citations (Raft, 2PC, etc.)

**Example Use Cases:**
- "Verify Raft implementation correctness"
- "Test distributed system failure scenarios"
- "Plan sharding strategy for v2.0"

---

### 5. Migration Planning Template
**File:** `ai-review-migration-planning.md`
**Best For:** Major migration projects

**Use When:**
- Planning library upgrades (e.g., RocksDB 7→8)
- Major refactorings (e.g., error handling migration)
- Breaking API changes
- Architecture changes

**Covers:**
- ✅ Impact quantification (files, LOC affected)
- ✅ Risk assessment with mitigation
- ✅ Phased rollout strategy
- ✅ Rollback procedures
- ✅ Testing strategy
- ✅ Historical migration analysis

**Output:**
- Exact file/LOC count affected
- Phase-by-phase plan with timelines
- Rollback procedure with time estimates
- Risk matrix with mitigations
- Lessons from similar migrations

**Example Use Cases:**
- "Plan migration from Error to Result<T>"
- "Upgrade RocksDB from 7.10 to 8.1"
- "Migrate from libcurl to C++23 networking"

---

### 6. Cost Optimization Template
**File:** `ai-review-cost-optimization.md`
**Best For:** Infrastructure cost reviews

**Use When:**
- Quarterly cost reviews
- Budget overruns
- Pre-scaling analysis
- FinOps initiatives

**Covers:**
- ✅ Current cost breakdown by service
- ✅ Resource utilization metrics
- ✅ Right-sizing opportunities
- ✅ Reserved vs on-demand analysis
- ✅ Quantified savings per action
- ✅ Quick wins vs long-term optimizations

**Output:**
- $ costs per category
- Resource utilization percentages
- Specific savings calculations
- Prioritized by ROI (savings/effort)
- Implementation timeline

**Example Use Cases:**
- "Reduce monthly cloud spend by 20%"
- "Optimize GPU utilization"
- "Evaluate reserved instance strategy"

---

## Combining Templates

### Scenario 1: New Feature Complete Review
**Goal:** Comprehensive review of newly completed feature

**Templates to Use:**
1. **Component Template** (primary) - Overall design, implementation, research
2. **Testing Template** - Ensure adequate test coverage
3. **Specialized Template** - If feature is AI/distributed/etc.

**Process:**
```bash
# 1. Component review
gh issue create --template ai-review-component-template.md \
  --title "[COMPONENT-REVIEW] New Feature X - Q1 2026"

# 2. Testing review  
gh issue create --template ai-review-testing-quality.md \
  --title "[TEST-REVIEW] Feature X Test Coverage"
```

---

### Scenario 2: Pre-Release Audit
**Goal:** Ensure release readiness

**Templates to Use:**
1. **Testing Template** - Verify coverage meets thresholds
2. **Component Template** - Security and performance audit
3. **Migration Template** - If breaking changes

**Process:**
```bash
# 1. Test coverage gate
gh issue create --template ai-review-testing-quality.md \
  --title "[TEST-REVIEW] v1.5.0 Pre-Release QA"

# 2. Security/performance check
gh issue create --template ai-review-component-template.md \
  --title "[COMPONENT-REVIEW] v1.5.0 Security Audit"

# 3. If breaking changes
gh issue create --template ai-review-migration-planning.md \
  --title "[MIGRATION-REVIEW] v1.5.0 Breaking Changes"
```

---

### Scenario 3: Performance Problem Investigation
**Goal:** Identify and fix performance bottleneck

**Templates to Use:**
1. **Component Template** (focused on performance sections)
2. **Testing Template** (check performance test coverage)
3. **Cost Optimization** (if cloud resources involved)

**Process:**
```bash
# 1. Deep-dive component analysis
gh issue create --template ai-review-component-template.md \
  --title "[COMPONENT-REVIEW] Query Engine Performance Investigation"

# 2. Ensure benchmarks exist
gh issue create --template ai-review-testing-quality.md \
  --title "[TEST-REVIEW] Query Engine Performance Tests"
```

---

### Scenario 4: AI Feature Development
**Goal:** Build new AI/LLM feature

**Templates to Use:**
1. **LLM Components Template** (primary) - AI-specific review
2. **Testing Template** - AI quality metrics (BLEU, ROUGE, etc.)
3. **Component Template** - Integration with rest of system

**Process:**
```bash
# 1. AI-specific review
gh issue create --template ai-review-llm-components.md \
  --title "[AI-REVIEW] New RAG Pipeline Implementation"

# 2. AI quality testing
gh issue create --template ai-review-testing-quality.md \
  --title "[TEST-REVIEW] RAG Pipeline Quality Metrics"
```

---

## Template Customization

### When to Customize
- Unique component type not covered by templates
- Specific regulatory requirements
- Domain-specific checklist items

### How to Customize
1. Start with closest template (usually component template)
2. Add domain-specific sections
3. Keep AI Agent Guidance structure
4. Maintain output format standards
5. Document customizations in comments

### Example Customization
```markdown
<!-- Custom section for hardware acceleration review -->
## 🚀 Hardware Acceleration Analysis

### CUDA Kernel Optimization
- [ ] Occupancy analysis
- [ ] Memory coalescing
- [ ] Bank conflicts identified
[... custom checks ...]
```

---

## Best Practices

### For AI Agents
1. **Read the guide first**: Always review `AI_AGENT_REVIEW_GUIDE.md` before starting
2. **Follow commands**: Execute all bash commands in "Initial Analysis Commands" section
3. **Be specific**: Every finding needs file:line + evidence + metrics
4. **Cite research**: Minimum 5-10 papers with DOIs
5. **Test examples**: Compile and run all documentation examples
6. **Quantify impact**: Use numbers (%, ops/sec, MB, days)

### For Human Reviewers
1. **Choose right template**: Use this guide to select appropriate template
2. **Fill completely**: Don't skip sections (write "N/A" with reason if truly not applicable)
3. **Provide evidence**: Link to files, commits, issues, benchmarks
4. **Action-oriented**: Every finding should have a clear next step
5. **Follow up**: Create actual issues for action items

---

## Template Evolution

These templates are living documents. As we learn from reviews:

### Feedback Loop
1. Complete review using template
2. Note what worked / didn't work
3. Suggest improvements via PR
4. Update templates quarterly

### Improvement Areas
- Add new domain-specific templates
- Refine AI agent guidance based on output quality
- Add more examples to guides
- Update research paper recommendations

---

## FAQ

**Q: Which template for a new component?**
A: Start with Component Template for comprehensive review.

**Q: Can I combine templates in one issue?**
A: No, use separate issues for better tracking. Link related issues.

**Q: How long should a review take?**
A: Varies by scope:
- Focused review (1 component): 2-4 hours
- Comprehensive review: 1-2 days
- Full system audit: 1-2 weeks

**Q: What if component doesn't fit any template?**
A: Use Component Template (most general) and customize as needed.

**Q: Do I need all sections filled?**
A: Yes, write "N/A" with brief reason if section doesn't apply.

**Q: Can AI agents use these templates?**
A: Yes! Templates are optimized for AI with specific guidance sections.

---

## Examples

See `EXAMPLE_COMPONENT_REVIEW.md` for a complete filled-out example showing:
- Proper finding format with evidence
- Research paper citations
- Documentation verification
- Roadmap with priorities
- All quality standards met

---

**Last Updated:** 2026-02-03
**Maintained by:** ThemisDB Core Team
