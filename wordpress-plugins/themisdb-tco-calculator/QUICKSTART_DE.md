# ThemisDB TCO Calculator - Quick Start Guide

## 🚀 Schnellstart in 3 Schritten

### Schritt 1: Plugin installieren

1. Laden Sie die Plugin-Dateien herunter
2. Kopieren Sie sie nach `/wp-content/plugins/themisdb-tco-calculator/`
3. Aktivieren Sie das Plugin unter **Plugins → Installierte Plugins**

### Schritt 2: Seite erstellen

1. Gehen Sie zu **Seiten → Erstellen**
2. Geben Sie einen Titel ein (z.B. "TCO-Rechner")
3. Fügen Sie den Shortcode ein:
   ```
   [themisdb_tco_calculator]
   ```
4. Klicken Sie auf **Veröffentlichen**

### Schritt 3: Fertig!

Ihre TCO-Rechner-Seite ist jetzt live und einsatzbereit!

## ⚙️ Optional: Einstellungen anpassen

Passen Sie die Standardwerte an Ihre Bedürfnisse an:

1. Gehen Sie zu **Einstellungen → TCO Calculator**
2. Ändern Sie die Werte nach Bedarf:
   - Anfragen pro Tag
   - Datengröße in GB
   - Spitzenlast-Faktor
   - Verfügbarkeit
3. Klicken Sie auf **Einstellungen speichern**

## 🔄 Updates

Das Plugin aktualisiert sich automatisch von GitHub:

- Updates erscheinen unter **Dashboard → Aktualisierungen**
- Einfach auf "Jetzt aktualisieren" klicken
- Fertig!

## 📊 Features

✅ Interaktive TCO-Berechnung mit Echtzeit-Updates
✅ Automatische Neuberechnung beim Bewegen der Schieberegler
✅ Visuelle Diagramme und Charts
✅ Export als PDF oder CSV
✅ Vollständig responsive
✅ Kompatibel mit allen Themes

## 💡 Tipps

### Shortcode-Optionen

**Ohne Einführung:**
```
[themisdb_tco_calculator show_intro="no"]
```

**Mit eigenem Titel:**
```
[themisdb_tco_calculator title="Kostenvergleich"]
```

### Page Builder Integration

Das Plugin funktioniert mit allen gängigen Page Buildern:

- **Elementor**: Shortcode-Widget verwenden
- **Divi**: Text-Modul mit Shortcode
- **Gutenberg**: Shortcode-Block verwenden

### Performance-Tipp

Die Styles und Scripts werden nur auf Seiten mit dem Shortcode geladen - keine Auswirkung auf die Performance anderer Seiten!

## 🆘 Hilfe benötigt?

**Problem:** Button funktioniert nicht
- Prüfen Sie die Browser-Konsole auf JavaScript-Fehler
- Stellen Sie sicher, dass keine Plugin-Konflikte vorliegen
- Deaktivieren Sie temporär andere Plugins zum Test

**Problem:** Styles werden nicht geladen
- Leeren Sie den WordPress-Cache
- Leeren Sie den Browser-Cache
- Prüfen Sie, ob Ihr Theme `wp_head()` und `wp_footer()` aufruft

**Problem:** Update wird nicht angezeigt
- Gehen Sie zu **Dashboard → Aktualisierungen**
- Klicken Sie auf "Nach Updates suchen"
- Der Cache wird alle 12 Stunden erneuert

## 📞 Support

Bei weiteren Fragen oder Problemen:

- **GitHub Issues**: https://github.com/makr-code/wordpressPlugins/issues
- **Dokumentation**: Siehe README.md im Plugin-Ordner

---

**Viel Erfolg mit Ihrem TCO-Rechner!** 🎉
