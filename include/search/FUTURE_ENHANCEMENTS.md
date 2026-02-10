# Search Module API - Future Enhancements

## Planned API Extensions

### Query Expansion API
**Priority:** High  
**Target Version:** v1.4.0

```cpp
// search/query_expander.h
namespace themis {

class QueryExpander {
public:
    struct Config {
        bool use_synonyms = true;
        bool correct_spelling = true;
        double synonym_weight = 0.8;
    };
    
    explicit QueryExpander(const Config& config);
    
    Result<ExpandedQuery> expand(const std::string& query);
    Result<std::string> correctSpelling(const std::string& query);
};

struct ExpandedQuery {
    std::string original;
    std::vector<std::string> expanded_terms;
    std::vector<std::string> synonyms;
    std::string corrected;
};

}
```

---

### Fuzzy Search API
**Priority:** Medium  
**Target Version:** v1.4.0

```cpp
// search/fuzzy_matcher.h
namespace themis {

class FuzzyMatcher {
public:
    enum class Algorithm {
        LEVENSHTEIN,
        SOUNDEX,
        METAPHONE
    };
    
    Result<std::vector<Match>> fuzzySearch(
        const std::string& query,
        size_t max_distance = 2
    );
};

}
```

---

### Faceted Search API
**Priority:** High  
**Target Version:** v1.4.0

```cpp
// search/faceted_search.h
namespace themis {

class FacetedSearch {
public:
    struct FacetResult {
        std::string field;
        std::map<std::string, size_t> value_counts;
    };
    
    Result<std::vector<FacetResult>> computeFacets(
        const std::string& query,
        const std::vector<std::string>& facet_fields
    );
};

}
```

---

## See Also

- [Current API](README.md)
- [Implementation FUTURE_ENHANCEMENTS](../../src/search/FUTURE_ENHANCEMENTS.md)

---

*Last Updated: February 2026*  
*Target API Version: v1.4.0*
