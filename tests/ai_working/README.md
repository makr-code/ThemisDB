<!-- Status: current | validated: 2026-11-18 -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

# tests/ai_working

`ai_working` has no C++ test target. `CMakeLists.txt` contains only the minimum CMake version and registers no executable with CTest. The standard-library Python tests exercise the compactor's generated trust label, unreadable-source failure, and archive collision handling:

```bash
python -m unittest discover -s tests/ai_working -p 'test_*.py'
```

These focused checks do not cover the separate `scripts/archive_ai_working.py` governance checker, the full compaction lifecycle, or performance. The directory is not registered as a CTest target.
