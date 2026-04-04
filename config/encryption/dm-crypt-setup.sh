#!/bin/bash
#
# ThemisDB Encrypted Storage Setup Script
# Sets up dm-crypt/LUKS encrypted volume for ThemisDB data
#
# Usage:
#   sudo ./dm-crypt-setup.sh /dev/sdb /var/lib/themisdb
#
# Requirements:
#   - Root privileges
#   - cryptsetup installed (apt install cryptsetup / yum install cryptsetup-luks)
#   - Target block device (e.g., /dev/sdb)
#   - AES-NI enabled CPU (recommended for performance)
#

set -e
set -o pipefail

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
DEVICE="${1:-}"
MOUNT_POINT="${2:-/var/lib/themisdb}"
MAPPER_NAME="themisdb_encrypted"
LUKS_CIPHER="aes-xts-plain64"
LUKS_KEY_SIZE=512
LUKS_HASH="sha256"
FILESYSTEM="ext4"
KEY_FILE="/etc/themisdb/luks-key"

# Functions
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root"
        exit 1
    fi
}

check_requirements() {
    # Check cryptsetup
    if ! command -v cryptsetup &> /dev/null; then
        print_error "cryptsetup not found. Install with: apt install cryptsetup"
        exit 1
    fi
    
    # Check AES-NI support
    if grep -q aes /proc/cpuinfo; then
        print_info "AES-NI hardware acceleration detected ✓"
    else
        print_warn "AES-NI not detected. Performance may be reduced."
    fi
    
    # Check device
    if [[ -z "$DEVICE" ]]; then
        print_error "Usage: $0 <device> [mount_point]"
        print_error "Example: $0 /dev/sdb /var/lib/themisdb"
        exit 1
    fi
    
    if [[ ! -b "$DEVICE" ]]; then
        print_error "Device $DEVICE not found or is not a block device"
        exit 1
    fi
    
    # Warn about data loss
    print_warn "WARNING: This will DESTROY all data on $DEVICE"
    print_warn "Device details:"
    lsblk "$DEVICE" || true
    echo -n "Continue? (yes/no): "
    read -r confirm
    if [[ "$confirm" != "yes" ]]; then
        print_info "Aborted by user"
        exit 0
    fi
}

setup_luks() {
    print_info "Setting up LUKS encryption on $DEVICE"
    
    # Wipe device signatures (removes existing partitions/filesystems)
    print_info "Wiping device signatures..."
    wipefs -a "$DEVICE"
    
    # Generate random key
    print_info "Generating encryption key..."
    mkdir -p "$(dirname "$KEY_FILE")"
    dd if=/dev/urandom of="$KEY_FILE" bs=1 count=4096 status=none
    chmod 600 "$KEY_FILE"
    
    # Format LUKS volume
    print_info "Formatting LUKS volume (this may take a few minutes)..."
    cryptsetup luksFormat \
        --type luks2 \
        --cipher "$LUKS_CIPHER" \
        --key-size "$LUKS_KEY_SIZE" \
        --hash "$LUKS_HASH" \
        --key-file "$KEY_FILE" \
        --batch-mode \
        "$DEVICE"
    
    print_info "LUKS volume created successfully ✓"
}

open_luks() {
    print_info "Opening LUKS volume..."
    cryptsetup luksOpen \
        --key-file "$KEY_FILE" \
        "$DEVICE" \
        "$MAPPER_NAME"
    
    print_info "LUKS volume opened at /dev/mapper/$MAPPER_NAME ✓"
}

create_filesystem() {
    print_info "Creating $FILESYSTEM filesystem..."
    
    local mapper_device="/dev/mapper/$MAPPER_NAME"
    
    # Create filesystem with optimized settings for database workloads
    if [[ "$FILESYSTEM" == "ext4" ]]; then
        # Note: Journaling is ENABLED for data safety in production
        # For benchmarking, you can disable with: -O ^has_journal
        mkfs.ext4 \
            -L themisdb_data \
            -m 1 \
            "$mapper_device"
    elif [[ "$FILESYSTEM" == "xfs" ]]; then
        mkfs.xfs \
            -L themisdb_data \
            "$mapper_device"
    else
        mkfs."$FILESYSTEM" "$mapper_device"
    fi
    
    print_info "Filesystem created successfully ✓"
}

mount_filesystem() {
    print_info "Mounting filesystem to $MOUNT_POINT..."
    
    # Create mount point
    mkdir -p "$MOUNT_POINT"
    
    # Mount with optimized flags for database workloads
    local mapper_device="/dev/mapper/$MAPPER_NAME"
    
    if [[ "$FILESYSTEM" == "ext4" ]]; then
        mount -o noatime,nodiratime,discard "$mapper_device" "$MOUNT_POINT"
    elif [[ "$FILESYSTEM" == "xfs" ]]; then
        mount -o noatime,nodiratime,discard "$mapper_device" "$MOUNT_POINT"
    else
        mount "$mapper_device" "$MOUNT_POINT"
    fi
    
    # Set ownership for themisdb user (if exists)
    if id "themisdb" &>/dev/null; then
        chown themisdb:themisdb "$MOUNT_POINT"
    else
        print_warn "User 'themisdb' not found. Mount point owned by root."
    fi
    
    print_info "Filesystem mounted successfully ✓"
}

setup_auto_mount() {
    print_info "Configuring automatic mount on boot..."
    
    # Get UUID
    local uuid
    uuid=$(blkid -s UUID -o value "$DEVICE")
    
    # Add to /etc/crypttab
    if ! grep -q "$MAPPER_NAME" /etc/crypttab 2>/dev/null; then
        echo "$MAPPER_NAME UUID=$uuid $KEY_FILE luks,discard" >> /etc/crypttab
        print_info "Added entry to /etc/crypttab ✓"
    fi
    
    # Add to /etc/fstab
    if ! grep -q "$MAPPER_NAME" /etc/fstab 2>/dev/null; then
        local mapper_device="/dev/mapper/$MAPPER_NAME"
        if [[ "$FILESYSTEM" == "ext4" ]]; then
            echo "$mapper_device $MOUNT_POINT ext4 noatime,nodiratime,discard 0 2" >> /etc/fstab
        elif [[ "$FILESYSTEM" == "xfs" ]]; then
            echo "$mapper_device $MOUNT_POINT xfs noatime,nodiratime,discard 0 2" >> /etc/fstab
        else
            echo "$mapper_device $MOUNT_POINT $FILESYSTEM defaults 0 2" >> /etc/fstab
        fi
        print_info "Added entry to /etc/fstab ✓"
    fi
    
    print_info "Auto-mount configured successfully ✓"
}

print_summary() {
    echo ""
    echo "═══════════════════════════════════════════════════════════"
    print_info "ThemisDB Encrypted Storage Setup Complete!"
    echo "═══════════════════════════════════════════════════════════"
    echo ""
    echo "Configuration:"
    echo "  Device:          $DEVICE"
    echo "  Mapper:          /dev/mapper/$MAPPER_NAME"
    echo "  Mount Point:     $MOUNT_POINT"
    echo "  Filesystem:      $FILESYSTEM"
    echo "  Encryption:      LUKS2 with $LUKS_CIPHER ($LUKS_KEY_SIZE-bit key)"
    echo "  Key File:        $KEY_FILE"
    echo ""
    echo "Status:"
    df -h "$MOUNT_POINT" | tail -1
    echo ""
    print_warn "IMPORTANT: Backup the key file securely!"
    echo "  Location: $KEY_FILE"
    echo "  Command:  sudo cp $KEY_FILE /secure/backup/location/"
    echo ""
    print_warn "Without this key file, data cannot be recovered!"
    echo ""
    echo "Next Steps:"
    echo "  1. Configure ThemisDB to use $MOUNT_POINT"
    echo "  2. Backup key file to secure location"
    echo "  3. Test auto-mount: sudo reboot"
    echo "  4. Verify mount: df -h $MOUNT_POINT"
    echo ""
    echo "═══════════════════════════════════════════════════════════"
}

# Main execution
main() {
    print_info "ThemisDB Encrypted Storage Setup"
    echo ""
    
    check_root
    check_requirements
    
    setup_luks
    open_luks
    create_filesystem
    mount_filesystem
    setup_auto_mount
    
    print_summary
}

main
