"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     222                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Zeitreihen-Monitor Datenmodelle
Datenstrukturen für Sensor-Daten und Messungen
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional, Dict
from enum import Enum
import random


class SensorType(Enum):
    """Typ eines Sensors."""
    CPU = "cpu"
    MEMORY = "memory"
    TEMPERATURE = "temperature"
    CUSTOM = "custom"


@dataclass
class Measurement:
    """
    Repräsentiert eine Sensor-Messung.
    
    Attributes:
        id: Eindeutige Messungs-ID
        sensor_id: Sensor-ID
        sensor_name: Name des Sensors
        value: Messwert
        unit: Einheit (%, °C, MB, etc.)
        timestamp: Zeitstempel
        tags: Zusätzliche Metadaten
    """
    id: str
    sensor_id: str
    sensor_name: str
    value: float
    unit: str
    timestamp: str = field(default_factory=lambda: datetime.utcnow().isoformat() + "Z")
    tags: Dict[str, str] = field(default_factory=dict)
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary."""
        return {
            "id": self.id,
            "sensor_id": self.sensor_id,
            "sensor_name": self.sensor_name,
            "value": self.value,
            "unit": self.unit,
            "timestamp": self.timestamp,
            "tags": self.tags
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Measurement':
        """Erstellt Measurement aus Dictionary."""
        return cls(
            id=data.get("id", ""),
            sensor_id=data.get("sensor_id", ""),
            sensor_name=data.get("sensor_name", ""),
            value=float(data.get("value", 0.0)),
            unit=data.get("unit", ""),
            timestamp=data.get("timestamp", ""),
            tags=data.get("tags", {})
        )


@dataclass
class Sensor:
    """
    Repräsentiert einen Sensor.
    
    Attributes:
        id: Eindeutige Sensor-ID
        name: Name des Sensors
        type: Typ des Sensors
        unit: Maßeinheit
        min_value: Minimaler erwarteter Wert
        max_value: Maximaler erwarteter Wert
        warning_threshold: Warnschwelle
        critical_threshold: Kritische Schwelle
        active: Ob der Sensor aktiv ist
    """
    id: str
    name: str
    type: SensorType
    unit: str
    min_value: float = 0.0
    max_value: float = 100.0
    warning_threshold: Optional[float] = None
    critical_threshold: Optional[float] = None
    active: bool = True
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary."""
        return {
            "id": self.id,
            "name": self.name,
            "type": self.type.value,
            "unit": self.unit,
            "min_value": self.min_value,
            "max_value": self.max_value,
            "warning_threshold": self.warning_threshold,
            "critical_threshold": self.critical_threshold,
            "active": self.active
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Sensor':
        """Erstellt Sensor aus Dictionary."""
        return cls(
            id=data.get("id", ""),
            name=data.get("name", ""),
            type=SensorType(data.get("type", "custom")),
            unit=data.get("unit", ""),
            min_value=float(data.get("min_value", 0.0)),
            max_value=float(data.get("max_value", 100.0)),
            warning_threshold=data.get("warning_threshold"),
            critical_threshold=data.get("critical_threshold"),
            active=data.get("active", True)
        )
    
    def simulate_value(self, last_value: Optional[float] = None) -> float:
        """
        Simuliert einen realistischen Sensor-Wert.
        
        Args:
            last_value: Letzter Wert für sanfte Übergänge
            
        Returns:
            Simulierter Wert
        """
        if last_value is None:
            # Startwert: Mittelwert
            return (self.min_value + self.max_value) / 2
        
        # Kleine zufällige Änderung
        range_size = self.max_value - self.min_value
        change = random.uniform(-range_size * 0.05, range_size * 0.05)
        new_value = last_value + change
        
        # Begrenzung auf min/max
        new_value = max(self.min_value, min(self.max_value, new_value))
        
        return new_value
    
    def check_threshold(self, value: float) -> str:
        """
        Prüft ob Schwellwerte überschritten sind.
        
        Args:
            value: Zu prüfender Wert
            
        Returns:
            Status: "ok", "warning", "critical"
        """
        if self.critical_threshold is not None and value >= self.critical_threshold:
            return "critical"
        elif self.warning_threshold is not None and value >= self.warning_threshold:
            return "warning"
        return "ok"


@dataclass
class Alert:
    """
    Repräsentiert einen Alarm.
    
    Attributes:
        id: Eindeutige Alarm-ID
        sensor_id: Sensor-ID
        level: Level (warning, critical)
        message: Alarm-Nachricht
        value: Wert der den Alarm ausgelöst hat
        threshold: Überschrittener Schwellwert
        timestamp: Zeitpunkt
        acknowledged: Ob der Alarm bestätigt wurde
    """
    id: str
    sensor_id: str
    level: str  # "warning" or "critical"
    message: str
    value: float
    threshold: float
    timestamp: str = field(default_factory=lambda: datetime.utcnow().isoformat() + "Z")
    acknowledged: bool = False
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary."""
        return {
            "id": self.id,
            "sensor_id": self.sensor_id,
            "level": self.level,
            "message": self.message,
            "value": self.value,
            "threshold": self.threshold,
            "timestamp": self.timestamp,
            "acknowledged": self.acknowledged
        }
