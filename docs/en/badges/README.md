# Badge Documentation

This section documents the repository badges used in the [ThemisDB README](../../../README.md) and related badge reference pages.

Workflow-generated metric badges resolve through committed JSON badge files in `.github/badges/`. Static Shields/GitHub badges remain direct links to their upstream services.

## Badges

### Row 1 – Build & Release

| Badge | What it shows | Details |
|-------|---------------|---------|
| CI Status | Latest build result (Themis Core CI) | [ci-status.md](ci-status.md) |
| Security CI | Latest security hardening CI result | [security-ci.md](security-ci.md) |
| Version/Release | Current release tag | [version.md](version.md) |
| License | Project license (MIT) | [license.md](license.md) |
| Docker Pulls | Total Docker Hub pulls for `themisdb/themisdb` | [docker.md](docker.md) |

### Row 1b – Edition CI

| Badge | What it shows | Details |
|-------|---------------|---------|
| MINIMAL | Latest MINIMAL edition CI result on `develop` | [edition-ci.md](edition-ci.md) |
| COMMUNITY | Latest COMMUNITY edition CI result on `develop` | [edition-ci.md](edition-ci.md) |
| ENTERPRISE | Latest ENTERPRISE edition CI result on `develop` | [edition-ci.md](edition-ci.md) |
| HYPERSCALER | Latest HYPERSCALER edition CI result on `develop` | [edition-ci.md](edition-ci.md) |
| MILITARY | Latest MILITARY edition CI result on `develop` | [edition-ci.md](edition-ci.md) |

### Row 2 – Code Metrics

| Badge | What it shows | Details |
|-------|---------------|---------|
| Lines of Code | Core C/C++ line count for `src/` + `include/`, published from `.github/badges/lines-of-code.json` | [loc.md](loc.md) |
| Repo Size | Total repository size (GitHub API) | [repo-size.md](repo-size.md) |
| Last Commit | Date of the most recent commit on `develop` | [last-commit.md](last-commit.md) |
| Stars | GitHub star count | [stars.md](stars.md) |
| Forks | GitHub fork count | [forks.md](forks.md) |

### Row 3 – Community

| Badge | What it shows | Details |
|-------|---------------|---------|
| Open Issues | Number of currently open GitHub Issues | [issues.md](issues.md) |
| Contributors | Number of unique commit authors | [contributors.md](contributors.md) |
| Docs | Link to the GitHub Pages documentation site | [docs-site.md](docs-site.md) |
| pre-commit | Indicates pre-commit hooks are configured | [pre-commit.md](pre-commit.md) |
| PRs Welcome | Community contribution signal | [prs-welcome.md](prs-welcome.md) |

### Row 4 – Tech Stack

| Badge | What it shows | Details |
|-------|---------------|---------|
| C++20 | C++ language standard used | [cpp-standard.md](cpp-standard.md) |
| Docker Image Size | Compressed size of the `latest` Docker image | [docker-image-size.md](docker-image-size.md) |
| GPU CI | Latest GPU module CI gate result | [gpu-ci.md](gpu-ci.md) |
| LLM CI | Latest LLM CPU-fallback CI result | [llm-ci.md](llm-ci.md) |
| Platform | Supported operating systems | [platform.md](platform.md) |

## Design rationale

The badge catalog groups repository badges by topic so generated metrics and service-backed badges stay explainable and reviewable:

- **Row 1** answers: *Is the build healthy? What version is this? Can I use it freely?*
- **Row 1b** answers: *Do all ThemisDB editions (MINIMAL/COMMUNITY/ENTERPRISE/HYPERSCALER/MILITARY) build successfully on `develop`?*
- **Row 2** answers: *How active is development? How large is the codebase?*
- **Row 3** answers: *Is this project maintained and welcoming? Where are the docs?*
- **Row 4** answers: *What technology does this use? Does GPU/LLM support work?*

Capability details (supported models, performance numbers, module documentation) are in the README body and module READMEs linked from there.
