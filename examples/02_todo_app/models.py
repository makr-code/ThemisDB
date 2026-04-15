"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     111                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Todo App Datenmodelle
Datenstrukturen für die Todo-Verwaltung
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional
from enum import Enum


class TaskStatus(Enum):
    """Status einer Aufgabe."""
    OPEN = "open"
    IN_PROGRESS = "in_progress"
    DONE = "done"


class TaskPriority(Enum):
    """Priorität einer Aufgabe."""
    LOW = "low"
    NORMAL = "normal"
    HIGH = "high"


@dataclass
class Task:
    """
    Repräsentiert eine Todo-Aufgabe.
    
    Attributes:
        id: Eindeutige Task-ID
        title: Titel der Aufgabe
        description: Beschreibung der Aufgabe
        status: Status (open, in_progress, done)
        priority: Priorität (low, normal, high)
        created_at: Erstellungszeitpunkt
        updated_at: Letztes Update
        due_date: Fälligkeitsdatum (optional)
    """
    id: str
    title: str
    description: str = ""
    status: TaskStatus = TaskStatus.OPEN
    priority: TaskPriority = TaskPriority.NORMAL
    created_at: str = field(default_factory=lambda: datetime.utcnow().isoformat() + "Z")
    updated_at: str = field(default_factory=lambda: datetime.utcnow().isoformat() + "Z")
    due_date: Optional[str] = None
    
    def to_dict(self) -> dict:
        """Konvertiert Task zu Dictionary für JSON-Serialisierung."""
        return {
            "id": self.id,
            "title": self.title,
            "description": self.description,
            "status": self.status.value,
            "priority": self.priority.value,
            "created_at": self.created_at,
            "updated_at": self.updated_at,
            "due_date": self.due_date
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Task':
        """Erstellt Task aus Dictionary."""
        return cls(
            id=data.get("id", ""),
            title=data.get("title", ""),
            description=data.get("description", ""),
            status=TaskStatus(data.get("status", "open")),
            priority=TaskPriority(data.get("priority", "normal")),
            created_at=data.get("created_at", ""),
            updated_at=data.get("updated_at", ""),
            due_date=data.get("due_date")
        )
    
    def __str__(self) -> str:
        """String-Repräsentation für UI."""
        status_icon = {
            TaskStatus.OPEN: "○",
            TaskStatus.IN_PROGRESS: "◐",
            TaskStatus.DONE: "●"
        }
        priority_icon = {
            TaskPriority.LOW: "↓",
            TaskPriority.NORMAL: "→",
            TaskPriority.HIGH: "↑"
        }
        return f"{status_icon[self.status]} {priority_icon[self.priority]} {self.title}"
