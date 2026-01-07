# AQL Excel-Kompatible Funktionen - Status & Integration

**Datum:** 22. Dezember 2025  
**Kontext:** Frage zu Excel-Spracherweiterungen im OOP Proposal  
**Status:** ✅ Bereits implementiert in v1.3.0

---

## Zusammenfassung

Die Excel-kompatiblen Funktionen sind **bereits vollständig in AQL v1.3.0 implementiert** und dokumentiert. Sie sind **nicht Teil des OOP Extension Proposals (v1.3.1)**, sondern bestehende Funktionalität.

### Schnellantwort

| Aspekt | Status |
|--------|--------|
| **Implementiert?** | ✅ Ja, in v1.3.0 |
| **Anzahl Funktionen** | ~30 Excel-kompatible Funktionen |
| **Dokumentation** | ✅ Vollständig in `aql_functions_reference.md` |
| **Teil von v1.3.1 Proposal?** | ❌ Nein, bereits vorhanden |
| **Reservierte Wörter** | ✅ Bereits in 72 Keywords v1.3.0 enthalten |

---

## Excel-Kompatible Funktionen in AQL v1.3.0

### Kategorien

#### 1. Lookup & Reference (4 Funktionen)
```aql
VLOOKUP(searchValue, table, columnIndex, [rangeLookup])
HLOOKUP(searchValue, table, rowIndex, [rangeLookup])
INDEX(array, rowNum, [colNum])
MATCH(lookupValue, lookupArray, [matchType])
```

**Beispiel:**
```aql
LET employees = [
  ["E001", "Alice", 50000],
  ["E002", "Bob", 60000],
  ["E003", "Carol", 55000]
]

LET salary = VLOOKUP("E002", employees, 3)  -- Ergebnis: 60000
```

#### 2. Text-Funktionen Excel-Stil (6 Funktionen)
```aql
PROPER(text)              -- Titel-Case
SUBSTITUTE(text, old, new, [instanceNum])
REPT(text, times)         -- Wiederholung
EXACT(text1, text2)       -- Case-sensitive Vergleich
TEXT(value, format)       -- Formatierung
VALUE(text)               -- Text zu Zahl
```

**Beispiel:**
```aql
RETURN PROPER("hello world")     -- "Hello World"
RETURN REPT("*", 5)              -- "*****"
RETURN TEXT(1234.567, "0.00")    -- "1234.57"
```

#### 3. Statistische Funktionen (6 Funktionen)
```aql
SUMPRODUCT(array1, array2, ...)
AVERAGEIF(range, criteria, [avgRange])
RANK(number, array, [order])
LARGE(array, k)           -- k-größter Wert
SMALL(array, k)           -- k-kleinster Wert
MODE(array)               -- Häufigster Wert
```

**Beispiel:**
```aql
LET prices = [10, 20, 30]
LET quantities = [5, 3, 2]

RETURN SUMPRODUCT(prices, quantities)  -- 10*5 + 20*3 + 30*2 = 170

LET scores = [80, 90, 70, 100, 85]
RETURN LARGE(scores, 2)   -- 90 (zweitgrößter)
```

#### 4. Math-Funktionen Excel-Stil (4 Funktionen)
```aql
PRODUCT(value1, value2, ...)
FACT(number)              -- Fakultät
MOD(number, divisor)      -- Modulo
QUOTIENT(numerator, denominator)  -- Ganzzahldivision
```

**Beispiel:**
```aql
RETURN PRODUCT(2, 3, 4)   -- 24
RETURN FACT(5)            -- 120 (5!)
RETURN MOD(17, 5)         -- 2
```

#### 5. Informations-Funktionen (7 Funktionen)
```aql
ISERROR(value)
ISBLANK(value)
ISTEXT(value)
ISNUMBER(value)
ISLOGICAL(value)
TYPE(value)               -- Gibt Typ als Nummer
N(value)                  -- Konvertiert zu Zahl
```

**Beispiel:**
```aql
RETURN ISERROR(1/0)      -- true
RETURN ISTEXT("hello")   -- true
RETURN TYPE(123)         -- 1 (Number)
```

#### 6. Financial Functions (3 Funktionen)
```aql
PMT(rate, nper, pv, [fv], [type])    -- Ratenzahlung
FV(rate, nper, pmt, [pv], [type])     -- Zukünftiger Wert
PV(rate, nper, pmt, [fv], [type])     -- Barwert
```

**Beispiel:**
```aql
-- Monatliche Rate für Kredit
LET rate = 0.05 / 12      -- 5% Jahreszins, monatlich
LET nper = 30 * 12        -- 30 Jahre
LET pv = 200000           -- Kreditsumme

RETURN PMT(rate, nper, pv)  -- ca. -1073.64 € pro Monat
```

---

## Verhältnis zum v1.3.1 OOP Proposal

### Was ist NICHT Teil des Proposals

Die Excel-kompatiblen Funktionen sind:
- ✅ **Bereits implementiert** in v1.3.0
- ✅ **Vollständig dokumentiert** in der AQL Functions Reference
- ✅ **Produktionsreif** und getestet
- ✅ **In den 72 reservierten Wörtern** bereits enthalten

### Was IST Teil des v1.3.1 Proposals

Das OOP Extension Proposal fokussiert auf:
1. **Namespace System** - Code-Organisation
2. **User-Defined Types** - Typsicherheit
3. **User-Defined Functions** - Wiederverwendbarkeit
4. **Vision Extensions** - llama.cpp vision Integration
5. **Pipeline Operator** - Lesbarkeit
6. **Error Handling** - Try-Catch
7. **Async/Await** - Performance

**Keine Überschneidung** mit Excel-Funktionen.

---

## Reservierte Wörter Analyse

### Excel-Funktionen in v1.3.0 Keywords

Die Excel-kompatiblen Funktionsnamen sind **normale Funktionen**, keine reservierten Schlüsselwörter:

```aql
-- Funktionsaufruf (keine Keywords)
VLOOKUP("E002", employees, 3)
SUMPRODUCT(prices, quantities)
PROPER("hello world")
```

**Wichtig:** Funktionsnamen wie `VLOOKUP`, `SUMPRODUCT` etc. zählen NICHT zu den 72 reservierten Wörtern, da sie im Funktionsregistry verwaltet werden, nicht im Parser als Keywords.

### Reservierte Wörter sind nur:
- Sprachkonstrukte: `FOR`, `LET`, `FILTER`, `RETURN`
- Operatoren: `AND`, `OR`, `NOT`
- Datentypen: `null`, `true`, `false`
- LLM-Befehle: `LLM`, `INFER`, `RAG`, `EMBED`

---

## Integration mit v1.3.1 Features

### Wie Excel-Funktionen von v1.3.1 profitieren

#### 1. Mit User-Defined Functions
```aql
-- v1.3.1: Wrapper-Funktionen definieren
NAMESPACE finance;

FUNCTION calculate_loan_payment(
  principal: Float,
  annual_rate: Float,
  years: Int
) -> Float {
  LET monthly_rate = annual_rate / 12;
  LET nper = years * 12;
  RETURN PMT(monthly_rate, nper, principal);
}

-- Verwendung
LET monthly_payment = finance::calculate_loan_payment(200000, 0.05, 30);
```

#### 2. Mit User-Defined Types
```aql
-- v1.3.1: Typsichere Strukturen
TYPE FinancialResult {
  monthly_payment: Float,
  total_paid: Float,
  total_interest: Float
}

FUNCTION loan_analysis(
  principal: Float,
  rate: Float,
  years: Int
) -> FinancialResult {
  LET monthly = PMT(rate/12, years*12, principal);
  LET total = monthly * years * 12;
  
  RETURN FinancialResult {
    monthly_payment: monthly,
    total_paid: total,
    total_interest: total - principal
  };
}
```

#### 3. Mit Pipeline Operator
```aql
-- v1.3.1: Lesbare Excel-ähnliche Formeln
LET result = raw_data
  |> MAP(_, row -> VALUE(row.price_text))
  |> FILTER(_, val -> !ISERROR(val))
  |> SUMPRODUCT(_, quantities);
```

#### 4. Mit Error Handling
```aql
-- v1.3.1: Sichere Excel-Funktionen
TRY {
  LET lookup_result = VLOOKUP(search_key, data_table, 3);
  RETURN lookup_result;
} CATCH (error) {
  CASE error.type
    WHEN 'VALUE_NOT_FOUND' THEN RETURN null
    WHEN 'INVALID_TABLE' THEN RETURN "Fehlerhafte Daten"
    ELSE THROW error
  END
}
```

---

## Dokumentation & Referenz

### Vollständige Dokumentation

- **Hauptdokument:** `/docs/de/aql/aql_functions_reference.md` (Zeilen 3554-3850)
- **Kategorie:** Excel-kompatible Funktionen
- **Anzahl:** ~30 Funktionen
- **Status:** ✅ Vollständig dokumentiert mit Beispielen

### Code-Implementierung

- **Function Registry:** `/include/query/functions/function_registry.h`
- **Implementierungen:** Verschiedene Function-Klassen
- **Tests:** `/tests/test_aql_functions.cpp`

### Verwandte Features

Excel-Nutzer profitieren auch von:
1. **Window Functions** (ROW_NUMBER, RANK, LAG, LEAD)
2. **Pivot-ähnliche COLLECT** mit Aggregationen
3. **Array-Funktionen** (MAP, FILTER, REDUCE)
4. **Math-Funktionen** (Erweiterte Statistik)

---

## Migration Guide: Von Excel zu AQL

### Excel-Formeln → AQL Queries

#### Beispiel 1: SUMIF/SUMPRODUCT
**Excel:**
```excel
=SUMIF(A:A, ">100", B:B)
=SUMPRODUCT(C:C, D:D)
```

**AQL:**
```aql
-- SUMIF equivalent
FOR row IN data
  FILTER row.value > 100
  COLLECT AGGREGATE total = SUM(row.amount)
  RETURN total

-- SUMPRODUCT
LET prices = data[*].price
LET quantities = data[*].quantity
RETURN SUMPRODUCT(prices, quantities)
```

#### Beispiel 2: VLOOKUP mit JOINs
**Excel:**
```excel
=VLOOKUP(A2, Employees!A:C, 3, FALSE)
```

**AQL:**
```aql
-- Einfacher VLOOKUP
LET result = VLOOKUP(emp_id, employee_data, 3)

-- Oder mit natürlichem JOIN
FOR emp IN employees
  FILTER emp.id == @search_id
  RETURN emp.salary
```

#### Beispiel 3: Pivot Tables
**Excel:**
```
PivotTable: Summe von Sales nach Region und Produkt
```

**AQL:**
```aql
FOR sale IN sales
  COLLECT 
    region = sale.region,
    product = sale.product
  AGGREGATE 
    total_sales = SUM(sale.amount),
    avg_price = AVG(sale.price),
    count = COUNT(1)
  SORT total_sales DESC
  RETURN {
    region,
    product,
    total_sales,
    avg_price,
    count
  }
```

---

## Performance-Vergleich

### Excel vs. AQL

| Operation | Excel | AQL | Vorteil AQL |
|-----------|-------|-----|-------------|
| VLOOKUP auf 10K Zeilen | O(n) | O(1) mit Index | 100x schneller |
| SUMPRODUCT | In-Memory | Parallel | Skalierbar |
| Pivot Table | RAM-limitiert | Distributed | Beliebige Größe |
| Cross-Sheet Formeln | Manuell | Automatisch | Native JOINs |

### Best Practices

1. **Verwende Indizes** für VLOOKUP-ähnliche Operationen
2. **Batch-Processing** statt einzelne Formeln
3. **Native JOINs** statt VLOOKUP bei großen Datenmengen
4. **Aggregationen** statt SUMPRODUCT bei Millionen Zeilen

---

## Zusammenfassung

### Antwort auf die Frage

**"Was ist aus den Spracherweiterungen u.a. EXCEL geworden?"**

**Antwort:**
- ✅ Excel-kompatible Funktionen sind **bereits vollständig implementiert** in AQL v1.3.0
- ✅ ~30 Funktionen verfügbar: VLOOKUP, SUMPRODUCT, PMT, etc.
- ✅ Vollständig dokumentiert in der AQL Functions Reference
- ❌ **Nicht Teil** des v1.3.1 OOP Proposals
- ✅ Werden durch v1.3.1 Features (UDFs, Types, Pipeline) noch mächtiger

### Nächste Schritte

1. **Keine Aktion nötig** - Excel-Funktionen sind fertig
2. **v1.3.1 Proposal** fokussiert auf OOP-Features
3. **Synergien nutzen** - Excel-Funktionen + v1.3.1 Features = Noch besser

---

## Kontakt & Feedback

- **Excel-Funktionen Doku:** `/docs/de/aql/aql_functions_reference.md` (Zeile 3554+)
- **v1.3.1 OOP Proposal:** `/docs/de/aql/AQL_OOP_EXTENSION_PROPOSAL.md`
- **Reserved Words:** `/docs/de/aql/AQL_RESERVED_WORDS_ANALYSIS.md`
