# Raspberry Pi Build Guide

## Hardware-Anforderungen

| Model | Prozessor | RAM | Storage | Empfohlen? |
|-------|-----------|-----|---------|------------|
| **Pi 4B** | Cortex-A72 (4 cores, 1.5GHz) | 4-8GB | microSD 32GB+ | ✅ Ja |
| **Pi 5** | Cortex-A76 (4 cores, 2.4GHz) | 4-8GB | microSD 32GB+ | ✅ Ja |
| **Pi 3B** | Cortex-A53 (4 cores, 1.2GHz) | 1GB | microSD 32GB | ⚠️ Marginal |
| **Pi Zero 2** | Cortex-A53 (4 cores, 1GHz) | 512MB | microSD 16GB | ❌ Zu schwach |

**Empfehlung**: Raspberry Pi 4 mit 8GB RAM + SSD

## OS Setup

### Schritt 1: OS Installation

```bash
# Option A: Raspberry Pi Imager (grafisch)
# https://www.raspberrypi.com/software/
# → Wähle "Raspberry Pi OS (64-bit)"

# Option B: Kommandozeile (Linux/WSL)
wget https://downloads.raspberrypi.com/raspios_arm64/images/raspios_arm64-*/
unzip *.zip
sudo dd if=*.img of=/dev/sdc bs=4M status=progress
sync
```

### Schritt 2: Erste Boot-Konfiguration

```bash
# SSH aktivieren
sudo raspi-config
# → Interface Options → SSH → Enable

# Hostname ändern (optional)
sudo hostnamectl set-hostname themis-rpi

# Neustart
sudo reboot
```

### Schritt 3: System aktualisieren

```bash
sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get autoremove -y
```

## Build-Vorbereitung

### Schritt 1: Build Dependencies

```bash
# Alle notwendigen Pakete
sudo apt-get install -y \
    build-essential \
    gcc-11 g++-11 \
    cmake ninja-build \
    git curl wget \
    pkg-config \
    libssl-dev libcurl4-openssl-dev \
    python3 python3-dev python3-pip

# Optional: Schnellere Rebuild mit ccache
sudo apt-get install -y ccache
```

### Schritt 2: vcpkg Setup

```bash
cd ~
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh -disableMetrics

# Zu PATH hinzufügen
echo 'export PATH=$HOME/vcpkg:$PATH' >> ~/.bashrc
source ~/.bashrc
```

### Schritt 3: ThemisDB klonen

```bash
cd ~
git clone https://github.com/makr-code/themisdb.git
cd themisdb
```

## Kompilierung

### Option 1: Community Edition (Standard)

```bash
cd ~/themisdb

# Configure - mit reduziertem Parallelismus für RPi
cmake -S . -B build-rpi \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF

# Build - 2-4 Cores für RPi (nicht 8!)
cmake --build build-rpi --parallel 2

# Binary: ~/themisdb/build-rpi/themis_server
```

### Option 2: Hyperscaler mit LLM (falls genug RAM)

```bash
# Nur für Pi 4 mit 8GB RAM!
cmake -S . -B build-rpi-llm \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=OFF \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF

cmake --build build-rpi-llm --parallel 2

# ~1.5-2 Stunden Build-Zeit!
```

### Build-Fehler verhindern

```bash
# RAM-Limite setzen (falls OOM)
export CXXFLAGS="-fno-lto"  # Disable Link-Time Optimization

# Oder: Swap hinzufügen
sudo fallocate -l 4G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

## Docker Setup

### Option A: Docker auf RPi installieren

```bash
curl -sSL https://get.docker.com | sh

# Zum docker group hinzufügen (ohne sudo)
sudo usermod -aG docker $USER
newgrp docker
```

### Option B: Vorkompiliertes Image von x86_64 übertragen

```bash
# Auf x86_64 Host:
docker build -f docker/Dockerfile.themis-server \
  --platform linux/arm64 \
  -t themis-server:rpi4 \
  .

docker save themis-server:rpi4 | \
  ssh pi@raspberrypi 'docker load'
```

## ThemisDB starten

### Als Standalone Process

```bash
# Binary ausführbar machen
chmod +x ~/themisdb/build-rpi/themis_server

# Starten
~/themisdb/build-rpi/themis_server \
  --port 18765 \
  --http-port 8080 \
  --data-dir /home/pi/.themisdb

# Im Hintergrund (detached)
nohup ~/themisdb/build-rpi/themis_server \
  --port 18765 \
  --http-port 8080 \
  --data-dir /home/pi/.themisdb &
```

### Mit Systemd Service

```bash
# Service-Datei erstellen
sudo tee /etc/systemd/system/themis.service > /dev/null <<EOF
[Unit]
Description=ThemisDB Server
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/.themisdb
ExecStart=/home/pi/themisdb/build-rpi/themis_server \\
  --port 18765 \\
  --http-port 8080 \\
  --data-dir /home/pi/.themisdb
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

# Service aktivieren und starten
sudo systemctl daemon-reload
sudo systemctl enable themis
sudo systemctl start themis
sudo systemctl status themis
```

### Mit Docker

```bash
docker run -d \
  --name themis-rpi \
  -p 18765:18765 \
  -p 8080:8080 \
  -v $HOME/.themisdb:/var/lib/themisdb \
  themis-server:rpi4
```

## Performance-Optimierungen

### 1. Filesystem Optimierung

```bash
# RPi Zero/3: microSD kann Bottleneck sein
# Empfehlung: USB-SSD verwenden

# SSD mit ext4 formatieren
sudo mkfs.ext4 /dev/sda1
sudo mount /dev/sda1 /mnt/data

# themisdb dorthin verschieben
mv /home/pi/.themisdb /mnt/data/themisdb
ln -s /mnt/data/themisdb /home/pi/.themisdb
```

### 2. CPU Frequency Scaling

```bash
# CPU-Frequenz ansehen
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq

# Auf max setzen
echo "powersave" | sudo tee /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo "performance" | sudo tee /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
```

### 3. Memory-optimierte Build

```bash
# Kleinere Binaries, weniger Speicher
cmake -S . -B build-rpi-small \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-Os" \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_BUILD_TESTS=OFF

cmake --build build-rpi-small --parallel 1
```

## Monitoring

### Temperatur überwachen

```bash
# Echtzeit-Temperatur
watch -n 1 'cat /sys/class/thermal/thermal_zone0/temp | awk "{print \$1/1000}"'

# Zu heiß? (>80°C) → Passive Kühlung oder Lüfter hinzufügen
```

### RAM & CPU Auslastung

```bash
# Echtzeit-Monitoring
top -o %MEM

# oder
htop
```

## Remote Access

### SSH Tunnel für sichere Verbindung

```bash
# Von Laptop zu RPi:
ssh -L 18765:localhost:18765 -L 8080:localhost:8080 pi@raspberrypi

# Dann lokal verbinden:
curl http://localhost:8080/health
```

## Troubleshooting

### Problem: "cc1plus: out of memory"
**Lösung**: Swap hinzufügen oder Parallelismus reduzieren
```bash
cmake --build . --parallel 1
```

### Problem: "fatal error: x.h: No such file or directory"
**Lösung**: vcpkg Dependencies installieren
```bash
export VCPKG_ROOT=$HOME/vcpkg
cmake --preset linux-gcc-release
```

### Problem: Binäre zu groß
**Lösung**: Binäre "strippen" (Symbole entfernen)
```bash
arm-linux-gnueabihf-strip ~/themisdb/build-rpi/themis_server
```

### Problem: Netzwerk zu langsam
**Lösung**: SSH + git clone ist langsam auf RPi Zero
```bash
# Besser: Auf x86_64 klonen, dann via USB übertragen
```

## Cluster-Setup (mehrere RPis)

```bash
# Alle RPis synchronisieren über NFS
sudo apt-get install nfs-client
sudo mount -t nfs 192.168.1.10:/export/themis /mnt/themis
```

## Nächste Schritte

Nach erfolgreichem Build lesen Sie:
- **RPi Deployment**: [docs/de/deployment/deployment_raspberry_tuning.md](../../de/deployment/deployment_raspberry_tuning.md) - Tuning & Deployment
- **Deployment allgemein**: [docs/de/deployment/deployment_strategy.md](../../de/deployment/deployment_strategy.md)
- **Releases**: [docs/de/releases/updates_distribution_strategy.md](../../de/releases/updates_distribution_strategy.md)

## Weitere Infos

- [BUILD_ARM.md](BUILD_ARM.md) - Generischer ARM-Guide
- [docker/Dockerfile.themis-server](../../docker/Dockerfile.themis-server) - Docker Config
- [cmake/CMakePresets.json](../../cmake/CMakePresets.json) - CMake Presets
