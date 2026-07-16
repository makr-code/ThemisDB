# Speicheroptimierung für Phase 3 Validator

## Aktuelle Situation
- **Verfügbar**: 145.7 GB (genug für deepseek-coder-v2:16b mit 8.9 GB)
- **Installierte Modelle**: 15 Stück
- **Speicher belegt**: ~146 GB in Modellen
- **Status**: ✅ Kein Handeln erforderlich für PoC

## Empfehlung: Optional Cleanup

Falls Sie generell Speicherplatz sparen möchten:

### Option 1: AGGRESSIV (~57 GB freigeben)
```bash
ollama rm mixtral:latest          # 26 GB
ollama rm gpt-oss:latest          # 13 GB  
ollama rm phi4:latest             # 9.1 GB
ollama rm llama3.1:8b             # 4.9 GB
ollama rm llama3:latest           # 4.7 GB
```

### Option 2: MITTELWEG (empfohlen - ~48 GB)
```bash
ollama rm mixtral:latest          # 26 GB
ollama rm gpt-oss:latest          # 13 GB
ollama rm phi4:latest             # 9.1 GB
```

### Option 3: MINIMAL (~4 GB)
```bash
ollama rm phi3:mini-4k            # 2.4 GB
ollama rm phi3:latest             # 2.2 GB
ollama rm llama3.2:latest         # 2.0 GB
```

## Zu behalten (für Phase 3 und Fallbacks)

✅ **deepseek-coder-v2:16b** (wird geladen)
   - 8.9 GB
   - **BESTE Wahl für Code-Generation**
   - Wird für PoC verwendet

✅ **codellama:latest** - 3.8 GB
   - Guter Fallback für Code
   - Behalten für später

✅ **qwen2.5-coder:1.5b-base** - 986 MB
   - Kleines Fallback-Modell
   - Schneller, weniger Qualität

✅ **all-minilm:latest** - 45 MB
   - Embedding-Modell
   - Klein, behalten

✅ **nomic-embed-text:latest** - 274 MB
   - Embedding-Modell
   - Klein, behalten

## Nicht erforderlich

❌ mixtral:latest (26 GB) - Generalist, nicht Code-spezialisiert
❌ gpt-oss:latest (13 GB) - Generalist, nicht Code-spezialisiert
❌ phi4:latest (9.1 GB) - Generalist
❌ llama3.1:8b (4.9 GB) - Generalist
❌ llama3:latest (4.7 GB) - Generalist
❌ gemma3:4b (3.3 GB) - Generalist
❌ phi3:mini-4k (2.4 GB) - Generalist
❌ phi3:latest (2.2 GB) - Generalist
❌ llama3.2:latest (2.0 GB) - Generalist

## Vorgehensweise

### Schritt 1: Nur deepseek download (AKTUELL)
- ✅ Speicher reicht (145.7 GB frei)
- Kann direkt losgeladen
- **Keine Cleanup nötig**

### Schritt 2: Nach PoC Validator (später)
- Falls cleanup gewünscht: Option 2 (Mittelweg) empfohlen
- Spart 48 GB ohne Risiko
- Behält alle Code-Modelle

### Schritt 3: Bei Bedarf erweitern
- Falls weitere Modelle nötig: Später neu downloaden
- Keine Eiligkeit

## Kommandos

Alle Modelle einer Option löschen:

```bash
# Option 2 (Empfohlen)
ollama rm mixtral:latest && ollama rm gpt-oss:latest && ollama rm phi4:latest
```

Einzelne Modelle prüfen:
```bash
ollama list                    # Alle anzeigen
ollama show mixtral:latest     # Details zu Modell
```

## TL;DR

**Für JETZT**: ✅ Nichts tun - Speicher reicht
**Später**: Optional Option 2 ausführen (48 GB Speicherersparnis, kein Risiko)

Der PoC Validator kann sofort laufen.
