"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     432                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Datenmodelle für IoT Sensor Network Beispiel.

Dieses Modul definiert die Datenstrukturen für:
- IoT-Geräte (Devices)
- Sensor-Readings (Zeit-Reihen Daten)
- CEP-Regeln (Complex Event Processing)
- Alerts und Anomalien
"""

from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from typing import List, Dict, Any, Optional
import random
import math


class DeviceType(Enum):
    """Gerätetyp für IoT-Sensoren."""
    TEMPERATURE = "temperature"
    HUMIDITY = "humidity"
    PRESSURE = "pressure"
    MOTION = "motion"
    LIGHT = "light"
    ENERGY = "energy"
    CUSTOM = "custom"


class DeviceStatus(Enum):
    """Status eines IoT-Geräts."""
    ACTIVE = "active"
    INACTIVE = "inactive"
    ERROR = "error"
    MAINTENANCE = "maintenance"


class AlertSeverity(Enum):
    """Schweregrad eines Alerts."""
    INFO = "info"
    WARNING = "warning"
    CRITICAL = "critical"


class RuleOperator(Enum):
    """Operatoren für CEP-Regeln."""
    GREATER_THAN = ">"
    LESS_THAN = "<"
    GREATER_EQUAL = ">="
    LESS_EQUAL = "<="
    EQUAL = "=="
    NOT_EQUAL = "!="


@dataclass
class Device:
    """IoT-Gerät mit Metadaten."""
    device_id: str
    name: str
    device_type: DeviceType
    location: str
    status: DeviceStatus = DeviceStatus.ACTIVE
    firmware_version: str = "1.0.0"
    battery_level: float = 100.0  # Prozent
    last_seen: Optional[datetime] = None
    
    def to_dict(self) -> Dict[str, Any]:
        """Konvertiert Device zu Dictionary."""
        return {
            "device_id": self.device_id,
            "name": self.name,
            "device_type": self.device_type.value,
            "location": self.location,
            "status": self.status.value,
            "firmware_version": self.firmware_version,
            "battery_level": self.battery_level,
            "last_seen": self.last_seen.isoformat() if self.last_seen else None
        }
    
    @staticmethod
    def from_dict(data: Dict[str, Any]) -> 'Device':
        """Erstellt Device aus Dictionary."""
        return Device(
            device_id=data["device_id"],
            name=data["name"],
            device_type=DeviceType(data["device_type"]),
            location=data["location"],
            status=DeviceStatus(data["status"]),
            firmware_version=data.get("firmware_version", "1.0.0"),
            battery_level=data.get("battery_level", 100.0),
            last_seen=datetime.fromisoformat(data["last_seen"]) if data.get("last_seen") else None
        )


@dataclass
class SensorReading:
    """Sensor-Messwert."""
    reading_id: str
    device_id: str
    timestamp: datetime
    value: float
    unit: str
    
    def to_dict(self) -> Dict[str, Any]:
        """Konvertiert SensorReading zu Dictionary."""
        return {
            "reading_id": self.reading_id,
            "device_id": self.device_id,
            "timestamp": self.timestamp.isoformat(),
            "value": self.value,
            "unit": self.unit
        }
    
    @staticmethod
    def from_dict(data: Dict[str, Any]) -> 'SensorReading':
        """Erstellt SensorReading aus Dictionary."""
        return SensorReading(
            reading_id=data["reading_id"],
            device_id=data["device_id"],
            timestamp=datetime.fromisoformat(data["timestamp"]),
            value=data["value"],
            unit=data["unit"]
        )


@dataclass
class CEPRule:
    """Complex Event Processing Regel."""
    rule_id: str
    name: str
    device_id: str
    operator: RuleOperator
    threshold: float
    severity: AlertSeverity
    enabled: bool = True
    description: str = ""
    
    def to_dict(self) -> Dict[str, Any]:
        """Konvertiert CEPRule zu Dictionary."""
        return {
            "rule_id": self.rule_id,
            "name": self.name,
            "device_id": self.device_id,
            "operator": self.operator.value,
            "threshold": self.threshold,
            "severity": self.severity.value,
            "enabled": self.enabled,
            "description": self.description
        }
    
    @staticmethod
    def from_dict(data: Dict[str, Any]) -> 'CEPRule':
        """Erstellt CEPRule aus Dictionary."""
        return CEPRule(
            rule_id=data["rule_id"],
            name=data["name"],
            device_id=data["device_id"],
            operator=RuleOperator(data["operator"]),
            threshold=data["threshold"],
            severity=AlertSeverity(data["severity"]),
            enabled=data.get("enabled", True),
            description=data.get("description", "")
        )
    
    def evaluate(self, value: float) -> bool:
        """Evaluiert die Regel gegen einen Wert."""
        if not self.enabled:
            return False
        
        if self.operator == RuleOperator.GREATER_THAN:
            return value > self.threshold
        elif self.operator == RuleOperator.LESS_THAN:
            return value < self.threshold
        elif self.operator == RuleOperator.GREATER_EQUAL:
            return value >= self.threshold
        elif self.operator == RuleOperator.LESS_EQUAL:
            return value <= self.threshold
        elif self.operator == RuleOperator.EQUAL:
            return abs(value - self.threshold) < 0.0001
        elif self.operator == RuleOperator.NOT_EQUAL:
            return abs(value - self.threshold) >= 0.0001
        return False


@dataclass
class Alert:
    """Alert/Ereignis."""
    alert_id: str
    rule_id: str
    device_id: str
    timestamp: datetime
    severity: AlertSeverity
    message: str
    value: float
    acknowledged: bool = False
    acknowledged_at: Optional[datetime] = None
    acknowledged_by: Optional[str] = None
    
    def to_dict(self) -> Dict[str, Any]:
        """Konvertiert Alert zu Dictionary."""
        return {
            "alert_id": self.alert_id,
            "rule_id": self.rule_id,
            "device_id": self.device_id,
            "timestamp": self.timestamp.isoformat(),
            "severity": self.severity.value,
            "message": self.message,
            "value": self.value,
            "acknowledged": self.acknowledged,
            "acknowledged_at": self.acknowledged_at.isoformat() if self.acknowledged_at else None,
            "acknowledged_by": self.acknowledged_by
        }
    
    @staticmethod
    def from_dict(data: Dict[str, Any]) -> 'Alert':
        """Erstellt Alert aus Dictionary."""
        return Alert(
            alert_id=data["alert_id"],
            rule_id=data["rule_id"],
            device_id=data["device_id"],
            timestamp=datetime.fromisoformat(data["timestamp"]),
            severity=AlertSeverity(data["severity"]),
            message=data["message"],
            value=data["value"],
            acknowledged=data.get("acknowledged", False),
            acknowledged_at=datetime.fromisoformat(data["acknowledged_at"]) if data.get("acknowledged_at") else None,
            acknowledged_by=data.get("acknowledged_by")
        )


@dataclass
class Anomaly:
    """Anomalie-Erkennung."""
    anomaly_id: str
    device_id: str
    timestamp: datetime
    value: float
    expected_value: float
    deviation: float
    z_score: float
    
    def to_dict(self) -> Dict[str, Any]:
        """Konvertiert Anomaly zu Dictionary."""
        return {
            "anomaly_id": self.anomaly_id,
            "device_id": self.device_id,
            "timestamp": self.timestamp.isoformat(),
            "value": self.value,
            "expected_value": self.expected_value,
            "deviation": self.deviation,
            "z_score": self.z_score
        }
    
    @staticmethod
    def from_dict(data: Dict[str, Any]) -> 'Anomaly':
        """Erstellt Anomaly aus Dictionary."""
        return Anomaly(
            anomaly_id=data["anomaly_id"],
            device_id=data["device_id"],
            timestamp=datetime.fromisoformat(data["timestamp"]),
            value=data["value"],
            expected_value=data["expected_value"],
            deviation=data["deviation"],
            z_score=data["z_score"]
        )


class SensorSimulator:
    """Simuliert Sensor-Werte mit realistischem Rauschen."""
    
    def __init__(self, device_type: DeviceType):
        self.device_type = device_type
        self.base_value = self._get_base_value()
        self.noise_factor = self._get_noise_factor()
        self.time_offset = random.uniform(0, 2 * math.pi)
    
    def _get_base_value(self) -> float:
        """Basis-Wert je nach Sensor-Typ."""
        base_values = {
            DeviceType.TEMPERATURE: 22.0,
            DeviceType.HUMIDITY: 50.0,
            DeviceType.PRESSURE: 1013.25,
            DeviceType.MOTION: 0.0,
            DeviceType.LIGHT: 300.0,
            DeviceType.ENERGY: 120.0,
            DeviceType.CUSTOM: 50.0
        }
        return base_values.get(self.device_type, 0.0)
    
    def _get_noise_factor(self) -> float:
        """Rausch-Faktor je nach Sensor-Typ."""
        noise_factors = {
            DeviceType.TEMPERATURE: 2.0,
            DeviceType.HUMIDITY: 5.0,
            DeviceType.PRESSURE: 2.0,
            DeviceType.MOTION: 1.0,
            DeviceType.LIGHT: 50.0,
            DeviceType.ENERGY: 10.0,
            DeviceType.CUSTOM: 5.0
        }
        return noise_factors.get(self.device_type, 1.0)
    
    def get_value(self, time_step: float = 0) -> float:
        """Generiert simulierten Sensor-Wert."""
        # Sinuskurve für zeitliche Variation
        sine_component = math.sin(time_step * 0.1 + self.time_offset) * self.noise_factor
        
        # Zufälliges Rauschen
        noise = random.gauss(0, self.noise_factor * 0.3)
        
        value = self.base_value + sine_component + noise
        
        # Sicherstellen, dass Wert nicht negativ wird
        return max(0, value)
    
    def get_unit(self) -> str:
        """Gibt die Einheit des Sensors zurück."""
        units = {
            DeviceType.TEMPERATURE: "°C",
            DeviceType.HUMIDITY: "%",
            DeviceType.PRESSURE: "hPa",
            DeviceType.MOTION: "events/min",
            DeviceType.LIGHT: "lux",
            DeviceType.ENERGY: "W",
            DeviceType.CUSTOM: "units"
        }
        return units.get(self.device_type, "")


class AnomalyDetector:
    """Anomalie-Erkennung mit Z-Score Methode."""
    
    def __init__(self, threshold: float = 3.0):
        self.threshold = threshold
        self.history: Dict[str, List[float]] = {}
    
    def add_reading(self, device_id: str, value: float):
        """Fügt einen Messwert zur Historie hinzu."""
        if device_id not in self.history:
            self.history[device_id] = []
        self.history[device_id].append(value)
        # Behalte nur die letzten 100 Werte
        if len(self.history[device_id]) > 100:
            self.history[device_id].pop(0)
    
    def detect_anomaly(self, device_id: str, value: float) -> Optional[Anomaly]:
        """Erkennt Anomalien basierend auf Z-Score."""
        if device_id not in self.history or len(self.history[device_id]) < 10:
            return None
        
        values = self.history[device_id]
        mean = sum(values) / len(values)
        variance = sum((x - mean) ** 2 for x in values) / len(values)
        std_dev = math.sqrt(variance) if variance > 0 else 1.0
        
        z_score = abs((value - mean) / std_dev) if std_dev > 0 else 0
        
        if z_score > self.threshold:
            import uuid
            return Anomaly(
                anomaly_id=str(uuid.uuid4()),
                device_id=device_id,
                timestamp=datetime.now(),
                value=value,
                expected_value=mean,
                deviation=abs(value - mean),
                z_score=z_score
            )
        return None


class CEPEngine:
    """Complex Event Processing Engine."""
    
    def __init__(self):
        self.rules: Dict[str, List[CEPRule]] = {}
    
    def add_rule(self, rule: CEPRule):
        """Fügt eine Regel hinzu."""
        if rule.device_id not in self.rules:
            self.rules[rule.device_id] = []
        self.rules[rule.device_id].append(rule)
    
    def remove_rule(self, rule_id: str):
        """Entfernt eine Regel."""
        for device_id in self.rules:
            self.rules[device_id] = [r for r in self.rules[device_id] if r.rule_id != rule_id]
    
    def process_reading(self, reading: SensorReading) -> List[Alert]:
        """Verarbeitet einen Messwert und generiert Alerts."""
        alerts = []
        
        if reading.device_id not in self.rules:
            return alerts
        
        for rule in self.rules[reading.device_id]:
            if rule.evaluate(reading.value):
                import uuid
                alert = Alert(
                    alert_id=str(uuid.uuid4()),
                    rule_id=rule.rule_id,
                    device_id=reading.device_id,
                    timestamp=reading.timestamp,
                    severity=rule.severity,
                    message=f"{rule.name}: Wert {reading.value:.2f} {reading.unit} triggert Regel (Schwellwert: {rule.threshold})",
                    value=reading.value
                )
                alerts.append(alert)
        
        return alerts
