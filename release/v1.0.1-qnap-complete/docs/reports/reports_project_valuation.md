# ThemisDB – Projektkostenschätzung und Gesamtwertanalyse

**Version:** 1.0  
**Stand:** November 2025  
**Typ:** Wirtschaftliche Bewertung / Commercial Valuation

---

## 📋 Executive Summary

Dieses Dokument enthält eine detaillierte Kostenschätzung für die Entwicklung der ThemisDB als SaaS-Projekt nach Industriestandards. Die Berechnung basiert auf:

- Lines of Code (LoC) und Komplexitätsanalyse
- COCOMO II / Function Point Methodik
- Aktuelle Marktpreise für Enterprise-Softwareentwicklung (2024/2025)
- Vergleich mit ähnlichen Open-Source und kommerziellen Projekten

---

## 📊 Projektumfang (Scope Analysis)

### Codebase-Metriken

| Komponente | Lines of Code | Dateien | Sprache |
|------------|---------------|---------|---------|
| **Core Database Engine** | 136.643 | ~300+ | C++ |
| **Admin Tools (WPF)** | 12.820 | 9 Projekte | C# |
| **Client SDKs** | 27.243 | 6 SDKs | JS/TS/Python/Go/Rust/Java/C# |
| **Test Suite** | 42.097 | 175+ | C++ |
| **Dokumentation** | 113.311 | 279+ | Markdown |
| **Gesamt Sourcecode** | ~220.000 | 856 | Multi-Language |

### Feature-Komplexität

| Feature-Kategorie | Komplexitätsstufe | Aufwand (PM) |
|-------------------|-------------------|--------------|
| **Multi-Model Database Engine** | Sehr hoch | 24-36 |
| **ACID/MVCC Transaktionen** | Sehr hoch | 12-18 |
| **Graph Engine (BFS, Dijkstra, A*)** | Hoch | 8-12 |
| **HNSW Vector Search** | Hoch | 6-10 |
| **AQL Query Language & Parser** | Hoch | 8-12 |
| **Time-Series Engine (Gorilla)** | Mittel | 4-6 |
| **Secondary Index System (7 Typen)** | Hoch | 6-10 |
| **Enterprise Security Stack** | Sehr hoch | 12-18 |
| **Audit Logging & Compliance** | Hoch | 6-10 |
| **HSM/Vault Key Management** | Hoch | 4-8 |
| **REST API & HTTP Server** | Mittel | 4-6 |
| **Admin Tools Suite (7 WPF Apps)** | Mittel | 8-12 |
| **Client SDKs (6 Sprachen)** | Mittel | 6-10 |
| **Observability (Prometheus/OTel)** | Mittel | 3-5 |
| **Content Processing Pipeline** | Mittel | 4-6 |
| **Geo-Spatial Features** | Mittel | 3-5 |
| **CDC (Change Data Capture)** | Mittel | 3-5 |
| **Backup & Recovery** | Mittel | 2-4 |
| **Docker/Container Support** | Niedrig | 2-3 |
| **Dokumentation & Guides** | Mittel | 8-12 |

**Geschätzter Gesamtaufwand:** 128-193 Personenmonate (PM)

---

## 💰 Kostenberechnung

### Methodik: COCOMO II + Industry Standards

#### Stundensätze (Enterprise-Level, DACH-Region)

| Rolle | Stundensatz (€) | Anteil |
|-------|-----------------|--------|
| Senior C++ Developer | 150-200 | 40% |
| Security Architect | 180-250 | 15% |
| Database Engineer | 160-220 | 20% |
| DevOps Engineer | 130-180 | 10% |
| .NET Developer (WPF) | 120-160 | 8% |
| Technical Writer | 80-120 | 7% |
| **Gewichteter Durchschnitt** | **155 €/h** | 100% |

#### Berechnung

| Parameter | Wert | Berechnung |
|-----------|------|------------|
| Personenmonate (mittel) | 160 PM | (128+193)/2 |
| Stunden pro PM | 160 h | Industrie-Standard |
| Gesamtstunden | 25.600 h | 160 PM × 160 h |
| Stundensatz (gewichtet) | 155 €/h | Siehe oben |
| **Reine Entwicklungskosten** | **3.968.000 €** | 25.600 h × 155 € |

### Zusatzkosten (Project Overhead)

| Kategorie | Prozent | Betrag (€) |
|-----------|---------|------------|
| Projektmanagement | 15% | 595.200 |
| Quality Assurance (Testing) | 20% | 793.600 |
| Security Audits & Penetration Testing | 5% | 198.400 |
| Infrastruktur & CI/CD | 3% | 119.040 |
| Lizenzen & Tools | 2% | 79.360 |
| Schulung & Knowledge Transfer | 3% | 119.040 |
| **Summe Zusatzkosten** | **48%** | **1.904.640 €** |

---

## 📈 Gesamtkosten (Development Cost)

| Position | Betrag (€) |
|----------|------------|
| Entwicklungskosten (Core) | 3.968.000 |
| Zusatzkosten (Overhead) | 1.904.640 |
| **GESAMTENTWICKLUNGSKOSTEN** | **5.872.640 €** |

### Kostenspanne (Range)

| Szenario | Betrag (€) |
|----------|------------|
| **Minimum** (effizient, erfahrenes Team) | 4.500.000 |
| **Median** (realistisch) | 5.900.000 |
| **Maximum** (Enterprise mit allen Audits) | 7.500.000 |

---

## 🏆 Gesamtwert der ThemisDB (Asset Valuation)

### Wertberechnung nach verschiedenen Methoden

#### 1. Cost-Based Valuation (Entwicklungskosten)

| Methode | Multiplikator | Wert (€) |
|---------|---------------|----------|
| Reine Entwicklungskosten | 1.0x | 5.900.000 |
| Mit IP-Premium | 1.5x | 8.850.000 |
| Mit Marktpositionierung | 2.0x | 11.800.000 |

#### 2. Market Comparison (Vergleichbare Projekte)

| Vergleichsprojekt | Bewertung | Notizen |
|-------------------|-----------|---------|
| TimescaleDB | $110M (Series C) | Time-Series fokussiert |
| QuestDB | $15M (Series A) | Time-Series, kleiner Scope |
| Dgraph | $21.8M (gesamt) | Graph-fokussiert |
| Weaviate | $50M (Series B) | Vector Search fokussiert |
| ArangoDB | Privat | Multi-Model, ähnlich |

**ThemisDB Positionierung:** Multi-Model mit Enterprise Security Stack – vergleichbar mit ArangoDB-Scope, aber mit erweiterten Compliance-Features.

#### 3. Revenue-Based Valuation (SaaS-Projektion)

**Annahmen für SaaS-Modell:**

| Tier | Preis/Monat | Kunden (Jahr 3) | ARR |
|------|-------------|-----------------|-----|
| Starter | 499 € | 100 | 599.400 € |
| Professional | 1.499 € | 50 | 899.400 € |
| Enterprise | 4.999 € | 20 | 1.199.760 € |
| **Gesamt ARR (Jahr 3)** | | | **2.698.560 €** |

**SaaS-Bewertung (10x ARR):** ~27.000.000 €

---

## 📋 Detaillierte Aufwandsschätzung nach Komponenten

### Phase 1: Core Database Engine (18-24 Monate)

| Komponente | Aufwand (PM) | Kosten (€) |
|------------|--------------|------------|
| RocksDB Integration & Abstraction | 6 | 148.800 |
| ACID Transactions / MVCC | 12 | 297.600 |
| Multi-Model Storage Layer | 8 | 198.400 |
| Secondary Index System | 8 | 198.400 |
| Query Parser & Optimizer | 10 | 248.000 |
| **Summe Phase 1** | **44** | **1.091.200 €** |

### Phase 2: Advanced Features (12-18 Monate)

| Komponente | Aufwand (PM) | Kosten (€) |
|------------|--------------|------------|
| Graph Engine & Algorithms | 10 | 248.000 |
| HNSW Vector Search | 8 | 198.400 |
| AQL Language Implementation | 8 | 198.400 |
| Time-Series Engine | 5 | 124.000 |
| Geo-Spatial Features | 4 | 99.200 |
| **Summe Phase 2** | **35** | **868.000 €** |

### Phase 3: Enterprise Security (8-12 Monate)

| Komponente | Aufwand (PM) | Kosten (€) |
|------------|--------------|------------|
| Encryption Layer (AES-256-GCM) | 4 | 99.200 |
| RBAC & Access Control | 4 | 99.200 |
| Audit Logging & Compliance | 6 | 148.800 |
| HSM/Vault Integration | 6 | 148.800 |
| PKI & Certificate Management | 4 | 99.200 |
| Rate Limiting & Security Headers | 2 | 49.600 |
| **Summe Phase 3** | **26** | **644.800 €** |

### Phase 4: Operations & Tooling (6-10 Monate)

| Komponente | Aufwand (PM) | Kosten (€) |
|------------|--------------|------------|
| REST API & HTTP Server | 5 | 124.000 |
| Admin Tools Suite (7 WPF Apps) | 10 | 248.000 |
| Client SDKs (6 Sprachen) | 8 | 198.400 |
| Observability (Prometheus/OTel) | 4 | 99.200 |
| CDC & Backup/Recovery | 5 | 124.000 |
| Docker/Container Support | 2 | 49.600 |
| **Summe Phase 4** | **34** | **843.200 €** |

### Phase 5: Documentation & Quality (4-6 Monate)

| Komponente | Aufwand (PM) | Kosten (€) |
|------------|--------------|------------|
| API Documentation | 3 | 74.400 |
| User Guides & Tutorials | 4 | 99.200 |
| Security Documentation | 3 | 74.400 |
| Test Suite (85%+ Coverage) | 8 | 198.400 |
| Performance Benchmarks | 2 | 49.600 |
| **Summe Phase 5** | **20** | **496.000 €** |

---

## 🔄 Vergleich: Build vs. Buy

### Option 1: Eigenentwicklung (wie ThemisDB)

| Aspekt | Details |
|--------|---------|
| **Kosten** | 4.5 - 7.5 Mio € |
| **Zeitraum** | 3-4 Jahre |
| **Team** | 8-12 Entwickler |
| **Risiken** | Technisch hoch, Personal-Abhängigkeit |
| **Vorteile** | Volle Kontrolle, IP-Ownership, Differenzierung |

### Option 2: Commercial Database + Extensions

| Aspekt | Details |
|--------|---------|
| **Kosten** | 200k-500k €/Jahr (Lizenz) + 500k-1M € (Integration) |
| **Zeitraum** | 6-12 Monate |
| **Team** | 2-4 Entwickler |
| **Risiken** | Vendor Lock-in, Lizenzkosten, Feature-Einschränkungen |
| **Vorteile** | Schneller Time-to-Market, Support, Updates |

### Option 3: Open-Source + Enterprise Support

| Aspekt | Details |
|--------|---------|
| **Kosten** | 100k-300k €/Jahr (Support) + 300k-600k € (Customization) |
| **Zeitraum** | 6-18 Monate |
| **Team** | 3-6 Entwickler |
| **Risiken** | Community-Abhängigkeit, Security-Patches |
| **Vorteile** | Flexibilität, keine Lizenzkosten, Community |

---

## 📊 Zusammenfassung der Gesamtwerte

| Bewertungsmethode | Wert (€) |
|-------------------|----------|
| **Entwicklungskosten (Minimum)** | 4.500.000 |
| **Entwicklungskosten (Median)** | 5.900.000 |
| **Entwicklungskosten (Maximum)** | 7.500.000 |
| **Mit IP-Premium (1.5x)** | 8.850.000 |
| **Marktbewertung (2.0x)** | 11.800.000 |
| **SaaS Revenue-Projektion (10x ARR Jahr 3)** | 27.000.000 |

### Empfohlene Gesamtbewertung

| Szenario | Wert (€) | Begründung |
|----------|----------|------------|
| **Konservativ** | 5.900.000 | Reine Entwicklungskosten |
| **Realistisch** | 8.500.000 | Mit IP und Tech-Stack Premium |
| **Optimistisch** | 15.000.000 | Mit Markt- und SaaS-Potenzial |

---

## 🎯 Schlussfolgerung

Die ThemisDB repräsentiert einen signifikanten Engineering-Aufwand und technischen Wert:

- **~220.000 Lines of Code** in einer Multi-Language Codebase
- **160+ Personenmonate** geschätzte Entwicklungszeit
- **85%+ Test Coverage** mit 303 bestandenen Tests
- **20+ Compliance-Standards** dokumentiert und implementiert
- **7 Admin-Tools** für Enterprise-Operationen
- **6 Client SDKs** für breite Adoption

### Gesamtwert der ThemisDB

| Kategorie | Betrag |
|-----------|--------|
| **Minimum (Entwicklungskosten)** | **4.500.000 €** |
| **Median (realistisch)** | **5.900.000 €** |
| **Mit IP-Premium** | **8.850.000 €** |
| **Marktwert (optimistisch)** | **15.000.000 €** |

---

## 📝 Methodik & Quellen

### Verwendete Bewertungsmethoden

1. **COCOMO II** - Constructive Cost Model für Software-Schätzungen
2. **Function Point Analysis** - ISO/IEC 20926
3. **Industry Benchmarks** - Gartner, IDC, Stack Overflow Survey
4. **Market Comparisons** - Crunchbase, PitchBook Daten

### Stundensatz-Quellen

- StepStone Gehaltsreport 2024 (DACH)
- Gulp IT-Freelancer Studie 2024
- Hays Technology Salary Guide 2024
- Indeed Germany Salary Data

### Vergleichbare Transaktionen

- MongoDB IPO (2017): $1.2B initial valuation
- Elastic IPO (2018): $2.4B initial valuation
- Cockroach Labs Series F (2024): $5B valuation
- Databricks Series I (2024): $43B valuation

---

**Letzte Aktualisierung:** November 2025  
**Dokumentverantwortlicher:** ThemisDB Finance Team  
**Disclaimer:** Diese Schätzung dient Planungszwecken und stellt keine verbindliche Bewertung dar.
