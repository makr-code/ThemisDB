# Resources - Bildverzeichnis

## 📁 Verzeichnisstruktur

```
Resources/
├── logo.png          # Haupt-Logo für SplashScreen (empfohlen: 250x250px)
├── splash_bg.png     # Optionaler Hintergrund (600x400px)
├── icon.ico          # Anwendungs-Icon
└── README.md         # Diese Datei
```

## 🎨 SplashScreen-Grafiken

### Logo (logo.png)
- **Format**: PNG (mit Transparenz)
- **Empfohlene Größe**: 250x250px oder 300x300px
- **Auflösung**: 96 DPI
- **Verwendung**: Zentriertes Logo im SplashScreen

### Hintergrund (splash_bg.png) - Optional
- **Format**: PNG oder JPG
- **Größe**: 600x400px (Breite x Höhe des SplashScreens)
- **Verwendung**: Vollbild-Hintergrund

## 📌 Unterstützte Formate

✅ **PNG** - Empfohlen (Transparenz-Support)
✅ **JPG/JPEG** - Für Fotos
✅ **BMP** - Windows Bitmap
✅ **GIF** - Animiert oder statisch
✅ **TIFF** - Hochqualität

## 🔧 Integration

Dateien in diesem Ordner werden automatisch als **Embedded Resources** kompiliert und können über folgende Pfade referenziert werden:

```xaml
<!-- Relativer Pfad -->
<Image Source="/Resources/logo.png" />

<!-- Pack URI -->
<Image Source="pack://application:,,,/Resources/logo.png" />
```

## 📝 Hinweise

1. **Dateinamen**: Kleinbuchstaben verwenden (logo.png statt Logo.PNG)
2. **Transparenz**: PNG für Logos mit transparentem Hintergrund
3. **Dateigröße**: Für schnelles Laden < 500 KB halten
4. **Optimierung**: Tools wie TinyPNG.com für kleinere Dateigrößen nutzen

## 🚀 Nächste Schritte

1. PNG-Grafik in diesen Ordner kopieren (z.B. `logo.png`)
2. SplashScreen.xaml wird automatisch aktualisiert
3. Build & Run ausführen
