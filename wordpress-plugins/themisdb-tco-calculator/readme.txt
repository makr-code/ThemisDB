=== ThemisDB TCO Calculator ===
Contributors: themisdb
Tags: database, tco, calculator, cost, analysis
Requires at least: 5.0
Tested up to: 6.7
Requires PHP: 7.4
Stable tag: 1.0.0
License: MIT
License URI: https://opensource.org/licenses/MIT

Total Cost of Ownership Calculator für ThemisDB - Vergleichen Sie die Gesamtbetriebskosten verschiedener Datenbanklösungen.

== Description ==

Der ThemisDB TCO Calculator ist ein interaktives WordPress-Plugin zur Berechnung und zum Vergleich der Total Cost of Ownership (TCO) verschiedener Datenbanklösungen über einen Zeitraum von 3 Jahren.

= Features =

* 💰 **Umfassende Kostenanalyse**: Infrastruktur, Personal, Lizenzen und Betriebskosten
* 📊 **Visuelle Darstellung**: Dynamische Charts mit Chart.js
* 📈 **Jahresvergleich**: Detaillierte Aufschlüsselung über 3 Jahre
* 💡 **Intelligente Insights**: Automatische Analyse und Empfehlungen
* 📥 **Export-Funktionen**: PDF, CSV und Druck-Optionen
* ⚙️ **Admin-Einstellungen**: Anpassbare Standardwerte
* 🔄 **GitHub Auto-Updates**: Automatische Updates direkt von GitHub
* 📱 **Responsive Design**: Optimiert für alle Bildschirmgrößen
* 🎨 **Theme-kompatibel**: Funktioniert mit jedem WordPress-Theme

= Verwendung =

Nach der Installation fügen Sie einfach den Shortcode in eine Seite oder einen Beitrag ein:

`[themisdb_tco_calculator]`

**Optionale Parameter:**
* `show_intro="no"` - Verbirgt die Einführungssektion
* `title="Mein Titel"` - Angepasster Titel

**Beispiele:**

`[themisdb_tco_calculator show_intro="no"]`

`[themisdb_tco_calculator title="Kostenrechner"]`

= Admin-Einstellungen =

Konfigurieren Sie Standardwerte unter: **Einstellungen → TCO Calculator**

* AI Features aktivieren/deaktivieren
* Standard Anfragen pro Tag
* Standard Datengröße (GB)
* Standard Spitzenlast-Faktor
* Standard Verfügbarkeitsanforderung

= GitHub Auto-Updates =

Das Plugin unterstützt automatische Updates direkt von GitHub. Neue Versionen werden automatisch im WordPress-Admin unter "Plugins → Installierte Plugins" angezeigt.

== Installation ==

= Automatische Installation =

1. WordPress Admin → Plugins → Installieren
2. Plugin-ZIP hochladen
3. Plugin aktivieren
4. Shortcode `[themisdb_tco_calculator]` auf einer Seite einfügen

= Manuelle Installation =

1. Plugin-Dateien in `/wp-content/plugins/themisdb-tco-calculator/` hochladen
2. WordPress Admin → Plugins
3. "ThemisDB TCO Calculator" aktivieren
4. Shortcode verwenden

= Nach der Installation =

1. Gehen Sie zu **Einstellungen → TCO Calculator** um Standardwerte zu konfigurieren
2. Erstellen Sie eine neue Seite oder Beitrag
3. Fügen Sie den Shortcode `[themisdb_tco_calculator]` ein
4. Veröffentlichen Sie die Seite

== Frequently Asked Questions ==

= Wie binde ich den Rechner ein? =

Verwenden Sie den Shortcode `[themisdb_tco_calculator]` auf einer beliebigen Seite oder in einem Beitrag.

= Kann ich die Standardwerte anpassen? =

Ja, unter **Einstellungen → TCO Calculator** können Sie alle Standardwerte konfigurieren.

= Funktioniert das Plugin mit meinem Theme? =

Ja, das Plugin ist so entwickelt, dass es mit jedem WordPress-Theme kompatibel ist. Es verwendet eigene CSS-Styles, die nicht mit Theme-Styles kollidieren.

= Werden externe Dienste benötigt? =

Das Plugin lädt Chart.js von einem CDN (jsdelivr.net) für die Visualisierungen. Ansonsten werden keine externen Dienste benötigt.

= Wie funktionieren die Updates? =

Das Plugin unterstützt automatische Updates von GitHub. Neue Versionen werden im WordPress-Admin angezeigt und können mit einem Klick installiert werden.

= Ist das Plugin kostenlos? =

Ja, das Plugin ist Open Source und unter der MIT-Lizenz verfügbar. Es ist komplett kostenlos nutzbar.

= Kann ich das Plugin für kommerzielle Projekte nutzen? =

Ja, die MIT-Lizenz erlaubt kommerzielle Nutzung ohne Einschränkungen.

== Screenshots ==

1. TCO-Rechner Frontend mit Eingabeformular
2. Ergebnis-Dashboard mit Kostenvergleich
3. Interaktive Charts und Visualisierungen
4. Admin-Einstellungsseite
5. Export-Optionen (PDF, CSV)

== Changelog ==

= 1.0.0 - 2026-01-07 =
* Initial release
* Shortcode-Integration `[themisdb_tco_calculator]`
* Admin-Einstellungsseite mit Standardwerten
* GitHub Auto-Update-Unterstützung
* Vollständige TCO-Berechnung
* Chart.js Integration für Visualisierungen
* Export-Funktionen (PDF, CSV, Druck)
* Responsive Design
* WordPress-Standards konform
* Multisite-Unterstützung
* Uninstall-Hook für saubere Deinstallation
* Plugin-Action-Links (Einstellungen)
* Deutsche Lokalisierung

== Upgrade Notice ==

= 1.0.0 =
Erste Veröffentlichung des Plugins.

== Development ==

Das Plugin ist Teil des ThemisDB-Projekts:
* GitHub: https://github.com/makr-code/wordpressPlugins
* Issues: https://github.com/makr-code/wordpressPlugins/issues

== Support ==

Bei Fragen oder Problemen erstellen Sie bitte ein Issue auf GitHub:
https://github.com/makr-code/wordpressPlugins/issues

== License ==

This plugin is licensed under the MIT License.

Copyright (c) 2026 ThemisDB Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
