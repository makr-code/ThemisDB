"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     163                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Kontaktmanager Datenmodelle
Datenstrukturen für Kontaktverwaltung
"""

from dataclasses import dataclass, field, asdict
from datetime import datetime
from typing import Optional
from enum import Enum


class ContactCategory(Enum):
    """Kategorie eines Kontakts."""
    FRIENDS = "friends"
    FAMILY = "family"
    WORK = "work"
    OTHER = "other"


@dataclass
class Address:
    """
    Adresse eines Kontakts.
    
    Attributes:
        street: Straße und Hausnummer
        city: Stadt
        postal_code: Postleitzahl
        country: Land
    """
    street: str = ""
    city: str = ""
    postal_code: str = ""
    country: str = ""
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary."""
        return asdict(self)
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Address':
        """Erstellt Address aus Dictionary."""
        return cls(
            street=data.get("street", ""),
            city=data.get("city", ""),
            postal_code=data.get("postal_code", ""),
            country=data.get("country", "")
        )
    
    def __str__(self) -> str:
        """String-Repräsentation."""
        parts = [self.street, f"{self.postal_code} {self.city}", self.country]
        return ", ".join(p for p in parts if p.strip())


@dataclass
class Contact:
    """
    Repräsentiert einen Kontakt.
    
    Attributes:
        id: Eindeutige Kontakt-ID
        first_name: Vorname
        last_name: Nachname
        email: Email-Adresse
        phone: Telefonnummer
        address: Adresse
        category: Kategorie (friends, family, work, other)
        is_favorite: Favorit-Status
        notes: Notizen
        created_at: Erstellungszeitpunkt
        updated_at: Letztes Update
    """
    id: str
    first_name: str
    last_name: str
    email: str = ""
    phone: str = ""
    address: Address = field(default_factory=Address)
    category: ContactCategory = ContactCategory.OTHER
    is_favorite: bool = False
    notes: str = ""
    created_at: str = field(default_factory=lambda: datetime.utcnow().isoformat() + "Z")
    updated_at: str = field(default_factory=lambda: datetime.utcnow().isoformat() + "Z")
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary für JSON-Serialisierung."""
        return {
            "id": self.id,
            "first_name": self.first_name,
            "last_name": self.last_name,
            "email": self.email,
            "phone": self.phone,
            "address": self.address.to_dict(),
            "category": self.category.value,
            "is_favorite": self.is_favorite,
            "notes": self.notes,
            "created_at": self.created_at,
            "updated_at": self.updated_at
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Contact':
        """Erstellt Contact aus Dictionary."""
        return cls(
            id=data.get("id", ""),
            first_name=data.get("first_name", ""),
            last_name=data.get("last_name", ""),
            email=data.get("email", ""),
            phone=data.get("phone", ""),
            address=Address.from_dict(data.get("address", {})),
            category=ContactCategory(data.get("category", "other")),
            is_favorite=data.get("is_favorite", False),
            notes=data.get("notes", ""),
            created_at=data.get("created_at", ""),
            updated_at=data.get("updated_at", "")
        )
    
    @property
    def full_name(self) -> str:
        """Vollständiger Name."""
        return f"{self.first_name} {self.last_name}".strip()
    
    @property
    def display_name(self) -> str:
        """Anzeige-Name mit Favorit-Icon."""
        star = "⭐ " if self.is_favorite else ""
        return f"{star}{self.full_name}"
    
    def matches_search(self, query: str) -> bool:
        """Prüft ob Kontakt zur Suchanfrage passt."""
        query = query.lower()
        return (
            query in self.first_name.lower() or
            query in self.last_name.lower() or
            query in self.email.lower() or
            query in self.phone.lower() or
            query in self.address.city.lower() or
            query in self.notes.lower()
        )
