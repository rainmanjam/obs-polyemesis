// Monitoring Data Updates

function updateMonitoringData() {
    updateMetrics();
    updateProcessesTable();
    updateSessionsTable();
}

function updateMetrics() {
    // Simulate real-time metrics
    const cpu = 20 + Math.random() * 20;
    const memory = 1024 + Math.random() * 512;
    const totalBitrate = mockProfiles
        .flatMap(p => p.destinations)
        .filter(d => d.status === 'active')
        .reduce((sum, d) => sum + d.currentBitrate, 0);
    const totalDropped = mockProfiles
        .flatMap(p => p.destinations)
        .filter(d => d.status === 'active')
        .reduce((sum, d) => sum + d.droppedFrames, 0);
    const totalFrames = mockProfiles
        .flatMap(p => p.destinations)
        .filter(d => d.status === 'active')
        .reduce((sum, d) => sum + d.totalFrames, 0);

    // Update CPU
    document.getElementById('cpuValue').textContent = cpu.toFixed(1) + '%';
    document.getElementById('cpuFill').style.width = cpu + '%';

    // Update Memory
    document.getElementById('memoryValue').textContent = formatBytes(memory * 1024 * 1024);
    const memoryPercent = (memory / 2048) * 100;
    document.getElementById('memoryFill').style.width = memoryPercent + '%';

    // Update Bitrate
    document.getElementById('bitrateValue').textContent = formatBitrate(totalBitrate);
    const bitratePercent = Math.min((totalBitrate / 30000) * 100, 100);
    document.getElementById('bitrateFill').style.width = bitratePercent + '%';

    // Update Dropped Frames
    const droppedPercent = totalFrames > 0 ? (totalDropped / totalFrames) * 100 : 0;
    document.getElementById('droppedValue').textContent = `${totalDropped} (${droppedPercent.toFixed(2)}%)`;
    document.getElementById('droppedFill').style.width = Math.min(droppedPercent * 50, 100) + '%';
}

function updateProcessesTable() {
    const tbody = document.getElementById('processesTableBody');
    tbody.innerHTML = '';

    mockProcesses.forEach(proc => {
        const row = document.createElement('tr');
        row.innerHTML = `
            <td>${proc.reference || proc.id}</td>
            <td>
                <span class="table-status">
                    <span class="table-status-dot ${proc.state}"></span>
                    ${proc.state}
                </span>
            </td>
            <td>${formatDuration(proc.uptime)}</td>
            <td>${proc.cpu.toFixed(1)}%</td>
            <td>${formatBytes(proc.memory * 1024 * 1024)}</td>
            <td class="table-actions">
                <button class="table-btn">Stop</button>
                <button class="table-btn">Restart</button>
            </td>
        `;
        tbody.appendChild(row);
    });
}

function updateSessionsTable() {
    const tbody = document.getElementById('sessionsTableBody');
    tbody.innerHTML = '';

    mockSessions.forEach(sess => {
        const row = document.createElement('tr');
        row.innerHTML = `
            <td>${sess.id}</td>
            <td>${sess.remoteAddr}</td>
            <td>${formatBytes(sess.bytesSent)}</td>
            <td>${formatDuration(sess.duration)}</td>
        `;
        tbody.appendChild(row);
    });
}

// Update metrics periodically while monitoring modal is open
setInterval(() => {
    const monitoringModal = document.getElementById('monitoringModal');
    if (monitoringModal.classList.contains('visible')) {
        updateMetrics();

        // Also update duration counters for active streams
        mockProfiles.forEach(profile => {
            profile.destinations.forEach(dest => {
                if (dest.status === 'active') {
                    dest.duration++;
                    dest.totalFrames += dest.fps;

                    // Randomly drop some frames
                    if (Math.random() < 0.001) {
                        dest.droppedFrames++;
                    }
                }
            });
        });

        updateProcessesTable();
        updateSessionsTable();
    }
}, 1000);
