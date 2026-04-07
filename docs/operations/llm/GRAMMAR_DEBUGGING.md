# Runbook: Grammar Debugging Guide

**Component:** ThemisDB LLM module — Grammar subsystem
**Severity:** Operational
**Last Updated:** April 2026

---

## Overview

This runbook helps diagnose and fix issues with the grammar-constrained generation feature (`src/llm/grammar.cpp`, `src/llm/llama_grammar_adapter.cpp`). Grammar constraints force the LLM to produce output that conforms to an EBNF grammar (e.g., strict JSON output, SQL-like queries, or structured data formats).

---

## Known Issues and Current Limitations

### Issue 1 — Null vocab pointer (v1.5)

`Grammar::compile()` calls `llama_grammar_init(nullptr, ...)` passing a null `llama_vocab*`. This is acknowledged in the source comment (`grammar.cpp:110`):

> *"Note: llama_grammar_init requires a vocab pointer from a loaded model. For now, we'll pass nullptr and handle this at usage time."*

**Symptom:** Grammar may silently produce unconstrained output for token-filtering rules that require vocab knowledge.

**Workaround (until Q1 fix):** Avoid grammar rules that rely on specific token-to-text mappings. Pure structural grammars (e.g., balanced bracket rules, numeric patterns) are less affected.

### Issue 2 — Silent fallback when API unavailable

If the llama.cpp grammar API symbols are absent at runtime (`themis_llama_grammar_available()` returns `false`), `Grammar::compile()` logs a warning and returns `false`, but inference continues **without constraints**.

**Symptom:** Output does not conform to the specified grammar despite no error being returned to the caller.

**Diagnostic:**

```bash
# Check whether the grammar API is present in the linked llama.cpp
grep -r "themis_llama_grammar_available\|Grammar compilation skipped" /var/log/themisdb.log
```

If you see `"Grammar compilation skipped"`, the llama.cpp binary was built without grammar support.

---

## Diagnosing Grammar Failures

### Step 1 — Enable debug logging

Set the log level to DEBUG before reproducing the issue:

```yaml
# config/logging.yaml
log_level: debug
```

Look for these log lines:

| Log message | Meaning |
|-------------|---------|
| `Grammar compiled successfully: start_symbol=...` | Grammar compiled OK |
| `Grammar compilation skipped: Grammar support is unavailable` | API absent; inference unconstrained |
| `Grammar compilation failed for start_symbol: ...` | EBNF syntax error or null vocab issue |
| `Exception during grammar compilation: ...` | Unexpected exception in llama.cpp |

### Step 2 — Validate the EBNF grammar offline

Use the llama.cpp `llama-cli` tool to test the grammar independently:

```bash
# Clone and build llama.cpp (if not already available)
git clone https://github.com/ggerganov/llama.cpp
cd llama.cpp && cmake -B build && cmake --build build -j8

# Test grammar with llama-cli
./build/bin/llama-cli \
  -m /models/llama-3.2-1b-q8_0.gguf \
  --grammar-file /path/to/your/grammar.gbnf \
  -p "Generate a JSON object:" \
  -n 200
```

### Step 3 — Common EBNF errors

| Error | EBNF cause | Fix |
|-------|-----------|-----|
| `Invalid EBNF syntax` | Missing quotes around literal strings | Wrap literals in `"..."` |
| `Invalid EBNF syntax` | Unbalanced parentheses | Count `(` and `)` |
| `Start symbol not found` | `start_symbol` name does not match a rule | Check spelling in the EBNF and the `Grammar(ebnf, start_symbol)` constructor call |
| Output does not match grammar | Null vocab issue (see Issue 1 above) | Avoid vocab-dependent rules until Q1 fix |

### Step 4 — Check grammar metric

Once `llm_grammar_compilations_total` is instrumented (Q1):

```bash
curl -s http://localhost:9091/metrics | grep llm_grammar_compilations_total
# llm_grammar_compilations_total{result="success"} 1042
# llm_grammar_compilations_total{result="fail_invalid_ebnf"} 3
# llm_grammar_compilations_total{result="fail_api_missing"} 0
```

---

## Testing a Grammar Before Using It in Production

### Minimal JSON grammar (for reference)

```ebnf
root   ::= object
value  ::= object | array | string | number | ("true" | "false" | "null")
object ::= "{" ws (string ":" ws value ("," ws string ":" ws value)*)? "}"
array  ::= "[" ws (value ("," ws value)*)? "]"
string ::= "\"" ([^\\"\x7F\x00-\x1F] | "\\" (["\\bfnrt] | "u" [0-9a-fA-F]{4}))* "\""
number ::= "-"? ([0-9] | [1-9] [0-9]*) ("." [0-9]+)? (([eE] [-+]? [0-9]+))?
ws     ::= [ \t\n\r]*
```

### Test procedure

1. Save the grammar to a `.gbnf` file.
2. Test with `llama-cli` (Step 2 above).
3. Verify at least 20 responses before declaring it production-ready.
4. Check for edge cases: empty objects `{}`, null values, nested arrays.

---

## Escalation

If grammar issues persist after following the steps above:

1. Confirm the llama.cpp version: `grep LLAMA_VERSION vcpkg.json`.
2. Check the [llama.cpp GBNF grammar documentation](https://github.com/ggerganov/llama.cpp/tree/master/grammars).
3. Open an issue tagged `area:llm` with:
   - The EBNF grammar text.
   - The model and quantisation used.
   - The relevant log lines.
   - Whether `themis_llama_grammar_available()` returned `true`.

---

## Related Documents

- `docs/llm_roadmap.md` — Section 1.2 (grammar gaps) and Q1 items
- `docs/GRAMMAR_IMPLEMENTATION_COMPLETE.md` — Grammar implementation history
- `docs/GRAMMAR_IMPLEMENTATION_SUMMARY.md` — Grammar summary
- `src/llm/grammar.cpp` — Grammar compilation source
- `src/llm/llama_grammar_adapter.cpp` — Dynamic API loader
- `src/llm/grammars/` — Example grammar files
