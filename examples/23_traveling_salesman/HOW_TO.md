> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Traveling Salesman Problem - Bedienungsanleitung

Schritt-für-Schritt-Anleitung zur Verwendung des TSP-Beispiels.

## 📚 Übersicht

Diese Anleitung führt Sie durch:
1. Erstmalige Einrichtung
2. Städte hinzufügen und verwalten
3. TSP-Algorithmen ausführen
4. Ergebnisse visualisieren und vergleichen
5. Daten exportieren

## 🚀 Schnellstart (5 Minuten)

### 1. Server starten

```bash
# Docker-Container starten
docker run -d --name themisdb -p 8080:8080 themisdb/themisdb:latest

# Status prüfen
curl http://localhost:8080/health
```

### 2. Anwendung starten

```bash
cd examples/23_traveling_salesman
pip install -r requirements.txt
python main.py
```

### 3. Demo-Daten laden

Nach dem Start der Anwendung:
1. Klicken Sie auf **"📦 Demo-Daten laden"**
2. Es werden 8 deutsche Städte mit realistischen Koordinaten geladen
3. Die Karte zeigt die Städte als Punkte an

### 4. Route berechnen

1. Wählen Sie einen Algorithmus (empfohlen: **2-Opt**)
2. Klicken Sie auf **"🔍 Route berechnen"**
3. Die optimale Route wird auf der Karte angezeigt
4. Die Gesamtdistanz und Berechnungszeit werden angezeigt

## 📝 Detaillierte Anleitung

### Städte verwalten

#### Stadt hinzufügen

1. Klicken Sie auf **"➕ Stadt hinzufügen"**
2. Geben Sie die Daten ein:
   - **Name**: z.B. "Berlin" (Pflichtfeld)
   - **X-Koordinate**: Breitengrad (z.B. 52.52)
   - **Y-Koordinate**: Längengrad (z.B. 13.405)
   - **Land**: Optional (z.B. "Deutschland")
3. Klicken Sie auf **"💾 Speichern"**

**Tipp**: Verwenden Sie reale GPS-Koordinaten für realistische Distanzen!

#### Stadt bearbeiten

1. Wählen Sie eine Stadt in der Liste aus
2. Klicken Sie auf **"✏️ Bearbeiten"**
3. Ändern Sie die Daten
4. Speichern Sie die Änderungen

#### Stadt löschen

1. Wählen Sie eine Stadt in der Liste aus
2. Klicken Sie auf **"🗑️ Löschen"**
3. Bestätigen Sie die Löschung

**Achtung**: Das Löschen einer Stadt entfernt auch alle zugehörigen Verbindungen!

### Algorithmen ausführen

#### 1. Brute Force (Exakte Lösung)

**Wann verwenden**: Nur bei maximal 10 Städten!

**Schritte**:
1. Wählen Sie "Brute Force" in der Dropdown-Liste
2. Klicken Sie auf **"🔍 Route berechnen"**
3. Warten Sie auf das Ergebnis (kann bei 10 Städten mehrere Sekunden dauern)

**Ergebnis**: Garantiert die optimale Route

**Hinweis**: 
- 8 Städte: ~40.320 Routen zu prüfen (< 1 Sekunde)
- 10 Städte: ~3.628.800 Routen (mehrere Sekunden)
- 12 Städte: ~479.001.600 Routen (mehrere Minuten) ⚠️

#### 2. Nearest Neighbor (Greedy)

**Wann verwenden**: Für schnelle Näherungslösung

**Schritte**:
1. Wählen Sie "Nearest Neighbor" 
2. Optional: Wählen Sie die Startstadt
3. Klicken Sie auf **"🔍 Route berechnen"**

**Ergebnis**: Sehr schnell, aber oft 20-30% länger als optimal

**Vorteil**: O(n²) Komplexität, auch für 100+ Städte geeignet

#### 3. 2-Opt Optimierung

**Wann verwenden**: Für gute Balance zwischen Qualität und Geschwindigkeit

**Schritte**:
1. Wählen Sie "2-Opt"
2. Klicken Sie auf **"🔍 Route berechnen"**
3. Der Algorithmus startet mit Nearest Neighbor und verbessert die Route

**Ergebnis**: Sehr gute Lösungen, oft nahe am Optimum

**Iterationen**: Die Anwendung zeigt an, wie viele Verbesserungen vorgenommen wurden

#### 4. Christofides (Approximation)

**Wann verwenden**: Wenn Qualitätsgarantie wichtig ist

**Schritte**:
1. Wählen Sie "Christofides"
2. Klicken Sie auf **"🔍 Route berechnen"**

**Ergebnis**: Maximal 50% länger als optimal (1.5× Approximation)

**Komplexität**: O(n³), etwas langsamer als 2-Opt

### Visualisierung verstehen

#### Karten-Elemente

- **🔵 Blaue Punkte**: Städte
- **🔴 Roter Punkt**: Startstadt der Route
- **─── Blaue Linien**: Berechnete Route
- **─── Gestrichelte Linien**: Rückkehr zum Start

#### Route-Informationen

Die Anwendung zeigt:
- **Gesamtdistanz**: In Kilometern (bei GPS-Koordinaten)
- **Berechnungszeit**: In Millisekunden
- **Anzahl Städte**: In der Route
- **Algorithmus**: Verwendetes Verfahren
- **Reihenfolge**: Liste der besuchten Städte

### Ergebnisse vergleichen

#### Mehrere Algorithmen testen

1. Berechnen Sie eine Route mit einem Algorithmus
2. Klicken Sie auf **"📊 Vergleichen"**
3. Wählen Sie einen anderen Algorithmus
4. Berechnen Sie erneut
5. Die Anwendung zeigt eine Vergleichstabelle

**Vergleich enthält**:
- Algorithmus-Name
- Routenlänge
- Berechnungszeit
- Qualität (% vom besten Ergebnis)

#### Export-Funktion

**Route exportieren**:
1. Berechnen Sie eine Route
2. Klicken Sie auf **"💾 Route exportieren"**
3. Wählen Sie Format: JSON oder CSV
4. Speichern Sie die Datei

**Distanzmatrix exportieren**:
1. Klicken Sie auf **"📤 Distanzmatrix exportieren"**
2. Die Matrix wird als CSV gespeichert
3. Verwenden Sie sie in Excel oder anderen Tools

## 🎯 Beispiel-Szenarien

### Szenario 1: Deutsche Großstädte

**Ziel**: Optimale Rundreise durch 8 deutsche Städte

**Städte**:
- Berlin (52.52, 13.405)
- Hamburg (53.55, 9.993)
- München (48.137, 11.576)
- Köln (50.937, 6.96)
- Frankfurt (50.11, 8.682)
- Stuttgart (48.78, 9.18)
- Düsseldorf (51.227, 6.773)
- Dortmund (51.514, 7.468)

**Empfohlener Algorithmus**: 2-Opt

**Erwartete Distanz**: ~2.200 km

### Szenario 2: Europäische Hauptstädte

**Ziel**: Rundreise durch 12 europäische Hauptstädte

**Empfohlener Algorithmus**: 2-Opt (Brute Force zu langsam!)

**Hinweis**: Verwenden Sie GPS-Koordinaten für realistische Distanzen

### Szenario 3: Lieferdienst

**Ziel**: Optimale Route für Paket-Zustellung

**Setup**:
1. Lager als Startpunkt (z.B. Zentrum einer Stadt)
2. 20-30 Lieferadressen als Städte
3. Verwenden Sie lokale Koordinaten (z.B. UTM)

**Empfohlener Algorithmus**: Nearest Neighbor (schnell genug für viele Stopps)

## ⌨️ Tastenkombinationen

- **Ctrl+N**: Neue Stadt hinzufügen
- **Ctrl+R**: Route berechnen
- **Ctrl+E**: Ergebnisse exportieren
- **Ctrl+D**: Demo-Daten laden
- **Ctrl+C**: Karte löschen
- **F5**: Ansicht aktualisieren
- **Ctrl+Q**: Anwendung beenden

## 💡 Tipps & Tricks

### Bessere Ergebnisse

1. **Startstadt wählen**: Bei Nearest Neighbor macht die Startstadt einen Unterschied
2. **2-Opt mehrmals ausführen**: Mit unterschiedlichen Startkonfigurationen
3. **Kombinieren**: Erst Nearest Neighbor, dann 2-Opt für beste Ergebnisse

### Performance-Optimierung

- Verwenden Sie Brute Force nur für ≤ 10 Städte
- 2-Opt skaliert gut bis ~50 Städte
- Für 100+ Städte: Nearest Neighbor oder spezielle Heuristiken

### Realistische Daten

- Verwenden Sie GPS-Koordinaten (Breitengrad/Längengrad)
- Die Anwendung berechnet euklidische Distanz
- Für exakte Straßendistanzen: Integrieren Sie eine Routing-API

## 🐛 Häufige Probleme

### "Nicht genügend Städte"

**Problem**: Weniger als 3 Städte vorhanden

**Lösung**: Fügen Sie mindestens 3 Städte hinzu

### "Berechnung dauert zu lange"

**Problem**: Brute Force bei zu vielen Städten

**Lösung**: Wechseln Sie zu 2-Opt oder Nearest Neighbor

### "Route sieht suboptimal aus"

**Problem**: Greedy-Algorithmus findet nicht das Optimum

**Lösung**: Verwenden Sie 2-Opt zur Verbesserung

### "Koordinaten außerhalb des Bereichs"

**Problem**: Ungültige GPS-Koordinaten

**Lösung**: 
- Breitengrad: -90 bis +90
- Längengrad: -180 bis +180

## 📚 Weiterführende Themen

### Erweiterungen

- **Zeitfenster**: Städte müssen zu bestimmten Zeiten besucht werden
- **Kapazitäten**: Fahrzeug hat begrenzte Kapazität (Vehicle Routing Problem)
- **Multiple Touren**: Mehrere Fahrzeuge (mTSP)
- **Asymmetrisch**: Hin- und Rückweg haben unterschiedliche Distanzen

### Literatur

- "Introduction to Algorithms" - Cormen et al. (Kapitel: NP-Completeness)
- "The Traveling Salesman Problem: A Computational Study" - Applegate et al.
- [Concorde TSP Solver](http://www.math.uwaterloo.ca/tsp/concorde.html) - State-of-the-art Solver

---

**Support**: Bei Fragen öffnen Sie ein Issue auf GitHub oder konsultieren Sie die [ThemisDB Dokumentation](../../docs/).
