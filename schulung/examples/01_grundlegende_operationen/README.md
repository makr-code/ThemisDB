# Grundlegende Operationen — CRUD mit ThemisDB

![Schwierigkeit](https://img.shields.io/badge/schwierigkeit-einsteiger-green)
![Dauer](https://img.shields.io/badge/dauer-20--30%20min-blue)

## Übersicht

Dieses Beispiel demonstriert alle grundlegenden CRUD-Operationen mit ThemisDB:

- **C**reate — Dokumente einfügen (`INSERT`, `UPSERT`)
- **R**ead — Dokumente abfragen (`FOR ... FILTER ... RETURN`)
- **U**pdate — Dokumente aktualisieren (`UPDATE`, `REPLACE`)
- **D**elete — Dokumente löschen (`REMOVE`)
- Explizite **Transaktionen**
- **Aggregationen** (`COLLECT`, `AVG`, `COUNT`, ...)

## Voraussetzungen

```bash
# ThemisDB starten
docker run -d --name themisdb -p 8080:8080 themisdb/themisdb:latest

# Abhängigkeiten installieren
pip install themis-client
```

## Ausführen

```bash
cd schulung/examples/01_grundlegende_operationen
python main.py
```

## Erwartete Ausgabe

```
ThemisDB Schulungsbeispiel 1: Grundlegende Operationen
=======================================================
Verbunden mit ThemisDB 1.8.0

=== Setup ===
Collection 'schulung_users' erstellt und geleert.
Eingefügte _keys: ['1234...', ...]

=== Lesen ===
Alle Benutzer (5):
  ✅ Anna     | Berlin     | 30 Jahre
  ✅ Bob      | Hamburg    | 25 Jahre
  ...

=== Transaktion (Kontotransfer) ===
Vor der Überweisung:
  Alice: 1000.00 EUR
  Bob:    500.00 EUR
Nach der Überweisung:
  Alice:  800.00 EUR
  Bob:    700.00 EUR

✅ Beispiel 1 erfolgreich abgeschlossen!
```

## Konzepte

### Collections
ThemisDB speichert Dokumente in **Collections** (ähnlich wie Tabellen oder MongoDB Collections):
```aql
CREATE COLLECTION IF NOT EXISTS users
INSERT { name: "Anna", age: 30 } INTO users
```

### Bind-Variablen
Immer Bind-Variablen statt String-Interpolation verwenden — sicherer und performanter:
```python
client.query(
    "FOR u IN users FILTER u.age > @min_age RETURN u",
    bind_vars={"min_age": 18}
)
```

### Transaktionen
ThemisDB unterstützt explizite ACID-Transaktionen:
```aql
BEGIN TRANSACTION
  UPDATE account1 WITH { balance: balance - 100 } IN accounts
  UPDATE account2 WITH { balance: balance + 100 } IN accounts
COMMIT
```

## Weiterführend

- [AQL Referenz](../../dokumente/02_aql_referenz_kurzuebersicht.md)
- [Nächstes Beispiel: AQL Queries](../02_aql_queries/)
