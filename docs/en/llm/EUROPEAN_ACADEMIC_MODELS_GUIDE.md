# European and Academic Model Configurations - Quick Reference

## Overview

Two new comprehensive YAML configurations have been added to support specialized model deployments for European languages and academic research.

## Files Added

### 1. `config/llm_deployment_european.yaml` (13KB)

**Purpose**: European language models with GDPR compliance

**Models Included**:
- **Multilingual**: Mistral 7B, Mixtral 8x7B (14 EU languages)
- **German**: LeoLM (Hessian.AI), DiscoLM (DFKI)
- **French**: Vigogne, CroissantLLM (CNRS)
- **Spanish**: Aguila (Projecte AINA, includes Catalan)
- **Italian**: Camoscio
- **Nordic**: ScandEval (Swedish, Danish, Norwegian, Finnish)
- **Eastern European**: Polish, Czech models
- **Embeddings**: Multilingual E5 for 12 EU languages

**Key Features**:
- GDPR, BDSG, CNIL compliance settings
- European data residency configuration
- Data retention policies (90-180 days)
- European LoRAs (legal, medical, business)
- Data center configuration (Frankfurt, Paris, Milan)

**Use Cases**:
- German legal documents (BDSG compliant)
- French medical terminology
- Spanish/Catalan business communications
- Nordic government services
- EU-wide multilingual support

### 2. `config/llm_deployment_academic.yaml` (20KB)

**Purpose**: Academic and research models for universities

**Models Included**:

**By Institution**:
- **Stanford**: Alpaca (instruction-following)
- **UC Berkeley/CMU/Stanford/UCSD**: Vicuna 7B & 13B (chatbots)
- **UC Berkeley BAIR**: Koala 13B (dialogue)
- **NYU/Meta AI**: Galactica 6.7B & 30B (scientific knowledge)
- **EleutherAI**: Pythia 6.9B & 12B, GPT-NeoX 20B (interpretability)
- **BigScience**: BLOOM 7B, BLOOMZ 7B (46 languages, 1000+ researchers)
- **Microsoft Research**: BioGPT (biomedical), WizardLM/Coder/Math
- **Allen AI**: SciBERT (scientific embeddings)
- **Technology Innovation Institute (UAE)**: Falcon 7B & 40B
- **MosaicML/Databricks**: MPT 7B, Dolly v2 12B

**By Domain**:
- **Scientific**: Galactica (LaTeX, chemistry, math, physics)
- **Biomedical**: BioGPT (PubMed, drug discovery)
- **Code**: WizardCoder, CodeLlama derivatives
- **Mathematics**: WizardMath (GSM8K, MATH benchmarks)
- **General**: Vicuna, Alpaca, BLOOM

**Research Features**:
- Reproducibility tracking (seed, parameters, versions)
- Standard benchmarks (MMLU, HellaSwag, MT-Bench, HumanEval)
- BibTeX generation and citation tracking
- HPC integration (SLURM, PBS)
- Multi-institution collaboration support
- License compliance enforcement

**Academic LoRAs**:
- Scientific paper writing (LaTeX generation)
- Medical research literature
- Mathematical proof generation

## Usage Examples

### European Models

```yaml
# Deploy German legal model
models:
  leo-mistral-7b-hessianai:
    sources:
      ollama: leo-mistral:7b-hessianai
    specialization:
      domains:
        - german_law
        - german_business
    deployment:
      gdpr_compliant: true
```

### Academic Models

```yaml
# Deploy scientific model for research
models:
  galactica-6.7b:
    institution: Meta AI Research / NYU
    research_paper: "Galactica: A Large Language Model for Science"
    specialization:
      - LaTeX generation
      - chemical formulas
      - mathematical reasoning
    academic_use:
      citation_required: true
```

## Integration

Both configurations work seamlessly with the LLM deployment plugin:

```cpp
// Load European models
auto eu_config = LLMDeploymentPlugin::loadConfigFromYAML(
    "config/llm_deployment_european.yaml"
);

// Load academic models
auto academic_config = LLMDeploymentPlugin::loadConfigFromYAML(
    "config/llm_deployment_academic.yaml"
);
```

## Compliance & Licensing

### European Models
- **GDPR**: Full compliance with data retention, right to deletion
- **BDSG**: German data protection compliance
- **CNIL**: French data protection compliance
- **Schrems II**: Standard contractual clauses

### Academic Models
- **Open Research**: Apache 2.0, MIT, CC BY-NC 4.0
- **Citation Tracking**: Research papers and BibTeX
- **Commercial Use**: Clearly marked for each model
- **Reproducibility**: Full parameter and version logging

## Model Selection Guide

### Choose European Configuration If:
- Operating in EU with GDPR requirements
- Need multilingual European language support
- Require European data residency
- Working with German, French, Spanish, Italian, Nordic, or Eastern European languages
- Need EU-specific legal, medical, or business models

### Choose Academic Configuration If:
- Research or educational institution
- Need reproducibility and citation tracking
- Working on scientific/biomedical research
- Require specific research models (Galactica, Pythia, BLOOM)
- Need HPC cluster integration
- Working on interpretability or training dynamics research
- Require mathematical or code generation models

## Quick Start

### European Deployment
```bash
# Deploy Mistral for multilingual EU support
themis-cli deploy mistral-7b-instruct-eu --config llm_deployment_european.yaml

# Deploy German legal model
themis-cli deploy leo-mistral-7b-hessianai --config llm_deployment_european.yaml
```

### Academic Deployment
```bash
# Deploy scientific model
themis-cli deploy galactica-6.7b --config llm_deployment_academic.yaml

# Deploy research chatbot
themis-cli deploy vicuna-7b-v1.5 --config llm_deployment_academic.yaml
```

## Model Counts

- **European Models**: 16 primary models + 3 LoRAs
- **Academic Models**: 25 primary models + 3 specialized LoRAs
- **Total Languages**: 46+ languages covered
- **Institutions**: 20+ universities and research organizations

## Further Reading

- Main Documentation: `docs/en/llm/llm_deployment_plugin.md`
- General Configuration: `config/llm_deployment.example.yaml`
- Implementation Summary: `LLM_DEPLOYMENT_PLUGIN_SUMMARY.md`
