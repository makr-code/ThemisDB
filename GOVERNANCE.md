# ThemisDB — Project Governance

> **Status:** Active  
> **Version:** 1.1  
> **Last Updated:** 2026-05-13

This document describes how the ThemisDB project is governed: how decisions are made, who has authority over what, and how contributors can advance to positions of greater responsibility.

---

## Table of Contents

1. [Guiding Principles](#1-guiding-principles)
2. [Roles and Responsibilities](#2-roles-and-responsibilities)
3. [Decision-Making Process](#3-decision-making-process)
4. [Contribution Policy](#4-contribution-policy)
5. [Code Review Standards](#5-code-review-standards)
6. [Conflict Resolution](#6-conflict-resolution)
7. [Unified Contribution and Escalation Paths](#7-unified-contribution-and-escalation-paths)
8. [Changes to Governance](#8-changes-to-governance)

---

## 1. Guiding Principles

ThemisDB is guided by the following principles:

- **Openness** — All source code is public. Development discussions happen in the open on GitHub.
- **Correctness over speed** — We prefer a well-reviewed, tested change over a fast one.
- **Backward compatibility** — Breaking changes require a MAJOR version bump and a migration guide.
- **Security first** — Security vulnerabilities are treated as P0 and handled with confidential disclosure. See [SECURITY.md](SECURITY.md).
- **Documentation parity** — Every API and module change must be reflected in the documentation.

---

## 2. Roles and Responsibilities

### 2.1 Users

Anyone using ThemisDB. Users contribute by filing issues, asking questions in Discussions, and providing feedback.

### 2.2 Contributors

Anyone who has submitted at least one merged pull request. Contributors are encouraged to review PRs, help triage issues, and mentor other contributors.

### 2.3 Module Maintainers

Maintainers are responsible for one or more source modules. They:
- Review and merge PRs for their module (with Project Lead final approval for large changes)
- Keep the module's `ROADMAP.md` and `README.md` up to date
- Ensure the module's tests remain green on `develop`

See [MAINTAINERS.md](MAINTAINERS.md) for the current list.

### 2.4 Project Lead

The Project Lead has final decision authority on:
- Architecture and API design
- Release timing and versioning
- Adding or removing maintainers
- Changes to governance

The current Project Lead is [@makr-code](https://github.com/makr-code).

---

## 3. Decision-Making Process

### Ordinary decisions (bug fixes, documentation, minor features)

1. Open a pull request.
2. Receive at least **1 approval** from a maintainer or the Project Lead.
3. CI must be green.
4. Merge.

### Significant decisions (new modules, API changes, architecture changes)

1. Open a GitHub Issue or Discussion describing the proposal.
2. Allow **at least 5 business days** for community feedback.
3. Achieve **consensus** among active maintainers, or final approval from the Project Lead if consensus is not reached.
4. Document the decision in `research/architecture_decisions/adr_NNN.md`.

### Breaking changes (require MAJOR version bump)

1. Follow the significant-decision process above.
2. Write a migration guide in `docs/migration/`.
3. Announce in GitHub Discussions at least **2 weeks** before the release.
4. Follow the process defined in [VERSIONING.md §9](VERSIONING.md#9-breaking-changes).

### Security-sensitive decisions

Handled privately per [SECURITY.md](SECURITY.md) and [SOP.md §SOP-05](SOP.md#sop-05--security-vulnerability-response).

---

## 4. Contribution Policy

### What we accept

- Bug fixes (all sizes)
- Documentation improvements
- New features aligned with the [roadmap](ROADMAP.md) or previously discussed in an Issue
- Performance improvements with benchmark evidence
- New client SDKs or plugins (reviewed for security and quality)

### What we do not accept (without prior discussion)

- Large architectural refactors without an approved ADR
- New mandatory third-party dependencies without a security review
- Changes that break backward compatibility without a MAJOR version plan
- Features that conflict with the edition model (see [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md))

### Licensing

All contributions must be compatible with the [MIT License with Government Clause](docs/de/legal/license.md). By submitting a pull request, you agree that your contribution is licensed under that license.

---

## 5. Code Review Standards

| Requirement | Rule |
|---|---|
| CI | All CI checks must pass before merging |
| Tests | New functionality must include tests; coverage must not drop |
| Documentation | Public API changes must update the relevant documentation |
| Style | Code must pass the project's clang-format / clang-tidy checks |
| Security | Changes touching security-sensitive paths require review by the security-responsible maintainer |
| Review SLA | PRs should be reviewed within **5 business days**; stale PRs may be closed after 30 days of inactivity |

---

## 6. Conflict Resolution

1. **Between contributors:** Discuss respectfully in the PR or issue. Reference the [Code of Conduct](CODE_OF_CONDUCT.md).
2. **Between contributor and maintainer:** Escalate to the Project Lead via a GitHub Discussion.
3. **Final authority:** The Project Lead's decision is final.

All participants must comply with the [Code of Conduct](CODE_OF_CONDUCT.md).

---

## 7. Unified Contribution and Escalation Paths

### External contributors

1. Use [CONTRIBUTING.md](CONTRIBUTING.md) for contribution workflow, PR process, and quality gates.
2. Respect [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) in all community interactions.
3. For module-specific review ownership, see [MAINTAINERS.md](MAINTAINERS.md).
4. For security vulnerabilities, use the private process in [SECURITY.md](SECURITY.md) and [SOP.md](SOP.md).

### Internal maintainers and project roles

1. Follow [SOP.md](SOP.md) for release, security response, and incident response operations.
2. Apply role boundaries and decision authority defined in this document and [MAINTAINERS.md](MAINTAINERS.md).
3. Escalate unresolved technical or process conflicts to the Project Lead.

### Contact and escalation channels (canonical)

- **General questions and design discussion:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Bugs and feature requests:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Security vulnerabilities:** [GitHub Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories/new)
- **Code of Conduct reports:** Follow [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md#project-specific-reporting-channels)

---

## 8. Changes to Governance

This document can be changed by:
1. Opening a pull request targeting `develop`.
2. Allowing **at least 7 days** for community comment.
3. Final approval by the Project Lead.

Every change must be recorded in the document's version header and noted in [CHANGELOG.md](CHANGELOG.md).

---

## Related Documents

- [MAINTAINERS.md](MAINTAINERS.md) — Current maintainer roster
- [CONTRIBUTING.md](CONTRIBUTING.md) — How to contribute
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) — Community standards
- [VERSIONING.md](VERSIONING.md) — Versioning and release policy
- [SOP.md](SOP.md) — Standard operating procedures
- [SECURITY.md](SECURITY.md) — Security policy
- [docs/ROOT_GOVERNANCE_COMMUNITY_REVIEW_AUDIT_2026-05-13.md](docs/ROOT_GOVERNANCE_COMMUNITY_REVIEW_AUDIT_2026-05-13.md) — Review and documentation audit evidence for root governance/community harmonization
- [.github/GOVERNANCE.md](.github/GOVERNANCE.md) — Issue/PR label schema and milestone standards

---
Zuletzt geprueft (Root-Sync): 2026-05-26

