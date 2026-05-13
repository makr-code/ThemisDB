# ThemisDB Documentation Guide

> This page describes the documentation structure and how to navigate it.
> Start reading here if you want to understand where to find things.

---

## 🗂️ Documentation Structure

```
docs/
├── README.md                   ← navigation entry point for docs/
├── README-DOCUMENTATION.md     ← You are here (how docs are organized)
├── QUICK_REFERENCE.md          ← One-page cheat sheet (commands & API)
├── FAQ.md                      ← Frequently asked questions
├── EXAMPLES_INDEX.md           ← Full index of 37+ example projects
├── EXAMPLES_QUICKSTART.md      ← Guided tour through example projects
├── INTEGRATION_GUIDE.md        ← Integrating ThemisDB with other systems
│
├── guides/                     ← How-to guides (task-oriented)
├── tutorials/                  ← Step-by-step tutorials (learning-oriented)
├── api/                        ← API reference documentation
├── de/                         ← German (authoritative) documentation
│   ├── guides/                 ← German guides (QUICKSTART, USER_GUIDE …)
│   └── …
├── en/                         ← English documentation
├── security/                   ← Security, HSM, encryption guides
├── deployment/                 ← Docker, Kubernetes, on-premise
├── architecture/               ← System architecture docs
└── troubleshooting/            ← Problem-specific fix guides
```

---

## 🧭 Entry Points by Audience

### New users
1. [de/guides/QUICKSTART.md](de/guides/QUICKSTART.md) – 5-minute setup
2. [tutorials/GETTING_STARTED_TUTORIAL.md](tutorials/GETTING_STARTED_TUTORIAL.md) – 45-minute full walkthrough
3. [EXAMPLES_QUICKSTART.md](EXAMPLES_QUICKSTART.md) – hands-on examples tour

### Application developers
1. [api/API_REFERENCE.md](api/API_REFERENCE.md) – complete API reference
2. [tutorials/CRUD_TUTORIAL.md](tutorials/CRUD_TUTORIAL.md) – CRUD patterns
3. [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) – integration patterns
4. [tutorials/BEST_PRACTICES.md](tutorials/BEST_PRACTICES.md) – production patterns

### Operators / DevOps
1. [de/guides/guides_deployment.md](de/guides/guides_deployment.md) – deployment guide
2. [de/guides/ADMINISTRATOR_GUIDE.md](de/guides/ADMINISTRATOR_GUIDE.md) – admin guide
3. [security/HSM_PRODUCTION_SETUP.md](security/HSM_PRODUCTION_SETUP.md) – production security
4. [de/guides/guides_tls_setup.md](de/guides/guides_tls_setup.md) – TLS configuration

### Architects
1. [de/architecture/ARCHITECTURE_OVERVIEW.md](de/architecture/ARCHITECTURE_OVERVIEW.md) – system design
2. [tutorials/SCHEMA_DESIGN.md](tutorials/SCHEMA_DESIGN.md) – schema patterns
3. [de/guides/SYSTEM_ARCHITECT_GUIDE.md](de/guides/SYSTEM_ARCHITECT_GUIDE.md) – architect guide

---

## 📐 Documentation Conventions

| Convention | Meaning |
|------------|---------|
| `de/` prefix | German-language docs (authoritative for this project) |
| `en/` prefix | English translations (may lag behind German) |
| `⚠️ WARNING` callouts | Security-critical information — read before production use |
| `_Automatisch erzeugt` footer | Auto-generated README, content may be minimal |

---

## ✏️ Contributing to Docs

See [CONTRIBUTING.md](../CONTRIBUTING.md) and [governance/DOCS_PR_POLICY.md](governance/DOCS_PR_POLICY.md) for the full contribution workflow.

**Quick rules:**
- German docs go in `docs/de/`, English in `docs/en/`
- User-facing guides belong in `docs/guides/` or `docs/tutorials/`
- Update the relevant index file when adding a new page
- Run the doc linter before submitting: `python3 scripts/docs-lint.py <file>`

---

## 🔗 Key Navigation Hubs

- [00_DOCUMENTATION_INDEX.md](00_DOCUMENTATION_INDEX.md) – master index of all docs
- [DOCUMENTATION_HUB.md](DOCUMENTATION_HUB.md) – role-based navigation hub
- [CATEGORY_INDEX.md](CATEGORY_INDEX.md) – docs grouped by topic

