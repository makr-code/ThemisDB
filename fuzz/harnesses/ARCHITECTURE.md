# fuzz/harnesses — Architecture

This document describes the design, build requirements, and integration conventions for all ThemisDB fuzz harnesses.

## Harness Pattern

All harnesses follow the standard AFL++ / libFuzzer entry point:

```cpp
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // feed data into the target under test
    // return 0 — never abort, let sanitizers detect bugs
    return 0;
}
```

**AFL++ persistent mode** is used in all harnesses for performance:

```cpp
#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
    while (__AFL_LOOP(1000)) {
        // process input
    }
#else
    // single-shot fallback
#endif
```

No global state is shared between loop iterations. Each iteration must leave the process state as clean as the harness receives it.

## Build Requirements

| Requirement | Value |
|-------------|-------|
| C++ compiler | `afl-clang-lto++` (LTO instrumentation for best edge coverage) |
| C compiler | `afl-clang-lto` |
| CMake flag | `-DENABLE_FUZZING=ON` |
| Build type | `Debug` (for full sanitizer instrumentation) |
| Sanitizers | ASan + UBSan (minimum); MSan for crypto harnesses |

Configure:

```bash
cmake -B build-fuzz \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_FUZZING=ON \
  -DCMAKE_C_COMPILER=afl-clang-lto \
  -DCMAKE_CXX_COMPILER=afl-clang-lto++
```

## Corpus Layout Convention

Each harness has a dedicated seed corpus directory under `fuzz/corpus/`:

```
fuzz/corpus/<target_name>/seed_*.<ext>
```

File extensions by target category:

| Category | Extension |
|----------|-----------|
| SQL/AQL | `.aql`, `.sql` |
| Binary protocols | `.bin` |
| JSON | `.json` |
| Text/grammar | `.txt` |
| LLM model files | `.gguf` |

Example:

```
fuzz/corpus/aql/seed_select.aql
fuzz/corpus/jwt/seed_rs256_valid_shape.bin
fuzz/corpus/pii/seed_email.txt
```

## Dictionary Convention

AFL++ token dictionaries live under `fuzz/dictionaries/`:

```
fuzz/dictionaries/<target_name>.dict
```

Currently available: `aql.dict`, `crypto.dict`, `json.dict`, `pii.dict`.

Pass to AFL++ with `-x fuzz/dictionaries/<target>.dict`.

## Sanitizer Requirements per Harness Category

| Category | Harnesses | Required Sanitizers |
|----------|-----------|---------------------|
| Crypto | `crypto` (future) | ASan + UBSan + MSan |
| Parser | `aql_parser`, `grammar`, `http_parser`, `ldap_dn` | ASan + UBSan |
| Auth/Token | `jwt_rbac_config` | ASan + UBSan |
| I/O / Importer | `postgres_importer`, `gguf_loader` | ASan + UBSan |
| Security | `security_input_validator`, `security_policy_engine`, `pii_redaction` | ASan + UBSan |

MSan (MemorySanitizer) is incompatible with ASan. Crypto harnesses requiring MSan must be built in a separate configuration.

## Integration Points

Harnesses:
1. **Include** ThemisDB public headers from `include/`.
2. **Link** against ThemisDB static libraries produced by the `build-fuzz` CMake configuration.
3. **Do not** open network sockets, spawn threads, or access external resources.
4. **Do not** write to the filesystem except via sanitizer/crash reporting.

Binary outputs are placed in `build-fuzz/fuzz/bin/` by the CMake `fuzz_targets` target.

## Non-Goals

- **No persistent state between fuzzing loop iterations.** Each call to `LLVMFuzzerTestOneInput` must be stateless from the harness perspective.
- **No multi-process coordination.** AFL++ master/secondary parallelism is handled by the fuzzer itself, not the harness.
- **No simulation or stub logic.** Harnesses call real ThemisDB production code paths; mocking is prohibited.
- **No output to stdout/stderr during normal operation** (breaks AFL++ output parsing). Use compiler-defined crash mechanisms (abort, sanitizer traps) only.

## File List

| File | Target |
|------|--------|
| `aql_parser_harness.cpp` | AQL query parser |
| `gguf_loader_harness.cpp` | GGUF model file loader |
| `grammar_harness.cpp` | LLM grammar parser |
| `http_parser_harness.cpp` | HTTP request parser |
| `jwt_rbac_config_harness.cpp` | JWT token + RBAC config parser |
| `ldap_dn_harness.cpp` | LDAP distinguished name parser |
| `pii_redaction_harness.cpp` | PII redaction pipeline |
| `postgres_importer_harness.cpp` | Postgres dump importer |
| `security_input_validator_harness.cpp` | Security input validation |
| `security_policy_engine_harness.cpp` | Policy engine evaluator |

## See Also

- [fuzz/harnesses/ROADMAP.md](ROADMAP.md) — planned enhancements and production readiness
- [fuzz/README.md](../README.md) — top-level fuzzing overview and quick start
- [.github/workflows/fuzzing.yml](../../.github/workflows/fuzzing.yml) — CI fuzzing workflow
