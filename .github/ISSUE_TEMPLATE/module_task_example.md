<!-- AI_Used: true; AI_Models: unknown; AI_Contributor: GitHub Copilot; Human_Review: false; Human_Author: ThemisDB Team; exact model backend not verifiable from final document text alone. -->

# Example: Module Task

## Scope
- Module: network
- Files: src/network/, include/network/, tests/network/
- Status: in-progress
- Labels: area:network, type:chore, priority:high, status:open
- Milestone: Documentation / Q4 2026 / LONG-TERM

## Source Evidence
- Module docs: src/network/README.md
- Source graph / symbol evidence: ai_working/sourcecode_graph.json
- Doxygen artifact: retrieve `module-doxygen-xml-<run>` (21-day retention) from `.github/workflows/gate-pr-module-doxygen-xml.yml`
- Compliance report: ai_context/developer_llm_wiki/SOLL_IST_GAP_REPORT.json

## What needs to be done
- Add missing public API documentation for the network handshake types.
- Update the module README and ARCHITECTURE notes to match the current retry and timeout behavior.
- Verify that the related focused tests still cover the handshake and streaming paths.

## Validation
- Repro command: ctest --preset windows-release -R "network_protocol_handshake|network_streaming" --output-on-failure
- Gate check: python scripts/check_module_direct_doxygen.py --module network
- Success condition: all relevant network tests pass and the public API has complete Doxygen coverage.

## Merge Gate
- Merge erst nach vollstaendig erfuellten Akzeptanzkriterien und Maintainer-Freigabe.

## Acceptance criteria
- [ ] All public handshake and stream APIs have brief and parameter documentation.
- [ ] README and ARCHITECTURE reflect current retry and timeout semantics.
- [ ] Focused network tests pass in the repo gate and no known evidence gaps remain for the module.

## Notes
- Risk/constraint: avoid altering external protocol compatibility without a migration note.
- Ownership/path rule: include/src/tests are repo-owned; external vendor code remains excluded from module scope.
- Follow the module quick rules in [.github/copilot/module-quick-rules.md](../copilot/module-quick-rules.md) and the detailed guidance in [.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md](../MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md).
- The labels and milestone mirror the automated module issue sync so manual and generated issues stay aligned.
- Archive or delete markdown files that are no longer relevant, except the governance documentation.
