# ThemisDB Performance Evaluation Paper

IEEE-format LaTeX source for:

> **"ThemisDB: Hardware-Normalized Performance Evaluation of a Hybrid Multi-Model Database System"**

## Files

| File | Description |
|------|-------------|
| `themisdb_performance_evaluation.tex` | Main LaTeX source (IEEEtran conference format) |
| `references.bib` | BibTeX bibliography (35 entries) |
| `themisdb_performance_evaluation.md` | Markdown export (human-readable, no LaTeX required) |
| `Makefile` | Convenience build targets |

## Build Requirements

- `pdflatex` (TeX Live 2020+ or MiKTeX)
- `bibtex`
- `IEEEtran.cls` — included in TeX Live; download from
  <https://ctan.org/pkg/ieeetran> if missing

## Building the PDF

```bash
# Using make
make

# Manual build sequence
pdflatex themisdb_performance_evaluation
bibtex   themisdb_performance_evaluation
pdflatex themisdb_performance_evaluation
pdflatex themisdb_performance_evaluation
```

Run `pdflatex` twice after `bibtex` to resolve all cross-references
and the bibliography.

## Source Data

All performance figures are sourced from
[`PERFORMANCE_EXPECTATIONS.md`](../../PERFORMANCE_EXPECTATIONS.md)
(sections 1, 1.7, 1.7.5 – 1.7.11) and the hardware baseline JSON at
`build-msvc-ninja-release/logs/hardware_baseline/hardware_baseline_gtest_1775806092.json`.

## Paper Structure

| Section | Content |
|---------|---------|
| Abstract | ThemisDB overview, 1 078 benchmark cases, HW-normalized model, key findings |
| §1 Introduction | Multi-model DB challenge, five contributions |
| §2 System Architecture Overview | Layer/module/technology table, SLO inventory (28 modules) |
| §3 Benchmark Methodology | Google Benchmark infra, HW baseline capture, CHIMERA suite |
| §4 HW-Normalized Efficiency Model | 8 factors, 6 workload-class formulas, efficiency computation, v1 calibration rules |
| §5 Experimental Results | Longitudinal KPI table, HW baseline, efficiency v0, HW-neutral scores, correlation matrix, raw benchmarks (9 subsections), regression overview, CHIMERA comparison |
| §6 Analysis and Discussion | Root-cause analysis, coverage gap table (29 gaps / 5 categories), meta-causes, known gaps D-1..D-7, CI governance |
| §7 Threats to Validity | Internal / external / construct validity |
| §8 Related Work | Multi-model surveys, STREAM/TPC/ANN benchmarks, RocksDB, Raft |
| §9 Conclusions | Key findings (5), future work |
| References | 35 BibTeX entries (IEEE/ACM/arXiv) |
| Appendix A | Efficiency formula derivation (HW factor definition, class capability, HW-neutral score, recalibration trigger) |
| Appendix B | CHIMERA workload schema (JSON field table) |
