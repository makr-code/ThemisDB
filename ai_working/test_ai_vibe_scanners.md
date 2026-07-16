# ThemisDB Gap Worklist for Remote Ollama gemma4

- [ ] Scope: actionable themis_core findings only (third_party is informational).

## Work Items

- [ ] HIGH | include | chimera_adapter_missing_interface | include/chimera/database_adapter.hpp:79
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: ArchitectureContract
// Location: include/chimera/database_adapter.hpp:79
// Problem: chimera_adapter_missing_interface
// Description: Chimera adapter 'IStreamingAdapter' should implement one of the required adapter interfaces
// Context: class IStreamingAdapter {
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | chimera_adapter_missing_interface | include/chimera/batch_executor.hpp:77
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: ArchitectureContract
// Location: include/chimera/batch_executor.hpp:77
// Problem: chimera_adapter_missing_interface
// Description: Chimera adapter 'IBatchAdapter' should implement one of the required adapter interfaces
// Context: class IBatchAdapter {
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | layer_dependency_violation | include/core/query_engine_builder.h:45
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: ArchitectureContract
// Location: include/core/query_engine_builder.h:45
// Problem: layer_dependency_violation
// Description: Module 'core' must not depend on 'query' (layer violation)
// Context: #include "query/query_engine.h"
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | layer_dependency_violation | include/llm/ai_orchestrator.h:744
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: ArchitectureContract
// Location: include/llm/ai_orchestrator.h:744
// Problem: layer_dependency_violation
// Description: Module 'llm' must not depend on 'server' (layer violation)
// Context: * #include "server/mcp_server.h"
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

