# ThemisDB Fuzzing Infrastructure

Dieses Verzeichnis enthält die AFL++ Fuzzing-Infrastruktur für ThemisDB.

## Übersicht

```
fuzz/
├── aflplusplus-config.json     # AFL++ Konfiguration (JSON)
├── aflplusplus-config.yaml     # AFL++ Konfiguration (YAML)
├── harnesses/                  # Fuzzing Harnesses (C++)
├── corpus/                     # Eingabe-Korpora
│   ├── aql/                   # AQL Query Seeds
│   ├── json/                  # JSON Document Seeds
│   ├── crypto/                # Crypto Test Vectors
│   ├── protocol/              # Network Protocol Seeds
│   ├── storage/               # Storage Engine Seeds
│   ├── auth/                  # Authentication Seeds
│   └── pii/                   # PII Redaction Seeds (email, SSN, IBAN, credit card, phone)
├── dictionaries/              # AFL++ Dictionaries
├── crashes/                   # Crash-Dateien (Output)
├── hangs/                     # Hang-Dateien (Output)
├── coverage/                  # Coverage Reports
└── reports/                   # Fuzzing Reports
```

## Schnellstart

### 1. AFL++ installieren

```bash
# Ubuntu/Debian
sudo apt-get install afl++

# Oder von Source
git clone https://github.com/AFLplusplus/AFLplusplus.git
cd AFLplusplus
make distrib
sudo make install
```

### 2. ThemisDB mit Instrumentation bauen

```bash
mkdir build-fuzz && cd build-fuzz
CC=afl-clang-lto CXX=afl-clang-lto++ cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_FUZZING=ON \
  -DENABLE_SANITIZERS=ON
make fuzz_targets
```

### 3. Fuzzing starten

```bash
# Einzelner Fuzzer
afl-fuzz -i fuzz/corpus/aql -o fuzz/output/aql \
  -x fuzz/dictionaries/aql.dict \
  -- ./build-fuzz/fuzz/bin/aql_parser_harness @@

# Paralleles Fuzzing (8 Cores)
afl-fuzz -M main -i fuzz/corpus/aql -o fuzz/output/aql -- ./harness @@
afl-fuzz -S sec1 -i fuzz/corpus/aql -o fuzz/output/aql -- ./harness @@
afl-fuzz -S sec2 -i fuzz/corpus/aql -o fuzz/output/aql -- ./harness @@
# ... etc
```

## Fuzzing Targets

| Target | Priorität | Beschreibung |
|--------|-----------|--------------|
| `aql_parser` | Hoch | AQL Query Parser |
| `json_parser` | Hoch | JSON Document Parser |
| `crypto_operations` | Kritisch | Kryptographische Operationen |
| `network_protocol` | Hoch | Netzwerk-Protokoll Parser |
| `storage_engine` | Mittel | Storage Engine I/O |
| `auth_handler` | Kritisch | Authentication Handler |
| `pii_redaction` | Kritisch | PII Redaction Pipeline – no PII leak, crash-free, idempotent |

## Konfiguration

### JSON Format

```json
{
  "fuzzing": {
    "engine": "AFL++",
    "targets": [
      {
        "name": "aql_parser",
        "timeout": 1000,
        "memory_limit": "512MB",
        "sanitizers": ["address", "undefined"]
      }
    ]
  }
}
```

### YAML Format

```yaml
fuzzing:
  engine: AFL++
  targets:
    - name: aql_parser
      timeout: 1000
      memory_limit: 512MB
      sanitizers:
        - address
        - undefined
```

## CI/CD Integration

GitHub Actions Workflow: `.github/workflows/fuzzing.yml`

- **Schedule:** Jeden Sonntag um Mitternacht
- **Manual Trigger:** Workflow dispatch mit Target-Auswahl
- **Artifacts:** Crashes, Coverage, Reports

## Crash-Analyse

### Mit CASR

```bash
# CASR installieren
cargo install casr

# Crash analysieren
casr-gdb -o crash_report.json -- ./harness crash_input

# Triage
casr-cluster -i crashes/ -o clustered/
```

### Severity Levels

| Level | Beschreibung |
|-------|--------------|
| Exploitable | Definitiv ausnutzbar |
| Probably Exploitable | Wahrscheinlich ausnutzbar |
| Probably Not Exploitable | Wahrscheinlich nicht ausnutzbar |
| Not Exploitable | Nicht ausnutzbar |

## Best Practices

1. **Sanitizers verwenden:** AddressSanitizer (ASan), UndefinedBehaviorSanitizer (UBSan)
2. **Coverage-guided:** Immer coverage-guided Fuzzing verwenden
3. **Dictionaries:** Spezifische Dictionaries für jeden Parser
4. **Persistent Mode:** Für bessere Performance
5. **Paralleles Fuzzing:** Mehrere Instanzen mit unterschiedlichen Power Schedules

## Referenzen

- [AFL++ Dokumentation](https://aflplus.plus/)
- [AFL++ GitHub](https://github.com/AFLplusplus/AFLplusplus)
- [CASR Crash Analyzer](https://github.com/ispras/casr)
- [OSS-Fuzz](https://google.github.io/oss-fuzz/)
