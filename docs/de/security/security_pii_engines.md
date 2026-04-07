# PII Detection Engine Extensions

**Stand:** 6. April 2026  
**Version:** v1.4.0  
**Kategorie:** 🔒 Security

---

## 📑 Inhaltsverzeichnis

- [Overview](#overview)
- [Current Status](#current-status)
- [NERDetectionEngine (Rule-Based Gazetteer)](#nerdetectionengine-rule-based-gazetteer)
- [Future Engine: Embeddings (Semantic Similarity)](#future-engine-embeddings-semantic-similarity)

---


## Overview

The PII detection system uses a plugin architecture that allows multiple detection engines to work together:

1. **RegexDetectionEngine** (default, always available)
2. **NERDetectionEngine** (built-in, rule-based gazetteer — always available)
3. **EmbeddingDetectionEngine** (optional, requires fastText or word2vec)

## Current Status

✅ **Implemented:**
- Plugin architecture (`IPIIDetectionEngine` interface)
- `RegexDetectionEngine` with YAML configuration
- `NERDetectionEngine` — rule-based Named Entity Recognition (no external ML dependency)
- Engine factory and orchestration
- Runtime reload with validation and config rollback

⏳ **Ready for Implementation:**
- EmbeddingDetectionEngine (requires fastText or word2vec)

---

## NERDetectionEngine (Rule-Based Gazetteer)

The `NERDetectionEngine` is a built-in engine that detects unstructured PII which regex cannot
capture. It requires **no external ML framework** and operates entirely on configurable gazetteers.

### Detected Entity Types

| Entity | Trigger | Confidence |
|--------|---------|-----------|
| `PERSON_NAME` | Honorific prefix (`Mr.`, `Dr.`, `Prof.`, etc.) + following capitalised tokens | 0.80–0.92 |
| `ORGANIZATION` | Legal-entity suffix (`Inc.`, `Corp.`, `GmbH`, etc.) + capitalised prefix tokens | 0.65–0.88 |
| `LOCATION` | Geographic preposition (`in`, `at`, `from`, `near`, etc.) + capitalised token | 0.72–0.82 |

### YAML Configuration

```yaml
detection_engines:
  - type: "ner"
    enabled: true
    version: "1.0.0"
    settings:
      min_confidence: 0.70
      default_redaction_mode: "strict"

    honorifics:
      - "Mr."
      - "Mrs."
      - "Dr."
      - "Prof."

    org_suffixes:
      - "Inc."
      - "Corp."
      - "Ltd."
      - "GmbH"

    location_prepositions:
      - "in"
      - "at"
      - "from"
      - "near"

    redaction_modes:
      PERSON_NAME: "strict"
      ORGANIZATION: "partial"
      LOCATION: "partial"
```

### Example Detection

```cpp
PIIDetector detector("config/pii_patterns.yaml");
std::string text = "Please contact Dr. Anna Mueller at Acme Corp. in Berlin.";
auto findings = detector.detectInText(text);

// Detected by NER engine:
// PERSON_NAME  "Dr. Anna Mueller"  confidence=0.92  engine="ner"
// ORGANIZATION "Acme Corp."        confidence=0.88  engine="ner"
// LOCATION     "Berlin"            confidence=0.72  engine="ner"
```

### Field Name Classification

The NER engine also classifies JSON field names:

```cpp
detector.classifyFieldName("full_name");   // PIIType::PERSON_NAME
detector.classifyFieldName("company");     // PIIType::ORGANIZATION
detector.classifyFieldName("city");        // PIIType::LOCATION
detector.classifyFieldName("created_at"); // PIIType::UNKNOWN
```

### Build Integration

The NER engine is compiled unconditionally — no feature flags required:

```cmake
# ner_detection_engine.cpp is always included:
set(THEMIS_CORE_SOURCES
    ...
    ../src/utils/pii_detection_engine.cpp
    ../src/utils/regex_detection_engine.cpp
    ../src/utils/ner_detection_engine.cpp
    ...
)
```

## Future Engine: Embeddings (Semantic Similarity)

### Dependencies

**fastText (Recommended)**
```bash
vcpkg install fasttext
```

### YAML Configuration

```yaml
detection_engines:
  - type: "embedding"
    enabled: true
    settings:
      model_path: "models/cc.de.300.bin"  # fastText German model
      model_type: "fasttext"
      similarity_threshold: 0.80
      context_window: 5  # Words before/after to consider
    
    sensitive_keywords:
      - keyword: "gehalt"
        pii_type: "SALARY"
        similarity_threshold: 0.85
        redaction_mode: "strict"
      
      - keyword: "krankheit"
        pii_type: "HEALTH_INFO"
        similarity_threshold: 0.85
        redaction_mode: "strict"
      
      - keyword: "passwort"
        pii_type: "CREDENTIAL"
        similarity_threshold: 0.90
        redaction_mode: "strict"
```

### Implementation Sketch

```cpp
class EmbeddingDetectionEngine : public IPIIDetectionEngine {
private:
    std::unique_ptr<fasttext::FastText> model_;
    std::vector<SensitiveKeyword> keywords_;
    
    struct SensitiveKeyword {
        std::string keyword;
        PIIType type;
        double threshold;
        std::string redaction_mode;
    };
    
public:
    std::vector<PIIFinding> detectInText(const std::string& text) const override {
        auto words = tokenize(text);
        std::vector<PIIFinding> findings;
        
        for (size_t i = 0; i < words.size(); ++i) {
            auto word_vec = model_->getWordVector(words[i]);
            
            for (const auto& keyword : keywords_) {
                auto keyword_vec = model_->getWordVector(keyword.keyword);
                double similarity = cosineSimilarity(word_vec, keyword_vec);
                
                if (similarity >= keyword.threshold) {
                    // Extract context window
                    std::string context = extractContext(words, i, context_window_);
                    
                    PIIFinding finding;
                    finding.type = keyword.type;
                    finding.value = context;
                    finding.confidence = similarity;
                    finding.pattern_name = keyword.keyword;
                    finding.engine_name = "embedding";
                    findings.push_back(finding);
                }
            }
        }
        
        return findings;
    }
};
```

### Pre-trained Models

**fastText:**
- Download: https://fasttext.cc/docs/en/crawl-vectors.html
- German: `cc.de.300.bin` (6.7 GB)
- English: `cc.en.300.bin` (5.8 GB)

**word2vec:**
- Google News: `GoogleNews-vectors-negative300.bin`
- German: `german.model` (DeReWo)

## Integration Steps

### NER Engine (Built-In — No Dependencies)

The `NERDetectionEngine` is compiled unconditionally. No vcpkg packages or CMake feature flags
are needed:

```cmake
# ner_detection_engine.cpp is already included in THEMIS_CORE_SOURCES
# No optional flags required
```

To enable the NER engine at runtime, set `enabled: true` in `pii_patterns.yaml` (enabled by
default in v1.4.0+).

### Embedding Engine (Optional)

```json
{
  "dependencies": [
    "fasttext"
  ]
}
```

```cmake
# Optional embedding support
option(ENABLE_PII_EMBEDDING "Enable embedding-based PII detection" OFF)
if(ENABLE_PII_EMBEDDING)
    find_package(fastText CONFIG)
    if(fastText_FOUND)
        target_link_libraries(themis_core PRIVATE fastText::fastText)
        target_compile_definitions(themis_core PRIVATE THEMIS_ENABLE_EMBEDDING)
    endif()
endif()
```

## Performance Considerations

| Engine | Speed | Accuracy | Memory | Use Case |
|--------|-------|----------|--------|----------|
| Regex | Very Fast | Good (95%+) | Low | Structured PII (email, SSN, cards) |
| NER (rule-based) | Fast | Good (85%+) | Very Low | Names, orgs, locations via honorifics/suffixes |
| Embedding | Slow | Variable | High | Context-based, semantic PII |

**Recommendation:**
- Default: Regex + NER (enabled by default, no overhead for NER)
- Advanced: All three (highest accuracy, higher latency)

## Testing Strategy

```cpp
TEST(PIIDetectorTest, MultiEngineDetection) {
    // Both regex and NER are enabled by default
    PIIDetector detector("config/pii_patterns.yaml");
    
    std::string text = "Contact Dr. Max Mustermann at max@example.com";
    auto findings = detector.detectInText(text);
    
    // Should find (at minimum):
    // 1. "Dr. Max Mustermann" via NER (PERSON_NAME)
    // 2. "max@example.com" via Regex (EMAIL)
    bool found_person = false, found_email = false;
    for (const auto& f : findings) {
        if (f.type == PIIType::PERSON_NAME && f.engine_name == "ner") found_person = true;
        if (f.type == PIIType::EMAIL && f.engine_name == "regex") found_email = true;
    }
    EXPECT_TRUE(found_person);
    EXPECT_TRUE(found_email);
}
```

## Deployment

**Production Checklist:**
1. ✅ Regex engine always enabled (safe default)
2. ✅ NER engine enabled by default (rule-based, no external model files needed)
3. ⏳ Embedding engine optional (enable for advanced use cases)
4. ✅ YAML config with engine sections
5. ✅ Fallback to embedded defaults

## Future Enhancements

- **Multi-language Support**: Load language-specific honorific/suffix lists per locale
- **Gazetteer Expansion**: Add city/country name lists for higher-precision location detection
- **Explainability**: Return detection reasoning (which rule triggered)
- **Confidence Calibration**: Adjust thresholds based on false positive rates
- **Optional ML Upgrade**: Drop-in MITIE or ONNX model support via the same `IPIIDetectionEngine` interface
