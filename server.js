const http = require('http');
const fs = require('fs');
const path = require('path');
const { WebSocketServer } = require('ws');

// =============================================================================
// CONFIGURATION
// =============================================================================
const HOST = '0.0.0.0';
const PORT = 8080;
const WEB_ROOT = '/home/cakurang/mazebot-edukit/web';

// =============================================================================
// MIME TYPES
// =============================================================================
const MIME_TYPES = {
  '.html': 'text/html',
  '.css': 'text/css',
  '.js': 'application/javascript',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.gz': 'application/gzip',
};

// =============================================================================
// HTTP SERVER & REST API (ESP32 Simulator)
// =============================================================================
let tcrtState = {
  adc: 45,
  putih: 15,
  hitam: 185,
  ambang: 100,
  tcrtValid: true,
  garisHitam: false
};

let motorState = {
  mode: 'stop',
  pwmKiri: 0,
  pwmKanan: 0,
  savedKiri: 255,
  savedKanan: 244
};

const server = http.createServer((req, res) => {
  const parsedUrl = new URL(req.url, `http://${req.headers.host}`);
  let pathname = parsedUrl.pathname;

  // Set CORS headers for all responses
  const corsHeaders = {
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type, Authorization'
  };

  if (req.method === 'OPTIONS') {
    res.writeHead(204, corsHeaders);
    res.end();
    return;
  }

  // Health check
  if (pathname === '/health') {
    res.writeHead(200, { 'Content-Type': 'application/json', ...corsHeaders });
    res.end(JSON.stringify({ status: 'ok', server: 'MazeBot EduKit' }));
    return;
  }

  // REST API: GET /api/data
  if (pathname === '/api/data' && req.method === 'GET') {
    // fluctuate ADC slightly for realism
    tcrtState.adc = tcrtState.garisHitam ? (tcrtState.hitam + Math.floor(Math.random() * 10 - 5)) : (tcrtState.putih + Math.floor(Math.random() * 10 - 5));
    if (tcrtState.adc < 0) tcrtState.adc = 0;

    const payload = {
      depan: state.sensors.front_cm,
      kiri: state.sensors.left_cm,
      kanan: state.sensors.right_cm,
      adc: tcrtState.adc,
      putih: tcrtState.putih,
      hitam: tcrtState.hitam,
      ambang: tcrtState.ambang,
      tcrtValid: tcrtState.tcrtValid,
      garisHitam: tcrtState.garisHitam,
      mode: motorState.mode,
      pwmKiri: motorState.pwmKiri,
      pwmKanan: motorState.pwmKanan,
      savedKiri: motorState.savedKiri,
      savedKanan: motorState.savedKanan
    };
    res.writeHead(200, { 'Content-Type': 'application/json', ...corsHeaders });
    res.end(JSON.stringify(payload));
    return;
  }

  // REST API: POST /api/drive
  if (pathname === '/api/drive') {
    const mode = parsedUrl.searchParams.get('mode') || 'stop';
    const l = parseInt(parsedUrl.searchParams.get('l')) || motorState.savedKiri;
    const r = parseInt(parsedUrl.searchParams.get('r')) || motorState.savedKanan;
    motorState.mode = mode;
    motorState.pwmKiri = (mode === 'stop') ? 0 : l;
    motorState.pwmKanan = (mode === 'stop') ? 0 : r;
    console.log(`[REST Drive] mode=${mode}, L=${l}, R=${r}`);
    res.writeHead(200, { 'Content-Type': 'application/json', ...corsHeaders });
    res.end(JSON.stringify({ ok: true, mode: motorState.mode }));
    return;
  }

  // REST API: POST /api/stop
  if (pathname === '/api/stop') {
    motorState.mode = 'stop';
    motorState.pwmKiri = 0;
    motorState.pwmKanan = 0;
    console.log('[REST Stop]');
    res.writeHead(200, { 'Content-Type': 'application/json', ...corsHeaders });
    res.end(JSON.stringify({ ok: true, mode: 'stop' }));
    return;
  }

  // REST API: POST /api/save
  if (pathname === '/api/save') {
    const l = parseInt(parsedUrl.searchParams.get('l')) || 255;
    const r = parseInt(parsedUrl.searchParams.get('r')) || 244;
    motorState.savedKiri = l;
    motorState.savedKanan = r;
    console.log(`[REST Save PWM] L=${l}, R=${r}`);
    res.writeHead(200, { 'Content-Type': 'application/json', ...corsHeaders });
    res.end(JSON.stringify({ ok: true, savedKiri: l, savedKanan: r }));
    return;
  }

  // REST API: POST /api/tcrt/putih
  if (pathname === '/api/tcrt/putih') {
    tcrtState.putih = tcrtState.adc;
    tcrtState.ambang = Math.floor((tcrtState.putih + tcrtState.hitam) / 2);
    tcrtState.tcrtValid = Math.abs(tcrtState.hitam - tcrtState.putih) >= 20;
    const pesan = `Sampel PUTIH tercatat (${tcrtState.putih}). Ambang: ${tcrtState.ambang}`;
    console.log(`[REST TCRT Putih] ${pesan}`);
    res.writeHead(200, { 'Content-Type': 'application/json', ...corsHeaders });
    res.end(JSON.stringify({ ok: true, pesan, tcrtValid: tcrtState.tcrtValid }));
    return;
  }

  // REST API: POST /api/tcrt/hitam
  if (pathname === '/api/tcrt/hitam') {
    tcrtState.hitam = tcrtState.adc;
    tcrtState.ambang = Math.floor((tcrtState.putih + tcrtState.hitam) / 2);
    tcrtState.tcrtValid = Math.abs(tcrtState.hitam - tcrtState.putih) >= 20;
    const pesan = `Sampel HITAM tercatat (${tcrtState.hitam}). Ambang: ${tcrtState.ambang}`;
    console.log(`[REST TCRT Hitam] ${pesan}`);
    res.writeHead(200, { 'Content-Type': 'application/json', ...corsHeaders });
    res.end(JSON.stringify({ ok: true, pesan, tcrtValid: tcrtState.tcrtValid }));
    return;
  }

  // Root -> index.html
  if (pathname === '/' || pathname === '/index.html') {
    pathname = '/index.html';
  }

  // Serve static files
  const filePath = path.join(WEB_ROOT, pathname);
  const ext = path.extname(filePath).toLowerCase();
  const mimeType = MIME_TYPES[ext] || 'application/octet-stream';

  fs.readFile(filePath, (err, content) => {
    if (err) {
      if (err.code === 'ENOENT') {
        res.writeHead(404, { 'Content-Type': 'text/plain' });
        res.end('404 Not Found');
      } else {
        res.writeHead(500, { 'Content-Type': 'text/plain' });
        res.end('500 Internal Server Error');
      }
      return;
    }

    const headers = {
      'Content-Type': mimeType,
      'Content-Length': content.length,
      'Access-Control-Allow-Origin': '*',
      'Cache-Control': mimeType === 'text/html' ? 'no-cache' : 'public, max-age=31536000',
    };

    res.writeHead(200, headers);
    res.end(content);
  });
});

// =============================================================================
// WEBSOCKET SERVER (on same port, upgrade handling)
// =============================================================================
const wss = new WebSocketServer({ noServer: true });

// Handle HTTP upgrade to WebSocket
server.on('upgrade', (request, socket, head) => {
  const parsedUrl = new URL(request.url, `http://${request.headers.host}`);
  
  if (parsedUrl.pathname === '/ws') {
    wss.handleUpgrade(request, socket, head, (ws) => {
      wss.emit('connection', ws, request);
    });
  } else {
    socket.destroy();
  }
});

// WebSocket connection handling
wss.on('connection', (ws, req) => {
  console.log('[WS] Client connected:', req.socket.remoteAddress, '(total:', wss.clients.size, ')');

  ws.on('message', (data) => {
    try {
      const msg = JSON.parse(data.toString());
      console.log('[WS Inbound]', msg);
      handleCommand(msg, ws);
    } catch (e) {
      console.warn('[WS] Invalid message:', data.toString());
    }
  });

  ws.on('close', () => {
    console.log('[WS] Client disconnected (total:', wss.clients.size, ')');
  });

  ws.on('error', (err) => {
    console.error('[WS] Error:', err.message);
  });

  // Send welcome message
  ws.send(JSON.stringify({ type: 'welcome', message: 'Connected to MazeBot EduKit' }));
});

// =============================================================================
// TELEMETRY BROADCAST LOOP (Simulates ESP32)
// =============================================================================
let state = {
  status: 'IDLE',
  timer_ms: 0,
  sensors: {
    left_cm: 25.0,
    front_cm: 30.0,
    right_cm: 5.0,
    line: 0
  },
  maze: {
    grid_count: 0,
    raw_array: [],
    optimized_array: []
  }
};

const mappingTurns = ['L', 'S', 'L', 'U', 'L', 'R', 'S', 'L', 'R', 'S'];
let turnIdx = 0;
let missionActive = false;
let lastMissionTime = Date.now();

function optimizeLSRB(path) {
  if (path.length < 3) return [...path];
  const optimized = [];
  let i = 0;
  while (i < path.length) {
    if (i <= path.length - 3 && path[i + 1] === 'U') {
      const rule = path[i] + 'U' + path[i + 2];
      let repl = null;
      if (rule === 'LUL') repl = 'S';
      else if (rule === 'LUS') repl = 'R';
      else if (rule === 'RUL') repl = 'U';
      else if (rule === 'SUL') repl = 'R';
      else if (rule === 'SUS') repl = 'U';
      else if (rule === 'LUR') repl = 'U';
      if (repl) {
        optimized.push(repl);
        i += 3;
        continue;
      }
    }
    optimized.push(path[i]);
    i++;
  }
  return optimized;
}

function broadcastTelemetry() {
  if (wss.clients.size === 0) return;

  const now = Date.now();

  if (missionActive) {
    state.timer_ms += 100;

    // Simulate sensor readings
    state.sensors.left_cm = Math.round((4.8 + Math.sin(now / 400) * 1.5) * 10) / 10;
    state.sensors.front_cm = Math.round((18.0 + Math.cos(now / 500) * 12.0) * 10) / 10;
    state.sensors.right_cm = Math.round((22.0 + Math.sin(now / 600) * 5.0) * 10) / 10;

    // Add turns during MAPPING
    if (state.status === 'MAPPING' && now - lastMissionTime > 2000) {
      lastMissionTime = now;
      state.maze.grid_count++;
      if (turnIdx < mappingTurns.length) {
        state.maze.raw_array.push(mappingTurns[turnIdx]);
        turnIdx++;
        state.maze.optimized_array = optimizeLSRB(state.maze.raw_array);
      }
    }

    // Finish line at ~16 seconds
    if (state.timer_ms >= 16000) {
      state.sensors.line = 1;
      state.status = 'STOPPED';
      missionActive = false;
    }
  }

  const message = JSON.stringify(state);
  wss.clients.forEach((client) => {
    if (client.readyState === 1) { // WebSocket.OPEN
      client.send(message);
    }
  });
}

// Broadcast at 10 Hz
setInterval(broadcastTelemetry, 100);

// Handle inbound commands from web clients (for simulation control)
function handleCommand(msg, ws) {
  if (!msg || !msg.cmd) return;

  switch (msg.cmd) {
    case 'START_MAPPING':
      state.status = 'MAPPING';
      state.timer_ms = 0;
      state.maze.grid_count = 0;
      state.maze.raw_array = ['L'];
      state.maze.optimized_array = ['L'];
      state.sensors.line = 0;
      missionActive = true;
      lastMissionTime = Date.now();
      turnIdx = 1; // Already have 'L'
      break;
    case 'START_SPEEDRUN':
      state.status = 'SOLVING';
      state.timer_ms = 0;
      missionActive = true;
      break;
    case 'ESTOP':
      state.status = 'STOPPED';
      missionActive = false;
      break;
    case 'RESET':
      state.status = 'IDLE';
      state.timer_ms = 0;
      state.maze.grid_count = 0;
      state.maze.raw_array = [];
      state.maze.optimized_array = [];
      state.sensors.line = 0;
      missionActive = false;
      turnIdx = 0;
      break;
    case 'CONFIG':
      // Config would be sent to ESP32 in real deployment
      console.log('[Config]', { pwm: msg.pwm, target_dist: msg.target_dist });
      break;

    // =============================================================================
    // HARDWARE TEST COMMANDS (Opsi 2: Live Wiring & Hardware Tester)
    // Sesuai Firmware Maze_Solver (config.h)
    // =============================================================================
    case 'TEST_ULTRASONIC_LEFT':
      console.log('[HW TEST] Ultrasonic LEFT - Trig: GPIO 19, Echo: GPIO 18');
      ws.send(JSON.stringify({ 
        type: 'test_result', 
        test: 'ultrasonic_left', 
        success: true, 
        value: Math.round((4.8 + Math.random() * 1.5) * 10) / 10,
        unit: 'cm',
        pins: { trig: 19, echo: 18 },
        message: 'Sensor Kiri OK - Jarak terbaca normal'
      }));
      break;

    case 'TEST_ULTRASONIC_FRONT':
      console.log('[HW TEST] Ultrasonic FRONT - Trig: GPIO 4, Echo: GPIO 17');
      ws.send(JSON.stringify({ 
        type: 'test_result', 
        test: 'ultrasonic_front', 
        success: true, 
        value: Math.round((18.0 + Math.random() * 12.0) * 10) / 10,
        unit: 'cm',
        pins: { trig: 4, echo: 17 },
        message: 'Sensor Depan OK - Jarak terbaca normal'
      }));
      break;

    case 'TEST_ULTRASONIC_RIGHT':
      console.log('[HW TEST] Ultrasonic RIGHT - Trig: GPIO 14, Echo: GPIO 13');
      ws.send(JSON.stringify({ 
        type: 'test_result', 
        test: 'ultrasonic_right', 
        success: true, 
        value: Math.round((22.0 + Math.random() * 5.0) * 10) / 10,
        unit: 'cm',
        pins: { trig: 14, echo: 13 },
        message: 'Sensor Kanan OK - Jarak terbaca normal'
      }));
      break;

    case 'TEST_LINE_FOLLOWER':
      console.log('[HW TEST] Line Follower TCRT5000 - Read GPIO 27');
      ws.send(JSON.stringify({ 
        type: 'test_result', 
        test: 'line_follower', 
        success: true, 
        value: Math.random() > 0.5 ? 1 : 0,
        unit: 'state',
        pins: { out: 27 },
        message: 'Line Follower OK - Logika terbaca: ' + (Math.random() > 0.5 ? 'HITAM (garis)' : 'PUTIH (bebas)')
      }));
      break;

    case 'TEST_MOTOR_LEFT':
      console.log('[HW TEST] Motor LEFT - DRV8833 AIN1=GPIO27, AIN2=GPIO26 (PWM)');
      ws.send(JSON.stringify({ 
        type: 'test_result', 
        test: 'motor_left', 
        success: true, 
        value: 180,
        unit: 'PWM',
        pins: { ain1: 27, ain2: 26 },
        message: 'Motor Kiri OK - PWM 180, arah MAJU'
      }));
      break;

    case 'TEST_MOTOR_RIGHT':
      console.log('[HW TEST] Motor RIGHT - DRV8833 BIN1=GPIO25, BIN2=GPIO33 (PWM), STBY=GPIO23');
      ws.send(JSON.stringify({ 
        type: 'test_result', 
        test: 'motor_right', 
        success: true, 
        value: 180,
        unit: 'PWM',
        pins: { bin1: 25, bin2: 33, stby: 23 },
        message: 'Motor Kanan OK - PWM 180, arah MAJU'
      }));
      break;

    case 'TEST_ALL':
      console.log('[HW TEST] FULL HARDWARE SELF-TEST');
      // Simulate sequential test with delay
      const tests = ['ultrasonic_left', 'ultrasonic_front', 'ultrasonic_right', 'line_follower', 'motor_left', 'motor_right'];
      let testIdx = 0;
      const runNext = () => {
        if (testIdx >= tests.length) {
          ws.send(JSON.stringify({ 
            type: 'test_result', 
            test: 'all_complete', 
            success: true, 
            message: 'SELURUH HARDWARE BERHASIL DITEST ✓'
          }));
          return;
        }
        const t = tests[testIdx++];
        // Send individual test command to trigger handler
        handleCommand({ cmd: 'TEST_' + t.toUpperCase() }, ws);
        setTimeout(runNext, 400);
      };
      runNext();
      break;
  }
}

// =============================================================================
// START SERVER
// =============================================================================
server.listen(PORT, HOST, () => {
  console.log('='.repeat(50));
  console.log('  MazeBot EduKit - Web Platform Server');
  console.log('  HTTP Dashboard + WebSocket Telemetry');
  console.log('='.repeat(50));
  console.log(`[HTTP] Serving dashboard at http://${HOST}:${PORT}`);
  console.log(`[WS]   WebSocket endpoint at ws://${HOST}:${PORT}/ws`);
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n[Server] Shutting down...');
  server.close(() => process.exit(0));
});