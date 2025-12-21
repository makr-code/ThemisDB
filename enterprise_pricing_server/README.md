# ThemisDB Enterprise Pricing Server

Ein umfassendes Python/FastAPI-basiertes Abonnement- und Zahlungsverwaltungssystem für ThemisDB Enterprise und Hyperscaler Editionen.

## Features

### 🔐 Kundenverwaltung (CRUD)
- Kundenregistrierung mit E-Mail und Passwort
- JWT-basierte Authentifizierung
- Profilaktualisierung und -löschung
- Sichere Passwortspeicherung mit bcrypt

### 💳 Abonnementverwaltung
- Erstellung von Abonnements für verschiedene Tiers:
  - **Community**: Kostenlos (Single-Node)
  - **Enterprise**: €5.000/Monat (bis 100 Nodes)
  - **Hyperscaler**: €25.000/Monat (unbegrenzt)
  - **Reseller**: €15.000/Monat (konfigurierbar)
- Automatische Lizenzschlüssel-Generierung
- Abonnementstatus-Tracking (Aktiv, Ausstehend, Gekündigt, Abgelaufen)
- Automatische Ablaufprüfung

### 💰 Zahlungsverifizierung
- Bankenschnittstelle zur Zahlungsüberprüfung
- Unterstützung für SEPA, SWIFT, Stripe, PayPal (erweiterbar)
- Webhook-Endpunkt für automatische Zahlungsbenachrichtigungen
- Automatische Abonnement-Aktivierung nach erfolgreicher Zahlung
- Transaktions-Tracking und -Historie

### 🖥️ Tkinter Admin UI
- Grafische Benutzeroberfläche für Verwaltungsaufgaben
- Kundenregistrierung und -anmeldung
- Abonnementverwaltung
- Zahlungsüberwachung
- Preisübersicht

## Installation

### Voraussetzungen

- Python 3.10 oder höher
- pip package manager

### Setup

1. **Repository klonen:**
```bash
cd /home/runner/work/ThemisDB/ThemisDB/enterprise_pricing_server
```

2. **Virtuelle Umgebung erstellen:**
```bash
python -m venv venv
source venv/bin/activate  # Linux/macOS
# oder
venv\Scripts\activate  # Windows
```

3. **Dependencies installieren:**
```bash
pip install -r requirements.txt
```

4. **Umgebungsvariablen konfigurieren:**
```bash
cp .env.example .env
# Bearbeiten Sie .env mit Ihren Einstellungen
```

## Verwendung

### FastAPI Server starten

```bash
# Entwicklungsmodus mit Auto-Reload
python -m uvicorn enterprise_pricing_server.app:app --reload --host 0.0.0.0 --port 8000

# Oder direkt über app.py
python enterprise_pricing_server/app.py
```

Der Server läuft dann unter: http://localhost:8000

### API-Dokumentation

- **Swagger UI**: http://localhost:8000/docs
- **ReDoc**: http://localhost:8000/redoc

### Tkinter Admin UI starten

```bash
python enterprise_pricing_server/tkinter_admin.py
```

## API-Endpunkte

### Authentifizierung

- `POST /auth/register` - Neuen Kunden registrieren
- `POST /auth/login` - Anmelden (OAuth2 Form)
- `POST /auth/login-json` - Anmelden (JSON)
- `GET /auth/me` - Aktuellen Benutzer abrufen

### Kundenverwaltung

- `GET /customers/me` - Eigenes Profil abrufen
- `GET /customers/{id}` - Kunde nach ID abrufen
- `PUT /customers/{id}` - Kundeninformationen aktualisieren
- `DELETE /customers/{id}` - Kundenkonto löschen

### Abonnements

- `POST /subscriptions` - Neues Abonnement erstellen
- `GET /subscriptions` - Alle eigenen Abonnements abrufen
- `GET /subscriptions/{id}` - Abonnement nach ID abrufen
- `POST /subscriptions/{id}/cancel` - Abonnement kündigen
- `GET /subscriptions/{id}/status` - Abonnementstatus prüfen

### Zahlungen

- `POST /payments` - Neue Zahlung erstellen
- `POST /payments/{id}/initiate` - Zahlung mit Bank initiieren
- `GET /payments` - Alle eigenen Zahlungen abrufen
- `GET /payments/{id}` - Zahlung nach ID abrufen
- `POST /payments/{id}/verify` - Zahlung manuell verifizieren
- `POST /payments/webhook` - Webhook für Zahlungsbenachrichtigungen

### Sonstiges

- `GET /` - Root-Endpunkt
- `GET /health` - Gesundheitsprüfung
- `GET /pricing` - Preisübersicht für alle Tiers

## Beispiel-Workflows

### 1. Kundenregistrierung und -anmeldung

```bash
# Registrierung
curl -X POST http://localhost:8000/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "email": "kunde@example.com",
    "password": "sicheres-passwort",
    "organization_name": "Beispiel GmbH",
    "contact_name": "Max Mustermann",
    "country": "Germany"
  }'

# Anmeldung
curl -X POST http://localhost:8000/auth/login-json \
  -H "Content-Type: application/json" \
  -d '{
    "email": "kunde@example.com",
    "password": "sicheres-passwort"
  }'

# Antwort: {"access_token": "eyJ...", "token_type": "bearer"}
```

### 2. Abonnement erstellen

```bash
curl -X POST http://localhost:8000/subscriptions \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "tier": "enterprise",
    "max_nodes": 10,
    "billing_period_months": 12
  }'
```

### 3. Zahlung erstellen und verifizieren

```bash
# Zahlung erstellen
curl -X POST http://localhost:8000/payments \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "subscription_id": 1,
    "amount": 60000.0,
    "currency": "EUR",
    "payment_method": "bank_transfer"
  }'

# Zahlung initiieren (gibt Payment-URL zurück)
curl -X POST http://localhost:8000/payments/1/initiate \
  -H "Authorization: Bearer YOUR_TOKEN"

# Zahlung verifizieren
curl -X POST http://localhost:8000/payments/1/verify \
  -H "Authorization: Bearer YOUR_TOKEN"
```

### 4. Webhook-Integration (von Banksystem)

```bash
curl -X POST http://localhost:8000/payments/webhook \
  -H "Content-Type: application/json" \
  -d '{
    "transaction_id": "TX-ABC123",
    "status": "completed",
    "amount": 60000.0,
    "currency": "EUR",
    "external_payment_id": "STRIPE-XYZ789"
  }'
```

## Bankenschnittstelle

Die Bankenschnittstelle ist modular aufgebaut und kann einfach erweitert werden:

### Mock-Modus (Standard)

Standardmäßig läuft der Server im Mock-Modus für Entwicklung und Tests. Zahlungen werden automatisch als verifiziert markiert.

### Produktionsmodus

Für Produktion konfigurieren Sie in `.env`:

```env
BANKING_API_URL=https://api.your-bank.com
BANKING_API_KEY=your-api-key
```

### Unterstützte Payment Gateways

Der Server kann mit folgenden Systemen integriert werden:

- **Stripe**: Über Stripe API
- **PayPal**: Über PayPal REST API
- **SEPA Direct Debit**: Über europäische Banken-APIs
- **SWIFT**: Über internationale Banken-APIs
- **Custom**: Eigene Banking-API

## Konfiguration

### Umgebungsvariablen (.env)

```env
# Application
APP_NAME=ThemisDB Enterprise Pricing Server
VERSION=1.0.0
DEBUG=false

# Server
HOST=0.0.0.0
PORT=8000

# Database
DATABASE_URL=sqlite+aiosqlite:///./pricing_server.db

# Security
SECRET_KEY=your-secret-key-here-change-in-production
ALGORITHM=HS256
ACCESS_TOKEN_EXPIRE_MINUTES=1440

# Payment Provider
STRIPE_API_KEY=sk_test_...
STRIPE_WEBHOOK_SECRET=whsec_...

# Banking Interface
BANKING_API_URL=https://api.bank.com
BANKING_API_KEY=your-banking-api-key

# Pricing (EUR per month)
COMMUNITY_PRICE=0.0
ENTERPRISE_PRICE=5000.0
HYPERSCALER_PRICE=25000.0
RESELLER_PRICE=15000.0
```

## Datenbank

Der Server verwendet SQLite mit SQLAlchemy für Entwicklung. Für Produktion kann auf PostgreSQL oder MySQL umgestellt werden:

```env
# PostgreSQL
DATABASE_URL=postgresql+asyncpg://user:password@localhost/pricing_db

# MySQL
DATABASE_URL=mysql+aiomysql://user:password@localhost/pricing_db
```

### Datenbankschema

- **customers**: Kundeninformationen
- **subscriptions**: Abonnements mit Lizenzschlüsseln
- **payments**: Zahlungstransaktionen

## Sicherheit

### Best Practices

- ✅ JWT-basierte Authentifizierung
- ✅ Bcrypt-Passwort-Hashing
- ✅ SQL Injection Prevention (SQLAlchemy)
- ✅ CORS-Konfiguration
- ✅ Rate Limiting (über Reverse Proxy)
- ✅ HTTPS in Produktion (über Reverse Proxy)

### Produktions-Deployment

Für Produktion sollte der Server hinter einem Reverse Proxy (nginx, Caddy) mit:

- SSL/TLS-Termination
- Rate Limiting
- Request Filtering
- DDoS-Schutz

## Tests

```bash
# Unit Tests
pytest tests/

# Mit Coverage
pytest --cov=enterprise_pricing_server tests/
```

## Docker-Deployment

```dockerfile
FROM python:3.11-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

EXPOSE 8000

CMD ["uvicorn", "enterprise_pricing_server.app:app", "--host", "0.0.0.0", "--port", "8000"]
```

```bash
# Build
docker build -t themisdb-pricing-server .

# Run
docker run -d -p 8000:8000 \
  -e SECRET_KEY=your-secret \
  -e DATABASE_URL=postgresql://... \
  themisdb-pricing-server
```

## Architektur

```
enterprise_pricing_server/
├── __init__.py
├── app.py                 # FastAPI Hauptanwendung
├── config.py              # Konfigurationsverwaltung
├── tkinter_admin.py       # Tkinter Admin UI
├── models/
│   └── __init__.py        # SQLAlchemy + Pydantic Modelle
├── routers/
│   ├── __init__.py
│   ├── auth.py           # Authentifizierung
│   ├── customers.py      # Kundenverwaltung
│   ├── subscriptions.py  # Abonnementverwaltung
│   └── payments.py       # Zahlungsverwaltung
├── services/
│   ├── __init__.py
│   ├── customer_service.py      # Kundenlogik
│   ├── subscription_service.py  # Abonnementlogik
│   └── payment_service.py       # Zahlungslogik
└── utils/
    ├── __init__.py
    ├── database.py       # Datenbankverbindung
    ├── security.py       # JWT, Hashing
    └── license.py        # Lizenzschlüssel-Generierung
```

## Lizenz

Dieses Modul ist Teil von ThemisDB und unterliegt der gleichen Lizenz wie das Hauptprojekt.

## Support

Für Fragen und Support:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Email: support@themisdb.com

## Weitere Entwicklung

Geplante Features:

- [ ] Admin-Dashboard (Web-basiert)
- [ ] Mehrere Währungen
- [ ] Automatische Rechnungserstellung (PDF)
- [ ] E-Mail-Benachrichtigungen
- [ ] Metriken und Analytics
- [ ] Multi-Tenant-Unterstützung
- [ ] Rabattcodes und Promotionen
- [ ] Rechnungshistorie
- [ ] Stripe/PayPal direkte Integration
- [ ] Automatische Verlängerung

---

**Entwickelt mit ❤️ für ThemisDB**
