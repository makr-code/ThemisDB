# Kapitel 5: Relationale Daten

> *"Relational databases are like spreadsheets that can talk to each other - 
> but with ACID guarantees and without the chaos."*

---

## Überblick

Relationale Datenbanken sind das Rückgrat der IT seit über 40 Jahren. In ThemisDB ist das relationale Modell eines von vier gleichberechtigten Datenmodellen - aber mit vollem SQL-Support und ACID-Garantien.

**Was Sie in diesem Kapitel lernen werden:**
- Relationales Datenmodell: Tabellen, Schemas, Constraints
- SQL in ThemisDB: DDL, DML, Joins, Subqueries
- Normalisierung vs. Denormalisierung
- Transaktionen und Integrität
- Indexes und Performance
- **Praxisbeispiel 1:** Inventory System (Lagerverwaltung)
- **Praxisbeispiel 2:** Expense Tracker (Ausgabenverwaltung)

**Voraussetzungen:** SQL-Grundkenntnisse hilfreich, aber nicht notwendig.

---

## 5.1 Das Relationale Modell

### Grundkonzepte

**Tabelle (Table):** Sammlung von Zeilen mit gleichem Schema

```
products (Tabelle)
+----+----------+-------+-------+
| id | name     | price | stock |
+----+----------+-------+-------+
|  1 | Laptop   |  1200 |    15 |
|  2 | Mouse    |    25 |   100 |
|  3 | Keyboard |    80 |    50 |
+----+----------+-------+-------+
```

**Zeile (Row):** Ein Datensatz in einer Tabelle  
**Spalte (Column):** Ein Attribut, das jede Zeile hat  
**Schema:** Definition der Spalten und Typen  
**Primärschlüssel (Primary Key):** Eindeutige ID für jede Zeile  
**Fremdschlüssel (Foreign Key):** Referenz zu einer anderen Tabelle  

### Warum Relational?

**✅ Vorteile:**
- **Struktur:** Klare, vorhersehbare Datenstruktur
- **Integrität:** Constraints garantieren Datenqualität
- **Joins:** Verknüpfe Daten aus mehreren Tabellen
- **ACID:** Transaktionen mit vollständiger Konsistenz
- **SQL:** Standardisierte, mächtige Abfragesprache

**❌ Nachteile:**
- Rigid: Schema-Änderungen erfordern Migrations
- Komplex: Normalisierung kann zu vielen Tables führen
- Performance: Joins können langsam sein bei vielen Tables
- Skalierung: Vertical Scaling bevorzugt

### Wann Relational wählen?

**Perfekt für:**
- ✅ Strukturierte Business-Daten (Orders, Invoices, Inventory)
- ✅ Daten mit klaren Beziehungen (Customers → Orders → Items)
- ✅ Transaktionale Systeme (Banking, E-Commerce)
- ✅ Reports mit Aggregationen (SUM, AVG, GROUP BY)
- ✅ Datenintegrität ist kritisch

**Weniger geeignet für:**
- ❌ Hochflexible Schemas (Metadata, User-Generated Content)
- ❌ Netzwerk-Daten mit vielen Hops (Social Graphs)
- ❌ Unstrukturierte Daten (Logs, Dokumente)
- ❌ Vektor-Daten (Embeddings, Features)

---

## 5.2 Tabellen und Schemas

### Tabelle erstellen

```sql
-- Basic Table
CREATE TABLE products (
  id INT PRIMARY KEY,
  name VARCHAR(255) NOT NULL,
  price DECIMAL(10, 2) NOT NULL,
  stock INT DEFAULT 0,
  created_at TIMESTAMP DEFAULT NOW()
);
```

**Datentypen in ThemisDB:**

| Typ | Beschreibung | Beispiel |
|-----|--------------|----------|
| INT | Ganzzahl | 42, -17, 0 |
| BIGINT | Große Ganzzahl | 9223372036854775807 |
| DECIMAL(p,s) | Festkommazahl | 123.45 |
| FLOAT/DOUBLE | Fließkommazahl | 3.14159 |
| VARCHAR(n) | Variable Zeichenkette | "Hello World" |
| TEXT | Unbegrenzte Zeichenkette | Langer Text |
| BOOLEAN | Wahrheitswert | true, false |
| TIMESTAMP | Zeitstempel | 2025-12-28 10:30:00 |
| DATE | Datum | 2025-12-28 |
| JSON | JSON-Dokument | {"key": "value"} |

### Constraints

**Primary Key:**
```sql
CREATE TABLE customers (
  id INT PRIMARY KEY,
  email VARCHAR(255) UNIQUE NOT NULL,
  name VARCHAR(255) NOT NULL
);
```

**Foreign Key:**
```sql
CREATE TABLE orders (
  id INT PRIMARY KEY,
  customer_id INT NOT NULL,
  total DECIMAL(10, 2) NOT NULL,
  created_at TIMESTAMP DEFAULT NOW(),
  
  FOREIGN KEY (customer_id) REFERENCES customers(id)
);
```

**Check Constraints:**
```sql
CREATE TABLE products (
  id INT PRIMARY KEY,
  name VARCHAR(255) NOT NULL,
  price DECIMAL(10, 2) NOT NULL CHECK (price >= 0),
  stock INT NOT NULL CHECK (stock >= 0),
  discount DECIMAL(3, 2) CHECK (discount BETWEEN 0 AND 1)
);
```

**Unique Constraints:**
```sql
CREATE TABLE users (
  id INT PRIMARY KEY,
  username VARCHAR(50) UNIQUE NOT NULL,
  email VARCHAR(255) UNIQUE NOT NULL
);
```

### Auto-Increment IDs

```sql
CREATE TABLE products (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255) NOT NULL
);

-- Insert ohne ID
INSERT INTO products (name) VALUES ('Laptop');
-- → Automatisch id=1

INSERT INTO products (name) VALUES ('Mouse');
-- → Automatisch id=2
```

---

## 5.3 Daten einfügen, ändern, löschen

### INSERT

```sql
-- Single Row
INSERT INTO products (id, name, price, stock)
VALUES (1, 'Laptop', 1200.00, 15);

-- Multiple Rows
INSERT INTO products (id, name, price, stock)
VALUES 
  (2, 'Mouse', 25.00, 100),
  (3, 'Keyboard', 80.00, 50),
  (4, 'Monitor', 350.00, 20);
```

### UPDATE

```sql
-- Update single row
UPDATE products
SET stock = stock - 1
WHERE id = 1;

-- Update multiple rows
UPDATE products
SET price = price * 0.9
WHERE stock > 50;

-- Update with JOIN
UPDATE order_items oi
SET oi.price = p.price
FROM products p
WHERE oi.product_id = p.id;
```

### DELETE

```sql
-- Delete single row
DELETE FROM products WHERE id = 1;

-- Delete multiple rows
DELETE FROM products WHERE stock = 0;

-- Delete all (Vorsicht!)
DELETE FROM products;
```

### UPSERT (Insert or Update)

```sql
-- Insert or Update on conflict
INSERT INTO products (id, name, price, stock)
VALUES (1, 'Laptop', 1200.00, 15)
ON CONFLICT (id) DO UPDATE
SET 
  price = EXCLUDED.price,
  stock = EXCLUDED.stock;
```

---

## 5.4 Queries und Joins

### SELECT Basics

```sql
-- All columns
SELECT * FROM products;

-- Specific columns
SELECT name, price FROM products;

-- With WHERE
SELECT * FROM products WHERE price > 100;

-- With ORDER BY
SELECT * FROM products ORDER BY price DESC LIMIT 10;

-- With aggregation
SELECT AVG(price), MAX(price), MIN(price), COUNT(*)
FROM products;

-- With GROUP BY
SELECT category, COUNT(*), AVG(price)
FROM products
GROUP BY category;
```

### INNER JOIN

```sql
-- Orders mit Customer-Namen
SELECT 
  o.id,
  o.total,
  c.name AS customer_name
FROM orders o
INNER JOIN customers c ON o.customer_id = c.id;
```

**Visualisierung:**
```
customers           orders
+---------+         +---------+
| id=1    |←--------|cust_id=1|  ✓ Match → returned
| Alice   |         | €100    |
+---------+         +---------+
| id=2    |         | cust_id=1|  ✓ Match → returned
| Bob     |         | €50     |
+---------+         +---------+
| id=3    |         
| Carol   |         → Kein Match, nicht returned
+---------+
```

### LEFT JOIN

```sql
-- Alle Customers, mit/ohne Orders
SELECT 
  c.name,
  COUNT(o.id) AS order_count,
  SUM(o.total) AS total_spent
FROM customers c
LEFT JOIN orders o ON c.customer_id = o.id
GROUP BY c.id, c.name;
```

**Visualisierung:**
```
customers           orders
+---------+         +---------+
| id=1    |←--------|cust_id=1|  ✓ Match
| Alice   |         | €100    |
+---------+         +---------+
| id=2    |
| Bob     |         → Kein Order, aber returned mit NULL
+---------+
```

### Multi-Table JOIN

```sql
-- Orders mit Items und Products
SELECT 
  o.id AS order_id,
  c.name AS customer,
  p.name AS product,
  oi.quantity,
  oi.price
FROM orders o
JOIN customers c ON o.customer_id = c.id
JOIN order_items oi ON oi.order_id = o.id
JOIN products p ON oi.product_id = p.id
WHERE o.created_at >= '2025-01-01';
```

### Subqueries

```sql
-- Products teurer als Durchschnitt
SELECT name, price
FROM products
WHERE price > (SELECT AVG(price) FROM products);

-- Customers mit mehr als 3 Orders
SELECT name
FROM customers
WHERE id IN (
  SELECT customer_id
  FROM orders
  GROUP BY customer_id
  HAVING COUNT(*) > 3
);
```

---

## 5.5 Transaktionen

### ACID Garantien

**Atomicity:** Alles oder nichts  
**Consistency:** Constraints werden eingehalten  
**Isolation:** Transaktionen sehen sich nicht gegenseitig  
**Durability:** Commits sind permanent  

### Transaction Beispiel

```python
from themis_client import ThemisDB

db = ThemisDB("localhost:8765")

# Start Transaction
tx = db.begin_transaction()

try:
    # 1. Reduce stock
    tx.execute("""
        UPDATE products
        SET stock = stock - 1
        WHERE id = ? AND stock > 0
    """, [product_id])
    
    # 2. Create order
    tx.execute("""
        INSERT INTO orders (customer_id, product_id, quantity, price)
        VALUES (?, ?, ?, ?)
    """, [customer_id, product_id, 1, price])
    
    # 3. Charge customer
    tx.execute("""
        UPDATE customers
        SET balance = balance - ?
        WHERE id = ? AND balance >= ?
    """, [price, customer_id, price])
    
    # All OK → Commit
    tx.commit()
    print("Order successful!")
    
except Exception as e:
    # Error → Rollback
    tx.rollback()
    print(f"Order failed: {e}")
```

**Was passiert intern:**

```
T1: BEGIN TRANSACTION (snapshot @ version 100)
  → UPDATE products ... (write intent @ version 101)
  → INSERT orders ... (write intent @ version 102)
  → UPDATE customers ... (write intent @ version 103)
  → COMMIT
    → All write intents become visible @ version 104

Falls Fehler:
  → ROLLBACK
    → All write intents discarded
    → Daten unverändert
```

### Isolation Levels

ThemisDB verwendet **Snapshot Isolation**:

```python
# Transaction 1
tx1 = db.begin_transaction()
tx1.execute("UPDATE products SET price = 100 WHERE id = 1")

# Transaction 2 (parallel)
tx2 = db.begin_transaction()
result = tx2.execute("SELECT price FROM products WHERE id = 1")
print(result)  # → Alter Wert! (z.B. 90)

# TX1 committed
tx1.commit()

# TX2 sieht immernoch alten Wert (Snapshot Isolation)
result = tx2.execute("SELECT price FROM products WHERE id = 1")
print(result)  # → Immernoch 90!

tx2.commit()

# Neue Transaction sieht neuen Wert
tx3 = db.begin_transaction()
result = tx3.execute("SELECT price FROM products WHERE id = 1")
print(result)  # → 100
```

---

## 5.6 Normalisierung

### Problem: Redundanz

**Nicht-normalisiert:**
```
orders
+----+----------+-------------+------------+----------+-------+
| id | customer | cust_email  | product    | quantity | price |
+----+----------+-------------+------------+----------+-------+
|  1 | Alice    | a@email.com | Laptop     |        1 |  1200 |
|  2 | Alice    | a@email.com | Mouse      |        2 |    25 |
|  3 | Bob      | b@email.com | Keyboard   |        1 |    80 |
+----+----------+-------------+------------+----------+-------+
```

**Probleme:**
- Customer email wird wiederholt (Update Anomaly)
- Product-Namen inkonsistent ("Laptop" vs "laptop")
- Keine Constraints möglich

### Normalisierung: 1NF, 2NF, 3NF

**1. Normal Form (1NF):** Atomare Werte, keine Arrays

```sql
-- ❌ Nicht 1NF
CREATE TABLE orders (
  id INT,
  products TEXT  -- "Laptop, Mouse, Keyboard"
);

-- ✅ 1NF
CREATE TABLE orders (
  id INT,
  product_id INT
);
```

**2. Normal Form (2NF):** Alle Nicht-Key-Attribute hängen vom ganzen Key ab

```sql
-- ❌ Nicht 2NF
CREATE TABLE order_items (
  order_id INT,
  product_id INT,
  quantity INT,
  product_name VARCHAR(255),  -- Hängt nur von product_id ab!
  PRIMARY KEY (order_id, product_id)
);

-- ✅ 2NF: Separate products table
CREATE TABLE products (
  id INT PRIMARY KEY,
  name VARCHAR(255)
);

CREATE TABLE order_items (
  order_id INT,
  product_id INT,
  quantity INT,
  PRIMARY KEY (order_id, product_id),
  FOREIGN KEY (product_id) REFERENCES products(id)
);
```

**3. Normal Form (3NF):** Keine transitiven Abhängigkeiten

```sql
-- ❌ Nicht 3NF
CREATE TABLE products (
  id INT PRIMARY KEY,
  category_id INT,
  category_name VARCHAR(255)  -- Hängt von category_id ab!
);

-- ✅ 3NF: Separate categories table
CREATE TABLE categories (
  id INT PRIMARY KEY,
  name VARCHAR(255)
);

CREATE TABLE products (
  id INT PRIMARY KEY,
  category_id INT,
  FOREIGN KEY (category_id) REFERENCES categories(id)
);
```

### Denormalisierung für Performance

Manchmal ist **bewusste** Denormalisierung sinnvoll:

```sql
-- Denormalisiert für schnelle Queries
CREATE TABLE order_summary (
  order_id INT PRIMARY KEY,
  customer_id INT,
  customer_name VARCHAR(255),  -- Denormalisiert!
  item_count INT,               -- Computed!
  total DECIMAL(10, 2),         -- Computed!
  created_at TIMESTAMP
);
```

**Trade-off:**
- ✅ Queries sind schneller (kein JOIN nötig)
- ❌ Updates sind komplexer (mehrere Tables updaten)
- ❌ Mehr Storage (Redundanz)

---

## 5.7 Indexes

### Warum Indexes?

**Ohne Index:**
```sql
SELECT * FROM products WHERE name = 'Laptop';
-- → Scannt ALLE Zeilen (Seq Scan): O(n)
```

**Mit Index:**
```sql
CREATE INDEX idx_products_name ON products(name);

SELECT * FROM products WHERE name = 'Laptop';
-- → Nutzt Index (Index Scan): O(log n)
```

**Performance-Unterschied:**
- 1 Million rows: Seq Scan ~100ms, Index Scan ~0.1ms
- **1000x schneller!**

### Index-Arten

**1. B-Tree Index (Standard):**
```sql
CREATE INDEX idx_products_price ON products(price);

-- Gut für:
SELECT * FROM products WHERE price > 100;
SELECT * FROM products WHERE price BETWEEN 50 AND 150;
SELECT * FROM products ORDER BY price;
```

**2. Unique Index:**
```sql
CREATE UNIQUE INDEX idx_users_email ON users(email);

-- Verhindert Duplikate automatisch
INSERT INTO users (email) VALUES ('test@example.com');  -- OK
INSERT INTO users (email) VALUES ('test@example.com');  -- ERROR!
```

**3. Composite Index:**
```sql
CREATE INDEX idx_orders_cust_date ON orders(customer_id, created_at);

-- Gut für:
SELECT * FROM orders WHERE customer_id = 123 AND created_at > '2025-01-01';
SELECT * FROM orders WHERE customer_id = 123;  -- Auch OK!

-- Nicht gut für:
SELECT * FROM orders WHERE created_at > '2025-01-01';  -- Index nicht nutzbar!
```

**4. Partial Index:**
```sql
CREATE INDEX idx_orders_pending ON orders(created_at)
WHERE status = 'pending';

-- Kleiner Index nur für pending orders
SELECT * FROM orders WHERE status = 'pending' AND created_at > '2025-01-01';
```

### Index Best Practices

**✅ Index erstellen für:**
- Primary Keys (automatisch)
- Foreign Keys (manuell)
- WHERE-Spalten in häufigen Queries
- ORDER BY-Spalten
- JOIN-Spalten

**❌ Zu viele Indexes vermeiden:**
- Jeder Index verlangsamt INSERT/UPDATE/DELETE
- Jeder Index braucht Storage
- **Faustregel:** Max 5-7 Indexes pro Table

---

## 5.8 Praxisbeispiel 1: Inventory System

### Überblick

Ein **Lagerverwaltungssystem** für ein kleines bis mittelgroßes Unternehmen:
- Products mit Categories
- Warehouses (mehrere Standorte)
- Stock Levels pro Warehouse
- Stock Movements (Ein-/Ausgang)
- Suppliers

**Location:** `examples/04_inventory_system/`

### Datenmodell

```sql
-- Categories
CREATE TABLE categories (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255) NOT NULL UNIQUE,
  description TEXT
);

-- Products
CREATE TABLE products (
  id INT PRIMARY KEY AUTO_INCREMENT,
  sku VARCHAR(50) NOT NULL UNIQUE,
  name VARCHAR(255) NOT NULL,
  description TEXT,
  category_id INT NOT NULL,
  unit_price DECIMAL(10, 2) NOT NULL CHECK (unit_price >= 0),
  min_stock_level INT DEFAULT 0,
  created_at TIMESTAMP DEFAULT NOW(),
  
  FOREIGN KEY (category_id) REFERENCES categories(id),
  INDEX idx_products_category (category_id),
  INDEX idx_products_sku (sku)
);

-- Warehouses
CREATE TABLE warehouses (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255) NOT NULL,
  location VARCHAR(255),
  capacity INT
);

-- Stock Levels
CREATE TABLE stock_levels (
  product_id INT NOT NULL,
  warehouse_id INT NOT NULL,
  quantity INT NOT NULL DEFAULT 0 CHECK (quantity >= 0),
  last_updated TIMESTAMP DEFAULT NOW(),
  
  PRIMARY KEY (product_id, warehouse_id),
  FOREIGN KEY (product_id) REFERENCES products(id),
  FOREIGN KEY (warehouse_id) REFERENCES warehouses(id)
);

-- Stock Movements
CREATE TABLE stock_movements (
  id INT PRIMARY KEY AUTO_INCREMENT,
  product_id INT NOT NULL,
  warehouse_id INT NOT NULL,
  quantity INT NOT NULL,  -- Positive = Eingang, Negative = Ausgang
  movement_type ENUM('purchase', 'sale', 'transfer', 'adjustment'),
  reference VARCHAR(255),
  created_at TIMESTAMP DEFAULT NOW(),
  
  FOREIGN KEY (product_id) REFERENCES products(id),
  FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
  INDEX idx_movements_product (product_id),
  INDEX idx_movements_warehouse (warehouse_id),
  INDEX idx_movements_date (created_at)
);

-- Suppliers
CREATE TABLE suppliers (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255) NOT NULL,
  contact_email VARCHAR(255),
  contact_phone VARCHAR(50)
);

-- Product Suppliers (Many-to-Many)
CREATE TABLE product_suppliers (
  product_id INT NOT NULL,
  supplier_id INT NOT NULL,
  supplier_sku VARCHAR(50),
  unit_cost DECIMAL(10, 2),
  
  PRIMARY KEY (product_id, supplier_id),
  FOREIGN KEY (product_id) REFERENCES products(id),
  FOREIGN KEY (supplier_id) REFERENCES suppliers(id)
);
```

### Häufige Queries

**1. Total Stock pro Product:**
```python
def get_total_stock(product_id):
    return db.query("""
        SELECT 
          p.name,
          SUM(sl.quantity) AS total_stock
        FROM products p
        LEFT JOIN stock_levels sl ON p.id = sl.product_id
        WHERE p.id = ?
        GROUP BY p.id, p.name
    """, [product_id])
```

**2. Low Stock Alert:**
```python
def get_low_stock_products():
    return db.query("""
        SELECT 
          p.sku,
          p.name,
          SUM(sl.quantity) AS total_stock,
          p.min_stock_level
        FROM products p
        LEFT JOIN stock_levels sl ON p.id = sl.product_id
        GROUP BY p.id
        HAVING SUM(sl.quantity) < p.min_stock_level
        ORDER BY total_stock ASC
    """)
```

**3. Stock Movement History:**
```python
def get_movement_history(product_id, days=30):
    return db.query("""
        SELECT 
          sm.created_at,
          sm.movement_type,
          sm.quantity,
          w.name AS warehouse,
          sm.reference
        FROM stock_movements sm
        JOIN warehouses w ON sm.warehouse_id = w.id
        WHERE sm.product_id = ?
          AND sm.created_at >= DATE_SUB(NOW(), INTERVAL ? DAY)
        ORDER BY sm.created_at DESC
    """, [product_id, days])
```

### Stock Movement Transaction

```python
def record_purchase(product_id, warehouse_id, quantity, supplier_id, cost):
    """Purchase new stock from supplier"""
    tx = db.begin_transaction()
    try:
        # 1. Record movement
        tx.execute("""
            INSERT INTO stock_movements 
            (product_id, warehouse_id, quantity, movement_type, reference)
            VALUES (?, ?, ?, 'purchase', ?)
        """, [product_id, warehouse_id, quantity, f"PO-{supplier_id}-{datetime.now().timestamp()}"])
        
        # 2. Update stock level
        tx.execute("""
            INSERT INTO stock_levels (product_id, warehouse_id, quantity)
            VALUES (?, ?, ?)
            ON CONFLICT (product_id, warehouse_id) DO UPDATE
            SET 
              quantity = stock_levels.quantity + EXCLUDED.quantity,
              last_updated = NOW()
        """, [product_id, warehouse_id, quantity])
        
        # 3. Update product cost (optional)
        tx.execute("""
            UPDATE products
            SET unit_price = ?
            WHERE id = ?
        """, [cost, product_id])
        
        tx.commit()
        return {"success": True}
        
    except Exception as e:
        tx.rollback()
        return {"success": False, "error": str(e)}
```

### Reporting: Value per Warehouse

```sql
SELECT 
  w.name AS warehouse,
  COUNT(DISTINCT sl.product_id) AS product_count,
  SUM(sl.quantity) AS total_units,
  SUM(sl.quantity * p.unit_price) AS total_value
FROM warehouses w
LEFT JOIN stock_levels sl ON w.id = sl.warehouse_id
LEFT JOIN products p ON sl.product_id = p.id
GROUP BY w.id, w.name
ORDER BY total_value DESC;
```

**Output:**
```
+------------------+---------------+-------------+-------------+
| warehouse        | product_count | total_units | total_value |
+------------------+---------------+-------------+-------------+
| Main Warehouse   |           250 |       5,420 |  €125,000   |
| Storage Berlin   |           180 |       3,210 |   €78,500   |
| Distribution Hub |           120 |       1,890 |   €42,300   |
+------------------+---------------+-------------+-------------+
```

---

## 5.9 Praxisbeispiel 2: Expense Tracker

### Überblick

Ein **Ausgabenverwaltungssystem** für persönliche Finanzen oder kleine Teams:
- Expenses mit Categories
- Budgets pro Category
- Recurring Expenses
- Multi-Currency Support
- Reports und Analytics

**Location:** `examples/12_expense_tracker/`

### Datenmodell

```sql
-- Categories
CREATE TABLE expense_categories (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(100) NOT NULL UNIQUE,
  color VARCHAR(7),  -- Hex color #FF5733
  icon VARCHAR(50)
);

-- Expenses
CREATE TABLE expenses (
  id INT PRIMARY KEY AUTO_INCREMENT,
  description VARCHAR(255) NOT NULL,
  amount DECIMAL(10, 2) NOT NULL CHECK (amount > 0),
  currency VARCHAR(3) DEFAULT 'EUR',
  category_id INT NOT NULL,
  expense_date DATE NOT NULL,
  payment_method ENUM('cash', 'credit_card', 'debit_card', 'transfer') DEFAULT 'cash',
  receipt_url VARCHAR(500),
  notes TEXT,
  created_at TIMESTAMP DEFAULT NOW(),
  
  FOREIGN KEY (category_id) REFERENCES expense_categories(id),
  INDEX idx_expenses_date (expense_date),
  INDEX idx_expenses_category (category_id)
);

-- Budgets
CREATE TABLE budgets (
  id INT PRIMARY KEY AUTO_INCREMENT,
  category_id INT NOT NULL,
  amount DECIMAL(10, 2) NOT NULL CHECK (amount > 0),
  period ENUM('daily', 'weekly', 'monthly', 'yearly') DEFAULT 'monthly',
  start_date DATE NOT NULL,
  end_date DATE,
  
  FOREIGN KEY (category_id) REFERENCES expense_categories(id),
  UNIQUE KEY (category_id, start_date)
);

-- Recurring Expenses
CREATE TABLE recurring_expenses (
  id INT PRIMARY KEY AUTO_INCREMENT,
  description VARCHAR(255) NOT NULL,
  amount DECIMAL(10, 2) NOT NULL,
  category_id INT NOT NULL,
  frequency ENUM('daily', 'weekly', 'monthly', 'yearly') NOT NULL,
  start_date DATE NOT NULL,
  end_date DATE,
  last_generated DATE,
  
  FOREIGN KEY (category_id) REFERENCES expense_categories(id)
);
```

### Häufige Queries

**1. Expenses für aktuellen Monat:**
```python
def get_monthly_expenses(year, month):
    return db.query("""
        SELECT 
          e.expense_date,
          e.description,
          e.amount,
          c.name AS category,
          e.payment_method
        FROM expenses e
        JOIN expense_categories c ON e.category_id = c.id
        WHERE YEAR(e.expense_date) = ?
          AND MONTH(e.expense_date) = ?
        ORDER BY e.expense_date DESC
    """, [year, month])
```

**2. Budget Status:**
```python
def get_budget_status(year, month):
    return db.query("""
        SELECT 
          c.name AS category,
          b.amount AS budget,
          COALESCE(SUM(e.amount), 0) AS spent,
          b.amount - COALESCE(SUM(e.amount), 0) AS remaining,
          (COALESCE(SUM(e.amount), 0) / b.amount * 100) AS percent_used
        FROM budgets b
        JOIN expense_categories c ON b.category_id = c.id
        LEFT JOIN expenses e ON e.category_id = c.category_id
          AND YEAR(e.expense_date) = ?
          AND MONTH(e.expense_date) = ?
        WHERE b.period = 'monthly'
          AND b.start_date <= ?
          AND (b.end_date IS NULL OR b.end_date >= ?)
        GROUP BY b.id, c.name, b.amount
    """, [year, month, f"{year}-{month:02d}-01", f"{year}-{month:02d}-01"])
```

**3. Top Categories:**
```python
def get_top_categories(year):
    return db.query("""
        SELECT 
          c.name,
          COUNT(e.id) AS expense_count,
          SUM(e.amount) AS total_amount
        FROM expense_categories c
        JOIN expenses e ON c.id = e.category_id
        WHERE YEAR(e.expense_date) = ?
        GROUP BY c.id, c.name
        ORDER BY total_amount DESC
        LIMIT 10
    """, [year])
```

### Add Expense Transaction

```python
def add_expense(description, amount, category_id, expense_date, payment_method):
    """Add new expense and check budget"""
    tx = db.begin_transaction()
    try:
        # 1. Insert expense
        result = tx.execute("""
            INSERT INTO expenses 
            (description, amount, category_id, expense_date, payment_method)
            VALUES (?, ?, ?, ?, ?)
        """, [description, amount, category_id, expense_date, payment_method])
        
        expense_id = result.last_insert_id
        
        # 2. Check budget
        year, month = expense_date.year, expense_date.month
        budget_status = tx.query("""
            SELECT 
              b.amount AS budget,
              COALESCE(SUM(e.amount), 0) AS spent
            FROM budgets b
            LEFT JOIN expenses e ON e.category_id = b.category_id
              AND YEAR(e.expense_date) = ?
              AND MONTH(e.expense_date) = ?
            WHERE b.category_id = ?
              AND b.period = 'monthly'
            GROUP BY b.amount
        """, [year, month, category_id])
        
        warning = None
        if budget_status and budget_status[0]['spent'] > budget_status[0]['budget']:
            warning = f"Budget exceeded! Spent: {budget_status[0]['spent']}, Budget: {budget_status[0]['budget']}"
        
        tx.commit()
        return {"success": True, "expense_id": expense_id, "warning": warning}
        
    except Exception as e:
        tx.rollback()
        return {"success": False, "error": str(e)}
```

### Recurring Expenses Generation

```python
def generate_recurring_expenses():
    """Generate expenses from recurring templates"""
    tx = db.begin_transaction()
    generated = []
    
    try:
        # Find due recurring expenses
        recurring = tx.query("""
            SELECT * FROM recurring_expenses
            WHERE (last_generated IS NULL OR last_generated < CURDATE())
              AND (end_date IS NULL OR end_date >= CURDATE())
        """)
        
        for rec in recurring:
            # Calculate next date
            next_date = calculate_next_date(rec['frequency'], rec['last_generated'] or rec['start_date'])
            
            if next_date <= datetime.now().date():
                # Generate expense
                tx.execute("""
                    INSERT INTO expenses 
                    (description, amount, category_id, expense_date, payment_method)
                    VALUES (?, ?, ?, ?, 'transfer')
                """, [rec['description'], rec['amount'], rec['category_id'], next_date])
                
                # Update last_generated
                tx.execute("""
                    UPDATE recurring_expenses
                    SET last_generated = ?
                    WHERE id = ?
                """, [next_date, rec['id']])
                
                generated.append(rec['description'])
        
        tx.commit()
        return {"success": True, "generated": generated}
        
    except Exception as e:
        tx.rollback()
        return {"success": False, "error": str(e)}
```

### Analytics: Monthly Trend

```sql
SELECT 
  DATE_FORMAT(expense_date, '%Y-%m') AS month,
  COUNT(*) AS expense_count,
  SUM(amount) AS total_spent,
  AVG(amount) AS avg_expense
FROM expenses
WHERE expense_date >= DATE_SUB(CURDATE(), INTERVAL 12 MONTH)
GROUP BY month
ORDER BY month;
```

**Output:**
```
+---------+---------------+-------------+-------------+
| month   | expense_count | total_spent | avg_expense |
+---------+---------------+-------------+-------------+
| 2024-01 |            45 |    €1,234   |      €27    |
| 2024-02 |            52 |    €1,456   |      €28    |
| 2024-03 |            48 |    €1,189   |      €25    |
| ...     |           ... |       ...   |       ...   |
+---------+---------------+-------------+-------------+
```

---

## 5.10 Performance Best Practices

### 1. Use Indexes Wisely

```sql
-- ❌ Slow: No index
SELECT * FROM expenses WHERE expense_date > '2025-01-01';

-- ✅ Fast: With index
CREATE INDEX idx_expenses_date ON expenses(expense_date);
SELECT * FROM expenses WHERE expense_date > '2025-01-01';
```

### 2. Avoid SELECT *

```sql
-- ❌ Transfers mehr Daten als nötig
SELECT * FROM products;

-- ✅ Nur benötigte Spalten
SELECT id, name, price FROM products;
```

### 3. Use LIMIT für große Result Sets

```sql
-- ❌ Könnte Millionen Rows returnen
SELECT * FROM orders ORDER BY created_at DESC;

-- ✅ Pagination
SELECT * FROM orders ORDER BY created_at DESC LIMIT 50 OFFSET 0;
```

### 4. Batch Inserts

```python
# ❌ Slow: Individual inserts
for product in products:
    db.execute("INSERT INTO products (name, price) VALUES (?, ?)", 
               [product.name, product.price])

# ✅ Fast: Batch insert
db.execute("""
    INSERT INTO products (name, price) VALUES 
    (?, ?), (?, ?), (?, ?), ...
""", [p1.name, p1.price, p2.name, p2.price, ...])
```

### 5. Use Transactions für Bulk Operations

```python
# ❌ Slow: Auto-commit nach jedem Statement
for i in range(1000):
    db.execute("INSERT INTO logs (...) VALUES (...)")

# ✅ Fast: Eine Transaction
tx = db.begin_transaction()
for i in range(1000):
    tx.execute("INSERT INTO logs (...) VALUES (...)")
tx.commit()
```

### 6. Analyze Query Plans

```sql
EXPLAIN SELECT * FROM orders o
JOIN customers c ON o.customer_id = c.id
WHERE o.created_at > '2025-01-01';
```

**Output:**
```
+----+------------+-------+------+---------------+
| id | table      | type  | key  | rows          |
+----+------------+-------+------+---------------+
|  1 | orders     | range | idx  | 1000          |
|  1 | customers  | ref   | pk   | 1             |
+----+------------+-------+------+---------------+
```

---

## 5.11 Zusammenfassung

In diesem Kapitel haben Sie gelernt:

✅ **Relationales Modell:** Tabellen, Schemas, Constraints  
✅ **SQL DDL:** CREATE TABLE, Datentypen, Constraints  
✅ **SQL DML:** INSERT, UPDATE, DELETE, UPSERT  
✅ **Queries:** SELECT, WHERE, JOIN, Subqueries, Aggregation  
✅ **Transaktionen:** ACID, Isolation, Commit/Rollback  
✅ **Normalisierung:** 1NF, 2NF, 3NF, Denormalisierung  
✅ **Indexes:** B-Tree, Unique, Composite, Performance  
✅ **Praxis:** Inventory System & Expense Tracker  

### Key Takeaways

1. **Relational ist perfekt für strukturierte Business-Daten**
2. **Constraints garantieren Datenintegrität**
3. **Normalisierung verhindert Redundanz**
4. **Denormalisierung kann Performance verbessern**
5. **Indexes sind essentiell für Query-Performance**
6. **Transaktionen garantieren Konsistenz**

### Nächster Schritt

Sie verstehen jetzt relationale Daten in ThemisDB. Im nächsten Kapitel lernen Sie **Graph-Datenbanken** kennen - perfekt für Netzwerk-Daten und Beziehungen.

**[Kapitel 6: Graph-Datenbanken →](chapter_06_graph.md)**

---

## Weiterführende Ressourcen

- **AQL Reference:** [../de/aql/README.md](../de/aql/README.md)
- **Schema Design:** [../de/guides/guides_schema_design.md](../de/guides/guides_schema_design.md)
- **Transaction Guide:** [../de/features/features_transactions.md](../de/features/features_transactions.md)
- **Inventory System Code:** [../../examples/04_inventory_system/](../../examples/04_inventory_system/)
- **Expense Tracker Code:** [../../examples/12_expense_tracker/](../../examples/12_expense_tracker/)

---

**Kapitel 5 von 30** | **Teil II: Datenmodelle** | **~9.500 Wörter**
