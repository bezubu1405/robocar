#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h> // For saving settings
#include <ESPmDNS.h> // For mDNS

// --- WiFi & Preferences Setup ---
Preferences preferences;
String sta_ssid = "AAIRAH5G";
String sta_password = "qwerty123";
String ap_ssid = "ORBITERBOOTHS";
String ap_password = "12345678";
String mdns_name = "esp32car";

// Motor control pins
#define MOTOR1_REN 32
#define MOTOR1_LEN 33
#define MOTOR1_RPWM 25
#define MOTOR1_LPWM 26

#define MOTOR2_REN 27

#define MOTOR2_LEN 14
#define MOTOR2_RPWM 12
#define MOTOR2_LPWM 13

// Define GPIO pin for light relay - NOTE: Pin 13 is shared with MOTOR2_LPWM in your original code.
// Changed to a different pin to avoid conflict. Please connect your relay to GPIO 2.
#define LIGHT_RELAY_PIN 2

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// --- Speed ramping variables ---
int targetSpeed1_R = 0, currentSpeed1_R = 0, lastTarget1_R = 0;
int targetSpeed1_L = 0, currentSpeed1_L = 0, lastTarget1_L = 0;
int targetSpeed2_R = 0, currentSpeed2_R = 0, lastTarget2_R = 0;
int targetSpeed2_L = 0, currentSpeed2_L = 0, lastTarget2_L = 0;
unsigned long lastRampUpdate = 0;
const int rampStepAccel = 15;      // Acceleration step
const int rampStepDecel = 7;       // Deceleration step (smaller for smoother stop)
const int rampStepDirChange = 5;   // Step when switching direction
const unsigned long rampInterval = 20;

// --- Emergency stop flag ---
volatile bool emergencyStopActive = false;

// --- Light relay state ---
bool lightRelayOn = false;

// --- Fully Featured Webpage ---
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title id="page-title">ROBOBOT Car Control</title>
    <style>
        html, body { height: 100%; margin: 0; padding: 0; }
        body { 
            font-family: 'Segoe UI', Arial, sans-serif; 
            background: #282a36; 
            color: #f8f8f2; 
            margin: 0; 
            min-height: 100vh; 
            display: flex; 
            flex-direction: column; 
            height: 100vh; 
            overflow-x: hidden;
        }
        #bg-gradient {
            position: fixed;
            top: 0; left: 0; width: 100vw; height: 100vh;
            z-index: 0;
            pointer-events: none;
            transition: background 0.5s cubic-bezier(.4,0,.2,1);
            background: linear-gradient(135deg, #282a36 0%, #44475a 100%);
        }
        .topbar { 
            width: 100vw; 
            display: flex; 
            justify-content: space-between; 
            align-items: center; 
            padding: 0 10px; 
            height: 56px; 
            background: rgba(40,42,54,0.95); 
            position: fixed; 
            top: 0; left: 0; z-index: 10; 
            border-bottom: 0 solid transparent; 
            box-shadow: 0 2px 12px #0002;
        }
        .top-controls {
            width: 100vw;
            display: flex;
            flex-direction: row;
            justify-content: center;
            align-items: flex-start;
            gap: 18px;
            position: fixed;
            top: 56px;
            left: 0;
            z-index: 10;
            padding: 16px 0 0 0;
            pointer-events: none;
        }
        .speed-container, .light-container {
            background: rgba(40,42,54,0.65);
            border-radius: 14px;
            box-shadow: 0 2px 12px #0002;
            padding: 14px 18px 10px 18px;
            margin: 0 6px;
            display: flex;
            flex-direction: column;
            align-items: center;
            pointer-events: auto;
        }
        .speed-container {
            min-width: 220px;
        }
        .slider-label { 
            margin-bottom: 6px; 
            font-size: 1.1em; 
            color: #bd93f9; 
            display: flex; 
            align-items: center; 
            gap: 10px;
        }
        .slider-controls {
            display: flex;
            flex-direction: row;
            align-items: center;
            gap: 10px;
            width: 100%;
        }
        .slider-btn {
            background: #44475a;
            color: #fff;
            border: none;
            border-radius: 50%;
            width: 34px;
            height: 34px;
            font-size: 1.3em;
            cursor: pointer;
            transition: background 0.2s;
            box-shadow: 0 2px 8px #0002;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .slider-btn:active { background: #6272a4; }
        .slider { 
            width: 100%; 
            margin: 0 auto 10px; 
            accent-color: #ff79c6; 
            height: 8px; 
            border-radius: 6px;
            background: #282a36;
        }
        .light-label {
            color: #f1fa8c;
            font-weight: 500;
            font-size: 1.1em;
            margin-left: 4px;
            letter-spacing: 1px;
        }
        .toggle-switch {
            position: relative;
            display: inline-block;
            width: 54px;
            height: 28px;
            margin-left: 10px;
            vertical-align: middle;
        }
        .toggle-switch input { display: none; }
        .slider-toggle {
            position: absolute;
            cursor: pointer;
            top: 0; left: 0; right: 0; bottom: 0;
            background: #44475a;
            border-radius: 34px;
            transition: background 0.3s;
        }
        .slider-toggle:before {
            position: absolute;
            content: "";
            height: 22px;
            width: 22px;
            left: 4px;
            bottom: 3px;
            background: #fff;
            border-radius: 50%;
            transition: transform 0.3s;
            box-shadow: 0 2px 8px #0003;
        }
        .toggle-switch input:checked + .slider-toggle {
            background: #28c76f;
        }
        .toggle-switch input:checked + .slider-toggle:before {
            transform: translateX(26px);
            background: #50fa7b;
        }
        .main-content { 
            flex: 1; 
            display: flex; 
            flex-direction: column; 
            align-items: center; 
            justify-content: center; 
            margin-top: 120px; /* space for topbar + controls */
            margin-bottom: 120px; 
            z-index: 1;
            position: relative;
        }
        .joystick-center-wrap { 
            display: flex; 
            flex-direction: column; 
            align-items: center; 
            justify-content: center; 
            flex: 1; 
            margin-top: 32px;
        }
        .joystick-container { 
            position: relative; 
            width: 180px; 
            height: 180px; 
            background: rgba(68,71,90,0.95); 
            border-radius: 50%; 
            box-shadow: 0 4px 16px #0005; 
            touch-action: none; 
            border: 2px solid #50fa7b; 
            margin-bottom: 18px; 
            display: flex; 
            align-items: center; 
            justify-content: center; 
        }
        .joystick-knob { 
            position: absolute; 
            width: 70px; 
            height: 70px; 
            background: radial-gradient(circle at 30% 30%, #8be9fd, #50fa7b); 
            border-radius: 50%; 
            top: 50%; left: 50%; 
            transform: translate(-50%, -50%); 
            box-shadow: 0 2px 10px #50fa7b88; 
            touch-action: none; 
            transition: box-shadow 0.2s;
        }
        .button { 
            padding: 12px 28px; 
            font-size: 1.1em; 
            color: #fff; 
            border: none; 
            border-radius: 8px; 
            cursor: pointer; 
            font-weight: bold; 
            transition: background 0.2s, box-shadow 0.2s; 
            margin-bottom: 14px; 
        }
        .emergency-btn {
            background: #ff5555;
            box-shadow: 0 2px 8px #ff555588;
            width: 90vw;
            max-width: 400px;
            margin: 0 auto 0 auto;
            display: block;
            position: fixed;
            left: 50%;
            transform: translateX(-50%);
            bottom: 64px;
            z-index: 101;
        }
        .emergency-btn:hover { background: #ff2222; }
        .footer-bar { 
            width: 100vw; 
            background: #181818; 
            color: #aaa; 
            font-size: 0.9em; 
            padding: 12px 0 12px 0; 
            letter-spacing: 1px; 
            display: flex; 
            justify-content: center; 
            align-items: center; 
            position: fixed; 
            left: 0; 
            bottom: 0; 
            z-index: 99; 
        }
        .footer-bar .footer-text { text-align: center; width: 100%; }
        @media (max-width: 900px) {
            .top-controls {
                flex-direction: column;
                align-items: stretch;
                gap: 10px;
                padding: 10px 0 0 0;
            }
            .speed-container, .light-container {
                margin: 0 auto;
                width: 90vw;
                min-width: unset;
            }
        }
        @media (max-width: 600px) {
            h1 { font-size: 1.5em; }
            .joystick-container { width: 140px; height: 140px; }
            .joystick-knob { width: 54px; height: 54px; }
            .main-content { margin-top: 120px; margin-bottom: 120px; }
            .footer-bar { font-size: 1em; padding: 10px 0 10px 0; }
            .emergency-btn { max-width: 98vw; }
            .speed-container, .light-container { padding: 10px 8px 8px 8px; }
        }
        .fullscreen-fix { background: #282a36 !important; }
        /* Hide modal by default */
        .modal { display: none; position: fixed; z-index: 200; left: 0; top: 0; width: 100%; height: 100%; overflow: auto; background-color: rgba(0,0,0,0.6); }
        .modal-content { background-color: #44475a; margin: 10% auto; padding: 20px; border: 1px solid #888; width: 90%; max-width: 400px; border-radius: 10px; color: #f8f8f2; }
        .close-btn { color: #aaa; float: right; font-size: 28px; font-weight: bold; cursor: pointer; }
        .modal-content h2 { color: #50fa7b; border-bottom: 1px solid #6272a4; padding-bottom: 10px; }
        .modal-content label { display: block; margin-top: 15px; text-align: left; color: #bd93f9; }
        .modal-content input, .modal-content select { width: 95%; padding: 8px; margin-top: 5px; border-radius: 5px; border: 1px solid #6272a4; background: #282a36; color:rgb(98, 231, 104); }
        .modal-content .button { width: 100%; margin-top: 20px; }
    </style>
</head>
<body>
    <div id="bg-gradient"></div>
    <div class="topbar" id="topbar">
        <button class="topbar-btn fullscreen" id="fullscreen-btn" title="Fullscreen" onclick="toggleFullscreen()">
            <svg viewBox="0 0 32 32"><path d="M9 23h14V9H9V23zM11 11h10v10H11V11z"/><polygon points="7,21 5,21 5,27 11,27 11,25 7,25 "/><polygon points="7,7 11,7 11,5 5,5 5,11 7,11 "/><polygon points="25,25 21,25 21,27 27,27 27,21 25,21 "/><polygon points="21,5 21,7 25,7 25,11 27,11 27,5 "/></svg>
        </button>
        <h1 id="header-title">ROBOBOT</h1>
        <button class="topbar-btn settings" id="settings-btn" title="Settings" onclick="openModal()">
            <svg viewBox="0 0 512 512"><path d="M496,293.984c9.031-0.703,16-8.25,16-17.297v-41.375c0-9.063-6.969-16.594-16-17.313l-54.828-4.281
		c-3.484-0.266-6.484-2.453-7.828-5.688l-18.031-43.516c-1.344-3.219-0.781-6.906,1.5-9.547l35.75-41.813
		c5.875-6.891,5.5-17.141-0.922-23.547l-29.25-29.25c-6.406-6.406-16.672-6.813-23.547-0.922l-41.813,35.75
		c-2.641,2.266-6.344,2.844-9.547,1.516l-43.531-18.047c-3.219-1.328-5.422-4.375-5.703-7.828l-4.266-54.813
		C293.281,6.969,285.75,0,276.688,0h-41.375c-9.063,0-16.594,6.969-17.297,16.016l-4.281,54.813c-0.266,3.469-2.469,6.5-5.688,7.828
		l-43.531,18.047c-3.219,1.328-6.906,0.75-9.563-1.516l-41.797-35.75c-6.875-5.891-17.125-5.484-23.547,0.922l-29.25,29.25
		c-6.406,6.406-6.797,16.656-0.922,23.547l35.75,41.813c2.25,2.641,2.844,6.328,1.5,9.547l-18.031,43.516
		c-1.313,3.234-4.359,5.422-7.813,5.688L16,218c-9.031,0.719-16,8.25-16,17.313v41.359c0,9.063,6.969,16.609,16,17.313l54.844,4.266
		c3.453,0.281,6.5,2.484,7.813,5.703l18.031,43.516c1.344,3.219,0.75,6.922-1.5,9.563l-35.75,41.813
		c-5.875,6.875-5.484,17.125,0.922,23.547l29.25,29.25c6.422,6.406,16.672,6.797,23.547,0.906l41.797-35.75
		c2.656-2.25,6.344-2.844,9.563-1.5l43.531,18.031c3.219,1.344,5.422,4.359,5.688,7.844l4.281,54.813
		c0.703,9.031,8.234,16.016,17.297,16.016h41.375c9.063,0,16.594-6.984,17.297-16.016l4.266-54.813
		c0.281-3.484,2.484-6.5,5.703-7.844l43.531-18.031c3.203-1.344,6.922-0.75,9.547,1.5l41.813,35.75
		c6.875,5.891,17.141,5.5,23.547-0.906l29.25-29.25c6.422-6.422,6.797-16.672,0.922-23.547l-35.75-41.813
		c-2.25-2.641-2.844-6.344-1.5-9.563l18.031-43.516c1.344-3.219,4.344-5.422,7.828-5.703L496,293.984z M256,342.516
		c-23.109,0-44.844-9-61.188-25.328c-16.344-16.359-25.344-38.078-25.344-61.203c0-23.109,9-44.844,25.344-61.172
		c16.344-16.359,38.078-25.344,61.188-25.344c23.125,0,44.844,8.984,61.188,25.344c16.344,16.328,25.344,38.063,25.344,61.172
		c0,23.125-9,44.844-25.344,61.203C300.844,333.516,279.125,342.516,256,342.516z"/></svg>
        </button>
    </div>
    <div class="top-controls">
        <div class="speed-container">
            <span class="slider-label">
                Speed: <span id="speedValue">50</span>%
            </span>
            <div class="slider-controls">
                <button class="slider-btn" id="minusBtn" onclick="changeSpeed(-5)">&#8722;</button>
                <input type="range" id="speed" class="slider" min="0" max="100" value="50" oninput="updateSpeed()">
                <button class="slider-btn" id="plusBtn" onclick="changeSpeed(5)">&#43;</button>
            </div>
        </div>
        <div class="light-container">
            <span class="light-label">Light
                <label class="toggle-switch">
                    <input type="checkbox" id="lightSwitch" onchange="toggleLightRelay()">
                    <span class="slider-toggle"></span>
                </label>
            </span>
        </div>
    </div>
    <div class="main-content">
        <div class="joystick-center-wrap">
            <div class="joystick-container" id="joystick"> <div class="joystick-knob" id="knob"></div> </div>
        </div>
    </div>
    <button class="button emergency-btn" onclick="emergencyStop()">EMERGENCY STOP</button>

    <div id="settingsModal" class="modal">
        <div class="modal-content">
            <span class="close-btn" onclick="closeModal()">&times;</span>
            <h2>WiFi Settings</h2>
            <div id="status"></div>
            <!-- mDNS config -->
            <label for="mdns_name">mDNS Hostname (.local):</label>
            <input type="text" id="mdns_name" name="mdns_name" maxlength="32" pattern="[a-zA-Z0-9\-]+">
            <button id="save-mdns-btn" class="button" onclick="saveMDNS()">Save & Restart</button>
            <hr style="margin: 20px 0; border-color: #6272a4;">
            <!-- STA config -->
            <label for="sta_ssid">Connect to WiFi (STA mode) SSID:</label>
            <input type="text" id="sta_ssid" name="sta_ssid">
            <label for="sta_pass">WiFi Password:</label>
            <input type="password" id="sta_pass" name="sta_pass">
            <button id="save-sta-btn" class="button" onclick="saveSTASettings()">Save & Restart</button>
            <hr style="margin: 20px 0; border-color:rgb(100, 255, 126);">
            <!-- AP config -->
            <label for="ap_ssid">Configure AP Mode SSID:</label>
            <input type="text" id="ap_ssid" name="ap_ssid">
            <label for="ap_pass">AP Password (min 8 chars):</label>
            <input type="password" id="ap_pass" name="ap_pass">
            <button id="save-ap-btn" class="button" onclick="saveAPSettings()">Save & Restart</button>
        </div>
    </div>

    <div class="footer-bar">
        <span class="footer-text">Developed by orbiterbooths</span>
    </div>
    <script>
        let currentSpeed = 128;
        const joystick = document.getElementById('joystick');
        const knob = document.getElementById('knob');
        let activeAction = null;
        let joystickActive = false;

        // --- Smooth background gradient logic ---
        const bgGradient = document.getElementById('bg-gradient');
        function setBgGradient(dx, dy) {
            // dx, dy: -1..1
            // Map direction to angle and color
            let angle = 135;
            let color1 = "#282a36", color2 = "#44475a";
            if (typeof dx === "number" && typeof dy === "number") {
                angle = Math.atan2(dy, dx) * 180 / Math.PI + 90;
                let intensity = Math.min(Math.sqrt(dx*dx + dy*dy), 1);
                // Color stops: forward=green, back=red, left=blue, right=yellow
                if (intensity > 0.1) {
                    if (Math.abs(dx) > Math.abs(dy)) {
                        if (dx > 0) { color2 = "#ffe066"; } // right
                        else { color2 = "#8be9fd"; } // left
                    } else {
                        if (dy > 0) { color2 = "#ff5555"; } // back
                        else { color2 = "#50fa7b"; } // forward
                    }
                    color1 = "#282a36";
                }
                bgGradient.style.background = `linear-gradient(${angle}deg, ${color1} 0%, ${color2} 100%)`;
            } else {
                bgGradient.style.background = `linear-gradient(135deg, #282a36 0%, #44475a 100%)`;
            }
        }

        function updateSpeed() {
            let percent = document.getElementById('speed').value;
            document.getElementById('speedValue').textContent = percent;
            currentSpeed = Math.round(percent * 2.55); // 0-100% to 0-255
        }
        function changeSpeed(delta) {
            let slider = document.getElementById('speed');
            let val = parseInt(slider.value) + delta;
            val = Math.max(0, Math.min(100, val));
            slider.value = val;
            updateSpeed();
        }

        function sendControl(action) {
            fetch(`/${action}?speed=${currentSpeed}`, { method: 'POST' }).catch(err => console.error(err));
        }

        function emergencyStop() {
            fetch('/emergencystop', { method: 'POST' }).finally(() => {
                activeAction = null; joystickActive = false; resetKnob();
                setBgGradient(); // reset bg
            });
        }

        function calculateDirection(dx, dy) {
            const threshold = 22;
            if (Math.abs(dx) < threshold && Math.abs(dy) < threshold) return 'stop';
            if (Math.abs(dx) > Math.abs(dy)) return dx > 0 ? 'right' : 'left';
            return dy > 0 ? 'backward' : 'forward';
        }

        function resetKnob() { knob.style.transform = 'translate(-50%, -50%)'; }

        function handleJoystickStart(event) {
            joystickActive = true; event.preventDefault();
        }

        function handleJoystickMove(event) {
            if (!joystickActive) return;
            event.preventDefault();
            const rect = joystick.getBoundingClientRect();
            const clientX = event.touches ? event.touches[0].clientX : event.clientX;
            const clientY = event.touches ? event.touches[0].clientY : event.clientY;
            let dx = clientX - (rect.left + rect.width / 2);
            let dy = clientY - (rect.top + rect.height / 2);
            const maxDist = rect.width / 2 - knob.offsetWidth / 2;
            const distance = Math.sqrt(dx * dx + dy * dy);
            if (distance > maxDist) {
                dx = (dx / distance) * maxDist;
                dy = (dy / distance) * maxDist;
            }
            knob.style.transform = `translate(calc(-50% + ${dx}px), calc(-50% + ${dy}px))`;
            // For background: normalize to -1..1
            setBgGradient(dx / maxDist, dy / maxDist);
            const direction = calculateDirection(clientX - (rect.left + rect.width / 2), clientY - (rect.top + rect.height / 2));
            if (activeAction !== direction && direction) {
                activeAction = direction; sendControl(direction);
            }
        }

        function handleJoystickEnd() {
            if (!joystickActive) return;
            joystickActive = false;
            if (activeAction !== 'stop') {
                activeAction = 'stop'; sendControl('stop');
            }
            resetKnob();
            setBgGradient(); // reset bg
        }

        // Keyboard arrow key support
        document.addEventListener('keydown', function(e) {
            if (e.repeat) return;
            switch (e.key) {
                case "ArrowUp": sendControl('forward'); activeAction = 'forward'; setBgGradient(0, -1); break;
                case "ArrowDown": sendControl('backward'); activeAction = 'backward'; setBgGradient(0, 1); break;
                case "ArrowLeft": sendControl('left'); activeAction = 'left'; setBgGradient(-1, 0); break;
                case "ArrowRight": sendControl('right'); activeAction = 'right'; setBgGradient(1, 0); break;
                case " ": emergencyStop(); break;
            }
        });
        document.addEventListener('keyup', function(e) {
            if (["ArrowUp","ArrowDown","ArrowLeft","ArrowRight"].includes(e.key)) {
                sendControl('stop');
                activeAction = null;
                setBgGradient();
            }
        });

        joystick.addEventListener('mousedown', handleJoystickStart);
        document.addEventListener('mousemove', handleJoystickMove);
        document.addEventListener('mouseup', handleJoystickEnd);
        joystick.addEventListener('touchstart', handleJoystickStart, { passive: false });
        document.addEventListener('touchmove', handleJoystickMove, { passive: false });
        document.addEventListener('touchend', handleJoystickEnd);
        document.addEventListener('touchcancel', handleJoystickEnd);

        // --- Light relay logic ---
        function updateLightSwitchUI(state) {
            document.getElementById('lightSwitch').checked = !!state;
        }
        function toggleLightRelay() {
            fetch('/togglelight', { method: 'POST' })
                .then(r => r.json())
                .then(data => updateLightSwitchUI(data.on));
        }
        function getLightRelayState() {
            fetch('/getlight').then r => r.json()).then(data => updateLightSwitchUI(data.on));
        }

        // --- Modal & WiFi Logic ---
        const modal = document.getElementById('settingsModal');
        const statusEl = document.getElementById('status');
        
        function openModal() { 
            modal.style.display = 'block'; 
            fetch('/getwifi').then(r => r.json()).then(data => {
                document.getElementById('ap_ssid').value = data.ap_ssid || '';
                document.getElementById('ap_pass').value = data.ap_pass || '';
                document.getElementById('sta_ssid').value = data.sta_ssid || '';
                document.getElementById('sta_pass').value = data.sta_pass || '';
                document.getElementById('mdns_name').value = data.mdns_name || 'esp32car';
            });
        }
        function closeModal() { modal.style.display = 'none'; }
        window.onclick = function(event) { if (event.target == modal) closeModal(); }

        function saveAPSettings() {
            const ssid = document.getElementById('ap_ssid').value;
            const pass = document.getElementById('ap_pass').value;
            if (pass.length > 0 && pass.length < 8) {
                statusEl.textContent = "Password must be at least 8 characters."; return;
            }
            statusEl.textContent = "Saving AP settings...";
            fetch(`/setap?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`, { method: 'POST' })
                .then(response => response.text()).then(text => {
                    statusEl.textContent = text;
                    setTimeout(closeModal, 2000);
                }).catch(() => statusEl.textContent = "Failed to save AP settings.");
        }
        function saveSTASettings() {
            const ssid = document.getElementById('sta_ssid').value;
            const pass = document.getElementById('sta_pass').value;
            statusEl.textContent = "Saving STA settings...";
            fetch(`/setsta?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`, { method: 'POST' })
                .then(response => response.text()).then(text => {
                    statusEl.textContent = text;
                    setTimeout(closeModal, 2000);
                }).catch(() => statusEl.textContent = "Failed to save STA settings.");
        }
        function saveMDNS() {
            const mdns = document.getElementById('mdns_name').value.trim();
            if (!mdns.match(/^[a-zA-Z0-9\-]+$/)) {
                statusEl.textContent = "Invalid mDNS name. Use letters, numbers, or hyphens.";
                return;
            }
            statusEl.textContent = "Saving mDNS hostname...";
            fetch(`/setmdns?name=${encodeURIComponent(mdns)}`, { method: 'POST' })
                .then(response => response.text()).then(text => {
                    statusEl.textContent = text;
                    setTimeout(closeModal, 2000);
                }).catch(() => statusEl.textContent = "Failed to save mDNS.");
        }

        function toggleFullscreen() {
            if (!document.fullscreenElement) {
                document.documentElement.requestFullscreen().then(() => {
                    document.getElementById('topbar').classList.add('fullscreen-fix');
                }).catch(err => {});
            } else {
                document.exitFullscreen();
            }
        }
        document.addEventListener('fullscreenchange', function() {
            const topbar = document.getElementById('topbar');
            if (document.fullscreenElement) {
                topbar.classList.add('fullscreen-fix');
            } else {
                topbar.classList.remove('fullscreen-fix');
            }
        });

        // Update header with mDNS name in uppercase
        function setHeaderMDNS(name) {
            document.getElementById('header-title').innerText = name.toUpperCase();
            document.title = name.toUpperCase() + " Car Control";
        }

        // Fetch mDNS name and update header on load
        window.addEventListener('DOMContentLoaded', function() {
            fetch('/getwifi').then(r => r.json()).then(data => {
                setHeaderMDNS(data.mdns_name || 'ROBOBOT');
            });
            updateSpeed();
            getLightRelayState();
            setBgGradient();
        });
    </script>
</body>
</html>
)rawliteral";


void initWiFi() {
  // Load credentials from non-volatile storage
  preferences.begin("car-wifi", false);
  sta_ssid = preferences.getString("sta_ssid", "AAIRAH5G");
  sta_password = preferences.getString("sta_pass", "qwerty123");
  ap_ssid = preferences.getString("ap_ssid", "ORBITERBOOTHS");
  ap_password = preferences.getString("ap_pass", "12345678");
  mdns_name = preferences.getString("mdns_name", "esp32car");
  preferences.end();

  // Enable both AP and STA mode
  WiFi.mode(WIFI_AP_STA);

  // Start AP mode (always available)
  WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());

  // Start STA mode (connect to router)
  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());

  Serial.println("Started Access Point.");
  Serial.print("AP SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP Password: ");
  Serial.println(ap_password);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("Attempting to connect to WiFi (STA): ");
  Serial.println(sta_ssid);

  int wait_time = 30; // 15 seconds
  while (WiFi.status() != WL_CONNECTED && wait_time > 0) {
    delay(500);
    Serial.print(".");
    wait_time--;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi (STA) connected!");
    Serial.print("STA IP Address: ");
    Serial.println(WiFi.localIP());
    // Start mDNS responder for easier access (http://<mdns_name>.local/)
    if (MDNS.begin(mdns_name.c_str())) {
      Serial.print("mDNS responder started: http://");
      Serial.print(mdns_name);
      Serial.println(".local/");
    } else {
      Serial.println("Error setting up mDNS responder!");
    }
  } else {
    Serial.println("\nFailed to connect to WiFi (STA). AP mode still available.");
  }
}

void rampMotor(int &current, int &target, int &lastTarget) {
    // If direction changes, ramp to zero first
    if ((current > 0 && target < 0) || (current < 0 && target > 0)) {
        // Decelerate toward zero before switching direction
        if (current > 0) {
            current -= rampStepDirChange;
            if (current < 0) current = 0;
        } else if (current < 0) {
            current += rampStepDirChange;
            if (current > 0) current = 0;
        }
    } else if (abs(target) < abs(current)) {
        // Decelerate
        if (current > target) {
            current -= rampStepDecel;
            if (current < target) current = target;
        } else if (current < target) {
            current += rampStepDecel;
            if (current > target) current = target;
        }
    } else if (abs(target) > abs(current)) {
        // Accelerate
        if (current < target) {
            current += rampStepAccel;
            if (current > target) current = target;
        } else if (current > target) {
            current -= rampStepAccel;
            if (current < target) current = target;
        }
    }
    lastTarget = target;
}

void rampMotors() {
    rampMotor(currentSpeed1_R, targetSpeed1_R, lastTarget1_R);
    rampMotor(currentSpeed1_L, targetSpeed1_L, lastTarget1_L);
    rampMotor(currentSpeed2_R, targetSpeed2_R, lastTarget2_R);
    rampMotor(currentSpeed2_L, targetSpeed2_L, lastTarget2_L);

    // Constrain within 0-255 (or -255 to 255 if you use bidirectional PWM)
    currentSpeed1_R = constrain(currentSpeed1_R, 0, 255);
    currentSpeed1_L = constrain(currentSpeed1_L, 0, 255);
    currentSpeed2_R = constrain(currentSpeed2_R, 0, 255);
    currentSpeed2_L = constrain(currentSpeed2_L, 0, 255);

    analogWrite(MOTOR1_RPWM, currentSpeed1_R);
    analogWrite(MOTOR1_LPWM, currentSpeed1_L);
    analogWrite(MOTOR2_RPWM, currentSpeed2_R);
    analogWrite(MOTOR2_LPWM, currentSpeed2_L);
}

void setMotorTargets(int m1r, int m1l, int m2r, int m2l) {
  targetSpeed1_R = m1r; targetSpeed1_L = m1l;
  targetSpeed2_R = m2r; targetSpeed2_L = m2l;
  emergencyStopActive = false;
}

void allMotorsStop() {
  setMotorTargets(0, 0, 0, 0);
}

void doEmergencyStop() {
  emergencyStopActive = true;
  allMotorsStop();
}

void setup() {
  Serial.begin(115200);

  pinMode(MOTOR1_REN, OUTPUT);
  pinMode(MOTOR1_LEN, OUTPUT);
  pinMode(MOTOR1_RPWM, OUTPUT);
  pinMode(MOTOR1_LPWM, OUTPUT);

  pinMode(MOTOR2_REN, OUTPUT);
  pinMode(MOTOR2_LEN, OUTPUT);
  pinMode(MOTOR2_RPWM, OUTPUT);
  pinMode(MOTOR2_LPWM, OUTPUT);
  
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  digitalWrite(LIGHT_RELAY_PIN, LOW);
  lightRelayOn = false;

  digitalWrite(MOTOR1_REN, HIGH);
  digitalWrite(MOTOR1_LEN, HIGH);
  digitalWrite(MOTOR2_REN, HIGH);
  digitalWrite(MOTOR2_LEN, HIGH);

  initWiFi();

  // --- Web Server Routes ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", webpage);
  });

  server.on("/forward", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!emergencyStopActive && request->hasParam("speed")) {
      int speed = request->getParam("speed")->value().toInt();
      setMotorTargets(speed, 0, speed, 0);
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/backward", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!emergencyStopActive && request->hasParam("speed")) {
      int speed = request->getParam("speed")->value().toInt();
      setMotorTargets(0, speed, 0, speed);
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/left", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!emergencyStopActive && request->hasParam("speed")) {
      int speed = request->getParam("speed")->value().toInt();
      setMotorTargets(speed, 0, 0, speed);
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/right", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!emergencyStopActive && request->hasParam("speed")) {
      int speed = request->getParam("speed")->value().toInt();
      setMotorTargets(0, speed, speed, 0);
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!emergencyStopActive) {
      allMotorsStop();
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/emergencystop", HTTP_POST, [](AsyncWebServerRequest *request){
    doEmergencyStop();
    Serial.println("EMERGENCY STOP ACTIVATED!");
    request->send(200, "text/plain", "Emergency Stopped");
  });
  
  // --- New WiFi Routes ---
  server.on("/setap", HTTP_POST, [](AsyncWebServerRequest *request){
    String ssid, pass;
    if (request->hasParam("ssid") && request->hasParam("pass")) {
        ssid = request->getParam("ssid")->value();
        pass = request->getParam("pass")->value();
        preferences.begin("car-wifi", false);
        preferences.putString("ap_ssid", ssid);
        preferences.putString("ap_pass", pass);
        preferences.end();
        request->send(200, "text/plain", "AP settings saved. Restarting...");
        delay(1000);
        ESP.restart();
    } else {
        request->send(400, "text/plain", "Missing AP SSID or password.");
    }
  });

  server.on("/setsta", HTTP_POST, [](AsyncWebServerRequest *request){
    String ssid, pass;
    if (request->hasParam("ssid") && request->hasParam("pass")) {
        ssid = request->getParam("ssid")->value();
        pass = request->getParam("pass")->value();
        preferences.begin("car-wifi", false);
        preferences.putString("sta_ssid", ssid);
        preferences.putString("sta_pass", pass);
        preferences.end();
        request->send(200, "text/plain", "STA settings saved. Restarting...");
        delay(1000);
        ESP.restart();
    } else {
        request->send(400, "text/plain", "Missing STA SSID or password.");
    }
  });

  // --- mDNS name change endpoint ---
  server.on("/setmdns", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("name")) {
      String name = request->getParam("name")->value();
      if (name.length() < 1 || name.length() > 32) {
        request->send(400, "text/plain", "Invalid mDNS name length.");
        return;
      }
      preferences.begin("car-wifi", false);
      preferences.putString("mdns_name", name);
      preferences.end();
      request->send(200, "text/plain", "mDNS hostname saved. Restarting...");
      delay(1000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", "Missing mDNS name.");
    }
  });

  server.on("/getwifi", HTTP_GET, [](AsyncWebServerRequest *request){
    preferences.begin("car-wifi", true);
    String ap_ssid_val = preferences.getString("ap_ssid", "ORBITERBOOTHS");
    String ap_pass_val = preferences.getString("ap_pass", "12345678");
    String sta_ssid_val = preferences.getString("sta_ssid", "AAIRAH5G");
    String sta_pass_val = preferences.getString("sta_pass", "qwerty123");
    String mdns_val = preferences.getString("mdns_name", "esp32car");
    preferences.end();
    String json = "{";
    json += "\"ap_ssid\":\"" + ap_ssid_val + "\",";
    json += "\"ap_pass\":\"" + ap_pass_val + "\",";
    json += "\"sta_ssid\":\"" + sta_ssid_val + "\",";
    json += "\"sta_pass\":\"" + sta_pass_val + "\",";
    json += "\"mdns_name\":\"" + mdns_val + "\"";
    json += "}";
    request->send(200, "application/json", json);
  });

  // --- New: Captive portal redirect for AP mode ---
  server.onNotFound([](AsyncWebServerRequest *request){
    // If client is connected to AP, redirect all unknown paths to "/"
    if (WiFi.getMode() & WIFI_AP) {
      IPAddress clientIP = request->client()->remoteIP();
      IPAddress apIP = WiFi.softAPIP();
      // Only redirect if client is in AP subnet (192.168.4.x by default)
      if ((clientIP[0] == apIP[0]) && (clientIP[1] == apIP[1]) && (clientIP[2] == apIP[2])) {
        request->redirect("/");
        return;
      }
    }
    // Otherwise, 404
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
}

void loop() {
  unsigned long now = millis();
  if (now - lastRampUpdate >= rampInterval) {
    lastRampUpdate = now;
    if (emergencyStopActive) {
       allMotorsStop(); // Ensure targets are zero
    }
    rampMotors();
  }
}

