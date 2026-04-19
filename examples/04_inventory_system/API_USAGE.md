> **Hinweis:** API-Signaturen gegen aktuelle Endpunkte/Typen prüfen. Abweichungen mit `<!-- TODO: verify API -->` markieren.

# Inventarsystem - API Verwendung und Code-Beispiele

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## Übersicht

Dieses Dokument zeigt praktische Code-Beispiele für die Verwendung der Inventarsystem-API mit ThemisDB.

## Client-Initialisierung

### Basis-Setup

```python
from themis_client import InventoryClient
from models import Product, Supplier, StockMovement, MovementType

# Client erstellen
client = InventoryClient(
    host="localhost",
    port=8080,
    timeout=10
)

# Verbindung testen
if client.ping():
    print("✓ Verbindung zu ThemisDB erfolgreich")
else:
    print("✗ Keine Verbindung möglich")
```

### Konfiguration mit Environment Variables

```python
import os
from themis_client import InventoryClient

client = InventoryClient(
    host=os.getenv("THEMIS_HOST", "localhost"),
    port=int(os.getenv("THEMIS_PORT", 8080)),
    timeout=int(os.getenv("THEMIS_TIMEOUT", 10))
)
```

## Produkt-Operationen

### Produkt erstellen

```python
from models import Product
from datetime import datetime

# Neues Produkt
product = Product(
    id="prod-12345",
    sku="LAPTOP-001",
    name="Business Laptop Dell Latitude 5420",
    description="14 Zoll, Intel i5, 16GB RAM, 512GB SSD",
    stock=50,
    min_stock=20,
    max_stock=100,
    unit_price=79900,  # in Cent: 799.00 EUR
    category="Electronics",
    location="Warehouse-A-Shelf-12",
    created_at=datetime.now(),
    updated_at=datetime.now()
)

# Speichern
success = client.create_product(product)
if success:
    print(f"✓ Produkt {product.sku} erstellt")
else:
    print(f"✗ Fehler beim Erstellen")
```

### Produkt laden

```python
# Nach ID
product = client.get_product("prod-12345")
if product:
    print(f"Produkt: {product.name}")
    print(f"Bestand: {product.stock} Einheiten")
    print(f"Wert: {(product.stock * product.unit_price) / 100:.2f} EUR")

# Nach SKU
product = client.get_product_by_sku("LAPTOP-001")
```

### Produkt aktualisieren

```python
# Produkt laden
product = client.get_product("prod-12345")

# Felder ändern
product.stock = 45  # Bestand reduziert
product.unit_price = 74900  # Preis gesenkt
product.updated_at = datetime.now()

# Speichern
if client.update_product(product):
    print("✓ Produkt aktualisiert")
```

### Produkte auflisten

```python
# Alle Produkte
all_products = client.list_products()
print(f"Gesamt: {len(all_products)} Produkte")

# Nach Kategorie filtern
electronics = client.list_products(filters={
    "category": "Electronics"
})

# Mit niedrigem Bestand
low_stock = client.list_products(filters={
    "stock_below": "min_stock"  # stock < min_stock
})

# Kombinierte Filter
critical = client.list_products(filters={
    "category": "Electronics",
    "stock_below": "min_stock"
})
```

### Produkt löschen

```python
# Soft Delete (empfohlen)
product = client.get_product("prod-12345")
product.active = False
client.update_product(product)

# Hard Delete (Vorsicht!)
client.delete_product("prod-12345")
```

## Lieferanten-Operationen

### Lieferant erstellen

```python
from models import Supplier

supplier = Supplier(
    id="supp-789",
    name="TechSupply GmbH",
    contact_name="Max Müller",
    email="m.mueller@techsupply.de",
    phone="+49 30 12345678",
    address="Hauptstraße 123, 10115 Berlin",
    rating=5,
    active=True
)

if client.create_supplier(supplier):
    print(f"✓ Lieferant {supplier.name} angelegt")
```

### Lieferanten verwalten

```python
# Alle Lieferanten
suppliers = client.list_suppliers()

# Nur aktive
active_suppliers = client.list_suppliers(filters={"active": True})

# Nach Rating
top_suppliers = client.list_suppliers(filters={"rating": 5})

# Lieferant aktualisieren
supplier = client.get_supplier("supp-789")
supplier.rating = 4
supplier.email = "new@techsupply.de"
client.update_supplier(supplier)
```

## Graph-Beziehungen

### Produkt-Lieferanten-Beziehung erstellen

```python
from models import ProductSupplier

# Beziehung definieren
relationship = ProductSupplier(
    product_id="prod-12345",
    supplier_id="supp-789",
    unit_price=75000,  # Einkaufspreis in Cent
    min_order_qty=10,
    delivery_days=3,
    is_preferred=True
)

# Beziehung speichern
client.link_product_supplier(relationship)
```

### Lieferanten eines Produkts finden

```python
# Alle Lieferanten für ein Produkt
suppliers = client.get_suppliers_for_product("prod-12345")

for supplier in suppliers:
    print(f"Lieferant: {supplier.name}")
    print(f"  Preis: {supplier.unit_price / 100:.2f} EUR")
    print(f"  Mindestmenge: {supplier.min_order_qty}")
    print(f"  Lieferzeit: {supplier.delivery_days} Tage")
    if supplier.is_preferred:
        print(f"  ⭐ Bevorzugter Lieferant")
```

### Produkte eines Lieferanten finden

```python
# Alle Produkte von einem Lieferanten
products = client.get_products_for_supplier("supp-789")

print(f"Lieferant hat {len(products)} Produkte im Sortiment:")
for product in products:
    print(f"  - {product.name} ({product.sku})")
```

### Bevorzugten Lieferanten finden

```python
# Bester Preis mit bevorzugtem Status
preferred = client.get_preferred_supplier_for_product("prod-12345")

if preferred:
    print(f"Bevorzugter Lieferant: {preferred.name}")
    print(f"Preis: {preferred.unit_price / 100:.2f} EUR")
else:
    # Fallback: Günstigster Lieferant
    suppliers = client.get_suppliers_for_product("prod-12345")
    if suppliers:
        cheapest = min(suppliers, key=lambda s: s.unit_price)
        print(f"Günstigster Lieferant: {cheapest.name}")
```

## Bestandsbewegungen

### Wareneingang buchen

```python
from models import StockMovement, MovementType

# Lieferung erhalten
movement = StockMovement(
    product_id="prod-12345",
    type=MovementType.IN,
    quantity=20,
    reason="Lieferung von TechSupply GmbH, Bestellung #4567",
    user="warehouse_user",
    timestamp=datetime.now()
)

# Bewegung speichern und Bestand aktualisieren
if client.add_stock_movement(movement):
    print(f"✓ Wareneingang gebucht: +{movement.quantity} Einheiten")
    
    # Bestand wurde automatisch aktualisiert
    product = client.get_product("prod-12345")
    print(f"Neuer Bestand: {product.stock} Einheiten")
```

### Warenausgang buchen

```python
# Verkauf/Verbrauch
movement = StockMovement(
    product_id="prod-12345",
    type=MovementType.OUT,
    quantity=5,
    reason="Verkauf an Kunde #789",
    user="sales_user",
    timestamp=datetime.now()
)

# Verfügbarkeit prüfen
product = client.get_product("prod-12345")
if product.stock >= movement.quantity:
    if client.add_stock_movement(movement):
        print(f"✓ Warenausgang gebucht: -{movement.quantity} Einheiten")
        
        # Warnung bei niedrigem Bestand
        product = client.get_product("prod-12345")
        if product.stock < product.min_stock:
            print(f"⚠️ Bestand unter Minimum! Nachbestellung empfohlen.")
else:
    print(f"✗ Nicht genug Bestand! Verfügbar: {product.stock}")
```

### Inventurkorrektur

```python
# Bestandskorrektur nach Inventur
actual_stock = 47  # Gezählter Bestand
product = client.get_product("prod-12345")
difference = actual_stock - product.stock

if difference != 0:
    movement = StockMovement(
        product_id="prod-12345",
        type=MovementType.ADJUSTMENT,
        quantity=abs(difference),
        reason=f"Inventurkorrektur: Soll {product.stock}, Ist {actual_stock}",
        user="inventory_manager",
        timestamp=datetime.now()
    )
    
    # Bei negativer Differenz als Abgang buchen
    if difference < 0:
        movement.type = MovementType.OUT
    
    client.add_stock_movement(movement)
    print(f"✓ Bestand korrigiert: {product.stock} → {actual_stock}")
```

### Bewegungshistorie abrufen

```python
# Alle Bewegungen eines Produkts
movements = client.get_stock_movements(
    product_id="prod-12345",
    limit=10  # Letzte 10 Bewegungen
)

print("Bewegungshistorie:")
for movement in movements:
    sign = "+" if movement.type == MovementType.IN else "-"
    print(f"{movement.timestamp}: {sign}{movement.quantity} ({movement.type.value})")
    print(f"  Grund: {movement.reason}")
    print(f"  Benutzer: {movement.user}")
```

### Zeitbasierte Abfragen

```python
from datetime import datetime, timedelta

# Bewegungen der letzten 7 Tage
last_week = datetime.now() - timedelta(days=7)
recent_movements = client.get_stock_movements(
    product_id="prod-12345",
    since=last_week
)

# Aggregierte Statistiken
total_in = sum(m.quantity for m in recent_movements if m.type == MovementType.IN)
total_out = sum(m.quantity for m in recent_movements if m.type == MovementType.OUT)

print(f"Letzte 7 Tage:")
print(f"  Zugänge: +{total_in}")
print(f"  Abgänge: -{total_out}")
print(f"  Netto: {total_in - total_out}")
```

## Statistiken und Reports

### Dashboard-Statistiken

```python
# Gesamtstatistiken
stats = client.get_statistics()

print(f"Produkte gesamt: {stats['total_products']}")
print(f"Lagerwert: {stats['total_value'] / 100:.2f} EUR")
print(f"Produkte mit niedrigem Bestand: {stats['low_stock_count']}")
print(f"Ausverkaufte Produkte: {stats['out_of_stock_count']}")
```

### Warnungen abrufen

```python
# Produkte mit niedrigem Bestand
warnings = client.get_low_stock_warnings()

print(f"{len(warnings)} Warnungen:")
for warning in warnings:
    product = warning['product']
    status = warning['status']  # CRITICAL, LOW, WARNING
    
    print(f"{status}: {product.name}")
    print(f"  Bestand: {product.stock}/{product.min_stock}")
    print(f"  SKU: {product.sku}")
```

### Kategorie-Analyse

```python
# Statistiken pro Kategorie
category_stats = client.get_category_statistics()

for cat_name, stats in category_stats.items():
    print(f"\nKategorie: {cat_name}")
    print(f"  Produkte: {stats['count']}")
    print(f"  Gesamtwert: {stats['value'] / 100:.2f} EUR")
    print(f"  Durchschnittspreis: {stats['avg_price'] / 100:.2f} EUR")
```

### Export für Reporting

```python
import json
import csv

# JSON Export
products = client.list_products()
with open('inventory_export.json', 'w', encoding='utf-8') as f:
    json.dump([p.to_dict() for p in products], f, indent=2, default=str)

# CSV Export
with open('inventory_export.csv', 'w', newline='', encoding='utf-8') as f:
    writer = csv.writer(f)
    writer.writerow(['SKU', 'Name', 'Stock', 'Min Stock', 'Price', 'Value'])
    
    for product in products:
        writer.writerow([
            product.sku,
            product.name,
            product.stock,
            product.min_stock,
            product.unit_price / 100,
            (product.stock * product.unit_price) / 100
        ])
```

## Erweiterte Szenarien

### Automatische Nachbestellung

```python
def auto_reorder():
    """Erstellt Bestellvorschläge für Produkte mit niedrigem Bestand"""
    low_stock = client.get_low_stock_warnings()
    
    reorder_list = []
    for warning in low_stock:
        product = warning['product']
        
        # Bestellmenge berechnen
        order_qty = product.max_stock - product.stock
        
        # Bevorzugten Lieferanten finden
        supplier = client.get_preferred_supplier_for_product(product.id)
        
        if supplier:
            # Mindestmenge beachten
            if order_qty < supplier.min_order_qty:
                order_qty = supplier.min_order_qty
            
            reorder_list.append({
                'product': product,
                'supplier': supplier,
                'quantity': order_qty,
                'total_cost': (order_qty * supplier.unit_price) / 100
            })
    
    return reorder_list

# Bestellvorschläge generieren
orders = auto_reorder()
for order in orders:
    print(f"Bestellen: {order['quantity']}x {order['product'].name}")
    print(f"  bei: {order['supplier'].name}")
    print(f"  Kosten: {order['total_cost']:.2f} EUR")
```

### Batch-Operationen

```python
def bulk_price_update(category: str, increase_percent: float):
    """Erhöht Preise aller Produkte einer Kategorie"""
    products = client.list_products(filters={"category": category})
    
    updated = 0
    for product in products:
        old_price = product.unit_price
        product.unit_price = int(old_price * (1 + increase_percent / 100))
        product.updated_at = datetime.now()
        
        if client.update_product(product):
            updated += 1
            print(f"✓ {product.name}: {old_price/100:.2f} → {product.unit_price/100:.2f} EUR")
    
    print(f"\n{updated}/{len(products)} Preise aktualisiert")

# Verwendung: 5% Preiserhöhung für Electronics
bulk_price_update("Electronics", 5.0)
```

### Bestandsoptimierung

```python
from datetime import datetime, timedelta

def calculate_optimal_stock(product_id: str, days: int = 30):
    """Berechnet optimalen Bestand basierend auf Verbrauch"""
    # Bewegungen der letzten X Tage
    since = datetime.now() - timedelta(days=days)
    movements = client.get_stock_movements(
        product_id=product_id,
        since=since
    )
    
    # Durchschnittlicher Verbrauch pro Tag
    total_out = sum(m.quantity for m in movements if m.type == MovementType.OUT)
    avg_daily_usage = total_out / days
    
    # Empfehlungen (Safety Stock + Lead Time)
    lead_time_days = 7  # Annahme
    safety_factor = 1.5
    
    recommended_min = int(avg_daily_usage * lead_time_days * safety_factor)
    recommended_max = recommended_min * 3
    
    product = client.get_product(product_id)
    
    return {
        'product': product,
        'avg_daily_usage': avg_daily_usage,
        'current_min': product.min_stock,
        'current_max': product.max_stock,
        'recommended_min': recommended_min,
        'recommended_max': recommended_max,
        'should_adjust': (
            abs(product.min_stock - recommended_min) > recommended_min * 0.2 or
            abs(product.max_stock - recommended_max) > recommended_max * 0.2
        )
    }

# Optimierung durchführen
optimization = calculate_optimal_stock("prod-12345")
if optimization['should_adjust']:
    print("Anpassung empfohlen:")
    print(f"  Min: {optimization['current_min']} → {optimization['recommended_min']}")
    print(f"  Max: {optimization['current_max']} → {optimization['recommended_max']}")
```

## Fehlerbehandlung

### Robuste API-Calls

```python
from themis_client import ThemisDBError, ConnectionError, ValidationError

def safe_create_product(product: Product) -> bool:
    try:
        return client.create_product(product)
    except ValidationError as e:
        print(f"✗ Validierungsfehler: {e}")
        return False
    except ConnectionError as e:
        print(f"✗ Verbindungsfehler: {e}")
        # Retry-Logik hier
        return False
    except ThemisDBError as e:
        print(f"✗ Datenbankfehler: {e}")
        return False
    except Exception as e:
        print(f"✗ Unerwarteter Fehler: {e}")
        import traceback
        traceback.print_exc()
        return False
```

### Retry-Mechanismus

```python
import time

def retry_operation(func, max_retries=3, delay=1):
    """Führt Operation mit automatischen Wiederholungen aus"""
    for attempt in range(max_retries):
        try:
            return func()
        except ConnectionError:
            if attempt < max_retries - 1:
                print(f"Retry {attempt + 1}/{max_retries}...")
                time.sleep(delay * (attempt + 1))  # Exponential backoff
            else:
                raise
    
# Verwendung
result = retry_operation(
    lambda: client.get_product("prod-12345"),
    max_retries=3
)
```

## Best Practices

### 1. Transaktionen verwenden

```python
# Atomare Bestandsänderung
with client.transaction() as tx:
    # Bestand reduzieren
    tx.add_stock_movement(movement_out)
    
    # Gleichzeitig Statistik aktualisieren
    tx.update_statistics()
    
    # Commit oder Rollback automatisch
```

### 2. Connection Pooling

```python
# Für Multi-Threading
from themis_client import InventoryClientPool

pool = InventoryClientPool(
    host="localhost",
    port=8080,
    pool_size=10
)

# Client aus Pool holen
with pool.get_client() as client:
    products = client.list_products()
```

### 3. Caching

```python
from functools import lru_cache
from datetime import datetime, timedelta

class CachedInventoryClient(InventoryClient):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._cache_time = {}
    
    @lru_cache(maxsize=100)
    def get_product_cached(self, product_id: str):
        """Cached Version von get_product"""
        return self.get_product(product_id)
    
    def invalidate_cache(self, product_id: str):
        """Cache für Produkt invalidieren"""
        self.get_product_cached.cache_clear()
```

## Zusammenfassung

Die Inventarsystem-API bietet:

✅ **CRUD-Operationen** für Produkte und Lieferanten
✅ **Graph-Queries** für Beziehungen
✅ **Time-Series** für Bewegungshistorie
✅ **Statistiken und Reports**
✅ **Fehlerbehandlung** mit spezifischen Exceptions
✅ **Performance** durch Caching und Pooling

**Siehe auch**:
- [DATA_MODEL.md](DATA_MODEL.md) - Datenmodell-Details
- [README.md](README.md) - Übersicht und Installation
- [HOW_TO.md](HOW_TO.md) - UI-Bedienungsanleitung
