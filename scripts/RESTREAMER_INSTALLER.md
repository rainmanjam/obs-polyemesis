# Datarhei Restreamer - One-Line Installer

A simple, interactive installer for [Datarhei Restreamer](https://datarhei.github.io/restreamer/) on Linux systems.

## Quick Start

Run this single command on your Linux server:

```bash
curl -fsSL https://raw.githubusercontent.com/rainmanjam/obs-polyemesis/main/scripts/install-restreamer.sh | sudo bash
```

That's it! The installer will walk you through the setup process.

## What Gets Installed

- **Docker** (if not already installed)
- **Datarhei Restreamer** (latest version)
- **Systemd service** (auto-start on boot)
- **Helper scripts** (update, uninstall)

## Requirements

- **Linux Distribution**: Ubuntu, Debian, CentOS, Fedora, RHEL, or Arch Linux
- **Root Access**: sudo or root privileges
- **Internet Connection**: to download Docker and Restreamer
- **Disk Space**: At least 2GB available
- **curl**: Usually pre-installed (installer will check)

## Installation Process

The installer asks you a series of questions:

### 1. Domain and SSL

```
Do you have a domain name for this Restreamer instance?
Enter domain name (leave empty to use IP address): stream.example.com

Enable SSL/TLS with Let's Encrypt? [Y/n]: y
Enter your email for Let's Encrypt notifications: admin@example.com
```

**With Domain + SSL**: Uses ports 80/443 (standard)
**Without Domain**: Uses ports 8080/8181 (to avoid conflicts)

### 2. Ports

```
Enter HTTP port [80]:
Enter HTTPS port [443]:
Enter RTMP port [1935]:
Enter SRT port [6000]:
```

Press Enter to accept defaults, or type custom port numbers.

### 3. Authentication

```
Enter admin username [admin]: myadmin
Enter admin password: ********
Confirm admin password: ********
```

Choose a strong password for the web interface.

### 4. Storage

```
Enter data storage path [/var/lib/restreamer]:
```

Where recordings and configuration will be stored.

### 5. Confirmation

Review your settings and confirm to proceed with installation.

## After Installation

### Access Restreamer

**With SSL enabled:**
```
https://your-domain.com
```

**Without SSL:**
```
http://your-server-ip:8080
```

Login with the username and password you created.

### RTMP/SRT Streaming

**RTMP URL:**
```
rtmp://your-server-ip:1935/live
```

**SRT URL:**
```
srt://your-server-ip:6000
```

Use these URLs in OBS or other streaming software.

## Managing Restreamer

### Update to Latest Version

```bash
sudo /opt/restreamer/update.sh
```

Updates Restreamer to the latest Docker image.

### Restart Service

```bash
sudo systemctl restart restreamer
```

### Stop Service

```bash
sudo systemctl stop restreamer
```

### View Logs

```bash
sudo docker compose -f /opt/restreamer/docker-compose.yml logs -f
```

Press `Ctrl+C` to exit log viewing.

### Check Status

```bash
sudo systemctl status restreamer
```

## Uninstalling

To completely remove Restreamer:

```bash
sudo /opt/restreamer/uninstall.sh
```

This script will:
- Stop and remove the Restreamer service
- Remove Docker containers
- Ask if you want to remove data (recordings/config)
- Ask if you want to remove Docker entirely

## Troubleshooting

### Firewall Issues

If you can't access Restreamer, you may need to open ports in your firewall:

**Ubuntu/Debian (ufw):**
```bash
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw allow 1935/tcp
sudo ufw allow 6000/udp
```

**CentOS/RHEL/Fedora (firewalld):**
```bash
sudo firewall-cmd --permanent --add-port=80/tcp
sudo firewall-cmd --permanent --add-port=443/tcp
sudo firewall-cmd --permanent --add-port=1935/tcp
sudo firewall-cmd --permanent --add-port=6000/udp
sudo firewall-cmd --reload
```

### SSL Certificate Issues

Let's Encrypt requires:
- A valid domain name pointing to your server
- Ports 80 and 443 accessible from the internet
- Valid email address

If SSL setup fails:
1. Verify your domain DNS points to the server
2. Check firewall allows ports 80 and 443
3. Reinstall and try again

### Container Won't Start

Check logs for errors:
```bash
sudo docker compose -f /opt/restreamer/docker-compose.yml logs
```

Common issues:
- **Port already in use**: Another service is using the port
- **Permission denied**: Check data directory permissions
- **Out of disk space**: Free up storage

### Port Already in Use

If you get "port already in use" errors:

1. Find what's using the port:
   ```bash
   sudo lsof -i :8080
   ```

2. Either stop that service or choose different ports during installation

## Directory Structure

```
/opt/restreamer/
├── docker-compose.yml    # Container configuration
├── update.sh            # Update script
└── uninstall.sh         # Uninstall script

/etc/restreamer/
└── config.env           # Installation configuration

/var/lib/restreamer/
├── config/              # Restreamer configuration
└── data/                # Stream data and recordings
```

## Advanced Usage

### Manual Docker Commands

Start:
```bash
cd /opt/restreamer && sudo docker compose up -d
```

Stop:
```bash
cd /opt/restreamer && sudo docker compose down
```

Restart:
```bash
cd /opt/restreamer && sudo docker compose restart
```

### Edit Configuration

Edit docker-compose file:
```bash
sudo nano /opt/restreamer/docker-compose.yml
```

After editing, restart:
```bash
sudo systemctl restart restreamer
```

### Backup Data

Backup your data directory:
```bash
sudo tar -czf restreamer-backup.tar.gz /var/lib/restreamer
```

Restore from backup:
```bash
sudo systemctl stop restreamer
sudo tar -xzf restreamer-backup.tar.gz -C /
sudo systemctl start restreamer
```

## Security Recommendations

1. **Use SSL**: Always enable SSL with Let's Encrypt for production
2. **Strong Password**: Use a complex admin password
3. **Firewall**: Only open necessary ports
4. **Updates**: Regularly run the update script
5. **Backups**: Backup your data directory regularly

## Support

- **Restreamer Documentation**: https://datarhei.github.io/restreamer/
- **Restreamer GitHub**: https://github.com/datarhei/restreamer
- **Report Installer Issues**: https://github.com/rainmanjam/obs-polyemesis/issues

## License

This installer script is part of the obs-polyemesis project.

## Credits

- Installer created for obs-polyemesis
- Restreamer by datarhei: https://datarhei.com/

## Script Source Code

```bash
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
        read -n 1 -r
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

        read -p "$(echo -e ${CYAN}Do you want to reinstall Docker? [y/N]:${NC} )" -n 1 -r
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

        read -p "$(echo -e ${CYAN}Continue anyway? [y/N]:${NC} )" -n 1 -r
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

        read -p "$(echo -e ${CYAN}Continue with this domain anyway? [y/N]:${NC} )" -n 1 -r
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
        read DOMAIN_NAME

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
        read -p "$(echo -e ${CYAN}Enable SSL/TLS with Let\'s Encrypt? [Y/n]:${NC} )" -n 1 -r ENABLE_SSL
        echo

        # Default to yes if they have a domain
        if [[ ! $ENABLE_SSL =~ ^[Nn]$ ]]; then
            ENABLE_SSL="y"
            read -p "$(echo -e ${CYAN}Enter your email for Let\'s Encrypt notifications:${NC} )" LETSENCRYPT_EMAIL

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
    read -p "$(echo -e ${CYAN}Enter HTTP port [${DEFAULT_HTTP_PORT}]:${NC} )" HTTP_PORT
    HTTP_PORT=${HTTP_PORT:-$DEFAULT_HTTP_PORT}

    # HTTPS Port
    read -p "$(echo -e ${CYAN}Enter HTTPS port [${DEFAULT_HTTPS_PORT}]:${NC} )" HTTPS_PORT
    HTTPS_PORT=${HTTPS_PORT:-$DEFAULT_HTTPS_PORT}

    # RTMP Port
    read -p "$(echo -e ${CYAN}Enter RTMP port [1935]:${NC} )" RTMP_PORT
    RTMP_PORT=${RTMP_PORT:-1935}

    # SRT Port
    read -p "$(echo -e ${CYAN}Enter SRT port [6000]:${NC} )" SRT_PORT
    SRT_PORT=${SRT_PORT:-6000}

    print_header "\n=== Authentication Configuration ==="

    # Admin username
    read -p "$(echo -e ${CYAN}Enter admin username [admin]:${NC} )" ADMIN_USER
    ADMIN_USER=${ADMIN_USER:-admin}

    # Admin password
    while true; do
        read -s -p "$(echo -e ${CYAN}Enter admin password:${NC} )" ADMIN_PASS
        echo
        if [ -z "$ADMIN_PASS" ]; then
            print_error "Password cannot be empty"
            continue
        fi

        read -s -p "$(echo -e ${CYAN}Confirm admin password:${NC} )" ADMIN_PASS_CONFIRM
        echo

        if [ "$ADMIN_PASS" = "$ADMIN_PASS_CONFIRM" ]; then
            break
        else
            print_error "Passwords do not match. Please try again."
        fi
    done

    print_header "\n=== Storage Configuration ==="

    # Storage path
    read -p "$(echo -e ${CYAN}Enter data storage path [${DATA_DIR}]:${NC} )" USER_DATA_DIR
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

    read -p "$(echo -e ${CYAN}Proceed with installation? [Y/n]:${NC} )" -n 1 -r CONFIRM
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
      - CORE_TLS_MAIL=${LETSENCRYPT_EMAIL}
EOF
    else
        cat >> "$INSTALL_DIR/docker-compose.yml" << EOF
      - CORE_ADDRESS=:8080
EOF
    fi

    # Add authentication
    cat >> "$INSTALL_DIR/docker-compose.yml" << EOF
      - CORE_AUTH_ENABLE=true
      - CORE_AUTH_USERNAME=${ADMIN_USER}
      - CORE_AUTH_PASSWORD=${ADMIN_PASS}
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
    $DOCKER_COMPOSE_CMD up -d

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
    read -p "$(echo -e ${GREEN}Ready to begin installation? [Y/n]:${NC} )" -n 1 -r
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

```
