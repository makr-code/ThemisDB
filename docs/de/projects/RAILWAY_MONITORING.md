# Railway Monitoring System - Zugüberwachung mit ThemisDB

## Übersicht

Echtzeit-Überwachungssystem für Zugverkehr der Deutschen Bahn und europäischer Bahnnetzwerke mit KI-gestützter Analyse.

### Hauptmerkmale

- **IoT Zeitreihenanalyse** - Kontinuierliche Erfassung von Zugpositionen, Geschwindigkeit, Verspätungen
- **Streckennetz-Graph** - Vollständiges Bahnnetz mit Bahnhöfen, Gleisen, Signalanlagen
- **Complex Event Processing (CEP)** - Echtzeit-Erkennung von Verspätungen, Störungen, Anomalien
- **LLM-Integration (Ollama)** - Natürlichsprachliche Analysen, Was-wäre-wenn Szenarien
- **Geo-Spatial Mapping** - OpenStreetMap Integration für Visualisierung
- **Effizienz-Metriken** - KPIs wie Pünktlichkeit, Durchschnittsverspätung, Auslastung

## Architektur

### Datenmodell

```
┌─────────────────────────────────────────────────────────────────┐
│                    ThemisDB Multi-Model Storage                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │
│  │   Graph DB   │  │ Time-Series  │  │  Geo-Spatial │           │
│  ├──────────────┤  ├──────────────┤  ├──────────────┤           │
│  │ Streckennetz │  │ Zug-Telemetry│  │ OSM Koordinaten│          │
│  │ Bahnhöfe     │  │ Position     │  │ Bahnhofs-Lage│           │
│  │ Gleise       │  │ Geschwind.   │  │ Streckenverl.│           │
│  │ Signale      │  │ Verspätung   │  │              │           │
│  └──────────────┘  └──────────────┘  └──────────────┘           │
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │
│  │  Document DB │  │  Vector DB   │  │   LLM Store  │           │
│  ├──────────────┤  ├──────────────┤  ├──────────────┤           │
│  │ Zug-Metadata │  │ Embeddings   │  │ Analysen     │           │
│  │ Fahrpläne    │  │ Hybrid Search│  │ Scenarios    │           │
│  └──────────────┘  └──────────────┘  └──────────────┘           │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
┌────────────────┐  ┌────────────────┐  ┌────────────────┐
│  CEP Engine    │  │ Ollama LLM     │  │  REST API      │
│  Pattern Match │  │ Real-time AI   │  │  WebSocket     │
│  Aggregations  │  │ Explanations   │  │  Grafana       │
└────────────────┘  └────────────────┘  └────────────────┘
```

## Datenmodell Details

### 1. Granulares Streckennetz (Graph Database)

#### Strecken-Segment (Edge) - Kleinteilige Unterteilung
```json
{
  "_key": "segment:3600_012_013",
  "_from": "track_point:3600_km12.0",
  "_to": "track_point:3600_km13.0",
  "type": "track_segment",
  "track_number": "3600",
  "segment_id": "012_013",
  "start_km": 12.0,
  "end_km": 13.0,
  "length_m": 1000,
  "geometry": {
    "type": "LineString",
    "coordinates": [
      [8.5678, 50.0234, 98.5],
      [8.5689, 50.0245, 99.2],
      [8.5701, 50.0256, 99.8]
    ]
  },
  "infrastructure": {
    "track_class": "D4",
    "max_speed_kmh": 200,
    "electrified": true,
    "voltage": "15kV_16.7Hz",
    "rail_type": "UIC60",
    "sleeper_type": "concrete",
    "ballast_condition": "good"
  },
  "gradient": {
    "avg_permille": 2.5,
    "max_permille": 4.0,
    "direction": "ascending"
  },
  "curve": {
    "radius_m": 1200,
    "cant_mm": 80,
    "cant_deficiency_mm": 120
  },
  "capacity": {
    "trains_per_hour": 12,
    "current_utilization_percent": 75
  }
}
```

#### Gleispunkt / Knoten (Vertex)
```json
{
  "_key": "track_point:3600_km12.0",
  "type": "track_point",
  "track_number": "3600",
  "km": 12.0,
  "location": {
    "lat": 50.0234,
    "lon": 8.5678,
    "altitude": 98.5
  },
  "node_type": "signal_location",
  "connected_elements": ["signal:F123", "switch:W045"]
}
```

#### Weiche (Vertex) - Detailliert
```json
{
  "_key": "switch:W045",
  "type": "switch",
  "switch_id": "W045",
  "switch_type": "EW60-500-1:9",
  "location": {
    "track": "3600",
    "km": 12.5,
    "lat": 50.0240,
    "lon": 8.5685,
    "altitude": 98.8
  },
  "configuration": {
    "design_speed_straight_kmh": 200,
    "design_speed_diverging_kmh": 60,
    "radius_diverging_m": 500,
    "left_right": "right",
    "hand_operated": false,
    "electric_control": true
  },
  "connections": {
    "stem_from": "track_point:3600_km12.0",
    "straight_to": "track_point:3600_km13.0",
    "diverging_to": "track_point:3601_km0.5"
  },
  "current_state": {
    "position": "straight",
    "locked": true,
    "signal_dependency": "signal:F123",
    "last_switched": "2024-12-13T14:23:45Z",
    "switch_count_today": 45
  },
  "monitoring": {
    "heating_active": true,
    "point_machine_current_ma": 450,
    "switch_time_ms": 8500,
    "failure_rate_per_1000": 0.3
  },
  "maintenance": {
    "last_inspection": "2024-11-20T08:00:00Z",
    "next_inspection": "2025-02-20T08:00:00Z",
    "condition": "good",
    "wear_level_percent": 12
  }
}
```

#### Signal (Vertex) - Granular
```json
{
  "_key": "signal:F123",
  "type": "signal",
  "signal_id": "F123",
  "signal_designation": "Hp0/Hp1/Hp2",
  "signal_type": {
    "category": "Hauptsignal",
    "system": "Ks-Signal",
    "aspects": ["Hp0_rot", "Hp1_gruen", "Hp2_gelb"]
  },
  "location": {
    "track": "3600",
    "km": 12.3,
    "side": "right",
    "lat": 50.0236,
    "lon": 8.5681,
    "altitude": 99.0,
    "viewing_distance_m": 1000
  },
  "control": {
    "interlocking": "ESTW_Frankfurt_Sued",
    "block_section": "BS_3600_012",
    "route_dependency": ["route:F_M_01", "route:F_M_02"],
    "switch_dependencies": ["switch:W045", "switch:W046"],
    "signal_dependencies": {
      "previous": "signal:F122",
      "next": "signal:F124",
      "distant_for": null
    }
  },
  "current_state": {
    "aspect": "Hp1_gruen",
    "commanded_aspect": "Hp1_gruen",
    "lamp_status": {
      "green": "on",
      "yellow": "off",
      "red": "off"
    },
    "aspect_change_count_today": 234,
    "last_aspect_change": "2024-12-13T15:42:18Z"
  },
  "operational_rules": {
    "approach_speed_limit_kmh": 200,
    "braking_distance_required_m": 1000,
    "signal_overlap_m": 50,
    "automatic_train_protection": "PZB90"
  },
  "monitoring": {
    "lamp_hours": {
      "green": 2345,
      "yellow": 1234,
      "red": 890
    },
    "failure_detection": "continuous",
    "last_lamp_change": "2024-10-15T10:00:00Z",
    "next_maintenance": "2025-01-15T10:00:00Z"
  }
}
```

#### Vorsignal (Vertex)
```json
{
  "_key": "signal:F123V",
  "type": "signal",
  "signal_type": {
    "category": "Vorsignal",
    "main_signal": "signal:F123",
    "distance_to_main_m": 1000
  },
  "aspects": ["Vr0_gelb", "Vr1_gruen", "Vr2_gruen_gelb"],
  "current_aspect": "Vr1_gruen",
  "repeater_for": "signal:F123"
}
```

#### Blockabschnitt (Edge)
```json
{
  "_key": "block:BS_3600_012",
  "_from": "signal:F122",
  "_to": "signal:F123",
  "type": "block_section",
  "block_id": "BS_3600_012",
  "length_m": 2500,
  "track": "3600",
  "occupancy": {
    "occupied": false,
    "train_id": null,
    "entry_time": null,
    "expected_clear_time": null
  },
  "axle_counter": {
    "in_count": 0,
    "out_count": 0,
    "status": "clear"
  },
  "track_circuit": {
    "voltage_mv": 850,
    "resistance_ohm": 2.3,
    "status": "clear"
  },
  "max_occupancy_time_sec": 180,
  "typical_occupancy_time_sec": 65
}
```

#### Fahrstraße (Graph Pattern)
```json
{
  "_key": "route:F_M_01",
  "type": "train_route",
  "route_id": "F_M_01",
  "name": "Frankfurt Süd Gleis 1 nach Mannheim",
  "start_signal": "signal:F122",
  "end_signal": "signal:F125",
  "route_elements": [
    {
      "type": "signal",
      "id": "signal:F122",
      "required_aspect": "Hp1_gruen"
    },
    {
      "type": "switch",
      "id": "switch:W045",
      "required_position": "straight",
      "lock_required": true
    },
    {
      "type": "switch",
      "id": "switch:W046",
      "required_position": "straight",
      "lock_required": true
    },
    {
      "type": "flank_protection",
      "switch_id": "switch:W047",
      "required_position": "diverging"
    },
    {
      "type": "block_section",
      "id": "block:BS_3600_012",
      "must_be_clear": true
    },
    {
      "type": "block_section",
      "id": "block:BS_3600_013",
      "must_be_clear": true
    }
  ],
  "length_m": 5200,
  "max_speed_kmh": 200,
  "setup_time_sec": 3.5,
  "release_time_sec": 2.0,
  "conflicts_with": ["route:F_M_03", "route:F_S_01"],
  "priority": "main_line",
  "last_used": "2024-12-13T15:42:15Z",
  "usage_count_today": 87
}
```

#### ESTW / Stellwerk (Vertex)
```json
{
  "_key": "interlocking:ESTW_FFM_S",
  "type": "interlocking",
  "name": "ESTW Frankfurt Süd",
  "interlocking_type": "electronic",
  "manufacturer": "Siemens",
  "model": "Simis C",
  "commissioned": "2015-03-20",
  "controlled_area": {
    "tracks": ["3600", "3601", "3602", "3603"],
    "km_from": 10.0,
    "km_to": 25.5,
    "stations": ["8000105"]
  },
  "controlled_elements": {
    "signals": 45,
    "switches": 28,
    "track_circuits": 32,
    "axle_counters": 18,
    "level_crossings": 2
  },
  "routes_available": 156,
  "status": {
    "operational": true,
    "fault_count_today": 0,
    "cpu_load_percent": 45,
    "last_restart": "2024-11-01T02:00:00Z"
  },
  "redundancy": {
    "backup_system": "ESTW_FFM_S_Backup",
    "failover_time_sec": 0.5
  }
}
```

#### Bahnübergang (Vertex)
```json
{
  "_key": "crossing:LX_3600_015",
  "type": "level_crossing",
  "crossing_id": "LX_3600_015",
  "location": {
    "track": "3600",
    "km": 15.2,
    "road_name": "Bundesstraße B3",
    "lat": 50.0280,
    "lon": 8.5720
  },
  "protection": {
    "type": "automatic_half_barriers",
    "warning_time_sec": 30,
    "barrier_descent_time_sec": 12,
    "acoustic_warning": true,
    "light_signal": true
  },
  "current_state": {
    "barriers": "open",
    "occupied_by_road_user": false,
    "last_closure": "2024-12-13T15:40:12Z",
    "closures_today": 234
  },
  "monitoring": {
    "camera_count": 2,
    "obstacle_detection": true,
    "malfunction_count_month": 1
  },
  "max_speed_kmh": 160,
  "advance_warning_signal": "signal:F123A"
}
```

#### Bahnhof (Vertex) - Erweitert
```json
{
  "_key": "station:8000105",
  "type": "station",
  "name": "Frankfurt(Main)Hbf",
  "eva_number": 8000105,
  "location": {
    "lat": 50.1067,
    "lon": 8.6625,
    "altitude": 98
  },
  "infrastructure": {
    "platforms": [
      {
        "number": "7",
        "length_m": 420,
        "height_cm": 76,
        "roof_covered": true,
        "tracks": ["track:7a", "track:7b"]
      },
      {
        "number": "11",
        "length_m": 405,
        "height_cm": 76,
        "tracks": ["track:11"]
      }
    ],
    "total_tracks": 24,
    "interlocking": "ESTW_FFM_Hbf",
    "passenger_capacity_per_hour": 50000
  },
  "category": "Fernverkehrsbahnhof",
  "operator": "DB Station&Service AG",
  "services": {
    "ticket_office": true,
    "luggage_lockers": 120,
    "elevators": 8,
    "escalators": 12
  }
}
```

### 2. Zugverkehr (Time-Series + Document DB)

#### Zug Entity (Document)
```json
{
  "_key": "train:ICE508",
  "type": "train",
  "train_number": "ICE 508",
  "category": "ICE",
  "operator": "DB Fernverkehr",
  "route": {
    "origin": "München Hbf",
    "destination": "Hamburg-Altona",
    "intermediate_stops": [
      {"station": "8000261", "scheduled": "2024-12-13T14:23:00Z"},
      {"station": "8000105", "scheduled": "2024-12-13T15:45:00Z"}
    ]
  },
  "capacity": {
    "seats_1st": 111,
    "seats_2nd": 456,
    "current_occupancy": 385
  }
}
```

#### Zug Telemetrie (Time-Series) - Granular

##### GPS & Bewegungsdaten (1 Hz Update)
```
Metric: train_gps_telemetry
Entity: ICE508
Timestamp: 2024-12-13T15:30:45.123Z
Value: {
  "gps": {
    "lat": 50.056789,
    "lon": 8.612345,
    "altitude_m": 105.2,
    "hdop": 0.8,
    "satellites": 12,
    "fix_quality": "3D_differential"
  },
  "kinematics": {
    "speed_kmh": 187.3,
    "acceleration_mps2": 0.15,
    "heading_deg": 42.5,
    "track_angle_deg": 43.1,
    "lateral_deviation_m": 0.02
  },
  "track_position": {
    "track_number": "3600",
    "km": 12.347,
    "segment_id": "3600_012_013",
    "next_signal": "signal:F123",
    "distance_to_signal_m": 653
  }
}
```

##### Fahrzeug-Systeme (10 Hz für kritische Werte)
```
Metric: train_vehicle_systems
Entity: ICE508
Timestamp: 2024-12-13T15:30:45.123Z
Value: {
  "traction": {
    "power_output_kw": 4200,
    "motor_rpm": 1850,
    "motor_temp_celsius": 78,
    "current_a": 1200,
    "voltage_v": 15000,
    "regenerative_braking": false
  },
  "braking": {
    "brake_pressure_bar": 0.0,
    "emergency_brake_active": false,
    "abs_active": false,
    "brake_pad_wear_percent": 23,
    "brake_temp_celsius": 45
  },
  "pantograph": {
    "contact_force_n": 120,
    "contact_quality": "good",
    "arcing_detected": false,
    "height_mm": 1850
  },
  "doors": {
    "all_closed": true,
    "locked": true,
    "door_faults": []
  },
  "hvac": {
    "car_1_temp_celsius": 21.5,
    "car_2_temp_celsius": 22.1,
    "outside_temp_celsius": 8.3
  },
  "diagnostics": {
    "fault_codes": [],
    "warnings": [],
    "system_health_percent": 98
  }
}
```

##### Zugbeeinflussungssystem (ETCS/PZB)
```
Metric: train_safety_systems
Entity: ICE508
Timestamp: 2024-12-13T15:30:45.123Z
Value: {
  "etcs": {
    "level": "L2",
    "mode": "FS",
    "permitted_speed_kmh": 200,
    "target_speed_kmh": 200,
    "target_distance_m": null,
    "brake_curve_active": false,
    "rbc_connection": "RBC_Frankfurt",
    "balise_last_passed": "balise:3600_km12.300"
  },
  "pzb": {
    "active": true,
    "monitoring_mode": "U",
    "freedom_enabled": false,
    "last_magnet": "1000Hz_3600_km12.250"
  },
  "lzb": {
    "active": false
  },
  "driver_intervention": {
    "vigilance_reset_required": false,
    "last_vigilance_reset": "2024-12-13T15:29:30Z"
  }
}
```

### 3. IoT Streckensensoren (Time-Series) - Infrastruktur

#### Gleisfreimeldeabschnitt / Achszähler
```
Metric: axle_counter_event
Entity: axle_counter:AC_3600_012_IN
Timestamp: 2024-12-13T15:30:42.456Z
Value: {
  "location": {
    "track": "3600",
    "km": 12.000,
    "direction": "in",
    "block_section": "BS_3600_012"
  },
  "event": {
    "type": "axle_detected",
    "axle_count": 16,
    "train_id": "ICE508",
    "speed_kmh": 186.5,
    "wheelbase_pattern": [2.5, 2.5, 18.5, 2.5, 2.5, 18.5],
    "train_length_m": 201.5
  },
  "block_status": {
    "before_event": "clear",
    "after_event": "occupied",
    "in_count": 16,
    "out_count": 0,
    "discrepancy": false
  }
}
```

#### Hotbox / Heißläufer-Detektor (HABD)
```
Metric: hotbox_detector
Entity: hotbox:HABD_3600_015
Timestamp: 2024-12-13T15:31:15.789Z
Value: {
  "location": {
    "track": "3600",
    "km": 15.234,
    "lat": 50.0289,
    "lon": 8.5734
  },
  "train": {
    "train_id": "ICE508",
    "speed_kmh": 188.2,
    "direction": "increasing_km"
  },
  "measurements": [
    {
      "axle_position": 1,
      "bearing_position": "left_front",
      "temperature_celsius": 42.3,
      "delta_ambient_celsius": 34.0,
      "status": "normal"
    },
    {
      "axle_position": 8,
      "bearing_position": "right_rear",
      "temperature_celsius": 78.5,
      "delta_ambient_celsius": 70.2,
      "status": "warning",
      "alert": "elevated_temperature"
    }
  ],
  "ambient_temperature_celsius": 8.3,
  "detection_quality": "good",
  "alert_triggered": true,
  "notification_sent": ["operations_center", "train:ICE508"]
}
```

#### Gleisgeometrie-Messwagen Daten
```
Metric: track_geometry
Entity: track_section:3600_012
Timestamp: 2024-12-10T03:15:00Z
Value: {
  "location": {
    "track": "3600",
    "km_from": 12.000,
    "km_to": 13.000
  },
  "geometry": {
    "gauge_mm": {
      "avg": 1435.2,
      "min": 1434.8,
      "max": 1435.7,
      "std_dev": 0.3
    },
    "alignment_mm": {
      "max_deviation": 2.1,
      "wavelength_m": 25,
      "status": "good"
    },
    "level_mm": {
      "left_rail_max": 1.8,
      "right_rail_max": 2.2,
      "cross_level_max": 1.5,
      "status": "good"
    },
    "cant_mm": {
      "design": 80,
      "measured_avg": 79.5,
      "deviation_max": 3.2
    }
  },
  "rail_surface": {
    "corrugation_depth_mm": 0.03,
    "wear_mm": 1.2,
    "defects_count": 0
  },
  "quality_index": 95.2,
  "speed_restriction_required": false,
  "next_measurement": "2025-03-10"
}
```

#### Weichen-Sensoren (Real-time)
```
Metric: switch_telemetry
Entity: switch:W045
Timestamp: 2024-12-13T15:42:18.234Z
Value: {
  "position": {
    "commanded": "straight",
    "actual": "straight",
    "locked": true,
    "detection_verified": true
  },
  "point_machine": {
    "motor_current_ma": 420,
    "switch_time_ms": 8350,
    "voltage_v": 220,
    "temperature_celsius": 32
  },
  "blade_position": {
    "left_blade_gap_mm": 0.5,
    "right_blade_gap_mm": 0.3,
    "contact_pressure_n": 450
  },
  "heating": {
    "active": true,
    "power_kw": 2.5,
    "rail_temperature_celsius": 12.5,
    "outside_temperature_celsius": -2.3
  },
  "diagnostics": {
    "switch_count_today": 87,
    "switch_count_total": 234567,
    "last_maintenance": "2024-11-15T08:00:00Z",
    "wear_estimated_percent": 15,
    "fault_history_24h": []
  }
}
```

#### Signal-Sensoren
```
Metric: signal_telemetry
Entity: signal:F123
Timestamp: 2024-12-13T15:42:19.123Z
Value: {
  "aspect": {
    "commanded": "Hp1_gruen",
    "displayed": "Hp1_gruen",
    "verification": "ok"
  },
  "lamps": {
    "green": {
      "status": "on",
      "brightness_percent": 98,
      "current_ma": 450,
      "hours_on": 2345
    },
    "yellow": {
      "status": "off",
      "brightness_percent": 0,
      "hours_on": 1234
    },
    "red": {
      "status": "off",
      "brightness_percent": 0,
      "hours_on": 890
    }
  },
  "visibility": {
    "ambient_light_lux": 450,
    "fog_detected": false,
    "viewing_distance_m": 1000
  },
  "power": {
    "voltage_v": 230,
    "backup_battery_percent": 100
  },
  "diagnostics": {
    "aspect_changes_today": 234,
    "lamp_test_result": "pass",
    "fault_detection": "active"
  }
}
```

#### Bahnübergang Sensoren
```
Metric: level_crossing_telemetry
Entity: crossing:LX_3600_015
Timestamp: 2024-12-13T15:40:12.456Z
Value: {
  "state": {
    "barriers": "descending",
    "lights_flashing": true,
    "acoustic_warning": true,
    "countdown_sec": 8
  },
  "detection": {
    "approaching_train": "ICE508",
    "train_distance_m": 850,
    "train_speed_kmh": 157,
    "eta_sec": 19
  },
  "barriers": {
    "left_position_deg": 45,
    "right_position_deg": 43,
    "descent_time_sec": 4.2,
    "motor_current_ma": 1200
  },
  "road_sensors": {
    "vehicle_on_crossing": false,
    "pedestrian_detected": false,
    "obstacle_detected": false
  },
  "cameras": {
    "camera_1_status": "recording",
    "camera_2_status": "recording",
    "video_storage_hours": 72
  },
  "safety": {
    "emergency_button_pressed": false,
    "fault_detected": false,
    "backup_power_active": false
  }
}
```

#### Oberleitungs-Monitoring
```
Metric: catenary_monitoring
Entity: catenary:3600_012
Timestamp: 2024-12-13T15:30:00Z
Value: {
  "location": {
    "track": "3600",
    "km_from": 12.0,
    "km_to": 13.0
  },
  "electrical": {
    "voltage_v": 15000,
    "current_a": 850,
    "frequency_hz": 16.7,
    "power_factor": 0.92
  },
  "mechanical": {
    "wire_height_mm": 5300,
    "wire_stagger_mm": 200,
    "wire_tension_n": 1500,
    "temperature_celsius": 15.2
  },
  "pantograph_contact": {
    "contact_force_n": 120,
    "arcing_events_count": 0,
    "wear_mm": 0.5
  },
  "ice_detection": {
    "ice_present": false,
    "temperature_celsius": 15.2,
    "heating_active": false
  }
}
```

#### Wetter-Stationen (entlang Strecke)
```
Metric: weather_station
Entity: weather:3600_015
Timestamp: 2024-12-13T15:30:00Z
Value: {
  "location": {
    "track": "3600",
    "km": 15.0,
    "lat": 50.0285,
    "lon": 8.5730
  },
  "meteorological": {
    "temperature_celsius": 8.3,
    "humidity_percent": 72,
    "pressure_hpa": 1013,
    "wind_speed_kmh": 12.5,
    "wind_direction_deg": 270,
    "precipitation_mm_h": 0.0,
    "visibility_m": 15000
  },
  "rail_conditions": {
    "rail_temperature_celsius": 9.1,
    "ice_risk": false,
    "leaf_detection": false,
    "snow_depth_cm": 0
  },
  "alerts": {
    "storm_warning": false,
    "fog_warning": false,
    "ice_warning": false,
    "heat_warning": false
  }
}
```

#### RFID / Balisen (Train Detection)
```
Metric: balise_reading
Entity: balise:3600_km12.300
Timestamp: 2024-12-13T15:30:42.345Z
Value: {
  "balise_id": "balise:3600_km12.300",
  "location": {
    "track": "3600",
    "km": 12.300,
    "lat": 50.0237,
    "lon": 8.5682
  },
  "read_event": {
    "train_id": "ICE508",
    "train_speed_kmh": 187.1,
    "read_quality": "excellent",
    "data_transmitted": {
      "track_number": "3600",
      "next_signal": "signal:F123",
      "distance_to_signal_m": 700,
      "permitted_speed_kmh": 200,
      "gradient_permille": 2.5
    }
  },
  "balise_health": {
    "battery_voltage_v": 3.2,
    "signal_strength_dbm": -45,
    "read_count_today": 234,
    "last_maintenance": "2024-10-01"
  }
}
```

### Metriken-Übersicht

#### Pro Zug (Hochfrequent 1-10 Hz):
- `train_gps_telemetry` - GPS Position, Geschwindigkeit, Beschleunigung
- `train_vehicle_systems` - Antrieb, Bremsen, Pantograph, Türen, HVAC
- `train_safety_systems` - ETCS, PZB, LZB Status
- `train_passenger_info` - Auslastung, Türaktivität, WLAN
- `train_energy_consumption` - Energieverbrauch, Rekuperation

#### Pro Infrastruktur-Element (Event-basiert oder Polling):
- `axle_counter_event` - Zugein-/ausfahrt in Blockabschnitte
- `hotbox_detector` - Heißläufer-Detektion
- `track_geometry` - Gleiszustandsdaten (periodisch)
- `switch_telemetry` - Weichenstellung, Zustand, Heizung
- `signal_telemetry` - Signalaspekt, Lampen, Sichtbarkeit
- `level_crossing_telemetry` - Bahnübergang Status
- `catenary_monitoring` - Oberleitung Spannung, Zustand
- `weather_station` - Wetter entlang Strecke
- `balise_reading` - RFID/Balisen Ereignisse

### 4. Störungsmeldungen (Document DB + Time-Series)

#### Baustelle (Document)
```json
{
  "_key": "construction:BS_2024_0123",
  "type": "construction",
  "title": "Gleiserneuerung Frankfurt-Mannheim",
  "status": "active",
  "priority": "high",
  "affected_tracks": ["3600", "3601"],
  "location": {
    "start_station": "8000105",
    "end_station": "8000244",
    "start_km": 12.5,
    "end_km": 45.8
  },
  "time_window": {
    "start": "2024-12-15T00:00:00Z",
    "end": "2025-01-20T23:59:59Z",
    "daily_hours": "22:00-06:00"
  },
  "impact": {
    "speed_limit_kmh": 80,
    "single_track_operation": true,
    "estimated_delay_min": 15,
    "affected_train_types": ["ICE", "IC", "RE"]
  },
  "description": "Erneuerung von 33 km Gleis inkl. Schwellen und Schotter",
  "created_at": "2024-11-01T10:00:00Z"
}
```

#### Technischer Ausfall (Document + Event)
```json
{
  "_key": "failure:FAIL_2024_0456",
  "type": "failure",
  "category": "signal_system",
  "severity": "critical",
  "status": "ongoing",
  "component": {
    "type": "signal",
    "id": "F123",
    "name": "Hauptsignal Frankfurt Km 12.3"
  },
  "location": {
    "track": "3600",
    "km": 12.3,
    "lat": 50.0234,
    "lon": 8.5678
  },
  "detection_time": "2024-12-13T14:32:15Z",
  "detection_method": "automatic_monitoring",
  "impact": {
    "trains_affected": 12,
    "speed_restriction_kmh": 40,
    "alternative_route_available": true,
    "estimated_repair_hours": 4
  },
  "error_code": "E_SIGNAL_LAMP_FAILURE",
  "technician_assigned": "TECH_4523",
  "eta_resolution": "2024-12-13T18:30:00Z"
}
```

#### Unfall/Notfall (Document + Event)
```json
{
  "_key": "incident:INC_2024_0789",
  "type": "incident",
  "category": "collision",
  "severity": "critical",
  "status": "emergency_response_active",
  "location": {
    "track": "3600",
    "km": 23.7,
    "lat": 50.0123,
    "lon": 8.4567,
    "nearest_station": "8000105"
  },
  "time": "2024-12-13T15:45:23Z",
  "involved_trains": [
    {"train_number": "ICE 508", "damage_level": "minor"},
    {"train_number": "CARGO_1234", "damage_level": "none"}
  ],
  "casualties": {
    "fatalities": 0,
    "injuries": 3,
    "evacuated": 245
  },
  "emergency_services": {
    "police": true,
    "fire_brigade": true,
    "ambulance": 2,
    "response_time_min": 8
  },
  "track_status": {
    "blocked_tracks": ["3600", "3601"],
    "alternative_route": "3602",
    "estimated_clearance_hours": 6
  },
  "description": "Auffahrunfall durch Notbremsung wegen Tierkollision",
  "investigation_status": "ongoing"
}
```

#### Störungs-Events (Time-Series)
```
Event Stream: disruption_events

{
  "timestamp": "2024-12-13T14:32:15Z",
  "event_type": "FAILURE_DETECTED",
  "disruption_id": "FAIL_2024_0456",
  "severity": "critical",
  "component": "signal:F123",
  "impact_score": 8.5,  // 0-10
  "affected_train_count": 12
}

{
  "timestamp": "2024-12-13T15:45:23Z",
  "event_type": "INCIDENT_REPORTED",
  "disruption_id": "INC_2024_0789",
  "severity": "critical",
  "category": "collision",
  "casualties": 3,
  "tracks_blocked": 2
}

{
  "timestamp": "2024-12-15T00:00:00Z",
  "event_type": "CONSTRUCTION_STARTED",
  "disruption_id": "BS_2024_0123",
  "duration_days": 37,
  "affected_trains_per_day": 45
}
```

### 5. Zugplanung (Document DB + Graph DB)

#### Fahrplan (Document)
```json
{
  "_key": "schedule:ICE_508_2024W50",
  "type": "train_schedule",
  "train_number": "ICE 508",
  "valid_from": "2024-12-09T00:00:00Z",
  "valid_to": "2024-12-15T23:59:59Z",
  "operating_days": {
    "monday": true,
    "tuesday": true,
    "wednesday": true,
    "thursday": true,
    "friday": true,
    "saturday": false,
    "sunday": false
  },
  "route": [
    {
      "station_id": "8000261",
      "station_name": "München Hbf",
      "arrival": null,
      "departure": "06:00:00",
      "platform": "11",
      "stop_duration_min": 0
    },
    {
      "station_id": "8000261",
      "station_name": "Augsburg Hbf",
      "arrival": "06:32:00",
      "departure": "06:34:00",
      "platform": "5",
      "stop_duration_min": 2
    },
    {
      "station_id": "8000105",
      "station_name": "Frankfurt(Main)Hbf",
      "arrival": "08:45:00",
      "departure": "08:50:00",
      "platform": "7",
      "stop_duration_min": 5
    }
  ],
  "rolling_stock": {
    "train_type": "ICE 3",
    "units": 1,
    "cars": 8,
    "total_seats": 456,
    "required_maintenance": false
  },
  "resource_requirements": {
    "crew_size": 3,
    "catering": true,
    "cleaning_required": true
  }
}
```

#### Umlaufplanung (Graph Edge)
```json
{
  "_from": "schedule:ICE_508_2024W50",
  "_to": "schedule:ICE_509_2024W50",
  "type": "train_rotation",
  "connection_type": "same_rolling_stock",
  "turnaround_location": "8011160",
  "turnaround_time_min": 45,
  "maintenance_window": {
    "start": "12:00:00",
    "end": "12:30:00",
    "type": "quick_inspection"
  }
}
```

#### Dynamische Fahrplan-Anpassung
```json
{
  "_key": "schedule_adjustment:ADJ_2024_0234",
  "type": "schedule_adjustment",
  "reason": "construction",
  "affected_schedule": "schedule:ICE_508_2024W50",
  "adjustment_type": "route_deviation",
  "valid_from": "2024-12-15T00:00:00Z",
  "valid_to": "2025-01-20T23:59:59Z",
  "changes": [
    {
      "station_id": "8000105",
      "original_arrival": "08:45:00",
      "adjusted_arrival": "09:00:00",
      "delay_min": 15,
      "platform_change": {"from": "7", "to": "9"}
    }
  ],
  "alternative_route": ["8000105", "8000244", "8000096"],
  "reason_details": "Bauarbeiten Strecke 3600 Km 12-45"
}
```

### 6. Personalplanung (Document DB + Time-Series)

#### Mitarbeiter-Stammdaten
```json
{
  "_key": "staff:EMP_12345",
  "type": "employee",
  "employee_id": "EMP_12345",
  "personal_info": {
    "name": "Max Mustermann",
    "employee_number": "123456",
    "date_of_birth": "1985-03-15",
    "hire_date": "2010-06-01"
  },
  "job_role": {
    "position": "Zugführer",
    "category": "train_driver",
    "qualification_level": "ICE_certified",
    "certifications": [
      {"type": "ICE_license", "valid_until": "2026-12-31"},
      {"type": "safety_training", "valid_until": "2025-06-30"}
    ]
  },
  "home_depot": "8000105",
  "contract": {
    "type": "full_time",
    "hours_per_week": 40,
    "max_consecutive_days": 5,
    "min_rest_hours": 11
  },
  "availability": {
    "shift_preferences": ["early", "late"],
    "unavailable_dates": ["2024-12-24", "2024-12-25"]
  }
}
```

#### Schichtplan
```json
{
  "_key": "shift:SHIFT_2024W50_001",
  "type": "shift_assignment",
  "employee_id": "EMP_12345",
  "week": "2024-W50",
  "shifts": [
    {
      "date": "2024-12-09",
      "shift_type": "early",
      "start_time": "05:30:00",
      "end_time": "13:30:00",
      "assignment": {
        "train_number": "ICE 508",
        "route": "München - Hamburg",
        "role": "Zugführer",
        "boarding_station": "8000261",
        "alighting_station": "8002549"
      },
      "break_times": [
        {"start": "08:45:00", "end": "09:00:00", "location": "8000105"}
      ]
    },
    {
      "date": "2024-12-10",
      "shift_type": "late",
      "start_time": "12:00:00",
      "end_time": "20:00:00",
      "assignment": {
        "train_number": "ICE 509",
        "route": "Hamburg - München",
        "role": "Zugführer"
      }
    }
  ],
  "total_hours": 16,
  "rest_compliance": true,
  "created_by": "scheduler_auto",
  "approved_by": "MANAGER_567",
  "status": "approved"
}
```

#### Personalverfügbarkeit (Time-Series)
```
Metric: staff_availability
Entity: EMP_12345
Timestamp: 2024-12-13T00:00:00Z
Value: {
  "status": "available",
  "location": "home_depot",
  "on_shift": false,
  "on_call": true,
  "hours_worked_this_week": 32,
  "hours_remaining": 8,
  "last_shift_end": "2024-12-12T20:00:00Z",
  "next_shift_start": "2024-12-14T05:30:00Z"
}
```

#### Qualifikations-Matrix
```json
{
  "_key": "qualification_matrix:2024",
  "type": "qualification_matrix",
  "depot": "8000105",
  "summary": {
    "total_staff": 145,
    "by_qualification": {
      "ICE_driver": 45,
      "IC_driver": 38,
      "RE_driver": 32,
      "conductor": 30
    },
    "certification_expiry_next_30_days": 8,
    "training_required": 12
  },
  "staff_list": [
    {
      "employee_id": "EMP_12345",
      "qualifications": ["ICE", "IC"],
      "availability_percent": 95,
      "overtime_hours_month": 8
    }
  ]
}
```

#### Personalbedarfs-Prognose
```json
{
  "_key": "staffing_forecast:2024W50",
  "type": "staffing_forecast",
  "week": "2024-W50",
  "depot": "8000105",
  "forecast": {
    "required_shifts": 420,
    "available_staff": 145,
    "coverage_ratio": 1.05,
    "shortfall_days": ["2024-12-11", "2024-12-13"],
    "shortfall_roles": [
      {"role": "ICE_driver", "missing": 2},
      {"role": "conductor", "missing": 1}
    ]
  },
  "optimization_suggestions": [
    "Hire 2 temporary ICE drivers for December",
    "Cross-train IC drivers for ICE certification",
    "Request staff loan from depot 8000244"
  ],
  "generated_at": "2024-12-01T10:00:00Z",
  "algorithm": "llm_based_optimization"
}
```

## CEP Rules (Complex Event Processing)

### Verspätungs-Erkennung
```javascript
CREATE RULE delayed_train_alert AS
SELECT trainNumber, delay_min, current_station
FROM TrainTelemetryEvents
WHERE delay_min > 5
WINDOW TUMBLING(1 MINUTE)
GROUP BY trainNumber
HAVING AVG(delay_min) > 5
ACTION notify('operations_center');
```

### Signal-Störung mit Kaskadeneffekt
```javascript
CREATE RULE signal_failure_cascade AS
SELECT signalId, COUNT(*) as failure_count
FROM SignalStatusEvents
WHERE status = 'failure'
WINDOW SLIDING(10 MINUTES)
GROUP BY signalId
HAVING COUNT(*) >= 3
ACTION alert('maintenance_team');
```

### Technischer Ausfall - Kritische Komponente
```javascript
CREATE RULE critical_component_failure AS
SELECT 
  component_id,
  component_type,
  failure_count,
  affected_trains
FROM ComponentFailureEvents
WHERE severity = 'critical'
WINDOW TUMBLING(1 MINUTE)
ACTION sequence(
  alert('emergency_response'),
  reroute_affected_trains(),
  notify_passengers()
);
```

### Unfall-Erkennung durch Anomalie
```javascript
CREATE RULE incident_detection AS
SELECT 
  t.trainNumber,
  t.speed_kmh,
  t.emergency_brake,
  s.signal_status
FROM TrainTelemetryEvents t
JOIN SignalStatusEvents s
  ON t.current_track = s.track_id
WHERE 
  (t.speed_kmh < 10 AND t.scheduled_speed > 100)
  OR t.emergency_brake = true
  OR (t.acceleration_mps2 < -5)
WINDOW SLIDING(30 SECONDS)
HAVING COUNT(*) > 0
ACTION sequence(
  create_incident_report(),
  alert('emergency_services'),
  block_affected_tracks(),
  initiate_emergency_protocol()
);
```

### Baustellen-Auswirkung auf Verkehr
```javascript
CREATE RULE construction_delay_correlation AS
SELECT 
  c.construction_id,
  c.location,
  COUNT(DISTINCT t.trainNumber) as delayed_trains,
  AVG(t.delay_min) as avg_delay
FROM ConstructionZoneEvents c
JOIN TrainTelemetryEvents t
  ON t.current_track = c.affected_track
  AND t.position_km BETWEEN c.start_km AND c.end_km
WHERE t.delay_min > 0
WINDOW TUMBLING(5 MINUTES)
GROUP BY c.construction_id, c.location
HAVING COUNT(DISTINCT t.trainNumber) > 5
ACTION sequence(
  adjust_construction_schedule(),
  notify('traffic_control'),
  suggest_alternative_routes()
);
```

### Mehrfach-Störungen - Krisenmodus
```javascript
CREATE RULE multiple_disruptions_crisis AS
SELECT 
  COUNT(DISTINCT disruption_id) as disruption_count,
  COLLECT_LIST(disruption_type) as types,
  SUM(affected_trains) as total_affected
FROM DisruptionEvents
WHERE severity IN ('high', 'critical')
WINDOW SLIDING(15 MINUTES)
HAVING COUNT(DISTINCT disruption_id) >= 3
ACTION sequence(
  activate_crisis_mode(),
  alert('central_command'),
  llm_analyze_situation(),
  propose_emergency_timetable()
);
```

### Notfall-Evakuierung erforderlich
```javascript
CREATE RULE evacuation_required AS
SELECT 
  incident_id,
  casualties,
  train_occupancy,
  location
FROM IncidentEvents
WHERE 
  casualties > 0
  OR blocked_tracks >= 2
  OR fire_detected = true
WINDOW TUMBLING(1 MINUTE)
ACTION sequence(
  alert('emergency_services', priority='CRITICAL'),
  evacuate_trains_in_zone(radius_km=5),
  block_tracks_in_zone(radius_km=2),
  notify_hospitals(),
  stream_to_crisis_dashboard()
);
```

### Überfüllung
```javascript
CREATE RULE overcrowded_train AS
SELECT trainNumber, occupancy_percent, route
FROM TrainOccupancyEvents
WHERE occupancy_percent > 95
WINDOW TUMBLING(5 MINUTES)
ACTION suggest('add_extra_train');
```

## LLM Integration (Ollama)

### Use Cases

#### 1. Echtzeit-Anomalie-Erklärung
```
User: "Warum hat ICE 508 15 Minuten Verspätung?"

LLM Analyse:
1. Abfrage Zeitreihen: train_delay_min für ICE 508
2. Korrelation mit Ereignissen: Signal F123 Störung um 14:32
3. Graph-Traversierung: Strecke Frankfurt-Mannheim
4. Antwort: "ICE 508 hat Verspätung aufgrund einer Signalstörung 
   (Signal F123) auf der Strecke Frankfurt-Mannheim. Die Störung 
   wurde um 14:32 gemeldet und führte zu einer Geschwindigkeits-
   begrenzung auf 80 km/h."
```

#### 2. Was-wäre-wenn Szenarien
```
User: "Was passiert, wenn Signal F123 für 2 Stunden ausfällt?"

LLM Simulation:
1. Betroffene Züge identifizieren (Graph Query)
2. Alternative Routen berechnen
3. Verspätungs-Propagation simulieren
4. Antwort: "Bei Ausfall von Signal F123 sind 12 Züge betroffen:
   - ICE 508: +25 Min Verspätung (Umleitung über F124)
   - IC 2314: +18 Min Verspätung
   - RE 4523: +12 Min Verspätung
   Empfehlung: Aktiviere Ersatzsignal F123a"
```

#### 3. Effizienz-Analyse
```
User: "Wie effizient war der Zugverkehr heute?"

LLM Report:
Zeitreihen-Aggregation (heute 00:00 - 23:59):
- Pünktlichkeitsquote: 87.3% (724 von 829 Zügen pünktlich)
- Durchschnittliche Verspätung: 4.2 Minuten
- Hauptursachen: Signalstörungen (42%), Wetterbedingungen (28%)
- Auslastung: 73% (Peak: 95% um 08:00 und 17:00)
- Energieeffizienz: 0.032 kWh/Passagier-km (12% besser als Vormonat)
```

### Prompt Templates

#### Template: Verspätungsanalyse
```python
PROMPT_DELAY_ANALYSIS = """
Du bist ein Experte für Bahnbetrieb. Analysiere folgende Zugverspätung:

Zug: {train_number}
Aktuelle Verspätung: {delay_min} Minuten
Strecke: {current_track}
Position: {current_position}

Ereignisse (letzte 30 Min):
{recent_events}

Zeitreihen-Daten:
{timeseries_data}

Aufgabe:
1. Identifiziere die Hauptursache der Verspätung
2. Bewerte den Schweregrad (niedrig/mittel/hoch)
3. Schlage Abhilfemaßnahmen vor
4. Schätze voraussichtliche Aufholzeit

Antwort als JSON:
{{
  "root_cause": "...",
  "severity": "...",
  "recommendations": [...],
  "estimated_recovery_min": ...
}}
"""
```

## API Endpoints

### Zugverkehr
```
GET  /api/trains                    # Alle aktiven Züge
GET  /api/trains/{id}                # Zug Details
GET  /api/trains/{id}/telemetry      # Zeitreihen-Daten
POST /api/trains/{id}/simulate       # Was-wäre-wenn Simulation
```

### Streckennetz
```
GET  /api/stations                   # Alle Bahnhöfe
GET  /api/stations/{id}              # Bahnhof Details
GET  /api/tracks                     # Alle Strecken
GET  /api/signals                    # Signalstatus
```

### Analytics
```
GET  /api/analytics/delays           # Verspätungs-Statistiken
GET  /api/analytics/efficiency       # Effizienz-Metriken
GET  /api/analytics/predictions      # ML Vorhersagen
POST /api/analytics/llm-query        # LLM Analyse-Anfrage
```

### Real-time
```
WS   /ws/trains                      # WebSocket: Live-Positionen
WS   /ws/events                      # WebSocket: CEP Events
```

## Beispiel-Queries

### AQL: Verspätete Züge auf Strecke
```sql
FOR train IN trains
  FILTER train.delay_min > 10
  FOR v, e, p IN 1..3 OUTBOUND train GRAPH 'railway_network'
    FILTER v.type == 'station'
  RETURN {
    train: train.train_number,
    delay: train.delay_min,
    route: p.vertices[*].name
  }
```

### Time-Series: Durchschnittsverspätung letzte 24h
```python
from_time = now() - 24*3600*1000
result = ts_store.aggregate(
    metric="train_delay_min",
    from_ms=from_time,
    to_ms=now(),
    aggregation="avg",
    bucket_size_ms=3600*1000  # 1 hour buckets
)
```

### CEP: Kaskadierende Verspätungen
```sql
SELECT 
  t1.trainNumber as delayed_train,
  t2.trainNumber as affected_train,
  t2.delay_min - t1.delay_min as additional_delay
FROM TrainDelayEvents t1
JOIN TrainDelayEvents t2
  ON t1.next_station = t2.current_station
  AND t2.timestamp > t1.timestamp
  AND t2.timestamp < t1.timestamp + 600000  -- 10 minutes
WHERE t1.delay_min > 5
```

## Deployment

### Docker Compose
```yaml
version: '3.8'
services:
  themisdb:
    image: themisdb/themisdb:latest
    ports:
      - "8765:8765"
    volumes:
      - ./data:/data
      - ./config/railway_monitoring.yaml:/etc/themis/config.yaml
    environment:
      - THEMIS_ENABLE_CEP=true
      - THEMIS_ENABLE_TIMESERIES=true
  
  ollama:
    image: ollama/ollama:latest
    ports:
      - "11434:11434"
    volumes:
      - ./models:/root/.ollama
    command: serve
  
  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"
    volumes:
      - ./grafana/dashboards:/etc/grafana/dashboards
```

### Konfiguration
```yaml
# config/railway_monitoring.yaml
storage:
  rocksdb_path: /data/railway_db

server:
  host: 0.0.0.0
  port: 8765

timeseries:
  enabled: true
  retention_days: 90
  compression: gorilla
  
cep:
  enabled: true
  rules_path: /etc/themis/cep_rules/
  
llm:
  enabled: true
  provider: ollama
  endpoint: http://ollama:11434
  model: llama3.2:latest
  
geo:
  enabled: true
  osm_import: true
```

## Datengenerierung

### Fake-Daten Generator
```cpp
// examples/railway/fake_data_generator.cpp
#include <themis/timeseries/timeseries.h>
#include <themis/graph/graph_index.h>

void generateRailwayNetwork() {
  // Deutsche Bahn Hauptstrecken
  std::vector<Station> stations = {
    {"8000105", "Frankfurt(Main)Hbf", 50.1067, 8.6625},
    {"8000261", "Nürnberg Hbf", 49.4458, 11.0839},
    {"8000244", "Mannheim Hbf", 49.4792, 8.4689}
  };
  
  for (auto& station : stations) {
    graph->addVertex("station:" + station.eva, station.toJson());
  }
  
  // Strecken hinzufügen
  graph->addEdge("track:3600", "station:8000105", "station:8000244", {
    {"distance_km", 73.5},
    {"max_speed_kmh", 200}
  });
}

void simulateTrainMovement(const std::string& train_id) {
  auto now_ms = currentTimeMillis();
  
  // Position simulieren (entlang Strecke)
  double lat = 50.1067 + (random() * 0.01);
  double lon = 8.6625 + (random() * 0.01);
  double speed = 150 + (random() * 50);
  int delay = randomDelay();  // -2 bis +15 Minuten
  
  nlohmann::json telemetry = {
    {"lat", lat},
    {"lon", lon},
    {"speed_kmh", speed},
    {"delay_min", delay}
  };
  
  ts_store->put("train_position", train_id, now_ms, telemetry);
}
```

## Performance-Anforderungen

### Erwartete Last
- **Züge aktiv**: ~5.000 (Deutschland) / ~50.000 (Europa)
- **IoT Updates**: 1 Hz pro Zug = 5.000 writes/sec
- **CEP Events**: ~10.000 events/sec
- **API Requests**: ~1.000 req/sec (peak)
- **WebSocket Connections**: ~10.000 concurrent

### ThemisDB Optimierung
- Time-Series Gorilla Compression: 10-20x Kompression
- Continuous Aggregates: Pre-computed hourly/daily stats
- LSM-Tree Write Buffer: 256 MB
- Block Cache: 4 GB
- CEP Window Size: 1-10 Minutes (konfigurierbar)

## Visualisierung

### Grafana Dashboards
1. **Übersichtskarte** - Züge in Echtzeit auf OSM
2. **Verspätungen** - Histogramm, Trends, Top 10
3. **Streckennetz** - Auslastung pro Strecke
4. **Effizienz** - KPIs, Pünktlichkeit, Energieverbrauch
5. **Anomalien** - CEP Alerts, Signalstörungen

### Kartendarstellung (Leaflet/MapLibre)
```html
<div id="railway-map"></div>
<script>
  const map = L.map('railway-map').setView([51.1657, 10.4515], 6);
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);
  
  // WebSocket für Live-Updates
  const ws = new WebSocket('ws://localhost:8765/ws/trains');
  ws.onmessage = (event) => {
    const train = JSON.parse(event.data);
    updateTrainMarker(train.id, train.lat, train.lon, train.delay_min);
  };
</script>
```

## Nächste Schritte

1. ✅ Datenmodell definiert
2. ⏳ Fake-Daten Generator implementieren
3. ⏳ CEP Rules konfigurieren
4. ⏳ LLM Adapter für Ollama
5. ⏳ REST API Endpoints
6. ⏳ Grafana Dashboards
7. ⏳ Dokumentation & Deployment Guide

## Referenzen

- ThemisDB Time-Series: `docs/features/features_time_series.md`
- ThemisDB CEP: `docs/analytics/CEP_STREAMING_ANALYTICS.md`
- ThemisDB LLM: `include/llm/llm_interaction_store.h`
- Deutsche Bahn Open Data: https://data.deutschebahn.com/
- OpenStreetMap Railway: https://wiki.openstreetmap.org/wiki/Railways
