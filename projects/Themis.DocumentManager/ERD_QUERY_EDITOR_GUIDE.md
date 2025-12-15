# ERM/ERD und Query Editor - Benutzerhandbuch

## Übersicht

Das ThemisDB Document Manager System bietet zwei leistungsstarke Tools für die Arbeit mit der Datenbank:

1. **ERM/ERD Visualisierung** - Interaktive Diagramme des Datenbankschemas
2. **Query Editor** - Professioneller Editor für AQL-Abfragen

## ERM/ERD Visualisierung

### Was ist ein ERD?

Ein Entity-Relationship-Diagramm (ERD) visualisiert die Struktur einer Datenbank:
- **Entitäten** (Entities): Tabellen/Collections in der Datenbank
- **Attribute**: Felder/Spalten einer Entität
- **Beziehungen** (Relationships): Verbindungen zwischen Entitäten

### Zugriff

Navigieren Sie zum Tab **"🗂️ ERD"** im Hauptfenster des Document Managers.

### Funktionen

#### Entitäten anzeigen

Jede Entität wird als Karte dargestellt mit:
- **Header**: Name und Typ der Entität (z.B. "Collection")
- **Attribute**: Liste aller Felder mit Datentyp
- **Primärschlüssel**: Mit 🔑-Symbol markiert
- **Beschreibung**: Zusätzliche Informationen zur Entität

#### Beziehungen visualisieren

Beziehungen zwischen Entitäten werden als gestrichelte Linien dargestellt:
- **OneToOne**: 1:1 Beziehung
- **OneToMany**: 1:N Beziehung  
- **ManyToOne**: N:1 Beziehung
- **ManyToMany**: N:M Beziehung

#### Navigation

- **Zoom**: Verwenden Sie die `➕` und `➖` Buttons oder das Mausrad
- **Pan**: Scrollen Sie mit den Scrollbars
- **Select**: Klicken Sie auf eine Entität, um Details anzuzeigen
- **Reset**: Button `⊡` setzt Zoom und Position zurück

#### Auto-Layout

Der Button **"📐 Auto-Layout"** ordnet alle Entitäten automatisch in einem Raster an.

#### Schema aktualisieren

Der Button **"🔄 Aktualisieren"** lädt das Schema neu aus ThemisDB.

### Details-Sidebar

Die rechte Sidebar zeigt Details zur ausgewählten Entität:
- Name und Typ
- Beschreibung
- Anzahl der Attribute
- Statistiken über das gesamte Schema

### Vordefinierte Entitäten

Das System zeigt folgende ThemisDB-Entitäten:

1. **documents** - Hauptdokumente
2. **document_revisions** - Dokumentrevisionen
3. **timeline_events** - Timeline-Ereignisse
4. **users** - Benutzerkonten
5. **processes** - Geschäftsprozesse
6. **tasks** - Aufgaben
7. **favorites** - Favoriten
8. **audit_log** - Audit-Trail

## Query Editor

### Was ist AQL?

AQL (ArangoDB Query Language) ist die Query-Sprache von ThemisDB. Sie ähnelt SQL, ist aber für Multi-Model-Datenbanken optimiert.

### Zugriff

Navigieren Sie zum Tab **"🔍 Query"** im Hauptfenster.

### Aufbau

Der Query Editor besteht aus drei Bereichen:

1. **Gespeicherte Queries** (links): Liste vorkonfigurierter Queries
2. **Query Editor** (oben): Textfeld für Query-Eingabe
3. **Ergebnisse** (unten): Tabellarische Darstellung der Resultate

### Grundlegende Verwendung

#### Query schreiben

```aql
FOR doc IN documents
  LIMIT 10
  RETURN doc
```

#### Query ausführen

1. Query in das Textfeld eingeben
2. Button **"▶ Ausführen"** klicken oder `F5` drücken
3. Ergebnisse werden in der unteren Tabelle angezeigt

#### Query validieren

Der Button **"✓ Validieren"** prüft die Syntax, ohne die Query auszuführen.

### AQL Syntax

#### FOR-Schleife

```aql
FOR variable IN collection
  RETURN variable
```

#### Filter

```aql
FOR doc IN documents
  FILTER doc.status == "active"
  RETURN doc
```

#### Sortierung

```aql
FOR doc IN documents
  SORT doc.created_at DESC
  RETURN doc
```

#### Limit

```aql
FOR doc IN documents
  LIMIT 10
  RETURN doc
```

#### Joins (mit LET)

```aql
FOR doc IN documents
  LET revisions = (
    FOR rev IN document_revisions
      FILTER rev.document_id == doc._key
      RETURN rev
  )
  RETURN { document: doc, revisions: revisions }
```

#### Aggregation

```aql
FOR doc IN documents
  COLLECT status = doc.status WITH COUNT INTO count
  RETURN { status: status, count: count }
```

### Gespeicherte Queries

#### Query speichern

1. Query schreiben
2. Button **"💾 Speichern"** klicken
3. Query wird zur Liste hinzugefügt

#### Query laden

1. Query in der linken Liste auswählen
2. Doppelklick lädt die Query in den Editor

#### Query löschen

Klicken Sie auf das **🗑️** Symbol neben der Query.

### Vorkonfigurierte Beispiel-Queries

1. **All Documents** - Alle Dokumente abrufen
2. **Recent Documents** - Dokumente der letzten 7 Tage
3. **Documents by Tag** - Dokumente mit bestimmtem Tag
4. **Document with Revisions** - Dokument mit allen Revisionen
5. **User Activity** - Timeline-Events eines Benutzers
6. **Documents by Process** - Dokumente eines Prozesses

### Ergebnisse

#### Anzeige

- Ergebnisse werden als Tabelle dargestellt
- Jede Spalte entspricht einem Feld im Ergebnis-Dokument
- Verschachtelte Objekte werden als JSON angezeigt

#### Metriken

- **Anzahl Zeilen**: Anzahl der zurückgegebenen Datensätze
- **Ausführungszeit**: Zeit in Millisekunden

### Fehlerbehandlung

Bei Fehlern wird eine rote Box mit Fehlermeldung angezeigt:
- Syntax-Fehler
- Ungültige Collection-Namen
- Fehlerhafte Filter-Bedingungen

### Tastaturkürzel

- **F5**: Query ausführen
- **Ctrl+S**: Query speichern (geplant)
- **Ctrl+N**: Neue Query (geplant)

## Best Practices

### ERD

1. **Schema regelmäßig aktualisieren** nach Änderungen an der Datenbank
2. **Auto-Layout verwenden** für bessere Übersicht bei vielen Entitäten
3. **Details-Sidebar nutzen** um Attribut-Details zu prüfen

### Query Editor

1. **Queries validieren** vor der Ausführung
2. **LIMIT verwenden** bei großen Collections
3. **Queries speichern** für häufig benötigte Abfragen
4. **Beispiel-Queries** als Ausgangspunkt nutzen
5. **WHERE-Klauseln** für performante Abfragen einsetzen

## Beispiele

### Dokumente der letzten 7 Tage

```aql
FOR doc IN documents
  FILTER doc.created_at >= DATE_SUBTRACT(DATE_NOW(), 7, 'days')
  SORT doc.created_at DESC
  RETURN doc
```

### Dokumente mit Anzahl Revisionen

```aql
FOR doc IN documents
  LET revision_count = LENGTH(
    FOR rev IN document_revisions
      FILTER rev.document_id == doc._key
      RETURN 1
  )
  RETURN {
    title: doc.title,
    created_at: doc.created_at,
    revision_count: revision_count
  }
```

### Top 10 aktivste Benutzer

```aql
FOR event IN timeline_events
  COLLECT user = event.user WITH COUNT INTO count
  SORT count DESC
  LIMIT 10
  RETURN { user: user, events: count }
```

### Dokumente mit Tags

```aql
FOR doc IN documents
  FILTER LENGTH(doc.tags) > 0
  RETURN {
    title: doc.title,
    tags: doc.tags,
    created_at: doc.created_at
  }
```

## Fehlerbehebung

### ERD lädt nicht

- Prüfen Sie, ob ThemisDB läuft (`http://localhost:8765`)
- Klicken Sie auf "🔄 Aktualisieren"
- Prüfen Sie die Status-Meldung am unteren Rand

### Query schlägt fehl

- Validieren Sie die Syntax mit "✓ Validieren"
- Prüfen Sie Collection-Namen
- Verwenden Sie einfachere Queries zum Testen
- Prüfen Sie die Fehler-Meldung für Details

### Keine Ergebnisse

- Prüfen Sie FILTER-Bedingungen
- Verwenden Sie LIMIT, um alle Dokumente zu sehen
- Prüfen Sie, ob die Collection Daten enthält

## Weiterführende Informationen

- [AQL Documentation](https://www.arangodb.com/docs/stable/aql/)
- [ThemisDB API Documentation](../../docs/openapi.yaml)
- [Document Manager README](README.md)
