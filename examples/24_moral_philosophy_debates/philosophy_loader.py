"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            philosophy_loader.py                               ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     284                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Philosophy Profile Loader

Lädt Philosophy-Profile aus YAML-Dateien im philosophies/ Verzeichnis.
Unterstützt dynamisches Laden und Caching.
"""

import os
import yaml
from pathlib import Path
from typing import Dict, Optional, List
from dataclasses import dataclass
from models import PhilosophySchool, PhilosophyProfile


class PhilosophyLoader:
    """
    Lädt und verwaltet Philosophy-Profile aus YAML-Dateien.
    """
    
    def __init__(self, philosophies_dir: str = "philosophies"):
        """
        Initialize the philosophy loader.
        
        Args:
            philosophies_dir: Verzeichnis mit YAML-Dateien
        """
        self.philosophies_dir = Path(philosophies_dir)
        self._cache: Dict[PhilosophySchool, PhilosophyProfile] = {}
        self._yaml_files: Dict[PhilosophySchool, Path] = {}
        self._discover_yaml_files()
    
    def _discover_yaml_files(self):
        """Entdeckt alle YAML-Dateien im Verzeichnis."""
        if not self.philosophies_dir.exists():
            print(f"Warning: Directory {self.philosophies_dir} does not exist")
            return
        
        # Mapping von school-Namen zu YAML-Dateien
        for yaml_file in self.philosophies_dir.glob("*.yaml"):
            try:
                with open(yaml_file, 'r', encoding='utf-8') as f:
                    data = yaml.safe_load(f)
                    if data and 'school' in data:
                        school_str = data['school']
                        try:
                            school = PhilosophySchool(school_str)
                            self._yaml_files[school] = yaml_file
                        except ValueError:
                            print(f"Warning: Unknown school '{school_str}' in {yaml_file}")
            except Exception as e:
                print(f"Error reading {yaml_file}: {e}")
    
    def load_profile(self, school: PhilosophySchool) -> Optional[PhilosophyProfile]:
        """
        Lädt ein Philosophy-Profile aus YAML-Datei.
        
        Args:
            school: Die zu ladende Philosophie
        
        Returns:
            PhilosophyProfile oder None wenn nicht gefunden
        """
        # Aus Cache zurückgeben wenn vorhanden
        if school in self._cache:
            return self._cache[school]
        
        # YAML-Datei laden
        if school not in self._yaml_files:
            return None
        
        yaml_file = self._yaml_files[school]
        try:
            with open(yaml_file, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)
            
            # Konvertiere YAML zu PhilosophyProfile
            profile = self._yaml_to_profile(data, school)
            
            # In Cache speichern
            self._cache[school] = profile
            
            return profile
        
        except Exception as e:
            print(f"Error loading profile for {school.value}: {e}")
            return None
    
    def _yaml_to_profile(self, data: dict, school: PhilosophySchool) -> PhilosophyProfile:
        """
        Konvertiert YAML-Daten zu PhilosophyProfile.
        
        Args:
            data: YAML-Daten als Dictionary
            school: PhilosophySchool Enum
        
        Returns:
            PhilosophyProfile Objekt
        """
        # Extrahiere core_principles aus YAML
        core_principles = data.get('core_principles', [])
        if isinstance(core_principles, list):
            # Liste von Strings
            core_principles_list = core_principles
        else:
            # Fallback zu leerer Liste
            core_principles_list = []
        
        # Erstelle description aus main_theses wenn nötig
        description = data.get('description', '')
        if not description and 'main_theses' in data:
            # Baue description aus ersten Hauptthesen
            main_theses = data['main_theses']
            if main_theses:
                first_thesis = list(main_theses.values())[0]
                description = first_thesis.get('description', '')
        
        # Erstelle PhilosophyProfile
        profile = PhilosophyProfile(
            school=school,
            name=data.get('name', school.value),
            philosopher_name=data.get('philosopher_name', ''),
            description=description.strip() if description else '',
            core_principles=core_principles_list,
            decision_framework=data.get('decision_framework', ''),
            example_application=data.get('example_application', '')
        )
        
        return profile
    
    def load_all_profiles(self) -> Dict[PhilosophySchool, PhilosophyProfile]:
        """
        Lädt alle verfügbaren Philosophy-Profile.
        
        Returns:
            Dictionary mit allen Profilen
        """
        profiles = {}
        for school in self._yaml_files.keys():
            profile = self.load_profile(school)
            if profile:
                profiles[school] = profile
        return profiles
    
    def get_available_schools(self) -> List[PhilosophySchool]:
        """
        Gibt Liste aller verfügbaren Schools zurück.
        
        Returns:
            Liste von PhilosophySchool Enums
        """
        return list(self._yaml_files.keys())
    
    def reload_profile(self, school: PhilosophySchool) -> Optional[PhilosophyProfile]:
        """
        Lädt ein Profile neu (ohne Cache).
        
        Args:
            school: Die neu zu ladende Philosophie
        
        Returns:
            PhilosophyProfile oder None
        """
        # Entferne aus Cache
        if school in self._cache:
            del self._cache[school]
        
        # Neu laden
        return self.load_profile(school)
    
    def get_profile_metadata(self, school: PhilosophySchool) -> Optional[dict]:
        """
        Gibt Metadaten eines Profiles zurück ohne vollständiges Laden.
        
        Args:
            school: Die Philosophie
        
        Returns:
            Dictionary mit Metadaten
        """
        if school not in self._yaml_files:
            return None
        
        yaml_file = self._yaml_files[school]
        try:
            with open(yaml_file, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)
            
            return {
                'name': data.get('name', ''),
                'philosopher_name': data.get('philosopher_name', ''),
                'philosopher_life': data.get('philosopher_life', ''),
                'nationality': data.get('nationality', ''),
                'has_main_theses': 'main_theses' in data,
                'has_secondary_theses': 'secondary_theses' in data,
                'key_works_count': len(data.get('key_works', [])),
                'file': str(yaml_file)
            }
        except Exception as e:
            print(f"Error reading metadata for {school.value}: {e}")
            return None


# Globaler Loader (Singleton)
_global_loader: Optional[PhilosophyLoader] = None


def get_philosophy_loader(philosophies_dir: str = "philosophies") -> PhilosophyLoader:
    """
    Gibt den globalen Philosophy-Loader zurück (Singleton).
    
    Args:
        philosophies_dir: Verzeichnis mit YAML-Dateien
    
    Returns:
        PhilosophyLoader Instanz
    """
    global _global_loader
    if _global_loader is None:
        _global_loader = PhilosophyLoader(philosophies_dir)
    return _global_loader


def load_philosophy_profiles() -> Dict[PhilosophySchool, PhilosophyProfile]:
    """
    Convenience-Funktion zum Laden aller Profile.
    
    Returns:
        Dictionary mit allen verfügbaren Profilen
    """
    loader = get_philosophy_loader()
    return loader.load_all_profiles()


# Beispiel-Verwendung
if __name__ == "__main__":
    # Lade alle Profile
    loader = get_philosophy_loader()
    
    print("Verfügbare Philosophien:")
    for school in loader.get_available_schools():
        metadata = loader.get_profile_metadata(school)
        if metadata:
            print(f"\n{metadata['name']} ({metadata['philosopher_name']})")
            print(f"  Leben: {metadata['philosopher_life']}")
            print(f"  Datei: {metadata['file']}")
            print(f"  Hauptthesen: {'Ja' if metadata['has_main_theses'] else 'Nein'}")
            print(f"  Werke: {metadata['key_works_count']}")
    
    print("\n" + "="*60)
    print("Lade alle Profile...")
    profiles = loader.load_all_profiles()
    print(f"✓ {len(profiles)} Profile geladen")
    
    # Beispiel: Kant laden
    if PhilosophySchool.KANT in profiles:
        kant = profiles[PhilosophySchool.KANT]
        print(f"\nBeispiel - {kant.name}:")
        print(f"Philosoph: {kant.philosopher_name}")
        print(f"Kernprinzipien: {len(kant.core_principles)}")
        print(f"Erstes Prinzip: {kant.core_principles[0] if kant.core_principles else 'N/A'}")
