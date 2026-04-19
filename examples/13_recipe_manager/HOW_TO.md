# Recipe Manager - Anleitung

> **Historischer Stand:** 2025-12-22 — Inhalte nicht gegen aktuelle Quellen geprüft.

## 🚀 Schnellstart

```bash
cd examples/13_recipe_manager
pip install -r requirements.txt
pip install themisdb-client
python main.py
```
<!-- TODO: verify against current source -->

## 📖 Hauptfunktionen

### Rezept erstellen
1. Klicken Sie auf "New Recipe"
2. Geben Sie Name und Kategorie ein
3. Fügen Sie Zutaten hinzu (mit Menge und Einheit)
4. Schreiben Sie Anweisungen
5. Optional: Tags hinzufügen
6. Speichern

### Nach Zutaten suchen
1. Geben Sie Zutat im Suchfeld ein
2. Rezepte mit dieser Zutat werden angezeigt
3. Filter nach mehreren Zutaten möglich

### Einkaufsliste generieren
1. Wählen Sie mehrere Rezepte aus
2. Klicken Sie auf "Generate Shopping List"
3. Zutaten werden automatisch zusammengefasst
4. Export als Text oder PDF

### Portionen anpassen
1. Öffnen Sie ein Rezept
2. Ändern Sie "Servings" Wert
3. Alle Zutatmengen werden automatisch umgerechnet

## 💡 Tipps
- Konsistente Einheiten verwenden (g, ml, Stück)
- Tags für Ernährungsformen (vegan, glutenfrei)
- Fotos zu Rezepten hinzufügen
- Kochzeit realistisch einschätzen

---

**Letzte Aktualisierung**: 2025-12-22
