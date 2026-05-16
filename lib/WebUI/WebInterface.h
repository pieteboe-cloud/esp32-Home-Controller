#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="theme-color" content="#1a1a2e">
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; margin: 0; padding: 0; background: linear-gradient(135deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%); color: #eee; min-height: 100vh; }
        .section { padding: 40px 20px; display: flex; flex-direction: column; align-items: center; justify-content: flex-start; min-height: 100vh; }
        .btn-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; width: 100%; max-width: 600px; margin: 20px 0; }
        button { padding: 18px 20px; font-size: 16px; cursor: pointer; border: none; border-radius: 8px; background: linear-gradient(45deg, #4ecca3, #5edbb3); color: #1a1a2e; transition: all 0.3s ease; box-shadow: 0 4px 15px rgba(78, 204, 163, 0.3); font-weight: bold; text-transform: uppercase; letter-spacing: 0.5px; }
        button:active { transform: translateY(2px); box-shadow: 0 2px 10px rgba(78, 204, 163, 0.3); }
        button:hover { background: linear-gradient(45deg, #5edbb3, #6eebc3); box-shadow: 0 6px 20px rgba(78, 204, 163, 0.4); }
        button.verbose-active { background: linear-gradient(45deg, #ff6b6b, #ff8b8b); color: #fff; box-shadow: 0 4px 15px rgba(255, 107, 107, 0.3); }
        .effect-btn.active { background: linear-gradient(45deg, #ff6b6b, #ff8b8b); color: #fff; box-shadow: 0 6px 25px rgba(255, 107, 107, 0.5); }
        #debug-container { background: rgba(22, 33, 62, 0.9); color: #eee; padding: 15px; margin: 20px auto; min-height: 300px; max-height: 500px; overflow-y: auto; text-align: left; font-family: 'Courier New', monospace; font-size: 13px; border-radius: 10px; border: 2px solid #4ecca3; box-shadow: 0 8px 25px rgba(0,0,0,0.5); width: 90%; max-width: 800px; }
        .status-header { display: flex; align-items: center; gap: 15px; margin-bottom: 30px; }
        .led { width: 15px; height: 15px; border-radius: 50%; background: #444; box-shadow: 0 0 5px #000; transition: background 0.3s ease; }
        .led.active { background: #00ff00; box-shadow: 0 0 15px #00ff00; }
        .log-info { color: #ffeb3b; font-weight: bold; }
        .log-warn { color: #ff9800; font-weight: bold; }
        .log-error { color: #f44336; font-weight: bold; }
        .log-packet { color: #bb86fc; }
        .log-test { color: #f4d03f; }
        .log-debug { color: #03dac6; }
        .remote-container { background: rgba(22, 33, 62, 0.9); padding: 20px; border-radius: 15px; border: 2px solid #0f3460; max-width: 320px; margin: 20px auto; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
        .remote-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 15px; }
        .remote-btn { width: 100%; aspect-ratio: 1; border-radius: 50%; font-size: 11px; cursor: pointer; border: none; transition: all 0.2s ease; font-weight: bold; color: white; text-shadow: 1px 1px 2px rgba(0,0,0,0.5); display: flex; align-items: center; justify-content: center; box-shadow: 0 4px 10px rgba(0,0,0,0.4); }
        .remote-btn:active { transform: translateY(2px); box-shadow: 0 2px 5px rgba(0,0,0,0.4); }
        .remote-zone-select { width: 100%; padding: 12px; margin-bottom: 20px; background: #1a1a2e; color: #4ecca3; border: 2px solid #4ecca3; border-radius: 8px; font-weight: bold; font-size: 14px; }
        .spacer { padding: 40px 20px; background: linear-gradient(135deg, #16213e 0%, #0f3460 100%); color: #eee; }
        .content-wrapper { display: flex; flex-direction: column; min-height: 100vh; }
        .main-content { flex: 1; }
        .log-section { background: #0f3460; padding: 40px 20px; border-top: 3px solid #1a1a2e; }
        h1, h2, h3 { color: #4ecca3; margin-bottom: 20px; text-shadow: 1px 1px 3px rgba(0,0,0,0.5); }
        p { color: #ccc; font-size: 16px; }
        label { color: #eee; font-weight: bold; }
        input[type="range"] { -webkit-appearance: none; appearance: none; width: 100%; height: 8px; border-radius: 5px; background: #ddd; outline: none; }
        input[type="range"]::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 20px; height: 20px; border-radius: 50%; background: #4ecca3; cursor: pointer; }
        input[type="range"]::-moz-range-thumb { width: 20px; height: 20px; border-radius: 50%; background: #4ecca3; cursor: pointer; border: none; }
        #color-wheel { border: 3px solid #4ecca3; }
        #color-preview { border: 3px solid #fff; }
    </style>
</head>
<body>
    <div class="content-wrapper">
        <div class="main-content">
            <div class="section">
                <div class="status-header">
                    <h1>Home Control Panel</h1>
                    <div id="running-led" class="led"></div>
                </div>
                <div class="btn-grid">
                    <button onclick="sendCmd('light_on')">Light ON</button>
                    <button onclick="sendCmd('light_off')">Light OFF</button>
                    <button onclick="sendCmd('fan_toggle')">Fan Toggle</button>
                    <button onclick="sendCmd('ac_boost')">AC Boost</button>
                </div>
            </div>

            <div class="spacer">
                <h2>Scene Effects</h2>
                <p style="color: #aaa;">Pre-configured automation scenes</p>
                
                <h3 style="margin-top: 20px; color: #ddd;">Calm & Relaxing</h3>
                <div class="btn-grid" style="margin-top: 10px; max-width: 400px;">
                    <button class="effect-btn" data-scene="warm_sunset" onclick="sceneButtonClicked('warm_sunset', this)">🌅 Sunset</button>
                    <button class="effect-btn" data-scene="night_mode" onclick="sceneButtonClicked('night_mode', this)">🌙 Night</button>
                    <button class="effect-btn" data-scene="movie_night" onclick="sceneButtonClicked('movie_night', this)">🎬 Movie</button>
                    <button class="effect-btn" data-scene="relaxation" onclick="sceneButtonClicked('relaxation', this)">☮️ Chill</button>
                </div>

                <h3 style="margin-top: 30px; color: #ddd;">Wild & Crazy</h3>
                <div class="btn-grid" style="margin-top: 10px; max-width: 400px;">
                    <button class="effect-btn" data-scene="disco_party" onclick="sceneButtonClicked('disco_party', this)">🕺 Disco</button>
                    <button class="effect-btn" data-scene="neon_madness" onclick="sceneButtonClicked('neon_madness', this)">⚡ Neon</button>
                    <button class="effect-btn" data-scene="psychedelic" onclick="sceneButtonClicked('psychedelic', this)">🌀 Psych</button>
                    <button class="effect-btn" data-scene="energy_burst" onclick="sceneButtonClicked('energy_burst', this)">💥 Energy</button>
                </div>

                <h3 style="margin-top: 30px; color: #ddd;">Extreme Modes</h3>
                <div class="btn-grid" style="margin-top: 10px; max-width: 400px;">
                    <button class="effect-btn" data-scene="volcanic_rage" onclick="sceneButtonClicked('volcanic_rage', this)">🌋 Volcano</button>
                    <button class="effect-btn" data-scene="ice_cold" onclick="sceneButtonClicked('ice_cold', this)">❄️ Freeze</button>
                    <button class="effect-btn" data-scene="purple_dream" onclick="sceneButtonClicked('purple_dream', this)">💜 Purple</button>
                    <button class="effect-btn" data-scene="rave_mode" onclick="sceneButtonClicked('rave_mode', this)">🎉 RAVE!</button>
                </div>

                <h3 style="margin-top: 30px; color: #ddd;">Specials</h3>
                <div class="btn-grid" style="margin-top: 10px; max-width: 400px;">
                    <button class="effect-btn" data-scene="rainbow_effect" onclick="sceneButtonClicked('rainbow_effect', this)">🌈 Rainbow</button>
                    <button class="effect-btn" data-scene="sunset_fade" onclick="sceneButtonClicked('sunset_fade', this)">🌆 Fade</button>
                    <button class="effect-btn" data-scene="warm_white" onclick="sceneButtonClicked('warm_white', this)">🔥 Warm</button>
                    <button class="effect-btn" data-scene="all_off" onclick="sceneButtonClicked('all_off', this)">💤 All Off</button>
                </div>
            </div>

            <div class="spacer">
                <h2>LivingColors Lamp Control</h2>
                <div style="max-width: 400px; margin: 0 auto; text-align: left;">
                    <label style="color: #eee; display: block; margin-bottom: 15px; font-weight: bold;">Select Lamps (Click to toggle):</label>
                    <div id="lamp-checkbox-grid" style="display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; margin-bottom: 20px;">
                        <!-- Checkboxes will be generated by JavaScript -->
                    </div>
                    <p id="lamp-selection-info" style="color: #4ecca3; font-size: 14px; margin-bottom: 15px; min-height: 20px;">Select at least one lamp to start</p>

                    <label style="color: #eee; display: block; margin-bottom: 10px;">Color Wheel:</label>
                    <div style="position: relative; width: 200px; height: 200px; margin: 0 auto;">
                        <canvas id="color-wheel" width="200" height="200" style="border-radius: 50%; cursor: pointer; box-shadow: 0 0 10px rgba(0,0,0,0.5);"></canvas>
                        <div id="color-preview" style="position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); width: 40px; height: 40px; border-radius: 50%; background: white; border: 2px solid #fff; box-shadow: 0 0 5px rgba(0,0,0,0.3);"></div>
                    </div>

                    <div class="btn-grid" style="margin-top: 20px;">
                        <button onclick="turnOnSelectedLamps()">Turn ON</button>
                        <button onclick="turnOffSelectedLamps()">Turn OFF</button>
                        <button onclick="setWhiteSelectedLamps()">White</button>
                        <button onclick="setColorSelectedLamps()">Set Color</button>
                    </div>

                    <h3 style="margin-top: 30px; color: #eee;">Master Brightness</h3>
                    <input type="range" id="master-dimmer" min="1" max="255" value="255" style="width: 100%; margin-bottom: 10px;" oninput="updateMasterDimmer()">
                    <span id="dimmer-value" style="color: #4ecca3;">255</span>

                    <h3 style="margin-top: 30px; color: #eee;">Presets</h3>
                    <div class="btn-grid" style="margin-top: 10px;">
                        <button onclick="savePreset(0)">Save P1</button>
                        <button onclick="loadPreset(0)">Load P1</button>
                        <button onclick="savePreset(1)">Save P2</button>
                        <button onclick="loadPreset(1)">Load P2</button>
                    </div>
                </div>
            </div>

            <div class="spacer" style="height: auto; padding-bottom: 60px;">
                <h2>RGB Strip Remote</h2>
                <div class="remote-container">
                    <select id="remote-zone" class="remote-zone-select">
                        <option value="bottom">BOTTOM ZONE</option>
                        <option value="mid">MIDDLE ZONE</option>
                        <option value="upper">UPPER ZONE</option>
                    </select>
                    <div class="remote-grid">
                        <!-- Row 1: Brightness & Power -->
                        <button class="remote-btn" style="background:#555;" onclick="sendRemote(0xEF00FF00)">B+</button>
                        <button class="remote-btn" style="background:#555;" onclick="sendRemote(0xEF00FE01)">B-</button>
                        <button class="remote-btn" style="background:#b33;" onclick="sendRemote(0xEF00FD02)">OFF</button>
                        <button class="remote-btn" style="background:#3b3;" onclick="sendRemote(0xEF00FC03)">ON</button>
                        <!-- Row 2: Basic Colors -->
                        <button class="remote-btn" style="background:#f00;" onclick="sendRemote(0xEF00FB04)">R</button>
                        <button class="remote-btn" style="background:#0f0;" onclick="sendRemote(0xEF00FA05)">G</button>
                        <button class="remote-btn" style="background:#00f;" onclick="sendRemote(0xEF00F906)">B</button>
                        <button class="remote-btn" style="background:#eee; color:#333;" onclick="sendRemote(0xEF00F807)">W</button>
                        <!-- Row 3: Colors + Flash -->
                        <button class="remote-btn" style="background:#ff8c00;" onclick="sendRemote(0xEF00F708)">OR</button>
                        <button class="remote-btn" style="background:#90ee90; color:#333;" onclick="sendRemote(0xEF00F609)">LG</button>
                        <button class="remote-btn" style="background:#00008b;" onclick="sendRemote(0xEF00F50A)">DB</button>
                        <button class="remote-btn" style="background:#444;" onclick="sendRemote(0xEF00EB14)">FLSH</button>
                        <!-- Row 4: Colors + Strobe -->
                        <button class="remote-btn" style="background:#ffcc00;" onclick="sendRemote(0xEF00F30C)">YO</button>
                        <button class="remote-btn" style="background:#00ffff; color:#333;" onclick="sendRemote(0xEF00F20D)">CY</button>
                        <button class="remote-btn" style="background:#800080;" onclick="sendRemote(0xEF00F10E)">PU</button>
                        <button class="remote-btn" style="background:#444;" onclick="sendRemote(0xEF00EA15)">STRB</button>
                        <!-- Row 5: Colors + Fade -->
                        <button class="remote-btn" style="background:#ffff00; color:#333;" onclick="sendRemote(0xEF00F00F)">YLW</button>
                        <button class="remote-btn" style="background:#444;" onclick="sendRemote(0xEF00E916)">FADE</button>
                        <button class="remote-btn" style="background:#444;" onclick="sendRemote(0xEF00E817)">SMTH</button>
                    </div>
                </div>
            </div>

            <div class="spacer">
                <h2>System Information</h2>
                <p style="color: #aaa;">Hardware status, uptime, etc.</p>
                <div class="btn-grid" style="margin-top: 20px;">
                    <button onclick="rebootController()">Reboot</button>
                    <button onclick="openSettings()">Open Settings</button>
                </div>
            </div>
        </div>

        <div class="log-section">
            <h2>Debug Logs</h2>
            <div class="btn-grid" style="margin-top: 10px;">
                <button id="verbose-btn" onclick="toggleVerbose()">Verbose: OFF</button>
                <button onclick="clearLogs()">Clear Logs</button>
            </div>
            <div id="debug-container"></div>
        </div>
    </div>

    <script>
        let activeScene = '';

        function sendCmd(cmd) {
            fetch('/command?val=' + cmd);
        }

        function sceneButtonClicked(sceneName, button) {
            if (activeScene === sceneName) {
                sendCmd('scene_off');
                activeScene = '';
            } else {
                sendCmd('scene:' + sceneName);
                activeScene = sceneName;
            }
            updateSceneButtons();
        }

        function updateSceneButtons() {
            document.querySelectorAll('.effect-btn').forEach(btn => {
                btn.classList.toggle('active', btn.dataset.scene === activeScene);
            });
        }

        function rebootController() {
            if (confirm('Are you sure you want to reboot the controller?')) {
                fetch('/reboot').then(response => {
                    if (response.ok) {
                        alert('Controller is rebooting...');
                    }
                });
            }
        }

        function openSettings() {
            fetch('/settings').then(r => r.text()).then(settings => {
                alert('Current Settings:\n\n' + settings);
            }).catch(error => {
                alert('Failed to load settings: ' + error);
            });
        }

        function updateLogs() {
            fetch('/logs').then(r => r.text()).then(text => {
                const container = document.getElementById('debug-container');
                const lines = text.trim().split('\n');
                container.innerHTML = '';
                lines.forEach(line => {
                    let cls = '';
                    let displayText = line;
                    const upperLine = line.toUpperCase();
                    if (upperLine.includes('[ERROR]')) {
                        cls = 'log-error';
                    } else if (upperLine.includes('[WARN]')) {
                        cls = 'log-warn';
                    } else if (upperLine.includes('[INFO]')) {
                        cls = 'log-info';
                        displayText = line.replace(/\[INFO\]/i, '').trim();
                    } else if (upperLine.includes('[PACKET]')) {
                        cls = 'log-packet';
                    } else if (upperLine.includes('[TEST]')) {
                        cls = 'log-test';
                    } else if (/\[.*\]/.test(line)) {
                        cls = 'log-debug';
                    }
                    
                    const span = document.createElement('div');
                    span.className = cls;
                    span.textContent = displayText;
                    container.appendChild(span);
                });
                // Auto-scroll to bottom only if user is already at bottom
                const isAtBottom = container.scrollHeight - container.scrollTop <= container.clientHeight + 50;
                if (isAtBottom) {
                    container.scrollTop = container.scrollHeight;
                }
            });
        }

        function updateStatus() {
            fetch('/status').then(r => r.text()).then(state => {
                const led = document.getElementById('running-led');
                if (state === "on") led.classList.add('active');
                else led.classList.remove('active');
            });
        }

        setInterval(updateLogs, 2000);
        setInterval(updateStatus, 500);

        let verboseEnabled = false;

        function toggleVerbose() {
            verboseEnabled = !verboseEnabled;
            const btn = document.getElementById('verbose-btn');
            if (verboseEnabled) {
                btn.textContent = 'Verbose: ON';
                btn.classList.add('verbose-active');
                fetch('/verbose?val=1');
            } else {
                btn.textContent = 'Verbose: OFF';
                btn.classList.remove('verbose-active');
                fetch('/verbose?val=0');
            }
        }

        function clearLogs() {
            document.getElementById('debug-container').innerHTML = '';
            fetch('/clearlogs');
        }

        // Color wheel and lamp control functions
        let selectedHue = 0;
        let selectedSaturation = 255;
        let selectedValue = 255;
        let selectedLamps = [];
        let lampStates = {}; // Store state for each lamp

        // Initialize lamp states
        function initLampStates() {
            for (let i = 0; i < 11; i++) {
                lampStates[i] = {
                    hue: 0,
                    saturation: 255,
                    value: 255
                };
            }
        }

        // Generate lamp checkbox grid
        function generateLampGrid() {
            const grid = document.getElementById('lamp-checkbox-grid');
            const colors = ['Red', 'Orange', 'Yellow', 'Green-Yellow', 'Green', 'Cyan', 'Blue', 'Purple', 'Magenta', 'Pink', 'Red-Pink'];
            
            for (let i = 0; i < 11; i++) {
                const label = document.createElement('label');
                label.style.cssText = 'display: flex; align-items: center; color: #eee; cursor: pointer; padding: 8px; border-radius: 5px; transition: all 0.2s; border: 2px solid transparent;';
                label.title = `Lamp ${i + 1} (${colors[i]})`;
                label.onmouseover = () => label.style.background = '#0f3460';
                label.onmouseout = () => label.style.background = 'transparent';
                
                const checkbox = document.createElement('input');
                checkbox.type = 'checkbox';
                checkbox.value = i;
                checkbox.style.cssText = 'margin-right: 8px; width: 18px; height: 18px; cursor: pointer;';
                checkbox.onchange = () => toggleLampSelection(i, checkbox.checked);
                
                const labelText = document.createTextNode(`Lamp ${i + 1}`);
                label.appendChild(checkbox);
                label.appendChild(labelText);
                label.style.justifyContent = 'center';
                grid.appendChild(label);
            }
        }

        // Toggle lamp selection with state management
        function toggleLampSelection(lampIndex, isSelected) {
            if (isSelected) {
                if (selectedLamps.length > 0) {
                    // Copy state from first selected lamp
                    lampStates[lampIndex] = {
                        hue: selectedHue,
                        saturation: selectedSaturation,
                        value: selectedValue
                    };
                }
                selectedLamps.push(lampIndex);
            } else {
                selectedLamps = selectedLamps.filter(i => i !== lampIndex);
            }
            
            updateLampSelectionUI();
        }

        // Update UI based on selected lamps
        function updateLampSelectionUI() {
            if (selectedLamps.length === 0) {
                document.getElementById('lamp-selection-info').textContent = 'Select at least one lamp to start';
                document.getElementById('lamp-selection-info').style.color = '#aaa';
            } else {
                const humanIndices = selectedLamps.map(index => index + 1);
                document.getElementById('lamp-selection-info').textContent = `${selectedLamps.length} lamp${selectedLamps.length > 1 ? 's' : ''} selected: ${humanIndices.join(', ')}`;
                document.getElementById('lamp-selection-info').style.color = '#4ecca3';
                
                // Sync controls to first selected lamp's state
                selectedHue = lampStates[selectedLamps[0]].hue;
                selectedSaturation = lampStates[selectedLamps[0]].saturation;
                selectedValue = lampStates[selectedLamps[0]].value;
                updateColorPreview();
            }
        }

        function initColorWheel() {
            const canvas = document.getElementById('color-wheel');
            const ctx = canvas.getContext('2d');
            const centerX = canvas.width / 2;
            const centerY = canvas.height / 2;
            const radius = canvas.width / 2;

            // Draw color wheel (HSL rotated to canvas coordinates)
            for (let hue = 0; hue < 360; hue++) {
                // Rotate HSL hue by -90° to align with canvas angles
                const canvasAngle = (hue - 90) * Math.PI / 180;
                const startAngle = canvasAngle - (2 * Math.PI / 180);
                const endAngle = canvasAngle + (2 * Math.PI / 180);
                
                ctx.beginPath();
                ctx.moveTo(centerX, centerY);
                ctx.arc(centerX, centerY, radius, startAngle, endAngle);
                ctx.closePath();
                ctx.fillStyle = `hsl(${hue}, 100%, 50%)`;
                ctx.fill();
            }

            // Add click handler
            canvas.addEventListener('click', function(e) {
                const rect = canvas.getBoundingClientRect();
                const x = e.clientX - rect.left - centerX;
                const y = e.clientY - rect.top - centerY;
                const distance = Math.sqrt(x * x + y * y);

                if (distance <= radius) {
                    let angle = Math.atan2(y, x) * 180 / Math.PI;
                    // Adjust canvas angle back to HSL hue
                    selectedHue = Math.round((angle + 90) % 360);
                    if (selectedHue < 0) selectedHue += 360;
                    updateColorPreview();
                }
            });
        }

        function updateColorPreview() {
            const preview = document.getElementById('color-preview');
            preview.style.background = `hsl(${selectedHue}, 100%, 50%)`;
        }

        function turnOnSelectedLamps() {
            if (selectedLamps.length === 0) {
                alert('Please select at least one lamp');
                return;
            }
            selectedLamps.forEach(lampIndex => {
                fetch(`/lamp?index=${lampIndex}&cmd=on`);
                lampStates[lampIndex].value = 255;
            });
        }

        function turnOffSelectedLamps() {
            if (selectedLamps.length === 0) {
                alert('Please select at least one lamp');
                return;
            }
            selectedLamps.forEach(lampIndex => {
                fetch(`/lamp?index=${lampIndex}&cmd=off`);
                lampStates[lampIndex].value = 0;
            });
        }

        function setWhiteSelectedLamps() {
            if (selectedLamps.length === 0) {
                alert('Please select at least one lamp');
                return;
            }
            selectedLamps.forEach(lampIndex => {
                fetch(`/lamp?index=${lampIndex}&cmd=white`);
                lampStates[lampIndex] = { hue: 0, saturation: 0, value: 255 };
            });
            selectedHue = 0;
            selectedSaturation = 0;
            selectedValue = 255;
            updateColorPreview();
        }

        function setColorSelectedLamps() {
            if (selectedLamps.length === 0) {
                alert('Please select at least one lamp');
                return;
            }
            // Map 0-360 hue to 0-255 for the LivingColors protocol
            const mappedHue = Math.round(selectedHue * 255 / 360);
            selectedLamps.forEach(lampIndex => {
                fetch(`/lamp?index=${lampIndex}&cmd=color&hue=${mappedHue}&sat=${selectedSaturation}&val=${selectedValue}`);
                lampStates[lampIndex] = {
                    hue: selectedHue,
                    saturation: selectedSaturation,
                    value: selectedValue
                };
            });
        }

        function updateMasterDimmer() {
            const value = document.getElementById('master-dimmer').value;
            document.getElementById('dimmer-value').textContent = value;
            sendCmd(`master_dim:${value}`);
            
            // Update all lamp states
            selectedLamps.forEach(lampIndex => {
                lampStates[lampIndex].value = parseInt(value);
            });
        }

        function savePreset(slot) {
            sendCmd(`preset_save:${slot}`);
            alert(`Preset ${slot + 1} saved!`);
        }

        function loadPreset(slot) {
            sendCmd(`preset_load:${slot}`);
            alert(`Preset ${slot + 1} loaded!`);
        }

        function sendRemote(hexCode) {
            const zone = document.getElementById('remote-zone').value;
            controlStrip(zone, hexCode);
        }

        function controlStrip(zone, hexCode) {
            const cmd = `strip:${zone}:${hexCode.toString(16)}`;
            sendCmd(cmd);
        }

        // Initialize on page load
        window.addEventListener('load', function() {
            initLampStates();
            generateLampGrid();
            initColorWheel();
            updateSceneButtons();
            
            // Sync device time from client
            const deviceTime = Math.floor(Date.now() / 1000);
            fetch(`/settime?time=${deviceTime}`).catch(err => console.log('Time sync failed:', err));
        });
    </script>
</body>
</html>
)=====";

#endif