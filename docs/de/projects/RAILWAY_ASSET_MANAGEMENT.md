# DB Asset Management - Lokomotiven & Waggons

## Übersicht

Vollständiges Asset-Management-System für Rollmaterial der Deutschen Bahn mit realistischen Bestandszahlen, Zustandsüberwachung und Wartungsplanung.

## Realistische Bestandszahlen Deutsche Bahn (2023)

### Gesamtbestand Rollmaterial

```
TRIEBFAHRZEUGE (Angetrieben):
├── Elektrische Triebzüge:      ~2.800 Einheiten
│   ├── ICE-Flotte:               ~360 Züge
│   │   ├── ICE 1 (401):           60 Züge (1991-1993)
│   │   ├── ICE 2 (402):           44 Züge (1996-1998)
│   │   ├── ICE 3 (403/406):      67 Züge (1999-2008)
│   │   ├── ICE 4 (412):          100 Züge (2017-2023)
│   │   ├── ICE-T (411/415):       71 Züge (1999-2006)
│   │   └── ICE-TD (605):          18 Züge (Diesel, 2001-2003)
│   ├── IC-Triebzüge:              ~50 Einheiten
│   ├── RE-Triebzüge:             ~800 Einheiten
│   ├── S-Bahn-Triebzüge:       ~1.200 Einheiten
│   └── Regional (Talent, etc):   ~390 Einheiten
│
├── Elektrische Lokomotiven:    ~1.800 Einheiten
│   ├── BR 101 (IC/EC):            145 Stück (1996-1999)
│   ├── BR 103 (ICE):               45 Stück (Museumsloks)
│   ├── BR 120/127:                 60 Stück (Regional)
│   ├── BR 143-147 (S-Bahn):       400 Stück
│   ├── BR 152 (Güterverkehr):     170 Stück (2000-)
│   ├── BR 185-189 (Güter):        550 Stück
│   └── Sonstige:                  430 Stück
│
├── Diesel-Lokomotiven:           ~750 Einheiten
│   ├── BR 218 (IC/RE):            100 Stück
│   ├── BR 232-234 (Güter):        250 Stück
│   └── Sonstige:                  400 Stück
│
└── Dieseltriebwagen:             ~500 Einheiten
    ├── BR 612 (RegioSwinger):     192 Stück
    ├── BR 628:                     150 Stück
    └── Sonstige:                   158 Stück

WAGEN (Angehängt):
├── Personenwagen:               ~7.500 Wagen
│   ├── IC/EC Wagen (Avmz):      ~800 Wagen
│   ├── Regionalwagen:          ~3.200 Wagen
│   ├── S-Bahn Wagen:           ~2.500 Wagen
│   └── Sonderwagen:            ~1.000 Wagen
│
└── Güterwagen:                 ~58.000 Wagen
    ├── Rungenwagen:             12.000 Wagen
    ├── Flachwagen:               8.500 Wagen
    ├── Kesselwagen:              6.200 Wagen
    ├── Güterwagen gedeckt:      18.000 Wagen
    ├── Containertragwagen:       8.300 Wagen
    └── Sonstige:                 5.000 Wagen

GESAMT: ~71.350 Fahrzeuge
```

### Verfügbarkeit & Zustand

```
STATUS-VERTEILUNG (Durchschnitt):
├── In Betrieb (verfügbar):      68% (~48.500 Fahrzeuge)
├── In Wartung (geplant):        18% (~12.850 Fahrzeuge)
├── In Reparatur (ungeplant):     8% (~5.700 Fahrzeuge)
├── Außer Betrieb (kaputt):       4% (~2.850 Fahrzeuge)
└── Reserveflotte:                2% (~1.450 Fahrzeuge)

VERFÜGBARKEIT NACH TYP:
├── ICE-Flotte:                  87,3% (Ziel: >90%)
│   └── Durchschnittlich nicht verfügbar: 46 Züge
├── IC-Lokomotiven:              82,5%
├── Regional-Triebzüge:          91,2%
├── S-Bahn:                      93,5%
└── Güterlokomotiven:            78,4%

WARTUNGSINTERVALLE:
├── Tägliche Inspektion (L1):     Alle Züge
├── Kleine Wartung (L2):          Alle 7 Tage
├── Große Wartung (L3):           Alle 30 Tage
├── Hauptuntersuchung (L4):       Alle 6 Monate
└── Schwere Revision (L5):        Alle 3-5 Jahre
```

### Wartungs-Kapazitäten

```
WERKE & KAPAZITÄTEN:
├── ICE-Werke:                    5 Standorte
│   ├── München-Pasing:          Kapazität: 15 Züge gleichzeitig
│   ├── Hamburg-Eidelstedt:      Kapazität: 12 Züge
│   ├── Frankfurt-Griesheim:     Kapazität: 18 Züge
│   ├── Dortmund-Betriebswerk:   Kapazität: 10 Züge
│   └── Berlin-Rummelsburg:      Kapazität: 14 Züge
│
├── Regional-Werke:               ~45 Standorte
│   └── Durchschnitt:            Kapazität: 25 Fahrzeuge
│
└── Güterverkehr-Werke:          ~28 Standorte
    └── Durchschnitt:            Kapazität: 40 Loks/Wagen

WARTUNGS-PERSONAL:
├── Techniker Gesamt:            ~12.000 Mitarbeiter
│   ├── ICE-Werke:                ~1.800 Techniker
│   ├── Regional:                 ~6.200 Techniker
│   └── Güterverkehr:             ~4.000 Techniker
│
├── Durchsatz pro Werk/Tag:
│   ├── L1 (täglich):            50-100 Fahrzeuge
│   ├── L2 (wöchentlich):        15-25 Fahrzeuge
│   ├── L3 (monatlich):          8-12 Fahrzeuge
│   └── L4 (halbjährlich):       2-4 Fahrzeuge

WARTUNGSKOSTEN (2023):
├── Gesamtkosten:                ~2,5 Mrd. EUR/Jahr
│   ├── Personal:                ~1,2 Mrd. EUR
│   ├── Ersatzteile:             ~900 Mio. EUR
│   └── Infrastruktur:           ~400 Mio. EUR
│
└── Pro Fahrzeugtyp/Jahr:
    ├── ICE:                     ~1,8 Mio. EUR/Zug
    ├── IC-Lok:                  ~450.000 EUR/Lok
    ├── Regional-Triebzug:       ~280.000 EUR
    └── S-Bahn:                  ~180.000 EUR
```

## Datenmodell: Rollmaterial-Bestand

### Fahrzeug-Stammdaten

```json
{
  "_key": "vehicle:403_658_6",
  "type": "railway_vehicle",
  
  "vehicle_identification": {
    "uic_number": "93 80 3 403 658-6 D-DB",
    "evn": "91 80 6403 658-6",
    "vehicle_number": "403 658-6",
    "series": "ICE 3",
    "variant": "403",
    "unit_number": 658,
    "car_position": 6,
    "owner": "DB Fernverkehr AG",
    "operator": "DB Fernverkehr AG",
    "manufacturer": "Siemens/Bombardier",
    "build_year": 2015,
    "commissioning_date": "2015-06-15"
  },
  
  "current_status": {
    "operational_status": "in_service",
    "availability": "available",
    "location": {
      "type": "in_formation",
      "formation_id": "FORMATION_ICE3_4658",
      "current_service": "ICE_508_2024_12_13"
    },
    "condition": "good",
    "total_km_run": 2345678,
    "km_since_last_revision": 15234,
    "operating_hours": 45678,
    "last_update": "2024-12-13T15:42:18Z"
  },
  
  "maintenance_data": {
    "last_l1_inspection": "2024-12-13T06:00:00Z",
    "last_l2_service": "2024-12-10T14:00:00Z",
    "last_l3_service": "2024-11-15T08:00:00Z",
    "last_l4_inspection": "2024-08-20T09:00:00Z",
    "last_l5_revision": "2022-03-15T10:00:00Z",
    
    "next_l2_due": "2024-12-17T14:00:00Z",
    "next_l3_due": "2024-12-15T08:00:00Z",
    "next_l4_due": "2025-02-20T09:00:00Z",
    "next_l5_due": "2027-03-15T10:00:00Z",
    
    "maintenance_status": "ok",
    "overdue_services": []
  },
  
  "defects": {
    "critical_faults": 0,
    "minor_faults": 2,
    "warnings": 1,
    "fault_list": [
      {
        "fault_id": "F_2024_12_003",
        "severity": "minor",
        "component": "door_2",
        "description": "Türschließung verzögert",
        "detected": "2024-12-12T18:30:00Z",
        "repair_scheduled": "2024-12-15T10:00:00Z"
      }
    ]
  },
  
  "lifetime_data": {
    "total_km_run": 2345678,
    "total_operating_hours": 45678,
    "total_passengers_transported": 8500000,
    "total_energy_consumed_mwh": 5867.5,
    "co2_saved_vs_car_tons": 12500,
    "design_lifetime_km": 6000000,
    "remaining_lifetime_percent": 60.9
  }
}
```

### Wartungs-Auftrag

```json
{
  "_key": "maintenance:MAINT_2024_12_0123",
  "type": "maintenance_order",
  
  "order_info": {
    "order_id": "MAINT_2024_12_0123",
    "order_type": "L3",
    "priority": "normal",
    "status": "scheduled",
    "created": "2024-12-01T10:00:00Z",
    "scheduled_start": "2024-12-15T08:00:00Z",
    "scheduled_end": "2024-12-15T16:00:00Z",
    "estimated_duration_hours": 8
  },
  
  "vehicle": {
    "uic_number": "93 80 3 403 658-6 D-DB",
    "vehicle_type": "ICE 3",
    "current_location": "München Hbf",
    "transfer_required": true,
    "transfer_km": 12.5
  },
  
  "workshop": {
    "workshop_id": "WERK_MUC_PASING",
    "name": "ICE-Werk München-Pasing",
    "capacity_current": 12,
    "capacity_max": 15,
    "technicians_assigned": 4
  },
  
  "work_packages": [
    {
      "package_id": "WP_001",
      "description": "Bremsprüfung",
      "duration_hours": 2.5,
      "technicians_required": 2,
      "parts_required": ["brake_pads_front", "brake_fluid"]
    },
    {
      "package_id": "WP_002",
      "description": "HVAC Wartung",
      "duration_hours": 1.5,
      "technicians_required": 1
    }
  ],
  
  "parts_required": [
    {
      "part_number": "SIE_BRK_PAD_001",
      "description": "Bremsbeläge Vorne",
      "quantity": 8,
      "unit_price_eur": 125.50,
      "availability": "in_stock",
      "warehouse": "MUC_PASING_MAG"
    }
  ],
  
  "costs": {
    "labor_cost_eur": 1200,
    "parts_cost_eur": 1804,
    "total_cost_eur": 3004,
    "downtime_cost_eur": 8500
  }
}
```

### Werkstatt (Maintenance Facility)

```json
{
  "_key": "workshop:WERK_MUC_PASING",
  "type": "maintenance_workshop",
  
  "facility_info": {
    "workshop_id": "WERK_MUC_PASING",
    "name": "ICE-Werk München-Pasing",
    "operator": "DB Fahrzeuginstandhaltung GmbH",
    "location": {
      "address": "Friedenheimer Str. 24, 80686 München",
      "lat": 48.1455,
      "lon": 11.5014
    },
    "operational_since": "1991-06-01"
  },
  
  "capacity": {
    "service_bays": 15,
    "current_occupancy": 12,
    "utilization_percent": 80.0,
    "vehicle_types_supported": ["ICE 1", "ICE 2", "ICE 3", "ICE 4", "ICE-T"],
    "max_daily_throughput": 8
  },
  
  "capabilities": {
    "service_levels": ["L1", "L2", "L3", "L4", "L5"],
    "specializations": [
      "Triebkopf-Revision",
      "Drehgestell-Tausch",
      "Bremssystem-Überholung",
      "Klimaanlagen-Wartung",
      "Innenausbau-Erneuerung"
    ]
  },
  
  "staff": {
    "total_employees": 385,
    "technicians": 280,
    "engineers": 45,
    "management": 15,
    "admin": 45,
    
    "shifts": {
      "shift_1": "06:00-14:00",
      "shift_2": "14:00-22:00",
      "shift_3": "22:00-06:00"
    },
    
    "availability": {
      "current_shift": "shift_1",
      "technicians_on_duty": 95,
      "technicians_available": 23
    }
  },
  
  "equipment": {
    "lifting_equipment": 8,
    "wheel_lathes": 4,
    "paint_booths": 2,
    "diagnostic_stations": 12,
    "parts_warehouse_sqm": 4500
  },
  
  "performance": {
    "avg_l3_duration_hours": 7.2,
    "avg_l4_duration_days": 3.5,
    "vehicles_serviced_month": 245,
    "on_time_completion_rate": 0.892,
    "quality_score": 0.953
  }
}
```

## Materialwirtschaft

### Ersatzteile-Lager

```json
{
  "_key": "warehouse:MUC_PASING_MAG",
  "type": "parts_warehouse",
  
  "warehouse_info": {
    "warehouse_id": "MUC_PASING_MAG",
    "name": "Ersatzteillager München-Pasing",
    "location": "München-Pasing",
    "size_sqm": 4500,
    "storage_locations": 8500
  },
  
  "inventory": {
    "total_items": 12500,
    "total_value_eur": 8500000,
    "categories": {
      "brake_systems": 1250,
      "electrical_components": 2800,
      "hvac_parts": 890,
      "interior_parts": 1500,
      "mechanical_parts": 3200,
      "safety_equipment": 450,
      "consumables": 2410
    }
  },
  
  "top_parts": [
    {
      "part_number": "SIE_BRK_PAD_001",
      "description": "Bremsbeläge ICE 3 Vorne",
      "quantity_in_stock": 850,
      "reorder_level": 200,
      "monthly_consumption": 145,
      "unit_price_eur": 125.50,
      "supplier": "Siemens Mobility",
      "lead_time_days": 14,
      "criticality": "high"
    }
  ],
  
  "logistics": {
    "avg_order_fulfillment_hours": 2.5,
    "emergency_delivery_available": true,
    "daily_movements": 320,
    "accuracy_rate": 0.997
  }
}
```

### Ersatzteil-Bestellung

```json
{
  "_key": "parts_order:PO_2024_12_0456",
  "type": "parts_order",
  
  "order_info": {
    "order_id": "PO_2024_12_0456",
    "order_date": "2024-12-10T14:30:00Z",
    "delivery_date_requested": "2024-12-24T08:00:00Z",
    "delivery_date_confirmed": "2024-12-22T10:00:00Z",
    "status": "in_transit",
    "priority": "normal"
  },
  
  "supplier": {
    "supplier_id": "SUP_SIEMENS_001",
    "name": "Siemens Mobility GmbH",
    "contact": "parts@siemens-mobility.com"
  },
  
  "items": [
    {
      "part_number": "SIE_BRK_PAD_001",
      "quantity": 500,
      "unit_price_eur": 125.50,
      "total_price_eur": 62750
    }
  ],
  
  "costs": {
    "parts_total_eur": 62750,
    "shipping_eur": 450,
    "tax_eur": 12030,
    "total_eur": 75230
  },
  
  "tracking": {
    "carrier": "DB Schenker",
    "tracking_number": "DBK12345678",
    "current_location": "Nürnberg Logistikzentrum",
    "estimated_arrival": "2024-12-22T10:00:00Z"
  }
}
```

## Personal-Management

### Techniker-Profil

```json
{
  "_key": "technician:TECH_12345",
  "type": "maintenance_technician",
  
  "personal_info": {
    "employee_id": "TECH_12345",
    "name": "Max Mustermann",
    "date_of_birth": "1985-05-15",
    "hire_date": "2010-08-01",
    "employment_years": 14.3
  },
  
  "job_info": {
    "position": "Facharbeiter Schienenfahrzeugtechnik",
    "grade": "E8",
    "home_workshop": "WERK_MUC_PASING",
    "supervisor": "TECH_67890",
    "shift": "shift_1"
  },
  
  "qualifications": {
    "apprenticeship": "Mechatroniker",
    "certifications": [
      {
        "cert_id": "ICE3_BRAKE_CERT",
        "name": "ICE 3 Bremssystem-Spezialist",
        "issued": "2018-03-15",
        "valid_until": "2025-03-15",
        "issuer": "DB Training"
      },
      {
        "cert_id": "SAFETY_L4",
        "name": "Sicherheitsbeauftragter Stufe 4",
        "issued": "2022-01-10",
        "valid_until": "2027-01-10"
      }
    ],
    "specializations": [
      "ICE 3 Triebkopf",
      "Bremssysteme",
      "Klimaanlagen"
    ],
    "vehicle_types_authorized": [
      "ICE 1", "ICE 2", "ICE 3", "ICE 4"
    ]
  },
  
  "performance": {
    "jobs_completed_year": 285,
    "avg_job_duration_vs_standard": 0.92,
    "quality_score": 0.96,
    "safety_incidents": 0,
    "training_hours_year": 45,
    "certifications_expiring_soon": 1
  },
  
  "availability": {
    "status": "available",
    "current_assignment": null,
    "hours_worked_this_week": 32.5,
    "overtime_hours_month": 8.5,
    "vacation_days_remaining": 18,
    "next_vacation": "2024-12-24 to 2025-01-06"
  }
}
```

### Schichtplanung Werkstatt

```json
{
  "_key": "shift_plan:WERK_MUC_PASING_2024W50",
  "type": "workshop_shift_plan",
  
  "planning_info": {
    "workshop_id": "WERK_MUC_PASING",
    "week": "2024-W50",
    "valid_from": "2024-12-09",
    "valid_to": "2024-12-15",
    "created_by": "MANAGER_567",
    "approved": true
  },
  
  "shifts": [
    {
      "date": "2024-12-09",
      "shift": "shift_1",
      "technicians_required": 95,
      "technicians_assigned": 97,
      "technicians": [
        {
          "employee_id": "TECH_12345",
          "specialization": "ICE 3 Bremssysteme",
          "assigned_bay": "Bay_05",
          "assigned_vehicles": ["403_658_6", "403_659_2"]
        }
      ],
      "workload": {
        "l1_inspections": 25,
        "l2_services": 8,
        "l3_services": 3,
        "unplanned_repairs": 2
      }
    }
  ],
  
  "coverage": {
    "total_technician_hours": 6840,
    "total_work_hours_required": 6520,
    "coverage_ratio": 1.05,
    "critical_skills_covered": true
  }
}
```

## Analysen & KPIs

### Flotten-Verfügbarkeit

```python
# Berechne Verfügbarkeit pro Fahrzeugtyp
def calculate_fleet_availability():
    """
    Verfügbarkeit = (Verfügbare Fahrzeuge) / (Gesamt-Flotte)
    """
    
    ice3_fleet = {
        "total": 67,
        "available": 58,
        "in_service": 52,
        "reserve": 6,
        "in_maintenance_planned": 7,
        "in_repair_unplanned": 2,
        "out_of_service": 0
    }
    
    availability = ice3_fleet["available"] / ice3_fleet["total"]
    # Result: 86.6%
    
    # Ziel: >90%
    shortfall = 0.90 - availability  # 3.4%
    vehicles_needed = ceil(ice3_fleet["total"] * shortfall)  # 3 Züge
    
    return {
        "availability_percent": availability * 100,
        "target_percent": 90.0,
        "shortfall_percent": shortfall * 100,
        "additional_vehicles_needed": vehicles_needed
    }
```

### Wartungs-Auslastung

```python
def analyze_workshop_utilization():
    """
    Werkstatt-Auslastung und Engpässe
    """
    
    workshop = {
        "capacity_bays": 15,
        "occupied_bays": 12,
        "avg_service_hours": 7.2,
        "technicians_total": 280,
        "technicians_on_shift": 95
    }
    
    # Aktuelle Auslastung
    bay_utilization = workshop["occupied_bays"] / workshop["capacity_bays"]
    # Result: 80%
    
    # Warteschlange
    pending_orders = 8  # Wartend auf freien Platz
    avg_wait_hours = 18.5
    
    # Optimierung
    if bay_utilization > 0.85:
        recommendation = "Kapazität erweitern oder Wartungen verschieben"
    elif pending_orders > 5:
        recommendation = "Zusätzliche Schicht oder Überstunden"
    else:
        recommendation = "Auslastung optimal"
    
    return {
        "utilization_percent": bay_utilization * 100,
        "pending_orders": pending_orders,
        "avg_wait_hours": avg_wait_hours,
        "recommendation": recommendation
    }
```

### Material-Bestandsoptimierung

```python
def optimize_parts_inventory():
    """
    ABC-Analyse für Ersatzteile
    """
    
    parts = [
        {"id": "BRK_PAD", "annual_consumption": 1740, "unit_price": 125.50, "criticality": "A"},
        {"id": "HVAC_FILTER", "annual_consumption": 3200, "unit_price": 15.80, "criticality": "B"},
        # ...
    ]
    
    for part in parts:
        annual_value = part["annual_consumption"] * part["unit_price"]
        
        # Bestellpunkt berechnen
        lead_time_days = 14
        safety_stock_days = 7
        daily_consumption = part["annual_consumption"] / 365
        
        reorder_point = (lead_time_days + safety_stock_days) * daily_consumption
        
        # Economic Order Quantity
        order_cost = 50  # EUR pro Bestellung
        holding_cost_percent = 0.20  # 20% des Werts
        
        eoq = sqrt((2 * part["annual_consumption"] * order_cost) / 
                   (part["unit_price"] * holding_cost_percent))
        
        part["reorder_point"] = round(reorder_point)
        part["order_quantity"] = round(eoq)
        part["annual_value"] = annual_value
    
    # Sortiere nach Wert (ABC-Analyse)
    parts_sorted = sorted(parts, key=lambda x: x["annual_value"], reverse=True)
    
    cumulative_value = 0
    total_value = sum(p["annual_value"] for p in parts)
    
    for part in parts_sorted:
        cumulative_value += part["annual_value"]
        cumulative_percent = cumulative_value / total_value
        
        if cumulative_percent <= 0.80:
            part["abc_class"] = "A"  # Top 80% Wert
        elif cumulative_percent <= 0.95:
            part["abc_class"] = "B"  # 80-95%
        else:
            part["abc_class"] = "C"  # Rest
    
    return parts_sorted
```

### Personal-Einsatzplanung

```python
def optimize_technician_assignment():
    """
    Optimale Techniker-Zuordnung basierend auf Qualifikation
    """
    
    pending_jobs = [
        {
            "vehicle": "ICE3_658",
            "service_type": "L3",
            "required_skills": ["ICE3_BRAKE", "ICE3_HVAC"],
            "estimated_hours": 7.2,
            "priority": "high"
        }
    ]
    
    available_technicians = [
        {
            "id": "TECH_12345",
            "skills": ["ICE3_BRAKE", "ICE3_HVAC", "ICE3_TRACTION"],
            "skill_level": {"ICE3_BRAKE": 0.95, "ICE3_HVAC": 0.88},
            "hours_available": 8.0
        }
    ]
    
    # Matching-Algorithmus
    assignments = []
    
    for job in sorted(pending_jobs, key=lambda x: x["priority"], reverse=True):
        best_match = None
        best_score = 0
        
        for tech in available_technicians:
            if tech["hours_available"] < job["estimated_hours"]:
                continue
            
            # Skill-Match Score
            matched_skills = set(job["required_skills"]) & set(tech["skills"])
            if len(matched_skills) < len(job["required_skills"]):
                continue  # Nicht qualifiziert
            
            skill_score = sum(tech["skill_level"].get(s, 0) 
                            for s in job["required_skills"])
            skill_score /= len(job["required_skills"])
            
            if skill_score > best_score:
                best_score = skill_score
                best_match = tech
        
        if best_match:
            assignments.append({
                "job": job,
                "technician": best_match["id"],
                "skill_match": best_score
            })
            best_match["hours_available"] -= job["estimated_hours"]
    
    return assignments
```

## Integration in Railway Monitoring

### Erweitertes Datenmodell

```python
# In train_simulator.py
class RollingStockManager:
    def __init__(self):
        self.fleet = self._load_fleet()
        self.workshops = self._load_workshops()
        self.parts_inventory = self._load_inventory()
        self.technicians = self._load_staff()
    
    def simulate_maintenance_cycle(self):
        """Simuliere Wartungszyklen für gesamte Flotte"""
        
        for vehicle in self.fleet:
            # Prüfe Wartungsintervalle
            if vehicle.km_since_l3 >= 30000:
                self._schedule_maintenance(vehicle, "L3")
            
            # Zufällige Ausfälle
            if random.random() < 0.001:  # 0.1% pro Update
                self._create_breakdown(vehicle)
    
    def _schedule_maintenance(self, vehicle, service_type):
        """Plane Wartung ein"""
        
        # Finde verfügbare Werkstatt
        workshop = self._find_available_workshop(vehicle.type)
        
        if not workshop:
            # Keine Kapazität → Warteschlange
            vehicle.status = "awaiting_maintenance"
            return
        
        # Erstelle Wartungsauftrag
        order = MaintenanceOrder(
            vehicle=vehicle,
            service_type=service_type,
            workshop=workshop,
            scheduled_start=datetime.now() + timedelta(days=2)
        )
        
        # Reserviere Kapazität
        workshop.reserve_bay(order)
        vehicle.status = "maintenance_scheduled"
    
    def calculate_fleet_availability(self):
        """Berechne Verfügbarkeit"""
        
        total = len(self.fleet)
        available = len([v for v in self.fleet if v.status == "available"])
        
        return {
            "total": total,
            "available": available,
            "availability_percent": (available / total) * 100,
            "in_maintenance": len([v for v in self.fleet if "maintenance" in v.status]),
            "broken": len([v for v in self.fleet if v.status == "broken"])
        }
```

## Gesamtanalyse DB (Dashboard)

### KPI-Übersicht

```
┌────────────────────────────────────────────────────────────────┐
│  Deutsche Bahn - Gesamtanalyse                                 │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  BETRIEBSFÜHRUNG:                                              │
│  ├── Züge im Einsatz:        1.234  (von 1.450 verfügbar)    │
│  ├── Pünktlichkeit:          87,3%  (Ziel: >90%)              │
│  ├── Ø Verspätung:           4,2 Min                          │
│  └── Ausfälle heute:         12 Züge                          │
│                                                                │
│  ROLLMATERIAL:                                                 │
│  ├── Gesamtbestand:          71.350 Fahrzeuge                 │
│  ├── Verfügbar:              48.500  (68,0%)                  │
│  ├── In Wartung:             12.850  (18,0%)                  │
│  ├── In Reparatur:            5.700  ( 8,0%)                  │
│  └── Außer Betrieb:           2.850  ( 4,0%)                  │
│                                                                │
│  WARTUNG:                                                      │
│  ├── Werkstätten:            78 Standorte                     │
│  ├── Auslastung:             82,5%  (Engpass!)                │
│  ├── Warteschlange:          145 Fahrzeuge                    │
│  ├── Ø Wartezeit:            2,3 Tage                         │
│  └── Kosten/Monat:           208 Mio. EUR                     │
│                                                                │
│  PERSONAL:                                                     │
│  ├── Techniker Gesamt:       12.000 Mitarbeiter               │
│  ├── Im Dienst (aktuell):    3.800  (Schicht 1)              │
│  ├── Auslastung:             89,5%                            │
│  ├── Überstunden/Monat:      18.500 Stunden                   │
│  └── Offene Stellen:         234 Positionen                   │
│                                                                │
│  MATERIALWIRTSCHAFT:                                           │
│  ├── Lagerwert:              825 Mio. EUR                     │
│  ├── Bestellungen offen:    1.234 (Wert: 45 Mio. EUR)        │
│  ├── Kritische Teile:        23 (unter Bestellpunkt)         │
│  └── Lieferzeit Ø:           18 Tage                          │
│                                                                │
│  ENERGIE:                                                      │
│  ├── Aktuelle Last:          687 MW                           │
│  ├── Tagesverbrauch:         14.250 MWh                       │
│  ├── Grünstrom-Anteil:       78,5%                            │
│  └── CO₂-Emission:           137 kg/MWh                       │
│                                                                │
│  FINANZEN:                                                     │
│  ├── Wartungskosten/Jahr:    2,5 Mrd. EUR                     │
│  ├── Energiekosten/Jahr:     1,8 Mrd. EUR                     │
│  ├── Personalkosten/Jahr:    4,2 Mrd. EUR                     │
│  └── Einsparungspotenzial:   122 Mio. EUR/Jahr                │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

## Nächste Schritte

1. ✅ Datenmodell definiert (Fahrzeuge, Wartung, Personal, Material)
2. ⏳ Realistische Bestandszahlen in Simulator integrieren
3. ⏳ Wartungssimulation (geplant + ungeplant)
4. ⏳ Personal-Einsatzplanung
5. ⏳ Materialwirtschaft (Bestellungen, Lager)
6. ⏳ Gesamtanalyse-Dashboard (WPF)
7. ⏳ Optimierungs-Algorithmen (Wartung, Personal, Kosten)

## Referenzen

- DB Geschäftsbericht 2023
- DB Fahrzeuginstandhaltung GmbH
- VDV-Statistik 2023
- Eisenbahn-Bundesamt Fahrzeugregister
