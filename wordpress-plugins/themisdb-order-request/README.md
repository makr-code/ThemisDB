# ThemisDB Order Request & Contract Management Plugin

Ein umfassendes WordPress-Plugin für automatisierte Bestellanfragen und Vertragsverwaltung für ThemisDB mit vollständiger rechtlicher Compliance, PDF-Generierung und E-Mail-Integration.

## 🎯 Überblick

Dieses Plugin bietet einen dialog-basierten Bestellprozess für ThemisDB-Produkte, Module und Schulungen mit automatischer Vertragserstellung, PDF-Generierung und E-Mail-Versand. Es erfüllt alle Aspekte des Vertragsrechts (CRUD) und integriert sich nahtlos mit dem epServer für Stammdatenverwaltung.

## ✨ Hauptfunktionen

### Dialog-basierter Bestellprozess
- **Schritt 1**: Produktauswahl (Community, Enterprise, Hyperscaler)
- **Schritt 2**: Modulauswahl (Vector Search, LLM Integration, Graph DB, etc.)
- **Schritt 3**: Schulungen (Online, On-Site, verschiedene Themen)
- **Schritt 4**: Kundendaten (Name, E-Mail, Firma)
- **Schritt 5**: Zusammenfassung und Bestätigung

### Vertragsrechtliche Funktionen (CRUD)
- ✅ **Create**: Automatische Vertragserstellung aus Bestellungen
- ✅ **Read**: Vollständige Vertragsansicht mit allen Details
- ✅ **Update**: Versionierung mit Revisionshistorie
- ✅ **Delete**: Sichere Löschung mit Backup

### Zahlungsverifizierung
- 💰 **Zahlungsüberwachung**: Automatische Erfassung aller Zahlungen
- ✅ **Verifizierung**: Manuelle und automatische Zahlungsbestätigung
- 📊 **Status-Tracking**: Pending, Verified, Failed
- 💳 **Transaktions-IDs**: Vollständige Nachverfolgbarkeit
- 📈 **Statistiken**: Zahlungsübersicht und Auswertungen
- 🔄 **epServer-Sync**: Automatische Synchronisation mit epServer

### Lizenzverwaltung
- 🔑 **Automatische Lizenzgenerierung**: Nach jeder Bestellung
- 📄 **Lizenzdatei**: Sichere JSON-Datei mit digitaler Signatur
- ⏱️ **Status-Management**: Pending, Active, Suspended, Expired
- 🎯 **Limitierungen**: Node-, Core- und Storage-Limits
- 🔐 **Validierung**: Echtheitsprüfung und Ablaufdatum-Check
- 📱 **epServer-Integration**: Echtzeit-Lizenzvalidierung

### Anmeldesystem mit Lizenzdatei
- 🔓 **Dual-Login**: Standard (Username/Password) + Lizenzdatei
- 📤 **Lizenz-Upload**: Sichere Lizenzdatei-Authentifizierung (.json)
- ✍️ **Signatur-Prüfung**: HMAC-SHA256 Verifikation
- 👤 **Auto-User-Creation**: Automatische Benutzererstellung aus Lizenz
- 🔒 **Session-Management**: Sichere Sitzungsverwaltung
- 📋 **Audit-Log**: Vollständige Authentifizierungs-Historie

### PDF-Generierung
- Automatische PDF-Erstellung für Verträge
- Professionelle Vorlagen mit Firmenbranding
- Speicherung in Datenbank oder Dateisystem
- Rechtlich konforme Dokumentation

### E-Mail-Funktionen
- Bestellbestätigung mit PDF-Anhang
- Vertragsversand per E-Mail
- E-Mail-Logging für Nachverfolgbarkeit
- Anpassbare E-Mail-Vorlagen

### epServer Integration
- Synchronisation von Produktdaten
- Automatische Kundenregistrierung
- Abonnement-Verwaltung
- Lizenzschlüssel-Validierung
- Zahlungsstatus-Abfragen
- Lizenzinformations-Abfragen

## 📋 Systemanforderungen

- **WordPress**: 5.0 oder höher
- **PHP**: 7.4 oder höher
- **MySQL**: 5.6 oder höher (oder MariaDB)
- **Speicherplatz**: Min. 50 MB
- **Optional**: wkhtmltopdf für PDF-Generierung (empfohlen)

## 🚀 Installation

### Methode 1: WordPress Admin
1. Laden Sie das Plugin als ZIP herunter
2. WordPress Admin → Plugins → Installieren → Plugin hochladen
3. Wählen Sie die ZIP-Datei aus
4. Klicken Sie auf "Jetzt installieren"
5. Aktivieren Sie das Plugin

### Methode 2: Manuell via FTP
1. Entpacken Sie die ZIP-Datei
2. Laden Sie den Ordner `themisdb-order-request` nach `wp-content/plugins/` hoch
3. WordPress Admin → Plugins → Aktivieren

## ⚙️ Konfiguration

### epServer Verbindung einrichten

1. WordPress Admin → **ThemisDB Orders → Einstellungen**
2. Konfigurieren Sie:
   - **epServer URL**: `https://service.themisdb.org:6734` (Standard)
   - **API Schlüssel**: Optional für authentifizierte Anfragen
3. Klicken Sie auf **"Verbindung testen"**
4. Klicken Sie auf **"Daten synchronisieren"** um Produkte zu aktualisieren

### E-Mail Einstellungen

1. **Absender E-Mail**: Die E-Mail-Adresse für ausgehende Nachrichten
2. **Absender Name**: Der Name, der in E-Mails angezeigt wird

### PDF Einstellungen

**Speicherung**:
- **Datenbank**: PDFs werden in der Datenbank gespeichert (Standard)
- **Dateisystem**: PDFs werden als Dateien unter `wp-content/uploads/themisdb-contracts/` gespeichert

**PDF-Generierung**:
- Installieren Sie `wkhtmltopdf` für optimale PDF-Qualität:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install wkhtmltopdf
  
  # macOS
  brew install wkhtmltopdf
  ```

## 📖 Verwendung

### Frontend Integration

#### Bestellformular einbinden

Verwenden Sie den Shortcode auf einer Seite oder in einem Beitrag:

```
[themisdb_order_flow]
```

#### Kundenbereichseiten

**Meine Bestellungen**:
```
[themisdb_my_orders]
```

**Meine Verträge**:
```
[themisdb_my_contracts]
```

**Anmeldeformular** (Standard + Lizenzdatei):
```
[themisdb_login]
```

**Lizenz-Upload**:
```
[themisdb_license_upload]
```

### Admin-Bereich

#### Bestellungen verwalten

1. **ThemisDB Orders → Bestellungen**
2. Sehen Sie alle Bestellungen mit Status
3. Klicken Sie auf "Ansehen" für Details
4. Status ändern: Draft → Pending → Processing → Completed

#### Zahlungen verwalten

1. **ThemisDB Orders → Zahlungen**
2. Übersicht aller Zahlungen mit Status (Pending, Verified, Failed)
3. Klicken Sie auf "Ansehen" für Details
4. **"Verifizieren"** Button für manuelle Zahlungsbestätigung
5. Statistik-Dashboard:
   - Gesamt Zahlungen und Betrag
   - Verifizierte Zahlungen und Betrag
   - Ausstehende und fehlgeschlagene Zahlungen
6. Automatische Lizenzaktivierung nach Zahlungsverifizierung

#### Lizenzen verwalten

1. **ThemisDB Orders → Lizenzen**
2. Übersicht aller Lizenzen mit Status (Pending, Active, Suspended, Expired)
3. Klicken Sie auf "Ansehen" für Details:
   - Vollständiger Lizenzschlüssel
   - Edition und Typ
   - Node-, Core-, Storage-Limits
   - Aktivierungs- und Ablaufdatum
   - Zugehörige Bestellung und Vertrag
   - Lizenzdatei (JSON) zum Download
4. Statistik-Dashboard:
   - Gesamt Lizenzen
   - Aktive, ausstehende, suspendierte Lizenzen

#### Verträge verwalten

1. **ThemisDB Orders → Verträge**
2. Alle Verträge mit Gültigkeitsdaten
3. PDF herunterladen oder per E-Mail versenden
4. Revisionshistorie einsehen

#### Produkte und Module

1. **ThemisDB Orders → Produkte**
2. Übersicht aller verfügbaren Produkte, Module und Schulungen
3. Preise werden automatisch vom epServer synchronisiert

#### E-Mail Log

1. **ThemisDB Orders → E-Mail Log**
2. Alle gesendeten E-Mails mit Status
3. Fehlersuche bei E-Mail-Problemen

### Anmeldesystem nutzen

#### Als Kunde - Standard-Anmeldung

1. Gehen Sie zur Login-Seite: `[themisdb_login]`
2. Wählen Sie Tab **"Standard-Anmeldung"**
3. Geben Sie E-Mail/Benutzername und Passwort ein
4. Optional: "Angemeldet bleiben" aktivieren
5. Klicken Sie auf "Anmelden"

#### Als Kunde - Lizenz-Anmeldung

1. Gehen Sie zur Login-Seite: `[themisdb_login]`
2. Wählen Sie Tab **"Lizenz-Anmeldung"**
3. Laden Sie Ihre Lizenzdatei (.json) hoch
4. System verifiziert automatisch die Lizenz
5. Bei erfolgreicher Verifizierung werden Sie eingeloggt
6. Falls noch kein Account existiert, wird dieser automatisch erstellt

**Lizenzdatei-Format**:
```json
{
  "version": "1.0",
  "license_key": "ENT-XXXX-XXXX-XXXX-XXXX-XXXX-XXXX-XXXX",
  "product_edition": "enterprise",
  "customer_name": "Max Mustermann",
  "customer_email": "max@example.com",
  "max_nodes": 100,
  "expiry_date": "2027-01-08",
  "signature": "..."
}
```

#### Ablauf: Von Bestellung bis Login

1. **Kunde bestellt** → Bestellung wird erstellt
2. **System erstellt automatisch**:
   - Zahlung (Status: Pending)
   - Vertrag mit PDF
   - Lizenz mit Lizenzdatei
3. **Admin verifiziert Zahlung** → Lizenz wird aktiviert
4. **Kunde erhält E-Mail** mit:
   - Bestellbestätigung
   - Vertrag als PDF
   - Download-Link für Lizenzdatei (optional)
5. **Kunde kann sich anmelden**:
   - Mit Lizenzdatei hochladen
   - Automatisch eingeloggt
   - Zugriff auf Bestellungen und Verträge

## 🗄️ Datenbankstruktur

Das Plugin erstellt folgende Tabellen:

- `wp_themisdb_orders` - Bestellungen
- `wp_themisdb_contracts` - Verträge
- `wp_themisdb_contract_revisions` - Vertragsrevisionen
- `wp_themisdb_payments` - Zahlungen mit Verifizierung
- `wp_themisdb_licenses` - Lizenzen mit Lizenzdateien
- `wp_themisdb_license_auth_log` - Authentifizierungs-Log
- `wp_themisdb_products` - Produkte
- `wp_themisdb_modules` - Module
- `wp_themisdb_training_modules` - Schulungen
- `wp_themisdb_email_log` - E-Mail Protokoll

## 🔐 Sicherheit und Compliance

### Rechtliche Compliance
- DSGVO-konform durch Einwilligungssystem
- Vollständige Revisionshistorie für Verträge
- Sichere Speicherung sensibler Daten
- E-Mail-Protokollierung für Nachweise

### Datenschutz
- Alle personenbezogenen Daten werden verschlüsselt gespeichert
- Benutzer können ihre Daten einsehen und löschen
- Transparente Datenverarbeitung

### Best Practices
- SQL-Injection-Schutz durch prepared statements
- XSS-Schutz durch sanitize/escape functions
- CSRF-Schutz durch nonces
- Sichere AJAX-Kommunikation

## 🎨 Anpassung

### CSS anpassen

Erstellen Sie eine custom CSS-Datei in Ihrem Theme:

```css
/* Überschreiben Sie Plugin-Styles */
.themisdb-order-flow {
    /* Ihre Anpassungen */
}
```

### E-Mail-Vorlagen anpassen

Filter verwenden, um E-Mail-Templates zu überschreiben:

```php
add_filter('themisdb_order_confirmation_template', function($html, $order) {
    // Ihre angepasste HTML-Template
    return $html;
}, 10, 2);
```

### PDF-Vorlagen anpassen

```php
add_filter('themisdb_contract_pdf_html', function($html, $contract, $order) {
    // Ihre angepasste PDF-Template
    return $html;
}, 10, 3);
```

## 🔄 Workflow-Beispiel

### Typischer Bestellablauf

1. **Kunde** besucht die Bestellseite
2. **Schritt 1**: Wählt ThemisDB Enterprise Edition
3. **Schritt 2**: Fügt Module hinzu (Vector Search, Sharding)
4. **Schritt 3**: Wählt Schulungen aus (Admin Training)
5. **Schritt 4**: Gibt Kontaktdaten ein
6. **Schritt 5**: Bestätigt die Bestellung

**Automatischer Prozess**:
- ✅ Bestellung wird in Datenbank gespeichert
- ✅ Bestellbestätigung per E-Mail mit PDF
- ✅ Vertrag wird automatisch erstellt
- ✅ Vertrags-PDF wird generiert und gespeichert
- ✅ Vertrag wird per E-Mail versendet
- ✅ Optional: Kunde wird im epServer registriert

### Vertragsänderung (Rechtlich konform)

1. Admin öffnet Vertrag
2. Ändert Vertragsdetails
3. System erstellt automatisch neue Revision
4. Änderungsgrund wird protokolliert
5. Neue Version wird gespeichert
6. Kunde wird über Änderung informiert

## 🐛 Fehlerbehebung

### E-Mails werden nicht versendet

1. Prüfen Sie die E-Mail-Einstellungen in WordPress
2. Testen Sie mit einem SMTP-Plugin (z.B. WP Mail SMTP)
3. Überprüfen Sie den E-Mail-Log: **ThemisDB Orders → E-Mail Log**

### PDFs werden nicht generiert

1. Installieren Sie wkhtmltopdf:
   ```bash
   which wkhtmltopdf
   ```
2. Stellen Sie sicher, dass PHP exec() aktiviert ist
3. Prüfen Sie Schreibrechte für Upload-Verzeichnis

### epServer-Verbindung fehlgeschlagen

1. Prüfen Sie die URL: `https://service.themisdb.org:6734`
2. Testen Sie die Verbindung: **Einstellungen → Verbindung testen**
3. Überprüfen Sie Firewall-Regeln
4. Kontaktieren Sie den epServer-Administrator

### Datenbank-Fehler

1. Deaktivieren und reaktivieren Sie das Plugin
2. Prüfen Sie MySQL-Logs
3. Stellen Sie sicher, dass ausreichend Speicherplatz vorhanden ist

## 📊 Performance

### Optimierung

- **Cache**: Plugin nutzt WordPress Transient API
- **Lazy Loading**: Bilder und Daten werden bei Bedarf geladen
- **Database Indexing**: Optimierte Indizes für schnelle Abfragen
- **AJAX**: Asynchrone Anfragen für bessere UX

### Empfohlene Einstellungen

- **PHP Memory Limit**: Min. 128 MB (256 MB empfohlen)
- **Max Execution Time**: Min. 60 Sekunden
- **Upload Max Filesize**: Min. 10 MB (für PDFs)

## 🔌 Hooks und Filter

### Actions

```php
// Nach Bestellung erstellt
do_action('themisdb_after_order_created', $order_id);

// Nach Vertrag erstellt
do_action('themisdb_after_contract_created', $contract_id);

// Vor E-Mail-Versand
do_action('themisdb_before_email_send', $to, $subject, $message);
```

### Filters

```php
// Order-Daten modifizieren
$order_data = apply_filters('themisdb_order_data', $order_data);

// Vertragsdaten modifizieren
$contract_data = apply_filters('themisdb_contract_data', $contract_data);

// E-Mail-Template ändern
$html = apply_filters('themisdb_email_template', $html, $type, $data);
```

## 📞 Support

- **GitHub Issues**: https://github.com/makr-code/wordpressPlugins/issues
- **Dokumentation**: https://github.com/makr-code/wordpressPlugins/tree/main/wordpress-plugin
- **E-Mail**: support@themisdb.com

## 📄 Lizenz

MIT License - Siehe [LICENSE](../../LICENSE) Datei

## 🙏 Credits

Entwickelt mit ❤️ für das ThemisDB-Projekt

- **ThemisDB Team**: https://github.com/makr-code/wordpressPlugins
- **Contributors**: Siehe GitHub Contributors

## 🗺️ Roadmap

### Geplante Features

- [ ] Multi-Währungs-Unterstützung
- [ ] Automatische Rechnungserstellung
- [ ] Zahlungsintegration (Stripe, PayPal)
- [ ] Elektronische Signaturen
- [ ] Multi-Sprachen-Support
- [ ] Erweiterte Reporting-Funktionen
- [ ] Workflow-Automatisierung
- [ ] Mobile App Integration

## 📚 Weitere Dokumentation

- [API-Dokumentation](API.md)
- [Entwickler-Handbuch](DEVELOPMENT.md)
- [Changelog](CHANGELOG.md)
- [Upgrade-Guide](UPGRADE.md)

---

**Version**: 1.0.0  
**Letzte Aktualisierung**: 2026-01-08  
**Autor**: ThemisDB Team
