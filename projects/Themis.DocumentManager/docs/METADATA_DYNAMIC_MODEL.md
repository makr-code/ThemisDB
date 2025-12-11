# Dynamisches Metadaten-Modell (ThemisDB)

## Ziel
Vollständig schemaflexible Metadaten für Dokumente, Vorgänge, Akten, Ordner und Prozesse; Validierung über Templates; Versionierung und Audit; einfache Verknüpfung zu Aufgaben/Timeline.

## Kern-Collections
- `metadata_templates`
  - `id`, `name`, `version`, `entity_types[]`
  - `fields[]`: `key`, `label`, `data_type` (string|number|bool|date|datetime|list|string[]|json|lookup:<key>|ref:<entity>|file), `is_required`, `options/lookup`, `validation` (regex/min/max), `group`, `order`
  - `created_at`, `created_by`
- `metadata_values`
  - `id`, `entity_type`, `entity_id`, `template_id` (optional), `version`, `values` (Dictionary<string, object>), `created_at/created_by`, `updated_at/updated_by`, optional `is_current`
- `metadata_audit`
  - `id`, `entity_type`, `entity_id`, `version`, `changed_at`, `changed_by`, `diff_json`
- `lookups` (optional)
  - `id`, `key`, `values[]`, `updated_at`

### Graph / Relations
- `entity_graph`
  - `id`, `source_type`, `source_id`, `target_type`, `target_id`, `relation` (e.g. references/derives_from/is_part_of/blocks/duplicates), `weight` (optional), `created_at`
  - Zweck: Querbeziehungen dokumentieren (Dokument ↔ Vorgang ↔ Prozessschritt; Duplikate; Abhängigkeiten)

### Vektor-Suche / Semantik
- `entity_vectors`
  - `id`, `entity_type`, `entity_id`, `embedding` (vector), `model`, `dimensions`, `created_at`
  - Optional: `chunk_id`, `chunk_range` für Teil-Embeddings großer Dokumente

### Zeitliche Daten / Events
- `timeline_items`
  - `id`, `entity_type`, `entity_id`, `title`, `description`, `timestamp`, `icon`, `category` (milestone/statuschange/note), optional `process_id`, `payload_json`
- `time_series` (optional, falls Metriken/Verläufe gespeichert werden)
  - `id`, `entity_type`, `entity_id`, `metric`, `timestamp`, `value_number`, `value_json`

### Geo-Spatial
- `entity_geo`
  - `id`, `entity_type`, `entity_id`, `geo_json` (Point/Polygon), `srid` (z.B. 4326), `updated_at`
  - Index: Geo-Index auf `geo_json`

### Prozesse / Workflow
- `processes`
  - `id`, `case_id` (optional), `name`, `process_type`, `status`, `started_at`, `ended_at`, `created_by`
- `process_steps`
  - `id`, `process_id`, `step_key`, `status`, `assignee`, `started_at`, `ended_at`, `payload_json`
- `process_links`
  - `process_id`, `entity_type`, `entity_id` (verknüpft Prozess mit Dokument/Vorgang/Akte)

## Verknüpfungen zu anderen Features
- Aufgaben (`tasks`, `task_links`): `task_links` referenziert `entity_type/entity_id` und optional `process_id`.
- Timeline/Audit (`timeline_items`, `audit_logs`): ebenfalls nur `entity_type/entity_id` + Payload JSON.
- Dokument/Akte/Vorgang/Prozess bleiben eigene Entitäten; Metadaten hängen dynamisch über `metadata_values`.
- Graph: `entity_graph` verbindet Entitäten (Abhängigkeiten, Duplikate, Referenzen).
- Vektor: `entity_vectors` erlaubt semantische Suche/Ähnlichkeit (Dokumenten-Embeddings, Chunking).
- Geo: `entity_geo` ermöglicht räumliche Filter/Mapping (Standorte von Projekten/Objekten).
- Prozess: `process_links` koppelt Prozesse an fachliche Objekte; `process_steps` für Workflow-Zustände.

## Versionierung & Audit
- Updates erzeugen neue Version in `metadata_values`; `is_current` oder höchste Version auswählen.
- Änderungen in `metadata_audit` als Diff (z.B. JSON Patch) speichern.

## Indexierungsempfehlungen
- `metadata_values (entity_type, entity_id)`
- Optional: Sekundärindex auf häufige Felder (falls der Store JSON-Indexes erlaubt), z.B. `values.customer_number`.
- `metadata_values (template_id, entity_type)` für Template-bezogene Auswertungen.
- Graph: Index auf `(source_type, source_id)`, `(target_type, target_id)`, `(relation)`
- Vektor: Vektorindex (HNSW/IVF) auf `embedding`
- Geo: Geo-Index (R-Tree) auf `geo_json`
- Timeline: Index `(entity_type, entity_id, timestamp)`, `(process_id, timestamp)`

## Validierung
- Laufzeit-Validierung gegen Template-Felddeskriptoren: Required, Typ, Regex/Min/Max, Lookup-Werte.
- Optional: JSON-Schema pro Template ableiten.

## Beispiel-Dokument (metadata_values)
```json
{
  "id": "meta_123",
  "entity_type": "document",
  "entity_id": "doc_001",
  "template_id": "tmpl_invoice_v1",
  "version": 3,
  "values": {
    "customer_number": "C-99812",
    "invoice_date": "2025-12-01",
    "amount": 12345.67,
    "currency": "EUR",
    "cost_center": "CC-42",
    "tags": ["bau", "muc"],
    "attachment_ref": null
  },
  "created_at": "2025-12-10T10:00:00Z",
  "created_by": "user:a",
  "updated_at": "2025-12-10T10:30:00Z",
  "updated_by": "user:b"
}
```

## Erweiterungsschritte in der App
1) Templates laden: `metadata_templates` einlesen, Feld-Deskriptoren in Form-Renderer binden.
2) Werte lesen: `metadata_values` per `entity_type/entity_id` + `is_current` abrufen.
3) Schreiben: Validieren gegen Template, neue Version anlegen, Audit-Eintrag erzeugen.
4) UI: Dynamische Form-Renderer nutzen (bereits vorhanden), Validation-Errors anzeigen.
5) Tasks/Timeline: Filter per `entity_type/entity_id` verwenden (bereits in Task-Filter vorbereitet).

## Offene Optionen
- Mehrsprachige Labels/Hints im Template (`label_i18n`).
- Feld-Layouts (Grid/Group/Step) im Template ablegen.
- Server-seitige Policies: Feld-Maskierung/Readonly je Rolle.
- Delta-Speicherung: `diff_json` oder `json_patch` zwischen Versionen.
