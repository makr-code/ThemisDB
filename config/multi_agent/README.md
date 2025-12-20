# Multi-Agent Configuration Examples

This directory contains example configurations for ThemisDB Multi-Agent LLM Reasoning (v1.4.0).

## Available Configurations

### 1. Legal Contract Analysis (`legal_contract_analysis.yaml`)

**Strategy:** PARALLEL  
**Use Case:** Comprehensive contract review from multiple perspectives

**Agents:**
- Legal Risk Analyst (legal_contracts_v2 LoRA)
- Compliance Validator (gdpr_compliance_v1 LoRA)
- Business Terms Expert (business_contracts_v1 LoRA)

**Consensus:** SYNTHESIZE (meta-agent combines all perspectives)

**Example Usage:**
```bash
curl -X POST http://localhost:8765/api/llm/multi-agent/analyze \
  -H "Content-Type: application/json" \
  -d '{
    "config": "legal_contract_analysis",
    "input": "Analyze this SaaS contract for legal risks, compliance, and business terms..."
  }'
```

### 2. Code Review (`code_review.yaml`)

**Strategy:** SEQUENTIAL  
**Use Case:** Multi-perspective code review prioritizing security

**Agents:**
- Security Analyst (security_patterns_v3 LoRA) - 50% weight
- Performance Analyst (performance_optimization_v2 LoRA) - 30% weight
- Code Quality Expert (code_quality_v1 LoRA) - 20% weight

**Consensus:** WEIGHTED_AVERAGE (security most important)

**Example Usage:**
```bash
curl -X POST http://localhost:8765/api/llm/multi-agent/review \
  -H "Content-Type: application/json" \
  -d '{
    "config": "code_review",
    "input": "Review this authentication implementation...",
    "context": {
      "language": "python",
      "framework": "flask"
    }
  }'
```

### 3. Research Assistant (`research_assistant.yaml`)

**Strategy:** HIERARCHICAL  
**Use Case:** Academic research with coordinated specialist agents

**Agents:**
- Literature Analyst (academic_papers_v1 LoRA)
- Methodology Designer (research_methods_v1 LoRA)
- Statistical Analyst (statistics_v2 LoRA)
- Research Coordinator (academic_synthesis_v1 LoRA) - meta-agent

**Consensus:** HIERARCHICAL (research coordinator makes final synthesis)

**Example Usage:**
```bash
curl -X POST http://localhost:8765/api/llm/multi-agent/research \
  -H "Content-Type: application/json" \
  -d '{
    "config": "research_assistant",
    "input": "Conduct literature review on AI applications in healthcare...",
    "context": {
      "domain": "healthcare",
      "timeframe": "2020-2024"
    }
  }'
```

## Configuration Format

All configurations follow this YAML structure:

```yaml
orchestrator:
  name: "Descriptive Name"
  strategy: PARALLEL | SEQUENTIAL | HIERARCHICAL
  
agents:
  - id: "unique_agent_id"
    role: "role_identifier"
    base_model: "model_name"
    lora_adapter: "adapter_id"
    max_context_length: 4096
    temperature: 0.7
    system_prompt: |
      Agent instructions...

consensus:
  strategy: MAJORITY_VOTE | WEIGHTED_AVERAGE | BEST_RESPONSE | SYNTHESIZE | HIERARCHICAL
  confidence_threshold: 0.75
  require_unanimity: false
  meta_agent_id: "agent_id"  # For HIERARCHICAL/SYNTHESIZE
  role_weights:  # For WEIGHTED_AVERAGE
    role_name: 0.5
```

## Strategy Types

### Orchestrator Strategies

1. **PARALLEL** - All agents work simultaneously
   - Fastest for independent tasks
   - Best when multiple perspectives needed

2. **SEQUENTIAL** - Agents work one after another
   - Later agents can see earlier results
   - Good for refinement workflows

3. **HIERARCHICAL** - Meta-agent coordinates sub-agents
   - Best for complex multi-step tasks
   - Enables dynamic task decomposition

### Consensus Strategies

1. **MAJORITY_VOTE** - Democratic voting
   - Groups similar responses
   - Winner is most common response

2. **WEIGHTED_AVERAGE** - Confidence-weighted
   - Higher confidence = more influence
   - Can apply role-specific weights

3. **BEST_RESPONSE** - Highest confidence wins
   - Simple and fast
   - Good when one agent is clearly expert

4. **SYNTHESIZE** - Meta-agent combines all
   - Most comprehensive
   - Requires additional LLM call

5. **HIERARCHICAL** - Meta-agent decides
   - Similar to SYNTHESIZE
   - Allows for iterative refinement

## Creating Custom Configurations

To create your own configuration:

1. Copy an existing example
2. Define your agents with appropriate roles
3. Specify LoRA adapters for specialization
4. Choose orchestrator and consensus strategies
5. Save as `your_config_name.yaml`
6. Use via API: `"config": "your_config_name"`

## LoRA Adapters

LoRA adapters enable domain specialization without full model retraining:

- **Legal:** `legal_contracts_v2`, `gdpr_compliance_v1`
- **Technical:** `tech_analysis_v1`, `security_patterns_v3`, `performance_optimization_v2`
- **Business:** `business_analysis_v1`, `business_contracts_v1`
- **Academic:** `academic_papers_v1`, `research_methods_v1`, `statistics_v2`
- **Code:** `code_quality_v1`, `security_patterns_v3`

To register custom LoRA adapters, use the LoRA Registry API.

## Documentation

For complete documentation, see:
- [Multi-Agent Reasoning Concept](../../docs/llm/MULTI_AGENT_REASONING_CONCEPT.md)
- [LLM Integration Overview](../../docs/llm/README.md)

## Version

**Framework Version:** v1.4.0  
**Status:** Core framework implemented  
**Full LLM Integration:** v1.5.0 (Q3 2026)
