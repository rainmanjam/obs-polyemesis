# Quick Start Guide

Get Restreamer up and running in 5 minutes!

## Prerequisites

- Linux server (Ubuntu, Debian, CentOS, RHEL, Fedora, or Arch)
- Root/sudo access
- Internet connection

## Installation

### 1. Run the One-Line Installer

```bash
curl -fsSL https://raw.githubusercontent.com/rainmanjam/obs-polyemesis/main/install.sh | sudo bash
```

### 2. Answer the Prompts

**Admin Credentials:**
```
Enter username for Restreamer admin: admin
Enter password for Restreamer admin: ********
Confirm password: ********
```

**Domain Name Question:**
```
Do you have a domain name you want to use for restreamer?
  (e.g., stream.example.com)

If you have a domain:
  - Restreamer will be accessible via HTTPS on ports 80/443
  - Automatic SSL certificate from Let's Encrypt
  - Production-ready and secure

If you don't have a domain:
  - Restreamer will be accessible via HTTP on ports 8080/8181
  - Perfect for local testing or private networks

Do you have a domain name you want to use for restreamer? [y/N]:
```

**For local/testing use:** Answer **No** (just press Enter)
- You'll get HTTP mode on ports 8080/8181
- No domain or SSL configuration needed
- Quick and simple!

**For production with a domain:** Answer **Yes** and provide:
- Your domain name (e.g., `stream.example.com`)
- Email address for Let's Encrypt SSL notifications

**Important:** If you answer Yes, make sure your DNS A record is pointing to your server BEFORE running the installer. The installer will automatically verify DNS configuration and guide you if something isn't configured correctly.

**Optional Features:**
```
Enable automatic container updates? [Y/n]: Y
Install monitoring tools? [Y/n]: Y
```

Recommended: Say **yes** to both!

### 3. Wait for Installation

The installer will:
- Update your system
- Install Docker
- Download and configure Restreamer
- Set up firewall rules
- Configure backups and monitoring

This takes 2-5 minutes depending on your server.

### 4. Access Restreamer

Once complete, you'll see:

```
================================================
  Installation Complete!
================================================

Access Restreamer at:
  Web UI: http://192.168.1.100:8080
  API:    http://192.168.1.100:8080/api

Credentials:
  Username: admin
  Password: ********
```

Open the Web UI URL in your browser and log in!

## Connect from OBS Polyemesis

### 1. Install OBS Plugin

Download and install the OBS Polyemesis plugin:
```
https://github.com/rainmanjam/obs-polyemesis/releases
```

### 2. Open Restreamer Control

In OBS Studio:
1. Go to **View** → **Docks** → **Restreamer Control**
2. Click the **Connection** tab

### 3. Configure Connection

Enter your Restreamer details:
- **Host**: Your server IP or domain
- **Port**: `8080` (or `443` for HTTPS)
- **Username**: Your admin username
- **Password**: Your admin password

Click **Test Connection** - you should see ✓ Connected!

### 4. Start Streaming

1. Click the **Profiles** tab
2. Click **Add Profile**
3. Configure your streaming destinations (YouTube, Twitch, etc.)
4. Click **Start Streaming** in OBS
5. Your stream will go live on all configured platforms!

## Quick Commands

### View Status
```bash
restreamer-manage status
```

### View Live Logs
```bash
restreamer-manage logs
```

### Restart Restreamer
```bash
restreamer-manage restart
```

### Create Backup
```bash
restreamer-manage backup
```

### Stop Restreamer
```bash
restreamer-manage stop
```

### Start Restreamer
```bash
restreamer-manage start
```

## Troubleshooting

### Can't Access Web UI?

**Check if container is running:**
```bash
docker ps
```

You should see `restreamer` in the list.

**Check logs for errors:**
```bash
restreamer-manage logs
```

**Check firewall:**
```bash
sudo ufw status
```

Make sure port 8080 is allowed.

### Connection from OBS Fails?

1. **Verify Restreamer is running:**
   ```bash
   curl http://localhost:8080/api
   ```
   You should get a JSON response.

2. **Check credentials:**
   Make sure username and password are correct.

3. **Check firewall on server:**
   ```bash
   sudo ufw allow 8080/tcp
   ```

4. **Check network connectivity:**
   From your OBS computer, try:
   ```bash
   curl http://YOUR-SERVER-IP:8080/api
   ```

### Container Won't Start?

**View detailed error:**
```bash
docker logs restreamer
```

**Check Docker status:**
```bash
sudo systemctl status docker
```

**Restart Docker:**
```bash
sudo systemctl restart docker
docker start restreamer
```

## Advanced Usage

### HTTPS Setup (After Initial Install)

If you installed in HTTP mode but want to switch to HTTPS:

1. Get a domain name
2. Point it to your server's IP
3. Run the installer again and choose HTTPS mode
4. Or manually set up nginx + Let's Encrypt (see README)

### Add More Streaming Destinations

In the Restreamer Web UI:
1. Go to **Processes**
2. Create a new process
3. Add outputs for each platform
4. Configure with your stream keys

Or use OBS Polyemesis plugin:
1. Open **Restreamer Control** dock
2. **Profiles** tab → **Add Profile**
3. Configure destinations
4. OBS will manage everything!

### Monitor Resource Usage

```bash
# Simple status
restreamer-manage status

# Detailed monitoring
restreamer-monitor

# Live Docker stats
docker stats restreamer

# System resources
htop
```

### Backups

**Automatic backups:** Daily at 2:00 AM

**Manual backup:**
```bash
restreamer-manage backup
```

**List backups:**
```bash
ls -lh /var/backups/restreamer/
```

**Restore from backup:**
```bash
restreamer-manage restore /var/backups/restreamer/restreamer_backup_YYYYMMDD_HHMMSS.tar.gz
```

## Next Steps

1. **Configure streaming destinations** in the Web UI
2. **Set up orientation routing** for vertical/horizontal streams
3. **Test your setup** with a short stream
4. **Monitor performance** during your first few streams
5. **Set up monitoring alerts** (optional)

## Getting Help

- **Full documentation**: See README.md
- **OBS Plugin issues**: https://github.com/rainmanjam/obs-polyemesis/issues
- **Restreamer docs**: https://docs.datarhei.com/restreamer/

## Uninstall

To completely remove Restreamer:

```bash
curl -fsSL https://raw.githubusercontent.com/rainmanjam/obs-polyemesis/main/uninstall.sh | sudo bash
```

---

**Happy Streaming! 🎥**
