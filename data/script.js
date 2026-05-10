const TOTAL_DASH = 330;
let gateway = `ws://${window.location.hostname}/ws`;
let websocket;

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    document.getElementById('connection-status').innerText = "Live Data Stream: Active";
    document.getElementById('connection-status').style.color = "var(--green)";
}

function onClose(event) {
    console.log('Connection closed');
    document.getElementById('connection-status').innerText = "Connection lost. Reconnecting...";
    document.getElementById('connection-status').style.color = "var(--red)";
    setTimeout(initWebSocket, 2000);
}

function updateGauge(el, value, min, max) {
    const clamped = Math.max(min, Math.min(max, value));
    const percent = (clamped - min) / (max - min);
    el.style.strokeDashoffset = TOTAL_DASH * (1 - percent);
}

function onMessage(event) {
    console.log('Received data:', event.data);
    let data;
    try {
        data = JSON.parse(event.data);
    } catch(e) {
        console.error("Invalid JSON:", event.data);
        return;
    }

    if (data.temperature !== undefined) {
        let t = parseFloat(data.temperature);
        document.getElementById('temp-val').innerText = t.toFixed(1);
        updateGauge(document.getElementById('temp-gauge'), t, 0, 50);
    }
    if (data.humidity !== undefined) {
        let h = parseFloat(data.humidity);
        document.getElementById('humi-val').innerText = h.toFixed(1);
        updateGauge(document.getElementById('humi-gauge'), h, 0, 100);
    }
    if (data.lcd_state !== undefined) {
        let el = document.getElementById('system-state');
        let desc = document.getElementById('system-desc');
        el.innerText = data.lcd_state;
        if(data.lcd_state === "NORMAL") {
            el.style.color = "var(--green)";
            desc.innerText = "System operating within safe parameters.";
        } else if (data.lcd_state === "WARNING") {
            el.style.color = "var(--orange)";
            desc.innerText = "Values outside optimal range. Monitor closely.";
        } else if (data.lcd_state === "CRITICAL") {
            el.style.color = "var(--red)";
            desc.innerText = "Thresholds exceeded! Action required.";
        }
    }
    
    if (data.tinyml_score !== undefined) {
        let el = document.getElementById('tinyml-status');
        document.getElementById('tinyml-score').innerText = parseFloat(data.tinyml_score).toFixed(4);
        document.getElementById('tinyml-infer').innerText = data.tinyml_infer_ms;
        
        if (data.tinyml_anomaly) {
            el.innerText = "ANOMALY ⚠️";
            el.style.color = "var(--red)";
            el.style.textShadow = "0 0 10px rgba(255, 123, 114, 0.5)";
        } else {
            el.innerText = "NORMAL ✅";
            el.style.color = "var(--green)";
            el.style.textShadow = "0 0 10px rgba(46, 160, 67, 0.5)";
        }
    }
}

function switchTab(tabId) {
    document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
    
    document.getElementById(tabId).classList.add('active');
    event.target.classList.add('active');
}

let deviceStates = { dev1: false, dev2: false };

function toggleDevice(gpio, devId) {
    deviceStates[devId] = !deviceStates[devId];
    let statusStr = deviceStates[devId] ? "ON" : "OFF";
    
    let btn = document.getElementById('btn-' + devId);
    btn.innerText = `Device ${devId === 'dev1' ? '1 (Pin 47)' : '2 (Pin 38)'}: ${statusStr}`;
    btn.style.background = deviceStates[devId] ? "var(--green)" : "var(--panel)";

    let controlData = {
        page: "device",
        value: {
            gpio: gpio,
            status: statusStr
        }
    };

    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify(controlData));
    }
}

function saveSettings() {
    let ssid = document.getElementById('wifi_ssid').value;
    let pass = document.getElementById('wifi_pass').value;
    let token = document.getElementById('core_iot_token').value;
    let server = document.getElementById('core_iot_server').value;
    let port = document.getElementById('core_iot_port').value;

    let statusMsg = document.getElementById('save-status');
    
    if(!ssid || !token) {
        statusMsg.innerText = "SSID and Token are required!";
        statusMsg.style.color = "var(--red)";
        return;
    }

    statusMsg.innerText = "Saving configuration...";
    statusMsg.style.color = "var(--cyan)";

    // Must match format expected by task_handler.cpp:
    // doc["page"] == "setting" && doc["value"]["ssid"], ["password"], ["token"], ["server"], ["port"]
    let configData = {
        page: "setting",
        value: {
            ssid: ssid,
            password: pass,
            token: token,
            server: server,
            port: port
        }
    };

    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify(configData));
        statusMsg.innerText = "Config sent! Rebooting ESP32...";
        statusMsg.style.color = "var(--green)";
    } else {
        statusMsg.innerText = "WebSocket not connected!";
        statusMsg.style.color = "var(--red)";
    }
}
