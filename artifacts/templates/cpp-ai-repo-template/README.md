# C++ AI Repo Template

Template files for AI-assisted C++ development with:

- Copilot and Claude project contracts
- C++ instruction files for agent behavior
- VS Code C++ semantic settings
- CodeQL workflow focused on C/C++ and Actions
- Doxygen coverage enforcement workflow
- AI context/workspace folder conventions

## Quick start

1. Copy all files into a new repository.
2. Replace placeholders:
- YOUR_MAIN_BRANCH
- YOUR_CPP_STANDARD
- PROJECT_NAME
3. Adjust doc coverage threshold in `.github/workflows/doxygen-coverage-gate.yml`.
4. Ensure Doxygen XML config exists (for example `Doxyfile_xml`).
5. Ensure `scripts/verify_docs.py` is executable in CI.

## Included structure

- `.github/copilot-instructions.md`
- `.github/instructions/*.instructions.md`
- `.github/workflows/codeql.yml`
- `.github/workflows/doxygen-coverage-gate.yml`
- `.vscode/settings.json`
- `CLAUDE.md`
- `ai_context/`
- `ai_working/`
- `scripts/verify_docs.py`
