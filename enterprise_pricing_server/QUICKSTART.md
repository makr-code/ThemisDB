# ThemisDB Enterprise Pricing Server - Quick Start Guide

## Schnellstart (5 Minuten)

### 1. Installation

```bash
cd ThemisDB/enterprise_pricing_server

# Setup-Script ausführen (installiert Dependencies und richtet .env ein)
./setup.sh
```

### 2. Server starten

```bash
# Terminal 1: Server starten
python run_server.py
```

Der Server läuft nun unter: http://localhost:8000

API Dokumentation:
- Swagger UI: http://localhost:8000/docs
- ReDoc: http://localhost:8000/redoc

### 3. API testen

```bash
# Terminal 2: API-Tests ausführen
python test_api.py
```

### 4. Admin UI starten

```bash
# Terminal 3: Tkinter Admin-Oberfläche
python tkinter_admin.py
```

## Erste Schritte

### 1. Kunde registrieren

```bash
curl -X POST http://localhost:8000/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "email": "kunde@firma.de",
    "password": "SicheresPasswort123!",
    "organization_name": "Meine Firma GmbH",
    "contact_name": "Max Mustermann",
    "country": "Germany"
  }'
```

### 2. Anmelden und Token erhalten

```bash
curl -X POST http://localhost:8000/auth/login-json \
  -H "Content-Type: application/json" \
  -d '{
    "email": "kunde@firma.de",
    "password": "SicheresPasswort123!"
  }'
```

Antwort:
```json
{
  "access_token": "eyJhbGc...",
  "token_type": "bearer"
}
```

Speichern Sie den Token für weitere Requests:
```bash
export TOKEN="eyJhbGc..."
```

### 3. Abonnement erstellen

```bash
curl -X POST http://localhost:8000/subscriptions \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "tier": "enterprise",
    "max_nodes": 10,
    "billing_period_months": 12
  }'
```

### 4. Zahlung erstellen

```bash
curl -X POST http://localhost:8000/payments \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "subscription_id": 1,
    "amount": 60000.0,
    "currency": "EUR",
    "payment_method": "bank_transfer"
  }'
```

### 5. Zahlung verifizieren

```bash
curl -X POST http://localhost:8000/payments/1/verify \
  -H "Authorization: Bearer $TOKEN"
```

Nach erfolgreicher Verifizierung wird das Abonnement automatisch aktiviert!

## Verfügbare Pricing Tiers

### Community (Kostenlos)
- Single-Node
- Bis zu 8 Worker Threads
- Single GPU
- Alle Core-Features

### Enterprise (€5.000/Monat)
- Bis zu 100 Nodes
- Advanced Analytics (OLAP/CEP)
- High Availability & Replikation
- Multi-GPU Support
- Priority Support

### Hyperscaler (€25.000/Monat)
- Unbegrenzte Nodes
- Alle Enterprise Features
- Kubernetes Operator
- Multi-Datacenter
- Dedicated Support

### Reseller (€15.000/Monat)
- Konfigurierbare Node-Limits
- White-Label Optionen
- Embed in kommerzielle Anwendungen
- Reseller Support

## Tkinter Admin UI

Die Admin-Oberfläche bietet:

1. **Login Tab**: Kundenregistrierung und Anmeldung
2. **Customers Tab**: Profilansicht und -verwaltung
3. **Subscriptions Tab**: Abonnement-Übersicht und -Verwaltung
4. **Payments Tab**: Zahlungsüberwachung und Verifizierung
5. **Pricing Tab**: Übersicht aller Pricing Tiers

## Banking-Interface

Der Server unterstützt verschiedene Zahlungssysteme:

### Mock-Modus (Standard)
Für Entwicklung und Tests. Zahlungen werden automatisch verifiziert.

### Produktionsmodus
In `.env` konfigurieren:

```env
# Stripe Integration
STRIPE_API_KEY=sk_live_...
STRIPE_WEBHOOK_SECRET=whsec_...

# Oder direkte Banking API
BANKING_API_URL=https://api.your-bank.com
BANKING_API_KEY=your-api-key
```

## Automatisierung

### Webhook für automatische Zahlungsverifizierung

Der Server bietet einen Webhook-Endpunkt für Zahlungsbenachrichtigungen:

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

Bei erfolgreicher Zahlung wird das Abonnement automatisch aktiviert!

## Sicherheit

### Produktions-Deployment

Für Produktion:

1. **Geheimen Schlüssel ändern**:
   ```bash
   openssl rand -hex 32
   ```
   In `.env` eintragen

2. **Reverse Proxy verwenden** (nginx/Caddy):
   - SSL/TLS-Termination
   - Rate Limiting
   - DDoS-Schutz

3. **Datenbank sichern**:
   - PostgreSQL statt SQLite verwenden
   - Regelmäßige Backups
   - Verschlüsselung

4. **CORS korrekt konfigurieren**:
   In `app.py` die erlaubten Origins einschränken

## Troubleshooting

### Server startet nicht

```bash
# Prüfen ob Port 8000 frei ist
lsof -i :8000

# Dependencies neu installieren
pip install -r requirements.txt
```

### Imports funktionieren nicht

```bash
# Sicherstellen dass Sie im richtigen Verzeichnis sind
cd enterprise_pricing_server

# Server mit run_server.py starten
python run_server.py
```

### Tests schlagen fehl

```bash
# pytest-asyncio installieren
pip install pytest pytest-asyncio

# Tests ausführen
python -m pytest tests/ -v
```

## Support

Für Fragen und Support:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Email: support@themisdb.com

## Weitere Dokumentation

- Vollständige API-Dokumentation: http://localhost:8000/docs
- README.md: Detaillierte Informationen
- ENTERPRISE.md: Enterprise-Features von ThemisDB
