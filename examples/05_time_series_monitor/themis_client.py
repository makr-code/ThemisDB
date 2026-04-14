"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     169                                            ║
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
ThemisDB Client für Zeitreihen-Monitor
Client-Operationen für Sensoren und Messungen
"""

import json
from typing import List, Optional
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
from models import Sensor, Measurement, Alert


# Configuration constants
DEFAULT_TIMEOUT = 10
DEFAULT_RETRY_COUNT = 3


class TimeSeriesClient:
    """
    Client für Time-Series-Operationen in ThemisDB.
    """
    
    def __init__(
        self,
        host: str = "localhost",
        port: int = 8080,
        protocol: str = "http",
        timeout: int = DEFAULT_TIMEOUT
    ):
        """Initialisiert den TimeSeriesClient."""
        self.base_url = f"{protocol}://{host}:{port}"
        self.timeout = timeout
        self.session = self._create_session()
    
    def _create_session(self) -> requests.Session:
        """Erstellt HTTP-Session mit Retry-Logik."""
        session = requests.Session()
        retry_strategy = Retry(
            total=DEFAULT_RETRY_COUNT,
            backoff_factor=0.5,
            status_forcelist=[429, 500, 502, 503, 504],
        )
        adapter = HTTPAdapter(max_retries=retry_strategy)
        session.mount("http://", adapter)
        session.mount("https://", adapter)
        return session
    
    def health_check(self) -> bool:
        """Prüft Verbindung zum Server."""
        try:
            response = self.session.get(
                f"{self.base_url}/health",
                timeout=5
            )
            return response.status_code == 200
        except Exception:
            return False
    
    # Sensor operations
    def create_sensor(self, sensor: Sensor) -> Sensor:
        """Erstellt einen neuen Sensor."""
        entity_key = f"sensors:{sensor.id}"
        sensor_data = sensor.to_dict()
        sensor_data_clean = {k: v for k, v in sensor_data.items() if k != 'id'}
        
        payload = {"blob": json.dumps(sensor_data_clean)}
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return sensor
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to create sensor: {str(e)}")
    
    def get_sensor(self, sensor_id: str) -> Optional[Sensor]:
        """Ruft einen Sensor ab."""
        entity_key = f"sensors:{sensor_id}"
        
        try:
            response = self.session.get(
                f"{self.base_url}/entities/{entity_key}",
                timeout=self.timeout
            )
            
            if response.status_code == 404:
                return None
            
            response.raise_for_status()
            
            data = response.json()
            if "blob" in data:
                sensor_data = json.loads(data["blob"])
                sensor_data["id"] = sensor_id
                return Sensor.from_dict(sensor_data)
            
            return None
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to get sensor: {str(e)}")
    
    # Measurement operations
    def record_measurement(self, measurement: Measurement) -> Measurement:
        """Speichert eine Messung."""
        entity_key = f"measurements:{measurement.id}"
        measurement_data = measurement.to_dict()
        measurement_data_clean = {k: v for k, v in measurement_data.items() if k != 'id'}
        
        payload = {"blob": json.dumps(measurement_data_clean)}
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return measurement
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to record measurement: {str(e)}")
    
    # Alert operations
    def create_alert(self, alert: Alert) -> Alert:
        """Erstellt einen Alarm."""
        entity_key = f"alerts:{alert.id}"
        alert_data = alert.to_dict()
        alert_data_clean = {k: v for k, v in alert_data.items() if k != 'id'}
        
        payload = {"blob": json.dumps(alert_data_clean)}
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return alert
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to create alert: {str(e)}")
