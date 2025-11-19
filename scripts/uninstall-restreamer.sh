#!/bin/bash
#
# Datarhei Restreamer Uninstaller
#
# Usage:
#   sudo ./uninstall-restreamer.sh
#   or
#   curl -fsSL https://raw.githubusercontent.com/rainmanjam/obs-polyemesis/main/scripts/uninstall-restreamer.sh | sudo bash
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Default paths
INSTALL_DIR="/opt/restreamer"
CONFIG_DIR="/etc/restreamer"
DATA_DIR="/var/lib/restreamer"
SERVICE_NAME="restreamer"

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "This script must be run as root or with sudo"
        exit 1
    fi
}

main() {
    echo -e "${RED}╔═══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║      Datarhei Restreamer - Uninstaller                   ║${NC}"
    echo -e "${RED}╚═══════════════════════════════════════════════════════════╝${NC}"
    echo

    check_root

    # Confirm uninstallation
    echo -e "${YELLOW}This will remove Restreamer and all associated files.${NC}"
    echo
    read -p "$(echo -e ${CYAN}Are you sure you want to continue? [y/N]:${NC} )" -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_info "Uninstall cancelled"
        exit 0
    fi

    # Stop and disable service
    print_info "Stopping Restreamer service..."
    if systemctl is-active --quiet ${SERVICE_NAME}; then
        systemctl stop ${SERVICE_NAME}
        print_success "Service stopped"
    fi

    if systemctl is-enabled --quiet ${SERVICE_NAME} 2>/dev/null; then
        systemctl disable ${SERVICE_NAME}
        print_success "Service disabled"
    fi

    # Remove systemd service file
    if [ -f "/etc/systemd/system/${SERVICE_NAME}.service" ]; then
        rm -f "/etc/systemd/system/${SERVICE_NAME}.service"
        systemctl daemon-reload
        print_success "Systemd service removed"
    fi

    # Stop and remove Docker containers
    if [ -f "$INSTALL_DIR/docker-compose.yml" ]; then
        print_info "Removing Docker containers..."
        cd "$INSTALL_DIR"
        docker compose down -v 2>/dev/null || true
        print_success "Docker containers removed"
    fi

    # Remove Docker images
    read -p "$(echo -e ${CYAN}Remove Restreamer Docker images? [y/N]:${NC} )" -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        docker rmi datarhei/restreamer:latest 2>/dev/null || true
        print_success "Docker images removed"
    fi

    # Remove installation directory
    if [ -d "$INSTALL_DIR" ]; then
        rm -rf "$INSTALL_DIR"
        print_success "Installation directory removed"
    fi

    # Remove configuration directory
    if [ -d "$CONFIG_DIR" ]; then
        rm -rf "$CONFIG_DIR"
        print_success "Configuration directory removed"
    fi

    # Ask about data directory
    if [ -d "$DATA_DIR" ]; then
        echo
        echo -e "${YELLOW}Data directory contains your streams, recordings, and configurations.${NC}"
        read -p "$(echo -e ${CYAN}Remove data directory? [y/N]:${NC} )" -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "$DATA_DIR"
            print_success "Data directory removed"
        else
            print_info "Data directory preserved at: $DATA_DIR"
        fi
    fi

    # Ask about Docker
    echo
    read -p "$(echo -e ${CYAN}Remove Docker? [y/N]:${NC} )" -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        print_info "Removing Docker..."

        # Detect distro
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            DISTRO=$ID
        fi

        case "$DISTRO" in
            ubuntu|debian)
                apt-get purge -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
                apt-get autoremove -y
                rm -rf /var/lib/docker
                rm -rf /var/lib/containerd
                ;;
            rhel|centos|fedora)
                yum remove -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
                rm -rf /var/lib/docker
                rm -rf /var/lib/containerd
                ;;
            arch)
                pacman -Rns --noconfirm docker docker-compose
                rm -rf /var/lib/docker
                ;;
        esac

        print_success "Docker removed"
    else
        print_info "Docker was not removed"
    fi

    echo
    print_success "Uninstallation complete!"
    echo
}

main "$@"
