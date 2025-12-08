// ------------------------------
// Global State
// ------------------------------
let lastTimestamp = 0;                 // Most recent timestamp received
let valueHistory = [];                // Stores [timestamp, valuesObject]
let countdownDuration = null;         // Duration in seconds
let countdownStartTime = null;        // Server's countdown start timestamp
let gridLabels = null;                // 5×5 labels array
let gridValues = null;                // 5×5 values array

// ------------------------------
// Helper: Build grid dynamically
// ------------------------------
function buildGrid() {
    const grid = document.getElementById("grid");
    grid.innerHTML = ""; // Clear old cells

    if (!gridLabels || !gridValues) return;

    for (let i = 0; i < 5; i++) {
        for (let j = 0; j < 5; j++) {
            const cell = document.createElement("div");
            cell.className = "cell";
            cell.id = `cell-${i}-${j}`;

            const label = gridLabels[i][j];
            const val = gridValues[i][j];

            cell.textContent = `${label}: ${val}`;
            cell.style.backgroundColor = getColor(val);

            grid.appendChild(cell);
        }
    }
}


// ------------------------------
// Helper: Convert 0→1 value into red→orange→green
// ------------------------------
function getColor(v) {
    v = Math.max(0, Math.min(1, v));

    // 0→0.5 = red → orange
    // 0.5→1 = orange → green
    let r, g, b = 0;

    if (v < 0.5) {
        r = 255;
        g = Math.floor(510 * v); // 0→255
    } else {
        r = Math.floor(255 - 510 * (v - 0.5)); // 255→0
        g = 255;
    }

    return `rgb(${r},${g},0)`;
}

// ------------------------------
// Update existing grid cells
// ------------------------------
function updateGridValues() {
    if (!gridLabels || !gridValues) return;

    for (let i = 0; i < 5; i++) {
        for (let j = 0; j < 5; j++) {
            const cell = document.getElementById(`cell-${i}-${j}`);
            if (!cell) continue;

            const label = gridLabels[i][j];
            const val = gridValues[i][j];

            cell.textContent = `${label}: ${val}`;
            cell.style.backgroundColor = getColor(val);
        }
    }
}

// ------------------------------
// Countdown update
// ------------------------------
function updateCountdown() {
    const cd = document.getElementById("countdown");

    if (countdownDuration === null || countdownStartTime === null) {
        cd.textContent = "Launch Countdown Not Initialized";
        return;
    }

    // Calculate elapsed time since server started countdown
    const now = Date.now() / 1000; // Convert to seconds
    const elapsed = now - countdownStartTime;
    const remaining = Math.max(0, countdownDuration - elapsed);

    if (remaining > 0) {
        cd.textContent = `LAUNCH IN T- ${remaining.toFixed(1)}s`;
    } else {
        cd.textContent = "LAUNCH!";
    }
}

// ------------------------------
// Poll server every second
// ------------------------------
async function poll() {
    const queryTime = (lastTimestamp + 0.001).toFixed(3);
    console.log(`Polling for timestamps > ${queryTime}`);

    try {
        const response = await fetch(`/data.json/${queryTime}`);
        if (!response.ok) return;

        const json = await response.json();
        console.log('Received data:', json);

        // ---------- Parse GO grid ----------
        if (json.goGridLabels) {
            gridLabels = JSON.parse(json.goGridLabels);
        }
        if (json.goGridValues) {
            gridValues = JSON.parse(json.goGridValues);
        }

        // Rebuild grid only if first time
        if (!document.getElementById("cell-0-0")) {
            buildGrid();
        } else {
            updateGridValues();
        }

        // ---------- Parse valuesUpdate ----------
        // Server sends array like [[timestamp, jsonData], ...]
        if (json.valuesUpdate) {
            const updates = typeof json.valuesUpdate === 'string' 
                ? JSON.parse(json.valuesUpdate) 
                : json.valuesUpdate;
            
            if (Array.isArray(updates)) {
                // Get all timestamps and find the maximum
                const timestamps = updates.map(entry => entry[0]);
                
                for (const [timestamp, values] of updates) {
                    valueHistory.push([timestamp, values]);
                }
                
                // Update lastTimestamp to the highest timestamp received
                if (timestamps.length > 0) {
                    const newMax = Math.max(...timestamps);
                    console.log(`Updated lastTimestamp from ${lastTimestamp} to ${newMax}`);
                    lastTimestamp = Math.max(lastTimestamp, newMax);
                }
            }
        }

        // ---------- Parse countdown ----------
        // Check for both typo variants
        const countdownField = json.countdownTime || json.coundownTime;
        const startTimeField = json.countdownStartTime || json.coundownStartTime;
        
        if (countdownField !== undefined && countdownField !== null) {
            const parsedDuration = typeof countdownField === 'string' 
                ? parseFloat(countdownField) 
                : countdownField;
            if (!isNaN(parsedDuration)) {
                countdownDuration = parsedDuration;
            }
        }
        
        if (startTimeField !== undefined && startTimeField !== null) {
            const parsedStartTime = typeof startTimeField === 'string' 
                ? parseFloat(startTimeField) 
                : startTimeField;
            if (!isNaN(parsedStartTime)) {
                countdownStartTime = parsedStartTime;
            }
        }

    } catch (e) {
        console.error("Error polling:", e);
    }
}

// ------------------------------
// Start update loops
// ------------------------------
setInterval(poll, 1000);         // server polling
setInterval(updateCountdown, 100); // smoother countdown display