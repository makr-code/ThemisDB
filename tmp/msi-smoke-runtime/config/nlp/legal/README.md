# Legal NLP Configuration

## German Administrative Law Modal Verbs

### Deontic Logic Notation
- `O(φ)` - Obligation (Pflicht)
- `O_default(φ)` - Default obligation (Soll-Vorschrift)
- `P(φ)` - Permission (Erlaubnis)
- `F(φ)` - Forbidden (Verbot)

### Strength Values
- `1.0` - Mandatory (binding)
- `0.8` - Default rule (deviation requires justification)
- `0.3` - Discretionary (administrative discretion)

### Legal Modality Categories

#### Mandatory (Pflicht)
Modal verbs expressing binding legal obligations:
- **muss** (must) - Unconditional legal requirement
- **hat zu** (has to) - Formal binding obligation

**Legal Effect**: No discretion; failure to comply = legal violation

**Example**: "Der Antragsteller muss die Unterlagen einreichen."
(The applicant must submit the documents.)

#### Default Rule (Soll-Vorschrift)
Modal verbs expressing default obligations with justification requirement for deviation:
- **soll** (shall) - Rule with justified exceptions allowed

**Legal Effect**: Presumptive obligation; deviation requires documented justification

**Example**: "Die Behörde soll binnen zwei Wochen entscheiden."
(The authority shall decide within two weeks.)

#### Discretionary (Ermessen)
Modal verbs granting administrative discretion:
- **kann** (may) - Discretionary decision power

**Legal Effect**: Administrative choice; must still comply with proportionality principles

**Example**: "Die Behörde kann eine Fristverlängerung gewähren."
(The authority may grant a deadline extension.)

### Combinations

Modal verb combinations have special legal meanings:

| Pattern | Legal Effect | Deontic Logic |
|---------|-------------|---------------|
| `kann...soll` | Discretion with expectation | `P(φ) ∧ O_default(φ)` |
| `darf nicht` | Prohibition | `F(φ)` |
| `muss nicht` | Prohibition (negated obligation) | `F(φ)` |

### Context Rules for AI Systems

When analyzing legal texts, AI systems must:

#### For Discretionary Decisions (`kann`):
1. Ensure **Verhältnismäßigkeitsprüfung** (proportionality test)
2. Apply **Gleichbehandlungsgrundsatz** (equal treatment principle)
3. Avoid **Ermessensfehler** (discretion errors)

#### For Default Rules (`soll`):
1. Document justification when deviating
2. Demonstrate atypical circumstances warrant deviation

### Usage in AQL

```sql
-- Analyze legal document for modal verbs
FOR doc IN legal_documents
  LET modalities = NLP_ANALYZE_LEGAL_MODALITIES(doc.text, {
    config: 'config/nlp/legal/german_modal_verbs.yaml'
  })
  RETURN {
    document_id: doc._key,
    modalities: modalities
  }

-- Filter documents with mandatory obligations
FOR doc IN legal_documents
  LET modalities = NLP_ANALYZE_LEGAL_MODALITIES(doc.text, {
    config: 'config/nlp/legal/german_modal_verbs.yaml'
  })
  FILTER LENGTH(modalities[* FILTER CURRENT.category == 'obligation']) > 0
  RETURN doc
```

### C++ API Usage

```cpp
#include "analytics/nlp_text_analyzer.h"

using namespace themis::analytics;

NlpTextAnalyzer analyzer;

// Analyze German administrative law text
std::string text = "Die Behörde muss binnen Monatsfrist entscheiden. "
                   "Die Genehmigung kann mit Auflagen versehen werden.";

auto modalities = analyzer.extractLegalModalities(text, "de");

for (const auto& modality : modalities) {
    std::cout << "Verb: " << modality.verb << std::endl;
    std::cout << "Category: " << modality.category << std::endl;
    std::cout << "Strength: " << modality.strength << std::endl;
    std::cout << "Deontic: " << modality.deontic_logic << std::endl;
    std::cout << "Interpretation: " << modality.interpretation << std::endl;
}
```

### References

#### Legal Framework
- **BImSchG** - Bundes-Immissionsschutzgesetz (Federal Immission Control Act)
- **VwVfG** - Verwaltungsverfahrensgesetz (Administrative Procedure Act)
- **BauGB** - Baugesetzbuch (Federal Building Code)

#### Legal Principles
- **Rechtsstaatlichkeit** - Rule of law
- **Verhältnismäßigkeitsprüfung** - Proportionality principle
- **Gleichbehandlungsgrundsatz** - Equal treatment principle
- **Ermessen** - Administrative discretion
- **Begründungspflicht** - Duty to provide justification

### Version History

- **v1.0** (2026-02-12): Initial release
  - German administrative law modal verbs
  - YAML-configurable rules
  - Deontic logic semantics
  - Multi-pattern support

### License

Same as ThemisDB (see LICENSE file in repository root)

### Contributing

To extend this configuration:

1. Add new modal verb patterns to `german_modal_verbs.yaml`
2. Test with real legal texts
3. Update this README with examples
4. Submit PR with changes

### Notes

- Modal verbs are case-insensitive in matching
- Word boundaries (`\b`) ensure precise matching
- Context requirements guide AI decision-making
- Deontic logic provides formal semantics
