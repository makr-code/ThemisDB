> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Kontaktmanager - Anfänger Tutorial

Willkommen zum Tutorial für den ThemisDB Kontaktmanager! Dieses Tutorial führt Sie Schritt für Schritt durch alle Funktionen der Anwendung.

## Inhaltsverzeichnis

1. [Erste Schritte](#erste-schritte)
2. [Ihren ersten Kontakt erstellen](#ihren-ersten-kontakt-erstellen)
3. [Kontakte durchsuchen](#kontakte-durchsuchen)
4. [Kontakte bearbeiten](#kontakte-bearbeiten)
5. [Kategorien verwenden](#kategorien-verwenden)
6. [Favoriten markieren](#favoriten-markieren)
7. [Import und Export](#import-und-export)
8. [Tipps und Tricks](#tipps-und-tricks)

## Erste Schritte

### Voraussetzungen

Bevor Sie beginnen, stellen Sie sicher, dass:

1. **ThemisDB Server läuft**
   ```bash
   docker run -d -p 8080:8080 themisdb/themisdb:latest
   ```

2. **Anwendung installiert ist**
   ```bash
   cd examples/03_contact_manager
   pip install -r requirements.txt
   ```

3. **Anwendung starten**
   ```bash
   python main.py
   ```

### Die Benutzeroberfläche

Wenn Sie die Anwendung starten, sehen Sie:

```
┌─────────────────────────────────────────────────────────┐
│ Kontaktmanager - ThemisDB                         [X]   │
├─────────────────────────────────────────────────────────┤
│ Datei  Bearbeiten  Ansicht  Hilfe                      │
├─────────────────────────────────────────────────────────┤
│ [+] [✏] [🗑] [⭐] [↻]         [Suchen...]   [Filter ▼] │
├─────────────────────────────────────────────────────────┤
│ Kontakte (25)      │  Kontakt-Details                   │
│ ──────────────     │  ────────────────                  │
│ ⭐ Anna Schmidt    │  Name:    Anna Schmidt            │
│   Max Mustermann   │  Email:   anna@example.com        │
│   Lisa Müller      │  Telefon: +49 123 456789          │
│   ...              │  Adresse: Musterstr. 1            │
│                    │           12345 Berlin            │
│                    │  Kategorie: Freunde               │
│                    │                                    │
│                    │  Notizen:                          │
│                    │  ┌──────────────────────────┐     │
│                    │  │ Geburtstag: 15.03.       │     │
│                    │  │                          │     │
│                    │  └──────────────────────────┘     │
└─────────────────────────────────────────────────────────┘
 Bereit | 25 Kontakte | Favoriten: 3
```

## Ihren ersten Kontakt erstellen

### Schritt 1: Neuer Kontakt

1. Klicken Sie auf den **[+]** Button in der Toolbar
2. Oder drücken Sie **Strg+N**
3. Oder wählen Sie **Datei → Neuer Kontakt**

### Schritt 2: Daten eingeben

Ein leeres Formular erscheint auf der rechten Seite:

```
┌─────────────────────────────────┐
│  Neuer Kontakt                  │
│  ─────────────                  │
│  Name*:                         │
│  ┌────────────────────────────┐ │
│  │ Max Mustermann             │ │
│  └────────────────────────────┘ │
│                                 │
│  Email*:                        │
│  ┌────────────────────────────┐ │
│  │ max@example.com            │ │
│  └────────────────────────────┘ │
│                                 │
│  Telefon:                       │
│  ┌────────────────────────────┐ │
│  │ +49 123 456789             │ │
│  └────────────────────────────┘ │
│                                 │
│  Kategorie:                     │
│  ┌────────────────────────────┐ │
│  │ Freunde              ▼     │ │
│  └────────────────────────────┘ │
│                                 │
│  [Speichern] [Abbrechen]       │
└─────────────────────────────────┘
```

**Wichtig**: Felder mit * sind Pflichtfelder!

### Schritt 3: Adresse hinzufügen

1. Scrollen Sie nach unten zum Adresse-Bereich
2. Geben Sie die Adressdaten ein:
   - Straße und Hausnummer
   - PLZ
   - Stadt
   - Land (optional)

### Schritt 4: Speichern

1. Klicken Sie auf **[Speichern]**
2. Oder drücken Sie **Strg+S**
3. Der neue Kontakt erscheint sofort in der Liste!

### Mögliche Fehler

❌ **"Name ist erforderlich"**
   → Geben Sie einen Namen ein (mind. 2 Zeichen)

❌ **"Ungültige E-Mail-Adresse"**
   → Format muss sein: name@domain.com

❌ **"Telefonnummer ungültig"**
   → Nur Zahlen, Leerzeichen, +, - und ()

## Kontakte durchsuchen

### Einfache Suche

Die Suchfunktion durchsucht **alle** Felder gleichzeitig:

1. Klicken Sie in das Suchfeld (oder drücken Sie **Strg+F**)
2. Tippen Sie einen Suchbegriff ein
3. Ergebnisse erscheinen sofort (Live-Suche!)

**Beispiele**:

```
Suche nach "schmidt"
→ Findet: Anna Schmidt, Karl-Schmidt-Straße, schmidt@email.com

Suche nach "berlin"
→ Findet alle Kontakte in Berlin

Suche nach "@example"
→ Findet alle example.com Email-Adressen
```

### Erweiterte Suche

Sie können auch nach mehreren Begriffen suchen:

```
Suche: "anna berlin"
→ Findet Anna in Berlin

Suche: "+49"
→ Findet alle deutschen Telefonnummern
```

### Filter verwenden

Für strukturierte Suche verwenden Sie die Filter:

1. Klicken Sie auf **[Filter ▼]**
2. Wählen Sie einen Filter:

```
┌────────────────────────┐
│ □ Alle Kontakte        │
│ ☑ Nur Favoriten        │
│ □ Kategorie:           │
│   • Freunde            │
│   • Familie            │
│   • Arbeit             │
│   • Andere             │
└────────────────────────┘
```

3. Die Liste wird automatisch gefiltert

### Suche zurücksetzen

- Löschen Sie den Suchtext
- Oder drücken Sie **ESC**
- Oder klicken Sie auf **[↻]** (Aktualisieren)

## Kontakte bearbeiten

### Kontakt auswählen

Es gibt drei Wege, einen Kontakt zu öffnen:

1. **Einfach-Klick**: Zeigt Details an
2. **Doppel-Klick**: Öffnet zum Bearbeiten
3. **Rechts-Klick**: Zeigt Kontextmenü

### Kontakt ändern

1. Wählen Sie einen Kontakt aus der Liste
2. Klicken Sie auf **[✏]** (Bearbeiten)
3. Oder drücken Sie **Strg+E**
4. Ändern Sie die gewünschten Felder
5. Klicken Sie auf **[Speichern]**

**Tipp**: Änderungen können Sie mit **ESC** abbrechen!

### Kontakt löschen

⚠️ **Achtung**: Löschen ist permanent!

1. Wählen Sie einen Kontakt
2. Klicken Sie auf **[🗑]** (Löschen)
3. Oder drücken Sie **Entf**
4. Bestätigen Sie die Sicherheitsabfrage

```
┌─────────────────────────────────┐
│  Kontakt löschen?               │
│  ──────────────────             │
│  Möchten Sie "Max Mustermann"   │
│  wirklich löschen?              │
│                                 │
│  Diese Aktion kann nicht        │
│  rückgängig gemacht werden!     │
│                                 │
│  [Löschen] [Abbrechen]         │
└─────────────────────────────────┘
```

## Kategorien verwenden

### Verfügbare Kategorien

Der Kontaktmanager bietet 4 Standard-Kategorien:

1. **👥 Freunde** - Persönliche Kontakte
2. **👨‍👩‍👧‍👦 Familie** - Familienangehörige
3. **💼 Arbeit** - Geschäftliche Kontakte
4. **📌 Andere** - Sonstige Kontakte

### Kategorie zuweisen

1. Öffnen Sie einen Kontakt zum Bearbeiten
2. Wählen Sie eine Kategorie aus dem Dropdown
3. Speichern Sie den Kontakt

### Nach Kategorie filtern

1. Klicken Sie auf **[Filter ▼]**
2. Wählen Sie **Kategorie**
3. Wählen Sie eine oder mehrere Kategorien

```
Filter aktiv: Freunde (8)
Filter aktiv: Familie (5)
```

### Mehrere Kategorien

Sie können auch nach mehreren Kategorien gleichzeitig filtern:

```
Filter: Freunde + Familie
→ Zeigt 13 Kontakte an
```

## Favoriten markieren

### Kontakt als Favorit markieren

Wichtige Kontakte können Sie als Favorit markieren:

1. Wählen Sie einen Kontakt
2. Klicken Sie auf **[⭐]** (Favorit)
3. Oder drücken Sie **Strg+D**

Der Kontakt erhält ein **⭐** Symbol in der Liste.

### Nur Favoriten anzeigen

1. Klicken Sie auf **[Filter ▼]**
2. Wählen Sie **☑ Nur Favoriten**
3. Die Liste zeigt nur Favoriten

**Tipp**: Favoriten erscheinen immer am Anfang der Liste!

### Favorit entfernen

Klicken Sie erneut auf **[⭐]** beim gewählten Kontakt.

## Import und Export

### Kontakte exportieren

Sie können alle oder ausgewählte Kontakte exportieren:

1. Wählen Sie **Datei → Exportieren**
2. Wählen Sie ein Format:

```
┌────────────────────────────┐
│ Exportformat wählen:       │
│ ○ JSON (.json)             │
│ ○ CSV (.csv)               │
│ ○ vCard (.vcf)             │
└────────────────────────────┘
```

3. Wählen Sie einen Speicherort
4. Klicken Sie auf **Speichern**

**Export-Formate**:

- **JSON**: Vollständige Daten, ThemisDB-kompatibel
- **CSV**: Für Excel/Sheets, einfache Tabelle
- **vCard**: Für Smartphones und Email-Clients

### Kontakte importieren

1. Wählen Sie **Datei → Importieren**
2. Wählen Sie eine Datei
3. Prüfen Sie die Vorschau:

```
┌─────────────────────────────────┐
│  Import-Vorschau               │
│  ─────────────                 │
│  5 Kontakte werden importiert  │
│                                │
│  ✓ Anna Schmidt                │
│  ✓ Max Mustermann              │
│  ⚠ Lisa Müller (Duplikat?)    │
│  ✓ Tom Weber                   │
│  ✗ Invalid Name (Fehler)       │
│                                │
│  [Importieren] [Abbrechen]    │
└─────────────────────────────────┘
```

4. Klicken Sie auf **[Importieren]**

### Duplikate behandeln

Bei Duplikaten haben Sie drei Optionen:

```
┌────────────────────────────────┐
│ Duplikat gefunden              │
│ ────────────────               │
│ "Max Mustermann" existiert     │
│ bereits. Was möchten Sie tun?  │
│                                │
│ ○ Überspringen                 │
│ ○ Ersetzen                     │
│ ○ Beide behalten               │
│                                │
│ [OK] [Abbrechen]              │
└────────────────────────────────┘
```

## Tipps und Tricks

### ⌨️ Tastenkombinationen

Die wichtigsten Shortcuts auf einen Blick:

```
Strg+N    Neuer Kontakt
Strg+E    Kontakt bearbeiten
Strg+S    Speichern
Strg+F    Suchen
Strg+D    Als Favorit markieren
Entf      Löschen
ESC       Abbrechen
F5        Aktualisieren
```

### 🎯 Pro-Tipps

**1. Schnelle Notizen**
   - Nutzen Sie das Notizen-Feld für Geburtstage
   - Format: "Geb: 15.03." für schnelles Finden

**2. Batch-Export**
   - Filter → Kategorie → Exportieren
   - Exportiert nur gefilterte Kontakte

**3. Backup erstellen**
   - Regelmäßig als JSON exportieren
   - JSON enthält alle Daten inkl. Kategorien

**4. Suche optimieren**
   - Verwenden Sie Teilwörter: "schmidt" statt "Anna Schmidt"
   - Suche ist case-insensitive (Groß-/Kleinschreibung egal)

**5. Favoriten-Workflow**
   - Markieren Sie häufig genutzte Kontakte
   - Filter → Nur Favoriten für schnellen Zugriff

### 🔧 Problemlösung

**Problem**: Kontakt wird nicht gefunden

✅ **Lösung**:
   1. Löschen Sie alle Filter
   2. Drücken Sie **F5** zum Aktualisieren
   3. Prüfen Sie die Rechtschreibung

**Problem**: Speichern schlägt fehl

✅ **Lösung**:
   1. Prüfen Sie Pflichtfelder (Name, Email)
   2. Prüfen Sie Email-Format
   3. Prüfen Sie Verbindung zu ThemisDB

**Problem**: Import funktioniert nicht

✅ **Lösung**:
   1. Prüfen Sie Dateiformat (JSON/CSV)
   2. Öffnen Sie Datei in Editor und prüfen Sie Struktur
   3. Prüfen Sie Encoding (UTF-8 erforderlich)

## Nächste Schritte

Jetzt, da Sie die Grundlagen kennen, können Sie:

1. **Fortgeschrittene Features erkunden**
   - Siehe [HOW_TO.md](HOW_TO.md) für Details

2. **API verwenden**
   - Bauen Sie eigene Tools mit der ThemisDB API
   - Siehe `themis_client.py` für Beispiele

3. **Daten analysieren**
   - Exportieren Sie als CSV
   - Analysieren Sie in Excel/Python

4. **Erweitern**
   - Fügen Sie eigene Felder hinzu
   - Implementieren Sie neue Features

## Weitere Hilfe

- 📖 **HOW_TO.md**: Detaillierte Bedienungsanleitung
- 📖 **README.md**: Technische Details und Installation
- 💬 **GitHub Issues**: Fragen und Probleme melden
- 📧 **Support**: support@themisdb.com

---

**Viel Erfolg mit dem Kontaktmanager!** 🎉

Bei Fragen oder Anregungen erstellen Sie gerne ein Issue auf GitHub.
