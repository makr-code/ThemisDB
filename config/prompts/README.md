# ThemisDB Prompt Templates

This directory contains domain-specific prompt templates for use with ThemisDB's LLM integration system.

## Overview

The prompt templates are organized into separate YAML files by domain, making it easy to load and manage prompts for specific use cases.

## Available Prompt Files

### 1. **standard_prompts.yaml** - General Purpose Queries
Default templates for general-purpose operations that don't require specialized domain knowledge.

**Prompts included:**
- `standard_query` - General information retrieval
- `information_retrieval` - Specific information lookup
- `data_summary` - Data summarization
- `comparison_query` - Compare data points
- `aggregation_query` - Aggregate and analyze
- `search_query` - Search operations

**Use cases:** Standard database queries, general information requests, data exploration

---

### 2. **scientific_prompts.yaml** - Scientific Research Methods
Templates for scientific analysis, research methodologies, and academic work.

**Prompts included:**
- `scientific_genesis` - Origin and development analysis
- `scientific_synthesis` - Synthesize multiple sources
- `scientific_hypothesis` - Hypothesis testing
- `scientific_analysis` - Rigorous data analysis
- `literature_review` - Scientific literature review
- `experimental_design` - Design experiments
- `peer_review_analysis` - Critical analysis

**Use cases:** Research papers, hypothesis testing, experimental design, peer review

---

### 3. **legal_prompts.yaml** - Legal Analysis
Templates for legal queries, contract analysis, and compliance checking.

**Prompts included:**
- `legal_analysis` - General legal analysis
- `contract_review` - Contract review and analysis
- `compliance_check` - Regulatory compliance
- `legal_research` - Legal research and precedents
- `risk_assessment` - Legal risk assessment
- `due_diligence` - Legal due diligence
- `policy_interpretation` - Policy interpretation

**Use cases:** Contract review, compliance checking, legal research, risk assessment

**Note:** These templates provide analysis only, not legal advice. Always consult qualified legal counsel.

---

### 3b. **administrative_law_prompts.yaml** - Administrative Law (Verwaltungsrecht)
German administrative law templates including environmental law and administrative procedures.

**Prompts included:**
- `verwaltungsrecht_analysis` - Administrative law analysis
- `verwaltungsverfahren` - Administrative procedure (VwVfG)
- `bimschg_immissionsschutz` - Federal Immission Control Act
- `ta_laerm_noise_analysis` - TA Lärm noise analysis
- `genehmigungsverfahren` - Licensing/approval procedures
- `umweltrecht_compliance` - Environmental law compliance
- `verwaltungsakt_pruefung` - Administrative act review
- `baurecht_genehmigung` - Building law and construction permits

**Use cases:** Administrative decisions, environmental permits (BImSchG, TA Lärm), building permits, compliance checking, administrative procedures

**Topics covered:** Verwaltungsrecht, Verwaltungsverfahren, Immissionsschutzrecht, BImSchG, TA Lärm, Umweltrecht, Baurecht, Genehmigungsverfahren

**Note:** Diese Templates bieten nur Analysen, keine Rechtsberatung. Konsultieren Sie immer qualifizierte Rechtsanwälte.

---

### 4. **technical_prompts.yaml** - Technical Engineering
Templates for technical queries, system architecture, and engineering problems.

**Prompts included:**
- `technical_architecture` - Architecture analysis
- `debugging_analysis` - Debug technical issues
- `performance_optimization` - Optimize performance
- `code_review` - Code quality review
- `system_design` - Design systems
- `technical_documentation` - Create documentation
- `security_analysis` - Security assessment
- `api_design` - API design and review

**Use cases:** System design, debugging, performance tuning, code review, documentation

---

### 5. **economic_prompts.yaml** - Economic & Business Analysis
Templates for business analysis, financial queries, and market research.

**Prompts included:**
- `business_analysis` - Business metrics and KPIs
- `financial_analysis` - Financial performance
- `market_research` - Market analysis
- `economic_forecast` - Economic forecasting
- `cost_benefit_analysis` - Cost-benefit analysis
- `pricing_strategy` - Pricing strategy
- `roi_analysis` - Return on investment
- `competitive_analysis` - Competitive landscape

**Use cases:** Business planning, financial analysis, market research, investment decisions

---

### 6. **mathematical_prompts.yaml** - Mathematical Analysis
Templates for mathematical problems, statistics, and numerical methods.

**Prompts included:**
- `mathematical_analysis` - General math problems
- `statistical_analysis` - Statistical analysis
- `optimization_problem` - Optimization
- `probability_calculation` - Probability
- `numerical_methods` - Numerical methods
- `linear_algebra` - Linear algebra
- `calculus_problem` - Calculus
- `combinatorics` - Combinatorics

**Use cases:** Statistical analysis, mathematical modeling, optimization, numerical computation

---

### 7. **geographic_prompts.yaml** - Geographic & Spatial Analysis
Templates for geographic queries, spatial analysis, and GIS operations.

**Prompts included:**
- `location_query` - Location information
- `spatial_analysis` - Spatial analysis
- `distance_calculation` - Distance calculations
- `region_analysis` - Regional analysis
- `map_generation` - Map descriptions
- `route_planning` - Route planning
- `geospatial_statistics` - Geospatial statistics
- `geocoding` - Address/coordinate conversion

**Use cases:** GIS operations, spatial analysis, route planning, location services

---

## Usage

### Loading Prompts with PromptManager

```cpp
#include "prompt_engineering/prompt_manager.h"

// Create prompt manager
auto prompt_manager = std::make_shared<themis::prompt_engineering::PromptManager>();

// Load specific domain prompts
prompt_manager->loadFromYAML("config/prompts/standard_prompts.yaml");
prompt_manager->loadFromYAML("config/prompts/scientific_prompts.yaml");
prompt_manager->loadFromYAML("config/prompts/legal_prompts.yaml");

// Or load all prompts
for (const auto& file : {
    "standard_prompts.yaml",
    "scientific_prompts.yaml", 
    "legal_prompts.yaml",
    "administrative_law_prompts.yaml",
    "technical_prompts.yaml",
    "economic_prompts.yaml",
    "mathematical_prompts.yaml",
    "geographic_prompts.yaml"
}) {
    prompt_manager->loadFromYAML("config/prompts/" + std::string(file));
}
```

### Using Prompts with Context

```cpp
// Build context from your data
std::unordered_map<std::string, std::string> context = {
    {"query", "What is the average temperature?"},
    {"context", temperature_data},
    {"version", "1.5.0"},
    {"table_count", "10"}
};

// Get prompt with context injection
auto prompt = prompt_manager->getPromptWithContext("statistical_analysis", context);

// Use with LLM
auto response = llm->generate(prompt.value());
```

## YAML Structure

Each prompt file follows this structure:

```yaml
version: "1.0.0"

prompts:
  prompt_id:
    name: "Human Readable Name"
    version: "1.0"
    description: "What this prompt does"
    active: true
    ethics_flag: "low|moderate|high"  # NEW: Ethics consideration level
    ethics_considerations:            # NEW: Specific ethical concerns
      - "concern_1"
      - "concern_2"
    ethics_note: "When to consult ethics module"  # NEW: Guidance
    content: |
      Template content with {variable} placeholders.
      
      The content uses template variables like:
      - {query} - User's query
      - {context} - Additional context data
      - {version} - Database version
      - {table_count} - Number of tables
      
      Variables are replaced at runtime using PromptManager.

metadata:
  created: "2026-02-10"
  author: "ThemisDB Development Team"
  purpose: "Description of file purpose"
  category: "domain"
```

## Ethics Integration (NEW)

Many prompts now include ethics metadata to flag potential ethical concerns and guide integration with ThemisDB's ethics module.

### Ethics Fields

- **`ethics_flag`**: Indicates the level of ethical consideration needed
  - `low`: Minimal ethical concerns
  - `moderate`: Some ethical considerations
  - `high`: Significant ethical implications requiring careful review

- **`ethics_considerations`**: List of specific ethical concerns that may arise
  - Examples: `fairness_justice`, `human_subjects_protection`, `environmental_protection`, `power_imbalance`

- **`ethics_note`**: Guidance on when and how to consult the ethics module

### When Ethics Flags Are Triggered

Prompts with `ethics_flag: "moderate"` or `"high"` should trigger consultation with ThemisDB's ethics module when:

1. The query involves conflicting stakeholder interests
2. Human subjects, animals, or vulnerable populations are affected
3. Environmental impacts or intergenerational justice are at stake
4. Power imbalances or procedural fairness concerns exist
5. Risk-benefit trade-offs must be evaluated

### Example: Ethics-Aware Prompt Usage

```cpp
auto prompt_result = prompt_manager->getPromptWithContext("bimschg_immissionsschutz", context);

// Check ethics flag
if (prompt_result.ethics_flag == "high") {
    // Consult ethics module
    auto ethics_assessment = ethics_module->assess({
        {"scenario", context["query"]},
        {"stakeholders", "facility_operator,neighbors,environment"},
        {"considerations", prompt_result.ethics_considerations}
    });
    
    // Enhance prompt with ethical guidance
    if (ethics_assessment.has_conflict) {
        context["ethical_guidance"] = ethics_assessment.recommendation;
    }
}

auto response = llm->generate(prompt_result.content);
```

### Prompts with Ethics Integration

**High Priority (ethics_flag: "high"):**
- `risk_assessment` - Risk distribution and stakeholder harm
- `bimschg_immissionsschutz` - Environmental vs. economic interests
- `ta_laerm_noise_analysis` - Quality of life vs. economic use
- `umweltrecht_compliance` - Sustainability and future generations
- `experimental_design` - Human/animal subjects protection
- `verwaltungsrecht_analysis` - Fundamental rights and public interest

**Moderate Priority (ethics_flag: "moderate"):**
- Legal and administrative prompts involving fairness, transparency, proportionality
- Scientific prompts involving research integrity and objectivity

### Ethics Module Integration

The ethics flags enable seamless integration with ThemisDB's ethics evaluation system (see `examples/24_moral_philosophy_debates/`):

1. **Automatic Detection**: System detects high-ethics prompts
2. **Ethical Assessment**: Evaluates stakeholder impacts and ethical principles
3. **Guidance Generation**: Provides recommendations for ethical decision-making
4. **Audit Trail**: Documents ethical considerations in prompt execution

This ensures that ThemisDB operations consider ethical implications systematically, not as an afterthought.

---

## Template Variables

Common variables available in prompts:

- `{query}` - User's query or question
- `{context}` - Additional context or data
- `{version}` - Database version
- `{edition}` - Database edition (Community, Enterprise)
- `{table_count}` - Number of tables
- `{total_rows}` - Total number of rows
- `{tables}` - JSON list of tables
- `{schema}` - Complete schema as JSON
- `{capabilities}` - Enabled features

## Creating Custom Prompts

1. Choose the appropriate file or create a new one
2. Follow the YAML structure
3. Use descriptive IDs and names
4. Include clear descriptions
5. Use template variables for dynamic content
6. Test with PromptManager

## Best Practices

1. **Use Specific Prompts**: Choose the most specific prompt for your use case
2. **Provide Context**: Include relevant data in the context map
3. **Variable Naming**: Use clear, consistent variable names
4. **Version Control**: Track prompt versions for reproducibility
5. **Test Thoroughly**: Test prompts with various inputs
6. **Document Changes**: Update descriptions when modifying prompts

## Integration with Prompt Engineering System

These prompts integrate with ThemisDB's complete prompt engineering system:

- **PromptManager** - Load and manage templates
- **PromptOptimizer** - Automatically improve prompts
- **PromptPerformanceTracker** - Track effectiveness
- **FeedbackCollector** - Gather feedback
- **PromptVersionControl** - Version management
- **SelfImprovementOrchestrator** - Autonomous optimization

## Statistics

**Total Prompt Files**: 8
**Total Prompts**: 61+
**Domains Covered**:
- Standard (6 prompts)
- Scientific (7 prompts)
- Legal (7 prompts)
- Administrative Law (8 prompts) - NEW!
- Technical (8 prompts)
- Economic (8 prompts)
- Mathematical (8 prompts)
- Geographic (8 prompts)

**Special Coverage**:
- German Administrative Law (Verwaltungsrecht)
- Environmental Law (BImSchG, TA Lärm, Umweltrecht)
- Building Law (Baurecht)
- Administrative Procedures (VwVfG, Genehmigungsverfahren)

## Contributing

To add new prompts:

1. Choose appropriate domain file (or create new)
2. Follow existing structure and style
3. Include comprehensive descriptions
4. Add examples in comments
5. Test with PromptManager
6. Update this README

## License

SPDX-License-Identifier: Apache-2.0
Copyright (c) 2026 ThemisDB Contributors
