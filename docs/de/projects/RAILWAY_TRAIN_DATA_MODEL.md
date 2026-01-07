# Vollständiges Zugdatenmodell für Railway Monitoring System

## Übersicht

Dieses Dokument beschreibt das detaillierte Datenmodell für Züge im Railway Monitoring System, das auf ThemisDB basiert.

## 1. Zug-Stammdaten (Document DB)

### 1.1 Zug-Service (Fahrt)

```json
{
  "_key": "service:ICE_508_2024_12_13",
  "type": "train_service",
  
  "service_info": {
    "train_number": "ICE 508",
    "service_id": "ICE_508_2024_12_13",
    "operating_date": "2024-12-13",
    "category": "ICE",
    "line_number": "25",
    "product_class": "high_speed"
  },
  
  "operator": {
    "company": "DB Fernverkehr AG",
    "division": "Personenverkehr",
    "business_unit": "Hochgeschwindigkeit"
  },
  
  "schedule": {
    "origin": {
      "station_id": "8000261",
      "station_name": "München Hbf",
      "platform": "11",
      "scheduled_departure": "2024-12-13T06:00:00Z",
      "actual_departure": "2024-12-13T06:02:00Z"
    },
    "destination": {
      "station_id": "8011160",
      "station_name": "Hamburg-Altona",
      "platform": "7",
      "scheduled_arrival": "2024-12-13T12:45:00Z",
      "estimated_arrival": "2024-12-13T12:48:00Z"
    },
    "stops": [
      {
        "sequence": 1,
        "station_id": "8000026",
        "station_name": "Augsburg Hbf",
        "platform": "5",
        "scheduled_arrival": "2024-12-13T06:30:00Z",
        "scheduled_departure": "2024-12-13T06:32:00Z",
        "actual_arrival": "2024-12-13T06:32:00Z",
        "actual_departure": "2024-12-13T06:34:00Z",
        "stop_type": "commercial",
        "stopping_time_sec": 120,
        "passenger_exchange": true
      },
      {
        "sequence": 2,
        "station_id": "8000105",
        "station_name": "Frankfurt(Main)Hbf",
        "platform": "7",
        "scheduled_arrival": "2024-12-13T08:43:00Z",
        "scheduled_departure": "2024-12-13T08:48:00Z",
        "stop_type": "commercial",
        "stopping_time_sec": 300
      }
    ],
    "route_distance_km": 782,
    "scheduled_duration_min": 405,
    "planned_running_time_min": 365,
    "buffer_time_min": 40
  },
  
  "rolling_stock_assignment": {
    "formation_id": "FORMATION_ICE3_4658",
    "train_set_number": "4658",
    "consist_length_m": 200.8,
    "consist_weight_tons": 435,
    "number_of_cars": 8,
    "traction_type": "electric_multiple_unit"
  },
  
  "commercial_data": {
    "booking_class": "standard",
    "reservation_required": true,
    "bicycle_transport": true,
    "supplements_required": false,
    "on_board_services": {
      "restaurant": true,
      "bistro": false,
      "wifi": true,
      "power_outlets": true,
      "air_conditioning": true
    }
  },
  
  "operational_status": {
    "status": "running",
    "current_position": {
      "track": "3600",
      "km": 45.3,
      "lat": 50.0567,
      "lon": 8.6123
    },
    "current_delay_min": 3,
    "delay_reason_code": "signal_failure",
    "next_station": "8000105",
    "eta_next_station": "2024-12-13T08:46:00Z"
  }
}
```

### 1.2 Rollmaterial-Stammdaten (Fahrzeug-Formation)

```json
{
  "_key": "formation:FORMATION_ICE3_4658",
  "type": "rolling_stock_formation",
  
  "formation_info": {
    "formation_id": "FORMATION_ICE3_4658",
    "train_set_number": "4658",
    "series": "ICE 3",
    "variant": "403",
    "manufacturer": "Siemens/Bombardier",
    "build_year": 2015,
    "owner": "DB Fernverkehr AG",
    "home_depot": "München Pasing"
  },
  
  "technical_data": {
    "traction_type": "EMU",
    "power_system": "15kV_16.7Hz_AC",
    "max_power_kw": 8000,
    "max_speed_kmh": 330,
    "operational_speed_kmh": 300,
    "length_over_buffers_mm": 200800,
    "width_mm": 2950,
    "height_mm": 3890,
    "empty_weight_tons": 410,
    "max_weight_tons": 454,
    "axle_count": 16,
    "bogie_count": 8,
    "wheel_diameter_mm": 920
  },
  
  "braking_system": {
    "brake_type": "electro_pneumatic",
    "eddy_current_brake": true,
    "magnetic_track_brake": true,
    "emergency_brake_deceleration_mps2": 1.2,
    "service_brake_deceleration_mps2": 0.9,
    "braking_distance_from_200kmh_m": 1800
  },
  
  "cars": [
    {
      "car_number": 1,
      "uic_number": "93 80 3 403 658-6 D-DB",
      "car_type": "driving_trailer_first_class",
      "position_in_formation": 1,
      "length_m": 25.1,
      "weight_tons": 52,
      "powered": true,
      "traction_motors": 2,
      "motor_power_kw": 1000,
      "pantograph": true,
      "class": "first",
      "seats": {
        "total": 41,
        "window": 28,
        "aisle": 13,
        "reduced_mobility": 2,
        "family_compartment": 0
      },
      "amenities": {
        "power_outlets": true,
        "wifi": true,
        "reading_lights": true,
        "luggage_racks": true,
        "coat_hooks": true
      }
    },
    {
      "car_number": 2,
      "uic_number": "93 80 3 403 658-7 D-DB",
      "car_type": "first_class_coach",
      "position_in_formation": 2,
      "length_m": 25.1,
      "weight_tons": 49,
      "powered": true,
      "traction_motors": 2,
      "class": "first",
      "seats": {
        "total": 52,
        "window": 36,
        "aisle": 16,
        "reduced_mobility": 2
      }
    },
    {
      "car_number": 3,
      "uic_number": "93 80 3 403 658-8 D-DB",
      "car_type": "restaurant_car",
      "position_in_formation": 3,
      "length_m": 25.1,
      "weight_tons": 51,
      "powered": true,
      "class": "mixed",
      "seats": {
        "total": 26,
        "restaurant": 18,
        "lounge": 8
      },
      "galley": {
        "equipped": true,
        "capacity": "full_service",
        "kitchen_staff": 3
      }
    },
    {
      "car_number": 4,
      "uic_number": "93 80 3 403 658-9 D-DB",
      "car_type": "second_class_coach",
      "position_in_formation": 4,
      "length_m": 25.1,
      "weight_tons": 48,
      "powered": true,
      "class": "second",
      "seats": {
        "total": 71,
        "window": 48,
        "aisle": 23,
        "reduced_mobility": 0
      }
    },
    {
      "car_number": 5,
      "uic_number": "93 80 3 403 658-0 D-DB",
      "car_type": "second_class_coach",
      "position_in_formation": 5,
      "length_m": 25.1,
      "weight_tons": 48,
      "powered": true,
      "class": "second",
      "seats": {
        "total": 71
      }
    },
    {
      "car_number": 6,
      "uic_number": "93 80 3 403 658-1 D-DB",
      "car_type": "second_class_coach",
      "position_in_formation": 6,
      "length_m": 25.1,
      "weight_tons": 48,
      "powered": true,
      "class": "second",
      "seats": {
        "total": 71
      }
    },
    {
      "car_number": 7,
      "uic_number": "93 80 3 403 658-2 D-DB",
      "car_type": "second_class_coach",
      "position_in_formation": 7,
      "length_m": 25.1,
      "weight_tons": 48,
      "powered": true,
      "class": "second",
      "seats": {
        "total": 71
      }
    },
    {
      "car_number": 8,
      "uic_number": "93 80 3 403 658-3 D-DB",
      "car_type": "driving_trailer_second_class",
      "position_in_formation": 8,
      "length_m": 25.1,
      "weight_tons": 52,
      "powered": true,
      "traction_motors": 2,
      "pantograph": true,
      "class": "second",
      "seats": {
        "total": 52
      }
    }
  ],
  
  "total_capacity": {
    "total_seats": 455,
    "first_class_seats": 93,
    "second_class_seats": 336,
    "restaurant_seats": 26,
    "wheelchair_spaces": 4,
    "bicycle_spaces": 8,
    "max_passengers": 550,
    "standing_capacity": 95
  },
  
  "maintenance_data": {
    "last_inspection_date": "2024-12-01",
    "next_inspection_date": "2025-01-15",
    "inspection_type": "L3",
    "total_km_run": 2345678,
    "km_since_last_revision": 15234,
    "next_revision_km": 50000,
    "maintenance_status": "operational",
    "known_issues": []
  },
  
  "safety_systems": {
    "etcs": {
      "equipped": true,
      "level": "L2",
      "baseline": "3.6.0"
    },
    "pzb": {
      "equipped": true,
      "version": "PZB90"
    },
    "lzb": {
      "equipped": true,
      "version": "LZB80E"
    },
    "sifa": true,
    "gsmr": true,
    "fire_detection": true,
    "emergency_brakes": 8,
    "emergency_exits": 16
  }
}
```

### 1.3 Einzelfahrzeug (Wagen)

```json
{
  "_key": "vehicle:93_80_3_403_658-6",
  "type": "railway_vehicle",
  
  "vehicle_identification": {
    "uic_number": "93 80 3 403 658-6 D-DB",
    "vehicle_number": "403 658-6",
    "owner_country": "D",
    "owner_code": "DB",
    "vehicle_keeper": "DB Fernverkehr AG",
    "series": "403",
    "unit_number": "658",
    "car_position": 6
  },
  
  "technical_specifications": {
    "vehicle_type": "power_car",
    "carbody_material": "aluminum",
    "length_over_buffers_mm": 25100,
    "width_mm": 2950,
    "height_mm": 3890,
    "floor_height_mm": 1250,
    "tare_weight_kg": 48000,
    "max_load_kg": 6000,
    "max_axle_load_kg": 13500
  },
  
  "bogie_data": [
    {
      "bogie_number": 1,
      "bogie_type": "MD523",
      "wheelbase_mm": 2500,
      "axle_1": {
        "wheel_set_id": "WS_658_6_1_L",
        "wheel_diameter_mm": 920,
        "wear_mm": 2.3,
        "profile_status": "good"
      },
      "axle_2": {
        "wheel_set_id": "WS_658_6_1_R",
        "wheel_diameter_mm": 920,
        "wear_mm": 2.1
      },
      "traction_motor": {
        "motor_id": "TM_658_6_1",
        "motor_type": "asynchronous_3phase",
        "power_kw": 1000,
        "voltage_v": 1500,
        "operating_hours": 12345
      },
      "brake_equipment": {
        "brake_type": "disc",
        "disc_count": 4,
        "pad_wear_percent": 35
      }
    },
    {
      "bogie_number": 2,
      "bogie_type": "MD523",
      "axle_1": {
        "wheel_set_id": "WS_658_6_2_L",
        "wheel_diameter_mm": 919
      },
      "axle_2": {
        "wheel_set_id": "WS_658_6_2_R",
        "wheel_diameter_mm": 920
      },
      "traction_motor": {
        "motor_id": "TM_658_6_2",
        "power_kw": 1000
      }
    }
  ],
  
  "equipment": {
    "hvac": {
      "system_type": "climate_control",
      "cooling_capacity_kw": 45,
      "heating_capacity_kw": 60,
      "air_filters": "HEPA"
    },
    "doors": {
      "door_count": 8,
      "door_type": "plug_sliding",
      "door_width_mm": 1300,
      "automatic_control": true,
      "emergency_release": true
    },
    "lighting": {
      "type": "LED",
      "emergency_lighting": true,
      "energy_consumption_w": 1200
    },
    "passenger_information": {
      "lcd_displays": 4,
      "audio_system": true,
      "emergency_intercom": 2
    }
  },
  
  "maintenance_history": [
    {
      "date": "2024-12-01",
      "type": "L3_inspection",
      "location": "München Pasing Werk",
      "duration_hours": 8,
      "findings": "OK",
      "work_performed": [
        "Visual inspection",
        "Brake test",
        "Door function test",
        "HVAC service"
      ]
    },
    {
      "date": "2024-10-15",
      "type": "wheel_reprofiling",
      "location": "München Pasing",
      "wheels_reprofiled": 4
    }
  ],
  
  "current_status": {
    "operational_status": "in_service",
    "current_formation": "FORMATION_ICE3_4658",
    "current_service": "ICE_508_2024_12_13",
    "location": "on_track_3600_km_45.3",
    "faults": []
  }
}
```

### 1.4 Zugpersonal-Zuordnung

```json
{
  "_key": "crew:ICE_508_2024_12_13",
  "type": "train_crew_assignment",
  
  "service_id": "ICE_508_2024_12_13",
  "train_number": "ICE 508",
  "operating_date": "2024-12-13",
  
  "driver_crew": [
    {
      "role": "train_driver",
      "employee_id": "EMP_12345",
      "name": "Max Mustermann",
      "license": "ICE_all_lines",
      "license_valid_until": "2026-12-31",
      "boarding_station": "8000261",
      "alighting_station": "8000105",
      "relief_at": "8000105",
      "shift_start": "2024-12-13T05:30:00Z",
      "shift_end": "2024-12-13T09:15:00Z"
    },
    {
      "role": "train_driver",
      "employee_id": "EMP_23456",
      "name": "Anna Schmidt",
      "boarding_station": "8000105",
      "alighting_station": "8011160",
      "shift_start": "2024-12-13T08:45:00Z",
      "shift_end": "2024-12-13T13:00:00Z"
    }
  ],
  
  "conductor_crew": [
    {
      "role": "chief_conductor",
      "employee_id": "EMP_34567",
      "name": "Peter Müller",
      "sector": "entire_train",
      "shift_start": "2024-12-13T05:30:00Z"
    },
    {
      "role": "conductor",
      "employee_id": "EMP_45678",
      "name": "Lisa Weber",
      "sector": "cars_1-4",
      "shift_start": "2024-12-13T05:30:00Z"
    },
    {
      "role": "conductor",
      "employee_id": "EMP_56789",
      "name": "Thomas Klein",
      "sector": "cars_5-8",
      "shift_start": "2024-12-13T05:30:00Z"
    }
  ],
  
  "service_crew": [
    {
      "role": "restaurant_manager",
      "employee_id": "EMP_67890",
      "assigned_car": 3
    },
    {
      "role": "chef",
      "employee_id": "EMP_78901",
      "assigned_car": 3
    },
    {
      "role": "waiter",
      "employee_id": "EMP_89012",
      "assigned_car": 3
    }
  ],
  
  "total_crew_count": 8,
  "crew_status": "complete",
  "emergency_contact": "+49 89 1234567"
}
```

### 1.5 Ladung / Fracht (für Güterzüge)

```json
{
  "_key": "cargo:GZ_62345_2024_12_13",
  "type": "cargo_manifest",
  
  "service_id": "GZ_62345_2024_12_13",
  "train_number": "GZ 62345",
  "train_type": "freight",
  
  "cargo_summary": {
    "total_weight_tons": 1850,
    "total_wagons": 25,
    "dangerous_goods": true,
    "total_teu": 0
  },
  
  "wagons": [
    {
      "wagon_position": 1,
      "wagon_id": "31 80 6650 123-4",
      "wagon_type": "tank_car",
      "load": {
        "cargo_type": "chemicals",
        "description": "Diesel fuel",
        "weight_tons": 60,
        "dangerous_goods": {
          "un_number": "UN1202",
          "class": "3",
          "packing_group": "III",
          "hazard_label": "flammable_liquid"
        }
      },
      "shipper": "Shell Deutschland",
      "consignee": "Aral Station Hamburg",
      "origin_station": "8000261",
      "destination_station": "8011160"
    },
    {
      "wagon_position": 2,
      "wagon_id": "31 80 6773 456-7",
      "wagon_type": "covered_goods_wagon",
      "load": {
        "cargo_type": "general_cargo",
        "description": "Auto parts",
        "weight_tons": 45
      }
    }
  ],
  
  "routing": {
    "origin": "München Rbf",
    "destination": "Hamburg-Wilhelmsburg",
    "via": ["Augsburg", "Nürnberg", "Fulda", "Kassel", "Hannover"]
  }
}
```

## 2. Echtzeit-Daten (Time-Series)

Siehe Hauptdokumentation für:
- GPS Telemetrie
- Fahrzeug-Systeme
- Safety Systems
- Passenger Information

## 3. Graph-Beziehungen

```
(Service) -[USES_FORMATION]-> (Formation)
(Formation) -[CONTAINS]-> (Vehicle)
(Service) -[ASSIGNED_CREW]-> (Crew)
(Service) -[FOLLOWS_ROUTE]-> (Schedule)
(Vehicle) -[LOCATED_AT]-> (Track_Segment)
(Service) -[CURRENTLY_AT]-> (Station)
```

## Zusammenfassung

Dieses vollständige Datenmodell ermöglicht:

1. **Asset Management** - Komplette Fahrzeugverwaltung
2. **Operations** - Echtzeitüberwachung aller Zugfahrten
3. **Maintenance** - Wartungsplanung und -historie
4. **Safety** - Lückenlose Dokumentation für Sicherheit
5. **Commercial** - Kapazitätsplanung und Auslastung
6. **Compliance** - Erfüllung aller regulatorischen Anforderungen
