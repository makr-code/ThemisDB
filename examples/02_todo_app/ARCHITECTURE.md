> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Todo-App - Architektur und Datenmodell

## Übersicht

Die Todo-App demonstriert eine einfache, aber vollständige Anwendung mit ThemisDB als Backend. Die Architektur folgt dem MVC-Pattern (Model-View-Controller) und ist für Erweiterbarkeit konzipiert.

## Systemarchitektur

```
┌─────────────────────────────────────────┐
│          Tkinter UI (View)              │
│  ┌──────────────┐  ┌─────────────────┐  │
│  │  Task List   │  │  Detail Panel   │  │
│  │  (Treeview)  │  │  (Form)         │  │
│  └──────────────┘  └─────────────────┘  │
└───────────────┬─────────────────────────┘
                │
┌───────────────▼─────────────────────────┐
│     Application Logic (Controller)      │
│  • Event Handler                        │
│  • Business Logic                       │
│  • Data Validation                      │
└───────────────┬─────────────────────────┘
                │
┌───────────────▼─────────────────────────┐
│       ThemisDB Client (Model)           │
│  • CRUD Operations                      │
│  • Query Building                       │
│  • Error Handling                       │
└───────────────┬─────────────────────────┘
                │
┌───────────────▼─────────────────────────┐
│          ThemisDB Server                │
│  • Relational Model                     │
│  • Secondary Indexes                    │
│  • Transaction Support                  │
└─────────────────────────────────────────┘
```

## Datenmodell

### Task Entity

Die Hauptentität ist `Task` mit folgenden Feldern:

```python
@dataclass
class Task:
    id: str                    # Eindeutige ID (UUID)
    title: str                 # Titel der Aufgabe (max 200 Zeichen)
    description: str           # Detaillierte Beschreibung
    status: TaskStatus         # Status: OPEN, IN_PROGRESS, DONE
    priority: TaskPriority     # Priorität: LOW, NORMAL, HIGH
    created_at: datetime       # Erstellungszeitpunkt
    updated_at: datetime       # Letzte Änderung
    due_date: Optional[datetime]  # Fälligkeitsdatum (optional)
    tags: List[str]           # Tags für Kategorisierung
```

### Enumerationen

```python
class TaskStatus(Enum):
    OPEN = "open"              # Neu/Offen
    IN_PROGRESS = "in_progress"  # In Bearbeitung
    DONE = "done"              # Erledigt

class TaskPriority(Enum):
    LOW = "low"                # Niedrige Priorität
    NORMAL = "normal"          # Normale Priorität
    HIGH = "high"              # Hohe Priorität
```

## Datenbankschema

### Collection: tasks

```json
{
  "name": "tasks",
  "type": "relational",
  "schema": {
    "id": "string",
    "title": "string",
    "description": "string",
    "status": "string",
    "priority": "string",
    "created_at": "timestamp",
    "updated_at": "timestamp",
    "due_date": "timestamp?",
    "tags": "array<string>"
  },
  "indexes": [
    {
      "name": "status_idx",
      "fields": ["status"],
      "type": "secondary"
    },
    {
      "name": "priority_idx",
      "fields": ["priority"],
      "type": "secondary"
    },
    {
      "name": "due_date_idx",
      "fields": ["due_date"],
      "type": "secondary"
    }
  ]
}
```

### Indizes für Performance

1. **status_idx**: Schnelle Filterung nach Status
   - Verwendet für: Status-Filter in der UI
   - Query: `WHERE status = 'open'`

2. **priority_idx**: Schnelle Filterung nach Priorität
   - Verwendet für: Prioritäts-Filter
   - Query: `WHERE priority = 'high'`

3. **due_date_idx**: Sortierung nach Fälligkeitsdatum
   - Verwendet für: Deadline-Ansicht
   - Query: `ORDER BY due_date ASC`

## Komponenten-Struktur

### 1. UI-Komponenten (main.py)

```python
class TodoApp:
    def __init__(self):
        self.root = tk.Tk()
        self.client = TodoClient()
        self.setup_ui()
        
    def setup_ui(self):
        """Erstellt das UI-Layout"""
        self.create_toolbar()      # Buttons für Aktionen
        self.create_task_list()    # Treeview für Tasks
        self.create_detail_panel() # Formular für Details
        self.create_statusbar()    # Statusleiste
```

### 2. Client-Logik (themis_client.py)

```python
class TodoClient:
    def create_task(self, task: Task) -> bool
    def get_task(self, task_id: str) -> Optional[Task]
    def update_task(self, task: Task) -> bool
    def delete_task(self, task_id: str) -> bool
    def list_tasks(self, filters: Dict) -> List[Task]
    def search_tasks(self, query: str) -> List[Task]
```

### 3. Datenmodelle (models.py)

```python
# Dataclasses für Type Safety
@dataclass
class Task:
    # Felder mit Type Hints
    
    def to_dict(self) -> Dict:
        """Serialisierung für API"""
        
    @classmethod
    def from_dict(cls, data: Dict) -> 'Task':
        """Deserialisierung von API"""
```

## Datenfluss

### Task Erstellen

```
1. User klickt "Neue Aufgabe"
   ↓
2. UI öffnet leeres Formular
   ↓
3. User gibt Daten ein und klickt "Speichern"
   ↓
4. UI validiert Eingaben (Client-Side)
   ↓
5. Controller ruft client.create_task() auf
   ↓
6. Client sendet POST /tasks an ThemisDB
   ↓
7. ThemisDB validiert und speichert
   ↓
8. Client erhält Response mit task_id
   ↓
9. Controller aktualisiert UI
   ↓
10. Statusbar zeigt Erfolg/Fehler
```

### Task Aktualisieren

```
1. User wählt Task in Liste aus
   ↓
2. UI lädt Details und zeigt sie an
   ↓
3. User ändert Felder und klickt "Speichern"
   ↓
4. UI validiert Änderungen
   ↓
5. Controller ruft client.update_task() auf
   ↓
6. Client sendet PUT /tasks/{id} an ThemisDB
   ↓
7. ThemisDB aktualisiert mit optimistic locking
   ↓
8. Controller aktualisiert UI mit neuen Daten
```

### Suche und Filterung

```
1. User gibt Suchtext ein ODER wählt Filter
   ↓
2. UI debounced Event-Handler (300ms)
   ↓
3. Controller baut Query-Parameter
   ↓
4. Client ruft list_tasks(filters) auf
   ↓
5. ThemisDB führt indizierte Query aus
   ↓
6. Client deserialisiert Results
   ↓
7. UI aktualisiert Treeview mit gefilterten Tasks
```

## Performance-Überlegungen

### Optimierungen

1. **Lazy Loading**: Nur sichtbare Tasks laden
2. **Caching**: Recent Tasks im Memory
3. **Batch Operations**: Mehrere Updates zusammenfassen
4. **Index Usage**: Alle Filter nutzen Indizes

### Skalierung

- **< 1.000 Tasks**: Keine besonderen Maßnahmen
- **1.000 - 10.000 Tasks**: Pagination implementieren
- **> 10.000 Tasks**: Virtual Scrolling + Server-Side Pagination

## Fehlerbehandlung

### Client-Side Validierung

```python
def validate_task(task: Task) -> List[str]:
    errors = []
    if not task.title or len(task.title) < 3:
        errors.append("Titel muss mindestens 3 Zeichen haben")
    if len(task.title) > 200:
        errors.append("Titel darf maximal 200 Zeichen haben")
    if task.due_date and task.due_date < datetime.now():
        errors.append("Fälligkeitsdatum darf nicht in der Vergangenheit liegen")
    return errors
```

### Server-Side Fehler

```python
try:
    response = client.create_task(task)
except ConnectionError:
    show_error("Keine Verbindung zum Server")
except ValidationError as e:
    show_error(f"Ungültige Daten: {e}")
except Exception as e:
    show_error(f"Unerwarteter Fehler: {e}")
    log_error(e)
```

## Erweiterungsmöglichkeiten

### Geplante Features

1. **Subtasks**: Hierarchische Tasks
   - Graph-Beziehungen zwischen Tasks
   - Parent-Child Relationships

2. **Attachments**: Dateien anhängen
   - Blob Storage Integration
   - Preview-Funktionalität

3. **Collaboration**: Mehrere Benutzer
   - User Model hinzufügen
   - Assignment-Funktionalität
   - Activity Log

4. **Notifications**: Erinnerungen
   - Time-Series für Reminders
   - Push Notifications

5. **Analytics**: Statistiken
   - Dashboard mit Aggregationen
   - Produktivitäts-Metriken

## Testing-Strategie

### Unit Tests

```python
def test_create_task():
    client = TodoClient()
    task = Task(title="Test", status=TaskStatus.OPEN)
    result = client.create_task(task)
    assert result is True
    
def test_invalid_task():
    task = Task(title="", status=TaskStatus.OPEN)
    errors = validate_task(task)
    assert len(errors) > 0
```

### Integration Tests

```python
def test_full_workflow():
    # Create
    task = create_test_task()
    # Read
    loaded = client.get_task(task.id)
    assert loaded.title == task.title
    # Update
    loaded.status = TaskStatus.DONE
    client.update_task(loaded)
    # Verify
    updated = client.get_task(task.id)
    assert updated.status == TaskStatus.DONE
    # Delete
    client.delete_task(task.id)
```

## Deployment

### Produktions-Setup

1. **ThemisDB Server**: Docker Container
2. **Application**: Standalone Python Executable
3. **Configuration**: YAML Config File
4. **Logging**: Rotating File Logs

### Konfiguration

```yaml
# config.yaml
themisdb:
  host: localhost
  port: 8080
  timeout: 10
  
app:
  cache_size: 100
  debounce_delay: 300
  auto_save: true
```

## Zusammenfassung

Die Todo-App ist ein einfaches, aber vollständiges Beispiel für eine CRUD-Anwendung mit ThemisDB. Die Architektur ist klar strukturiert und kann als Template für komplexere Anwendungen dienen.

**Stärken**:
- ✅ Klare Trennung von UI, Logic und Data
- ✅ Type-Safe mit Dataclasses
- ✅ Comprehensive Error Handling
- ✅ Performance durch Indizes
- ✅ Erweiterbar für neue Features

**Nächste Schritte**:
1. Implementierung der Basis-Funktionalität
2. Unit Tests schreiben
3. UI Screenshots erstellen
4. Performance-Testing
5. Dokumentation vervollständigen
