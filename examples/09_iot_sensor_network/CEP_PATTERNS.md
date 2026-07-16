> **Hinweis:** Inhalt mit aktuellem Modulcode und -stand abgleichen.

# CEP Patterns - Complex Event Processing

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## 📋 Übersicht

Complex Event Processing (CEP) ermöglicht die Erkennung von Mustern und Korrelationen in Echtzeit-Datenströmen von IoT-Sensoren.

## 🔍 Grundkonzepte

### Event Types

```python
from dataclasses import dataclass
from datetime import datetime
from typing import Any, Dict

@dataclass
class SensorEvent:
    """Basis-Event von einem Sensor."""
    sensor_id: str
    event_type: str
    value: Any
    timestamp: datetime
    location: Dict[str, float]
    metadata: Dict[str, Any] = None

@dataclass
class ComplexEvent:
    """Abgeleitetes Event aus Pattern-Matching."""
    pattern_id: str
    pattern_name: str
    triggering_events: list
    timestamp: datetime
    severity: str  # info, warning, critical
    description: str
    metadata: Dict[str, Any] = None
```

## 🎯 Event Patterns

### 1. Threshold Pattern

Einfachstes Pattern: Wert überschreitet Schwellwert

```python
class ThresholdPattern:
    """Erkennt Schwellwert-Überschreitungen."""
    
    def __init__(self, sensor_id: str, threshold: float, operator: str = ">"):
        self.sensor_id = sensor_id
        self.threshold = threshold
        self.operator = operator
        self.operators = {
            ">": lambda x, y: x > y,
            "<": lambda x, y: x < y,
            ">=": lambda x, y: x >= y,
            "<=": lambda x, y: x <= y,
            "==": lambda x, y: x == y
        }
    
    def match(self, event: SensorEvent) -> bool:
        """Prüft ob Event Pattern matcht."""
        if event.sensor_id != self.sensor_id:
            return False
        
        return self.operators[self.operator](event.value, self.threshold)
    
    def create_complex_event(self, event: SensorEvent) -> ComplexEvent:
        """Erstellt Complex Event bei Match."""
        severity = "warning" if abs(event.value - self.threshold) < 10 else "critical"
        
        return ComplexEvent(
            pattern_id="threshold_breach",
            pattern_name=f"Threshold {self.operator} {self.threshold}",
            triggering_events=[event],
            timestamp=datetime.now(),
            severity=severity,
            description=f"Sensor {event.sensor_id} value {event.value} breached threshold {self.threshold}",
            metadata={"threshold": self.threshold, "actual_value": event.value}
        )

# Verwendung
pattern = ThresholdPattern("temp_01", threshold=30.0, operator=">")
event = SensorEvent("temp_01", "temperature", 35.5, datetime.now(), {"lat": 52.52, "lon": 13.40})

if pattern.match(event):
    complex_event = pattern.create_complex_event(event)
    print(f"Alert: {complex_event.description}")
```

### 2. Sequence Pattern

Reihenfolge von Events muss auftreten:

```python
from collections import deque

class SequencePattern:
    """Erkennt Event-Sequenzen."""
    
    def __init__(self, event_types: list, time_window_seconds: int = 60):
        self.event_types = event_types
        self.time_window = time_window_seconds
        self.event_buffer = deque(maxlen=100)
    
    def add_event(self, event: SensorEvent):
        """Fügt Event zum Buffer hinzu."""
        self.event_buffer.append(event)
        
        # Entferne alte Events außerhalb des Zeitfensters
        cutoff = datetime.now().timestamp() - self.time_window
        while (self.event_buffer and 
               self.event_buffer[0].timestamp.timestamp() < cutoff):
            self.event_buffer.popleft()
    
    def match(self) -> bool:
        """Prüft ob Sequenz im Buffer vorhanden."""
        if len(self.event_buffer) < len(self.event_types):
            return False
        
        # Suche nach Sequenz
        for i in range(len(self.event_buffer) - len(self.event_types) + 1):
            match_found = True
            for j, expected_type in enumerate(self.event_types):
                if self.event_buffer[i + j].event_type != expected_type:
                    match_found = False
                    break
            
            if match_found:
                return True
        
        return False

# Beispiel: "Tür öffnen" → "Bewegung" → "Licht an"
pattern = SequencePattern(["door_open", "motion_detected", "light_on"], time_window_seconds=30)

pattern.add_event(SensorEvent("door_01", "door_open", True, datetime.now(), {}))
pattern.add_event(SensorEvent("motion_01", "motion_detected", True, datetime.now(), {}))
pattern.add_event(SensorEvent("light_01", "light_on", True, datetime.now(), {}))

if pattern.match():
    print("Sequence detected: Door → Motion → Light")
```

### 3. Conjunction Pattern (AND)

Mehrere Bedingungen müssen gleichzeitig erfüllt sein:

```python
class ConjunctionPattern:
    """Erkennt gleichzeitige Bedingungen (AND)."""
    
    def __init__(self, conditions: list, time_window_seconds: int = 10):
        self.conditions = conditions  # Liste von Pattern-Objekten
        self.time_window = time_window_seconds
        self.recent_matches = {}
    
    def check_event(self, event: SensorEvent) -> bool:
        """Prüft Event gegen alle Conditions."""
        now = datetime.now()
        
        # Prüfe jede Condition
        for i, condition in enumerate(self.conditions):
            if condition.match(event):
                self.recent_matches[i] = now
        
        # Entferne alte Matches
        cutoff = now.timestamp() - self.time_window
        self.recent_matches = {
            k: v for k, v in self.recent_matches.items()
            if v.timestamp() >= cutoff
        }
        
        # AND: Alle Conditions müssen erfüllt sein
        return len(self.recent_matches) == len(self.conditions)

# Beispiel: Hohe Temperatur UND Hohe Feuchtigkeit
temp_condition = ThresholdPattern("temp_01", threshold=30, operator=">")
humidity_condition = ThresholdPattern("hum_01", threshold=80, operator=">")

pattern = ConjunctionPattern([temp_condition, humidity_condition], time_window_seconds=60)

# Events prüfen
temp_event = SensorEvent("temp_01", "temperature", 35, datetime.now(), {})
hum_event = SensorEvent("hum_01", "humidity", 85, datetime.now(), {})

if pattern.check_event(temp_event) or pattern.check_event(hum_event):
    print("High temperature AND high humidity detected!")
```

### 4. Disjunction Pattern (OR)

Eine von mehreren Bedingungen muss erfüllt sein:

```python
class DisjunctionPattern:
    """Erkennt alternative Bedingungen (OR)."""
    
    def __init__(self, conditions: list):
        self.conditions = conditions
    
    def check_event(self, event: SensorEvent) -> bool:
        """Prüft ob Event irgendeine Condition erfüllt."""
        for condition in self.conditions:
            if condition.match(event):
                return True
        return False

# Beispiel: Temperatur zu hoch ODER zu niedrig
high_temp = ThresholdPattern("temp_01", threshold=40, operator=">")
low_temp = ThresholdPattern("temp_01", threshold=0, operator="<")

pattern = DisjunctionPattern([high_temp, low_temp])
```

### 5. Absence Pattern

Event fehlt innerhalb eines Zeitfensters:

```python
class AbsencePattern:
    """Erkennt fehlende Events."""
    
    def __init__(self, sensor_id: str, expected_interval_seconds: int):
        self.sensor_id = sensor_id
        self.expected_interval = expected_interval_seconds
        self.last_event_time = None
    
    def check_event(self, event: SensorEvent):
        """Registriert Event."""
        if event.sensor_id == self.sensor_id:
            self.last_event_time = datetime.now()
    
    def is_absent(self) -> bool:
        """Prüft ob Event fehlt."""
        if self.last_event_time is None:
            return True
        
        time_since_last = (datetime.now() - self.last_event_time).total_seconds()
        return time_since_last > self.expected_interval

# Verwendung für Heartbeat-Monitoring
pattern = AbsencePattern("temp_01", expected_interval_seconds=60)

# Regelmäßig prüfen
if pattern.is_absent():
    print("Sensor temp_01 is not responding!")
```

## ⏱️ Sliding Windows

### Time-Based Window

```python
from collections import defaultdict

class SlidingTimeWindow:
    """Zeit-basiertes Sliding Window."""
    
    def __init__(self, window_size_seconds: int):
        self.window_size = window_size_seconds
        self.events = deque()
    
    def add_event(self, event: SensorEvent):
        """Fügt Event hinzu."""
        self.events.append(event)
        self._cleanup()
    
    def _cleanup(self):
        """Entfernt alte Events."""
        cutoff = datetime.now().timestamp() - self.window_size
        while self.events and self.events[0].timestamp.timestamp() < cutoff:
            self.events.popleft()
    
    def get_events(self) -> list:
        """Gibt alle Events im Window zurück."""
        self._cleanup()
        return list(self.events)
    
    def count(self) -> int:
        """Zählt Events im Window."""
        self._cleanup()
        return len(self.events)
    
    def average_value(self) -> float:
        """Berechnet Durchschnitt der Werte."""
        self._cleanup()
        if not self.events:
            return 0.0
        return sum(e.value for e in self.events) / len(self.events)

# Beispiel: Durchschnitt der letzten 5 Minuten
window = SlidingTimeWindow(window_size_seconds=300)

for _ in range(10):
    event = SensorEvent("temp_01", "temperature", random.uniform(20, 30), datetime.now(), {})
    window.add_event(event)
    time.sleep(1)

avg = window.average_value()
print(f"Average temperature (last 5 min): {avg:.2f}°C")
```

### Count-Based Window

```python
class SlidingCountWindow:
    """Count-basiertes Sliding Window (letzte N Events)."""
    
    def __init__(self, window_size: int):
        self.window_size = window_size
        self.events = deque(maxlen=window_size)
    
    def add_event(self, event: SensorEvent):
        """Fügt Event hinzu (älteste wird automatisch entfernt)."""
        self.events.append(event)
    
    def get_events(self) -> list:
        """Gibt alle Events im Window zurück."""
        return list(self.events)
    
    def is_full(self) -> bool:
        """Prüft ob Window voll ist."""
        return len(self.events) == self.window_size
    
    def calculate_trend(self) -> str:
        """Berechnet Trend (steigend/fallend/stabil)."""
        if not self.is_full():
            return "insufficient_data"
        
        values = [e.value for e in self.events]
        first_half = sum(values[:len(values)//2]) / (len(values)//2)
        second_half = sum(values[len(values)//2:]) / (len(values) - len(values)//2)
        
        diff = second_half - first_half
        if abs(diff) < 1.0:
            return "stable"
        elif diff > 0:
            return "rising"
        else:
            return "falling"

# Beispiel: Trend der letzten 10 Messungen
window = SlidingCountWindow(window_size=10)
```

## 📊 Event Correlation

### Spatial Correlation

Korrelation basierend auf geografischer Nähe:

```python
import math

class SpatialCorrelation:
    """Korreliert Events basierend auf Standort."""
    
    @staticmethod
    def distance_km(loc1: dict, loc2: dict) -> float:
        """Berechnet Distanz zwischen zwei Koordinaten (Haversine)."""
        R = 6371  # Erdradius in km
        
        lat1, lon1 = math.radians(loc1["lat"]), math.radians(loc1["lon"])
        lat2, lon2 = math.radians(loc2["lat"]), math.radians(loc2["lon"])
        
        dlat = lat2 - lat1
        dlon = lon2 - lon1
        
        a = math.sin(dlat/2)**2 + math.cos(lat1) * math.cos(lat2) * math.sin(dlon/2)**2
        c = 2 * math.asin(math.sqrt(a))
        
        return R * c
    
    @staticmethod
    def find_nearby_events(
        event: SensorEvent,
        other_events: list,
        radius_km: float
    ) -> list:
        """Findet Events in der Nähe."""
        nearby = []
        for other in other_events:
            if other.sensor_id == event.sensor_id:
                continue
            
            distance = SpatialCorrelation.distance_km(
                event.location,
                other.location
            )
            
            if distance <= radius_km:
                nearby.append((other, distance))
        
        return sorted(nearby, key=lambda x: x[1])

# Beispiel: Finde alle hohen Temperaturen im Umkreis von 1km
high_temp_event = SensorEvent(
    "temp_01",
    "temperature",
    35,
    datetime.now(),
    {"lat": 52.5200, "lon": 13.4050}
)

other_events = [
    SensorEvent("temp_02", "temperature", 34, datetime.now(), {"lat": 52.5210, "lon": 13.4060}),
    SensorEvent("temp_03", "temperature", 22, datetime.now(), {"lat": 52.5300, "lon": 13.4200}),
]

nearby = SpatialCorrelation.find_nearby_events(high_temp_event, other_events, radius_km=1.0)
if nearby:
    print(f"Found {len(nearby)} sensors with similar conditions nearby")
```

### Temporal Correlation

Korrelation basierend auf zeitlicher Nähe:

```python
class TemporalCorrelation:
    """Korreliert Events basierend auf Zeit."""
    
    @staticmethod
    def find_concurrent_events(
        event: SensorEvent,
        other_events: list,
        time_window_seconds: int
    ) -> list:
        """Findet Events die zeitlich nah sind."""
        concurrent = []
        
        for other in other_events:
            time_diff = abs(
                (event.timestamp - other.timestamp).total_seconds()
            )
            
            if time_diff <= time_window_seconds:
                concurrent.append((other, time_diff))
        
        return sorted(concurrent, key=lambda x: x[1])
    
    @staticmethod
    def detect_cascade(
        events: list,
        max_delay_seconds: int = 60
    ) -> list:
        """Erkennt Kaskaden-Effekte (A → B → C)."""
        events = sorted(events, key=lambda e: e.timestamp)
        cascades = []
        
        for i in range(len(events) - 1):
            cascade = [events[i]]
            
            for j in range(i + 1, len(events)):
                time_diff = (events[j].timestamp - cascade[-1].timestamp).total_seconds()
                
                if time_diff <= max_delay_seconds:
                    cascade.append(events[j])
                else:
                    break
            
            if len(cascade) >= 3:  # Mindestens 3 Events für Kaskade
                cascades.append(cascade)
        
        return cascades
```

## 🎛️ Rule Engine

### Rule Definition

```python
from enum import Enum
from typing import Callable

class RulePriority(Enum):
    LOW = 1
    MEDIUM = 2
    HIGH = 3
    CRITICAL = 4

class Rule:
    """CEP Rule Definition."""
    
    def __init__(
        self,
        rule_id: str,
        name: str,
        condition: Callable[[SensorEvent], bool],
        action: Callable[[SensorEvent], None],
        priority: RulePriority = RulePriority.MEDIUM,
        enabled: bool = True
    ):
        self.rule_id = rule_id
        self.name = name
        self.condition = condition
        self.action = action
        self.priority = priority
        self.enabled = enabled
        self.execution_count = 0
        self.last_execution = None
    
    def evaluate(self, event: SensorEvent) -> bool:
        """Evaluiert Rule gegen Event."""
        if not self.enabled:
            return False
        
        try:
            if self.condition(event):
                self.action(event)
                self.execution_count += 1
                self.last_execution = datetime.now()
                return True
        except Exception as e:
            print(f"Rule {self.rule_id} failed: {e}")
        
        return False

class RuleEngine:
    """CEP Rule Engine."""
    
    def __init__(self):
        self.rules = []
    
    def add_rule(self, rule: Rule):
        """Fügt Rule hinzu."""
        self.rules.append(rule)
        # Sortiere nach Priorität
        self.rules.sort(key=lambda r: r.priority.value, reverse=True)
    
    def remove_rule(self, rule_id: str):
        """Entfernt Rule."""
        self.rules = [r for r in self.rules if r.rule_id != rule_id]
    
    def process_event(self, event: SensorEvent) -> list:
        """Verarbeitet Event durch alle Rules."""
        matched_rules = []
        
        for rule in self.rules:
            if rule.evaluate(event):
                matched_rules.append(rule.rule_id)
        
        return matched_rules

# Beispiel: Definiere Rules
def high_temp_condition(event: SensorEvent) -> bool:
    return event.event_type == "temperature" and event.value > 30

def high_temp_action(event: SensorEvent):
    print(f"ALERT: High temperature {event.value}°C at sensor {event.sensor_id}")
    # Sende Notification, Email, etc.

high_temp_rule = Rule(
    "rule_high_temp",
    "High Temperature Alert",
    condition=high_temp_condition,
    action=high_temp_action,
    priority=RulePriority.HIGH
)

# Initialisiere Engine
engine = RuleEngine()
engine.add_rule(high_temp_rule)

# Verarbeite Event
event = SensorEvent("temp_01", "temperature", 35, datetime.now(), {})
matched = engine.process_event(event)
```

### Complex Rule Example

```python
class ComplexRule:
    """Komplexe Rule mit State."""
    
    def __init__(self, rule_id: str, name: str):
        self.rule_id = rule_id
        self.name = name
        self.state = {}
    
    def condition(self, event: SensorEvent) -> bool:
        """
        Beispiel: Erkenne wenn Temperatur 3x hintereinander steigt
        """
        sensor_id = event.sensor_id
        
        if sensor_id not in self.state:
            self.state[sensor_id] = {"last_value": None, "increase_count": 0}
        
        sensor_state = self.state[sensor_id]
        
        if sensor_state["last_value"] is not None:
            if event.value > sensor_state["last_value"]:
                sensor_state["increase_count"] += 1
            else:
                sensor_state["increase_count"] = 0
        
        sensor_state["last_value"] = event.value
        
        return sensor_state["increase_count"] >= 3
    
    def action(self, event: SensorEvent):
        """Action bei Match."""
        print(f"Temperature rising trend detected at {event.sensor_id}")
        # Reset state
        self.state[event.sensor_id]["increase_count"] = 0
```

## 📈 Advanced Patterns

### Rate-of-Change Pattern

```python
class RateOfChangePattern:
    """Erkennt schnelle Änderungen."""
    
    def __init__(self, sensor_id: str, max_rate_per_second: float):
        self.sensor_id = sensor_id
        self.max_rate = max_rate_per_second
        self.last_event = None
    
    def check(self, event: SensorEvent) -> bool:
        """Prüft Rate of Change."""
        if self.last_event is None:
            self.last_event = event
            return False
        
        if event.sensor_id != self.sensor_id:
            return False
        
        # Berechne Rate
        time_diff = (event.timestamp - self.last_event.timestamp).total_seconds()
        value_diff = abs(event.value - self.last_event.value)
        
        if time_diff > 0:
            rate = value_diff / time_diff
            self.last_event = event
            return rate > self.max_rate
        
        return False

# Beispiel: Temperatur steigt mehr als 5°C/Sekunde
pattern = RateOfChangePattern("temp_01", max_rate_per_second=5.0)
```

### Periodicity Pattern

```python
class PeriodicityPattern:
    """Erkennt periodische Muster."""
    
    def __init__(self, expected_period_seconds: int, tolerance: float = 0.1):
        self.expected_period = expected_period_seconds
        self.tolerance = tolerance
        self.event_times = deque(maxlen=10)
    
    def add_event(self, event: SensorEvent):
        """Fügt Event hinzu."""
        self.event_times.append(event.timestamp)
    
    def is_periodic(self) -> bool:
        """Prüft ob Periodizität vorhanden."""
        if len(self.event_times) < 3:
            return False
        
        # Berechne Abstände
        intervals = []
        for i in range(len(self.event_times) - 1):
            interval = (self.event_times[i+1] - self.event_times[i]).total_seconds()
            intervals.append(interval)
        
        # Prüfe ob Abstände konsistent
        avg_interval = sum(intervals) / len(intervals)
        max_deviation = self.expected_period * self.tolerance
        
        return all(
            abs(interval - self.expected_period) <= max_deviation
            for interval in intervals
        )
```

## 🎓 Best Practices

1. **Performance**
   - Verwende Indizes für zeitbasierte Queries
   - Limitiere Window-Größen
   - Cleanup alte Events regelmäßig

2. **Fehlerbehandlung**
   - Wrape Rule-Actions in try-catch
   - Logge fehlgeschlagene Rules
   - Implementiere Circuit Breaker für Actions

3. **Monitoring**
   - Tracke Rule-Ausführungen
   - Monitore Pattern-Match-Raten
   - Alert bei ungewöhnlichen Patterns

4. **Testing**
   - Unit-Tests für jedes Pattern
   - Integration-Tests mit realen Daten
   - Load-Tests für Performance

## 📚 Weitere Dokumentation

- [SENSOR_SIMULATION.md](SENSOR_SIMULATION.md) - Sensor-Setup
- [ML_MODELS.md](ML_MODELS.md) - Anomalie-Erkennung
- [SCALING_GUIDE.md](SCALING_GUIDE.md) - Skalierung
