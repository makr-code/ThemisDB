# Philosophy Profiles

This directory contains YAML-based philosophy profiles for the Ethics AI Plugin.

## Available Philosophy Schools

### Classical Philosophy Schools (10)

1. **kant.yaml** - Kantian Ethics (Deontological)
2. **utilitarianism.yaml** - Utilitarian Ethics (Consequentialist)
3. **contractualism.yaml** - Contractualist Ethics
4. **rationalism.yaml** - Rationalist Ethics
5. **socratic.yaml** - Socratic Ethics (Virtue Ethics)
6. **arendt.yaml** - Arendtian Political Philosophy
7. **dilthey.yaml** - Dilthey's Hermeneutic Philosophy
8. **marx.yaml** - Marxist Philosophy
9. **nietzsche.yaml** - Nietzschean Philosophy
10. **schopenhauer.yaml** - Schopenhauerian Philosophy

### Applied Ethics Schools (6) - NEW

11. **business_ethics.yaml** - Business and Corporate Ethics
12. **scientific_ethics.yaml** - Research and Scientific Ethics
13. **political_ethics.yaml** - Political and Governance Ethics
14. **social_ethics.yaml** - Social Welfare and Community Ethics
15. **environmental_ethics.yaml** - Environmental and Ecological Ethics
16. **technology_ethics.yaml** - Technology and AI Ethics

**Total: 16 Philosophy Schools**

## YAML Structure

Each philosophy profile contains:

```yaml
school_id: unique_identifier
name: "Display Name"
name_de: "German Name"

founders:
  - name: "Philosopher Name"
    years: "1724-1804"
    nationality: "Nationality"
    key_works:
      - title: "Work Title"
        year: YYYY
        significance: "Description"

main_theses:
  - "Core thesis 1"
  - "Core thesis 2"

secondary_theses:
  - "Supporting thesis 1"

decision_framework:
  primary_test: "Main decision principle"
  secondary_test: "Supporting principle"

strengths:
  - "Strength 1"
  - "Strength 2"

weaknesses:
  - "Weakness 1"
  - "Weakness 2"

internal_debate:
  issue_1: "Description"

philosophical_positioning:
  relation_to_school: "Description"
```

## Usage

The Philosophy Loader will automatically scan this directory and load all YAML files:

```cpp
auto result = ethics_plugin->loadPhilosophyProfiles(
    "plugins/ethics_ai/philosophies"
);

if (auto* count = std::get_if<size_t>(&result)) {
    std::cout << "Loaded " << *count << " philosophy profiles" << std::endl;
    // Expected: 16 profiles (10 classical + 6 applied)
}
```

### Domain-Specific Usage Examples

**Business Decision:**
```cpp
auto decision = ethics_plugin->makeDecision(
    "Should we outsource manufacturing to reduce costs?",
    {"business_ethics", "utilitarianism", "kant"},
    "business",
    true  // use RAG
);
```

**Scientific Research:**
```cpp
auto decision = ethics_plugin->makeDecision(
    "Should we publish dual-use research with potential weapons applications?",
    {"scientific_ethics", "kant", "utilitarian"},
    "research",
    true
);
```

**Political Policy:**
```cpp
auto decision = ethics_plugin->makeDecision(
    "Should we implement universal basic income?",
    {"political_ethics", "social_ethics", "rawls"},
    "policy",
    true
);
```

**Environmental Action:**
```cpp
auto decision = ethics_plugin->makeDecision(
    "Should we ban single-use plastics?",
    {"environmental_ethics", "utilitarianism"},
    "environment",
    true
);
```

**Technology Deployment:**
```cpp
auto decision = ethics_plugin->makeDecision(
    "Should we deploy facial recognition in public spaces?",
    {"technology_ethics", "kant", "political_ethics"},
    "technology",
    true
);
```

## Adding New Philosophies

To add a new philosophy school:

1. Create a new YAML file following the structure above
2. Choose a unique `school_id`
3. Fill in all required fields
4. Place the file in this directory
5. The plugin will automatically detect and load it

## Philosophy School Characteristics

### Classical Philosophical Approaches

#### Deontological (Duty-Based)
- **Kant**: Universal moral laws, categorical imperative, respect for persons

#### Consequentialist (Outcome-Based)
- **Utilitarianism**: Greatest happiness principle, utility maximization

#### Virtue Ethics
- **Socratic**: Knowledge as virtue, examined life, dialectical method

#### Social Contract
- **Contractualism**: Agreement-based morality, mutual benefit

#### Other Classical Approaches
- **Rationalism**: Reason-based ethics
- **Arendt**: Political philosophy, plurality, human condition
- **Marx**: Materialist ethics, social justice, class analysis
- **Nietzsche**: Will to power, beyond good and evil, revaluation
- **Schopenhauer**: Compassion-based ethics, will and representation
- **Dilthey**: Hermeneutic understanding, life philosophy

### Applied Ethics by Domain

#### Business & Economics
- **Business Ethics**: Corporate responsibility, stakeholder theory, sustainability, CSR

#### Science & Research
- **Scientific Ethics**: Research integrity, reproducibility, responsible innovation, peer review

#### Politics & Governance
- **Political Ethics**: Justice as fairness, deliberative democracy, political legitimacy, human rights

#### Society & Community
- **Social Ethics**: Social solidarity, capability approach, collective responsibility, welfare

#### Environment & Sustainability
- **Environmental Ethics**: Land ethic, precautionary principle, intergenerational justice, intrinsic value

#### Technology & AI
- **Technology Ethics**: Algorithmic fairness, digital rights, privacy, value-sensitive design

## Language Support

All profiles include both English and German content:
- `name`: English name
- `name_de`: German name
- Bilingual descriptions where applicable

## Source

These philosophy profiles were originally developed for the ThemisDB Moral Philosophy Debates example (examples/24_moral_philosophy_debates/) and have been integrated into the Ethics AI Plugin for production use.
