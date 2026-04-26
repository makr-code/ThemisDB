# Ethical Guidelines for ThemisDB LLM

## Quick Start

The Ethical Guidelines System ensures that ThemisDB's AI never patronizes humans and respects human autonomy. It's based on the **Universal Declaration of Human Rights** and **Asimov's Laws of Robotics** (adapted for AI).

### What it does

✅ **Detects ethical/moral contexts** in queries and retrieved documents  
✅ **Augments LLM prompts** with appropriate ethical guidelines  
✅ **Supports moral imperatives** from different philosophical traditions  
✅ **Prevents patronizing behavior** - AI presents options, never commands  
✅ **Bilingual support** - German and English

### Foundation

1. **Universal Declaration of Human Rights (UN, 1948)**
   - Article 1: Dignity and equality
   - Article 18: Freedom of thought, conscience, religion
   - Article 19: Freedom of opinion and expression

2. **Asimov's Laws (adapted)**
   - Law 1: Do no harm
   - Law 2: Respect human autonomy (not "obey orders")
   - Law 3: Maintain integrity

### Example

**User Query:**
```
"What is my moral duty in this situation?"
```

**System Behavior:**
1. Detects moral imperative context
2. Augments prompt with ethical guidelines
3. Presents MULTIPLE philosophical perspectives:
   - Kantian ethics (categorical imperative)
   - Utilitarianism (greatest happiness)
   - Virtue ethics (character traits)
   - Religious ethics (various traditions)
   - Cultural relativism (context-dependent)

**AI Response Style:**
- ✅ "From perspective X, this would be..."
- ✅ "Different moral traditions view this as..."
- ❌ "You must do X"
- ❌ "It is your moral duty to..."

### Configuration

See `config/ethical_guidelines.yaml` for full configuration.

```yaml
config:
  enabled: true
  detection_threshold: 0.6
  always_apply_default: true
  show_disclaimers: true
  language_mode: "both"
```

### Documentation

- **German**: [docs/de/llm/ETHICAL_GUIDELINES_SYSTEM.md](../../docs/de/llm/ETHICAL_GUIDELINES_SYSTEM.md)
- **English**: (Coming soon)

### API

```cpp
#include "llm/ethical_guidelines_manager.h"

themis::llm::EthicalGuidelinesManager manager;

// Detect ethical context
auto result = manager.detectEthicalContext(query);

// Augment prompt with guidelines
std::string augmented = manager.augmentPrompt(prompt, result);

// Augment response with disclaimer
std::string final = manager.augmentResponse(response, result);
```

### Key Principle

> **"Augmentation, not Replacement"**  
> The AI assists humans in making decisions but never makes decisions for them.

---

**License:** Based on UN Human Rights (Public Domain) and Asimov's Laws (Adapted)  
**Implementation:** MIT License
