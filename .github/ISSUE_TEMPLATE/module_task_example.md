# Example: Module Task

## Scope
- Module: network
- Files: src/network/, include/network/, tests/network/
- Status: in-progress
- Labels: maintenance, soll-ist
- Milestone: backlog

## Source Evidence
- Module docs: src/network/README.md
- Source graph / symbol evidence: ai_working/sourcecode_graph.json
- Doxygen artifact: ai_context/developer_llm_wiki/module_doxygen_artifacts/network/xml/index.xml
- Compliance report: ai_context/developer_llm_wiki/SOLL_IST_GAP_REPORT.json

## What needs to be done
- Add missing public API documentation for the network handshake types.
- Update the module README and ARCHITECTURE notes to match the current retry and timeout behavior.
- Verify that the related focused tests still cover the handshake and streaming paths.

## Validation
- Repro command: ctest --preset windows-release -R "network_protocol_handshake|network_streaming" --output-on-failure
- Gate check: python scripts/check_module_direct_doxygen.py --module network
- Success condition: all relevant network tests pass and the public API has complete Doxygen coverage.

## Acceptance criteria
- [ ] All public handshake and stream APIs have brief and parameter documentation.
- [ ] README and ARCHITECTURE reflect current retry and timeout semantics.
- [ ] Focused network tests pass in the repo gate and no known evidence gaps remain for the module.

## Notes
- Risk/constraint: avoid altering external protocol compatibility without a migration note.
- Ownership/path rule: include/src/tests are repo-owned; external vendor code remains excluded from module scope.
- Follow the module quick rules in [.github/copilot/module-quick-rules.md](../copilot/module-quick-rules.md) and the detailed guidance in [.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md](../MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md).
- The labels and milestone mirror the automated module issue sync so manual and generated issues stay aligned.
