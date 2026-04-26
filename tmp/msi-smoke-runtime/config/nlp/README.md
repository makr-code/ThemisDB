# NLP Configuration Directory

This directory contains configuration files for the ThemisDB NLP Text Analyzer.

## Overview

The NLP Text Analyzer provides lightweight, CPU-efficient natural language processing capabilities for query optimization and text analysis. It's designed as a **non-compute-intensive alternative** to LLM/SLM approaches.

## Directory Structure

```
config/nlp/
├── README.md                    # This file
├── nlp_config.yaml             # Main NLP configuration
└── stopwords/                  # Stop words per language
    ├── en.yaml                 # English stop words
    ├── de.yaml                 # German stop words
    ├── fr.yaml                 # French stop words
    ├── es.yaml                 # Spanish stop words (add as needed)
    ├── it.yaml                 # Italian stop words (add as needed)
    └── nl.yaml                 # Dutch stop words (add as needed)
```

## Configuration Files

### Main Configuration: `nlp_config.yaml`

Controls the overall NLP analyzer behavior:

```yaml
default_language: "en"
enable_stemming: true
enable_stopwords: true
max_keywords: 10
min_word_length: 3

stopwords:
  directory: "config/nlp/stopwords"
  auto_load: true
  languages:
    - "en"
    - "de"
    - "fr"
```

### Stop Words: `stopwords/*.yaml`

Each language has its own YAML file with stop words:

```yaml
language:
  code: "en"
  name: "English"
  
stopwords:
  - "a"
  - "an"
  - "the"
  # ... more stop words

custom_stopwords:
  database:
    - "query"
    - "select"
  enabled: false
```

## Adding a New Language

To add stop words for a new language:

1. **Create a new YAML file** in `stopwords/` directory:
   ```bash
   # Example for Spanish
   cp stopwords/en.yaml stopwords/es.yaml
   ```

2. **Edit the language metadata**:
   ```yaml
   language:
     code: "es"
     name: "Español"
   ```

3. **Add Spanish stop words**:
   ```yaml
   stopwords:
     - "el"
     - "la"
     - "de"
     - "que"
     # ... etc
   ```

4. **Update main config** (optional):
   ```yaml
   # In nlp_config.yaml
   stopwords:
     languages:
       - "en"
       - "de"
       - "fr"
       - "es"  # Add here
   ```

5. **Restart ThemisDB** - stop words are loaded on startup.

## Using Custom Stop Words

You can define domain-specific stop words for special contexts:

```yaml
# In your language YAML file
custom_stopwords:
  database:
    - "query"
    - "select"
    - "insert"
    - "update"
    - "delete"
  
  medical:
    - "patient"
    - "diagnosis"
    - "treatment"
  
  # Enable custom stop words
  enabled: true
```

## Stop Words Sources

Good sources for stop word lists:

- **English**: [NLTK](https://www.nltk.org/), [spaCy](https://spacy.io/)
- **German**: [German stop words](https://github.com/solariz/german_stopwords)
- **Multiple Languages**: [stopwords-iso](https://github.com/stopwords-iso/stopwords-iso)

## Format Requirements

### YAML Structure

Each stop words file must follow this structure:

```yaml
language:
  code: "xx"      # ISO 639-1 language code
  name: "Name"    # Full language name

stopwords:
  - "word1"
  - "word2"
  # ... list of stop words (lowercase)

custom_stopwords:
  enabled: false  # or true to enable custom words
```

### Best Practices

1. **Lowercase only**: All stop words should be lowercase
2. **No duplicates**: Each word should appear only once
3. **Common words**: Include only very common, low-information words
4. **No special chars**: Avoid punctuation in stop words
5. **Sort alphabetically**: Makes maintenance easier

## Performance Impact

Stop word filtering improves:
- 🚀 **Keyword extraction** - removes noise
- 📊 **Query analysis** - focuses on meaningful terms
- 💾 **Memory usage** - smaller token sets
- ⚡ **Processing speed** - fewer words to analyze

## Troubleshooting

### Stop words not loading

**Problem**: NLP analyzer uses built-in stop words instead of YAML files

**Solutions**:
1. Check file path: `config/nlp/stopwords/en.yaml`
2. Verify YAML syntax: `yamllint config/nlp/stopwords/*.yaml`
3. Check file permissions: readable by ThemisDB process
4. Check logs: enable NLP logging in `nlp_config.yaml`

### Wrong language detected

**Problem**: Language detection chooses wrong language

**Solutions**:
1. Add more stop words for your language
2. Improve heuristics in `detectLanguage()`
3. Set `default_language` in config
4. Explicitly specify language in API call

## API Usage

### Loading Stop Words

```cpp
#include "analytics/nlp_text_analyzer.h"

// Auto-load from config
NlpTextAnalyzer::Config config;
config.stopwords_directory = "config/nlp/stopwords";
config.auto_load_stopwords = true;
NlpTextAnalyzer analyzer(config);

// Or load manually
analyzer.loadStopWordsFromDirectory("config/nlp/stopwords");

// Or load single language
analyzer.loadStopWordsFromYaml("config/nlp/stopwords/de.yaml", 
                                NlpTextAnalyzer::Language::GERMAN);
```

### Checking Stop Words

```cpp
bool is_stop = analyzer.isStopWord("the", NlpTextAnalyzer::Language::ENGLISH);
// Returns: true

bool not_stop = analyzer.isStopWord("database", NlpTextAnalyzer::Language::ENGLISH);
// Returns: false
```

## Related Documentation

- [NLP Text Analyzer Documentation](../../docs/de/analytics/NLP_TEXT_ANALYZER.md)
- [AQL Query Optimization](../../docs/de/aql/README.md)
- [Compendium Chapter 13](../../compendium/chapter_13_fulltext.md)

## Version History

- **v1.0** (2025-01-11): Initial release with EN, DE, FR support
  - YAML-based configuration
  - Multi-language support
  - Custom stop words

## License

Same as ThemisDB (see LICENSE file in repository root)

## Contributing

To contribute new stop word lists:

1. Create YAML file following format above
2. Test with NLP analyzer
3. Submit PR with language support
4. Update this README

---

**Note**: Stop words are loaded once at startup. Changes to YAML files require a restart of ThemisDB to take effect.
