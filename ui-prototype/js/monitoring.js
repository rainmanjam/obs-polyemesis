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

        // Use DOM methods to prevent XSS
        const tdId = document.createElement('td');
        tdId.textContent = proc.reference || proc.id;

        const tdStatus = document.createElement('td');
        const statusSpan = document.createElement('span');
        statusSpan.className = 'table-status';
        const statusDot = document.createElement('span');
        statusDot.className = `table-status-dot ${proc.state}`;
        statusSpan.appendChild(statusDot);
        statusSpan.appendChild(document.createTextNode(' ' + proc.state));
        tdStatus.appendChild(statusSpan);

        const tdUptime = document.createElement('td');
        tdUptime.textContent = formatDuration(proc.uptime);

        const tdCpu = document.createElement('td');
        tdCpu.textContent = proc.cpu.toFixed(1) + '%';

        const tdMemory = document.createElement('td');
        tdMemory.textContent = formatBytes(proc.memory * 1024 * 1024);

        const tdActions = document.createElement('td');
        tdActions.className = 'table-actions';
        const stopBtn = document.createElement('button');
        stopBtn.className = 'table-btn';
        stopBtn.textContent = 'Stop';
        const restartBtn = document.createElement('button');
        restartBtn.className = 'table-btn';
        restartBtn.textContent = 'Restart';
        tdActions.appendChild(stopBtn);
        tdActions.appendChild(restartBtn);

        row.appendChild(tdId);
        row.appendChild(tdStatus);
        row.appendChild(tdUptime);
        row.appendChild(tdCpu);
        row.appendChild(tdMemory);
        row.appendChild(tdActions);

        tbody.appendChild(row);
    });
}

function updateSessionsTable() {
    const tbody = document.getElementById('sessionsTableBody');
    tbody.innerHTML = '';

    mockSessions.forEach(sess => {
        const row = document.createElement('tr');

        // Use DOM methods to prevent XSS
        const tdId = document.createElement('td');
        tdId.textContent = sess.id;

        const tdAddr = document.createElement('td');
        tdAddr.textContent = sess.remoteAddr;

        const tdBytes = document.createElement('td');
        tdBytes.textContent = formatBytes(sess.bytesSent);

        const tdDuration = document.createElement('td');
        tdDuration.textContent = formatDuration(sess.duration);

        row.appendChild(tdId);
        row.appendChild(tdAddr);
        row.appendChild(tdBytes);
        row.appendChild(tdDuration);

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
