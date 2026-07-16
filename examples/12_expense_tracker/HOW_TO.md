> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Expense Tracker - Anleitung

## 🚀 Schnellstart

```bash
cd examples/12_expense_tracker
pip install -r requirements.txt
python main.py
```

## 📖 Hauptfunktionen

### Transaktion hinzufügen

1. Klicken Sie auf **"New Transaction"**
2. Wählen Sie Typ: **Income** oder **Expense**
3. Geben Sie Betrag ein
4. Wählen Sie Kategorie
5. Fügen Sie Beschreibung hinzu
6. Klicken Sie auf **"Save"**

### Budget erstellen

1. Gehen Sie zu **"Budgets"** Tab
2. Klicken Sie auf **"New Budget"**
3. Wählen Sie Kategorie
4. Setzen Sie Betrag und Zeitraum
5. Optional: Alert-Schwellwert (z.B. 80%)
6. Speichern

### Statistiken anzeigen

1. Wählen Sie Zeitraum (Monat/Jahr)
2. Statistiken werden automatisch berechnet:
   - Gesamteinnahmen
   - Gesamtausgaben
   - Bilanz
   - Durchschnitt pro Tag
3. Charts zeigen Verteilung nach Kategorie

### Export

1. **File** → **Export**
2. Wählen Sie Format (CSV/PDF)
3. Wählen Sie Zeitraum
4. Speichern Sie die Datei

## 💡 Tipps

- Transaktionen täglich erfassen
- Konsistente Kategorien verwenden
- Budgets realistisch setzen
- Regelmäßig Berichte prüfen

## ⌨️ Tastenkombinationen

- `Ctrl + N`: Neue Transaktion
- `Ctrl + B`: Neues Budget
- `Ctrl + E`: Export
- `Ctrl + S`: Statistiken aktualisieren

---

**Letzte Aktualisierung**: 2025-12-22
