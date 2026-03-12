# ThemisDB Wiki Integration - Quick Start Guide

**⚡ Schnellstart in 5 Minuten / Quick Start in 5 Minutes**

---

## 🚀 Schritt 1: Installation (2 Minuten)

### WordPress Admin Panel

1. **Plugin hochladen:**
   - WordPress Admin → **Plugins** → **Installieren** → **Plugin hochladen**
   - Wählen Sie `themisdb-wiki-integration.zip`
   - Klicken Sie auf **Jetzt installieren**

2. **Aktivieren:**
   - Klicken Sie auf **Plugin aktivieren**

✅ **Plugin ist jetzt aktiv!**

---

## ⚙️ Schritt 2: Grundkonfiguration (1 Minute)

### Einstellungen öffnen

**WordPress Admin → Einstellungen → ThemisDB Wiki**

### Minimale Konfiguration

Belassen Sie die Standardwerte:
- **GitHub Repository**: `makr-code/wordpressPlugins` ✅
- **Branch**: `main` ✅
- **Dokumentationspfad**: `docs` ✅
- **Standard-Sprache**: `de` ✅

Klicken Sie auf **Änderungen speichern**

✅ **Plugin ist konfiguriert!**

---

## 📝 Schritt 3: Erste Seite erstellen (2 Minuten)

### Neue Seite anlegen

1. **WordPress Admin → Seiten → Erstellen**
2. **Titel**: "ThemisDB Dokumentation"
3. **Inhalt**: Fügen Sie folgenden Shortcode ein:

```php
[themisdb_wiki file="README.md" lang="de"]
```

4. Klicken Sie auf **Veröffentlichen**

✅ **Erste Seite mit Dokumentation ist live!**

---

## 🎉 Fertig!

Besuchen Sie Ihre Seite und sehen Sie die ThemisDB-Dokumentation direkt aus GitHub!

---

## 🔥 Bonus: Weitere Beispiele

### Beispiel 1: Feature-Übersicht mit Inhaltsverzeichnis
```php
[themisdb_wiki file="features/FEATURES.md" lang="de" show_toc="yes"]
```

### Beispiel 2: Englische Dokumentation
```php
[themisdb_wiki file="README.md" lang="en"]
```

### Beispiel 3: Liste aller Dokumente
```php
[themisdb_docs lang="de" layout="grid"]
```

### Beispiel 4: Architektur-Dokumentation
```php
[themisdb_wiki file="architecture/ARCHITECTURE.md" lang="en" show_toc="yes"]
```

---

## 🛠️ Erweiterte Einstellungen (Optional)

### GitHub Token für höhere API Limits

**Warum?** 60 → 5.000 Requests/Stunde

**Wie:**
1. https://github.com/settings/tokens → **Generate new token**
2. Name: "WordPress ThemisDB Wiki"
3. Scope: ✅ `public_repo`
4. Token kopieren
5. WordPress Admin → Einstellungen → ThemisDB Wiki
6. Token einfügen → **Änderungen speichern**

### Auto-Sync aktivieren

**WordPress Admin → Einstellungen → ThemisDB Wiki**
- ✅ **Auto-Sync** aktivieren
- Dokumentation wird stündlich aktualisiert

---

## 📚 Vollständige Dokumentation

- **Installation**: Siehe `INSTALLATION.md`
- **Alle Features**: Siehe `README.md`
- **Support**: https://github.com/makr-code/wordpressPlugins/issues

---

## ❓ Probleme?

### Fehler: "GitHub API returned status code 404"
**Lösung:** Überprüfen Sie Repository-Name und Dateipfad

### Fehler: "Rate limit exceeded"
**Lösung:** GitHub Token hinzufügen (siehe oben)

### Dokumentation wird nicht angezeigt
**Lösung:** Admin-Panel → "Sync Now" Button klicken

---

## 🎯 Nächste Schritte

1. ✅ Erstellen Sie weitere Seiten mit verschiedenen Dokumenten
2. ✅ Passen Sie das Styling in `wiki-integration.css` an
3. ✅ Integrieren Sie Navigation für Dokumentationsseiten
4. ✅ Aktivieren Sie Auto-Sync für automatische Updates

---

**Viel Erfolg! 🚀**

Bei Fragen: https://github.com/makr-code/wordpressPlugins/issues
