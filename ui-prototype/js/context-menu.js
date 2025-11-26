// Context Menu Management

class ContextMenu {
    constructor() {
        this.menu = document.getElementById('contextMenu');
        this.currentTarget = null;
        this.currentType = null;

        // Hide menu when clicking outside
        document.addEventListener('click', (e) => {
            if (!this.menu.contains(e.target)) {
                this.hide();
            }
        });

        // Hide menu on escape key
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                this.hide();
            }
        });
    }

    show(x, y, items, target, type) {
        this.currentTarget = target;
        this.currentType = type;

        // Clear existing items
        this.menu.innerHTML = '';

        // Add items
        items.forEach(item => {
            if (item.type === 'separator') {
                const separator = document.createElement('div');
                separator.className = 'context-menu-separator';
                this.menu.appendChild(separator);
            } else if (item.type === 'label') {
                const label = document.createElement('div');
                label.className = 'context-menu-label';
                label.textContent = item.text;
                this.menu.appendChild(label);
            } else {
                const menuItem = document.createElement('div');
                menuItem.className = 'context-menu-item';
                if (item.disabled) menuItem.classList.add('disabled');
                if (item.danger) menuItem.classList.add('danger');

                // Use DOM methods to prevent XSS
                const iconSpan = document.createElement('span');
                iconSpan.className = 'icon';
                iconSpan.textContent = item.icon || '';

                const textSpan = document.createElement('span');
                textSpan.textContent = item.text;

                menuItem.appendChild(iconSpan);
                menuItem.appendChild(textSpan);

                if (!item.disabled && item.action) {
                    menuItem.addEventListener('click', (e) => {
                        e.stopPropagation();
                        item.action(this.currentTarget);
                        this.hide();
                    });
                }

                this.menu.appendChild(menuItem);
            }
        });

        // Position menu
        this.menu.style.left = x + 'px';
        this.menu.style.top = y + 'px';

        // Show menu
        this.menu.classList.add('visible');

        // Adjust position if menu goes off screen
        setTimeout(() => {
            const rect = this.menu.getBoundingClientRect();
            if (rect.right > window.innerWidth) {
                this.menu.style.left = (window.innerWidth - rect.width - 10) + 'px';
            }
            if (rect.bottom > window.innerHeight) {
                this.menu.style.top = (window.innerHeight - rect.height - 10) + 'px';
            }
        }, 0);
    }

    hide() {
        this.menu.classList.remove('visible');
        this.currentTarget = null;
        this.currentType = null;
    }
}

// Create global context menu instance
const contextMenu = new ContextMenu();

// Context menu items for different targets
const contextMenuItems = {
    profile: (profile) => [
        {
            icon: '▶',
            text: 'Start Profile',
            action: (target) => {
                // Start profile action
                profile.status = 'starting';
                setTimeout(() => {
                    profile.status = 'active';
                    profile.destinations.forEach(d => d.status = 'active');
                    renderProfiles();
                }, 1500);
                renderProfiles();
            },
            disabled: profile.status === 'active' || profile.status === 'starting'
        },
        {
            icon: '■',
            text: 'Stop Profile',
            action: (target) => {
                // Stop profile action
                profile.status = 'inactive';
                profile.destinations.forEach(d => {
                    d.status = 'inactive';
                    d.currentBitrate = 0;
                    d.duration = 0;
                });
                renderProfiles();
            },
            disabled: profile.status === 'inactive'
        },
        {
            icon: '↻',
            text: 'Restart Profile',
            action: (target) => {
                // Restart profile action
                profile.status = 'starting';
                setTimeout(() => {
                    profile.status = 'active';
                    renderProfiles();
                }, 1500);
                renderProfiles();
            },
            disabled: profile.status === 'inactive'
        },
        { type: 'separator' },
        {
            icon: '✎',
            text: 'Edit Profile...',
            action: (target) => openProfileEditModal(profile)
        },
        {
            icon: '📋',
            text: 'Duplicate Profile',
            action: (target) => {
                const newProfile = JSON.parse(JSON.stringify(profile));
                newProfile.id = 'profile-' + Date.now();
                newProfile.name += ' (Copy)';
                newProfile.status = 'inactive';
                mockProfiles.push(newProfile);
                renderProfiles();
            }
        },
        {
            icon: '🗑️',
            text: 'Delete Profile',
            danger: true,
            action: (target) => {
                if (confirm(`Delete profile "${profile.name}"?`)) {
                    const index = mockProfiles.findIndex(p => p.id === profile.id);
                    if (index > -1) {
                        mockProfiles.splice(index, 1);
                        renderProfiles();
                    }
                }
            }
        },
        { type: 'separator' },
        {
            icon: '📊',
            text: 'View Statistics',
            action: (target) => {
                alert('Statistics feature coming soon!');
            }
        },
        {
            icon: '📝',
            text: 'Export Configuration',
            action: (target) => {
                const config = JSON.stringify(profile, null, 2);
                // Configuration exported (console.log removed for security)
                alert('Configuration exported');
            }
        },
        { type: 'separator' },
        {
            icon: '⚙️',
            text: 'Profile Settings...',
            action: (target) => openProfileEditModal(profile)
        }
    ],

    destination: (dest, profile) => [
        {
            icon: '▶',
            text: 'Start Stream',
            action: (target) => {
                // Start stream action
                dest.status = 'starting';
                setTimeout(() => {
                    dest.status = 'active';
                    dest.currentBitrate = dest.bitrate * 0.95;
                    renderProfiles();
                }, 1000);
                renderProfiles();
            },
            disabled: dest.status === 'active' || dest.status === 'starting'
        },
        {
            icon: '■',
            text: 'Stop Stream',
            action: (target) => {
                // Stop stream action
                dest.status = 'inactive';
                dest.currentBitrate = 0;
                dest.duration = 0;
                renderProfiles();
            },
            disabled: dest.status === 'inactive'
        },
        {
            icon: '↻',
            text: 'Restart Stream',
            action: (target) => {
                // Restart stream action
                dest.status = 'starting';
                setTimeout(() => {
                    dest.status = 'active';
                    renderProfiles();
                }, 1000);
                renderProfiles();
            },
            disabled: dest.status === 'inactive'
        },
        {
            icon: '⏸',
            text: 'Pause Stream',
            action: (target) => {
                // Pause stream action
                dest.status = 'paused';
                renderProfiles();
            },
            disabled: dest.status !== 'active'
        },
        { type: 'separator' },
        {
            icon: '✎',
            text: 'Edit Destination...',
            action: (target) => {
                alert('Edit destination feature coming soon!');
            }
        },
        {
            icon: '📋',
            text: 'Copy Stream URL',
            action: (target) => {
                const url = `rtmp://live.${dest.service.toLowerCase()}.tv/live/stream_key`;
                navigator.clipboard.writeText(url);
                alert('Stream URL copied to clipboard!');
            }
        },
        {
            icon: '📋',
            text: 'Copy Stream Key',
            action: (target) => {
                navigator.clipboard.writeText('****_STREAM_KEY_****');
                alert('Stream key copied to clipboard!');
            }
        },
        { type: 'separator' },
        {
            icon: '📊',
            text: 'View Stream Stats',
            action: (target) => {
                const stats = `
Service: ${dest.service}
Status: ${getStatusText(dest.status)}
Resolution: ${dest.resolution}
Bitrate: ${formatBitrate(dest.currentBitrate)} / ${formatBitrate(dest.bitrate)}
FPS: ${dest.fps}
Dropped: ${dest.droppedFrames} (${calculateDroppedPercent(dest.droppedFrames, dest.totalFrames)}%)
Duration: ${formatDuration(dest.duration)}
                `.trim();
                alert(stats);
            }
        },
        {
            icon: '📝',
            text: 'View Stream Logs',
            action: (target) => {
                alert('Stream logs feature coming soon!');
            }
        },
        {
            icon: '🔍',
            text: 'Test Stream Health',
            action: (target) => {
                alert('Testing stream health...\n\n✓ Connection: OK\n✓ Bitrate: Stable\n✓ Server: Responding');
            }
        },
        { type: 'separator' },
        {
            icon: '⚠️',
            text: dest.status === 'inactive' ? 'Enable Destination' : 'Disable Destination',
            action: (target) => {
                alert('Toggle destination feature coming soon!');
            }
        },
        {
            icon: '🗑️',
            text: 'Remove Destination',
            danger: true,
            action: (target) => {
                if (confirm(`Remove ${dest.service} from this profile?`)) {
                    const index = profile.destinations.findIndex(d => d.id === dest.id);
                    if (index > -1) {
                        profile.destinations.splice(index, 1);
                        renderProfiles();
                    }
                }
            }
        }
    ],

    connection: () => [
        {
            icon: '🔄',
            text: 'Test Connection',
            action: () => {
                const indicator = document.getElementById('connectionIndicator');
                const text = document.getElementById('connectionText');

                indicator.className = 'status-indicator starting';
                text.textContent = 'Testing...';

                setTimeout(() => {
                    indicator.className = 'status-indicator active';
                    text.textContent = 'restreamer.example.com:8080';
                    alert('Connection test successful!');
                }, 1500);
            }
        },
        {
            icon: '🔌',
            text: 'Reconnect',
            action: () => {
                alert('Reconnecting to Restreamer...');
            }
        },
        {
            icon: '⏸',
            text: 'Disconnect',
            action: () => {
                const indicator = document.getElementById('connectionIndicator');
                const text = document.getElementById('connectionText');
                indicator.className = 'status-indicator inactive';
                text.textContent = 'Disconnected';
            }
        },
        { type: 'separator' },
        {
            icon: '✎',
            text: 'Edit Connection...',
            action: () => {
                document.getElementById('connectionSettingsModal').classList.add('visible');
            }
        },
        {
            icon: '📋',
            text: 'Copy Server URL',
            action: () => {
                navigator.clipboard.writeText('http://restreamer.example.com:8080');
                alert('Server URL copied to clipboard!');
            }
        },
        { type: 'separator' },
        {
            icon: '📊',
            text: 'View Server Stats',
            action: () => {
                document.getElementById('monitoringModal').classList.add('visible');
            }
        },
        {
            icon: '📝',
            text: 'View Server Logs',
            action: () => {
                alert('Server logs feature coming soon!');
            }
        },
        {
            icon: '🔍',
            text: 'Probe Server',
            action: () => {
                alert('Probing server...\n\nServer: Restreamer v16.16.0\nAPI: v3\nUptime: 5 days, 3 hours\nLoad: 24% CPU, 1.2GB RAM');
            }
        },
        { type: 'separator' },
        {
            icon: '⚙️',
            text: 'Server Settings...',
            action: () => {
                document.getElementById('connectionSettingsModal').classList.add('visible');
            }
        }
    ]
};
