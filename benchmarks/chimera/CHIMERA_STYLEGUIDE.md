# CHIMERA Suite Style Guide

**Version:** 1.0.0  
**Last Updated:** April 2026  
**Status:** Official Branding Guidelines

---

## Overview

This document defines the visual identity and branding guidelines for the CHIMERA Suite. These guidelines ensure vendor neutrality, scientific credibility, and accessibility in all CHIMERA materials.

---

## Core Principles

### 1. Vendor Neutrality
- **No proprietary branding** from any database vendor
- **No vendor colors** or marketing materials
- **Equal treatment** of all database systems
- **Alphabetical ordering** or performance-based (no preferential placement)

### 2. Scientific Credibility
- **Professional appearance** suitable for academic papers
- **IEEE/ACM compliant** formatting
- **Proper citations** for all methods
- **Data-driven design** without visual manipulation

### 3. Accessibility
- **Color-blind friendly** palettes only
- **High contrast** for readability
- **Clear typography** with adequate sizing
- **Screen-reader compatible** HTML reports

---

## Brand Identity

### Name
**Full Name:** CHIMERA Suite  
**Acronym:** CHIMERA  
**Expansion:** **C**omprehensive, **H**onest, **I**mpartial **M**etrics for **E**mpirical **R**eporting and **A**nalysis

### Tagline
*"Honest metrics for everyone"*

### Mission Statement
"CHIMERA Suite provides vendor-neutral, scientifically rigorous database benchmarking tools that ensure fair, transparent, and accessible performance comparisons."

---

## Visual Identity

### Logo
CHIMERA does **not** use a logo to maintain vendor neutrality.

Alternative identification:
- Text-based heading: "CHIMERA Suite"
- Tagline: *"Honest metrics for everyone"*
- Badge style (optional):
  ```
  ┌─────────────────────────┐
  │   CHIMERA Suite v1.0    │
  │ Honest metrics for all  │
  └─────────────────────────┘
  ```

### Colors

#### Primary Palette: Paul Tol's Muted Scheme
Chosen for professional appearance and color-blind accessibility:

| Color Name | Hex Code  | RGB             | Usage                    |
|------------|-----------|-----------------|--------------------------|
| Indigo     | `#332288` | (51, 34, 136)   | Primary accent, headers  |
| Cyan       | `#88CCEE` | (136, 204, 238) | Secondary accent, links  |
| Teal       | `#44AA99` | (68, 170, 153)  | Success indicators       |
| Green      | `#117733` | (17, 119, 51)   | Positive metrics         |
| Sand       | `#DDCC77` | (221, 204, 119) | Neutral backgrounds      |
| Rose       | `#CC6677` | (204, 102, 119) | Warning indicators       |
| Wine       | `#882255` | (136, 34, 85)   | Critical indicators      |
| Grey       | `#DDDDDD` | (221, 221, 221) | Borders, dividers        |

#### Chart Palette: Okabe-Ito
Used for all data visualizations (optimized for color blindness):

| Color Name      | Hex Code  | RGB             | Usage                    |
|-----------------|-----------|-----------------|--------------------------|
| Blue            | `#0072B2` | (0, 114, 178)   | System A in comparisons  |
| Vermillion      | `#D55E00` | (213, 94, 0)    | System B in comparisons  |
| Bluish Green    | `#009E73` | (0, 158, 115)   | System C in comparisons  |
| Yellow          | `#F0E442` | (240, 228, 66)  | System D in comparisons  |
| Sky Blue        | `#56B4E9` | (86, 180, 233)  | System E in comparisons  |
| Reddish Purple  | `#CC79A7` | (204, 121, 167) | System F in comparisons  |
| Orange          | `#E69F00` | (230, 159, 0)   | System G in comparisons  |

**Never use:**
- Red (#FF0000) - too aggressive, not color-blind safe
- Pure green (#00FF00) - problematic for deuteranopia
- Vendor-specific brand colors

---

## Typography

### Font Families

#### Primary Font (Digital)
- **Sans-Serif**: Segoe UI, Helvetica Neue, Arial, sans-serif
- **Usage**: Headers, body text, UI elements
- **Rationale**: Clean, modern, widely available

#### Code Font
- **Monospace**: Consolas, Monaco, 'Courier New', monospace
- **Usage**: Code blocks, configuration examples
- **Rationale**: Clear distinction, good readability

#### Academic Font (Print)
- **Serif**: Times New Roman, Georgia, serif
- **Usage**: PDF reports, academic papers
- **Rationale**: Traditional, professional for print

### Font Sizing

| Element           | Size (px) | Weight  | Line Height |
|-------------------|-----------|---------|-------------|
| H1 (Main Title)   | 32        | Bold    | 1.2         |
| H2 (Section)      | 24        | Bold    | 1.3         |
| H3 (Subsection)   | 18        | Semibold| 1.4         |
| Body Text         | 14        | Regular | 1.6         |
| Small Text        | 12        | Regular | 1.5         |
| Code              | 13        | Regular | 1.4         |

---

## Report Design

### HTML Reports

#### Structure
```
┌─────────────────────────────────────┐
│  CHIMERA Suite Report Header        │
│  [Neutrality Seal]                  │
├─────────────────────────────────────┤
│  Executive Summary                  │
│  - System descriptions              │
│  - Key metrics                      │
├─────────────────────────────────────┤
│  Statistical Analysis               │
│  - Descriptive statistics           │
│  - Hypothesis tests                 │
│  - Effect sizes                     │
├─────────────────────────────────────┤
│  Visualizations                     │
│  - Box plots                        │
│  - Distribution charts              │
├─────────────────────────────────────┤
│  Methodology                        │
│  - Test descriptions                │
│  - Statistical methods              │
├─────────────────────────────────────┤
│  References                         │
│  - IEEE citations                   │
└─────────────────────────────────────┘
```

#### Color Scheme
- **Background**: White (#FFFFFF)
- **Text**: Dark grey (#2C3E50)
- **Section Headers**: Indigo (#332288)
- **Links**: Cyan (#88CCEE)
- **Borders**: Light grey (#DDDDDD)

#### Spacing
- **Section margin**: 2rem
- **Paragraph spacing**: 1rem
- **Table padding**: 0.75rem
- **Chart margins**: 1.5rem

### CSV Reports

Format: RFC 4180 compliant
- UTF-8 encoding
- Comma delimiter
- Quoted strings when containing commas
- Header row included
- Consistent decimal places (4 significant figures)

### PDF Reports

Generated from HTML using WeasyPrint
- A4 page size (210mm × 297mm)
- Margins: 2cm on all sides
- Header with page numbers
- Footer with generation date
- Serif fonts for better print readability

---

## Chart Design

### Box Plots
- **Width**: 800px standard
- **Height**: 500px standard
- **Grid**: Light grey (#CCCCCC), alpha 0.3
- **Whiskers**: 1.5 × IQR
- **Outliers**: Marked with circles
- **Box fill**: 70% opacity
- **Border**: 2px solid
- **Axes**: 1px solid black

### Bar Charts
- **Bar width**: 0.7 (70% of available space)
- **Spacing**: 0.3 between bars
- **Colors**: From Okabe-Ito palette
- **Border**: None
- **Grid**: Horizontal only, light grey

### Line Charts
- **Line width**: 2px
- **Marker size**: 6px
- **Grid**: Both horizontal and vertical
- **Legend**: Outside plot area (top-right)

### Annotations
- **Font size**: 10px
- **Color**: Dark grey (#2C3E50)
- **Background**: Semi-transparent white
- **Arrows**: 1px, grey

---

## Language Guidelines

### Tone
- **Objective**: No marketing language
- **Professional**: Suitable for academic papers
- **Accessible**: Clear to non-experts
- **Precise**: Technical accuracy required

### Terminology

**Use:**
- "System A outperforms System B by X%"
- "Statistically significant difference (p < 0.05)"
- "Effect size: Cohen's d = 0.8 (large)"
- "Confidence interval: [lower, upper]"

**Avoid:**
- "Winner" or "loser"
- "Best" without qualification
- "Revolutionary" or marketing superlatives
- Vendor-specific terminology

### System Naming
- **Generic identifiers**: System A, System B, Database 1, Database 2
- **Descriptive types**: Multi-model database, Graph database, Relational database
- **Version included**: SystemName v2.1.0
- **No marketing names**: Avoid "Pro", "Enterprise", "Ultimate"

---

## Badge System

### Neutrality Seal
Display on all reports:
```
┌────────────────────────────────┐
│   ✓ VENDOR NEUTRAL             │
│   ✓ IEEE COMPLIANT             │
│   ✓ COLOR-BLIND FRIENDLY       │
│   ✓ STATISTICALLY RIGOROUS     │
└────────────────────────────────┘
```

### Format Badges
Use standard shields.io style:
- [![IEEE Compliant](https://img.shields.io/badge/IEEE-Compliant-blue)](https://www.ieee.org/)
- [![Color Blind Friendly](https://img.shields.io/badge/ColorBlind-Friendly-green)](https://jfly.uni-koeln.de/color/)
- [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## File Naming Conventions

### Reports
- Format: `CHIMERA_report_YYYYMMDD_HHMM.{html|csv|pdf}`
- Example: `CHIMERA_report_20260119_1430.html`

### Configurations
- Format: `chimera_config_{description}.{yaml|toml}`
- Example: `chimera_config_ycsb_comparison.yaml`

### Data Files
- Format: `chimera_data_{system}_{date}.csv`
- Example: `chimera_data_system_a_20260119.csv`

---

## Version Tagging

### Git Tags
- Format: `chimera-v{major}.{minor}.{patch}`
- Examples: `chimera-v1.0.0`, `chimera-v1.1.0`, `chimera-v2.0.0`
- **Not**: `themis-bench-v1.0.0` (avoid mixing brands)

### Release Artifacts
- Format: `chimera-suite-v{version}.{tar.gz|zip}`
- Example: `chimera-suite-v1.0.0.tar.gz`

---

## Dos and Don'ts

### ✅ DO
- Use established color palettes (Okabe-Ito, Paul Tol)
- Sort systems alphabetically or by performance
- Include statistical significance tests
- Cite all methodologies
- Provide complete reproducibility information
- Make reports accessible to color-blind users

### ❌ DON'T
- Use vendor logos or brand colors
- Declare a "winner" without context
- Manipulate chart scales to favor systems
- Hide statistical methods or parameters
- Use jargon without explanation
- Make inaccessible visualizations

---

## Compliance Checklist

Before releasing any CHIMERA material, verify:

- [ ] No vendor logos or branding present
- [ ] Colors from approved palettes only
- [ ] System names normalized and neutral
- [ ] Statistical methods properly cited
- [ ] Charts are color-blind accessible
- [ ] All data sources disclosed
- [ ] Methodology fully transparent
- [ ] IEEE/ACM formatting followed
- [ ] License information included
- [ ] Version number present

---

## References

1. **Okabe, M., & Ito, K. (2008)**. Color Universal Design. J*FLY. https://jfly.uni-koeln.de/color/
2. **Tol, P. (2021)**. Colour Schemes. https://personal.sron.nl/~pault/
3. **IEEE Std 730-2014**. IEEE Standard for Software Quality Assurance Processes.
4. **Tufte, E. R. (2001)**. The Visual Display of Quantitative Information. Graphics Press.

---

## Revision History

| Version | Date       | Changes                                    |
|---------|------------|--------------------------------------------|
| 1.0.0   | 2026-01-20 | Initial style guide with vendor neutrality |

---

**CHIMERA Suite Style Guide v1.0.0**  
*Maintaining scientific integrity through design*
