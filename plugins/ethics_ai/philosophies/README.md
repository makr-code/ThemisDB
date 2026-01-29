# Philosophy Profiles

This directory contains YAML-based philosophy profiles for the Ethics AI Plugin.

## Available Philosophy Schools

1. **kant.yaml** - Kantian Ethics (Deontological)
2. **utilitarianism.yaml** - Utilitarian Ethics (Consequentialist)
3. **contractualism.yaml** - Contractualist Ethics
4. **rationalism.yaml** - Rationalist Ethics
5. **socratic.yaml** - Socratic Ethics
6. **arendt.yaml** - Arendtian Political Philosophy
7. **dilthey.yaml** - Dilthey's Hermeneutic Philosophy
8. **marx.yaml** - Marxist Philosophy
9. **nietzsche.yaml** - Nietzschean Philosophy
10. **schopenhauer.yaml** - Schopenhauerian Philosophy

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
}
```

## Adding New Philosophies

To add a new philosophy school:

1. Create a new YAML file following the structure above
2. Choose a unique `school_id`
3. Fill in all required fields
4. Place the file in this directory
5. The plugin will automatically detect and load it

## Philosophy School Characteristics

### Deontological (Duty-Based)
- **Kant**: Universal moral laws, categorical imperative, respect for persons

### Consequentialist (Outcome-Based)
- **Utilitarianism**: Greatest happiness principle, utility maximization

### Virtue Ethics
- **Socratic**: Knowledge as virtue, examined life, dialectical method

### Social Contract
- **Contractualism**: Agreement-based morality, mutual benefit

### Other Approaches
- **Rationalism**: Reason-based ethics
- **Arendt**: Political philosophy, plurality, human condition
- **Marx**: Materialist ethics, social justice, class analysis
- **Nietzsche**: Will to power, beyond good and evil, revaluation
- **Schopenhauer**: Compassion-based ethics, will and representation
- **Dilthey**: Hermeneutic understanding, life philosophy

## Language Support

All profiles include both English and German content:
- `name`: English name
- `name_de`: German name
- Bilingual descriptions where applicable

## Source

These philosophy profiles were originally developed for the ThemisDB Moral Philosophy Debates example (examples/24_moral_philosophy_debates/) and have been integrated into the Ethics AI Plugin for production use.
