# Central runtime/package layout definition for ThemisDB releases.
# This file is consumed by CMake install rules and ZIP staging.

include_guard(GLOBAL)

set(THEMIS_RUNTIME_LAYOUT_VERSION "1")

# Components expected in full Windows release ZIP builds.
set(THEMIS_RELEASE_COMPONENTS_ZIP
    runtime
    tools
    models
    shaders
    documentation
    benchmarks
    tests
)

# Runtime directories that must exist in release packages.
set(THEMIS_RELEASE_RUNTIME_DIRS
    config
    data
    certs
    plugins
)

# Entry documentation to place at package root.
set(THEMIS_RELEASE_ROOT_DOC_FILES
    README.md
    CHANGELOG.md
    RELEASE_STRATEGY.md
    QUICKSTART.md
    SETUP.md
    VERSION
    VERSIONING.md
)

# Runtime environment bootstrap script for Windows operators.
set(THEMIS_RELEASE_SETUP_SCRIPT "scripts/setup-runtime-env.ps1")

# Human-readable canonical runtime layout definition.
set(THEMIS_RELEASE_LAYOUT_DOC "docs/packaging/runtime-layout.md")
