# Lizenzkompatibilität: Image Analysis Plugins mit ThemisDB

**Datum:** Dezember 2025  
**Version:** 1.0.0  
**Status:** Final  
**Kategorie:** Legal / Compliance

---

## Übersicht

Dieses Dokument beschreibt die Lizenzkompatibilität von Image Analysis C++ Bibliotheken mit **ThemisDB MIT License + Government Clause** und gibt Empfehlungen für den Einsatz im öffentlichen Sektor.

---

## ThemisDB Lizenzmodell

### Basis: MIT License

ThemisDB verwendet eine **MIT License** als Basis, eine der permissivsten Open Source Lizenzen:

```
MIT License with Government Clause

Copyright (c) 2025 The ThemisDB Authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

### Government Clause (Sovereignty Protection)

Zusätzlich zur MIT License enthält ThemisDB eine **Government Clause** zum Schutz der digitalen Souveränität:

**Kernaussagen:**
1. **Service Provider Verpflichtung**: Änderungen an ThemisDB für kommerzielle Cloud-Services müssen veröffentlicht werden
2. **Öffentlicher Sektor**: Bundesbehörden, Länder und Kommunen haben volle Rechte zur Nutzung und Modifikation
3. **Keine Vendor Lock-in**: Verhindert proprietäre Erweiterungen ohne Quellcode-Offenlegung

### Kompatibilitätsanforderungen

Für Bibliotheken, die in ThemisDB integriert werden, gelten folgende Anforderungen:

✅ **Erforderlich:**
- Open Source Lizenz (OSI-approved)
- Keine Copyleft-Verpflichtungen (GPL, AGPL, etc. sind problematisch)
- Kommerzielle Nutzung erlaubt
- Weitergabe ohne Einschränkungen
- Kompatibel mit MIT License

✅ **Wünschenswert:**
- Expliziter Patent Grant
- Keine Trademark-Einschränkungen
- Aktive Community und Maintenance

---

## Lizenzanalyse: Image Analysis Bibliotheken

### 1. ONNX Runtime

**Lizenz:** MIT License  
**Kompatibilität:** ✅ **VOLLSTÄNDIG KOMPATIBEL**

**Details:**
- **Quelle**: https://github.com/microsoft/onnxruntime/blob/main/LICENSE
- **Copyright**: Microsoft Corporation
- **Patent Grant**: ✅ Ja - Microsoft gewährt implizit Patent-Rechte durch MIT License
- **Trademark**: "ONNX" ist geschützt, aber Nutzung als Bibliothek erlaubt
- **Dependencies**: Alle Dependencies sind ebenfalls MIT oder Apache 2.0

**Rechtliche Bewertung:**
- ✅ Identische Lizenz wie ThemisDB Basis (MIT)
- ✅ Keine zusätzlichen Einschränkungen
- ✅ Kommerzielle Nutzung ohne Gebühren
- ✅ Weitergabe unter MIT oder MIT + Government Clause möglich
- ✅ Keine Verpflichtung zur Offenlegung von Änderungen (außer durch Government Clause)

**Empfehlung für öffentlichen Sektor:** ✅ **UNEINGESCHRÄNKT EMPFOHLEN**

---

### 2. llama.cpp (inkl. Vision Branch)

**Lizenz:** MIT License  
**Kompatibilität:** ✅ **VOLLSTÄNDIG KOMPATIBEL**

**Details:**
- **Quelle**: https://github.com/ggerganov/llama.cpp/blob/master/LICENSE
- **Copyright**: Georgi Gerganov and contributors
- **Patent Grant**: ⚠️ Nicht explizit (aber MIT-typisch)
- **GGML Library**: Ebenfalls MIT License
- **Submodules**: Alle unter MIT oder BSD License

**Rechtliche Bewertung:**
- ✅ Identische Lizenz wie ThemisDB
- ✅ Perfekte Integration möglich (gleiche Lizenz-Familie)
- ✅ Keine Lizenz-Konflikte bei Unified LLM+Vision Architecture
- ✅ Community-getrieben, keine Corporate Control
- ⚠️ Kein expliziter Patent Grant (aber bei MIT üblich implizit)

**Besonderheit für ThemisDB:**
Da llama.cpp bereits für LLM-Integration vorgesehen ist, ergibt sich eine **nahtlose Lizenz-Kontinuität** für Vision-Erweiterungen.

**Empfehlung für öffentlichen Sektor:** ✅ **UNEINGESCHRÄNKT EMPFOHLEN**

---

### 3. OpenCV

**Lizenz:** Apache License 2.0  
**Kompatibilität:** ✅ **VOLLSTÄNDIG KOMPATIBEL**

**Details:**
- **Quelle**: https://github.com/opencv/opencv/blob/4.x/LICENSE
- **Copyright**: OpenCV Foundation und Contributors
- **Patent Grant**: ✅ Ja - Expliziter Patent-Schutz in Apache 2.0
- **Trademark**: "OpenCV" ist geschützt, aber Library-Nutzung erlaubt
- **Dependencies**: Mix aus BSD, Apache 2.0, MIT

**Rechtliche Bewertung:**
- ✅ Apache 2.0 ist kompatibel mit MIT
- ✅ Stärkerer Patent-Schutz als MIT (Vorteil!)
- ✅ Keine Copyleft-Verpflichtungen
- ✅ Weitergabe unter MIT + Government Clause möglich
- ⚠️ Apache 2.0 Notice muss in Dokumentation erwähnt werden

**Patent-Klausel (Apache 2.0):**
```
3. Grant of Patent License. [...] each Contributor hereby grants to You
a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable
(except as stated in this section) patent license...
```

Dies bietet **zusätzlichen Schutz** gegenüber Patent-Klagen.

**Empfehlung für öffentlichen Sektor:** ✅ **UNEINGESCHRÄNKT EMPFOHLEN**

---

### 4. TensorRT

**Lizenz:** NVIDIA Proprietary EULA (TensorRT SDK) + Apache 2.0 (TensorRT OSS)  
**Kompatibilität:** ⚠️ **EINGESCHRÄNKT KOMPATIBEL**

**Details:**
- **TensorRT SDK**: Proprietary, kostenlos aber nicht Open Source
- **TensorRT Runtime**: Darf frei verteilt werden
- **TensorRT OSS Components**: Apache 2.0 (https://github.com/NVIDIA/TensorRT)
- **Patent Grant**: ✅ Implizit durch NVIDIA gewährt
- **EULA**: Muss akzeptiert werden für SDK-Nutzung

**Rechtliche Bewertung:**
- ⚠️ **Runtime**: Darf in ThemisDB integriert und verteilt werden
- ⚠️ **SDK**: Nicht redistributable, nur für Build-Zeit
- ❌ **Source Code**: SDK ist closed source
- ✅ **Kommerzielle Nutzung**: Erlaubt (kostenlos)
- ⚠️ **EULA-Akzeptanz**: Erforderlich bei Installation

**NVIDIA TensorRT EULA - Kernpunkte:**
```
2.1.1. Rights. Subject to the terms of this Agreement, NVIDIA hereby grants
you a non-exclusive, non-transferable license, without the right to
sublicense to:
(i) Install and use the SOFTWARE
(ii) Distribute the runtime components [...] subject to the distribution
     requirements described in this Agreement
```

**Probleme für öffentlichen Sektor:**
1. **Nicht vollständig Open Source**: SDK ist closed source
2. **EULA-Akzeptanz**: Behörden müssen EULA zustimmen
3. **Vendor Lock-in**: Abhängigkeit von NVIDIA
4. **Redistribution**: Eingeschränkte Weitergabe-Rechte

**Lösungsansätze:**
1. **Option 1**: ONNX Runtime + TensorRT Execution Provider
   - ONNX Runtime ist MIT License
   - TensorRT nur als optionales Backend
   - Keine direkte TensorRT-Abhängigkeit in ThemisDB

2. **Option 2**: TensorRT als optionales Plugin
   - Nicht in Core ThemisDB integriert
   - Separate Installation erforderlich
   - Nutzer akzeptiert EULA selbst

**Empfehlung für öffentlichen Sektor:** ⚠️ **NUR ALS OPTIONALES PLUGIN**

**Best Practice:**
```yaml
# config/image_analysis.yaml
image_analysis:
  plugins:
    - name: onnx_clip
      enabled: true
      backend: "CUDA"  # ONNX Runtime nutzt CUDA direkt
      
    # TensorRT optional, wenn verfügbar
    - name: tensorrt_optimizer
      enabled: false  # Optional, manuell aktivieren
      requires_eula: true
```

---

### 5. OpenVINO

**Lizenz:** Apache License 2.0  
**Kompatibilität:** ✅ **VOLLSTÄNDIG KOMPATIBEL**

**Details:**
- **Quelle**: https://github.com/openvinotoolkit/openvino/blob/master/LICENSE
- **Copyright**: Intel Corporation
- **Patent Grant**: ✅ Ja - Expliziter Patent-Schutz
- **Dependencies**: Mix aus Apache 2.0, MIT, BSD
- **Trademark**: "OpenVINO" ist Intel Trademark

**Rechtliche Bewertung:**
- ✅ Apache 2.0 vollständig kompatibel mit MIT
- ✅ Intel gewährt umfassende Patent-Rechte
- ✅ Keine Einschränkungen für kommerzielle Nutzung
- ✅ Weitergabe unter MIT + Government Clause möglich
- ✅ Vollständig Open Source

**Empfehlung für öffentlichen Sektor:** ✅ **UNEINGESCHRÄNKT EMPFOHLEN**

---

### 6. ncnn

**Lizenz:** BSD 3-Clause License  
**Kompatibilität:** ✅ **VOLLSTÄNDIG KOMPATIBEL**

**Details:**
- **Quelle**: https://github.com/Tencent/ncnn/blob/master/LICENSE.txt
- **Copyright**: Tencent Inc.
- **Patent Grant**: ⚠️ Nicht explizit (BSD-typisch)
- **Redistribution**: Mit BSD Notice erlaubt

**BSD 3-Clause License - Kernpunkte:**
```
1. Redistributions of source code must retain the above copyright notice
2. Redistributions in binary form must reproduce the above copyright notice
3. Neither the name [...] nor the names of its contributors may be used
   to endorse or promote products derived from this software without
   specific prior written permission
```

**Rechtliche Bewertung:**
- ✅ BSD ist kompatibel mit MIT
- ✅ Sehr permissiv, keine Copyleft-Klauseln
- ✅ Kommerzielle Nutzung erlaubt
- ✅ Weitergabe unter MIT + Government Clause möglich
- ⚠️ Kein expliziter Patent Grant (aber unproblematisch)
- ✅ Trademark-Klausel verhindert Missbrauch (positiv)

**Empfehlung für öffentlichen Sektor:** ✅ **UNEINGESCHRÄNKT EMPFOHLEN**

---

### 7. libtorch (PyTorch C++)

**Lizenz:** BSD 3-Clause License (Modified) mit Patent Grant  
**Kompatibilität:** ✅ **VOLLSTÄNDIG KOMPATIBEL**

**Details:**
- **Quelle**: https://github.com/pytorch/pytorch/blob/main/LICENSE
- **Copyright**: Facebook, Inc. (Meta Platforms)
- **Patent Grant**: ✅ Ja - Zusätzliche Patent-Schutz-Klausel
- **Dependencies**: Mix aus BSD, MIT, Apache 2.0

**PyTorch Patent Grant - Kernpunkte:**
```
Additional Grant of Patent Rights Version 2

[...] Subject to the terms and conditions of this License, each Contributor
hereby grants to You a perpetual, worldwide, non-exclusive, no-charge,
royalty-free, irrevocable (except as stated in this section) patent license
[...]
```

**Rechtliche Bewertung:**
- ✅ Modified BSD mit zusätzlichem Patent-Schutz
- ✅ Kompatibel mit MIT
- ✅ **Stärkerer Patent-Schutz** als Standard-BSD (Vorteil!)
- ✅ Meta/Facebook gewährt umfassende Patent-Rechte
- ✅ Keine Einschränkungen für kommerzielle Nutzung

**Besonderheit:**
PyTorch hat eine der **besten Patent-Schutz-Klauseln** im ML-Bereich, ähnlich Apache 2.0 aber mit BSD-Basis.

**Empfehlung für öffentlichen Sektor:** ✅ **UNEINGESCHRÄNKT EMPFOHLEN**

---

## Zusammenfassung: Lizenz-Matrix

| Bibliothek | Lizenz | Kompatibilität | Patent Grant | Open Source | Empfehlung |
|------------|--------|----------------|--------------|-------------|------------|
| **ONNX Runtime** | MIT | ✅ Vollständig | ✅ Implizit | ✅ 100% | ⭐⭐⭐⭐⭐ |
| **llama.cpp** | MIT | ✅ Vollständig | ⚠️ Implizit | ✅ 100% | ⭐⭐⭐⭐⭐ |
| **OpenCV** | Apache 2.0 | ✅ Vollständig | ✅ Explizit | ✅ 100% | ⭐⭐⭐⭐⭐ |
| **OpenVINO** | Apache 2.0 | ✅ Vollständig | ✅ Explizit | ✅ 100% | ⭐⭐⭐⭐⭐ |
| **ncnn** | BSD 3-Clause | ✅ Vollständig | ⚠️ Implizit | ✅ 100% | ⭐⭐⭐⭐ |
| **libtorch** | BSD + Patent | ✅ Vollständig | ✅ Explizit | ✅ 100% | ⭐⭐⭐⭐⭐ |
| **TensorRT** | Proprietary | ⚠️ Eingeschränkt | ✅ Implizit | ❌ SDK closed | ⭐⭐ (Optional) |

---

## Empfehlungen für ThemisDB Integration

### Tier 1: Primäre Bibliotheken (Core Dependencies)

Diese Bibliotheken sollten als **Kern-Dependencies** integriert werden:

1. **llama.cpp Vision** (MIT) ← **PRIMARY CHOICE**
   - ✅ Identische Lizenz wie ThemisDB
   - ✅ Nahtlose LLM+Vision Integration
   - ✅ Gemeinsame Memory-Infrastruktur
   - ✅ Keine rechtlichen Bedenken
   - ✅ **Bevorzugte Wahl** für Unified Architecture

2. **ONNX Runtime** (MIT)
   - ✅ Identische Lizenz wie ThemisDB
   - ✅ Beste Cross-Platform-Unterstützung
   - ✅ Ergänzung für dedizierte Bildanalyse
   - ✅ Keine rechtlichen Bedenken

3. **OpenCV DNN** (Apache 2.0)
   - ✅ Starker Patent-Schutz
   - ✅ CPU-Fallback ohne GPU
   - ✅ Keine rechtlichen Bedenken

### Tier 2: Optionale Bibliotheken (Optional Dependencies)

Diese Bibliotheken sollten als **optionale Plugins** angeboten werden:

1. **OpenVINO** (Apache 2.0)
   - ✅ Für Intel-Hardware-Optimierung
   - ✅ Keine rechtlichen Bedenken

2. **ncnn** (BSD 3-Clause)
   - ✅ Für Embedded/Mobile Devices
   - ✅ Keine rechtlichen Bedenken

3. **libtorch** (BSD + Patent)
   - ✅ Für Forschung und Entwicklung
   - ✅ Starker Patent-Schutz

### Tier 3: Problematische Bibliotheken (Use with Caution)

Diese Bibliotheken sollten **nur als externe Plugins** unterstützt werden:

1. **TensorRT** (Proprietary)
   - ⚠️ Nur Runtime-Redistribution erlaubt
   - ⚠️ SDK nicht Open Source
   - ⚠️ EULA-Akzeptanz erforderlich
   - **Lösung**: ONNX Runtime + TensorRT EP als Alternative

---

## Compliance-Checkliste für ThemisDB Plugins

### Vor Integration einer Bibliothek:

- [ ] Lizenz ist OSI-approved
- [ ] Lizenz ist kompatibel mit MIT License
- [ ] Keine Copyleft-Verpflichtungen (GPL, AGPL, LGPL problematisch)
- [ ] Kommerzielle Nutzung erlaubt
- [ ] Weitergabe ohne Einschränkungen
- [ ] Dependencies sind ebenfalls Open Source
- [ ] Patent Grant vorhanden oder implizit
- [ ] Keine Trademark-Konflikte
- [ ] Source Code verfügbar

### Bei Verwendung in ThemisDB:

- [ ] LICENSE-Datei in Plugin-Verzeichnis
- [ ] NOTICE-Datei mit Copyright-Hinweisen
- [ ] Dokumentation der verwendeten Bibliotheken
- [ ] Dependencies-Liste mit Lizenzen
- [ ] Keine proprietären Abhängigkeiten im Core
- [ ] Optionale proprietäre Komponenten als Plugins

---

## Rechtliche Hinweise

### Haftungsausschluss

Dieses Dokument stellt **keine Rechtsberatung** dar. Für verbindliche rechtliche Einschätzungen konsultieren Sie bitte einen Fachanwalt für IT-Recht.

### Lizenz-Notices

Bei Integration in ThemisDB müssen folgende Notices beachtet werden:

**ONNX Runtime (MIT):**
```
Copyright (c) Microsoft Corporation
Licensed under the MIT License
```

**OpenCV (Apache 2.0):**
```
Copyright (c) OpenCV Foundation
Licensed under the Apache License, Version 2.0
```

**llama.cpp (MIT):**
```
Copyright (c) 2023 Georgi Gerganov
Licensed under the MIT License
```

### NOTICE-Datei Beispiel

```
ThemisDB Image Analysis Plugins
Copyright (c) 2025 The ThemisDB Authors

This software includes components from:

1. ONNX Runtime (MIT License)
   Copyright (c) Microsoft Corporation
   https://github.com/microsoft/onnxruntime

2. OpenCV (Apache License 2.0)
   Copyright (c) OpenCV Foundation
   https://opencv.org

3. llama.cpp (MIT License)
   Copyright (c) Georgi Gerganov
   https://github.com/ggerganov/llama.cpp
```

---

## Kontakt und Fragen

Bei Fragen zur Lizenzkompatibilität kontaktieren Sie:

- **ThemisDB Maintainer**: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Legal Team**: legal@themisdb.org (falls vorhanden)

---

## Referenzen

- [ThemisDB License](../../LICENSE)
- [OSI Approved Licenses](https://opensource.org/licenses)
- [SPDX License List](https://spdx.org/licenses/)
- [GitHub License Compatibility](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository)
- [Apache vs MIT Comparison](https://opensource.stackexchange.com/questions/1640/apache-2-mit-license-compatibility)
