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

app.get('/beagle/:id', (req, res) => {
  const file = path.join(ROOT, req.params.id + '.json');
  if (!fs.existsSync(file)) return res.status(404).json({});
  try {
    res.json(JSON.parse(fs.readFileSync(file, 'utf8')));
  } catch (e) {
    res.status(500).json({});
  }
});

function deepMerge(target, source) {
  for (const key of Object.keys(source)) {
    if (source[key] instanceof Object && !Array.isArray(source[key])) {
      if (!target[key]) target[key] = {};
      deepMerge(target[key], source[key]);
    } else {
      target[key] = source[key];
    }
  }
}

app.post('/beagle/:id', (req, res) => {
  const dir = ROOT;
  if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
  const file = path.join(dir, req.params.id + '.json');
  
  let data = {};
  if (fs.existsSync(file)) {
    try {
      data = JSON.parse(fs.readFileSync(file, 'utf8'));
    } catch (e) {}
  }
  
  if (req.body && typeof req.body === 'object') {
    deepMerge(data, req.body);
  }
  
  fs.writeFileSync(file, JSON.stringify(data, null, 2), 'utf8');
  res.json(data);
});

app.post('/beagle/:id/sync', async (req, res) => {
  const targetUrl = req.body.url;
  if (!targetUrl) return res.status(400).json({ error: 'No URL provided' });
  
  const file = path.join(ROOT, req.params.id + '.json');
  let localData = {};
  if (fs.existsSync(file)) {
    try {
      localData = JSON.parse(fs.readFileSync(file, 'utf8'));
    } catch (e) {}
  }

  try {
    const fetch = (await import('node-fetch')).default || globalThis.fetch;
    const remoteRes = await fetch(targetUrl);
    if (remoteRes.ok) {
      const remoteData = await remoteRes.json();
      deepMerge(localData, remoteData);
      if (!fs.existsSync(ROOT)) fs.mkdirSync(ROOT, { recursive: true });
      fs.writeFileSync(file, JSON.stringify(localData, null, 2), 'utf8');
      
      await fetch(targetUrl, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(localData)
      });
    }
    res.json({ success: true });
  } catch (e) {
    res.status(500).json({ error: e.message });
  }
});

function getLocalIP() {
  try {
    const nets = os.networkInterfaces();
    for (const name of Object.keys(nets)) {
      for (const net of nets[name]) {
        if (net.family === 'IPv4' && !net.internal) return net.address;
      }
    }
  } catch (e) {
    return '127.0.0.1';
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
