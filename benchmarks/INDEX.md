# 🗂️ CHIMERA Suite - Benchmark Navigation & Index

**Multi-Shard RAID Benchmarks - Part of CHIMERA Suite**

> **CHIMERA Suite** - _Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment_

---

## 📍 HAUPTDOKUMENTE

### 🚀 Starten Sie hier
1. **[COMPLETION_REPORT.md](COMPLETION_REPORT.md)** - ⭐ Lesen Sie ZUERST
   - Status: ✅ Alles fertig
   - Inhalt: Überblick, Quick-Start, nächste Schritte
   - Dauer: 5 Minuten

2. **[MULTI_SHARD_RAID_QUICKSTART.md](MULTI_SHARD_RAID_QUICKSTART.md)** - Schnell einsteigen
   - Für: Sofort Tests starten
   - Enthält: 5 Quick-Start Befehle
   - Dauer: 2 Minuten

### 📚 Detaillierte Dokumentation
3. **[MULTI_SHARD_RAID_BENCHMARK_PLAN.md](MULTI_SHARD_RAID_BENCHMARK_PLAN.md)** - Vollständiger Test-Plan
   - 8 Szenarien (S1-S8) mit Details
   - Workload-Spezifikationen
   - Expected Results für jedes Szenario

4. **[README.md](README.md)** - Umfassende Dokumentation
   - Features und Capabilities
   - Alle Befehle und Optionen
   - Troubleshooting Guide

### 📊 Status & Berichte
5. **[DEPLOYMENT_STATUS_REPORT.md](DEPLOYMENT_STATUS_REPORT.md)** - Aktuelle Situation
   - Infrastruktur-Status
   - Build-Fortschritt
   - System-Metriken

6. **[BENCHMARK_STATUS.md](BENCHMARK_STATUS.md)** - Komponenten-Übersicht
   - Alle implementierten Dateien
   - Verzeichnisstruktur
   - Was ist fertig

---

## 🛠️ VERWENDETE TOOLS & SKRIPTE

### Benchmark Ausführung
```
run_benchmark_simple.ps1      ← HAUPTSCRIPT (Windows PowerShell)
run_benchmark.ps1             ← Erweiterte Version (repariert)
run_benchmark.sh              ← Linux/Mac Version
quickstart.py                 ← Interaktive CLI-Menü
```

### Verwaltung
```
validate_infrastructure.py    ← Pre-Flight Checks
analyze_results.py            ← Ergebnis-Analyse
docker-compose.multi-shard-raid.yml ← Cluster-Konfiguration
```

### Data & Monitoring
```
scripts/load_test_data.py     ← Test-Daten Generator
monitoring/prometheus.yml      ← Metriken Config
```

---

## 🚀 SCHNELLANLEITUNG

### 1️⃣ Validierung (5 Minuten)
```powershell
cd c:\VCC\themis\benchmarks
python validate_infrastructure.py
```
✅ Sollte zeigen: "Alle Überprüfungen bestanden"

### 2️⃣ Einfacher Test (1-2 Stunden)
```powershell
powershell -ExecutionPolicy Bypass -File run_benchmark_simple.ps1 `
  -Scenario S1 -RaidLevel RAID10 -NumShards 3 -DurationHours 1
```
- S1 = Baseline Test
- RAID10 = Balanced RAID
- 3 Shards = Development Setup
- 1h = Schneller Test

### 3️⃣ Monitoring (während Test)
- Grafana: `http://localhost:3000` (admin/admin)
- Prometheus: `http://localhost:9090`

### 4️⃣ Ergebnisse (nach Test)
```powershell
python analyze_results.py
```
Zeigt: Tabellen, Vergleiche, Empfehlungen

---

## 📋 SZENARIO-ÜBERSICHT

| S# | Shards | RAID | Workload | Zeit | Für | Befehl |
|----|--------|------|----------|------|-----|--------|
| **S1** | 3 | RAID0 | OLTP | 4h | Quick-Test | `S1 RAID10` |
| **S3** | 3 | RAID5 | OLTP | 8h | Balanced | `S3 RAID5` |
| **S4** | 6 | RAID10 | Mixed | 12h | **Standard** ⭐ | `S4 RAID10` |
| **S5** | 12 | RAID6 | OLAP | 18h | Analytics | `S5 RAID6` |
| **S6** | 24 | RAID10 | TimeSeries | 24h | Enterprise | `S6 RAID10` |
| **S7** | 6 | RAID5 | VectorSearch | 10h | ML/AI | `S7 RAID5` |
| **S8** | 12 | RAID1 | Mixed | 16h | Multi-DC | `S8 RAID1` |

**Empfohlung:** Starten Sie mit S1 oder S4

---

## 🔧 HÄUFIGSTE BEFEHLE

### Benchmark Starten
```powershell
# S1 (1 Stunde) - für Tests
.\run_benchmark_simple.ps1 -Scenario S1 -RaidLevel RAID10 -NumShards 3 -DurationHours 1

# S4 (12 Stunden) - Production Standard
.\run_benchmark_simple.ps1 -Scenario S4 -RaidLevel RAID10 -NumShards 6 -DurationHours 12

# S5 (18 Stunden) - Data Warehouse
.\run_benchmark_simple.ps1 -Scenario S5 -RaidLevel RAID6 -NumShards 12 -DurationHours 18
```

### Cluster Verwalten
```powershell
# Cluster starten (ohne Benchmark)
docker-compose -f docker-compose.multi-shard-raid.yml --profile 6-shards up -d

# Health Check
curl http://localhost:8080/health

# Logs anzeigen
docker-compose -f docker-compose.multi-shard-raid.yml logs -f themis-shard-0

# Cluster stoppen
docker-compose -f docker-compose.multi-shard-raid.yml --profile 6-shards down -v
```

### Ergebnisse Analysieren
```bash
# Alle Ergebnisse anzeigen
python analyze_results.py

# Als CSV exportieren
python analyze_results.py --export results.csv

# RAID-Vergleich
python analyze_results.py --raid-comparison
```

---

## 📊 DATEISTRUKTUR

```
benchmarks/
├── RUN THIS FIRST
│   ├── COMPLETION_REPORT.md          ⭐ Status & Quick-Start
│   ├── MULTI_SHARD_RAID_QUICKSTART   Quick Start
│   └── validate_infrastructure.py     Pre-Flight Checks
│
├── EXECUTION SCRIPTS
│   ├── run_benchmark_simple.ps1       ← Windows HAUPTSCRIPT
│   ├── run_benchmark.ps1              ← Erweitert
│   ├── run_benchmark.sh               ← Linux
│   ├── quickstart.py                  ← Interaktives Menü
│   └── analyze_results.py             ← Ergebnis-Analyse
│
├── INFRASTRUCTURE
│   ├── docker-compose.multi-shard-raid.yml
│   ├── scripts/load_test_data.py
│   ├── monitoring/prometheus.yml
│   ├── monitoring/grafana/
│   └── run_multi_shard_raid_benchmark.py
│
├── DOCUMENTATION
│   ├── MULTI_SHARD_RAID_BENCHMARK_PLAN.md
│   ├── README.md
│   ├── DEPLOYMENT_STATUS_REPORT.md
│   ├── BENCHMARK_STATUS.md
│   ├── COMPLETION_REPORT.md
│   └── INDEX.md                       ← Sie sind hier
│
├── OUTPUT FOLDERS (nach Tests)
│   ├── results/
│   ├── logs/
│   └── data/
```

---

## 📈 MONITORING URLS

| Service | URL | Zugangsdaten |
|---------|-----|-------------|
| **Grafana** | http://localhost:3000 | admin / admin |
| **Prometheus** | http://localhost:9090 | — |
| **Shard 0** | http://localhost:8080/health | — |
| **Shard 1** | http://localhost:8081/health | — |
| **Shard 2** | http://localhost:8082/health | — |

---

## ✅ CHECKLISTE FÜR ERSTEN TEST

- [ ] Lesen Sie `COMPLETION_REPORT.md` (5 min)
- [ ] Führen Sie `validate_infrastructure.py` aus (2 min)
- [ ] Starten Sie S1 Test (1h) oder S4 Test (12h)
- [ ] Öffnen Sie Grafana während Test: http://localhost:3000
- [ ] Nach Test: Führen Sie `analyze_results.py` aus (2 min)
- [ ] Lesen Sie die Ergebnisse

**Gesamtdauer für kompletten Cycle:**
- Minimale Validierung: ~5 Minuten
- Mit S1 Test: ~1.5 Stunden
- Mit S4 Test: ~13 Stunden

---

## 🆘 HÄUFIGE FRAGEN

**F: Wo starte ich?**
A: Lesen Sie `COMPLETION_REPORT.md` und folgen Sie den Quick-Start Anweisungen.

**F: Welches Szenario sollte ich testen?**
A: Starten Sie mit **S1** (1h) zum Validieren oder **S4** (12h) für Production-Tests.

**F: Wie lange dauern Tests?**
A: S1 = 1-4h, S4 = 12h, S5 = 18h (abhängig von Hardware)

**F: Was wenn Port 8080 belegt ist?**
A: Das ist normal (ThemisDB-Server läuft bereits). Docker nutzt andere Ports.

**F: Wie kann ich Metriken während Tests sehen?**
A: Öffnen Sie Grafana: http://localhost:3000

**F: Wo sind die Ergebnisse?**
A: Im Verzeichnis `results/` nach Abschluss des Tests.

---

## 🔗 VERWANDTE LINKS

- **ThemisDB Hauptprojekt:** `c:\VCC\themis\`
- **HTTP Server Fix:** `src/server/http_server.cpp` (Zeilen 1008-1010)
- **DMS Client:** `ViewModels/DocumentBrowserViewModel.cs` (Auto-Refresh Event)
- **Multi-Arch Build:** Terminal `2d707fd6-3056-4d39-a731-7c80966af2ee`

---

## 📞 KONTAKT & SUPPORT

**Probleme?** → Siehe `README.md` Troubleshooting Sektion

**Mehr Infos?** → Siehe `MULTI_SHARD_RAID_BENCHMARK_PLAN.md`

**System-Infos?** → Führen Sie `validate_infrastructure.py` aus

---

**Version:** 1.0
**Aktualisiert:** 11. Dezember 2025
**Status:** ✅ Produktionsreif

🚀 **Sie sind bereit zu starten! Viel Erfolg mit den Benchmarks!**
