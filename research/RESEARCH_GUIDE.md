# Research Guide — How to Document Research in ThemisDB

This guide explains **when** and **how** to document research sources that influence ThemisDB's implementation.

---

## When Should You Document a Source?

Document a source whenever you:

1. **Base an algorithm or data structure on a scientific paper**  
   Example: Using HNSW for vector indexing → document the HNSW paper.

2. **Adopt a best practice from another open-source project or industry reference**  
   Example: Zero-copy I/O from io_uring best practices.

3. **Make a significant architecture decision** (especially when alternatives were evaluated)  
   Example: Choosing HNSW over FAISS for the index layer.

4. **Update your understanding of the state of the art** in an area relevant to ThemisDB  
   Example: Quarterly landscape review of ANN algorithms.

**Rule of thumb:** If you researched something before writing code, document what you found.

---

## Canonical Clusters & Document States

Use these clusters consistently:

- `research/manuscripts/` → canonical ThemisDB-authored manuscript portfolio
- `research/papers/` → canonical paper entries
- `research/architecture_decisions/` → canonical ADR decisions
- `research/experiments/` → canonical validation evidence
- `research/*_DRAFT.md` (top-level legacy) → working manuscripts only

Use one of these states in draft/index tracking:

- `ACTIVE_DRAFT`
- `SUPERSEDED_DRAFT` (canonical successor exists)
- `ARCHIVE_CANDIDATE` (obsolete, only historical value)

If a canonical successor exists, mark the old draft as `SUPERSEDED_DRAFT` in `research/README.md`.

---

## Step-by-Step Workflow

### Step 1 — Identify the source type

| Source Type | Where to document |
|-------------|-------------------|
| ThemisDB-authored manuscript / publication draft | `research/manuscripts/` |
| Scientific paper (journal, conference, ArXiv) | `research/papers/` |
| Best practice (open-source project, blog, standard) | `research/best_practices/` |
| Architecture / design decision | `research/architecture_decisions/` |
| Broad landscape review | `research/stand_der_technik/` |

### Step 2 — Create the documentation file

Copy the appropriate template:

```bash
# For a ThemisDB manuscript
cp research/templates/MANUSCRIPT_TEMPLATE.md \
   research/manuscripts/<cluster>/<paper_name>.md

# For a paper
cp research/papers/_template_paper.md \
   research/papers/<topic>_<year>.md

# For a best practice
cp research/best_practices/_template_best_practice.md \
   research/best_practices/<short_name>.md

# For an architecture decision
cp research/architecture_decisions/_template_decision.md \
   research/architecture_decisions/adr_<NNN>_<short_title>.md
```

Fill in **all required fields**. Empty fields should be filled or removed — no placeholder text in committed files.

### Step 3 — Register portfolio placement

For ThemisDB-authored manuscripts:

1. add or update the manuscript in `research/manuscripts/README.md`
2. place it in the correct cluster (`flagship`, `systems`, `retrieval_rag`, `llm_runtime_training`, `distributed_consistency_resilience`, `geo_temporal_streaming`, `security_governance_ethics`, `verticals`)
3. record overlap / successor / predecessor relations when the topic intersects an existing draft

### Step 4 — Link it in the affected module README

Add a row to the *Wissenschaftliche Grundlagen & Einflüsse* table in the relevant `src/<module>/README.md`:

```markdown
## 🔬 Wissenschaftliche Grundlagen & Einflüsse

| Kategorie | Quelle | Status | Links |
|-----------|--------|--------|-------|
| **Paper** | [HNSW (2018)](../../research/papers/hnsw_efficient_ann_2018.md) | ✅ v1.4.1+ | [Influence Index](../../research/implementation_influence/by_module.md#src-index) |
| **Best Practice** | [Zero-Copy I/O](../../research/best_practices/zero_copy_io.md) | ✅ v1.4.1+ | - |
| **Architecture** | [ADR-001: Vector Index Choice](../../research/architecture_decisions/adr_001_vector_index_hnsw_vs_faiss.md) | ✅ v1.4.1+ | - |
```

If the module README does not yet have this section, add it before the last section.

### Step 5 — Update the master influence index

Add a row to the influence matrix in  
[`research/implementation_influence/README.md`](implementation_influence/README.md).

### Step 6 — Use the correct commit message prefix

```
ref(research): Add [Source Title] to [Module Name]
```

Example:
```
ref(research): Add HNSW (2018) to src/index/
```

### Step 7 — Mark draft status clearly

When touching draft files, also update `research/README.md`:

1. Add/update status in the draft lifecycle table
2. Link the canonical successor (if available)
3. Keep obsolete drafts discoverable, but clearly non-canonical

---

## PR Checklist

Before submitting a PR that contains algorithm/design work, confirm:

- [ ] Does this PR base work on a scientific paper, best practice, or architecture decision?
- [ ] If yes: Is the research file created in the correct canonical cluster (`research/manuscripts/`, `research/papers/`, `research/best_practices/`, `research/architecture_decisions/`)?
- [ ] Is the module README updated with the *Wissenschaftliche Grundlagen & Einflüsse* section?
- [ ] Is `research/implementation_influence/README.md` updated?

**For PRs that introduce or replace an algorithm / method:**

- [ ] Was the [Algorithm Validation Process](ALGORITHM_VALIDATION_PROCESS.md) applied? (6 steps)
- [ ] Is a Ziel-ID from `PERFORMANCE_EXPECTATIONS.md §1.2` referenced in the PR and ROADMAP?
- [ ] Is the baseline frozen in `benchmarks/baselines/<modul>/`?
- [ ] Does a CI-Gate exist for the affected Ziel-ID?
- [ ] Is the ADR created under `research/architecture_decisions/`?

---

## Examples

### Example: Implementing HNSW-based vector index

1. Create `research/papers/hnsw_efficient_ann_2018.md` from the paper template
2. Fill in author (Malkov & Yashunin), ArXiv link, tags `vector-search graph-index`
3. Add to `src/index/README.md` under *Wissenschaftliche Grundlagen*
4. Add row to `research/implementation_influence/README.md`
5. Commit: `ref(research): Add HNSW (2018) to src/index/`

### Example: Architecture decision between RocksDB and LMDB

1. Create `research/architecture_decisions/adr_002_storage_engine_choice.md`
2. Document context, options considered, decision (RocksDB), and trade-offs
3. Add to `src/storage/README.md` under *Wissenschaftliche Grundlagen*
4. Add row to `research/implementation_influence/README.md`

---

## Automation

The following scripts help maintain the research documentation:

| Script | Purpose | When to Run |
|--------|---------|------------|
| `scripts/validate_research_links.py` | Finds code comments referencing papers/sources without a documentation file | Every PR (also runs in CI) |
| `scripts/generate_research_index.py` | Re-generates `implementation_influence/by_module.md`, `by_paper.md`, `by_version.md` | After adding/updating research entries |
| `scripts/validate_research_metadata.py` | Validates that all research files have required frontmatter fields | Every PR (also runs in CI) |

---

## Algorithm Validation Process

Wenn eine bestehende Methode oder ein Algorithmus in einem Modul durch einen besseren ersetzt
werden soll, muss das **6-Schritte-Framework** vollständig durchlaufen werden:

1. Ziel-ID + SLO aus `PERFORMANCE_EXPECTATIONS.md` fixieren
2. Baseline (Benchmark-JSON + HW-Profil) einfrieren
3. ≥ 5 Kandidaten aus aktueller Literatur sammeln (Research-Steckbrief je Kandidat)
4. Experiment mit Welch's t-Test standardisieren (P50/P95/P99 + Throughput + RSS)
5. CI-Gate in `benchmark_target_mapping.json` + Workflow-Datei erzwingen
6. ADR + Research-Dokumentation abschließen

Vollständige Anleitung: **[ALGORITHM_VALIDATION_PROCESS.md](ALGORITHM_VALIDATION_PROCESS.md)**  
Fertige Prompt-Templates: **[PROMPTING_TEMPLATES.md](PROMPTING_TEMPLATES.md)**  
Framework-ADR: **[ADR-009](architecture_decisions/adr_009_algorithm_validation_framework.md)**

> **Praktische Regel:** Eine Optimierungsidee gilt erst als "gewonnen", wenn alle 6 Schritte
> abgeschlossen sind — Benchmarkbar → Reproduzierbar → CI-gated → Dokumentiert → Roadmap-verankert.

---

## Directory Structure Reference

```
research/
├── README.md                              ← You are here (overview)
├── RESEARCH_GUIDE.md                      ← This file (contributor guide)
├── ALGORITHM_VALIDATION_PROCESS.md        ← 6-Schritte-Framework für Algorithmus-Validierung
├── PROMPTING_TEMPLATES.md                 ← Modul-spezifische + Cross-Module-Prompt-Templates
├── manuscripts/
│   ├── README.md                          ← Canonical portfolio for ThemisDB-authored manuscripts
│   ├── flagship/
│   ├── systems/
│   ├── retrieval_rag/
│   ├── llm_runtime_training/
│   ├── distributed_consistency_resilience/
│   ├── geo_temporal_streaming/
│   ├── security_governance_ethics/
│   └── verticals/
├── papers/
│   ├── README.md                          ← Index of all papers
│   ├── TEMPLATES.md                       ← Required fields & formatting
│   └── _template_paper.md                 ← Copy-paste starter
├── templates/
│   ├── MANUSCRIPT_TEMPLATE.md             ← Default template for ThemisDB manuscripts
│   ├── EXPERIMENT_TEMPLATE.md             ← Experiment protocol companion
│   └── ARTIFACT_CHECKLIST.md              ← Submission-candidate evidence checklist
├── best_practices/
│   ├── README.md
│   ├── TEMPLATES.md
│   └── _template_best_practice.md
├── stand_der_technik/
│   ├── README.md
│   ├── quarterly_updates.md               ← Process & schedule
│   └── 2026_q1_landscape.md
├── architecture_decisions/
│   ├── README.md
│   ├── decision_log.md                    ← Chronological log
│   ├── _template_decision.md
│   └── adr_009_algorithm_validation_framework.md ← Framework-ADR
├── experiments/                           ← Experiment-Protokolle (angelegt bei Bedarf)
│   └── <ziel_id>/
│       ├── <kandidat>_<datum>.json        ← Benchmark-Rohdaten
│       └── protokoll_<kandidat>.md        ← Experiment-Protokoll
└── implementation_influence/
    ├── README.md                          ← Master matrix
    ├── by_module.md                       ← Grouped by src module
    ├── by_paper.md                        ← Grouped by paper/source
    └── by_version.md                      ← Grouped by ThemisDB version
```
