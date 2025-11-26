// Modal Management

// Close modals when clicking outside
document.querySelectorAll('.modal').forEach(modal => {
    modal.addEventListener('click', (e) => {
        if (e.target === modal) {
            modal.classList.remove('visible');
        }
    });
});

// Close buttons
document.querySelectorAll('.modal-close, [data-modal]').forEach(btn => {
    btn.addEventListener('click', (e) => {
        const modalId = btn.getAttribute('data-modal');
        if (modalId) {
            document.getElementById(modalId).classList.remove('visible');
        } else {
            btn.closest('.modal').classList.remove('visible');
        }
    });
});

// Escape key closes modals
document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        document.querySelectorAll('.modal.visible').forEach(modal => {
            modal.classList.remove('visible');
        });
    }
});

// Connection settings modal
document.getElementById('connectionSettingsBtn').addEventListener('click', () => {
    document.getElementById('connectionSettingsModal').classList.add('visible');
});

document.getElementById('settingsBtn').addEventListener('click', () => {
    document.getElementById('connectionSettingsModal').classList.add('visible');
});

document.getElementById('saveConnectionBtn').addEventListener('click', () => {
    const host = document.getElementById('hostInput').value;
    const port = document.getElementById('portInput').value;
    const https = document.getElementById('httpsCheck').checked;

    // Update connection display
    const protocol = https ? 'https' : 'http';
    document.getElementById('connectionText').textContent = `${host}:${port}`;
    document.getElementById('connectionIndicator').className = 'status-indicator active';

    // Close modal
    document.getElementById('connectionSettingsModal').classList.remove('visible');

    alert('Connection settings saved and tested successfully!');
});

// Monitoring modal
document.getElementById('monitoringBtn').addEventListener('click', () => {
    document.getElementById('monitoringModal').classList.add('visible');
    updateMonitoringData();
});

// Advanced modal
document.getElementById('advancedBtn').addEventListener('click', () => {
    document.getElementById('advancedModal').classList.add('visible');
});

// Server settings modal
document.getElementById('serverSettingsBtn').addEventListener('click', () => {
    alert('Server Settings: View/edit Restreamer server configuration, manage processes, and system settings.');
});

// Help button
document.getElementById('helpBtn').addEventListener('click', () => {
    alert('OBS Polyemesis Help\n\nRight-click on profiles, destinations, or connection status for more options.\n\nKeyboard shortcuts:\n- Esc: Close menus/modals\n- Enter: Expand/collapse selected profile\n\nFor more help, visit the documentation.');
});

// Profile edit modal
function openProfileEditModal(profile) {
    const modal = document.getElementById('profileEditModal');
    const title = document.getElementById('profileEditTitle');
    const nameInput = document.getElementById('profileNameInput');
    const destList = document.getElementById('destinationsEditList');

    // Set modal title and profile name
    title.textContent = profile ? 'Edit Profile' : 'New Profile';
    nameInput.value = profile ? profile.name : '';

    // Populate destinations
    destList.innerHTML = '';
    if (profile && profile.destinations) {
        profile.destinations.forEach(dest => {
            const destItem = document.createElement('div');
            destItem.className = 'destination-edit-item';
            destItem.innerHTML = `
                <div class="destination-edit-info">
                    <div class="destination-edit-name">${dest.service}</div>
                    <div class="destination-edit-details">
                        ${dest.resolution} @ ${formatBitrate(dest.bitrate)}
                    </div>
                </div>
                <div class="destination-edit-actions">
                    <button class="btn btn-secondary btn-sm" onclick="editDestination('${dest.id}')">
                        Edit
                    </button>
                    <button class="btn btn-danger btn-sm" onclick="removeDestination('${profile.id}', '${dest.id}')">
                        Remove
                    </button>
                </div>
            `;
            destList.appendChild(destItem);
        });
    }

    modal.classList.add('visible');
}

// New profile button
document.getElementById('newProfileBtn').addEventListener('click', () => {
    openProfileEditModal(null);
});

// Add destination button in edit modal
document.getElementById('addDestinationBtn').addEventListener('click', () => {
    document.getElementById('destinationEditModal').classList.add('visible');
});

// Save profile button
document.getElementById('saveProfileBtn').addEventListener('click', () => {
    const name = document.getElementById('profileNameInput').value;
    if (!name) {
        alert('Please enter a profile name');
        return;
    }

    // Create new profile or update existing
    const newProfile = {
        id: 'profile-' + Date.now(),
        name: name,
        status: 'inactive',
        destinations: []
    };

    mockProfiles.push(newProfile);
    renderProfiles();

    document.getElementById('profileEditModal').classList.remove('visible');
});

// Save destination button
document.getElementById('saveDestinationBtn').addEventListener('click', () => {
    const service = document.getElementById('serviceSelect').value;
    const streamKey = document.getElementById('streamKeyInput').value;
    const resolution = document.getElementById('resolutionInput').value || '1920x1080';
    const bitrate = parseInt(document.getElementById('bitrateInput').value) || 6000;
    const fps = parseInt(document.getElementById('fpsInput').value) || 60;

    if (!streamKey) {
        alert('Please enter a stream key');
        return;
    }

    alert(`Destination added:\n\nService: ${service}\nResolution: ${resolution}\nBitrate: ${bitrate} kbps\nFPS: ${fps}`);

    document.getElementById('destinationEditModal').classList.remove('visible');
});

// Toggle stream key visibility
document.getElementById('toggleStreamKey').addEventListener('click', function() {
    const input = document.getElementById('streamKeyInput');
    if (input.type === 'password') {
        input.type = 'text';
        this.textContent = 'Hide';
    } else {
        input.type = 'password';
        this.textContent = 'Show';
    }
});

// Helper functions for destination editing
function editDestination(destId) {
    alert('Edit destination: ' + destId);
}

function removeDestination(profileId, destId) {
    if (confirm('Remove this destination?')) {
        const profile = mockProfiles.find(p => p.id === profileId);
        if (profile) {
            const index = profile.destinations.findIndex(d => d.id === destId);
            if (index > -1) {
                profile.destinations.splice(index, 1);
                renderProfiles();
                openProfileEditModal(profile); // Refresh the edit modal
            }
        }
    }
}
