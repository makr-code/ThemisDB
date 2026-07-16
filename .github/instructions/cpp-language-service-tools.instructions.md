---
description: "Use when editing C/C++ files and symbol-level impact analysis is required; enforce C++ language-service tools over text grep for symbol operations."
applyTo: "**/*.{c,cc,cpp,cxx,h,hh,hpp,hxx,ipp,tpp}"
---

# C++ Language Service Tools Instructions

For C/C++ symbol analysis and refactoring safety, use language-service symbol tools first.

## Mandatory Rules

- Use `GetSymbolInfo_CppTools` for symbol definitions and type/intellisense context.
- Use `GetSymbolReferences_CppTools` before any rename/refactor/edit that can affect call sites.
- Use `GetSymbolCallHierarchy_CppTools` to analyze incoming/outgoing call relationships before behavior-changing edits.
- Always pass absolute file paths to all tool calls.
- Do not rely on `grep`/`ripgrep` for authoritative C++ symbol references; use them only as a supplemental fallback after symbol tools.
- Simulation/Stub/Mockup paths are forbidden without explicit human approval and explicit human marking.

## Safe Refactor Workflow

1. Resolve the symbol with `GetSymbolInfo_CppTools`.
2. Enumerate all references with `GetSymbolReferences_CppTools`.
3. Inspect call impact with `GetSymbolCallHierarchy_CppTools`.
4. Apply edits.
5. Build and run relevant tests.

## Notes

- C++ overloads/templates/macros can make text search incomplete or ambiguous.
- Symbol tools are required to reduce missed call sites and regressions.

## Mandatory Marker For Approved Non-Production Paths

If a human explicitly approves a Simulation/Stub/Mockup path, the code must include a human-marked block:

```cpp
// NON-PRODUCTION PATH (Simulation/Stub/Mockup)
// Reason: <why needed>
// Activation: <when active>
// Production Delta: <difference to primary path>
// Approved By: <human name/role + ticket/PR reference>
// Removal Target: <date or milestone>
```
