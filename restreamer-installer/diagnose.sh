#!/bin/bash
#
# Polyemesis Restreamer Diagnostic Tool
# Automatically checks for common issues and provides solutions
#
# Usage: curl -fsSL https://raw.githubusercontent.com/rainmanjam/obs-polyemesis/main/diagnose.sh | bash
#        or: sudo bash diagnose.sh
#

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASS="${GREEN}✓${NC}"
FAIL="${RED}✗${NC}"
WARN="${YELLOW}!${NC}"
INFO="${BLUE}ℹ${NC}"

print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  Restreamer Diagnostic Tool${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo ""
}

print_section() {
    echo ""
    echo -e "${BLUE}==>${NC} $1"
    echo ""
}

check_item() {
    local status=$1
    local message=$2
    local detail=$3

    if [ "$status" = "pass" ]; then
        echo -e "$PASS $message"
    elif [ "$status" = "fail" ]; then
        echo -e "$FAIL $message"
    elif [ "$status" = "warn" ]; then
        echo -e "$WARN $message"
    else
        echo -e "$INFO $message"
    fi

    if [ ! -z "$detail" ]; then
        echo -e "    $detail"
    fi
}

ISSUES_FOUND=0
WARNINGS_FOUND=0

# System checks
print_header
print_section "System Information"

# OS Detection
if [ -f /etc/os-release ]; then
    . /etc/os-release
    check_item "pass" "Operating System: $PRETTY_NAME"
else
    check_item "warn" "Could not detect operating system"
    ((WARNINGS_FOUND++))
fi

# Kernel version
KERNEL=$(uname -r)
check_item "pass" "Kernel: $KERNEL"

# Memory
TOTAL_MEM=$(free -h | awk '/^Mem:/ {print $2}')
AVAILABLE_MEM=$(free -h | awk '/^Mem:/ {print $7}')
check_item "info" "Memory: $AVAILABLE_MEM available of $TOTAL_MEM total"

MEM_MB=$(free -m | awk '/^Mem:/ {print $2}')
if [ "$MEM_MB" -lt 2048 ]; then
    check_item "warn" "Low memory detected (< 2GB)" "Restreamer may have performance issues"
    ((WARNINGS_FOUND++))
fi

# Disk space
DISK_USAGE=$(df -h /var/lib | awk 'NR==2 {print $5}' | sed 's/%//')
DISK_AVAIL=$(df -h /var/lib | awk 'NR==2 {print $4}')
if [ "$DISK_USAGE" -gt 90 ]; then
    check_item "fail" "Disk space critical: ${DISK_USAGE}% used, ${DISK_AVAIL} available" "Free up disk space immediately"
    ((ISSUES_FOUND++))
elif [ "$DISK_USAGE" -gt 80 ]; then
    check_item "warn" "Disk space low: ${DISK_USAGE}% used, ${DISK_AVAIL} available"
    ((WARNINGS_FOUND++))
else
    check_item "pass" "Disk space: ${DISK_USAGE}% used, ${DISK_AVAIL} available"
fi

# Docker checks
print_section "Docker Status"

if command -v docker &> /dev/null; then
    check_item "pass" "Docker installed"
    DOCKER_VERSION=$(docker --version)
    check_item "info" "$DOCKER_VERSION"

    # Docker daemon
    if systemctl is-active --quiet docker; then
        check_item "pass" "Docker daemon is running"
    else
        check_item "fail" "Docker daemon is not running" "Run: sudo systemctl start docker"
        ((ISSUES_FOUND++))
    fi

    # Docker daemon enabled
    if systemctl is-enabled --quiet docker; then
        check_item "pass" "Docker daemon auto-start enabled"
    else
        check_item "warn" "Docker daemon auto-start disabled" "Run: sudo systemctl enable docker"
        ((WARNINGS_FOUND++))
    fi
else
    check_item "fail" "Docker is not installed" "Install Docker using the installer script"
    ((ISSUES_FOUND++))
fi

# Container checks
print_section "Restreamer Container"

if docker ps -a --filter "name=restreamer" --format "{{.Names}}" | grep -q "^restreamer$"; then
    check_item "pass" "Restreamer container exists"

    # Check if running
    if docker ps --filter "name=restreamer" --format "{{.Names}}" | grep -q "^restreamer$"; then
        check_item "pass" "Restreamer container is running"

        # Get container stats
        CONTAINER_STATUS=$(docker inspect -f '{{.State.Status}}' restreamer)
        CONTAINER_UPTIME=$(docker inspect -f '{{.State.StartedAt}}' restreamer)
        check_item "info" "Status: $CONTAINER_STATUS since $CONTAINER_UPTIME"

        # Check resource usage
        CPU=$(docker stats --no-stream --format "{{.CPUPerc}}" restreamer)
        MEM=$(docker stats --no-stream --format "{{.MemUsage}}" restreamer)
        check_item "info" "CPU: $CPU, Memory: $MEM"

        # Check restart count
        RESTART_COUNT=$(docker inspect -f '{{.RestartCount}}' restreamer)
        if [ "$RESTART_COUNT" -gt 5 ]; then
            check_item "warn" "Container has restarted $RESTART_COUNT times" "Check logs: docker logs restreamer"
            ((WARNINGS_FOUND++))
        fi
    else
        check_item "fail" "Restreamer container is stopped" "Run: docker start restreamer"
        ((ISSUES_FOUND++))
    fi

    # Check container image
    IMAGE=$(docker inspect -f '{{.Config.Image}}' restreamer)
    check_item "info" "Image: $IMAGE"
else
    check_item "fail" "Restreamer container not found" "Run the installer script to create it"
    ((ISSUES_FOUND++))
fi

# Watchtower check
if docker ps -a --filter "name=watchtower" --format "{{.Names}}" | grep -q "^watchtower$"; then
    if docker ps --filter "name=watchtower" --format "{{.Names}}" | grep -q "^watchtower$"; then
        check_item "pass" "Watchtower is running (auto-updates enabled)"
    else
        check_item "warn" "Watchtower container exists but is stopped"
        ((WARNINGS_FOUND++))
    fi
else
    check_item "info" "Watchtower not installed (auto-updates disabled)"
fi

# Network checks
print_section "Network Connectivity"

# Check if ports are listening
if docker ps --filter "name=restreamer" --format "{{.Names}}" | grep -q "^restreamer$"; then
    # Check HTTP port
    if netstat -tuln 2>/dev/null | grep -q ":8080 " || ss -tuln 2>/dev/null | grep -q ":8080 "; then
        check_item "pass" "Port 8080 is listening"
    else
        if netstat -tuln 2>/dev/null | grep -q ":80 " || ss -tuln 2>/dev/null | grep -q ":80 "; then
            check_item "pass" "Port 80 is listening (HTTPS mode)"
        else
            check_item "warn" "HTTP port not listening" "Container may still be starting"
            ((WARNINGS_FOUND++))
        fi
    fi

    # Check RTMP port
    if netstat -tuln 2>/dev/null | grep -q ":1935 " || ss -tuln 2>/dev/null | grep -q ":1935 "; then
        check_item "pass" "Port 1935 (RTMP) is listening"
    else
        check_item "fail" "Port 1935 (RTMP) not listening"
        ((ISSUES_FOUND++))
    fi

    # Check SRT port
    if netstat -uln 2>/dev/null | grep -q ":6000 " || ss -uln 2>/dev/null | grep -q ":6000 "; then
        check_item "pass" "Port 6000 (SRT) is listening"
    else
        check_item "warn" "Port 6000 (SRT) not listening"
        ((WARNINGS_FOUND++))
    fi

    # Test API endpoint
    if curl -s -f http://localhost:8080/api > /dev/null 2>&1; then
        check_item "pass" "API endpoint responding"
    else
        if curl -s -f http://localhost:80/api > /dev/null 2>&1; then
            check_item "pass" "API endpoint responding (HTTPS mode)"
        else
            check_item "fail" "API endpoint not responding" "Check logs: docker logs restreamer"
            ((ISSUES_FOUND++))
        fi
    fi
fi

# Firewall checks
print_section "Firewall Configuration"

if command -v ufw &> /dev/null; then
    if ufw status | grep -q "Status: active"; then
        check_item "pass" "UFW firewall is active"

        # Check specific rules
        if ufw status | grep -q "8080"; then
            check_item "pass" "Port 8080 allowed in firewall"
        elif ufw status | grep -q "80"; then
            check_item "pass" "Port 80 allowed in firewall (HTTPS mode)"
        else
            check_item "fail" "HTTP port not allowed in firewall" "Run: sudo ufw allow 8080/tcp"
            ((ISSUES_FOUND++))
        fi

        if ufw status | grep -q "1935"; then
            check_item "pass" "Port 1935 (RTMP) allowed in firewall"
        else
            check_item "warn" "Port 1935 (RTMP) not allowed in firewall" "Run: sudo ufw allow 1935/tcp"
            ((WARNINGS_FOUND++))
        fi
    else
        check_item "info" "UFW firewall is inactive"
    fi
elif command -v firewall-cmd &> /dev/null; then
    if systemctl is-active --quiet firewalld; then
        check_item "pass" "firewalld is active"

        if firewall-cmd --list-ports | grep -q "8080/tcp"; then
            check_item "pass" "Port 8080 allowed in firewall"
        elif firewall-cmd --list-ports | grep -q "80/tcp"; then
            check_item "pass" "Port 80 allowed in firewall (HTTPS mode)"
        else
            check_item "fail" "HTTP port not allowed in firewall" "Run: sudo firewall-cmd --permanent --add-port=8080/tcp && sudo firewall-cmd --reload"
            ((ISSUES_FOUND++))
        fi
    else
        check_item "info" "firewalld is inactive"
    fi
else
    check_item "info" "No firewall detected"
fi

# Storage checks
print_section "Data & Backups"

if [ -d /var/lib/restreamer ]; then
    check_item "pass" "Data directory exists: /var/lib/restreamer"
    DATA_SIZE=$(du -sh /var/lib/restreamer 2>/dev/null | cut -f1)
    check_item "info" "Data directory size: $DATA_SIZE"
else
    check_item "warn" "Data directory not found: /var/lib/restreamer"
    ((WARNINGS_FOUND++))
fi

if [ -d /var/backups/restreamer ]; then
    check_item "pass" "Backup directory exists: /var/backups/restreamer"
    BACKUP_COUNT=$(ls -1 /var/backups/restreamer/*.tar.gz 2>/dev/null | wc -l)
    if [ "$BACKUP_COUNT" -gt 0 ]; then
        check_item "pass" "$BACKUP_COUNT backup(s) found"
        LATEST_BACKUP=$(ls -1t /var/backups/restreamer/*.tar.gz 2>/dev/null | head -1)
        if [ ! -z "$LATEST_BACKUP" ]; then
            BACKUP_AGE=$(stat -c %y "$LATEST_BACKUP" 2>/dev/null | cut -d' ' -f1)
            check_item "info" "Latest backup: $BACKUP_AGE"
        fi
    else
        check_item "warn" "No backups found" "Run: restreamer-backup"
        ((WARNINGS_FOUND++))
    fi
else
    check_item "warn" "Backup directory not found: /var/backups/restreamer"
    ((WARNINGS_FOUND++))
fi

# Check for backup cron job
if crontab -l 2>/dev/null | grep -q "restreamer-backup"; then
    check_item "pass" "Automatic backup cron job configured"
else
    check_item "warn" "Automatic backups not configured"
    ((WARNINGS_FOUND++))
fi

# Management scripts
print_section "Management Tools"

if [ -f /usr/local/bin/restreamer-manage ]; then
    check_item "pass" "Management script installed"
else
    check_item "warn" "Management script not found"
    ((WARNINGS_FOUND++))
fi

if [ -f /usr/local/bin/restreamer-backup ]; then
    check_item "pass" "Backup script installed"
else
    check_item "warn" "Backup script not found"
    ((WARNINGS_FOUND++))
fi

if [ -f /usr/local/bin/restreamer-restore ]; then
    check_item "pass" "Restore script installed"
else
    check_item "warn" "Restore script not found"
    ((WARNINGS_FOUND++))
fi

if [ -f /usr/local/bin/restreamer-monitor ]; then
    check_item "pass" "Monitor script installed"
else
    check_item "info" "Monitor script not found (optional)"
fi

# HTTPS checks
print_section "HTTPS Configuration"

# Check if Restreamer has HTTPS enabled by checking environment variables
if docker ps --filter "name=restreamer" --format "{{.Names}}" | grep -q "^restreamer$"; then
    HTTPS_ENABLED=$(docker inspect restreamer 2>/dev/null | grep -c "CORE_TLS_ENABLE=true")
    HTTPS_AUTO=$(docker inspect restreamer 2>/dev/null | grep -c "CORE_TLS_AUTO=true")

    if [ "$HTTPS_ENABLED" -gt 0 ]; then
        check_item "pass" "Restreamer HTTPS enabled (built-in Let's Encrypt)"

        if [ "$HTTPS_AUTO" -gt 0 ]; then
            check_item "pass" "Automatic SSL certificate management enabled"
            check_item "info" "SSL certificates are managed by Restreamer's built-in Let's Encrypt"
            check_item "info" "Certificates auto-renew automatically - no manual intervention needed"
        else
            check_item "info" "Manual SSL certificate configuration"
        fi

        # Check if HTTPS port is accessible
        if curl -s -k -f https://localhost:443/api > /dev/null 2>&1; then
            check_item "pass" "HTTPS endpoint responding"
        else
            check_item "warn" "HTTPS endpoint not accessible yet (may still be obtaining certificate)"
            check_item "info" "Check logs: docker logs restreamer"
        fi
    else
        check_item "info" "Restreamer running in HTTP mode (no HTTPS)"
    fi
else
    check_item "info" "Cannot check HTTPS config (container not running)"
fi

# Container logs check
print_section "Recent Errors"

if docker ps --filter "name=restreamer" --format "{{.Names}}" | grep -q "^restreamer$"; then
    ERROR_COUNT=$(docker logs --since 24h restreamer 2>&1 | grep -i "error" | wc -l)
    if [ "$ERROR_COUNT" -eq 0 ]; then
        check_item "pass" "No errors in recent logs (last 24h)"
    elif [ "$ERROR_COUNT" -lt 10 ]; then
        check_item "warn" "$ERROR_COUNT error(s) found in recent logs" "Run: docker logs restreamer | grep -i error"
        ((WARNINGS_FOUND++))
    else
        check_item "fail" "$ERROR_COUNT errors found in recent logs" "Run: docker logs restreamer"
        ((ISSUES_FOUND++))
    fi
fi

# Summary
echo ""
echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}  Diagnostic Summary${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""

if [ "$ISSUES_FOUND" -eq 0 ] && [ "$WARNINGS_FOUND" -eq 0 ]; then
    echo -e "$PASS ${GREEN}All checks passed! Restreamer appears to be healthy.${NC}"
elif [ "$ISSUES_FOUND" -eq 0 ]; then
    echo -e "$WARN ${YELLOW}$WARNINGS_FOUND warning(s) found. Restreamer should work but may need attention.${NC}"
else
    echo -e "$FAIL ${RED}$ISSUES_FOUND critical issue(s) found! Restreamer may not function correctly.${NC}"
    if [ "$WARNINGS_FOUND" -gt 0 ]; then
        echo -e "$WARN ${YELLOW}$WARNINGS_FOUND additional warning(s) found.${NC}"
    fi
fi

echo ""
echo -e "${BLUE}Next steps:${NC}"
if [ "$ISSUES_FOUND" -gt 0 ]; then
    echo -e "  1. Review critical issues above (marked with $FAIL)"
    echo -e "  2. Follow the suggested solutions"
    echo -e "  3. Run this diagnostic again to verify fixes"
    echo ""
    echo -e "${BLUE}Common fixes:${NC}"
    echo -e "  - Start Docker: sudo systemctl start docker"
    echo -e "  - Start container: docker start restreamer"
    echo -e "  - View logs: docker logs restreamer"
    echo -e "  - Restart container: docker restart restreamer"
    echo -e "  - Check firewall: sudo ufw status"
else
    echo -e "  1. Review warnings if any (marked with $WARN)"
    echo -e "  2. View detailed status: restreamer-manage status"
    echo -e "  3. Check logs: docker logs restreamer"
fi

echo ""
echo -e "${BLUE}For more help:${NC}"
echo -e "  - Full documentation: README.md"
echo -e "  - View logs: restreamer-manage logs"
echo -e "  - Status: restreamer-manage status"
echo -e "  - Issues: https://github.com/rainmanjam/obs-polyemesis/issues"
echo ""

exit $ISSUES_FOUND
