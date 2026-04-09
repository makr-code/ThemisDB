## include/llama_cpp

### Scope
- Improve plugin-host ABI guidance and extension points.

### Design Constraints
- Keep `ILLMPlugin` override contract intact.

### Required Interfaces
- `LlamaCppPlugin` methods in `llama_cpp_plugin.h`

### Implementation Notes
- Prefer additive methods through interface evolution in core llm APIs.

### Test Strategy
- Compile and dynamic-load contract tests.

### Performance Targets
- No API-level regressions for generation and embedding call overhead.

### Security / Reliability
- Ensure robust behavior when model is not loaded or LoRA assets are invalid.