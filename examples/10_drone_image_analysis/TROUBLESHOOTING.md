> **Hinweis:** Troubleshooting-Schritte gegen aktuellen Build/Test-Flow verifizieren.

# Troubleshooting - Drohnenbild-Analyse

## 📋 Übersicht

Häufige Probleme und Lösungen für das Drohnenbild-Analyse-System.

## 🐛 Häufige Probleme

### 1. CUDA Out of Memory

**Symptom:**
\`\`\`
RuntimeError: CUDA out of memory
\`\`\`

**Lösungen:**
- Reduziere Batch Size
- Verwende kleineres Model
- Aktiviere Gradient Checkpointing
- Clear Cache: \`torch.cuda.empty_cache()\`

### 2. Model Loading Failed

**Symptom:**
\`\`\`
FileNotFoundError: Model not found
\`\`\`

**Lösungen:**
- Führe \`python download_models.py\` aus
- Prüfe Internet-Verbindung
- Verifiziere Model-Pfade in config

### 3. ThemisDB Connection Error

**Symptom:**
\`\`\`
ConnectionError: Could not connect to ThemisDB
\`\`\`

**Lösungen:**
- Prüfe ob ThemisDB läuft: \`curl http://localhost:8080/health\`
- Prüfe Firewall-Einstellungen
- Verifiziere \`THEMISDB_URL\` Environment Variable

### 4. Image Processing Too Slow

**Symptom:**
- Processing dauert > 5 Sekunden pro Bild

**Lösungen:**
- Aktiviere GPU: \`CUDA_VISIBLE_DEVICES=0\`
- Verwende Model Quantization
- Implementiere Batch Processing
- Prüfe CPU/GPU Usage mit \`nvidia-smi\`

### 5. Inaccurate Detections

**Symptom:**
- Viele False Positives/Negatives

**Lösungen:**
- Tune Confidence Threshold
- Verwende größeres Model (YOLOv8m statt YOLOv8n)
- Fine-tune Model auf eigenen Daten
- Verbessere Image Quality (Auflösung, Beleuchtung)

## 🔍 Debug-Strategien

### Logging aktivieren

\`\`\`python
import logging

logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
\`\`\`

### Performance Profiling

\`\`\`python
import cProfile

cProfile.run('process_image(image_path)', 'profile.stats')

# Analyze
import pstats
p = pstats.Stats('profile.stats')
p.sort_stats('cumulative').print_stats(20)
\`\`\`

### Memory Profiling

\`\`\`python
from memory_profiler import profile

@profile
def process_image(image_path):
    # Your code
    pass
\`\`\`

## 🚨 Error Codes

| Code | Beschreibung | Lösung |
|------|--------------|--------|
| E001 | Image format not supported | Konvertiere zu JPG/PNG |
| E002 | Image too large | Resize oder komprimiere |
| E003 | Missing EXIF data | Manuell GPS hinzufügen |
| E004 | Model inference failed | Prüfe Model + GPU |
| E005 | Database write error | Prüfe ThemisDB Connection |

## 📞 Support

Bei weiteren Problemen:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: Siehe andere MD-Dateien

## 📚 Weitere Dokumentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - System-Design
- [PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md) - Optimierung
- [DEPLOYMENT.md](DEPLOYMENT.md) - Deployment Guide
