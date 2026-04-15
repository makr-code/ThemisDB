"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     259                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Inventarsystem Datenmodelle
Datenstrukturen für Lagerverwaltung
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional, List
from enum import Enum


class MovementType(Enum):
    """Art der Bestandsbewegung."""
    IN = "in"          # Wareneingang
    OUT = "out"        # Warenausgang
    ADJUSTMENT = "adjustment"  # Inventurkorrektur


@dataclass
class Product:
    """
    Repräsentiert ein Produkt im Lager.
    
    Attributes:
        id: Eindeutige Produkt-ID
        sku: Stock Keeping Unit (Artikelnummer)
        name: Produktname
        description: Beschreibung
        price: Preis pro Einheit
        quantity: Aktuelle Menge
        min_quantity: Mindestbestand
        location: Lagerort
        category: Kategorie
        created_at: Erstellungszeitpunkt
    """
    id: str
    sku: str
    name: str
    description: str = ""
    price: float = 0.0
    quantity: int = 0
    min_quantity: int = 10
    location: str = ""
    category: str = ""
    created_at: str = field(default_factory=lambda: datetime.utcnow().isoformat() + "Z")
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary."""
        return {
            "id": self.id,
            "sku": self.sku,
            "name": self.name,
            "description": self.description,
            "price": self.price,
            "quantity": self.quantity,
            "min_quantity": self.min_quantity,
            "location": self.location,
            "category": self.category,
            "created_at": self.created_at
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Product':
        """Erstellt Product aus Dictionary."""
        return cls(
            id=data.get("id", ""),
            sku=data.get("sku", ""),
            name=data.get("name", ""),
            description=data.get("description", ""),
            price=float(data.get("price", 0.0)),
            quantity=int(data.get("quantity", 0)),
            min_quantity=int(data.get("min_quantity", 10)),
            location=data.get("location", ""),
            category=data.get("category", ""),
            created_at=data.get("created_at", "")
        )
    
    @property
    def stock_status(self) -> str:
        """Bestandsstatus."""
        if self.quantity == 0:
            return "Ausverkauft"
        elif self.quantity < self.min_quantity * 0.25:
            return "Kritisch"
        elif self.quantity < self.min_quantity * 0.5:
            return "Niedrig"
        elif self.quantity < self.min_quantity:
            return "Warnung"
        return "OK"
    
    @property
    def stock_status_color(self) -> str:
        """Farbe für Bestandsstatus."""
        status = self.stock_status
        if status == "Ausverkauft":
            return "#c0392b"
        elif status == "Kritisch":
            return "#e74c3c"
        elif status == "Niedrig":
            return "#e67e22"
        elif status == "Warnung":
            return "#f39c12"
        return "#27ae60"
    
    @property
    def total_value(self) -> float:
        """Gesamtwert des Bestands."""
        return self.quantity * self.price


@dataclass
class StockMovement:
    """
    Repräsentiert eine Bestandsbewegung.
    
    Attributes:
        id: Eindeutige Bewegungs-ID
        product_id: Produkt-ID
        type: Art der Bewegung (in, out, adjustment)
        quantity: Menge (positiv für IN, negativ für OUT)
        reason: Grund/Beschreibung
        timestamp: Zeitpunkt
        user: Benutzer
    """
    id: str
    product_id: str
    type: MovementType
    quantity: int
    reason: str = ""
    timestamp: str = field(default_factory=lambda: datetime.utcnow().isoformat() + "Z")
    user: str = "admin"
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary."""
        return {
            "id": self.id,
            "product_id": self.product_id,
            "type": self.type.value,
            "quantity": self.quantity,
            "reason": self.reason,
            "timestamp": self.timestamp,
            "user": self.user
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'StockMovement':
        """Erstellt StockMovement aus Dictionary."""
        return cls(
            id=data.get("id", ""),
            product_id=data.get("product_id", ""),
            type=MovementType(data.get("type", "in")),
            quantity=int(data.get("quantity", 0)),
            reason=data.get("reason", ""),
            timestamp=data.get("timestamp", ""),
            user=data.get("user", "admin")
        )


@dataclass
class Supplier:
    """
    Repräsentiert einen Lieferanten.
    
    Attributes:
        id: Eindeutige Lieferanten-ID
        name: Name des Lieferanten
        contact: Kontaktperson
        email: Email
        phone: Telefon
        lead_time_days: Lieferzeit in Tagen
    """
    id: str
    name: str
    contact: str = ""
    email: str = ""
    phone: str = ""
    lead_time_days: int = 7
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary."""
        return {
            "id": self.id,
            "name": self.name,
            "contact": self.contact,
            "email": self.email,
            "phone": self.phone,
            "lead_time_days": self.lead_time_days
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Supplier':
        """Erstellt Supplier aus Dictionary."""
        return cls(
            id=data.get("id", ""),
            name=data.get("name", ""),
            contact=data.get("contact", ""),
            email=data.get("email", ""),
            phone=data.get("phone", ""),
            lead_time_days=int(data.get("lead_time_days", 7))
        )


@dataclass
class ProductSupplier:
    """
    Repräsentiert die Beziehung zwischen Produkt und Lieferant.
    
    Attributes:
        product_id: Produkt-ID
        supplier_id: Lieferanten-ID
        unit_price: Einkaufspreis
        min_order_quantity: Mindestbestellmenge
    """
    product_id: str
    supplier_id: str
    unit_price: float = 0.0
    min_order_quantity: int = 1
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary."""
        return {
            "product_id": self.product_id,
            "supplier_id": self.supplier_id,
            "unit_price": self.unit_price,
            "min_order_quantity": self.min_order_quantity
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'ProductSupplier':
        """Erstellt ProductSupplier aus Dictionary."""
        return cls(
            product_id=data.get("product_id", ""),
            supplier_id=data.get("supplier_id", ""),
            unit_price=float(data.get("unit_price", 0.0)),
            min_order_quantity=int(data.get("min_order_quantity", 1))
        )
