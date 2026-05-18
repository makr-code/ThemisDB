# ⚠️ KRITISCHES MINDSET FÜR KAPITEL-GENERIERUNG

**REGEL 1: Nie neue Kapitel schreiben - immer bestehende verbessern!**

---

## 🎯 Mentale Modelle

### FALSCH ❌
```
"Ich schreibe ein neues Kapitel 6: Graph-Datenmodell"
→ Problem: chapter_06_graph.md existiert bereits!
→ Resultat: Duplikate, Verwirrung, überflüssige Arbeit
```

### RICHTIG ✅
```
"Ich verbessere Kapitel 6: Graph-Datenmodell"
→ Input: Existierendes chapter_06_graph.md
→ Output: Bessere Version des GLEICHEN Kapitels
→ Resultat: Mehr Tiefe, bessere Quellen, keine Duplikate
```

---

## 📋 Vor jeder Kapitel-Generierung:

### Schritt 0: Existiert das Kapitel bereits?

**CHECKLIST:**
```
1. Öffne: ./docs/ (Datei-Explorer)
2. Suche: chapter_*.md Dateien
3. Finde ich ein Kapitel mit meinem Thema?
   - JA → Gehe zu "Umformulierungs-Workflow"
   - NEIN → Gehe zu "Neues Kapitel - Ausnahmeverfahren"
```

### Option A: Kapitel existiert (99% der Fälle)

**Umformulierungs-Workflow:**

1. **Öffne bestehendes Kapitel**
   ```
   Datei: ./docs/chapter_N.md
   Lesend öffnen
   ```

2. **Analysiere aktuellen Zustand**
   ```
   - Was ist bereits vorhanden?
   - Was fehlt (Quellen, Tiefe, Code)?
   - Was ist veraltet?
   ```

3. **Erstelle LLM-Prompt**
   ```
   INPUT: [Kompletter Text aus chapter_N.md]
   
   ANFORDERUNG: "Verbessere diesen Text zu:"
   - Wissenschaftlicherer Stil
   - Bessere Quellen-Integration
   - Mehr Code-Beispiele
   - Performance-Daten
   
   OUTPUT: Bessere Version des GLEICHEN Kapitels
   ```

4. **Übernehme verbessertes Kapitel**
   ```
   Ersetze: ./docs/chapter_N.md
   Nicht: Neue Datei erstellen
   Nicht: Kapitel-Nummer ändern
   ```

### Option B: Kapitel existiert nicht (1% der Fälle)

**Ausnahmeverfahren:**

```
1. ✅ Überprüfe nochmal: 47 Kapitel durchsuchen
2. ✅ Mit Team abstimmen: "Ist das wirklich ein neues Thema?"
3. ✅ Falls JA: Neue chapter_N.md anlegen
4. ✅ In mkdocs-nav.yml registrieren
5. ✅ Mit Team Kapitel-Nummer klären
```

---

## 🔄 Typische Szenarien

### Szenario 1: "Ich will Graph-Datenbank-Kapitel verbessern"
```
Befund: chapter_06_graph.md existiert bereits
Aktion: NICHT chapter_06_graph_improved.md erstellen
Aktion: NICHT chapter_07_graph_advanced.md erstellen
Aktion: JA - ./docs/chapter_06_graph.md umformulieren
```

### Szenario 2: "Ich will über Clustering-Algorithmen schreiben"
```
Befund: Ist das in chapter_19_monitoring.md erwähnt?
        Ist das in chapter_15_analytics.md erwähnt?
Aktion: NICHT chapter_XX_clustering.md erstellen
Aktion: JA - Relevantes bestehendes Kapitel ergänzen
```

### Szenario 3: "Ich will neue Feature XY dokumentieren"
```
Befund: Existiert XY in keinem der 47 Kapitel
Aktion: Team fragen: "Wo passt XY hin?"
        - In bestehendes Kapitel integrieren? (wahrscheinlich)
        - Neues Kapitel brauchen? (selten)
```

---

## 🚫 DON'Ts - Was vermeiden

```
❌ NICHT: "Ich schreibe ein neues Kapitel"
✓ JA: "Ich verbessere ein bestehendes Kapitel"

❌ NICHT: chapter_06_graph_v2.md
✓ JA: chapter_06_graph.md (überschreiben)

❌ NICHT: 3 neue Dateien mit Varianten
✓ JA: 1 Datei mit besserem Inhalt

❌ NICHT: Blindlings generieren, ohne check
✓ JA: Erst checken ob Kapitel existiert

❌ NICHT: mkdocs-nav.yml ändern
✓ JA: Nur Datei-Inhalt ändern
```

---

## ✅ Beste Praktiken

### Vor Generierung:
- [ ] Bestehendes Kapitel geöffnet?
- [ ] Aktuelle Version gelesen?
- [ ] Gaps identifiziert (Quellen, Tiefe, Code)?
- [ ] Relevante externe Ressourcen gesammelt?

### Bei LLM-Prompt:
- [ ] Bestehendes Kapitel als INPUT eingegeben?
- [ ] Nicht: "Schreibe ein Kapitel über..."
- [ ] Ja: "Verbessere dieses Kapitel..."
- [ ] Quellen mitgeliefert?

### Nach Generierung:
- [ ] Code-Beispiele getestet?
- [ ] Neue Quellen-Links überprüft?
- [ ] **Design-Richtlinien beachtet?** (IMPLEMENTATION_COMPLETE.md, THEMISDB_CUSTOM_THEME.md)
- [ ] Layout-Standards beachtet?
- [ ] Datei-Name gleich geblieben? (chapter_N.md)
- [ ] mkdocs-nav.yml unverändert? (falls kein neues Kapitel)

---

## 📊 Statistik

```
Bestehende Kapitel: 47 (chapter_00.md bis chapter_46.md)
Anhänge: 7 (appendix_*.md)
GESAMT: 54 Dateien

Wahrscheinlichkeit neue Kapitel brauchen: <1%
Wahrscheinlichkeit Umformulierung brauchen: >99%

FOLGERUNG:
→ Wenn du eine Idee hast: Bestehendes Kapitel verbessern!
→ Nicht: Neues Kapitel schreiben!
```

---

## 🎓 Lernziele für dieses "Mindset"

Nach dieser Anleitung solltest du wissen:

1. **95% der Fälle:** Bestehendes Kapitel verbessern
2. **5% der Fälle:** Im bestehendem Kapitel integrieren
3. **<1% der Fälle:** Neues Kapitel (mit Abstimmung)
4. **Nie:** Willkürlich neue Dateien/Kapitel erstellen
5. **Immer:** Struktur erhalten, Inhalt erweitern

---

**Version:** 1.0  
**Status:** Pflichtlektüre vor Kapitel-Generierung  
**Gültig:** Ab sofort
