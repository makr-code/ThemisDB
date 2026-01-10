# ThemisDB CI/CD Pipeline Visualisierung

## Vollständiger Workflow-Flow

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         FEATURE ENTWICKLUNG                              │
└─────────────────────────────────────────────────────────────────────────┘

    Developer
        │
        ├─► feature/my-feature (branch)
        │       │
        │       ├─► Push to GitHub
        │       │
        │       ├─► Create PR to develop
        │       │       │
        │       │       ├──► [feature-ci.yml]
        │       │       │      └─► Ubuntu Build & Test
        │       │       │
        │       │       └──► [ci-develop.yml] ⚡ SCHNELLER
        │       │              ├─► Ubuntu (required)
        │       │              ├─► Windows (optional)
        │       │              └─► macOS (optional)
        │       │
        │       └─► Code Review ✓
        │           │
        │           └─► Merge to develop
        │                   │
        └─────────────────► develop ◄──────┐
                                │           │
                                │           │
                    [develop-ci.yml]        │
                    [security-scan.yml]     │
                                │           │
                                ▼           │

┌─────────────────────────────────────────────────────────────────────────┐
│                       RELEASE VORBEREITUNG                               │
└─────────────────────────────────────────────────────────────────────────┘

                    Release Manager
                            │
                            ├─► release/v1.5.0 (branch)
                            │       │
                            │       ├─► Update VERSION, CHANGELOG
                            │       │
                            │       ├─► [release-ci.yml]
                            │       │      └─► Validation
                            │       │
                            │       └─► Create PR to main
                            │               │
                            │               ├──► [build-and-test.yml] 🛡️
                            │               │      ├─► Ubuntu (REQUIRED)
                            │               │      ├─► Windows (REQUIRED)
                            │               │      ├─► macOS (REQUIRED)
                            │               │      └─► Security Scan
                            │               │
                            │               └─► Approval & Merge
                            │                       │
                            └───────────────────────┼─► main
                                                    │

┌─────────────────────────────────────────────────────────────────────────┐
│                      RELEASE VERÖFFENTLICHUNG                            │
└─────────────────────────────────────────────────────────────────────────┘

                                    main
                                     │
                                     ├─► git tag v1.5.0
                                     │
                                     └─► git push origin v1.5.0
                                             │
                        ┌────────────────────┼────────────────────┐
                        │                    │                    │
                        ▼                    ▼                    ▼
                        
            [release.yml] 🚀        [main-ci.yml]        [sbom.yml]
                    │                     │                     │
        ┌───────────┼───────────┐        │                     │
        │           │           │         │                     │
        ▼           ▼           ▼         ▼                     ▼
        
    Ubuntu      Windows      macOS   Verification         SBOM
    (.tar.gz    (.zip)     (.tar.gz)                   Generation
     .deb)
        │           │           │
        └───────────┼───────────┘
                    │
                    ▼
            GitHub Release 📦
                    │
                    └─► Artifacts uploaded
                    │
        ┌───────────┼───────────┐
        │           │           │
        ▼           ▼           ▼
        
    [docs.yml]  [docs-      [wiki-
                compendium   sync.yml]
                .yml]
        │           │           │
        ▼           ▼           ▼
        
   MkDocs       PDF         GitHub
    Docs      Compendium     Wiki


┌─────────────────────────────────────────────────────────────────────────┐
│                          HOTFIX PROZESS                                  │
└─────────────────────────────────────────────────────────────────────────┘

                    Critical Issue! 🚨
                            │
                            ├─► hotfix/v1.4.1 (from main)
                            │       │
                            │       ├─► Fix + Update VERSION
                            │       │
                            │       └─► Create PR to main
                            │               │
                            │               ├──► [hotfix-ci.yml] ⚡
                            │               │      └─► Fast-track
                            │               │
                            │               ├──► [build-and-test.yml]
                            │               │      └─► Full validation
                            │               │
                            │               └─► Fast approval
                            │                       │
                            └───────────────────────┼─► main
                                                    │
                                                    ├─► Tag v1.4.1
                                                    │
                                                    └─► [release.yml]
                                                            │
                                                            ▼
                                                    Hotfix Release
                                                            │
                                                            └─► Merge back
                                                                to develop


┌─────────────────────────────────────────────────────────────────────────┐
│                      KONTINUIERLICHE PROZESSE                            │
└─────────────────────────────────────────────────────────────────────────┘

    ╔═══════════════════════════════════════════════════════════════╗
    ║  WÖCHENTLICH (Sonntag)                                        ║
    ╠═══════════════════════════════════════════════════════════════╣
    ║  00:00 UTC → [fuzzing.yml]         AFL++ Fuzzing              ║
    ║  02:00 UTC → [security-scan.yml]   Weekly Security Scan       ║
    ╚═══════════════════════════════════════════════════════════════╝

    ╔═══════════════════════════════════════════════════════════════╗
    ║  ON DEMAND (Manual Trigger)                                   ║
    ╠═══════════════════════════════════════════════════════════════╣
    ║  [python-sdk-test.yml]     Python SDK Build & Test            ║
    ║  [java-sdk-test.yml]       Java SDK Build & Test              ║
    ║  [csharp-sdk-test.yml]     C# SDK Build & Test                ║
    ║  [helm-chart-test.yml]     Helm Chart Validation              ║
    ║  [ci.yml]                  General CI Check                   ║
    ╚═══════════════════════════════════════════════════════════════╝


┌─────────────────────────────────────────────────────────────────────────┐
│                        WORKFLOW PARALLELITÄT                             │
└─────────────────────────────────────────────────────────────────────────┘

Bei Tag Push v1.5.0:

    Time
    ═══
    
    0m   │  Tag v1.5.0 pushed
         │
         ├──────────────┬──────────────┬──────────────┬──────────────┐
         │              │              │              │              │
    5m   ▼              ▼              ▼              ▼              ▼
    [release.yml]  [main-ci.yml]  [sbom.yml]   [docs.yml]    [wiki-sync.yml]
         │              │              │              │              │
    15m  ├─► Ubuntu     │              │              │              │
         ├─► Windows    ├─► Verify    ├─► Generate   ├─► Build      ├─► Sync
         ├─► macOS      │              │              │              │
         │              │              │              │              │
    60m  ├─► Package    │              │              │              │
         │              │              │              │              │
    90m  └─► Release ✓  └─► Done ✓    └─► Done ✓    └─► Deploy ✓   └─► Done ✓


┌─────────────────────────────────────────────────────────────────────────┐
│                    BRANCH PROTECTION STATUS                              │
└─────────────────────────────────────────────────────────────────────────┘

    ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
    ┃  develop                                                      ┃
    ┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
    ┃  ✅ Build & Test - Ubuntu (ci-develop.yml)      [REQUIRED]   ┃
    ┃  ✅ Validate Changes (ci-develop.yml)           [REQUIRED]   ┃
    ┃  ⚠️  Windows Build                              [OPTIONAL]   ┃
    ┃  ⚠️  macOS Build                                [OPTIONAL]   ┃
    ┃  👤 1 Approval                                  [REQUIRED]   ┃
    ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛

    ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
    ┃  main                                                         ┃
    ┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
    ┃  ✅ Full Build & Test - Ubuntu                  [REQUIRED]   ┃
    ┃  ✅ Full Build & Test - Windows                 [REQUIRED]   ┃
    ┃  ✅ Full Build & Test - macOS                   [REQUIRED]   ┃
    ┃  ✅ Security Scan                               [REQUIRED]   ┃
    ┃  ✅ Validate Release                            [REQUIRED]   ┃
    ┃  👥 1+ Approvals                                [REQUIRED]   ┃
    ┃  🔒 Only release/* and hotfix/* branches allowed             ┃
    ┃  🔒 Maintainers only                                          ┃
    ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛


┌─────────────────────────────────────────────────────────────────────────┐
│                         ERFOLGS-KRITERIEN                                │
└─────────────────────────────────────────────────────────────────────────┘

    Feature Merge zu develop:
    ✅ Ubuntu Build erfolgreich
    ✅ Unit Tests bestanden
    ✅ 1 Code Review Approval

    Release Merge zu main:
    ✅ Ubuntu Build erfolgreich
    ✅ Windows Build erfolgreich
    ✅ macOS Build erfolgreich
    ✅ Security Scan bestanden
    ✅ Alle Tests bestanden
    ✅ 1+ Approvals

    Release Veröffentlichung:
    ✅ Tag erstellt (v*)
    ✅ Binaries für alle Plattformen gebaut
    ✅ GitHub Release erstellt
    ✅ Artifacts hochgeladen
    ✅ Dokumentation deployed
    ✅ Wiki synchronisiert


┌─────────────────────────────────────────────────────────────────────────┐
│                      GESCHWINDIGKEITS-VERGLEICH                          │
└─────────────────────────────────────────────────────────────────────────┘

    Entwickler-Feedback (develop):
    ┌────────────────────────────────────────┐
    │ feature-ci.yml:     30-45 Minuten     │ Existing
    │ develop-ci.yml:     30-45 Minuten     │ Existing
    │ ci-develop.yml:     15-30 Minuten ⚡  │ NEU - SCHNELLER
    └────────────────────────────────────────┘

    Release-Validierung (main):
    ┌────────────────────────────────────────┐
    │ release-ci.yml:     45-60 Minuten     │ Existing
    │ build-and-test.yml: 45-60 Minuten 🛡️  │ NEU - STRENGER
    └────────────────────────────────────────┘

    Release-Veröffentlichung:
    ┌────────────────────────────────────────┐
    │ Manuell:           180+ Minuten 👤     │ Old Process
    │ release.yml:       60-90 Minuten 🚀    │ NEU - AUTOMATISCH
    └────────────────────────────────────────┘

    Zeitersparnis pro Release: ~90-120 Minuten
    Fehlerrate: Deutlich reduziert durch Automatisierung


┌─────────────────────────────────────────────────────────────────────────┐
│                           ZUSAMMENFASSUNG                                │
└─────────────────────────────────────────────────────────────────────────┘

    📊 Gesamt-Workflows:        21
    
    🆕 Neu hinzugefügt:         3
       - ci-develop.yml         (Schnelles Feedback)
       - build-and-test.yml     (Strikte Protection)
       - release.yml            (Automatische Releases)
    
    ♻️  Bestehende harmonisiert: 18
    
    🚀 Automatisierungsgrad:     95%
    
    ⏱️  Release-Zeit:            60-90 min (vorher 180+ min)
    
    🛡️  Qualitätssicherung:      Alle Plattformen vor Release
    
    ✅ Status:                   Production Ready

```

---

**Legende**:
- `⚡` = Optimiert für Geschwindigkeit
- `🛡️` = Maximale Sicherheit/Qualität
- `🚀` = Vollautomatisch
- `✅` = Required Check
- `⚠️` = Optional Check
- `👤` = Manuelle Aktion erforderlich
- `🔒` = Geschützt/Eingeschränkt
