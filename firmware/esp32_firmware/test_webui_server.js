/**
 * Mock API server for testing ESP32 web UI without hardware.
 * Run: node test_webui_server.js
 * Open: http://localhost:8765
 */
const http = require('http');
const fs = require('fs');
const path = require('path');

const PORT = 8765;
let state = {
  connected: false,
  mode: 2,
  index: 0,
  brightness: 128,
  framerate: 50,
  sdCardPresent: false
};

function parseJsonBody(body) {
  try {
    return JSON.parse(body);
  } catch (_) {
    return {};
  }
}

function jsonResponse(res, code, obj) {
  res.writeHead(code, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(obj));
}

const routes = {
  'GET /api/status': (req, res) => {
    jsonResponse(res, 200, {
      connected: state.connected,
      mode: state.mode,
      index: state.index,
      brightness: state.brightness,
      framerate: state.framerate,
      sdCardPresent: state.sdCardPresent
    });
  },
  'POST /api/mode': (req, res, body) => {
    const d = parseJsonBody(body);
    if (d.mode !== undefined) state.mode = Math.min(4, Math.max(0, d.mode));
    if (d.index !== undefined) state.index = Math.min(17, Math.max(0, d.index));
    jsonResponse(res, 200, { status: 'ok' });
  },
  'POST /api/brightness': (req, res, body) => {
    const d = parseJsonBody(body);
    if (d.brightness !== undefined) state.brightness = Math.min(255, Math.max(0, d.brightness));
    jsonResponse(res, 200, { status: 'ok' });
  },
  'POST /api/framerate': (req, res, body) => {
    const d = parseJsonBody(body);
    if (d.framerate !== undefined) state.framerate = Math.min(120, Math.max(10, d.framerate));
    jsonResponse(res, 200, { status: 'ok' });
  },
  'POST /api/pattern': (req, res) => jsonResponse(res, 200, { status: 'ok' }),
  'POST /api/image': (req, res) => jsonResponse(res, 200, { status: 'ok' }),
  'POST /api/live': (req, res) => jsonResponse(res, 200, { status: 'ok' }),
  'GET /api/sd/list': (req, res) => jsonResponse(res, 200, { files: ['demo1.pov', 'demo2.pov'] }),
  'GET /api/sd/info': (req, res) => jsonResponse(res, 200, {
    present: state.sdCardPresent,
    totalSpace: 4 * 1024 * 1024 * 1024,
    freeSpace: 2 * 1024 * 1024 * 1024
  }),
  'POST /api/sd/delete': (req, res) => jsonResponse(res, 200, { status: 'ok' }),
  'POST /api/sd/load': (req, res) => jsonResponse(res, 200, { status: 'ok' }),
  'GET /manifest.json': (req, res) => {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ name: 'Nebula Poi Control', short_name: 'Nebula Poi' }));
  },
  'GET /sw.js': (req, res) => {
    res.writeHead(200, { 'Content-Type': 'application/javascript' });
    res.end('self.addEventListener("install",()=>{});');
  }
};

function extractHtmlFromIno() {
  const inoPath = path.join(__dirname, 'esp32_firmware.ino');
  if (!fs.existsSync(inoPath)) return null;
  const content = fs.readFileSync(inoPath, 'utf8');
  const marker = 'String html = R"rawliteral(';
  const start = content.indexOf(marker);
  if (start === -1) return null;
  const htmlStart = start + marker.length;
  const end = content.indexOf(')rawliteral";', htmlStart);
  if (end === -1) return null;
  return content.substring(htmlStart, end);
}

const server = http.createServer((req, res) => {
  const method = req.method;
  const url = req.url.split('?')[0];
  const key = `${method} ${url}`;

  if (key === 'GET /' || key === 'GET /index.html') {
    let html = null;
    try {
      html = extractHtmlFromIno();
    } catch (_) {}
    if (!html) {
      const previewPath = path.join(__dirname, '..', 'esp32_firmware', 'web_preview.html');
      if (fs.existsSync(path.join(__dirname, 'web_preview.html'))) {
        html = fs.readFileSync(path.join(__dirname, 'web_preview.html'), 'utf8');
      } else if (fs.existsSync(previewPath)) {
        html = fs.readFileSync(previewPath, 'utf8');
      }
    }
    if (html) {
      res.writeHead(200, { 'Content-Type': 'text/html' });
      res.end(html.replace('PREVIEW MODE', 'MOCK SERVER TEST'));
    } else {
      res.writeHead(404);
      res.end('Not found');
    }
    return;
  }

  const handler = routes[key];
  if (handler) {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', () => handler(req, res, body));
  } else {
    res.writeHead(404);
    res.end();
  }
});

server.listen(PORT, () => {
  console.log(`Mock web UI server: http://localhost:${PORT}`);
  console.log('Connect to test the ESP32 web interface.');
});
