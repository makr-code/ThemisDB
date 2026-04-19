# Inventarsystem - Datenmodell und ER-Diagramm

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## Übersicht

Das Inventarsystem verwendet ein Multi-Model-Design mit ThemisDB, das relationale, zeitbasierte und Graph-Datenmodelle kombiniert.

## Konzeptionelles Datenmodell

### Entity-Relationship-Diagramm (ERD)

```
┌──────────────┐         ┌──────────────────┐         ┌─────────────┐
│   Supplier   │◄───────►│ ProductSupplier  │◄───────►│   Product   │
│              │  n:m     │   (Graph Edge)   │  n:m    │             │
├──────────────┤         ├──────────────────┤         ├─────────────┤
│ id (PK)      │         │ product_id (FK)  │         │ id (PK)     │
│ name         │         │ supplier_id (FK) │         │ sku         │
│ contact_name │         │ unit_price       │         │ name        │
│ email        │         │ min_order_qty    │         │ description │
│ phone        │         │ delivery_days    │         │ stock       │
│ address      │         │ is_preferred     │         │ min_stock   │
│ rating       │         └──────────────────┘         │ max_stock   │
│ active       │                                      │ unit_price  │
└──────────────┘                                      │ category    │
                                                      │ location    │
                    ┌──────────────────┐              │ created_at  │
                    │ StockMovement    │              │ updated_at  │
                    │ (Time-Series)    │              └──────┬──────┘
                    ├──────────────────┤                     │
                    │ id (PK)          │                     │
                    │ product_id (FK)  │◄────────────────────┘
                    │ type             │        1:n
                    │ quantity         │
                    │ reason           │
                    │ user             │
                    │ timestamp        │
                    └──────────────────┘
```

## Physisches Datenmodell

### 1. Product Collection (Relational)

Speichert Produktstammdaten.

```sql
CREATE TABLE products (
  id            VARCHAR(36) PRIMARY KEY,
  sku           VARCHAR(50) UNIQUE NOT NULL,
  name          VARCHAR(200) NOT NULL,
  description   TEXT,
  stock         INTEGER DEFAULT 0,
  min_stock     INTEGER DEFAULT 10,
  max_stock     INTEGER DEFAULT 100,
  unit_price    DECIMAL(10,2) NOT NULL,
  category      VARCHAR(50),
  location      VARCHAR(100),
  created_at    TIMESTAMP DEFAULT NOW(),
  updated_at    TIMESTAMP DEFAULT NOW(),
  
  -- Constraints
  CONSTRAINT check_stock CHECK (stock >= 0),
  CONSTRAINT check_min_stock CHECK (min_stock >= 0),
  CONSTRAINT check_max_stock CHECK (max_stock >= min_stock),
  CONSTRAINT check_price CHECK (unit_price >= 0)
);

-- Indexes für Performance
CREATE INDEX idx_products_sku ON products(sku);
CREATE INDEX idx_products_category ON products(category);
CREATE INDEX idx_products_stock ON products(stock);
CREATE INDEX idx_products_location ON products(location);
```

**Datentypen**:
- `id`: UUID für eindeutige Identifikation
- `sku`: Stock Keeping Unit (Artikelnummer)
- `stock`: Aktueller Bestand
- `min_stock`: Mindestbestand (Warnschwelle)
- `max_stock`: Maximaler Bestand
- `unit_price`: Preis pro Einheit in Cent

**Geschäftsregeln**:
- SKU muss eindeutig sein
- Bestand kann nicht negativ werden
- min_stock ≤ max_stock
- Preise müssen ≥ 0 sein

### 2. Supplier Collection (Relational)

Speichert Lieferanteninformationen.

```sql
CREATE TABLE suppliers (
  id            VARCHAR(36) PRIMARY KEY,
  name          VARCHAR(200) NOT NULL,
  contact_name  VARCHAR(200),
  email         VARCHAR(255),
  phone         VARCHAR(50),
  address       TEXT,
  rating        INTEGER CHECK (rating BETWEEN 1 AND 5),
  active        BOOLEAN DEFAULT TRUE,
  created_at    TIMESTAMP DEFAULT NOW(),
  updated_at    TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_suppliers_name ON suppliers(name);
CREATE INDEX idx_suppliers_active ON suppliers(active);
```

**Geschäftsregeln**:
- Name ist Pflichtfeld
- Rating zwischen 1-5 Sternen
- active = FALSE für deaktivierte Lieferanten

### 3. ProductSupplier Collection (Graph)

Verwaltet n:m-Beziehungen zwischen Produkten und Lieferanten.

```sql
CREATE EDGE product_supplier (
  product_id    VARCHAR(36) REFERENCES products(id),
  supplier_id   VARCHAR(36) REFERENCES suppliers(id),
  unit_price    DECIMAL(10,2),
  min_order_qty INTEGER DEFAULT 1,
  delivery_days INTEGER,
  is_preferred  BOOLEAN DEFAULT FALSE,
  created_at    TIMESTAMP DEFAULT NOW(),
  
  PRIMARY KEY (product_id, supplier_id),
  
  CONSTRAINT check_unit_price CHECK (unit_price >= 0),
  CONSTRAINT check_min_order CHECK (min_order_qty > 0),
  CONSTRAINT check_delivery CHECK (delivery_days >= 0)
);

-- Graph-Indizes für Traversierung
CREATE INDEX idx_ps_product ON product_supplier(product_id);
CREATE INDEX idx_ps_supplier ON product_supplier(supplier_id);
CREATE INDEX idx_ps_preferred ON product_supplier(is_preferred);
```

**Verwendung**:
- Jedes Produkt kann mehrere Lieferanten haben
- Jeder Lieferant kann mehrere Produkte liefern
- `is_preferred` markiert bevorzugten Lieferanten
- `unit_price` kann vom Produkt-Preis abweichen

**Graph-Queries**:

```sql
-- Alle Lieferanten für ein Produkt
MATCH (p:Product {id: 'prod-123'})-[r:SUPPLIES]-(s:Supplier)
RETURN s, r.unit_price, r.is_preferred
ORDER BY r.is_preferred DESC, r.unit_price ASC;

-- Alle Produkte eines Lieferanten
MATCH (s:Supplier {id: 'supp-456'})-[r:SUPPLIES]-(p:Product)
RETURN p, r.unit_price, r.min_order_qty;

-- Lieferanten-Netzwerk (2. Ordnung)
MATCH (s1:Supplier)-[:SUPPLIES]-(p:Product)-[:SUPPLIES]-(s2:Supplier)
WHERE s1.id = 'supp-123' AND s1 <> s2
RETURN DISTINCT s2.name, COUNT(p) AS shared_products
ORDER BY shared_products DESC;
```

### 4. StockMovement Collection (Time-Series)

Protokolliert alle Bestandsänderungen für Audit und Analyse.

```sql
CREATE TIMESERIES stock_movements (
  id           VARCHAR(36) PRIMARY KEY,
  product_id   VARCHAR(36) REFERENCES products(id),
  type         VARCHAR(20) NOT NULL,  -- IN, OUT, ADJUSTMENT, RETURN
  quantity     INTEGER NOT NULL,
  reason       TEXT,
  user         VARCHAR(100),
  timestamp    TIMESTAMP DEFAULT NOW(),
  
  -- Time-Series Partitionierung
  PARTITION BY RANGE (timestamp) INTERVAL '1 month',
  
  -- Retention Policy
  RETENTION 24 MONTHS
);

-- Time-Series Indizes
CREATE INDEX idx_movements_product ON stock_movements(product_id, timestamp DESC);
CREATE INDEX idx_movements_type ON stock_movements(type, timestamp DESC);
CREATE INDEX idx_movements_timestamp ON stock_movements(timestamp DESC);
```

**Movement Types**:
- `IN`: Wareneingang (Lieferung)
- `OUT`: Warenausgang (Verkauf/Verbrauch)
- `ADJUSTMENT`: Korrektur (Inventur)
- `RETURN`: Rücksendung

**Time-Series Features**:
- Automatische Partitionierung nach Monat
- 24-Monate Retention Policy
- Optimiert für Zeitbereichs-Queries

**Beispiel-Queries**:

```sql
-- Bewegungen der letzten 7 Tage
SELECT * FROM stock_movements
WHERE timestamp >= NOW() - INTERVAL '7 days'
ORDER BY timestamp DESC;

-- Aggregierte Statistiken pro Produkt
SELECT 
  product_id,
  SUM(CASE WHEN type = 'IN' THEN quantity ELSE 0 END) AS total_in,
  SUM(CASE WHEN type = 'OUT' THEN quantity ELSE 0 END) AS total_out,
  COUNT(*) AS movement_count
FROM stock_movements
WHERE timestamp >= NOW() - INTERVAL '30 days'
GROUP BY product_id;
```

## Datenfluss-Diagramme

### Wareneingang-Workflow

```
1. Lieferung erhalten
   ↓
2. Produkt auswählen
   ↓
3. Menge eingeben
   ↓
4. StockMovement erstellen (Type: IN)
   ↓
5. Product.stock aktualisieren (+quantity)
   ↓
6. Status prüfen (OK/Warning/Low/Critical)
   ↓
7. UI aktualisieren
```

### Warenausgang-Workflow

```
1. Verkauf/Verbrauch
   ↓
2. Produkt auswählen
   ↓
3. Verfügbarkeit prüfen (stock >= quantity)
   ↓
4. Falls OK:
   StockMovement erstellen (Type: OUT)
   ↓
5. Product.stock aktualisieren (-quantity)
   ↓
6. Falls stock < min_stock:
   Warning generieren
   ↓
7. UI aktualisieren
```

### Lieferanten-Bestellung

```
1. Produkt mit niedrigem Bestand identifizieren
   ↓
2. Graph-Query: Lieferanten laden
   ↓
3. Bevorzugten Lieferanten wählen (is_preferred)
   ↓
4. Bestellmenge berechnen (max_stock - stock)
   ↓
5. Berücksichtigen: min_order_qty, unit_price
   ↓
6. Bestellung extern erstellen
   ↓
7. Bei Lieferung: Wareneingang-Workflow
```

## Berechnungen und Aggregationen

### Bestandsstatus

```python
def calculate_stock_status(product):
    if product.stock == 0:
        return "OUT_OF_STOCK"  # Rot
    elif product.stock < product.min_stock:
        return "CRITICAL"      # Orange
    elif product.stock < product.min_stock * 1.5:
        return "LOW"          # Gelb
    elif product.stock < product.min_stock * 2:
        return "WARNING"       # Hellgelb
    else:
        return "OK"           # Grün
```

### Lagerwert berechnen

```sql
-- Gesamter Lagerwert
SELECT SUM(stock * unit_price) AS total_value
FROM products;

-- Lagerwert pro Kategorie
SELECT 
  category,
  SUM(stock * unit_price) AS category_value,
  COUNT(*) AS product_count
FROM products
GROUP BY category
ORDER BY category_value DESC;
```

### Bestandsumschlag (Inventory Turnover)

```sql
-- Umschlag der letzten 30 Tage
SELECT 
  p.id,
  p.name,
  p.stock AS current_stock,
  SUM(CASE WHEN sm.type = 'OUT' THEN sm.quantity ELSE 0 END) AS units_sold,
  ROUND(
    SUM(CASE WHEN sm.type = 'OUT' THEN sm.quantity ELSE 0 END) / 
    NULLIF(p.stock, 0), 
    2
  ) AS turnover_ratio
FROM products p
LEFT JOIN stock_movements sm ON p.id = sm.product_id
WHERE sm.timestamp >= NOW() - INTERVAL '30 days'
GROUP BY p.id, p.name, p.stock
ORDER BY turnover_ratio DESC;
```

### ABC-Analyse

```sql
-- Klassifizierung nach Wert (Pareto-Prinzip)
WITH product_values AS (
  SELECT 
    id,
    name,
    stock * unit_price AS value,
    SUM(stock * unit_price) OVER () AS total_value
  FROM products
),
cumulative AS (
  SELECT 
    *,
    SUM(value) OVER (ORDER BY value DESC) AS cumulative_value
  FROM product_values
)
SELECT 
  id,
  name,
  value,
  ROUND(100.0 * value / total_value, 2) AS value_percent,
  ROUND(100.0 * cumulative_value / total_value, 2) AS cumulative_percent,
  CASE
    WHEN cumulative_value / total_value <= 0.80 THEN 'A'
    WHEN cumulative_value / total_value <= 0.95 THEN 'B'
    ELSE 'C'
  END AS abc_class
FROM cumulative
ORDER BY value DESC;
```

## Performance-Optimierung

### Indizierung-Strategie

**Häufige Queries**:
1. Produkte nach SKU suchen → `idx_products_sku`
2. Produkte nach Kategorie filtern → `idx_products_category`
3. Produkte mit niedrigem Bestand → `idx_products_stock`
4. Bewegungen eines Produkts → `idx_movements_product`
5. Lieferanten eines Produkts → `idx_ps_product`

### Denormalisierung

**Stock-Level in Product**:
- Statt jedes Mal StockMovements zu summieren
- Cached in `product.stock`
- Update via Trigger bei jedem Movement

### Partitionierung

**StockMovements**:
- Monatliche Partitionen
- Alte Partitionen können archiviert werden
- Queries auf aktuelle Daten bleiben schnell

## Datenmigration

### Beispiel-Daten laden

```python
# Produkte
products = [
    {
        "sku": "LAPTOP-001",
        "name": "Business Laptop",
        "stock": 50,
        "min_stock": 20,
        "unit_price": 79900  # 799.00 EUR in Cent
    },
    # ...
]

# Lieferanten
suppliers = [
    {
        "name": "TechSupply GmbH",
        "contact_name": "Max Müller",
        "rating": 5
    },
    # ...
]

# Beziehungen
relationships = [
    {
        "product_sku": "LAPTOP-001",
        "supplier_name": "TechSupply GmbH",
        "unit_price": 75000,  # Einkaufspreis
        "is_preferred": True
    },
    # ...
]
```

## Backup und Recovery

### Backup-Strategie

1. **Vollbackup**: Täglich nachts
   ```sql
   BACKUP DATABASE TO '/backup/inventory_full_2024-01-15.bak';
   ```

2. **Incremental**: Stündlich
   ```sql
   BACKUP DATABASE TO '/backup/inventory_incr_2024-01-15_14h.bak' 
   INCREMENTAL SINCE LAST;
   ```

3. **Export kritischer Daten**:
   ```bash
   # Produkte als JSON
   curl http://localhost:8080/api/products > products_backup.json
   ```

### Disaster Recovery

```sql
-- Restore von Vollbackup
RESTORE DATABASE FROM '/backup/inventory_full_2024-01-15.bak';

-- Apply Incremental
RESTORE DATABASE FROM '/backup/inventory_incr_2024-01-15_14h.bak'
INCREMENTAL;
```

## Datenqualität

### Validierungsregeln

```python
def validate_product(product):
    errors = []
    
    # SKU Format
    if not re.match(r'^[A-Z0-9-]+$', product.sku):
        errors.append("SKU must contain only uppercase letters, numbers, and hyphens")
    
    # Stock Levels
    if product.min_stock > product.max_stock:
        errors.append("min_stock must be <= max_stock")
    
    # Price
    if product.unit_price <= 0:
        errors.append("unit_price must be positive")
    
    return errors
```

### Data Integrity Checks

```sql
-- Inkonsistente Bestände
SELECT p.id, p.name, p.stock, 
       COALESCE(SUM(
         CASE 
           WHEN sm.type IN ('IN', 'RETURN') THEN sm.quantity
           WHEN sm.type IN ('OUT', 'ADJUSTMENT') THEN -sm.quantity
           ELSE 0
         END
       ), 0) AS calculated_stock
FROM products p
LEFT JOIN stock_movements sm ON p.id = sm.product_id
GROUP BY p.id, p.name, p.stock
HAVING p.stock <> calculated_stock;
```

## Zusammenfassung

Das Inventarsystem-Datenmodell kombiniert:

✅ **Relational**: Produkte und Lieferanten
✅ **Graph**: n:m-Beziehungen mit Eigenschaften  
✅ **Time-Series**: Audit-Trail aller Bewegungen

**Vorteile**:
- Flexible Abfragen durch Multi-Model
- Performance durch richtige Indizierung
- Historisierung für Compliance
- Skalierbar durch Partitionierung

**Best Practices**:
- Transaktionen für Bestandsänderungen
- Constraints für Datenintegrität
- Regular Backups
- Monitoring von Bestandsschwellen
