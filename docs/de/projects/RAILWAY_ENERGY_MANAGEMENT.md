# Energie-Management System für Railway Monitoring

## Übersicht

Echtzeit-Berechnung und -Steuerung des Stromverbrauchs für das Bahnnetz mit Integration zur Kraftwerkssteuerung.

## Energieverbrauch Bahnnetz

### Realistische Zahlen Deutsche Bahn (2023)

```
Gesamtstromverbrauch DB: ~3,2 TWh/Jahr
├─ Fernverkehr (ICE/IC):  1,8 TWh/Jahr (~56%)
├─ Regionalverkehr:       1,0 TWh/Jahr (~31%)
└─ S-Bahnen:              0,4 TWh/Jahr (~13%)

Durchschnitt pro Zug:
- ICE 3:     ~2,5 kWh/km  (200 km/h, 8 Wagen, 435 Tonnen)
- ICE 4:     ~2,8 kWh/km  (250 km/h)
- IC/EC:     ~2,0 kWh/km  (160 km/h)
- RE:        ~1,5 kWh/km  (120 km/h)
- RB:        ~1,2 kWh/km  (100 km/h)
- S-Bahn:    ~0,8 kWh/km  (Leichter Triebzug)

Spitzenlast:
- Morgens (07:00-09:00): ~800 MW
- Abends (17:00-19:00):  ~750 MW
- Nacht (00:00-05:00):   ~150 MW
```

### Einflussfaktoren auf Energieverbrauch

1. **Geschwindigkeit** (kubische Beziehung)
   ```
   P ∝ v³
   Beispiel: 200 km/h → 300 km/h = 3,4x mehr Energie
   ```

2. **Steigung/Gefälle**
   ```
   Steigung 10‰ (1%): +30% Energieverbrauch
   Gefälle 10‰:       -20% (Rekuperation)
   ```

3. **Masse**
   ```
   Pro 100 Tonnen: +12% Energieverbrauch
   Leerzug vs. Vollzug: 20-25% Unterschied
   ```

4. **Beschleunigung/Bremsen**
   ```
   Anfahren: ~500 kWh (ICE)
   Bremsen mit Rekuperation: -400 kWh zurückgewonnen
   ```

5. **Wetter**
   ```
   Gegenwind 30 km/h: +8% Energie
   Temperatur <0°C: +5% (Heizung)
   Temperatur >30°C: +3% (Klimaanlage)
   ```

## Bahnstrom-System

### Versorgungsstruktur

```
Kraftwerke → Umspannwerke → Unterwerke → Oberleitungen → Züge
              (380/220 kV)   (110 kV)     (15 kV)        (15 kV)
```

### Unterwerke (Bahnstrom)

```
Deutschland: ~800 Unterwerke
Abstand: durchschnittlich 40-60 km
Leistung: 10-60 MW pro Unterwerk
Spannung: 15 kV, 16,7 Hz (DB) / 25 kV, 50 Hz (Hochgeschwindigkeit)
```

### Kraftwerke für Bahnstrom

```
DB Energie GmbH Kraftwerke:
1. Kraftwerk Hamburg-Tiefstack:     220 MW
2. Kraftwerk Kirchmöser:            150 MW
3. Umrichterwerke (50 Hz → 16,7 Hz)

Externe Beschaffung:
- Wasserkraft:  45%
- Windkraft:    25%
- Solar:        15%
- Konventionell: 15%

Ziel 2030: 100% Grünstrom
```

## Energieverbrauchs-Berechnung

### Formel

```python
# Basis-Energieverbrauch (Traktionsenergie)
E_traction = mass_kg * distance_m * gradient_factor * speed_factor

# Rekuperation (Bremsenergie-Rückgewinnung)
E_recuperation = braking_energy * recuperation_efficiency  # 0.6-0.85

# Nebenverbraucher
E_auxiliary = (heating_power + ac_power + lighting_power) * time_h

# Gesamtverbrauch
E_total = E_traction - E_recuperation + E_auxiliary
```

### Beispiel: ICE 3 München → Hamburg (782 km)

```
Fahrzeugdaten:
- Masse: 435 Tonnen (leer) + 25 Tonnen (Passagiere) = 460 Tonnen
- Durchschnittsgeschwindigkeit: 180 km/h
- Fahrzeit: 4,3 Stunden

Energieverbrauch:
1. Traktionsenergie:     ~1.950 kWh
2. Rekuperation:          -320 kWh (16%)
3. Nebenverbraucher:
   - Klimaanlage:          +85 kWh
   - Beleuchtung:          +40 kWh
   - Bordküche:            +25 kWh
   - Systeme:              +30 kWh
   
Gesamt:                  ~1.810 kWh
Pro km:                   ~2,31 kWh/km
CO₂-Äquivalent:          ~362 kg (bei 200g/kWh Strommix)
```

## Kraftwerks-Steuerung

### Lastprognose

```python
# Vorhersage Strombedarf für nächste 24h
def predict_power_demand():
    trains_schedule = get_timetable_next_24h()
    
    power_profile = []
    for hour in range(24):
        trains_in_hour = filter_trains_by_hour(trains_schedule, hour)
        
        total_power_mw = 0
        for train in trains_in_hour:
            power_kw = calculate_train_power(
                train.category,
                train.speed_kmh,
                train.weight_tons,
                train.gradient
            )
            total_power_mw += power_kw / 1000
        
        power_profile.append({
            'hour': hour,
            'power_mw': total_power_mw,
            'trains_count': len(trains_in_hour)
        })
    
    return power_profile
```

### Kraftwerks-Dispatch

```python
# Optimiere Kraftwerks-Einsatz
def dispatch_power_plants(demand_profile):
    """
    Ziel: Minimale Kosten + Maximale Grünstrom-Nutzung
    """
    
    power_sources = [
        {'type': 'hydro', 'capacity_mw': 200, 'cost_eur_mwh': 25, 'co2_g_kwh': 0},
        {'type': 'wind', 'capacity_mw': 150, 'cost_eur_mwh': 30, 'co2_g_kwh': 0},
        {'type': 'solar', 'capacity_mw': 100, 'cost_eur_mwh': 35, 'co2_g_kwh': 0},
        {'type': 'battery', 'capacity_mw': 50, 'cost_eur_mwh': 80, 'co2_g_kwh': 0},
        {'type': 'gas', 'capacity_mw': 300, 'cost_eur_mwh': 120, 'co2_g_kwh': 350},
    ]
    
    dispatch_plan = []
    for hour_data in demand_profile:
        demand_mw = hour_data['power_mw']
        
        # Sortiere nach Kosten (Merit-Order)
        sources_sorted = sorted(power_sources, key=lambda x: x['cost_eur_mwh'])
        
        allocation = {}
        remaining_demand = demand_mw
        
        for source in sources_sorted:
            if remaining_demand <= 0:
                break
            
            # Verfügbarkeit prüfen (Wetter, Tageszeit)
            available_mw = get_available_capacity(source, hour_data['hour'])
            
            # Allokiere Kapazität
            used_mw = min(available_mw, remaining_demand)
            allocation[source['type']] = used_mw
            remaining_demand -= used_mw
        
        dispatch_plan.append({
            'hour': hour_data['hour'],
            'demand_mw': demand_mw,
            'allocation': allocation,
            'total_cost_eur': calculate_cost(allocation, power_sources),
            'co2_kg': calculate_co2(allocation, power_sources)
        })
    
    return dispatch_plan
```

### Lastverschiebung (Smart Charging)

```python
# Verschiebe nicht-kritische Züge in Zeiten mit günstiger Energie
def optimize_timetable_for_energy():
    """
    Güterverkehr kann zeitlich flexibel sein
    → Verschiebe in Nachtstunden (Wind, günstiger)
    """
    
    freight_trains = get_freight_trains()
    
    for train in freight_trains:
        # Prüfe Flexibilität
        if train.time_window_hours >= 4:
            # Finde günstigste Zeitfenster
            cheapest_slot = find_cheapest_energy_slot(train.time_window_hours)
            
            # Verschiebe Fahrplan
            reschedule_train(train, cheapest_slot)
            
            savings_eur = calculate_savings(train, cheapest_slot)
            co2_reduction_kg = calculate_co2_reduction(train, cheapest_slot)
```

## Unterwerks-Management

### Lastverteilung

```python
# Verteile Last auf Unterwerke
class SubstationManager:
    def __init__(self):
        self.substations = load_substations()  # ~800 Unterwerke
    
    def allocate_power(self, train_position, train_power_kw):
        """
        Finde nächstes Unterwerk und prüfe Kapazität
        """
        nearest_substation = self.find_nearest(train_position)
        
        # Prüfe Auslastung
        current_load = nearest_substation.current_load_mw
        max_capacity = nearest_substation.capacity_mw
        
        if current_load + (train_power_kw / 1000) > max_capacity * 0.9:
            # Überlast → Warnung
            alert_overload(nearest_substation, train_power_kw)
            
            # Alternative Versorgung
            alternative = self.find_alternative_path(train_position)
            return alternative
        
        # Allokiere Last
        nearest_substation.current_load_mw += train_power_kw / 1000
        return nearest_substation
    
    def find_nearest(self, train_pos):
        """Finde nächstes Unterwerk (spatial query)"""
        return min(self.substations, 
                   key=lambda s: distance(s.location, train_pos))
```

### Unterwerks-Datenmodell

```json
{
  "_key": "substation:UW_FFM_SUED",
  "type": "substation",
  "name": "Unterwerk Frankfurt Süd",
  "location": {
    "track": "3600",
    "km": 12.5,
    "lat": 50.0234,
    "lon": 8.5678
  },
  "technical_data": {
    "capacity_mw": 45,
    "voltage_input_kv": 110,
    "voltage_output_kv": 15,
    "frequency_hz": 16.7,
    "transformer_count": 2,
    "transformer_power_mva": 25
  },
  "current_state": {
    "current_load_mw": 32.5,
    "utilization_percent": 72.2,
    "temperature_celsius": 45,
    "status": "operational"
  },
  "connected_tracks": ["3600", "3601", "3602"],
  "supply_range_km": {
    "start": 0.0,
    "end": 50.0
  },
  "backup_substations": ["UW_FFM_NORD", "UW_MAINZ"]
}
```

## Energiedaten-Modell (Time-Series)

### Zug-Energieverbrauch

```json
{
  "metric": "train_energy_consumption",
  "entity": "ICE508",
  "timestamp": "2024-12-13T15:30:00Z",
  "value": {
    "instantaneous_power_kw": 4200,
    "cumulative_energy_kwh": 1523.5,
    "traction_power_kw": 3800,
    "auxiliary_power_kw": 400,
    "recuperation_power_kw": -300,
    "efficiency_percent": 87.3,
    "voltage_v": 15000,
    "current_a": 280
  }
}
```

### Unterwerks-Last

```json
{
  "metric": "substation_load",
  "entity": "UW_FFM_SUED",
  "timestamp": "2024-12-13T15:30:00Z",
  "value": {
    "total_load_mw": 32.5,
    "transformer_1_load_mw": 18.2,
    "transformer_2_load_mw": 14.3,
    "utilization_percent": 72.2,
    "voltage_kv": 15.1,
    "frequency_hz": 16.71,
    "temperature_celsius": 45,
    "active_trains": 12,
    "peak_load_today_mw": 42.1
  }
}
```

### Netz-Gesamtlast

```json
{
  "metric": "grid_total_load",
  "entity": "DB_NETZ",
  "timestamp": "2024-12-13T15:30:00Z",
  "value": {
    "total_power_mw": 687,
    "active_trains": 1234,
    "substations_active": 782,
    "substations_overload": 3,
    "renewable_share_percent": 78.5,
    "grid_frequency_hz": 16.70,
    "reserve_capacity_mw": 213
  }
}
```

## Kraftwerks-Integration

### API für Lastprognose

```python
POST /api/energy/forecast
{
  "horizon_hours": 24,
  "include_scenarios": ["normal", "peak", "disruption"]
}

Response:
{
  "forecast": [
    {
      "hour": 0,
      "power_mw": 687,
      "trains_count": 1234,
      "confidence": 0.92
    },
    ...
  ],
  "peak_hours": [7, 8, 17, 18],
  "total_energy_mwh": 14250,
  "renewable_share_percent": 78.5
}
```

### API für Kraftwerks-Steuerung

```python
POST /api/energy/dispatch
{
  "target_hour": 17,
  "target_power_mw": 750,
  "optimize_for": "cost"  # oder "co2", "reliability"
}

Response:
{
  "allocation": {
    "hydro": 200,
    "wind": 150,
    "solar": 80,
    "battery": 50,
    "gas": 270
  },
  "total_cost_eur": 82500,
  "co2_kg": 94500,
  "renewable_percent": 64.0
}
```

## Visualisierung (WPF)

### Energie-Dashboard

```
┌──────────────────────────────────────────────────────────┐
│  Echtzeit Energieverbrauch                               │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  Aktuelle Last: 687 MW         Grünstrom: 78.5%        │
│  Spitzenlast:   842 MW         CO₂: 137 kg/MWh         │
│                                                          │
│  ┌────────────────────────────────────────────────┐    │
│  │         Lastprofil (24h)                       │    │
│  │  900 MW ┤                                      │    │
│  │         │     ╭─╮           ╭─╮                │    │
│  │  600 MW ┤    ╭╯ ╰╮         ╭╯ ╰╮               │    │
│  │         │   ╭╯   ╰─╮   ╭──╯   ╰╮              │    │
│  │  300 MW ┤  ╭╯      ╰───╯       ╰╮             │    │
│  │         │──╯                    ╰─────        │    │
│  │    0 MW └──────────────────────────────────   │    │
│  │          0h    6h   12h   18h   24h           │    │
│  └────────────────────────────────────────────────┘    │
│                                                          │
│  ┌────────────────────────────────────────────────┐    │
│  │         Kraftwerks-Mix                         │    │
│  │                                                 │    │
│  │  Wasserkraft:  ████████████████░░░░ 200 MW    │    │
│  │  Wind:         ████████████░░░░░░░░ 150 MW    │    │
│  │  Solar:        ████████░░░░░░░░░░░  80 MW     │    │
│  │  Batterie:     ████░░░░░░░░░░░░░░░  50 MW     │    │
│  │  Gas:          █████████████████░░ 207 MW     │    │
│  │                                                 │    │
│  └────────────────────────────────────────────────┘    │
│                                                          │
│  Top Energie-Verbraucher:                               │
│  1. ICE 508  →  4.2 MW  (Frankfurt → Hamburg)          │
│  2. ICE 711  →  3.8 MW  (München → Berlin)             │
│  3. GZ 52341 →  3.5 MW  (Frachtzug)                    │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

## CEP Rules für Energie-Anomalien

```javascript
// Überlast-Warnung Unterwerk
CREATE RULE substation_overload AS
SELECT 
  substation_id,
  current_load_mw,
  capacity_mw,
  utilization_percent
FROM SubstationLoadEvents
WHERE utilization_percent > 90
WINDOW TUMBLING(1 MINUTE)
ACTION sequence(
  alert('grid_operator', priority='HIGH'),
  redistribute_load(substation_id),
  activate_backup_power()
);

// Unerwarteter Energie-Anstieg
CREATE RULE unexpected_power_spike AS
SELECT 
  AVG(power_mw) as avg_power,
  STDDEV(power_mw) as stddev
FROM GridTotalLoadEvents
WINDOW SLIDING(15 MINUTES)
HAVING AVG(power_mw) > (SELECT AVG(power_mw) FROM history WHERE time > now() - 24h) * 1.3
ACTION investigate_anomaly();
```

## Kosten-Optimierung

### Einsparungspotenziale

```
1. Rekuperation (Bremsenergie):
   Potential: 15-20% Energie-Rückgewinnung
   Einsparung: ~640 GWh/Jahr = ~64 Mio. EUR

2. Fahrweise-Optimierung:
   Potential: 5-10% durch vorausschauendes Fahren
   Einsparung: ~320 GWh/Jahr = ~32 Mio. EUR

3. Lastverschiebung (Güterverkehr):
   Potential: 3-5% durch Nutzung günstiger Zeiten
   Einsparung: ~160 GWh/Jahr = ~16 Mio. EUR

4. Grünstrom-Maximierung:
   CO₂-Reduktion: 200.000 Tonnen/Jahr
   CO₂-Preis-Einsparung: ~10 Mio. EUR

Gesamt-Einsparung: ~122 Mio. EUR/Jahr
```

## Integration in Railway Monitoring

### Zusätzliche Metriken

```python
# In train_simulator.py erweitern
class TrainEnergyCalculator:
    def calculate_energy_consumption(self, train, dt_seconds):
        # Traktionsenergie
        traction_kw = self._calculate_traction_power(
            train.mass_kg,
            train.speed_kmh,
            train.acceleration_mps2,
            train.gradient_permille
        )
        
        # Nebenverbraucher
        auxiliary_kw = self._calculate_auxiliary_power(train)
        
        # Rekuperation (beim Bremsen)
        recuperation_kw = 0
        if train.braking:
            recuperation_kw = -self._calculate_recuperation(
                train.mass_kg,
                train.speed_kmh,
                train.deceleration_mps2
            )
        
        # Gesamt
        total_power_kw = traction_kw + auxiliary_kw + recuperation_kw
        energy_kwh = (total_power_kw * dt_seconds) / 3600
        
        return {
            'power_kw': total_power_kw,
            'energy_kwh': energy_kwh,
            'traction_kw': traction_kw,
            'auxiliary_kw': auxiliary_kw,
            'recuperation_kw': recuperation_kw
        }
```

## Nächste Schritte

1. ✅ Energiedaten-Modell definiert
2. ⏳ Energieverbrauchs-Berechnung in Simulator integrieren
3. ⏳ Unterwerks-Lastverteilung implementieren
4. ⏳ Kraftwerks-Dispatch-Algorithmus
5. ⏳ WPF Energie-Dashboard
6. ⏳ API für Lastprognose
7. ⏳ CEP Rules für Energie-Anomalien

## Referenzen

- DB Energie GmbH: Geschäftsbericht 2023
- VDE-Studie: Energieeffizienz Schienenverkehr
- IEEE: Railway Power Systems
- Siemens Mobility: Energy Management White Papers
