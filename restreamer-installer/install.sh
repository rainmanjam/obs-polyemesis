#!/bin/bash
#
# Polyemesis Restreamer Installer
# One-line installation and configuration for datarhei Restreamer
#
# Usage: curl -fsSL https://raw.githubusercontent.com/rainmanjam/obs-polyemesis/main/restreamer-installer/install.sh | bash
#

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration variables
RESTREAMER_VERSION="${RESTREAMER_VERSION:-latest}"
RESTREAMER_CONTAINER_NAME="restreamer"
DATA_DIR="/var/lib/restreamer"
BACKUP_DIR="/var/backups/restreamer"
WATCHTOWER_CONTAINER_NAME="watchtower"

# Installation state tracking for rollback
INSTALLATION_STARTED=false
DOCKER_INSTALLED_BY_US=false
RESTREAMER_CONTAINER_CREATED=false
WATCHTOWER_INSTALLED_BY_US=false
DIRECTORIES_CREATED=false
SCRIPTS_CREATED=false
FIREWALL_CONFIGURED=false
CANCELLED_BY_USER=false
INSTALLATION_COMPLETE=false

# Print functions
print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  Polyemesis Restreamer Installer${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo ""
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}!${NC} $1"
}

print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

print_step() {
    echo -e "\n${BLUE}==>${NC} $1"
}

# Error handler
error_exit() {
    print_error "$1"
    CANCELLED_BY_USER=true
    exit 1
}

# Cleanup function for rollback
cleanup_installation() {
    # Don't run cleanup if installation completed successfully
    if [ "$INSTALLATION_COMPLETE" = true ]; then
        return 0
    fi

    # Don't run cleanup if nothing was started
    if [ "$INSTALLATION_STARTED" = false ]; then
        return 0
    fi

    echo ""
    echo -e "${YELLOW}================================================${NC}"
    echo -e "${YELLOW}  Installation Interrupted${NC}"
    echo -e "${YELLOW}================================================${NC}"
    echo ""

    if [ "$CANCELLED_BY_USER" = true ]; then
        print_warning "Installation was cancelled or failed"
    else
        print_warning "Installation was interrupted (Ctrl+C pressed)"
    fi

    echo ""
    echo "The following items were installed or configured:"
    [ "$DOCKER_INSTALLED_BY_US" = true ] && echo "  • Docker"
    [ "$RESTREAMER_CONTAINER_CREATED" = true ] && echo "  • Restreamer container"
    [ "$WATCHTOWER_INSTALLED_BY_US" = true ] && echo "  • Watchtower"
    [ "$DIRECTORIES_CREATED" = true ] && echo "  • Data directories"
    [ "$SCRIPTS_CREATED" = true ] && echo "  • Management scripts"
    [ "$FIREWALL_CONFIGURED" = true ] && echo "  • Firewall rules"

    echo ""
    read -p "Would you like to remove what was installed and start fresh? [y/N]: " CLEANUP_CHOICE
    CLEANUP_CHOICE=${CLEANUP_CHOICE:-N}

    if [[ ! $CLEANUP_CHOICE =~ ^[Yy]$ ]]; then
        echo ""
        print_info "Keeping installed components"
        print_info "You can:"
        print_info "  - Re-run the installer to continue/complete setup"
        print_info "  - Manually clean up with: sudo bash uninstall.sh"
        echo ""
        return 0
    fi

    echo ""
    print_step "Rolling back installation"

    # Remove Restreamer container
    if [ "$RESTREAMER_CONTAINER_CREATED" = true ]; then
        print_info "Removing Restreamer container..."
        docker stop $RESTREAMER_CONTAINER_NAME 2>/dev/null || true
        docker rm $RESTREAMER_CONTAINER_NAME 2>/dev/null || true
        print_success "Restreamer container removed"
    fi

    # Remove Watchtower
    if [ "$WATCHTOWER_INSTALLED_BY_US" = true ]; then
        print_info "Removing Watchtower..."
        docker stop $WATCHTOWER_CONTAINER_NAME 2>/dev/null || true
        docker rm $WATCHTOWER_CONTAINER_NAME 2>/dev/null || true
        print_success "Watchtower removed"
    fi

    # Remove management scripts
    if [ "$SCRIPTS_CREATED" = true ]; then
        print_info "Removing management scripts..."
        rm -f /usr/local/bin/restreamer-manage
        rm -f /usr/local/bin/restreamer-backup
        rm -f /usr/local/bin/restreamer-restore
        rm -f /usr/local/bin/restreamer-monitor
        print_success "Management scripts removed"
    fi

    # Remove cron jobs
    print_info "Removing cron jobs..."
    crontab -l 2>/dev/null | grep -v restreamer | crontab - 2>/dev/null || true
    print_success "Cron jobs removed"

    # Remove directories
    if [ "$DIRECTORIES_CREATED" = true ]; then
        read -p "Remove data directories? (This deletes any data created) [y/N]: " REMOVE_DATA
        REMOVE_DATA=${REMOVE_DATA:-N}
        if [[ $REMOVE_DATA =~ ^[Yy]$ ]]; then
            print_info "Removing data directories..."
            rm -rf "$DATA_DIR" 2>/dev/null || true
            rm -rf "$BACKUP_DIR" 2>/dev/null || true
            print_success "Data directories removed"
        else
            print_info "Keeping data directories"
        fi
    fi

    # Note about Docker (don't auto-remove as it might have been pre-existing)
    if [ "$DOCKER_INSTALLED_BY_US" = true ]; then
        echo ""
        print_warning "Note: Docker was not removed"
        print_info "Docker may be useful for other purposes"
        print_info "If you want to remove it, use your package manager:"
        print_info "  Docker: apt/dnf/pacman remove docker-ce docker-ce-cli containerd.io"
    fi

    echo ""
    print_success "Rollback complete - system restored to pre-installation state"
    echo ""
    print_info "You can run the installer again when ready:"
    print_info "  curl -fsSL https://raw.githubusercontent.com/rainmanjam/obs-polyemesis/main/restreamer-installer/install.sh | sudo bash"
    echo ""
}

# Trap EXIT and SIGINT to run cleanup
trap cleanup_installation EXIT
trap 'CANCELLED_BY_USER=true; exit 130' INT

# Check if running as root or with sudo
check_root() {
    if [ "$EUID" -ne 0 ]; then
        error_exit "Please run as root or with sudo"
    fi
}

# Detect Linux distribution
detect_distro() {
    print_step "Detecting Linux distribution"

    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
        print_success "Detected: $NAME $VERSION"
    else
        error_exit "Unable to detect Linux distribution"
    fi

    # Set package manager based on distro
    case $DISTRO in
        ubuntu|debian|linuxmint|pop)
            PKG_MANAGER="apt"
            PKG_UPDATE="apt update"
            PKG_INSTALL="apt install -y"
            FIREWALL_CMD="ufw"
            ;;
        centos|rhel|fedora|rocky|almalinux)
            PKG_MANAGER="dnf"
            if ! command -v dnf &> /dev/null; then
                PKG_MANAGER="yum"
            fi
            PKG_UPDATE="$PKG_MANAGER check-update || true"
            PKG_INSTALL="$PKG_MANAGER install -y"
            FIREWALL_CMD="firewalld"
            ;;
        arch|manjaro)
            PKG_MANAGER="pacman"
            PKG_UPDATE="pacman -Sy"
            PKG_INSTALL="pacman -S --noconfirm"
            FIREWALL_CMD="ufw"
            ;;
        *)
            print_warning "Unsupported distribution: $DISTRO"
            print_info "Attempting to use generic commands"
            PKG_MANAGER="apt"
            PKG_UPDATE="apt update || yum check-update || pacman -Sy"
            PKG_INSTALL="apt install -y || yum install -y || pacman -S --noconfirm"
            FIREWALL_CMD="ufw"
            ;;
    esac
}

# Update system packages
update_system() {
    print_step "Updating system packages"
    eval $PKG_UPDATE
    print_success "System packages updated"
}

# Install required dependencies
install_dependencies() {
    print_step "Installing required dependencies"

    DEPS="curl wget ca-certificates gnupg lsb-release"

    case $PKG_MANAGER in
        apt)
            eval $PKG_INSTALL $DEPS
            ;;
        dnf|yum)
            eval $PKG_INSTALL curl wget ca-certificates gnupg
            ;;
        pacman)
            eval $PKG_INSTALL curl wget ca-certificates gnupg
            ;;
    esac

    print_success "Dependencies installed"
}

# Check if Docker is installed
check_docker() {
    if command -v docker &> /dev/null; then
        print_success "Docker is already installed"
        docker --version
        return 0
    else
        return 1
    fi
}

# Install Docker
install_docker() {
    print_step "Installing Docker"

    if check_docker; then
        return 0
    fi

    # Use official Docker installation script
    curl -fsSL https://get.docker.com -o /tmp/get-docker.sh
    sh /tmp/get-docker.sh
    rm /tmp/get-docker.sh

    # Start and enable Docker service
    systemctl start docker
    systemctl enable docker

    DOCKER_INSTALLED_BY_US=true
    print_success "Docker installed successfully"
    docker --version
}

# Configure Docker daemon
configure_docker() {
    print_step "Configuring Docker"

    # Create Docker daemon.json if it doesn't exist
    if [ ! -f /etc/docker/daemon.json ]; then
        cat > /etc/docker/daemon.json <<EOF
{
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "10m",
    "max-file": "3"
  }
}
EOF
        systemctl restart docker
        print_success "Docker configured with log rotation"
    fi
}

# Get user input
get_user_input() {
    print_step "Configuration"

    # Username
    read -p "Enter username for Restreamer admin: " RESTREAMER_USER
    while [ -z "$RESTREAMER_USER" ]; do
        print_warning "Username cannot be empty"
        read -p "Enter username for Restreamer admin: " RESTREAMER_USER
    done

    # Password
    read -s -p "Enter password for Restreamer admin: " RESTREAMER_PASS
    echo ""
    while [ -z "$RESTREAMER_PASS" ]; do
        print_warning "Password cannot be empty"
        read -s -p "Enter password for Restreamer admin: " RESTREAMER_PASS
        echo ""
    done

    read -s -p "Confirm password: " RESTREAMER_PASS_CONFIRM
    echo ""
    while [ "$RESTREAMER_PASS" != "$RESTREAMER_PASS_CONFIRM" ]; do
        print_error "Passwords do not match"
        read -s -p "Enter password for Restreamer admin: " RESTREAMER_PASS
        echo ""
        read -s -p "Confirm password: " RESTREAMER_PASS_CONFIRM
        echo ""
    done

    # Ask about domain name
    echo ""
    echo "Do you have a domain name you want to use for restreamer?"
    echo "  (e.g., stream.example.com)"
    echo ""
    echo "If you have a domain:"
    echo "  - Restreamer will be accessible via HTTPS on ports 80/443"
    echo "  - Automatic SSL certificate from Let's Encrypt"
    echo "  - Production-ready and secure"
    echo ""
    echo "If you don't have a domain:"
    echo "  - Restreamer will be accessible via HTTP on ports 8080/8181"
    echo "  - Perfect for local testing or private networks"
    echo ""
    read -p "Do you have a domain name you want to use for restreamer? [y/N]: " HAS_DOMAIN
    HAS_DOMAIN=${HAS_DOMAIN:-N}

    if [[ $HAS_DOMAIN =~ ^[Yy]$ ]]; then
        # HTTPS mode with domain
        INSTALL_MODE="https"
        HTTP_PORT=80
        HTTPS_PORT=443
        RTMP_PORT=1935
        SRT_PORT=6000

        echo ""
        read -p "Enter your domain name (e.g., stream.example.com): " DOMAIN_NAME
        while [ -z "$DOMAIN_NAME" ]; do
            print_warning "Domain name cannot be empty"
            read -p "Enter your domain name: " DOMAIN_NAME
        done

        # Validate domain format (basic check)
        if [[ ! $DOMAIN_NAME =~ ^[a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$ ]]; then
            print_warning "Domain name format looks unusual: $DOMAIN_NAME"
            read -p "Continue anyway? [y/N]: " CONTINUE_DOMAIN
            if [[ ! $CONTINUE_DOMAIN =~ ^[Yy]$ ]]; then
                error_exit "Installation cancelled"
            fi
        fi

        read -p "Enter email address for Let's Encrypt SSL notifications: " LETSENCRYPT_EMAIL
        while [ -z "$LETSENCRYPT_EMAIL" ]; do
            print_warning "Email cannot be empty"
            read -p "Enter email address: " LETSENCRYPT_EMAIL
        done

        echo ""
        print_success "✓ Configuration: HTTPS with domain $DOMAIN_NAME"
        print_info "Restreamer will be accessible at: https://$DOMAIN_NAME"
    else
        # HTTP mode without domain
        INSTALL_MODE="http"
        HTTP_PORT=8080
        HTTPS_PORT=8181
        RTMP_PORT=1935
        SRT_PORT=6000

        echo ""
        print_success "✓ Configuration: HTTP mode (no domain required)"
        print_info "Restreamer will be accessible at: http://YOUR-SERVER-IP:8080"
    fi

    # Auto-updates
    echo ""
    read -p "Enable automatic container updates? [Y/n]: " AUTO_UPDATE
    AUTO_UPDATE=${AUTO_UPDATE:-Y}

    # Monitoring
    echo ""
    read -p "Install monitoring tools? [Y/n]: " INSTALL_MONITORING
    INSTALL_MONITORING=${INSTALL_MONITORING:-Y}
}

# Create data directories
create_directories() {
    print_step "Creating data directories"

    mkdir -p "$DATA_DIR"
    mkdir -p "$BACKUP_DIR"
    chmod 755 "$DATA_DIR"
    chmod 755 "$BACKUP_DIR"

    DIRECTORIES_CREATED=true
    print_success "Data directories created"
}

# Configure firewall
configure_firewall() {
    print_step "Configuring firewall"

    case $FIREWALL_CMD in
        ufw)
            if command -v ufw &> /dev/null; then
                ufw allow $HTTP_PORT/tcp comment 'Restreamer HTTP/HTTPS'
                ufw allow $HTTPS_PORT/tcp comment 'Restreamer HTTPS/API'
                ufw allow $RTMP_PORT/tcp comment 'Restreamer RTMP'
                ufw allow $SRT_PORT/udp comment 'Restreamer SRT'

                # Enable UFW if not already enabled
                if ! ufw status | grep -q "Status: active"; then
                    # Detect SSH port to prevent lockout
                    SSH_PORT=$(ss -tnlp 2>/dev/null | grep sshd | grep -oP ':\K[0-9]+' | head -1)
                    SSH_PORT=${SSH_PORT:-22}  # Default to 22 if detection fails

                    print_warning "UFW is not active."
                    print_info "Detected SSH on port ${SSH_PORT}"

                    # Ensure SSH is allowed before enabling firewall
                    if ! ufw status | grep -q "${SSH_PORT}/tcp"; then
                        print_info "Adding firewall rule to preserve SSH access..."
                        ufw allow ${SSH_PORT}/tcp comment 'SSH - auto-detected'
                        print_success "SSH access secured on port ${SSH_PORT}"
                    fi

                    print_warning "Enable UFW firewall? [Y/n]"
                    read -p "> " ENABLE_UFW
                    ENABLE_UFW=${ENABLE_UFW:-Y}
                    if [[ $ENABLE_UFW =~ ^[Yy]$ ]]; then
                        ufw --force enable
                        print_success "UFW enabled with SSH access preserved"
                    fi
                fi

                FIREWALL_CONFIGURED=true
                print_success "Firewall rules added"
            else
                print_warning "UFW not installed, skipping firewall configuration"
            fi
            ;;
        firewalld)
            if command -v firewall-cmd &> /dev/null; then
                # Detect SSH port to prevent lockout
                SSH_PORT=$(ss -tnlp 2>/dev/null | grep sshd | grep -oP ':\K[0-9]+' | head -1)
                SSH_PORT=${SSH_PORT:-22}  # Default to 22 if detection fails

                # Ensure SSH is allowed
                if ! firewall-cmd --list-ports 2>/dev/null | grep -q "${SSH_PORT}/tcp"; then
                    print_info "Detected SSH on port ${SSH_PORT}"
                    print_info "Adding firewall rule to preserve SSH access..."
                    firewall-cmd --permanent --add-port=${SSH_PORT}/tcp
                    print_success "SSH access secured on port ${SSH_PORT}"
                fi

                # Add Restreamer ports
                firewall-cmd --permanent --add-port=$HTTP_PORT/tcp
                firewall-cmd --permanent --add-port=$HTTPS_PORT/tcp
                firewall-cmd --permanent --add-port=$RTMP_PORT/tcp
                firewall-cmd --permanent --add-port=$SRT_PORT/udp
                firewall-cmd --reload
                FIREWALL_CONFIGURED=true
                print_success "Firewall rules added"
            else
                print_warning "firewalld not installed, skipping firewall configuration"
            fi
            ;;
    esac
}

# Install Restreamer (HTTP mode)
install_restreamer_http() {
    print_step "Installing Restreamer (HTTP mode)"

    # Stop and remove existing container if it exists
    docker stop $RESTREAMER_CONTAINER_NAME 2>/dev/null || true
    docker rm $RESTREAMER_CONTAINER_NAME 2>/dev/null || true

    # Run Restreamer container
    docker run -d \
        --name $RESTREAMER_CONTAINER_NAME \
        --restart unless-stopped \
        -p $HTTP_PORT:8080 \
        -p $HTTPS_PORT:8181 \
        -p $RTMP_PORT:1935 \
        -p $SRT_PORT:6000/udp \
        -v $DATA_DIR:/core/data \
        -e CORE_USERNAME="$RESTREAMER_USER" \
        -e CORE_PASSWORD="$RESTREAMER_PASS" \
        datarhei/restreamer:$RESTREAMER_VERSION

    RESTREAMER_CONTAINER_CREATED=true
    print_success "Restreamer container started"

    # Wait for container to be ready
    print_info "Waiting for Restreamer to start..."
    sleep 10

    # Test connection
    if curl -s -f http://localhost:$HTTP_PORT/api > /dev/null; then
        print_success "Restreamer is running and accessible"
    else
        print_warning "Restreamer may still be starting up"
    fi
}

# Verify DNS configuration
verify_dns() {
    print_step "Verifying DNS configuration for $DOMAIN_NAME"

    # Get server's public IP
    print_info "Detecting server's public IP address..."
    SERVER_IP=$(curl -s -4 https://ifconfig.me 2>/dev/null || curl -s -4 https://api.ipify.org 2>/dev/null || curl -s -4 https://icanhazip.com 2>/dev/null)

    if [ -z "$SERVER_IP" ]; then
        print_warning "Could not detect server's public IP address"
        print_info "Attempting to continue anyway..."
        return 0
    fi

    print_success "Server public IP: $SERVER_IP"

    # Resolve domain name
    print_info "Resolving $DOMAIN_NAME..."
    DOMAIN_IP=$(dig +short $DOMAIN_NAME @8.8.8.8 2>/dev/null | grep -E '^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$' | head -1)

    if [ -z "$DOMAIN_IP" ]; then
        # Try with host command if dig is not available
        DOMAIN_IP=$(host $DOMAIN_NAME 8.8.8.8 2>/dev/null | grep "has address" | head -1 | awk '{print $4}')
    fi

    if [ -z "$DOMAIN_IP" ]; then
        # Try with nslookup as last resort
        DOMAIN_IP=$(nslookup $DOMAIN_NAME 8.8.8.8 2>/dev/null | grep -A1 "Name:" | grep "Address:" | head -1 | awk '{print $2}')
    fi

    if [ -z "$DOMAIN_IP" ]; then
        print_error "DNS resolution failed for $DOMAIN_NAME"
        echo ""
        echo -e "${YELLOW}DNS is NOT configured correctly!${NC}"
        echo ""
        echo "Before continuing, you need to:"
        echo "  1. Add an A record in your DNS settings:"
        echo "     Name: $DOMAIN_NAME"
        echo "     Type: A"
        echo "     Value: $SERVER_IP"
        echo "     TTL: 300 (or your provider's default)"
        echo ""
        echo "  2. Wait for DNS propagation (usually 5-15 minutes)"
        echo ""
        echo "  3. Verify with: dig $DOMAIN_NAME"
        echo ""
        read -p "Do you want to continue anyway? [y/N]: " CONTINUE_ANYWAY
        if [[ ! $CONTINUE_ANYWAY =~ ^[Yy]$ ]]; then
            error_exit "DNS verification failed. Please configure DNS and run the installer again."
        fi
        print_warning "Continuing without DNS verification - SSL certificate request will likely fail"
        return 1
    fi

    print_success "Domain resolves to: $DOMAIN_IP"

    # Compare IPs
    if [ "$SERVER_IP" = "$DOMAIN_IP" ]; then
        print_success "DNS is configured correctly! $DOMAIN_NAME → $SERVER_IP"
        echo ""
        print_info "Waiting 5 seconds before proceeding with SSL certificate..."
        sleep 5
        return 0
    else
        print_error "DNS mismatch detected!"
        echo ""
        echo -e "${YELLOW}DNS Configuration Issue:${NC}"
        echo "  Domain $DOMAIN_NAME resolves to: $DOMAIN_IP"
        echo "  This server's IP address is:     $SERVER_IP"
        echo ""
        echo "This means:"
        echo "  - Your domain is pointing to a different server"
        echo "  - DNS changes haven't propagated yet"
        echo "  - You may have multiple A records configured"
        echo ""
        echo "To fix this:"
        echo "  1. Update your DNS A record to point to $SERVER_IP"
        echo "  2. Wait for DNS propagation (5-60 minutes depending on TTL)"
        echo "  3. Verify with: dig $DOMAIN_NAME"
        echo ""
        echo "Let's Encrypt will fail if DNS doesn't point to this server!"
        echo ""
        read -p "Do you want to continue anyway? [y/N]: " CONTINUE_ANYWAY
        if [[ ! $CONTINUE_ANYWAY =~ ^[Yy]$ ]]; then
            error_exit "DNS verification failed. Please fix DNS configuration and run the installer again."
        fi
        print_warning "Continuing with DNS mismatch - SSL certificate request will likely fail"
        return 1
    fi
}

# Install Restreamer (HTTPS mode with built-in Let's Encrypt)
install_restreamer_https() {
    print_step "Installing Restreamer (HTTPS mode with automatic SSL)"

    # Verify DNS before starting
    if ! verify_dns; then
        print_warning "DNS verification did not pass, but continuing as requested..."
        print_warning "Let's Encrypt certificate generation may fail"
    fi

    # Stop and remove existing container if it exists
    docker stop $RESTREAMER_CONTAINER_NAME 2>/dev/null || true
    docker rm $RESTREAMER_CONTAINER_NAME 2>/dev/null || true

    # Run Restreamer with built-in HTTPS support
    # Restreamer will automatically obtain and renew Let's Encrypt certificates
    print_info "Starting Restreamer with automatic HTTPS configuration..."
    docker run -d \
        --name $RESTREAMER_CONTAINER_NAME \
        --restart unless-stopped \
        -p 80:8080 \
        -p 443:8181 \
        -p $RTMP_PORT:1935 \
        -p $SRT_PORT:6000/udp \
        -v $DATA_DIR:/core/data \
        -e CORE_USERNAME="$RESTREAMER_USER" \
        -e CORE_PASSWORD="$RESTREAMER_PASS" \
        -e CORE_TLS_ENABLE=true \
        -e CORE_TLS_AUTO=true \
        -e CORE_HOST_NAME="$DOMAIN_NAME" \
        -e CORE_TLS_ADDRESS=:8181 \
        -e CORE_ADDRESS=:8080 \
        datarhei/restreamer:$RESTREAMER_VERSION

    RESTREAMER_CONTAINER_CREATED=true
    print_success "Restreamer container started with HTTPS enabled"

    # Wait for container to be ready
    print_info "Waiting for Restreamer to start and obtain SSL certificate..."
    print_info "This may take 30-60 seconds for Let's Encrypt validation..."
    sleep 15

    # Test connection (HTTP should redirect to HTTPS)
    if curl -s -f -L http://localhost/api > /dev/null 2>&1; then
        print_success "Restreamer is running and accessible"
    else
        print_warning "Restreamer may still be starting up or obtaining SSL certificate"
        print_info "Check logs with: docker logs $RESTREAMER_CONTAINER_NAME"
    fi

    echo ""
    print_info "Let's Encrypt certificate will be obtained automatically on first HTTPS access"
    print_info "Certificate renewal is automatic - no manual intervention needed"

    print_success "HTTPS setup complete"
}

# Install Watchtower for auto-updates
install_watchtower() {
    print_step "Installing Watchtower for automatic updates"

    docker run -d \
        --name $WATCHTOWER_CONTAINER_NAME \
        --restart unless-stopped \
        -v /var/run/docker.sock:/var/run/docker.sock \
        containrrr/watchtower \
        --interval 86400 \
        --cleanup \
        $RESTREAMER_CONTAINER_NAME

    WATCHTOWER_INSTALLED_BY_US=true
    print_success "Watchtower installed (daily update checks)"
}

# Create backup script
create_backup_script() {
    print_step "Creating backup script"

    cat > /usr/local/bin/restreamer-backup <<'EOF'
#!/bin/bash
# Restreamer Backup Script

BACKUP_DIR="/var/backups/restreamer"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE="$BACKUP_DIR/restreamer_backup_$TIMESTAMP.tar.gz"

echo "Creating backup..."
docker exec restreamer tar czf - /core/data > "$BACKUP_FILE"

if [ $? -eq 0 ]; then
    echo "✓ Backup created: $BACKUP_FILE"

    # Keep only last 7 backups
    ls -t $BACKUP_DIR/restreamer_backup_*.tar.gz | tail -n +8 | xargs rm -f 2>/dev/null
    echo "✓ Old backups cleaned up"
else
    echo "✗ Backup failed"
    exit 1
fi
EOF

    chmod +x /usr/local/bin/restreamer-backup

    # Create daily backup cron job
    (crontab -l 2>/dev/null; echo "0 2 * * * /usr/local/bin/restreamer-backup") | crontab -

    print_success "Backup script created at /usr/local/bin/restreamer-backup"
    print_info "Daily automatic backups scheduled at 2:00 AM"
}

# Create restore script
create_restore_script() {
    print_step "Creating restore script"

    cat > /usr/local/bin/restreamer-restore <<'EOF'
#!/bin/bash
# Restreamer Restore Script

BACKUP_DIR="/var/backups/restreamer"

if [ -z "$1" ]; then
    echo "Usage: restreamer-restore <backup_file>"
    echo ""
    echo "Available backups:"
    ls -lh $BACKUP_DIR/restreamer_backup_*.tar.gz 2>/dev/null || echo "  No backups found"
    exit 1
fi

BACKUP_FILE="$1"

if [ ! -f "$BACKUP_FILE" ]; then
    echo "✗ Backup file not found: $BACKUP_FILE"
    exit 1
fi

echo "Stopping Restreamer..."
docker stop restreamer

echo "Restoring from backup..."
docker exec restreamer tar xzf - -C / < "$BACKUP_FILE"

if [ $? -eq 0 ]; then
    echo "✓ Restore successful"
    docker start restreamer
    echo "✓ Restreamer restarted"
else
    echo "✗ Restore failed"
    docker start restreamer
    exit 1
fi
EOF

    chmod +x /usr/local/bin/restreamer-restore

    print_success "Restore script created at /usr/local/bin/restreamer-restore"
}

# Install monitoring tools
install_monitoring() {
    print_step "Installing monitoring tools"

    # Install htop if not present
    case $PKG_MANAGER in
        apt)
            eval $PKG_INSTALL htop
            ;;
        dnf|yum)
            eval $PKG_INSTALL htop
            ;;
        pacman)
            eval $PKG_INSTALL htop
            ;;
    esac

    # Create monitoring script
    cat > /usr/local/bin/restreamer-monitor <<'EOF'
#!/bin/bash
# Restreamer Monitoring Script

echo "================================================"
echo "  Restreamer Status"
echo "================================================"
echo ""

# Container status
echo "Container Status:"
docker ps --filter "name=restreamer" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
echo ""

# Resource usage
echo "Resource Usage:"
docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.NetIO}}" restreamer
echo ""

# Logs (last 20 lines)
echo "Recent Logs:"
docker logs --tail 20 restreamer
echo ""

echo "================================================"
echo "To view live logs: docker logs -f restreamer"
echo "To view container details: docker inspect restreamer"
echo "================================================"
EOF

    chmod +x /usr/local/bin/restreamer-monitor

    print_success "Monitoring tools installed"
    print_info "Run 'restreamer-monitor' to view status"
}

# Create management script
create_management_script() {
    print_step "Creating management script"

    cat > /usr/local/bin/restreamer-manage <<EOF
#!/bin/bash
# Restreamer Management Script

case "\$1" in
    start)
        docker start $RESTREAMER_CONTAINER_NAME
        echo "✓ Restreamer started"
        ;;
    stop)
        docker stop $RESTREAMER_CONTAINER_NAME
        echo "✓ Restreamer stopped"
        ;;
    restart)
        docker restart $RESTREAMER_CONTAINER_NAME
        echo "✓ Restreamer restarted"
        ;;
    status)
        /usr/local/bin/restreamer-monitor
        ;;
    logs)
        docker logs -f $RESTREAMER_CONTAINER_NAME
        ;;
    backup)
        /usr/local/bin/restreamer-backup
        ;;
    restore)
        /usr/local/bin/restreamer-restore "\$2"
        ;;
    update)
        docker pull datarhei/restreamer:$RESTREAMER_VERSION
        docker stop $RESTREAMER_CONTAINER_NAME
        docker rm $RESTREAMER_CONTAINER_NAME
        # Re-run with saved configuration
        echo "Please run the installer again to update with preserved settings"
        ;;
    *)
        echo "Restreamer Management Tool"
        echo ""
        echo "Usage: restreamer-manage {start|stop|restart|status|logs|backup|restore|update}"
        echo ""
        echo "Commands:"
        echo "  start   - Start Restreamer container"
        echo "  stop    - Stop Restreamer container"
        echo "  restart - Restart Restreamer container"
        echo "  status  - Show Restreamer status and resource usage"
        echo "  logs    - View live logs (Ctrl+C to exit)"
        echo "  backup  - Create backup of Restreamer data"
        echo "  restore - Restore from backup (requires backup file path)"
        echo "  update  - Update Restreamer to latest version"
        ;;
esac
EOF

    chmod +x /usr/local/bin/restreamer-manage

    SCRIPTS_CREATED=true
    print_success "Management script created at /usr/local/bin/restreamer-manage"
}

# Print completion message
print_completion() {
    echo ""
    echo -e "${GREEN}================================================${NC}"
    echo -e "${GREEN}  Installation Complete!${NC}"
    echo -e "${GREEN}================================================${NC}"
    echo ""

    if [ "$INSTALL_MODE" = "http" ]; then
        echo -e "${BLUE}Access Restreamer at:${NC}"
        echo -e "  Web UI: http://$(hostname -I | awk '{print $1}'):$HTTP_PORT"
        echo -e "  API:    http://$(hostname -I | awk '{print $1}'):$HTTP_PORT/api"
    else
        echo -e "${BLUE}Access Restreamer at:${NC}"
        echo -e "  Web UI: https://$DOMAIN_NAME"
        echo -e "  API:    https://$DOMAIN_NAME/api"
    fi

    echo ""
    echo -e "${BLUE}Credentials:${NC}"
    echo -e "  Username: $RESTREAMER_USER"
    echo -e "  Password: ********"
    echo ""

    echo -e "${BLUE}Streaming endpoints:${NC}"
    echo -e "  RTMP: rtmp://$(hostname -I | awk '{print $1}'):$RTMP_PORT"
    echo -e "  SRT:  srt://$(hostname -I | awk '{print $1}'):$SRT_PORT"
    echo ""

    echo -e "${BLUE}Management commands:${NC}"
    echo -e "  restreamer-manage start   - Start Restreamer"
    echo -e "  restreamer-manage stop    - Stop Restreamer"
    echo -e "  restreamer-manage restart - Restart Restreamer"
    echo -e "  restreamer-manage status  - View status and stats"
    echo -e "  restreamer-manage logs    - View live logs"
    echo -e "  restreamer-manage backup  - Create backup"
    echo -e "  restreamer-manage restore - Restore from backup"
    echo ""

    echo -e "${BLUE}Additional info:${NC}"
    echo -e "  Data directory: $DATA_DIR"
    echo -e "  Backup directory: $BACKUP_DIR"
    if [[ $AUTO_UPDATE =~ ^[Yy]$ ]]; then
        echo -e "  Auto-updates: Enabled (daily at midnight)"
    fi
    if [[ $INSTALL_MONITORING =~ ^[Yy]$ ]]; then
        echo -e "  Monitoring: Run 'restreamer-monitor' for details"
    fi
    echo ""

    echo -e "${YELLOW}Next steps:${NC}"
    echo -e "  1. Open the Web UI in your browser"
    echo -e "  2. Log in with your credentials"
    echo -e "  3. Configure your streaming destinations"
    echo -e "  4. Start streaming from OBS with the Polyemesis plugin!"
    echo ""

    echo -e "${GREEN}For support, visit: https://github.com/rainmanjam/obs-polyemesis${NC}"
    echo ""
}

# Main installation flow
main() {
    print_header
    INSTALLATION_STARTED=true

    check_root
    detect_distro
    update_system
    install_dependencies
    install_docker
    configure_docker
    get_user_input
    create_directories
    configure_firewall

    # Install based on mode
    if [ "$INSTALL_MODE" = "http" ]; then
        install_restreamer_http
    else
        install_restreamer_https
    fi

    # Optional features
    if [[ $AUTO_UPDATE =~ ^[Yy]$ ]]; then
        install_watchtower
    fi

    create_backup_script
    create_restore_script
    create_management_script

    if [[ $INSTALL_MONITORING =~ ^[Yy]$ ]]; then
        install_monitoring
    fi

    print_completion
    INSTALLATION_COMPLETE=true
}

# Run main function
main "$@"
