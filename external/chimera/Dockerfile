# CHIMERA Benchmark Suite – Runtime Image
# ============================================================
# Zweck:
# - Endurance-Runner (Default ENTRYPOINT)
# - CLI/Server-Runtime ueber command override
#
# Build:
#   docker build -t chimera-benchmark .
# ============================================================

FROM python:3.11-slim

# System-Abhängigkeiten für numpy/scipy und TLS
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        gcc \
        libffi-dev \
        libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Abhängigkeiten zuerst kopieren (Layer-Caching)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Runtime-Quellcode explizit whitelisten (kein pauschales COPY . .)
COPY __init__.py ./
COPY benchmark_harness.py ./
COPY benchmark_standards.py ./
COPY chimera_cli.py ./
COPY chimera_server.py ./
COPY citations.py ./
COPY colors.py ./
COPY dashboard.py ./
COPY exporter.py ./
COPY reporter.py ./
COPY run_endurance.py ./
COPY scoring.py ./
COPY statistics.py ./
COPY adapters/chimera/adapter_config_example.toml ./adapters/chimera/

# Ergebnisverzeichnis
RUN mkdir -p /results /certs

# Kein Root-Benutzer im Container
RUN useradd -m -u 1000 chimera && chown -R chimera:chimera /app /results /certs
USER chimera

ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    PYTHONPATH=/app

ENTRYPOINT ["python", "run_endurance.py"]
