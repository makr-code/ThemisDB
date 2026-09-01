# Code Review Skill

This skill enables Copilot to perform context-aware code reviews on pull requests in this repository.

## Skill Configuration

- **Skill type**: `code-review`
- **Trigger**: Automatically on pull request open/synchronize, or on `@copilot review` comment

## Review Focus Areas

When reviewing pull requests in this repository, Copilot will check:

1. **C++ Safety & Correctness**
   - RAII and resource management (no raw `new`/`delete` without justification)
   - Exception safety and error propagation
   - Undefined behaviour, type-limits, sign-compare issues
   - Thread safety and lock ordering

2. **Compiler Diagnostic Hygiene**
   - No new `-Wunused-*`, `-Wmissing-field-initializers`, `-Wsign-compare`, `-Wswitch`, `-Wformat-truncation` warnings
   - No suppression pragmas without accompanying justification comment

3. **Security**
   - Input validation on trust-boundary crossings (T3/T4/T5)
   - No new raw pointer arithmetic or unchecked casts
   - Strict-aliasing compliance (`std::bit_cast` over `reinterpret_cast`)

4. **Code Style**
   - Designated initialisers for aggregate types with ≥3 fields
   - Dead code removal preferred over `[[maybe_unused]]` annotation
   - Modern C++20 features where applicable

5. **Documentation**
   - Doxygen `@brief`/`@param`/`@return`/`@throws` for all new public APIs
   - Intent and constraint comments ("why"), not implementation paraphrase ("what")

## References

- `.github/instructions/cpp-best-practices.instructions.md`
- `.github/instructions/documentation-enforcement.instructions.md`
- `ai_context/developer_llm_wiki/MODULES_AND_APIS.md`
