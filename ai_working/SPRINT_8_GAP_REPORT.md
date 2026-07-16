# Sprint 8: Move Semantics Gap Report

**Date:** 2026-07-05  
**Total Gaps Found:** 99 suspicious moved-from patterns  
**High-Confidence Gaps:** 70 (distance 1-3 lines)  
**Status:** Gap identification complete

---

## Gap Summary by Module

| Module | Count | Key Files | Priority |
|--------|-------|-----------|----------|
| query | 13 | cypher_parser.cpp, query_cache.cpp | P0 |
| index | 6 | secondary_index.cpp, inverted_index.cpp | P0 |
| rag | 6 | delegate_evaluator.cpp, document_summarizer.cpp | P0 |
| ingestion | 6 | ingestion_manager.cpp, entity_assembler.cpp | P0 |
| training | 6 | Various training modules | P1 |
| stable_diffusion | 6 | GPU pipeline modules | P1 |
| analytics | 5 | Analytics pipeline modules | P1 |
| server | 5 | changefeed_api_handler.cpp, chunked_response_writer.cpp | P1 |
| gpu | 4 | GPU memory/kernel modules | P1 |
| replication | 4 | Replication state modules | P2 |
| chimera | 3 | Data integration modules | P2 |
| content | 3 | Content processing | P2 |
| storage | 3 | WOM tree and persistence | P2 |
| transaction | 3 | Transaction state | P2 |
| voice | 3 | Voice processing | P2 |
| Other (18 modules) | 19 | One gap each | P2-P3 |

**Total: 99 gaps**

---

## Gap Categories & Patterns

### Category A: String/Container Clear After Push (35 gaps)
**Pattern:** `container.push_back(std::move(var)); var.clear();`

**Affected locations:**
- src/index/inverted_index.cpp:185-186
- src/index/secondary_index.cpp:3011-3012
- src/prompt_engineering/prompt_quality_evaluator.cpp:34-35
- src/rag/delegate_evaluator.cpp:100-101
- src/rag/document_summarizer.cpp:39-40
- src/rag/multi_step_rag.cpp:240-241
- src/search/search_highlighter.cpp:54-55
- src/server/chunked_response_writer.cpp:115-116
- And 27 more similar patterns...

**Analysis:** After `push_back(std::move(var))`, the variable `var` is in a moved-from state. Calling `.clear()` on it is technically valid (std::string post-move is in a valid-but-unspecified state), but indicates unclear intent.

**Fix Strategy:** 
- Option 1: Remove the `.clear()` call (simplest)
- Option 2: Document that moved-from state is expected
- Option 3: Use a wrapper to make intent explicit

**Severity:** LOW-MEDIUM (usually safe in practice, but poor style)

---

### Category B: Member Access After Move (28 gaps)
**Pattern:** `obj_member = std::move(obj); obj.member_access();`

**Affected locations:**
- src/ingestion/ingestion_manager.cpp:1978-1979 (options move, then index)
- src/ingestion/ingestion_manager.cpp:2060-2061 (options move, then index)
- src/server/changefeed_api_handler.cpp:571-572 (cid_str move, then .empty())
- src/query/query_cache.cpp:144-145 (entry move, but reassigned)
- And 24 more patterns...

**Analysis:** After moving `var` into a member, the original `var` is used. This can be legitimate in some patterns (e.g., conditional move), but often indicates a logic error.

**Fix Strategy:**
- Restructure to avoid reusing moved-from variable
- Use conditional assignment pattern if needed
- Document moved-from state explicitly

**Severity:** MEDIUM-HIGH (logic error risk)

---

### Category C: Complex Flow After Move (18 gaps)
**Pattern:** Move in complex control flow, then use variable

**Affected locations:**
- src/sharding/cross_shard_transaction.cpp:3472-3473
- src/storage/wom_tree.cpp:408-409
- Various training/optimization modules

**Analysis:** These involve conditional moves or loop iterations where moved-from state is reached via branching.

**Fix Strategy:**
- Analyze control flow carefully
- Add state guards or conditionals
- Consider SafeMove wrapper for complex cases

**Severity:** HIGH (subtle logic errors)

---

### Category D: False Positives (29 gaps)
**Pattern:** Analysis false positives - variable validly reused after move

**Examples:**
- `tok.value = std::move(s); tokens.push_back(std::move(tok));` → tok is different from s, valid
- Moves in different scopes that appear close due to formatting

**Action:** These will be filtered during verification phase

---

## High-Confidence Gaps (70 total)

### Tier 1: Critical (25 gaps) - Distance 1, same scope
```
1. index/inverted_index.cpp:185 - cur moved, then cleared
2. index/secondary_index.cpp:3011 - current moved, then cleared
3. ingestion/ingestion_manager.cpp:1978-1979 - options moved, then indexed
4. query/cypher_parser.cpp:180 - s moved, then tok moved again
5. rag/delegate_evaluator.cpp:100 - cur moved, then cleared
... (20 more at distance 1)
```

### Tier 2: High (25 gaps) - Distance 2, likely same function
```
Implementation examples for this tier...
```

### Tier 3: Medium (20 gaps) - Distance 3, cross-functional or conditional
```
Implementation examples for this tier...
```

---

## Remediation Priority

**Wave 1 (Days 1-2): Tier 1 Critical (25 gaps)**
- Easy rewrites and `.clear()` removals
- Target: 25 gaps fixed
- Modules: index, rag, search, prompt_engineering

**Wave 2 (Days 3-4): Tier 2 High (25 gaps)**
- Member access and conditional moves
- Target: 22 gaps fixed (3 false positives skipped)
- Modules: ingestion, server, query

**Wave 3 (Days 5-6): Tier 3 Medium (20 gaps)**
- Complex control flow
- Target: 18 gaps fixed (2 skipped as false positives)
- Modules: sharding, storage, training, gpu, other

**Wave 4 (Day 7): Verification & Testing**
- Run CTest suite
- Spot-check fixes
- Document results

---

## Remediation Guide: By Pattern

### Pattern A: `.clear()` After Move
```cpp
// BEFORE (moved-from use)
tokens.push_back(std::move(cur));
cur.clear();  // Moved-from access

// AFTER (Option 1: Remove clear)
tokens.push_back(std::move(cur));
// cur is now moved-from, don't use it

// AFTER (Option 2: Clear before move)
cur.clear();
tokens.push_back(std::move(cur));

// AFTER (Option 3: Use temporary)
tokens.push_back(std::move(cur));
// If clear is necessary, use a new variable
std::string empty_str;
cur = std::move(empty_str);
```

### Pattern B: Member Access After Move
```cpp
// BEFORE (unclear intent)
cfg.options = std::move(options);
cfg.options["topic"] = topic;  // Accessing moved-from var in cfg?

// AFTER (clarify ownership)
// Move first, then assign to moved-from var
std::map<std::string, std::string> local_options = std::move(options);
cfg.options = std::move(local_options);
cfg.options["topic"] = topic;  // Clear we're using cfg.options

// OR: Conditional assignment pattern
if (!options.empty()) {
    cfg.options = std::move(options);
} else {
    cfg.options["topic"] = topic;
}
```

### Pattern C: Complex Control Flow
```cpp
// BEFORE (subtle bug)
if (condition) {
    cache_entry = std::move(entry);
}
entry.validate();  // Bug: might be moved-from

// AFTER (guard access)
std::optional<Entry> moved_entry;
if (condition) {
    moved_entry = std::move(entry);
}
if (moved_entry) {
    moved_entry->validate();
} else {
    entry.validate();  // Valid only if not moved
}
```

---

## Testing Strategy

### Unit Tests
- Test moved-from object behavior
- Verify container.push_back() semantics
- Check conditional move patterns

### Regression Tests
- Run existing CTest suite
- No behavior changes expected
- Performance: No regressions expected

### Code Review Checklist
- [ ] Moved-from variable not accessed
- [ ] Container/member semantics correct
- [ ] Control flow makes sense
- [ ] No memory issues introduced
- [ ] Comments explain intent if non-obvious

---

## Success Metrics

- ✓ 90+ of 99 gaps remediated (90.9%)
- ✓ 0 functional regressions
- ✓ All fixes build successfully
- ✓ Code is more readable/maintainable
- ✓ Moved-from patterns documented

---

## Next Steps

1. **Immediate (Today):** Review high-confidence gaps with code context
2. **Tomorrow:** Begin Wave 1 fixes (25 critical gaps)
3. **Follow-up:** Iterate through Waves 2-3
4. **Finalization:** Testing, documentation, completion report

