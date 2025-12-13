#!/usr/bin/env python3
"""
Real-time Train Simulator for Railway Monitoring System

Simuliert realistische Zugbewegungen basierend auf:
- Deutsche Bahn Fahrplan-Daten (ca. 40.000 Züge/Tag in Deutschland)
- Echte Geschwindigkeitsprofile
- Verspätungen nach realistischen Verteilungen
- IoT Sensordaten (GPS, Fahrzeug-Systeme, Strecken-Sensoren)

Daten-Quellen:
- DB Open Data Portal
- OpenStreetMap
- Simulierte Werte basierend auf realen Statistiken
"""

import json
import requests
import time
import random
import math
import sys
from datetime import datetime, timedelta
from typing import Dict, List, Tuple
import threading

class TrainSimulator:
    """Simuliert realistische Zugbewegungen"""
    
    # Realistische Zahlen basierend auf DB Statistiken 2023
    DAILY_TRAINS_GERMANY = 40000  # Gesamt pro Tag
    DAILY_ICE_TRAINS = 1200       # Fernverkehr ICE
    DAILY_IC_TRAINS = 800          # Fernverkehr IC/EC
    DAILY_RE_TRAINS = 8000         # Regionalverkehr RE
    DAILY_RB_TRAINS = 15000        # Regionalverkehr RB
    DAILY_FREIGHT_TRAINS = 5000    # Güterverkehr
    
    # Verspätungsstatistiken DB 2023
    # Pünktlichkeit (< 6 Min Verspätung): ICE 91.5%, RE/RB 94.2%
    PUNCTUALITY_ICE = 0.915
    PUNCTUALITY_RE = 0.942
    
    # Durchschnittliche Verspätung bei verspäteten Zügen
    AVG_DELAY_ICE_MIN = 12.3
    AVG_DELAY_RE_MIN = 8.5
    
    def __init__(self, themis_url: str = "http://localhost:8765"):
        self.themis_url = themis_url
        self.running = False
        self.trains = []
        self.track_network = {}
        self.current_time = datetime.now()
        
        # Statistiken
        self.stats = {
            "trains_active": 0,
            "updates_sent": 0,
            "avg_delay_min": 0.0,
            "delays_5min_plus": 0,
            "signals_changed": 0,
            "hotbox_alerts": 0
        }
    
    def load_network(self, network_file: str):
        """Lade Streckennetz-Daten"""
        print(f"Loading network data from {network_file}...")
        with open(network_file, 'r') as f:
            data = json.load(f)
        
        self.track_network = {
            "stations": {s['eva_number']: s for s in data.get('stations', [])},
            "segments": data.get('track_segments', []),
            "signals": {sig['signal_id']: sig for sig in data.get('signals', [])},
            "switches": {sw['switch_id']: sw for sw in data.get('switches', [])}
        }
        
        print(f"✓ Loaded {len(self.track_network['stations'])} stations")
        print(f"✓ Loaded {len(self.track_network['segments'])} track segments")
        print(f"✓ Loaded {len(self.track_network['signals'])} signals")
    
    def generate_train_schedule(self, num_trains: int = 100):
        """Generiere realistischen Fahrplan"""
        print(f"\nGenerating schedule for {num_trains} trains...")
        
        stations = list(self.track_network['stations'].values())
        if len(stations) < 2:
            print("ERROR: Need at least 2 stations!")
            return
        
        # Verteilung nach Zugtypen (basierend auf realen Zahlen)
        ice_count = int(num_trains * 0.15)   # 15% ICE
        ic_count = int(num_trains * 0.10)    # 10% IC
        re_count = int(num_trains * 0.35)    # 35% RE
        rb_count = int(num_trains * 0.40)    # 40% RB
        
        train_id = 1
        
        # ICE Züge
        for i in range(ice_count):
            self.trains.append(self._create_train(
                f"ICE{500+train_id}",
                "ICE",
                random.choice(stations),
                random.choice(stations),
                base_speed=200,
                punctuality=self.PUNCTUALITY_ICE,
                avg_delay=self.AVG_DELAY_ICE_MIN
            ))
            train_id += 1
        
        # IC Züge
        for i in range(ic_count):
            self.trains.append(self._create_train(
                f"IC{2000+train_id}",
                "IC",
                random.choice(stations),
                random.choice(stations),
                base_speed=160,
                punctuality=self.PUNCTUALITY_ICE,
                avg_delay=self.AVG_DELAY_ICE_MIN
            ))
            train_id += 1
        
        # RE Züge
        for i in range(re_count):
            self.trains.append(self._create_train(
                f"RE{4000+train_id}",
                "RE",
                random.choice(stations),
                random.choice(stations),
                base_speed=140,
                punctuality=self.PUNCTUALITY_RE,
                avg_delay=self.AVG_DELAY_RE_MIN
            ))
            train_id += 1
        
        # RB Züge
        for i in range(rb_count):
            self.trains.append(self._create_train(
                f"RB{6000+train_id}",
                "RB",
                random.choice(stations),
                random.choice(stations),
                base_speed=120,
                punctuality=self.PUNCTUALITY_RE,
                avg_delay=self.AVG_DELAY_RE_MIN
            ))
            train_id += 1
        
        print(f"✓ Generated {len(self.trains)} trains")
        print(f"  ICE: {ice_count}, IC: {ic_count}, RE: {re_count}, RB: {rb_count}")
    
    def _create_train(self, train_number: str, category: str, 
                     origin: Dict, destination: Dict,
                     base_speed: int, punctuality: float, avg_delay: float) -> Dict:
        """Erstelle einen Zug mit realistischen Parametern"""
        
        # Verspätung simulieren (basierend auf Pünktlichkeitsstatistik)
        if random.random() > punctuality:
            # Zug ist verspätet - Exponentialverteilung für Verspätung
            delay_min = int(random.expovariate(1.0 / avg_delay))
            delay_min = min(delay_min, 60)  # Max 60 Min Verspätung
        else:
            delay_min = random.randint(-2, 2)  # Leicht vorzeitig/pünktlich
        
        # Berechne Strecke zwischen Origin und Destination
        distance_km = self._calculate_distance(
            origin['location'], destination['location']
        )
        
        # Finde passende Track Segments
        track_segments = self._find_route_segments(origin, destination)
        
        # Start-Position irgendwo auf der Strecke
        progress = random.uniform(0.0, 0.8)  # 0-80% der Strecke
        current_km = progress * distance_km
        current_speed = base_speed * random.uniform(0.85, 1.0)
        
        # Passagierzahlen (realistische Verteilung)
        if category == "ICE":
            capacity = random.randint(400, 900)
            occupancy = random.randint(150, capacity)
        elif category == "IC":
            capacity = random.randint(300, 600)
            occupancy = random.randint(100, capacity)
        else:  # RE/RB
            capacity = random.randint(200, 500)
            occupancy = random.randint(50, capacity)
        
        return {
            "train_number": train_number,
            "category": category,
            "origin": origin,
            "destination": destination,
            "current_km": current_km,
            "total_distance_km": distance_km,
            "current_speed_kmh": current_speed,
            "max_speed_kmh": base_speed,
            "delay_min": delay_min,
            "track_segments": track_segments,
            "current_segment_idx": int(progress * len(track_segments)) if track_segments else 0,
            "capacity": capacity,
            "occupancy": occupancy,
            "last_update": datetime.now()
        }
    
    def _calculate_distance(self, loc1: Dict, loc2: Dict) -> float:
        """Berechne Distanz zwischen zwei Punkten (Haversine)"""
        R = 6371  # Earth radius in km
        
        lat1, lon1 = math.radians(loc1['lat']), math.radians(loc1['lon'])
        lat2, lon2 = math.radians(loc2['lat']), math.radians(loc2['lon'])
        
        dlat = lat2 - lat1
        dlon = lon2 - lon1
        
        a = math.sin(dlat/2)**2 + math.cos(lat1) * math.cos(lat2) * math.sin(dlon/2)**2
        c = 2 * math.asin(math.sqrt(a))
        
        return R * c
    
    def _find_route_segments(self, origin: Dict, destination: Dict) -> List[Dict]:
        """Finde Streckenabschnitte für Route"""
        # Vereinfachte Logik: Nehme alle Segmente auf passender Strecke
        # In Realität: Verwende Graph-Algorithmus für optimale Route
        segments = []
        for seg in self.track_network['segments']:
            # Prüfe ob Segment in grober Richtung liegt
            if self._segment_on_route(seg, origin, destination):
                segments.append(seg)
        return segments[:50]  # Max 50 Segmente pro Route
    
    def _segment_on_route(self, segment: Dict, origin: Dict, dest: Dict) -> bool:
        """Prüfe ob Segment grob auf Route liegt (vereinfacht)"""
        # Vereinfachte Logik
        return random.random() > 0.7
    
    def start_simulation(self, update_interval_sec: float = 1.0):
        """Starte Echtzeit-Simulation"""
        print("\n" + "="*60)
        print("Starting Real-time Train Simulation")
        print("="*60)
        print(f"Active trains: {len(self.trains)}")
        print(f"Update interval: {update_interval_sec}s")
        print(f"ThemisDB URL: {self.themis_url}")
        print("\nPress Ctrl+C to stop...\n")
        
        self.running = True
        self.stats['trains_active'] = len(self.trains)
        
        try:
            while self.running:
                start_time = time.time()
                
                # Update alle Züge
                for train in self.trains:
                    self._update_train_position(train, update_interval_sec)
                    self._send_train_telemetry(train)
                    self._send_infrastructure_events(train)
                
                # Update Statistics
                self._update_statistics()
                
                # Wait for next update
                elapsed = time.time() - start_time
                sleep_time = max(0, update_interval_sec - elapsed)
                time.sleep(sleep_time)
                
        except KeyboardInterrupt:
            print("\n\nStopping simulation...")
            self.running = False
    
    def _update_train_position(self, train: Dict, dt: float):
        """Update Zugposition basierend auf Geschwindigkeit"""
        # Bewegung: v * t
        distance_km = (train['current_speed_kmh'] / 3600.0) * dt
        train['current_km'] += distance_km
        
        # Geschwindigkeit variieren (Beschleunigung/Bremsen)
        speed_change = random.uniform(-5, 5)
        train['current_speed_kmh'] = max(0, min(
            train['max_speed_kmh'],
            train['current_speed_kmh'] + speed_change
        ))
        
        # Verspätung dynamisch ändern
        if random.random() < 0.01:  # 1% Chance pro Update
            train['delay_min'] += random.randint(-1, 2)
            train['delay_min'] = max(-5, min(60, train['delay_min']))
        
        # Ziel erreicht? Neue Route
        if train['current_km'] >= train['total_distance_km']:
            self._respawn_train(train)
    
    def _respawn_train(self, train: Dict):
        """Respawn Zug mit neuer Route"""
        stations = list(self.track_network['stations'].values())
        new_origin = random.choice(stations)
        new_dest = random.choice(stations)
        
        train['origin'] = new_origin
        train['destination'] = new_dest
        train['current_km'] = 0.0
        train['total_distance_km'] = self._calculate_distance(
            new_origin['location'], new_dest['location']
        )
        train['track_segments'] = self._find_route_segments(new_origin, new_dest)
        train['current_segment_idx'] = 0
    
    def _send_train_telemetry(self, train: Dict):
        """Sende Zug-Telemetrie an ThemisDB"""
        
        # Berechne aktuelle GPS Position (interpoliert)
        if train['track_segments']:
            seg_idx = min(train['current_segment_idx'], len(train['track_segments']) - 1)
            segment = train['track_segments'][seg_idx]
            
            # Interpoliere Position auf Segment
            coords = segment['geometry']['coordinates']
            progress = (train['current_km'] % 1.0)  # Position innerhalb Segment
            lat = coords[0][1] + progress * (coords[1][1] - coords[0][1])
            lon = coords[0][0] + progress * (coords[1][0] - coords[0][0])
            alt = coords[0][2] + progress * (coords[1][2] - coords[0][2])
        else:
            # Fallback: Interpoliere zwischen Origin und Destination
            progress = train['current_km'] / train['total_distance_km']
            lat = train['origin']['location']['lat'] + progress * (
                train['destination']['location']['lat'] - train['origin']['location']['lat']
            )
            lon = train['origin']['location']['lon'] + progress * (
                train['destination']['location']['lon'] - train['origin']['location']['lon']
            )
            alt = train['origin']['location']['altitude']
        
        # GPS Telemetrie
        telemetry = {
            "gps": {
                "lat": lat,
                "lon": lon,
                "altitude_m": alt,
                "hdop": round(random.uniform(0.6, 1.2), 2),
                "satellites": random.randint(10, 14),
                "fix_quality": "3D_differential"
            },
            "kinematics": {
                "speed_kmh": round(train['current_speed_kmh'], 1),
                "acceleration_mps2": round(random.uniform(-0.5, 0.5), 2),
                "heading_deg": round(random.uniform(0, 360), 1)
            },
            "operational": {
                "delay_min": train['delay_min'],
                "occupancy": train['occupancy'],
                "capacity": train['capacity'],
                "occupancy_percent": round(100 * train['occupancy'] / train['capacity'], 1)
            }
        }
        
        # Sende an ThemisDB Time-Series
        self._put_timeseries("train_telemetry", train['train_number'], telemetry)
        self.stats['updates_sent'] += 1
    
    def _send_infrastructure_events(self, train: Dict):
        """Simuliere Infrastruktur-Events (Achszähler, Hotbox, etc.)"""
        
        # Achszähler Event (wenn Zug Segment betritt)
        if random.random() < 0.05:  # 5% Chance
            axle_event = {
                "event": "axle_detected",
                "train_id": train['train_number'],
                "speed_kmh": train['current_speed_kmh'],
                "axle_count": random.randint(12, 20)
            }
            self._put_timeseries("axle_counter_events", f"AC_{train['train_number']}", axle_event)
        
        # Hotbox Detector (Heißläufer - selten!)
        if random.random() < 0.001:  # 0.1% Chance (selten)
            hotbox = {
                "train_id": train['train_number'],
                "bearing_temp_celsius": random.uniform(75, 95),
                "alert": "elevated_temperature",
                "status": "warning"
            }
            self._put_timeseries("hotbox_detector", f"HABD_{train['train_number']}", hotbox)
            self.stats['hotbox_alerts'] += 1
        
        # Signal-Aspekt ändern
        if random.random() < 0.02:  # 2% Chance
            signals = list(self.track_network['signals'].keys())
            if signals:
                signal_id = random.choice(signals)
                aspect = random.choice(["Hp0_rot", "Hp1_gruen", "Hp2_gelb"])
                signal_event = {
                    "signal_id": signal_id,
                    "aspect": aspect,
                    "train_approaching": train['train_number']
                }
                self._put_timeseries("signal_events", signal_id, signal_event)
                self.stats['signals_changed'] += 1
    
    def _put_timeseries(self, metric: str, entity: str, value: Dict):
        """Schreibe Time-Series Daten an ThemisDB"""
        try:
            timestamp_ms = int(datetime.now().timestamp() * 1000)
            
            # ThemisDB Time-Series API
            # PUT /timeseries/{metric}/{entity}/{timestamp}
            url = f"{self.themis_url}/timeseries/{metric}/{entity}/{timestamp_ms}"
            
            requests.put(url, json=value, timeout=1)
        except requests.exceptions.RequestException as e:
            # Log connection errors but don't stop simulation
            if self.stats['updates_sent'] % 1000 == 0:
                print(f"\nWarning: Failed to send telemetry: {e}", file=sys.stderr)
        except Exception as e:
            # Log unexpected errors
            print(f"\nError in _put_timeseries: {e}", file=sys.stderr)
    
    def _update_statistics(self):
        """Update und zeige Statistiken"""
        if self.stats['updates_sent'] % 100 == 0:
            # Berechne durchschnittliche Verspätung
            total_delay = sum(t['delay_min'] for t in self.trains)
            self.stats['avg_delay_min'] = total_delay / len(self.trains) if self.trains else 0
            
            # Zähle Verspätungen >5 Min
            self.stats['delays_5min_plus'] = sum(1 for t in self.trains if t['delay_min'] > 5)
            
            # Zeige Stats
            print(f"\r[{datetime.now().strftime('%H:%M:%S')}] "
                  f"Updates: {self.stats['updates_sent']} | "
                  f"Active Trains: {self.stats['trains_active']} | "
                  f"Avg Delay: {self.stats['avg_delay_min']:.1f} min | "
                  f"Delays >5min: {self.stats['delays_5min_plus']} | "
                  f"Signals: {self.stats['signals_changed']} | "
                  f"Hotbox Alerts: {self.stats['hotbox_alerts']}", 
                  end='', flush=True)

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description='Railway Train Simulator')
    parser.add_argument('--network', required=True, help='Railway network JSON file')
    parser.add_argument('--trains', type=int, default=50, help='Number of trains to simulate')
    parser.add_argument('--interval', type=float, default=1.0, help='Update interval in seconds')
    parser.add_argument('--themis', default='http://localhost:8765', help='ThemisDB URL')
    
    args = parser.parse_args()
    
    simulator = TrainSimulator(args.themis)
    simulator.load_network(args.network)
    simulator.generate_train_schedule(args.trains)
    simulator.start_simulation(args.interval)
