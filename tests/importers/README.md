---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "tests/importers/CMakeLists.txt; tests/importers/test_importers_contract_hardening_focused.cpp; include/importers/importers_api_contract.h; src/importers/BUILD_STATUS.md"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.

# tests/importers

Importer-focused C++ tests. `CMakeLists.txt` globs `test_*.cpp`, skips three
known suites before local target creation, and registers the remaining sources
as `module_importers_<stem>_focused` targets with focused CTest names. Check
that file for current exclusions and labels.

The contract-hardening suite uses inline mocks, not live connector I/O. Its
repeated-`import_id` mock behavior conflicts with the idempotent-replay wording
in `include/importers/importers_api_contract.h`; this remains TODO for source
and test owners. See [build/test status](../../src/importers/BUILD_STATUS.md).
