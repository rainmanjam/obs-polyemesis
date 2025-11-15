#!/bin/bash
#
# Polyemesis Restreamer Uninstaller
# Complete removal of Restreamer and associated components
#
# Usage: sudo bash uninstall.sh
#

set -e

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo -e "${RED}================================================${NC}"
    echo -e "${RED}  Polyemesis Restreamer Uninstaller${NC}"
    echo -e "${RED}================================================${NC}"
    echo ""
}

print_warning() {
    echo -e "${YELLOW}!${NC} $1"
}

print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Check root
if [ "$EUID" -ne 0 ]; then
    print_error "Please run as root or with sudo"
    exit 1
fi

print_header

print_warning "This will completely remove Restreamer and all associated components."
print_warning "This action CANNOT be undone!"
echo ""

read -p "Are you sure you want to continue? [y/N]: " CONFIRM
if [[ ! $CONFIRM =~ ^[Yy]$ ]]; then
    echo "Uninstallation cancelled."
    exit 0
fi

echo ""
read -p "Do you want to keep backups in /var/backups/restreamer? [Y/n]: " KEEP_BACKUPS
KEEP_BACKUPS=${KEEP_BACKUPS:-Y}

echo ""
read -p "Do you want to keep data in /var/lib/restreamer? [y/N]: " KEEP_DATA
KEEP_DATA=${KEEP_DATA:-N}

echo ""
print_info "Starting uninstallation..."
echo ""

# Stop and remove containers
print_info "Stopping and removing Docker containers..."
docker stop restreamer 2>/dev/null && print_success "Stopped restreamer container" || print_info "Restreamer container not running"
docker rm restreamer 2>/dev/null && print_success "Removed restreamer container" || print_info "Restreamer container not found"

docker stop watchtower 2>/dev/null && print_success "Stopped watchtower container" || print_info "Watchtower container not running"
docker rm watchtower 2>/dev/null && print_success "Removed watchtower container" || print_info "Watchtower container not found"

# Remove Docker images
print_info "Removing Docker images..."
docker rmi datarhei/restreamer:latest 2>/dev/null && print_success "Removed Restreamer image" || print_info "Restreamer image not found"
docker rmi containrrr/watchtower 2>/dev/null && print_success "Removed Watchtower image" || print_info "Watchtower image not found"

# Remove data directory
if [[ ! $KEEP_DATA =~ ^[Yy]$ ]]; then
    print_info "Removing data directory..."
    if [ -d /var/lib/restreamer ]; then
        rm -rf /var/lib/restreamer
        print_success "Removed /var/lib/restreamer"
    else
        print_info "Data directory not found"
    fi
else
    print_info "Keeping data directory at /var/lib/restreamer"
fi

# Remove backup directory
if [[ ! $KEEP_BACKUPS =~ ^[Yy]$ ]]; then
    print_info "Removing backup directory..."
    if [ -d /var/backups/restreamer ]; then
        rm -rf /var/backups/restreamer
        print_success "Removed /var/backups/restreamer"
    else
        print_info "Backup directory not found"
    fi
else
    print_info "Keeping backups at /var/backups/restreamer"
fi

# Remove scripts
print_info "Removing management scripts..."
rm -f /usr/local/bin/restreamer-manage && print_success "Removed restreamer-manage"
rm -f /usr/local/bin/restreamer-backup && print_success "Removed restreamer-backup"
rm -f /usr/local/bin/restreamer-restore && print_success "Removed restreamer-restore"
rm -f /usr/local/bin/restreamer-monitor && print_success "Removed restreamer-monitor"

# Remove cron jobs
print_info "Removing cron jobs..."
crontab -l 2>/dev/null | grep -v restreamer | crontab - 2>/dev/null && print_success "Removed cron jobs" || print_info "No cron jobs found"

# Note: SSL certificates are managed by Restreamer's built-in Let's Encrypt
# They will be removed automatically when the container is removed

# Remove firewall rules (optional)
echo ""
read -p "Do you want to remove firewall rules? [y/N]: " REMOVE_FW
if [[ $REMOVE_FW =~ ^[Yy]$ ]]; then
    print_info "Removing firewall rules..."

    if command -v ufw &> /dev/null; then
        ufw delete allow 8080/tcp 2>/dev/null || true
        ufw delete allow 8181/tcp 2>/dev/null || true
        ufw delete allow 80/tcp 2>/dev/null || true
        ufw delete allow 443/tcp 2>/dev/null || true
        ufw delete allow 1935/tcp 2>/dev/null || true
        ufw delete allow 6000/udp 2>/dev/null || true
        print_success "Removed ufw rules"
    fi

    if command -v firewall-cmd &> /dev/null; then
        firewall-cmd --permanent --remove-port=8080/tcp 2>/dev/null || true
        firewall-cmd --permanent --remove-port=8181/tcp 2>/dev/null || true
        firewall-cmd --permanent --remove-port=80/tcp 2>/dev/null || true
        firewall-cmd --permanent --remove-port=443/tcp 2>/dev/null || true
        firewall-cmd --permanent --remove-port=1935/tcp 2>/dev/null || true
        firewall-cmd --permanent --remove-port=6000/udp 2>/dev/null || true
        firewall-cmd --reload 2>/dev/null || true
        print_success "Removed firewalld rules"
    fi
fi

echo ""
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}  Uninstallation Complete${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""

if [[ $KEEP_DATA =~ ^[Yy]$ ]]; then
    print_info "Data preserved at: /var/lib/restreamer"
fi

if [[ $KEEP_BACKUPS =~ ^[Yy]$ ]]; then
    print_info "Backups preserved at: /var/backups/restreamer"
fi

echo ""
print_info "Docker itself was not removed. To remove Docker, run:"
echo "  Ubuntu/Debian: sudo apt remove docker-ce docker-ce-cli containerd.io"
echo "  CentOS/RHEL:   sudo dnf remove docker-ce docker-ce-cli containerd.io"
echo "  Arch:          sudo pacman -R docker"
echo ""

print_success "Thank you for using Polyemesis Restreamer!"
echo ""
