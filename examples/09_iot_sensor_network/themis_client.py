"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     344                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Client für IoT Sensor Network Beispiel.

Dieser Client bietet Funktionen für:
- IoT-Geräte Verwaltung
- Sensor-Readings (Zeit-Reihen)
- CEP-Regeln
- Alert-Management
"""

import requests
from typing import List, Optional, Dict, Any
from datetime import datetime

from models import (
    Device, SensorReading, CEPRule, Alert, Anomaly,
    DeviceType, DeviceStatus, AlertSeverity
)


class IoTClient:
    """Client für IoT Sensor Network Operationen."""
    
    def __init__(self, host: str = "localhost", port: int = 8080, timeout: int = 10):
        """
        Initialisiert den IoT Client.
        
        Args:
            host: ThemisDB Server Hostname
            port: ThemisDB Server Port
            timeout: Request Timeout in Sekunden
        """
        self.base_url = f"http://{host}:{port}/api"
        self.timeout = timeout
        self.session = requests.Session()
    
    def test_connection(self) -> bool:
        """Testet die Verbindung zu ThemisDB."""
        try:
            response = self.session.get(
                f"{self.base_url}/health",
                timeout=self.timeout
            )
            return response.status_code == 200
        except:
            return False
    
    # ===== Device Operations =====
    
    def create_device(self, device: Device) -> bool:
        """Erstellt ein neues IoT-Gerät."""
        try:
            response = self.session.post(
                f"{self.base_url}/devices",
                json=device.to_dict(),
                timeout=self.timeout
            )
            return response.status_code in [200, 201]
        except Exception as e:
            print(f"Fehler beim Erstellen des Geräts: {e}")
            return False
    
    def get_device(self, device_id: str) -> Optional[Device]:
        """Holt ein IoT-Gerät."""
        try:
            response = self.session.get(
                f"{self.base_url}/devices/{device_id}",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return Device.from_dict(response.json())
            return None
        except Exception as e:
            print(f"Fehler beim Abrufen des Geräts: {e}")
            return None
    
    def list_devices(self) -> List[Device]:
        """Listet alle IoT-Geräte."""
        try:
            response = self.session.get(
                f"{self.base_url}/devices",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [Device.from_dict(d) for d in response.json()]
            return []
        except Exception as e:
            print(f"Fehler beim Auflisten der Geräte: {e}")
            return []
    
    def update_device(self, device: Device) -> bool:
        """Aktualisiert ein IoT-Gerät."""
        try:
            response = self.session.put(
                f"{self.base_url}/devices/{device.device_id}",
                json=device.to_dict(),
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"Fehler beim Aktualisieren des Geräts: {e}")
            return False
    
    def delete_device(self, device_id: str) -> bool:
        """Löscht ein IoT-Gerät."""
        try:
            response = self.session.delete(
                f"{self.base_url}/devices/{device_id}",
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"Fehler beim Löschen des Geräts: {e}")
            return False
    
    # ===== Sensor Reading Operations =====
    
    def create_reading(self, reading: SensorReading) -> bool:
        """Erstellt einen Sensor-Messwert."""
        try:
            response = self.session.post(
                f"{self.base_url}/readings",
                json=reading.to_dict(),
                timeout=self.timeout
            )
            return response.status_code in [200, 201]
        except Exception as e:
            print(f"Fehler beim Erstellen des Messwerts: {e}")
            return False
    
    def get_readings_by_device(self, device_id: str, limit: int = 100) -> List[SensorReading]:
        """Holt Messwerte für ein Gerät."""
        try:
            response = self.session.get(
                f"{self.base_url}/readings/device/{device_id}",
                params={"limit": limit},
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [SensorReading.from_dict(r) for r in response.json()]
            return []
        except Exception as e:
            print(f"Fehler beim Abrufen der Messwerte: {e}")
            return []
    
    def list_readings(self, limit: int = 1000) -> List[SensorReading]:
        """Listet alle Messwerte."""
        try:
            response = self.session.get(
                f"{self.base_url}/readings",
                params={"limit": limit},
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [SensorReading.from_dict(r) for r in response.json()]
            return []
        except Exception as e:
            print(f"Fehler beim Auflisten der Messwerte: {e}")
            return []
    
    # ===== CEP Rule Operations =====
    
    def create_rule(self, rule: CEPRule) -> bool:
        """Erstellt eine CEP-Regel."""
        try:
            response = self.session.post(
                f"{self.base_url}/rules",
                json=rule.to_dict(),
                timeout=self.timeout
            )
            return response.status_code in [200, 201]
        except Exception as e:
            print(f"Fehler beim Erstellen der Regel: {e}")
            return False
    
    def get_rule(self, rule_id: str) -> Optional[CEPRule]:
        """Holt eine CEP-Regel."""
        try:
            response = self.session.get(
                f"{self.base_url}/rules/{rule_id}",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return CEPRule.from_dict(response.json())
            return None
        except Exception as e:
            print(f"Fehler beim Abrufen der Regel: {e}")
            return None
    
    def list_rules(self) -> List[CEPRule]:
        """Listet alle CEP-Regeln."""
        try:
            response = self.session.get(
                f"{self.base_url}/rules",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [CEPRule.from_dict(r) for r in response.json()]
            return []
        except Exception as e:
            print(f"Fehler beim Auflisten der Regeln: {e}")
            return []
    
    def update_rule(self, rule: CEPRule) -> bool:
        """Aktualisiert eine CEP-Regel."""
        try:
            response = self.session.put(
                f"{self.base_url}/rules/{rule.rule_id}",
                json=rule.to_dict(),
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"Fehler beim Aktualisieren der Regel: {e}")
            return False
    
    def delete_rule(self, rule_id: str) -> bool:
        """Löscht eine CEP-Regel."""
        try:
            response = self.session.delete(
                f"{self.base_url}/rules/{rule_id}",
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"Fehler beim Löschen der Regel: {e}")
            return False
    
    # ===== Alert Operations =====
    
    def create_alert(self, alert: Alert) -> bool:
        """Erstellt einen Alert."""
        try:
            response = self.session.post(
                f"{self.base_url}/alerts",
                json=alert.to_dict(),
                timeout=self.timeout
            )
            return response.status_code in [200, 201]
        except Exception as e:
            print(f"Fehler beim Erstellen des Alerts: {e}")
            return False
    
    def get_alert(self, alert_id: str) -> Optional[Alert]:
        """Holt einen Alert."""
        try:
            response = self.session.get(
                f"{self.base_url}/alerts/{alert_id}",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return Alert.from_dict(response.json())
            return None
        except Exception as e:
            print(f"Fehler beim Abrufen des Alerts: {e}")
            return None
    
    def list_alerts(self, acknowledged: Optional[bool] = None) -> List[Alert]:
        """Listet Alerts."""
        try:
            params = {}
            if acknowledged is not None:
                params["acknowledged"] = acknowledged
            
            response = self.session.get(
                f"{self.base_url}/alerts",
                params=params,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [Alert.from_dict(a) for a in response.json()]
            return []
        except Exception as e:
            print(f"Fehler beim Auflisten der Alerts: {e}")
            return []
    
    def acknowledge_alert(self, alert_id: str, user: str = "System") -> bool:
        """Bestätigt einen Alert."""
        try:
            response = self.session.post(
                f"{self.base_url}/alerts/{alert_id}/acknowledge",
                json={"user": user},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"Fehler beim Bestätigen des Alerts: {e}")
            return False
    
    # ===== Anomaly Operations =====
    
    def create_anomaly(self, anomaly: Anomaly) -> bool:
        """Erstellt eine Anomalie."""
        try:
            response = self.session.post(
                f"{self.base_url}/anomalies",
                json=anomaly.to_dict(),
                timeout=self.timeout
            )
            return response.status_code in [200, 201]
        except Exception as e:
            print(f"Fehler beim Erstellen der Anomalie: {e}")
            return False
    
    def list_anomalies(self, device_id: Optional[str] = None) -> List[Anomaly]:
        """Listet Anomalien."""
        try:
            params = {}
            if device_id:
                params["device_id"] = device_id
            
            response = self.session.get(
                f"{self.base_url}/anomalies",
                params=params,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [Anomaly.from_dict(a) for a in response.json()]
            return []
        except Exception as e:
            print(f"Fehler beim Auflisten der Anomalien: {e}")
            return []
