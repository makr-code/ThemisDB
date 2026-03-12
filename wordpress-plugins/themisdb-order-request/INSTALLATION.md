# Installation und Einrichtung

## Schritt-für-Schritt Anleitung zur Installation des ThemisDB Order Request Plugins

### Voraussetzungen prüfen

Bevor Sie beginnen, stellen Sie sicher, dass Ihr System die folgenden Anforderungen erfüllt:

- ✅ WordPress 5.0 oder höher
- ✅ PHP 7.4 oder höher
- ✅ MySQL 5.6 oder höher (oder MariaDB)
- ✅ Min. 128 MB PHP Memory Limit (256 MB empfohlen)
- ✅ Min. 50 MB freier Speicherplatz

## Installation

### Option 1: WordPress Admin Panel (Empfohlen)

1. **Plugin herunterladen**
   - Laden Sie die neueste Version als ZIP-Datei herunter
   - Oder erstellen Sie eine ZIP-Datei aus dem Plugin-Verzeichnis

2. **Plugin hochladen**
   - Melden Sie sich in Ihrem WordPress Admin Panel an
   - Navigieren Sie zu **Plugins → Installieren**
   - Klicken Sie auf **Plugin hochladen**
   - Wählen Sie die ZIP-Datei aus
   - Klicken Sie auf **Jetzt installieren**

3. **Plugin aktivieren**
   - Nach erfolgreicher Installation klicken Sie auf **Plugin aktivieren**
   - Das Plugin erstellt automatisch alle notwendigen Datenbanktabellen

### Option 2: FTP/SFTP Upload

1. **Dateien extrahieren**
   ```bash
   unzip themisdb-order-request.zip
   ```

2. **Per FTP hochladen**
   - Verbinden Sie sich via FTP mit Ihrem Server
   - Navigieren Sie zu `/wp-content/plugins/`
   - Laden Sie den kompletten Ordner `themisdb-order-request` hoch

3. **Plugin aktivieren**
   - WordPress Admin → **Plugins**
   - Finden Sie "ThemisDB Order Request & Contract Management"
   - Klicken Sie auf **Aktivieren**

### Option 3: WP-CLI (für Entwickler)

```bash
# Navigieren zum WordPress-Verzeichnis
cd /path/to/wordpress

# Plugin installieren
wp plugin install /path/to/themisdb-order-request.zip

# Plugin aktivieren
wp plugin activate themisdb-order-request

# Datenbank-Setup verifizieren
wp db query "SHOW TABLES LIKE 'wp_themisdb_%'"
```

## Ersteinrichtung

### 1. epServer Verbindung konfigurieren

Nach der Aktivierung konfigurieren Sie die epServer-Verbindung:

1. Gehen Sie zu **ThemisDB Orders → Einstellungen**
2. Konfigurieren Sie die epServer-Einstellungen:
   ```
   epServer URL: https://service.themisdb.org:6734
   API Schlüssel: [Optional - für authentifizierte Anfragen]
   ```
3. Klicken Sie auf **"Verbindung testen"**
4. Bei erfolgreicher Verbindung klicken Sie auf **"Daten synchronisieren"**

### 2. E-Mail Einstellungen

Konfigurieren Sie die E-Mail-Einstellungen:

1. **Absender E-Mail**: Ihre geschäftliche E-Mail-Adresse
2. **Absender Name**: Ihr Firmenname oder "ThemisDB Team"

**Empfehlung**: Für zuverlässigen E-Mail-Versand installieren Sie ein SMTP-Plugin:
- WP Mail SMTP
- Post SMTP
- Easy WP SMTP

### 3. PDF-Einstellungen

#### PDF-Generator installieren (Optional, aber empfohlen)

Für hochwertige PDFs installieren Sie `wkhtmltopdf`:

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install wkhtmltopdf
```

**CentOS/RHEL:**
```bash
sudo yum install wkhtmltopdf
```

**macOS:**
```bash
brew install wkhtmltopdf
```

**Windows:**
- Download von: https://wkhtmltopdf.org/downloads.html
- Installieren Sie und fügen Sie den Pfad zur PATH-Variable hinzu

#### PDF-Speicherung konfigurieren

1. **Datenbank** (Standard):
   - PDFs werden direkt in der Datenbank gespeichert
   - Keine zusätzliche Konfiguration nötig
   - Empfohlen für kleinere Installationen

2. **Dateisystem**:
   - PDFs werden unter `wp-content/uploads/themisdb-contracts/` gespeichert
   - Stellen Sie sicher, dass der Ordner beschreibbar ist:
   ```bash
   chmod 755 wp-content/uploads/themisdb-contracts/
   ```

### 4. Frontend-Integration

#### Bestellformular-Seite erstellen

1. **Neue Seite erstellen**:
   - WordPress Admin → **Seiten → Erstellen**
   - Titel: "ThemisDB Bestellen" oder "Jetzt bestellen"

2. **Shortcode einfügen**:
   ```
   [themisdb_order_flow]
   ```

3. **Seite veröffentlichen**:
   - Klicken Sie auf **Veröffentlichen**
   - Optional: Fügen Sie die Seite zur Navigation hinzu

#### Kundenbereich-Seiten erstellen

**Meine Bestellungen**:
1. Neue Seite: "Meine Bestellungen"
2. Shortcode: `[themisdb_my_orders]`
3. Veröffentlichen

**Meine Verträge**:
1. Neue Seite: "Meine Verträge"
2. Shortcode: `[themisdb_my_contracts]`
3. Veröffentlichen

### 5. Rechtliche Seiten verlinken

Stellen Sie sicher, dass Sie Links zu folgenden Seiten haben:
- Allgemeine Geschäftsbedingungen (AGB)
- Datenschutzerklärung
- Impressum

Diese sollten im Bestellformular verlinkt werden.

## Verifizierung der Installation

### Datenbank-Tabellen prüfen

Überprüfen Sie, ob alle Tabellen erstellt wurden:

**Via phpMyAdmin**:
1. Öffnen Sie phpMyAdmin
2. Wählen Sie Ihre WordPress-Datenbank
3. Suchen Sie nach Tabellen mit dem Präfix `wp_themisdb_`:
   - `wp_themisdb_orders`
   - `wp_themisdb_contracts`
   - `wp_themisdb_contract_revisions`
   - `wp_themisdb_products`
   - `wp_themisdb_modules`
   - `wp_themisdb_training_modules`
   - `wp_themisdb_email_log`

**Via WP-CLI**:
```bash
wp db query "SHOW TABLES LIKE 'wp_themisdb_%'"
```

### Funktionstest

1. **Admin-Bereich testen**:
   - Gehen Sie zu **ThemisDB Orders → Produkte**
   - Sie sollten Standard-Produkte sehen (Community, Enterprise, Hyperscaler)

2. **Frontend testen**:
   - Besuchen Sie Ihre Bestellformular-Seite
   - Der 5-Schritt-Dialog sollte angezeigt werden

3. **E-Mail-Test** (Optional):
   - Erstellen Sie eine Test-Bestellung
   - Überprüfen Sie **ThemisDB Orders → E-Mail Log**

## Fehlerbehebung

### Problem: Plugin kann nicht aktiviert werden

**Lösung**:
1. Überprüfen Sie die PHP-Version:
   ```bash
   php -v
   ```
2. Stellen Sie sicher, dass PHP 7.4+ läuft
3. Prüfen Sie das WordPress-Debug-Log:
   ```php
   // In wp-config.php aktivieren
   define('WP_DEBUG', true);
   define('WP_DEBUG_LOG', true);
   ```

### Problem: Datenbanktabellen werden nicht erstellt

**Lösung**:
1. Deaktivieren Sie das Plugin
2. Löschen Sie ggf. vorhandene Tabellen manuell
3. Reaktivieren Sie das Plugin
4. Oder führen Sie SQL manuell aus:
   ```bash
   wp plugin deactivate themisdb-order-request
   wp plugin activate themisdb-order-request
   ```

### Problem: E-Mails werden nicht versendet

**Lösung**:
1. Installieren Sie ein SMTP-Plugin
2. Konfigurieren Sie SMTP-Einstellungen
3. Testen Sie mit: https://wordpress.org/plugins/check-email/

### Problem: PDFs werden nicht generiert

**Lösung**:
1. Installieren Sie wkhtmltopdf (siehe oben)
2. Prüfen Sie, ob es im PATH ist:
   ```bash
   which wkhtmltopdf
   ```
3. Stellen Sie sicher, dass PHP `exec()` erlaubt ist
4. Prüfen Sie php.ini: `disable_functions` sollte `exec` nicht enthalten

### Problem: epServer-Verbindung fehlschlägt

**Lösung**:
1. Prüfen Sie die URL: `https://service.themisdb.org:6734`
2. Testen Sie die Verbindung mit curl:
   ```bash
   curl https://service.themisdb.org:6734/health
   ```
3. Überprüfen Sie Firewall-Regeln
4. Prüfen Sie, ob `allow_url_fopen` in php.ini aktiviert ist

## Deinstallation

### Plugin deaktivieren und löschen

1. **Via WordPress Admin**:
   - Plugins → Installierte Plugins
   - Deaktivieren Sie "ThemisDB Order Request"
   - Klicken Sie auf "Löschen"

2. **Daten behalten oder löschen**:
   - Standardmäßig bleiben Daten erhalten
   - Um Daten zu löschen, bearbeiten Sie `uninstall.php`

### Manuelle Bereinigung (optional)

```sql
-- Tabellen löschen
DROP TABLE IF EXISTS wp_themisdb_orders;
DROP TABLE IF EXISTS wp_themisdb_contracts;
DROP TABLE IF EXISTS wp_themisdb_contract_revisions;
DROP TABLE IF EXISTS wp_themisdb_products;
DROP TABLE IF EXISTS wp_themisdb_modules;
DROP TABLE IF EXISTS wp_themisdb_training_modules;
DROP TABLE IF EXISTS wp_themisdb_email_log;

-- Optionen löschen
DELETE FROM wp_options WHERE option_name LIKE 'themisdb_order_%';
```

## Nächste Schritte

Nach erfolgreicher Installation:

1. 📖 Lesen Sie die [vollständige Dokumentation](README.md)
2. 🎨 Passen Sie das Design an (siehe [Anpassung](README.md#anpassung))
3. 📧 Richten Sie E-Mail-Vorlagen ein
4. 🔐 Konfigurieren Sie Benutzerrollen und Berechtigungen
5. 🧪 Testen Sie den kompletten Bestellprozess
6. 🚀 Gehen Sie live!

## Support

Bei Problemen oder Fragen:
- 📧 E-Mail: support@themisdb.com
- 🐛 GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
- 📚 Dokumentation: https://github.com/makr-code/wordpressPlugins/tree/main/wordpress-plugin

---

**Viel Erfolg mit Ihrem ThemisDB Order Request Plugin!** 🎉
