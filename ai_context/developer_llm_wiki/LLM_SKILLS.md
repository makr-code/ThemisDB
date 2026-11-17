---
Author: "ThemisDB Team"
Created: "2026-11-17"
Last Updated: "2026-11-17"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# LLM Agent Skills

## Scope and runtime behavior

The LLM module provides `themis::llm::SkillRegistry`, an opt-in, read-only
catalog for portable skill packages. A package is a child directory of an
explicitly configured root containing a `SKILL.md` with YAML frontmatter.
`refresh()` validates package metadata and atomically publishes a snapshot;
`list()` returns sorted name/description metadata, while `load(name)` and
`readResource(name, relative_path)` return selected content only on request.

The optional `registerSkillTools()` helper exposes `list_skills`, `load_skill`,
and `read_skill_resource` through the existing `ToolRegistry`. Registration
does not enable these tools: the active `ModeSpec` allow/deny rules still
control calls. The shipped starter packages are `aql-read-query`,
`evidence-answer`, and `graph-exploration`.

## Trust and deployment boundaries

Skill instructions and resources are untrusted prompt input. They cannot grant
permissions, execute scripts, access database state, or authorize query or
graph operations. Any retrieval or executor tool must be separately configured
and enforce caller scope, read-only policy, and resource limits itself.

The registry validates paths and content, including symlinks, path escapes,
UTF-8 and configured size limits. Path validation and opening are not atomic
against concurrent filesystem replacement; configured roots, packages and
parent directories must not be writable by untrusted users while the registry
is in use. Buffered discovery may read ahead into the instruction body, so
zero body-byte I/O is not guaranteed.

The CMake runtime install and ZIP staging include the complete `config/`
directory, which contains these packages. Package presence does not make them
discoverable or activate tools. A host must configure
`SkillRegistryOptions::roots` to the deployed `agent_skills` directory (or
another trusted root), call `refresh()`, handle failures, and explicitly
register and allow any skill tools it intends to expose. Custom deployments
that omit the runtime config payload must copy the packages to their chosen
trusted root.

With the current CMake Linux package prefix `/usr`, the primary installed
directory is `/usr/etc/themisdb/agent_skills`; a compatibility copy is also
installed under `/usr/bin/config/agent_skills`. Windows installs and ZIP
packages carry it under `config/agent_skills` at the install/package root.
These paths describe current packaging behavior, not a default registry root.

## Focused coverage

`tests/llm/test_skill_registry.cpp` covers registry validation, snapshot
behavior, optional tool policy, concurrency, and discovery/loading of the
shipped packages. The test target is registered with the LLM focused tests;
source registration alone is not evidence that the suite has passed.

## Canonical sources

- [`SkillRegistry` API](../../include/llm/skill_registry.h)
- [LLM architecture](../../src/llm/ARCHITECTURE.md)
- [LLM API contract](../api_contracts/llm.md)
- [Skill package catalog and host setup](../../config/agent_skills/README.md)
- [Runtime layout](../../docs/packaging/runtime-layout.md)
- [Runtime layout definition](../../cmake/packaging/RuntimeLayout.cmake)
- [Runtime install rules](../../cmake/CMakeLists.txt)
- [ZIP staging](../../cmake/packaging/CreateZipPackage.cmake)
- [Registry implementation](../../src/llm/skill_registry.cpp)
- [Focused tests](../../tests/llm/test_skill_registry.cpp)
