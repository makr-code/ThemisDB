> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Kanban Board / Project Management - Agile Projektmanagement mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-medium-yellow)
![Duration](https://img.shields.io/badge/duration-60%20min-blue)

## 📝 Übersicht

Das Kanban Board demonstriert Agile Projektmanagement-Features. Sie lernen:
- Task-Management mit Kanban-Boards
- Sprint-Planung und -Durchführung
- Team-Collaboration
- Burndown-Charts
- Time-Tracking
- Task-Dependencies mit Graph

## ✨ Features

- ✅ **Kanban-Board** - Todo → In Progress → Done
- ✅ **Sprint-Management** - Planung und Tracking
- ✅ **Tasks** - Mit Prioritäten und Assignees
- ✅ **Dependencies** - Task-Abhängigkeiten
- ✅ **Time-Tracking** - Zeiterfassung
- ✅ **Burndown-Charts** - Sprint-Fortschritt
- ✅ **Comments** - Team-Kommunikation
- ✅ **Attachments** - Dateien anhängen

## 📊 Datenmodell

### Task

```json
{
  "id": "task_uuid",
  "title": "Implement User Authentication",
  "description": "Add JWT-based auth",
  "status": "in_progress",
  "priority": "high",
  "assignee": "john.doe",
  "sprint_id": "sprint_1",
  "story_points": 5,
  "estimated_hours": 8,
  "actual_hours": 4,
  "created_at": "2025-12-15T10:00:00Z",
  "due_date": "2025-12-22",
  "tags": ["backend", "security"]
}
```

## 🛠️ ThemisDB Features

- **Relational** für Tasks und Sprints
- **Time-Series** für Time-Tracking
- **Graph** für Task-Dependencies

## 🔗 Navigation

- ⬅️ [15 - Event Management](../15_event_management/)
- ➡️ [17 - CRM](../17_crm/)
- 🏠 [Übersicht](../README.md)

---

**Status**: Ready | **Letzte Aktualisierung**: 2025-12-22
