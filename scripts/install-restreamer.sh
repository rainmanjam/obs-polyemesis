#!/bin/bash
#
# Datarhei Restreamer Installer
# An interactive installer for datarhei/restreamer on Linux
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/rainmanjam/obs-polyemesis/main/scripts/install-restreamer.sh | bash
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default configuration
INSTALL_DIR="/opt/restreamer"
CONFIG_DIR="/etc/restreamer"
DATA_DIR="/var/lib/restreamer"
SERVICE_NAME="restreamer"
DOCKER_COMPOSE_CMD=""

# Installation state tracking
INSTALLATION_STARTED=false
INSTALLATION_COMPLETED=false
DOCKER_WAS_INSTALLED=false
DOCKER_INSTALLED_BY_US=false
DIRECTORIES_CREATED=false
SYSTEMD_SERVICE_CREATED=false
CONTAINER_STARTED=false

# Print colored output
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

print_header() {
    echo -e "${MAGENTA}${1}${NC}"
}

# Safe input function that works in all environments
# Tries /dev/tty first (for piped execution), falls back to stdin
safe_read() {
    local options="$1"
    local varname="$2"

    # Try /dev/tty first if it exists and is usable
    if [ -c /dev/tty ] && eval "read $options $varname < /dev/tty" 2>/dev/null; then
        return 0
    # Fall back to stdin if it's a terminal
    elif [ -t 0 ]; then
        eval "read $options $varname"
    # Last resort: try stdin anyway (for scripted input)
    elif eval "read $options $varname" 2>/dev/null; then
        return 0
    else
        # No way to read input
        print_error "No interactive terminal available. Please run this script directly with a terminal."
        exit 1
    fi
}

# Cleanup function for rollback
cleanup_on_exit() {
    local exit_code=$?

    # If installation completed successfully, don't clean up
    if [ "$INSTALLATION_COMPLETED" = true ]; then
        return 0
    fi

    # If installation never started, don't clean up
    if [ "$INSTALLATION_STARTED" = false ]; then
        return 0
    fi

    echo
    print_warning "Installation was interrupted or failed"
    print_info "Rolling back changes..."

    # Stop and remove container if started
    if [ "$CONTAINER_STARTED" = true ]; then
        print_info "Stopping Restreamer container..."
        cd "$INSTALL_DIR" 2>/dev/null && $DOCKER_COMPOSE_CMD down 2>/dev/null || true
    fi

    # Remove systemd service if created
    if [ "$SYSTEMD_SERVICE_CREATED" = true ]; then
        print_info "Removing systemd service..."
        systemctl stop ${SERVICE_NAME} 2>/dev/null || true
        systemctl disable ${SERVICE_NAME} 2>/dev/null || true
        rm -f "/etc/systemd/system/${SERVICE_NAME}.service"
        systemctl daemon-reload 2>/dev/null || true
    fi

    # Remove directories if created
    if [ "$DIRECTORIES_CREATED" = true ]; then
        print_info "Removing created directories..."
        rm -rf "$INSTALL_DIR" 2>/dev/null || true
        rm -rf "$CONFIG_DIR" 2>/dev/null || true
        # Only remove data dir if empty
        if [ -d "$DATA_DIR" ] && [ -z "$(ls -A "$DATA_DIR" 2>/dev/null)" ]; then
            rm -rf "$DATA_DIR" 2>/dev/null || true
        fi
    fi

    # Remove Docker if we installed it
    if [ "$DOCKER_INSTALLED_BY_US" = true ]; then
        echo
        echo -e "${CYAN}Remove Docker (we just installed it)? [y/N]:${NC}"
        safe_read "-n 1 -r" "REPLY"
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            print_info "Removing Docker..."
            case "$DISTRO" in
                ubuntu|debian)
                    apt-get purge -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin 2>/dev/null || true
                    ;;
                rhel|centos|fedora)
                    yum remove -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin 2>/dev/null || true
                    ;;
                arch)
                    pacman -Rns --noconfirm docker docker-compose 2>/dev/null || true
                    ;;
            esac
        fi
    fi

    print_success "Rollback complete"
    echo
    print_info "You can run the installer again when ready"
    exit $exit_code
}

# Trap interrupts and errors
trap cleanup_on_exit INT TERM ERR

# Detect Linux distribution
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
        DISTRO_VERSION=$VERSION_ID
    elif [ -f /etc/redhat-release ]; then
        DISTRO="rhel"
    elif [ -f /etc/arch-release ]; then
        DISTRO="arch"
    else
        DISTRO="unknown"
    fi

    print_info "Detected distribution: $DISTRO $DISTRO_VERSION"
}

# Check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "This script must be run as root or with sudo"
        exit 1
    fi
}

# Check system requirements
check_requirements() {
    print_info "Checking system requirements..."

    # Check for curl
    if ! command -v curl &> /dev/null; then
        print_error "curl is required but not installed"
        exit 1
    fi

    # Check available disk space (need at least 2GB)
    available_space=$(df / | tail -1 | awk '{print $4}')
    if [ "$available_space" -lt 2097152 ]; then
        print_warning "Less than 2GB disk space available"
    fi

    # Check for DNS tools
    if ! command -v dig &> /dev/null && ! command -v host &> /dev/null && ! command -v nslookup &> /dev/null; then
        print_warning "DNS tools (dig, host, or nslookup) not found. DNS verification might be skipped."
    fi

    print_success "System requirements check passed"
}

# Install Docker
install_docker() {
    print_header "\n=== Docker Installation ==="

    if command -v docker &> /dev/null; then
        print_info "Docker is already installed ($(docker --version))"
        DOCKER_WAS_INSTALLED=true

        # Determine Docker Compose command for existing Docker installation
        if docker compose version &> /dev/null; then
            DOCKER_COMPOSE_CMD="docker compose"
        elif command -v docker-compose &> /dev/null; then
            DOCKER_COMPOSE_CMD="docker-compose"
        else
            print_error "Docker Compose not found. Please install it manually."
            exit 1
        fi
        print_info "Using Docker Compose command: $DOCKER_COMPOSE_CMD"

        echo -e "${CYAN}Do you want to reinstall Docker? [y/N]:${NC}"
        safe_read "-n 1 -r" "REPLY"
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            return 0
        fi
    fi

    print_info "Installing Docker..."
    DOCKER_INSTALLED_BY_US=true

    case "$DISTRO" in
        ubuntu|debian)
            # Remove old versions
            apt-get remove -y docker docker-engine docker.io containerd runc 2>/dev/null || true

            # Update package index
            apt-get update

            # Install dependencies
            apt-get install -y \
                ca-certificates \
                curl \
                gnupg \
                lsb-release

            # Add Docker's official GPG key
            install -m 0755 -d /etc/apt/keyrings
            curl -fsSL https://download.docker.com/linux/$DISTRO/gpg | gpg --dearmor -o /etc/apt/keyrings/docker.gpg
            chmod a+r /etc/apt/keyrings/docker.gpg

            # Set up repository
            echo \
                "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/$DISTRO \
                $(lsb_release -cs) stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null

            # Install Docker
            apt-get update
            apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
            ;;

        rhel|centos|fedora)
            # Remove old versions
            yum remove -y docker docker-client docker-client-latest docker-common docker-latest \
                docker-latest-logrotate docker-logrotate docker-engine 2>/dev/null || true

            # Install dependencies
            yum install -y yum-utils

            # Set up repository
            yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo

            # Install Docker
            yum install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
            ;;

        arch)
            # Update package database
            pacman -Sy

            # Install Docker
            pacman -S --noconfirm docker docker-compose
            ;;

        *)
            print_error "Unsupported distribution: $DISTRO"
            print_info "Please install Docker manually from https://docs.docker.com/engine/install/"
            exit 1
            ;;
    esac

    # Start and enable Docker service
    systemctl start docker
    systemctl enable docker

    # Determine Docker Compose command
    if docker compose version &> /dev/null; then
        DOCKER_COMPOSE_CMD="docker compose"
    elif command -v docker-compose &> /dev/null; then
        DOCKER_COMPOSE_CMD="docker-compose"
    else
        print_error "Docker Compose not found. Please install it manually."
        exit 1
    fi
    print_info "Using Docker Compose command: $DOCKER_COMPOSE_CMD"

    print_success "Docker installed successfully"
}

# Check DNS resolution
check_dns_resolution() {
    local domain=$1

    print_info "Verifying DNS configuration..."

    # Get server's public IP address
    PUBLIC_IP=$(curl -s -4 --max-time 5 https://ifconfig.me || curl -s -4 --max-time 5 https://api.ipify.org || curl -s -4 --max-time 5 https://icanhazip.com)

    if [ -z "$PUBLIC_IP" ]; then
        print_warning "Unable to determine server's public IP address"
        print_info "Skipping DNS verification"
        return 0
    fi

    print_info "Server public IP: ${PUBLIC_IP}"

    # Resolve domain to IP
    RESOLVED_IP=$(dig +short "$domain" A | tail -n1)

    # If dig is not available, try host
    if [ -z "$RESOLVED_IP" ]; then
        RESOLVED_IP=$(host "$domain" 2>/dev/null | grep "has address" | head -n1 | awk '{print $4}')
    fi

    # If still no result, try nslookup
    if [ -z "$RESOLVED_IP" ]; then
        RESOLVED_IP=$(nslookup "$domain" 2>/dev/null | grep -A1 "Name:" | tail -n1 | awk '{print $2}')
    fi

    if [ -z "$RESOLVED_IP" ]; then
        print_error "Unable to resolve domain: ${domain}"
        print_warning "DNS may not be configured correctly"
        echo
        echo -e "${YELLOW}For Let's Encrypt SSL to work, your domain must:${NC}"
        echo -e "  1. Point to this server's IP: ${GREEN}${PUBLIC_IP}${NC}"
        echo -e "  2. Be publicly resolvable"
        echo -e "  3. Have ports 80 and 443 accessible"
        echo

        echo -e "${CYAN}Continue anyway? [y/N]:${NC}"
        safe_read "-n 1 -r" "REPLY"
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            return 1
        fi
    elif [ "$RESOLVED_IP" != "$PUBLIC_IP" ]; then
        print_warning "DNS mismatch detected!"
        echo -e "  Domain ${CYAN}${domain}${NC} resolves to: ${RED}${RESOLVED_IP}${NC}"
        echo -e "  Server public IP: ${GREEN}${PUBLIC_IP}${NC}"
        echo
        echo -e "${YELLOW}Let's Encrypt SSL will fail unless DNS points to this server.${NC}"
        echo

        echo -e "${CYAN}Continue with this domain anyway? [y/N]:${NC}"
        safe_read "-n 1 -r" "REPLY"
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            return 1
        fi
    else
        print_success "DNS correctly configured (${domain} → ${RESOLVED_IP})"
    fi

    return 0
}

# Gather configuration from user
gather_configuration() {
    print_header "\n=== Domain and SSL Configuration ==="

    # Ask about domain name first
    echo
    echo -e "${CYAN}Do you have a domain name for this Restreamer instance?${NC}"
    echo -e "${YELLOW}(Required for SSL/TLS with Let's Encrypt)${NC}"

    while true; do
        echo -e "${CYAN}Enter domain name (leave empty to use IP address):${NC}"
        safe_read "" "DOMAIN_NAME"

        # If no domain provided, break out of loop
        if [ -z "$DOMAIN_NAME" ]; then
            break
        fi

        # Check DNS resolution
        if check_dns_resolution "$DOMAIN_NAME"; then
            break
        else
            print_info "Please enter a different domain or press Enter to skip"
            echo
        fi
    done

    # SSL/TLS configuration (only if domain provided)
    ENABLE_SSL="n"
    if [ -n "$DOMAIN_NAME" ]; then
        echo
        echo -e "${CYAN}Domain verified: ${GREEN}${DOMAIN_NAME}${NC}"
        echo -e "${CYAN}Enable SSL/TLS with Let's Encrypt? [Y/n]:${NC}"
        safe_read "-n 1 -r" "ENABLE_SSL"
        echo

        # Default to yes if they have a domain
        if [[ ! $ENABLE_SSL =~ ^[Nn]$ ]]; then
            ENABLE_SSL="y"
            echo -e "${CYAN}Enter your email for Let's Encrypt notifications:${NC}"
            safe_read "" "LETSENCRYPT_EMAIL"

            if [ -z "$LETSENCRYPT_EMAIL" ]; then
                print_warning "Email is recommended for SSL certificate expiration notices"
            fi
        else
            ENABLE_SSL="n"
        fi
    else
        print_info "No domain provided - SSL will be disabled"
        print_info "You can access Restreamer via IP address"
    fi

    # Set default ports based on SSL configuration
    if [[ $ENABLE_SSL =~ ^[Yy]$ ]]; then
        DEFAULT_HTTP_PORT=80
        DEFAULT_HTTPS_PORT=443
        print_info "Using standard SSL ports (80/443)"
    else
        DEFAULT_HTTP_PORT=8080
        DEFAULT_HTTPS_PORT=8181
        print_info "Using non-standard ports (8080/8181)"
    fi

    print_header "\n=== Port Configuration ==="

    # HTTP Port
    echo -e "${CYAN}Enter HTTP port [${DEFAULT_HTTP_PORT}]:${NC}"
    safe_read "" "HTTP_PORT"
    HTTP_PORT=${HTTP_PORT:-$DEFAULT_HTTP_PORT}

    # HTTPS Port
    echo -e "${CYAN}Enter HTTPS port [${DEFAULT_HTTPS_PORT}]:${NC}"
    safe_read "" "HTTPS_PORT"
    HTTPS_PORT=${HTTPS_PORT:-$DEFAULT_HTTPS_PORT}

    # RTMP Port
    echo -e "${CYAN}Enter RTMP port [1935]:${NC}"
    safe_read "" "RTMP_PORT"
    RTMP_PORT=${RTMP_PORT:-1935}

    # SRT Port
    echo -e "${CYAN}Enter SRT port [6000]:${NC}"
    safe_read "" "SRT_PORT"
    SRT_PORT=${SRT_PORT:-6000}

    print_header "\n=== Authentication Configuration ==="

    # Admin username
    echo -e "${CYAN}Enter admin username [admin]:${NC}"
    safe_read "" "ADMIN_USER"
    ADMIN_USER=${ADMIN_USER:-admin}

    # Admin password
    while true; do
        echo -e "${CYAN}Enter admin password:${NC}"
        safe_read "-s" "ADMIN_PASS"
        echo
        if [ -z "$ADMIN_PASS" ]; then
            print_error "Password cannot be empty"
            continue
        fi

        echo -e "${CYAN}Confirm admin password:${NC}"
        safe_read "-s" "ADMIN_PASS_CONFIRM"
        echo

        if [ "$ADMIN_PASS" = "$ADMIN_PASS_CONFIRM" ]; then
            break
        else
            print_error "Passwords do not match. Please try again."
        fi
    done

    print_header "\n=== Storage Configuration ==="

    # Storage path
    echo -e "${CYAN}Enter data storage path [${DATA_DIR}]:${NC}"
    safe_read "" "USER_DATA_DIR"
    DATA_DIR=${USER_DATA_DIR:-$DATA_DIR}

    # Summary
    print_header "\n=== Configuration Summary ==="
    echo "HTTP Port:       $HTTP_PORT"
    echo "HTTPS Port:      $HTTPS_PORT"
    echo "RTMP Port:       $RTMP_PORT"
    echo "SRT Port:        $SRT_PORT"
    echo "Admin User:      $ADMIN_USER"
    echo "Data Directory:  $DATA_DIR"
    if [[ $ENABLE_SSL =~ ^[Yy]$ ]]; then
        echo "SSL/TLS:         Enabled"
        echo "Domain:          $DOMAIN_NAME"
        echo "Email:           $LETSENCRYPT_EMAIL"
    else
        echo "SSL/TLS:         Disabled"
    fi
    echo

    echo -e "${CYAN}Proceed with installation? [Y/n]:${NC}"
    safe_read "-n 1 -r" "CONFIRM"
    echo
    if [[ $CONFIRM =~ ^[Nn]$ ]]; then
        print_info "Installation cancelled"
        exit 0
    fi
}

# Create directories
create_directories() {
    print_info "Creating directories..."

    mkdir -p "$INSTALL_DIR"
    mkdir -p "$CONFIG_DIR"
    mkdir -p "$DATA_DIR/config"
    mkdir -p "$DATA_DIR/data"

    DIRECTORIES_CREATED=true

    print_success "Directories created"
}

# Generate docker-compose.yml
generate_docker_compose() {
    print_info "Generating docker-compose configuration..."

    cat > "$INSTALL_DIR/docker-compose.yml" << EOF
version: '3.8'

services:
  restreamer:
    image: datarhei/restreamer:latest
    container_name: restreamer
    restart: unless-stopped
    ports:
      - "${HTTP_PORT}:8080"
      - "${HTTPS_PORT}:8181"
      - "${RTMP_PORT}:1935"
      - "${SRT_PORT}:6000/udp"
    volumes:
      - ${DATA_DIR}/config:/core/config
      - ${DATA_DIR}/data:/core/data
    environment:
EOF

    # Add SSL/TLS configuration if enabled
    if [[ $ENABLE_SSL =~ ^[Yy]$ ]]; then
        cat >> "$INSTALL_DIR/docker-compose.yml" << EOF
      - CORE_ADDRESS=:80
      - CORE_HOST_NAME=["${DOMAIN_NAME}"]
      - CORE_HOST_AUTO=false
      - CORE_TLS_ADDRESS=:8181
      - CORE_TLS_ENABLE=true
      - CORE_TLS_AUTO=true
      - CORE_TLS_EMAIL=${LETSENCRYPT_EMAIL}
EOF
    else
        cat >> "$INSTALL_DIR/docker-compose.yml" << EOF
      - CORE_ADDRESS=:8080
EOF
    fi

    # Add authentication
    cat >> "$INSTALL_DIR/docker-compose.yml" << EOF
      - CORE_API_AUTH_ENABLE=true
      - CORE_API_AUTH_USERNAME=${ADMIN_USER}
      - CORE_API_AUTH_PASSWORD=${ADMIN_PASS}
      - CORE_STORAGE_DISK_DIR=/core/data
      - CORE_RTMP_ENABLE=true
      - CORE_RTMP_ADDRESS=:1935
      - CORE_SRT_ENABLE=true
      - CORE_SRT_ADDRESS=:6000
    logging:
      driver: "json-file"
      options:
        max-size: "10m"
        max-file: "3"
EOF

    print_success "Docker Compose configuration created"
}

# Create systemd service
create_systemd_service() {
    print_info "Creating systemd service..."

    local exec_start
    local exec_stop

    if [ "$DOCKER_COMPOSE_CMD" = "docker compose" ]; then
        exec_start="/usr/bin/docker compose up -d"
        exec_stop="/usr/bin/docker compose down"
    else
        local dc_path=$(command -v docker-compose)
        exec_start="$dc_path up -d"
        exec_stop="$dc_path down"
    fi

    cat > "/etc/systemd/system/${SERVICE_NAME}.service" << EOF
[Unit]
Description=Datarhei Restreamer
Requires=docker.service
After=docker.service

[Service]
Type=oneshot
RemainAfterExit=yes
WorkingDirectory=${INSTALL_DIR}
ExecStart=${exec_start}
ExecStop=${exec_stop}
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF

    # Reload systemd
    systemctl daemon-reload
    systemctl enable ${SERVICE_NAME}.service

    SYSTEMD_SERVICE_CREATED=true

    print_success "Systemd service created and enabled"
}

# Start Restreamer
start_restreamer() {
    print_info "Starting Restreamer..."

    cd "$INSTALL_DIR"
    if ! $DOCKER_COMPOSE_CMD up -d 2>&1 | tee /tmp/docker_compose_output.log; then
        print_error "Failed to start Docker Compose"
        exit 1
    fi

    # Check if the output contains error messages
    if grep -qi "error\|invalid\|failed" /tmp/docker_compose_output.log; then
        print_error "Docker Compose reported errors. Check the output above."
        rm -f /tmp/docker_compose_output.log
        exit 1
    fi
    rm -f /tmp/docker_compose_output.log

    CONTAINER_STARTED=true

    # Wait for container to be healthy
    print_info "Waiting for Restreamer to start..."
    sleep 5

    if docker ps | grep -q restreamer; then
        print_success "Restreamer started successfully"
    else
        print_error "Failed to start Restreamer. Check logs with: docker compose logs"
        exit 1
    fi
}

# Create helper scripts
create_helper_scripts() {
    print_info "Creating helper scripts..."

    # Create update script
    cat > "$INSTALL_DIR/update.sh" << EOF
#!/bin/bash
set -e

echo "Updating Restreamer..."

cd /opt/restreamer
$DOCKER_COMPOSE_CMD pull
$DOCKER_COMPOSE_CMD up -d

echo "Restreamer updated successfully"
EOF

    chmod +x "$INSTALL_DIR/update.sh"

    # Create uninstall script
    cat > "$INSTALL_DIR/uninstall.sh" << EOF
#!/bin/bash
set -e

# Colors
RED='\033[0;31m'
NC='\033[0m'

echo -e "\${RED}WARNING: THIS WILL DELETE ALL RECORDINGS AND DATA!\${NC}"
read -p "Are you sure you want to uninstall Restreamer? [y/N]: " -n 1 -r
echo
if [[ ! \$REPLY =~ ^[Yy]$ ]]; then
    echo "Uninstall cancelled"
    exit 0
fi

echo "Stopping and removing Restreamer..."

# Stop service
systemctl stop restreamer 2>/dev/null || true
systemctl disable restreamer 2>/dev/null || true

# Remove systemd service
rm -f /etc/systemd/system/restreamer.service
systemctl daemon-reload

# Stop and remove containers
cd /opt/restreamer
$DOCKER_COMPOSE_CMD down -v 2>/dev/null || true

# Remove directories
read -p "Remove data directory /var/lib/restreamer? [y/N]: " -n 1 -r
echo
if [[ \$REPLY =~ ^[Yy]$ ]]; then
    rm -rf /var/lib/restreamer
fi

rm -rf /opt/restreamer
rm -rf /etc/restreamer

echo "Restreamer uninstalled successfully"
EOF

    chmod +x "$INSTALL_DIR/uninstall.sh"

    print_success "Helper scripts created"
}

# Save configuration
save_configuration() {
    print_info "Saving configuration..."

    cat > "$CONFIG_DIR/config.env" << EOF
HTTP_PORT=$HTTP_PORT
HTTPS_PORT=$HTTPS_PORT
RTMP_PORT=$RTMP_PORT
SRT_PORT=$SRT_PORT
ADMIN_USER=$ADMIN_USER
DATA_DIR=$DATA_DIR
INSTALL_DIR=$INSTALL_DIR
ENABLE_SSL=$ENABLE_SSL
DOMAIN_NAME=$DOMAIN_NAME
LETSENCRYPT_EMAIL=$LETSENCRYPT_EMAIL
INSTALLED_DATE=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
EOF

    chmod 600 "$CONFIG_DIR/config.env"

    print_success "Configuration saved to $CONFIG_DIR/config.env"
}

# Print final instructions
print_final_instructions() {
    print_header "\n=== Installation Complete ==="
    print_success "Datarhei Restreamer has been successfully installed!"

    echo
    echo -e "${GREEN}Access Information:${NC}"

    if [[ $ENABLE_SSL =~ ^[Yy]$ ]]; then
        echo -e "  Web UI:      ${CYAN}https://${DOMAIN_NAME}:${HTTPS_PORT}${NC}"
    else
        echo -e "  Web UI:      ${CYAN}http://$(hostname -I | awk '{print $1}'):${HTTP_PORT}${NC}"
    fi

    echo -e "  Username:    ${CYAN}${ADMIN_USER}${NC}"
    echo -e "  Password:    ${CYAN}********${NC} (the one you set)"

    echo
    echo -e "${GREEN}RTMP/SRT Endpoints:${NC}"
    echo -e "  RTMP:        ${CYAN}rtmp://$(hostname -I | awk '{print $1}'):${RTMP_PORT}/live${NC}"
    echo -e "  SRT:         ${CYAN}srt://$(hostname -I | awk '{print $1}'):${SRT_PORT}${NC}"

    echo
    echo -e "${GREEN}Useful Commands:${NC}"
    echo -e "  Update:      ${CYAN}sudo $INSTALL_DIR/update.sh${NC}"
    echo -e "  Uninstall:   ${CYAN}sudo $INSTALL_DIR/uninstall.sh${NC}"
    echo -e "  Restart:     ${CYAN}sudo systemctl restart restreamer${NC}"
    echo -e "  Stop:        ${CYAN}sudo systemctl stop restreamer${NC}"
    echo -e "  Logs:        ${CYAN}sudo docker compose -f $INSTALL_DIR/docker-compose.yml logs -f${NC}"

    if [[ ! $ENABLE_SSL =~ ^[Yy]$ ]]; then
        echo
        print_warning "SSL is not enabled. Consider enabling it for production use."
        print_info "You can reconfigure SSL later by editing $INSTALL_DIR/docker-compose.yml"
    fi

    INSTALLATION_COMPLETED=true

    echo
}

# Show introduction and get user confirmation
show_introduction() {
    echo
    echo -e "${CYAN}Welcome to the Datarhei Restreamer installer!${NC}"
    echo
    echo "This interactive script will guide you through installing and configuring"
    echo "Datarhei Restreamer on your Linux server."
    echo
    echo -e "${GREEN}What this installer will do:${NC}"
    echo "  • Detect your Linux distribution and version"
    echo "  • Install Docker (if not already installed)"
    echo "  • Configure Restreamer with your custom settings"
    echo "  • Set up SSL/TLS with Let's Encrypt (optional)"
    echo "  • Create a systemd service for auto-start on boot"
    echo "  • Generate update and uninstall scripts"
    echo
    echo -e "${GREEN}What you'll need to provide:${NC}"
    echo "  • Domain name (optional, required for SSL)"
    echo "  • Port numbers (or use defaults)"
    echo "  • Admin username and password"
    echo "  • Storage location for recordings"
    echo
    echo -e "${YELLOW}Time required:${NC} 5-10 minutes (depending on your internet speed)"
    echo -e "${YELLOW}Disk space:${NC} At least 2GB available"
    echo
    echo -e "${BLUE}After installation, you'll be able to:${NC}"
    echo "  • Access Restreamer web interface"
    echo "  • Stream via RTMP or SRT protocols"
    echo "  • Easily update or uninstall Restreamer"
    echo
    echo -e "${GREEN}Ready to begin installation? [Y/n]:${NC}"
    safe_read "-n 1 -r" "REPLY"
    echo
    if [[ $REPLY =~ ^[Nn]$ ]]; then
        print_info "Installation cancelled by user"
        exit 0
    fi

    INSTALLATION_STARTED=true
    echo
}

# Main installation flow
main() {
    print_header "╔═══════════════════════════════════════════════════════════╗"
    print_header "║      Datarhei Restreamer - Interactive Installer         ║"
    print_header "╚═══════════════════════════════════════════════════════════╝"

    show_introduction
    check_root
    detect_distro
    check_requirements
    install_docker
    gather_configuration
    create_directories
    generate_docker_compose
    create_systemd_service
    start_restreamer
    create_helper_scripts
    save_configuration
    print_final_instructions
}

# Run main function
main "$@"
