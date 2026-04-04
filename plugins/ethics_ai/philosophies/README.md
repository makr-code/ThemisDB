# Philosophy Profiles

This directory contains YAML-based philosophy profiles for the Ethics AI Plugin.

## Available Philosophy Schools

### Classical Philosophy Schools (10)

1. **kant.yaml** - Immanuel Kant: Kantian Ethics (Deontological)
2. **utilitarianism.yaml** - Mill/Bentham: Utilitarian Ethics (Consequentialist)
3. **contractualism.yaml** - Rawls/Scanlon: Contractualist Ethics
4. **rationalism.yaml** - Rationalist Ethics
5. **socratic.yaml** - Socrates: Socratic Ethics (Virtue Ethics)
6. **arendt.yaml** - Hannah Arendt: Political Philosophy
7. **dilthey.yaml** - Wilhelm Dilthey: Hermeneutic Philosophy
8. **marx.yaml** - Karl Marx: Marxist Philosophy
9. **nietzsche.yaml** - Friedrich Nietzsche: Nietzschean Philosophy
10. **schopenhauer.yaml** - Arthur Schopenhauer: Schopenhauerian Philosophy

### Applied Ethics Schools (6) - NEW

11. **adam_smith.yaml** - Adam Smith: Business and Corporate Ethics
12. **merton.yaml** - Robert K. Merton: Research and Scientific Ethics
13. **rawls.yaml** - John Rawls: Political and Governance Ethics
14. **durkheim.yaml** - Émile Durkheim: Social Welfare and Community Ethics
15. **leopold.yaml** - Aldo Leopold: Environmental and Ecological Ethics
16. **wiener.yaml** - Norbert Wiener: Technology and AI Ethics

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
    {"adam_smith", "utilitarianism", "kant"},
    "business",
    true  // use RAG
);
```

**Scientific Research:**
```cpp
auto decision = ethics_plugin->makeDecision(
    "Should we publish dual-use research with potential weapons applications?",
    {"merton", "kant", "utilitarianism"},
    "research",
    true
);
```

**Political Policy:**
```cpp
auto decision = ethics_plugin->makeDecision(
    "Should we implement universal basic income?",
    {"rawls", "durkheim"},
    "policy",
    true
);
```

**Environmental Action:**
```cpp
auto decision = ethics_plugin->makeDecision(
    "Should we ban single-use plastics?",
    {"leopold", "utilitarianism"},
    "environment",
    true
);
```

**Technology Deployment:**
```cpp
auto decision = ethics_plugin->makeDecision(
    "Should we deploy facial recognition in public spaces?",
    {"wiener", "kant", "rawls"},
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
- **Adam Smith** (adam_smith.yaml): Corporate responsibility, stakeholder theory, sustainability, CSR

#### Science & Research
- **Robert K. Merton** (merton.yaml): Research integrity, reproducibility, responsible innovation, peer review

#### Politics & Governance
- **John Rawls** (rawls.yaml): Justice as fairness, deliberative democracy, political legitimacy, human rights

#### Society & Community
- **Émile Durkheim** (durkheim.yaml): Social solidarity, capability approach, collective responsibility, welfare

#### Environment & Sustainability
- **Aldo Leopold** (leopold.yaml): Land ethic, precautionary principle, intergenerational justice, intrinsic value

#### Technology & AI
- **Norbert Wiener** (wiener.yaml): Algorithmic fairness, digital rights, privacy, value-sensitive design

## Language Support

All profiles include both English and German content:
- `name`: English name
- `name_de`: German name
- Bilingual descriptions where applicable

## Source

These philosophy profiles were originally developed for the ThemisDB Moral Philosophy Debates example (examples/24_moral_philosophy_debates/) and have been integrated into the Ethics AI Plugin for production use.
