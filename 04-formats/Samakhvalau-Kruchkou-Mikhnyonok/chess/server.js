const express = require('express');
const path = require('path');
const fs = require('fs');
const os = require('os');

const app = express();
const PORT = process.env.PORT || 3000;
const ROOT = path.join(__dirname, 'games');

app.use(express.json());
app.use(express.static(__dirname));

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'index.html'));
});

app.get('/games/:gameId/moves/', (req, res) => {
  const dir = path.join(ROOT, req.params.gameId, 'moves');
  if (!fs.existsSync(dir)) {
    return res.type('text/plain').send('');
  }
  const files = fs.readdirSync(dir).filter(f => /^\d{4}\.json$/.test(f)).sort();
  res.type('text/plain').send(files.join('\n'));
});

app.get('/games/:gameId/moves/:filename', (req, res) => {
  if (!/^\d{4}\.json$/.test(req.params.filename)) {
    return res.status(400).end();
  }
  const file = path.join(ROOT, req.params.gameId, 'moves', req.params.filename);
  if (!fs.existsSync(file)) {
    return res.status(404).end();
  }
  res.sendFile(path.resolve(file));
});

app.put('/games/:gameId/moves/:filename', (req, res) => {
  if (!/^\d{4}\.json$/.test(req.params.filename)) {
    return res.status(400).end();
  }
  const dir = path.join(ROOT, req.params.gameId, 'moves');
  fs.mkdirSync(dir, { recursive: true });
  const file = path.join(dir, req.params.filename);
  let body = req.body;
  if (typeof body === 'object' && body !== null) {
    body = JSON.stringify(body);
  } else if (typeof body !== 'string') {
    body = '';
  }
  fs.writeFileSync(file, body, 'utf8');
  res.status(200).end();
});

app.get('/games/:gameId/slots.json', (req, res) => {
  const file = path.join(ROOT, req.params.gameId, 'slots.json');
  if (!fs.existsSync(file)) {
    return res.status(404).json({ white: null, black: null });
  }
  try {
    const data = JSON.parse(fs.readFileSync(file, 'utf8'));
    res.json(data);
  } catch (e) {
    res.status(404).json({ white: null, black: null });
  }
});

app.put('/games/:gameId/slots.json', (req, res) => {
  const dir = path.join(ROOT, req.params.gameId);
  fs.mkdirSync(dir, { recursive: true });
  const file = path.join(dir, 'slots.json');
  const body = typeof req.body === 'object' && req.body !== null ? req.body : { white: null, black: null };
  fs.writeFileSync(file, JSON.stringify({ white: body.white || null, black: body.black || null }), 'utf8');
  res.status(200).end();
});

app.get('/games/:gameId/state.json', (req, res) => {
  const file = path.join(ROOT, req.params.gameId, 'state.json');
  if (!fs.existsSync(file)) return res.status(404).json({});
  try {
    res.json(JSON.parse(fs.readFileSync(file, 'utf8')));
  } catch (e) {
    res.status(404).json({});
  }
});

app.put('/games/:gameId/state.json', (req, res) => {
  const dir = path.join(ROOT, req.params.gameId);
  fs.mkdirSync(dir, { recursive: true });
  const file = path.join(dir, 'state.json');
  const body = typeof req.body === 'object' && req.body !== null ? req.body : {};
  fs.writeFileSync(file, JSON.stringify({ version: Math.max(0, parseInt(body.version, 10) || 0) }), 'utf8');
  res.status(200).end();
});

app.get('/games/:gameId/undo_request.json', (req, res) => {
  const file = path.join(ROOT, req.params.gameId, 'undo_request.json');
  if (!fs.existsSync(file)) return res.json({});
  try {
    res.json(JSON.parse(fs.readFileSync(file, 'utf8')));
  } catch (e) {
    res.json({});
  }
});

app.put('/games/:gameId/undo_request.json', (req, res) => {
  const dir = path.join(ROOT, req.params.gameId);
  fs.mkdirSync(dir, { recursive: true });
  const file = path.join(dir, 'undo_request.json');
  const body = typeof req.body === 'object' && req.body !== null ? req.body : {};
  fs.writeFileSync(file, JSON.stringify({ from: body.from || null, atVersion: body.atVersion != null ? body.atVersion : null }), 'utf8');
  res.status(200).end();
});

function getLocalIP() {
  const nets = os.networkInterfaces();
  for (const name of Object.keys(nets)) {
    for (const net of nets[name]) {
      if (net.family === 'IPv4' && !net.internal) return net.address;
    }
  }
  return '?';
}

app.listen(PORT, '0.0.0.0', () => {
  const ip = getLocalIP();
  console.log(`Chess server running`);
  console.log(`  Local:   http://localhost:${PORT}`);
  console.log(`  Network: http://${ip}:${PORT}`);
  console.log(`  From another device, open: http://${ip}:${PORT}`);
});
